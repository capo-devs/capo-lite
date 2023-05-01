#pragma once
#include <chrono>
#include <string>

using namespace std::chrono_literals;

namespace capo {
///
/// \brief A duration of time, in seconds using float.
///
using Duration = std::chrono::duration<float>;

///
/// \brief Format a duration as "[H:]MM:SS".
///
std::string format_duration(Duration duration, std::string_view separator = ":");
} // namespace capo
