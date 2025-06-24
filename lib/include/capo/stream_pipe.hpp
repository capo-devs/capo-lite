#pragma once
#include <capo/stream.hpp>
#include <vector>

namespace capo {
/// \brief IStream wrapper that enables subtypes to push arbitrary number of samples.
class IStreamPipe : public IStream {
  protected:
	/// \brief Push desired number of samples at the end of out.
	/// Push nothing to indicate end of stream.
	/// \param out Buffer to push next samples into. Do not modify existing data!
	virtual void push_samples(std::vector<float>& out) = 0;

  private:
	[[nodiscard]] auto read_samples(std::span<float> out) -> std::size_t final;

	auto drain_buffer(std::span<float> out) -> std::size_t;

	std::vector<float> m_buffer{};
	std::vector<float> m_staging{};
};
} // namespace capo
