#include <capo/decode.hpp>
#include <capo/engine.hpp>
#include <capo/format.hpp>
#include <cassert>
#include <cmath>
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

	void load_and_play(char const* file_path) {
		m_source = m_engine->create_source();
		if (!m_source) { throw std::runtime_error{"Failed to create Source"}; }
		if (!m_source->open_stream(file_path)) {
			throw std::runtime_error{std::format("Failed to open audio stream: {}", file_path)};
		}

		m_duration = m_source->get_duration();
		playback(file_path);
	}

  private:
	void playback(std::string_view const path) {
		std::println("Playing '{}' ({}x{}Hz)...", path, m_pcm.channels, capo::Pcm::sample_rate_v);
		m_source->play();
		while (m_source->is_playing()) {
			print_progress();
			std::this_thread::sleep_for(100ms);
		}
		print_progress();
		std::println("\nplayback complete");
	}

	void print_progress() {
		clear_progress();
		auto const cursor = m_source->get_cursor();
		auto const progress = cursor / m_duration;
		m_progress.clear();
		m_progress += '[';
		static constexpr auto steps_v = 50;
		for (int i = 1; i <= steps_v; ++i) {
			auto const current = float(i) / float(steps_v);
			auto const ch = progress >= current ? '=' : '_';
			m_progress += ch;
		}
		m_progress += ']';
		if (m_duration > 1h) {
			std::format_to(std::back_inserter(m_progress), " {:%T} / {:%T}", cursor, m_duration);
		} else {
			std::format_to(std::back_inserter(m_progress), " {:%M:%S} / {:%M:%S}", cursor, m_duration);
		}
		std::print("{}", m_progress);
		std::fflush(stdout);
	}

	void clear_progress() {
		for (auto& c : m_progress) { c = ' '; }
		std::print("\r{}\r", m_progress);
	}

	std::unique_ptr<capo::IEngine> m_engine{};

	capo::Pcm m_pcm{};
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
		exe_name = fs::path{args.front()}.filename().string();
		args = args.subspan(1);
		if (args.empty()) {
			std::println("Usage: {} <path/to/audio/file>", exe_name);
			return EXIT_FAILURE;
		}

		auto app = example::App{};
		app.load_and_play(args.front());
	} catch (std::exception const& e) {
		std::println("PANIC: {}", e.what());
		return EXIT_FAILURE;
	} catch (...) {
		std::println("PANIC!");
		return EXIT_FAILURE;
	}
}
