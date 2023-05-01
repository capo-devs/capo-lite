#pragma once
#include <capo/channels.hpp>
#include <cstdint>

namespace capo {
///
/// \brief A single audio sample.
///
using Sample = std::int16_t;

///
/// \brief Compute the number of samples for the given frame_count and channels.
///
constexpr std::size_t sample_count_for(std::size_t frame_count, Channels channels) { return frame_count * static_cast<std::size_t>(channels); }

///
/// \brief Compute the number of frames for the given sample_count and channels.
///
constexpr std::size_t frame_count_for(std::size_t sample_count, Channels channels) {
	if (channels == Channels::eNone) { return {}; }
	return sample_count / static_cast<std::size_t>(channels);
}
} // namespace capo
