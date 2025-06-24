#pragma once
#include <capo/polymorphic.hpp>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace capo {
/// \brief Interface for custom audio stream (data source).
/// Supports streaming data of indefinite length.
/// The API is in terms of samples, they are converted to frames by the library.
class IStream : public Polymorphic {
  public:
	/// \brief Must return positive value.
	[[nodiscard]] virtual auto get_sample_rate() const -> std::uint32_t = 0;
	/// \brief Must return positive value.
	[[nodiscard]] virtual auto get_channels() const -> std::uint8_t = 0;

	/// \returns Count of samples read, 0 if at end.
	[[nodiscard]] virtual auto read_samples(std::span<float> out) -> std::size_t = 0;

	/// \param index Sample index to seek to.
	/// \returns false if seeking is not supported.
	[[nodiscard]] virtual auto seek_to_sample([[maybe_unused]] std::size_t index) -> bool { return false; }
	/// \returns nullopt if not supported.
	[[nodiscard]] virtual auto get_cursor() const -> std::optional<std::size_t> { return {}; }
	/// \returns 0 if not supported.
	[[nodiscard]] virtual auto get_sample_count() const -> std::size_t { return 0; }
};
} // namespace capo
