#pragma once
#include <capo/clip.hpp>
#include <capo/compression.hpp>
#include <capo/sample.hpp>
#include <vector>

namespace capo {
///
/// \brief Storage for decompressed audio.
///
struct Pcm {
	struct Result;

	std::vector<Sample> samples{};
	std::uint32_t sample_rate{};
	Channels channels{};

	///
	/// \brief Attempt to decompress bytes using compression into Pcm.
	///
	/// If compression is Unknown, all supported formats will be tried in succession.
	///
	static Result make(std::span<std::byte const> bytes, Compression compression = {});
	///
	/// \brief Attempt to decompress an audio file.
	///
	static Result from_file(char const* file_path);

	///
	/// \brief Obtain a clip viewing the entire range of this Pcm.
	///
	Clip clip() const { return {samples, sample_rate, channels}; }

	explicit operator bool() const { return !samples.empty() && sample_rate > 0u && channels > Channels::eNone; }
};

struct Pcm::Result {
	Pcm pcm{};
	Compression compression{};

	explicit operator bool() const { return !!pcm; }
};
} // namespace capo
