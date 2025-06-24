#include <capo/engine.hpp>
#include <capo/format.hpp>
#include <cassert>
#include <exception>
#include <filesystem>
#include <format>
#include <print>
#include <thread>

namespace fs = std::filesystem;

namespace example {
namespace {
using namespace std::chrono_literals;

class App {
  public:
	explicit App() : m_engine(capo::create_engine()) {
		if (!m_engine) { throw std::runtime_error{"Failed to create Engine"}; }
	}

	void load_and_play(char const* file_path, bool const preload) {
		m_source = m_engine->create_source();
		if (!m_source) { throw std::runtime_error{"Failed to create Source"}; }

		if (preload) {
			std::println("preloading...");
			// decode and preload PCM data into an Audio Buffer.
			load_buffer(file_path);
		} else {
			// open file stream.
			open_stream(file_path);
		}

		// store duration for later use.
		m_duration = m_source->get_duration();
		// start playback and progress reporting.
		playback();
	}

  private:
	void load_buffer(char const* file_path) {
		// decode file into Buffer.
		if (!m_buffer.decode_file(file_path)) { throw std::runtime_error{"Failed to decode into Audio Buffer"}; }
		// bind Source to Buffer.
		if (!m_source->bind_to(&m_buffer)) { throw std::runtime_error{"Failed to bind Audio Buffer to Audio Source"}; }
	}

	void open_stream(char const* file_path) {
		// open file stream.
		if (!m_source->open_file_stream(file_path)) {
			throw std::runtime_error{std::format("Failed to open audio stream: {}", file_path)};
		}
	}

	void playback() {
		m_source->play();
		// poll and print progress (cursor vs duration).
		while (m_source->is_playing()) {
			print_progress();
			std::this_thread::sleep_for(100ms);
		}
		// ensure last print is for 100% progress.
		print_progress();
		std::println("\nplayback complete");
	}

	// not particularly relevant to capo, just window dressing.
	void print_progress() {
		// clear current line.
		clear_progress();
		auto const cursor = m_source->get_cursor();
		// compute progress.
		auto const progress = cursor / m_duration;
		// reset text buffer.
		m_progress.clear();
		// append progress bar.
		m_progress += '[';
		static constexpr auto steps_v = 50;
		for (int i = 1; i <= steps_v; ++i) {
			auto const current = float(i) / float(steps_v);
			auto const ch = progress >= current ? '=' : '_';
			m_progress += ch;
		}
		m_progress += ']';
		// append formatted cursor and duration.
		if (m_duration > 1h) {
			std::format_to(std::back_inserter(m_progress), " {:%T} / {:%T}", cursor, m_duration);
		} else {
			std::format_to(std::back_inserter(m_progress), " {:%M:%S} / {:%M:%S}", cursor, m_duration);
		}
		// print line and flush.
		std::print("{}", m_progress);
		std::fflush(stdout);
	}

	void clear_progress() {
		// fill existing buffer with spaces to clear out previously printed line.
		for (auto& c : m_progress) { c = ' '; }
		std::print("\r{}\r", m_progress);
	}

	std::unique_ptr<capo::IEngine> m_engine{};

	capo::Buffer m_buffer{};
	std::unique_ptr<capo::ISource> m_source{};
	std::chrono::duration<float> m_duration{};

	std::string m_progress{};
};
} // namespace
} // namespace example

auto main(int argc, char** argv) -> int {
	try {
		auto args = std::span{argv, std::size_t(argc)};
		auto exe_name = std::string{"<exe>"};
		assert(!args.empty());
		exe_name = fs::path{args.front()}.stem().string();

		// [--preload] <path>
		args = args.subspan(1);
		auto preload = false;
		// parse flags first.
		for (; !args.empty(); args = args.subspan(1)) {
			auto flag = std::string_view{args.front()};
			// flags ended.
			if (!flag.starts_with("--")) { break; }
			// consume preload flag.
			if (flag == "--preload") {
				preload = true;
				continue;
			}

			std::println(stderr, "Unrecognized flag: '{}'", flag);
			return EXIT_FAILURE;
		}

		if (args.empty()) {
			// missing file path.
			std::println("Usage: {} [--preload] <path/to/audio/file>", exe_name);
			return EXIT_FAILURE;
		}

		auto app = example::App{};
		// ignore args beyond 0th.
		app.load_and_play(args.front(), preload);
	} catch (std::exception const& e) {
		std::println("PANIC: {}", e.what());
		return EXIT_FAILURE;
	} catch (...) {
		std::println("PANIC!");
		return EXIT_FAILURE;
	}
}
