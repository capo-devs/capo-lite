#pragma once
#include <chrono>
#include <string>

namespace capo {
void format_duration_to(std::string& out, std::chrono::duration<float> dt);
[[nodiscard]] auto format_duration(std::chrono::duration<float> dt) -> std::string;

void format_bytes_to(std::string& out, std::uint64_t bytes);
[[nodiscard]] auto format_bytes(std::uint64_t bytes) -> std::string;
} // namespace capo
