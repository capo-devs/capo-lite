#pragma once
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace capo {
enum class Encoding : std::int8_t { Wav, Mp3, Flac };

class Buffer {
  public:
	static constexpr auto sample_rate_v = 48000u;

	[[nodiscard]] auto get_samples() const -> std::span<float const> { return m_samples; }
	[[nodiscard]] auto get_channels() const -> std::uint8_t { return m_channels; }

	[[nodiscard]] auto get_frame_count() const -> std::uint64_t {
		if (m_samples.empty() || m_channels == 0) { return 0; }
		return m_samples.size() / m_channels;
	}

	[[nodiscard]] auto is_loaded() const -> bool { return m_channels > 0 && !m_samples.empty(); }

	void set_frames(std::vector<float> samples, std::uint8_t channels);
	[[nodiscard]] auto decode_bytes(std::span<std::byte const> bytes, std::optional<Encoding> encoding = {}) -> bool;
	[[nodiscard]] auto decode_file(char const* path, std::optional<Encoding> encoding = {}) -> bool;

  private:
	std::vector<float> m_samples{};
	std::uint8_t m_channels{};
};

[[nodiscard]] auto guess_encoding(char const* path) -> std::optional<Encoding>;
[[nodiscard]] auto file_to_bytes(char const* path) -> std::vector<std::byte>;
} // namespace capo
