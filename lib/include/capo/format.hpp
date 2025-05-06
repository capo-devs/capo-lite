#pragma once
#include <chrono>
#include <string>

namespace capo {
/// \brief Format duration as [HH:]MM:SS.
/// \param out Output string.
/// \param dt Duration.
void format_duration_to(std::string& out, std::chrono::duration<float> dt);

/// \brief Format duration as [HH:]MM:SS.
/// \param dt Duration.
/// \returns Formatted string.
[[nodiscard]] auto format_duration(std::chrono::duration<float> dt) -> std::string;

/// \brief Format bytes as human-readable, eg '924.5MiB'.
/// \param out Output string.
/// \param bytes Byte count.
void format_bytes_to(std::string& out, std::uint64_t bytes);

/// \brief Format bytes as human-readable, eg '924.5MiB'.
/// \param bytes Byte count.
/// \returns Formatted string.
[[nodiscard]] auto format_bytes(std::uint64_t bytes) -> std::string;
} // namespace capo
