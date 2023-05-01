#include <dr_libs/dr_flac.h>
#include <dr_libs/dr_mp3.h>
#include <dr_libs/dr_wav.h>
#include <capo/pcm.hpp>
#include <detail/unique.hpp>
#include <filesystem>
#include <fstream>
#include <variant>

namespace capo {
namespace {
template <typename... T>
struct Visitor : T... {
	using T::operator()...;
};

template <typename... T>
Visitor(T...) -> Visitor<T...>;

template <typename T>
using Ptr = T*;

struct Decoder {
	using Payload = std::variant<drwav, drmp3, drflac*>;
	struct Deleter {
		void operator()(Payload&& payload) const {
			auto const visitor = Visitor{
				[](drwav& wav) { drwav_uninit(&wav); },
				[](drmp3& mp3) { drmp3_uninit(&mp3); },
				[](drflac* flac) { drflac_close(flac); },
			};
			std::visit(visitor, payload);
		}
	};

	Unique<Payload, Deleter> payload{Payload{}};

	Compression try_initialize(std::span<const std::byte> bytes, Compression compression) {
		auto const visitor = Visitor{
			[bytes](drwav& wav) { return drwav_init_memory(&wav, bytes.data(), bytes.size(), {}) == DRWAV_TRUE; },
			[bytes](drmp3& mp3) { return drmp3_init_memory(&mp3, bytes.data(), bytes.size(), {}) == DRWAV_TRUE; },
			[bytes](Ptr<drflac>& flac) {
				flac = drflac_open_memory(bytes.data(), bytes.size(), {});
				return flac != nullptr;
			},
		};
		switch (compression) {
		case Compression::eWav: payload.get() = drwav{}; break;
		case Compression::eMp3: payload.get() = drmp3{}; break;
		case Compression::eFlac: payload.get() = Ptr<drflac>{}; break;
		default: return Compression::eUnknown;
		}
		if (!std::visit(visitor, payload.get())) { return {}; }
		return compression;
	}

	Compression initialize(std::span<const std::byte> bytes, Compression compression) {
		if (compression == Compression::eUnknown) {
			for (auto ret = Compression(int(1)); ret < Compression::eCOUNT_; ret = Compression(int(ret) + 1)) {
				if (try_initialize(bytes, compression) != Compression::eUnknown) { return compression; }
			}
			return Compression::eUnknown;
		}
		return try_initialize(bytes, compression);
	}

	std::size_t get_frame_count() {
		auto const visitor = Visitor{
			[](std::monostate) { return drwav_uint64{}; },
			[](drwav& wav) { return wav.totalPCMFrameCount; },
			[](drmp3& mp3) { return drmp3_get_pcm_frame_count(&mp3); },
			[](Ptr<drflac>& flac) { return flac->totalPCMFrameCount; },
		};
		return std::visit(visitor, payload.get());
	}

	Channels get_channels() const {
		auto const visitor = Visitor{
			[](auto const& obj) { return static_cast<Channels>(obj.channels); },
			[](Ptr<drflac> const obj) { return static_cast<Channels>(obj->channels); },
		};
		return std::visit(visitor, payload.get());
	}

	std::uint32_t get_sample_rate() const {
		auto const visitor = Visitor{
			[](auto const& obj) { return static_cast<std::uint32_t>(obj.sampleRate); },
			[](Ptr<drflac> const flac) { return flac ? static_cast<std::uint32_t>(flac->sampleRate) : 0u; },
		};
		return std::visit(visitor, payload.get());
	}

	std::span<Sample> read_samples(std::span<Sample> out, Channels channels) {
		auto const frame_count = frame_count_for(out.size(), channels);
		auto const visitor = Visitor{
			[](std::monostate) { return drwav_uint64{}; },
			[&](drwav& wav) { return drwav_read_pcm_frames_s16(&wav, static_cast<drwav_uint64>(frame_count), out.data()); },
			[&](drmp3& mp3) { return drmp3_read_pcm_frames_s16(&mp3, static_cast<drwav_uint64>(frame_count), out.data()); },
			[&](Ptr<drflac>& flac) { return drflac_read_pcm_frames_s16(flac, static_cast<drwav_uint64>(frame_count), out.data()); },
		};
		auto const ret = std::visit(visitor, payload.get());
		return out.subspan(0u, sample_count_for(static_cast<std::size_t>(ret), channels));
	}

	Pcm::Result operator()(std::span<std::byte const> bytes, Compression compression) {
		compression = initialize(bytes, compression);
		if (compression == Compression::eUnknown) { return {}; }
		auto const frame_count = get_frame_count();
		auto ret = Pcm{};
		ret.channels = get_channels();
		ret.sample_rate = get_sample_rate();
		ret.samples = std::vector<Sample>(sample_count_for(frame_count, ret.channels));
		auto read = read_samples(ret.samples, ret.channels);
		if (read.size() != ret.samples.size()) { return {}; }
		return {std::move(ret), compression};
	}
};
} // namespace

auto Pcm::make(std::span<std::byte const> bytes, Compression compression) -> Result { return Decoder{}(bytes, compression); }

auto Pcm::from_file(char const* file_path) -> Result {
	auto file = std::ifstream{file_path, std::ios::binary | std::ios::ate};
	if (!file) { return {}; }
	auto const ssize = file.tellg();
	file.seekg({}, std::ios::beg);
	auto bytes = std::vector<std::byte>(static_cast<std::size_t>(ssize));
	file.read(reinterpret_cast<char*>(bytes.data()), ssize);
	auto const compression = to_compression(std::filesystem::path{file_path}.extension().string());
	return make(bytes, compression);
}
} // namespace capo
