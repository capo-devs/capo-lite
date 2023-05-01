#include <capo/capo.hpp>
#include <filesystem>
#include <functional>
#include <iostream>
#include <ranges>
#include <string>

namespace {
struct FormatF {
	std::array<char, 8> buffer{};

	FormatF(char const* fmt, float value) { std::snprintf(buffer.data(), buffer.size(), fmt, value); }

	friend std::ostream& operator<<(std::ostream& out, FormatF const& f) { return out << f.buffer.data(); }
};

struct App {
	capo::Device device{};
	capo::StreamSource source{};

	std::string name{};
	capo::Duration duration{};
	capo::Channels channels{};
	capo::Pcm pcm{};
	std::uint32_t sample_rate{};
	bool running{true};

	struct Cmd {
		std::string id{};
		std::function<void(std::stringstream)> func{};
	};

	std::vector<Cmd> cmds{};

	template <typename T>
	static T next(std::stringstream& out, T fallback = {}) {
		auto ret = T{};
		if (out >> ret) { return ret; }
		return fallback;
	}

	static std::string format_size(float bytes) {
		auto str = std::stringstream{};
		if (bytes < 1024.0f) {
			str << static_cast<int>(bytes) << "B";
			return str.str();
		}
		bytes /= 1024.0f;
		static constexpr std::array units_v = {"KiB", "MiB", "GiB"};
		for (auto const unit : units_v) {
			if (bytes < 1024.0f) {
				str << FormatF{"%.2f", bytes} << unit;
				break;
			}
			bytes /= 1024.0f;
		}
		auto ret = str.str();
		if (ret.empty()) {
			str << FormatF{"%.2f", bytes} << units_v.back();
			ret = str.str();
		}
		return ret;
	}

	bool load(char const* path) {
		auto const start = std::chrono::steady_clock::now();
		auto result = capo::Pcm::from_file(path);
		if (!result.pcm) {
			std::cerr << "Couldn't load audio file.\n";
			return {};
		}
		auto const dt = capo::Duration{std::chrono::steady_clock::now() - start}.count() * 1000.0f;
		auto const size = std::span{result.pcm.samples}.size_bytes();
		std::cout << "Decompressed " << format_size(static_cast<float>(size)) << " in " << FormatF{"%0.1f", dt} << "ms\n";

		name = std::filesystem::path{path}.filename().string();
		duration = result.pcm.clip().duration();
		channels = result.pcm.channels;
		sample_rate = result.pcm.sample_rate;

		pcm = std::move(result.pcm);
		source.set_stream(pcm.clip());

		return true;
	}

	bool run(char const* path) {
		if (!device) {
			std::cerr << "Couldn't create valid device.\n";
			return false;
		}

		source = device.make_stream_source();
		if (!load(path)) { return false; }

		setup_cmds();

		std::cout << "\n";
		auto line = std::string{};
		while (running) {
			print_display();
			std::cout << "> ";
			std::getline(std::cin, line);
			execute(std::stringstream{std::move(line)});
			std::cout << "\n";
		}

		return true;
	}

	void setup_cmds() {
		cmds.push_back({"play", [&](std::stringstream) { source.play(); }});
		cmds.push_back({"stop", [&](std::stringstream) { source.stop(); }});
		cmds.push_back({"pause", [&](std::stringstream) { source.pause(); }});
		cmds.push_back({"rewind", [&](std::stringstream) { source.rewind(); }});

		cmds.push_back({"gain", [&](std::stringstream str) {
							auto value = next<float>(str, source.gain());
							if (value >= 0.0f && value <= 1.0f) { source.set_gain(value); };
						}});
		cmds.push_back({"seek", [&](std::stringstream str) {
							auto value = next<float>(str, source.cursor().count());
							if (value >= 0.0f && value <= duration.count()) { source.seek(capo::Duration{value}); };
						}});
		cmds.push_back({"load", [&](std::stringstream str) {
							auto path = std::string{};
							str >> path;
							load(path.c_str());
						}});
		cmds.push_back({"quit", [&](std::stringstream) { running = false; }});
	}

	void print_display() const {
		static constexpr std::string_view state_v[] = {"Unknown", "Idle", "Playing", "Paused", "Stopped"};
		std::cout << name << "\t";
		std::cout << "[" << capo::format_duration(source.cursor()) << " / " << capo::format_duration(duration) << "]\t";
		std::cout << "[" << state_v[static_cast<std::size_t>(source.state())] << "]\t";
		std::cout << "[" << FormatF{"%.1f", source.gain()} << "]\t";
		std::cout << "\n";

		for (auto const& [id, _] : cmds) { std::cout << "<" << id << "> "; }
		std::cout << "\n";
	}

	void execute(std::stringstream line) {
		auto cmd = std::string{};
		line >> cmd;
		auto it = std::ranges::find_if(cmds, [&cmd](auto const& c) { return c.id == cmd; });
		if (it != cmds.end()) { it->func(std::move(line)); }
	}
};
} // namespace

int main(int argc, char** argv) {
	if (argc < 2) {
		std::cerr << "Syntax: " << argv[0] << " <audio file path>" << std::endl;
		return EXIT_FAILURE;
	}
	try {
		if (!App{}.run(argv[1])) { return EXIT_FAILURE; }
	} catch (std::exception const& e) {
		std::cerr << "Fatal error: " << e.what() << "\n";
		return EXIT_FAILURE;
	}
}
