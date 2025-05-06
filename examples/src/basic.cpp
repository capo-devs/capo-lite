#include <capo/engine.hpp>
#include <cmath>
#include <exception>
#include <print>

namespace example {
namespace {
using namespace std::chrono_literals;

[[nodiscard]] auto create_sine_wave_samples(std::chrono::duration<float> const duration) {
	// frame storage (interleaved samples, 1xframe = 1xfloat * channels).
	auto ret = std::vector<float>{};
	auto get_duration = [&ret] {
		return std::chrono::duration<float>{float(ret.size()) / float(capo::Buffer::sample_rate_v)};
	};
	// push enough samples for duration.
	for (auto rad = 0.0f; get_duration() < duration; rad += 0.05f) { ret.push_back(0.2f * std::sin(rad)); }
	return ret;
}

void run() {
	// create an Engine instance.
	auto engine = capo::create_engine();
	// fatal error if no Engine.
	if (!engine) { throw std::runtime_error{"Failed to create Engine"}; }

	// create an Audio Source.
	auto source = engine->create_source();
	// fatal error if no Source.
	if (!source) { throw std::runtime_error{"Failed to create Source"}; }

	// target duration.
	static constexpr auto duration_v = 3s;
	// create an Audio Buffer.
	auto buffer = capo::Buffer{};
	// manually set PCM frames.
	buffer.set_frames(create_sine_wave_samples(duration_v), 1);
	// bind Buffer to Source. fatal error if not bound.
	if (!source->bind_to(&buffer)) { throw std::runtime_error{"Failed to bind Source to Pcm"}; }

	std::println("Playing sine wave ({}x{}Hz) for {}s...", buffer.get_channels(), capo::Buffer::sample_rate_v,
				 duration_v.count());

	// fade in to hide click (this is not real audio data, it abruptly hits max amplitude immediately).
	source->set_fade_in(0.5s);
	// start playback.
	source->play();
	// wait until playback has ended.
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
