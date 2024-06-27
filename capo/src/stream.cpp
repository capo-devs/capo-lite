#include <capo/stream.hpp>
#include <cassert>

namespace capo {
Clip Stream::read(std::span<Sample> out) {
	if (!m_clip || m_next_sample >= m_clip.samples.size()) { return {}; }
	auto const sample_count = std::min(out.size(), m_clip.samples.size() - m_next_sample);
	for (std::size_t i = 0; i < sample_count; ++i) {
		assert(m_next_sample < m_clip.samples.size());
		out[i] = m_clip.samples[m_next_sample++];
	}
	return Clip{
		.samples = out.subspan(0u, sample_count),
		.sample_rate = m_clip.sample_rate,
		.channels = m_clip.channels,
	};
}

FrameIndex Stream::next_frame_index() const {
	if (!m_clip) { return {}; }
	return m_next_sample / static_cast<std::size_t>(m_clip.channels);
}

bool Stream::seek_to_frame(FrameIndex frame_index) {
	if (frame_index >= m_frame_count) { return {}; }
	m_next_sample = frame_index * static_cast<std::size_t>(m_clip.channels);
	return true;
}
} // namespace capo
