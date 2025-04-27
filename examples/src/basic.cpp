#include <capo/engine.hpp>
#include <cmath>
#include <exception>
#include <print>

namespace example {
namespace {
using namespace std::chrono_literals;

[[nodiscard]] auto create_sine_wave_samples(std::chrono::duration<float> const duration) {
	auto ret = std::vector<float>{};
	auto get_duration = [&ret] {
		return std::chrono::duration<float>{float(ret.size()) / float(capo::Pcm::sample_rate_v)};
	};
	for (auto rad = 0.0f; get_duration() < duration; rad += 0.05f) { ret.push_back(0.2f * std::sin(rad)); }
	return ret;
}

void run() {
	auto engine = capo::create_engine();
	if (!engine) { throw std::runtime_error{"Failed to create Engine"}; }

	auto source = engine->create_source();
	if (!source) { throw std::runtime_error{"Failed to create Source"}; }

	static constexpr auto duration_v = 3s;
	auto pcm = capo::Pcm{.samples = create_sine_wave_samples(duration_v), .channels = 1};
	if (!source->bind_to(&pcm)) { throw std::runtime_error{"Failed to bind Source to Pcm"}; }

	std::println("Playing sine wave ({}x{}Hz) for {}s...", pcm.channels, capo::Pcm::sample_rate_v, duration_v.count());

	source->set_fade_in(0.5s);
	source->play();
	source->wait_until_ended();
}
} // namespace
} // namespace example

auto main() -> int {
	try {
		example::run();
	} catch (std::exception const& e) {
		std::println("PANIC: {}", e.what());
		return EXIT_FAILURE;
	} catch (...) {
		std::println("PANIC!");
		return EXIT_FAILURE;
	}
}
