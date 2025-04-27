#pragma once
#include <cstdint>
#include <vector>

namespace capo {
struct Pcm {
	static constexpr auto sample_rate_v = 48000u;

	[[nodiscard]] auto get_frame_count() const -> std::uint64_t {
		if (samples.empty() || channels == 0) { return 0; }
		return samples.size() / channels;
	}

	[[nodiscard]] auto is_loaded() const -> bool { return channels > 0 && !samples.empty(); }

	std::vector<float> samples{};
	std::uint8_t channels{};
};

auto run_test(char const* path) -> int;
} // namespace capo
