#pragma once
#include <cstdint>

namespace capo {
///
/// \brief Number of channels.
///
enum class Channels : std::uint8_t {
	eNone = 0,
	eMono = 1,
	eStereo = 2,
};
} // namespace capo
