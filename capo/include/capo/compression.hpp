#pragma once
#include <string_view>

namespace capo {
///
/// \brief Compression format.
///
enum class Compression : std::uint8_t { eUnknown, eWav, eMp3, eFlac, eCOUNT_ };

///
/// \brief Parse a file extension into its corresponding Compression format.
///
constexpr Compression to_compression(std::string_view file_extension) {
	if (file_extension == ".wav") { return Compression::eWav; }
	if (file_extension == ".flac") { return Compression::eFlac; }
	if (file_extension == ".mp3") { return Compression::eMp3; }
	return Compression::eUnknown;
}
} // namespace capo
