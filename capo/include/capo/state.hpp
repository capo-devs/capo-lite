#pragma once
#include <cstdint>

namespace capo {
///
/// \brief The state of an audio source.
///
enum class State : std::uint8_t { eUnknown, eIdle, ePlaying, ePaused, eStopped };
} // namespace capo
