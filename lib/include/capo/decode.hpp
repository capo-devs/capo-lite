#pragma once
#include <capo/pcm.hpp>
#include <cstddef>
#include <optional>
#include <span>

namespace capo {
enum class Encoding : std::int8_t { Wav, Mp3, Flac };

auto decode_bytes(std::span<std::byte const> bytes, std::optional<Encoding> encoding = {}) -> Pcm;
auto decode_file(char const* path, std::optional<Encoding> encoding = {}) -> Pcm;
} // namespace capo
