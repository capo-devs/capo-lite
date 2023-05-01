#pragma once
#include <capo/channels.hpp>
#include <capo/duration.hpp>
#include <capo/sample.hpp>
#include <span>

namespace capo {
///
/// \brief Lightweight view of a decompressed audio clip.
///
/// Clips are views into existing sample storage, thus the storage must outlive all clips viewing into it.
///
struct Clip {
	std::span<Sample const> samples{};
	std::uint32_t sample_rate{};
	Channels channels{};

	///
	/// \brief Obtain the number of frames in this clip.
	///
	constexpr std::size_t frame_count() const { return channels == Channels::eNone ? 0u : samples.size() / static_cast<std::size_t>(channels); }

	///
	/// \brief Obtain the total duration of this clip.
	///
	constexpr Duration duration() const {
		if (samples.empty()) { return {}; }
		return Duration{static_cast<float>(samples.size()) / static_cast<float>(sample_rate * static_cast<std::uint32_t>(channels))};
	}

	explicit constexpr operator bool() const { return !samples.empty() && sample_rate > 0u && channels > Channels::eNone; }
};
} // namespace capo
