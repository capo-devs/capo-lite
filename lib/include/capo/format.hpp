#pragma once
#include <chrono>
#include <string>

namespace capo {
[[nodiscard]] auto format_duration(std::chrono::duration<float> dt) -> std::string;
[[nodiscard]] auto format_bytes(std::uint64_t bytes) -> std::string;
} // namespace capo
