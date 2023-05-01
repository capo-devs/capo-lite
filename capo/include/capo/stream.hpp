#pragma once
#include <capo/clip.hpp>
#include <capo/duration.hpp>
#include <capo/frame_index.hpp>
#include <memory>

namespace capo {
///
/// \brief Stateful sreaming helper.
///
/// Streams are views into existing sample storage, thus the storage must outlive all streams viewing into it.
///
class Stream {
  public:
	Stream() = default;
	Stream(Clip clip) : m_clip(clip), m_duration(clip.duration()), m_frame_count(clip.frame_count()) {}

	Clip clip() const { return m_clip; }
	Duration duration() const { return m_duration; }
	std::size_t frame_count() const { return m_frame_count; }
	Clip read(std::span<Sample> out);
	FrameIndex next_frame_index() const;

	bool at_end() const { return m_next_sample >= m_clip.samples.size(); }
	bool seek_to_frame(FrameIndex frame_index);

	explicit operator bool() const { return !!m_clip; }

  private:
	Clip m_clip{};
	Duration m_duration{};
	std::size_t m_frame_count{};
	FrameIndex m_next_sample{};
};
} // namespace capo
