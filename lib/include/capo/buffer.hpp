#pragma once
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace capo {
/// \brief Format of encoded data.
enum class Encoding : std::int8_t { Wav, Mp3, Flac };

/// \brief Audio Buffer: stores decoded PCM data in memory.
class Buffer {
  public:
	/// \brief Sample rate is constant (48kHz).
	static constexpr auto sample_rate_v = 48000u;

	[[nodiscard]] auto get_samples() const -> std::span<float const> { return m_samples; }
	[[nodiscard]] auto get_channels() const -> std::uint8_t { return m_channels; }

	[[nodiscard]] auto get_frame_count() const -> std::uint64_t {
		if (m_samples.empty() || m_channels == 0) { return 0; }
		return m_samples.size() / m_channels;
	}

	[[nodiscard]] auto is_loaded() const -> bool { return m_channels > 0 && !m_samples.empty(); }

	/// \brief Set custom PCM data.
	void set_frames(std::vector<float> samples, std::uint8_t channels);

	/// \brief Decode bytes in memory.
	/// \param bytes Encoded bytes.
	/// \param encoding Encoding format, if known.
	/// \returns true on success.
	[[nodiscard]] auto decode_bytes(std::span<std::byte const> bytes, std::optional<Encoding> encoding = {}) -> bool;

	/// \brief Decode an audio file.
	/// \param path Path to audio file.
	/// \param encoding Encoding format, if known.
	/// \returns true on success.
	[[nodiscard]] auto decode_file(char const* path, std::optional<Encoding> encoding = {}) -> bool;

  private:
	std::vector<float> m_samples{};
	std::uint8_t m_channels{};
};

/// \brief Guess the Encoding format based on the file extension.
[[nodiscard]] auto guess_encoding(std::string_view path) -> std::optional<Encoding>;

/// \brief Load file data as a binary byte array.
[[nodiscard]] auto file_to_bytes(char const* path) -> std::vector<std::byte>;
} // namespace capo
