#include <miniaudio.h>
#include <capo/decode.hpp>
#include <capo/engine.hpp>
#include <capo/format.hpp>
#include <algorithm>
#include <atomic>
#include <bit>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <list>
#include <memory>
#include <optional>
#include <span>
#include <vector>

#include <print>

using namespace std::chrono_literals;

namespace fs = std::filesystem;

namespace capo {
namespace {
constexpr auto to_ma_encoding(std::optional<Encoding> const encoding) -> ma_encoding_format {
	if (!encoding) { return ma_encoding_format_unknown; }
	switch (*encoding) {
	case Encoding::Wav: return ma_encoding_format_wav;
	case Encoding::Mp3: return ma_encoding_format_mp3;
	case Encoding::Flac: return ma_encoding_format_flac;
	default: return ma_encoding_format_unknown;
	}
}

constexpr auto guess_encoding(std::string_view const extension) -> std::optional<Encoding> {
	if (extension == ".wav") { return Encoding::Wav; }
	if (extension == ".mp3") { return Encoding::Mp3; }
	if (extension == ".flac") { return Encoding::Flac; }
	return {};
}

// exponentially growing deque: [=] -> [==] -> [====] ...
// builds a vector of samples (floats) of unknown initial size.
// setup by pushing a chain of chunks of exponentially increasing size.
// this is slightly better than resizing AND copying all existing elements,
// as would happen with vector + push_back.
class Framebuffer {
  public:
	explicit Framebuffer(std::size_t const first_chunk_size = 1024 * 1024uz) : m_next_chunk_size(first_chunk_size) {}

	[[nodiscard]] auto acquire() const -> std::vector<float> {
		auto ret = std::vector<float>{};
		ret.resize(m_next_chunk_size);
		return ret;
	}

	void release(std::vector<float>&& chunk) {
		m_next_chunk_size *= 2;
		m_total_size += chunk.size();
		m_chunks.push_back(std::move(chunk));
	}

	[[nodiscard]] auto build() const {
		auto ret = std::vector<float>{};
		if (m_chunks.empty()) { return ret; }

		ret.resize(m_total_size);
		auto dst = std::span{ret};
		for (std::span<float const> const src : m_chunks) {
			assert(dst.size() >= src.size());
			std::memcpy(dst.data(), src.data(), src.size_bytes());
			dst = dst.subspan(src.size());
		}

		return ret;
	}

  private:
	std::list<std::vector<float>> m_chunks{};
	std::size_t m_next_chunk_size{};
	std::size_t m_total_size{};
};

class Decoder : public ma_decoder {
  public:
	Decoder(Decoder const&) = delete;
	Decoder(Decoder&&) = delete;
	auto operator=(Decoder const&) -> Decoder& = delete;
	auto operator=(Decoder&&) -> Decoder& = delete;

	explicit Decoder(std::span<std::byte const> bytes, std::optional<Encoding> const encoding) : ma_decoder({}) {
		auto config = ma_decoder_config_init(ma_format_f32, 0, Pcm::sample_rate_v);
		config.encodingFormat = to_ma_encoding(encoding);
		auto result = ma_decoder_init_memory(bytes.data(), bytes.size(), &config, this);
		if (result != MA_SUCCESS) {
			failed = true;
			return;
		}

		result = ma_decoder_get_data_format(this, nullptr, &config.channels, nullptr, nullptr, 0);
		if (result != MA_SUCCESS) {
			failed = true;
			return;
		}

		m_channels = std::uint8_t(config.channels);
		m_reserve = bytes.size();
	}

	~Decoder() { ma_decoder_uninit(this); }

	[[nodiscard]] auto decode(Pcm& out) -> bool {
		out = Pcm{.channels = m_channels};
		out.samples.reserve(m_reserve);
		auto framebuffer = Framebuffer{};
		while (true) {
			auto buffer = framebuffer.acquire();
			assert(!buffer.empty());
			auto const frame_count = buffer.size() / m_channels;
			auto frames_read = ma_uint64{};
			auto const result = ma_decoder_read_pcm_frames(this, buffer.data(), frame_count, &frames_read);
			if (result != MA_SUCCESS || frames_read == 0) { return false; }
			buffer.resize(frames_read * m_channels);
			framebuffer.release(std::move(buffer));
			if (frames_read < frame_count) { break; }
		}
		out.samples = framebuffer.build();
		return true;
	}

	[[nodiscard]] auto get_channels() const -> std::uint8_t { return m_channels; }

	bool failed{};

  private:
	std::uint8_t m_channels{};
	std::size_t m_reserve{};
};

class Sound : public ma_sound {
  public:
	Sound(Sound const&) = delete;
	Sound(Sound&&) = delete;
	auto operator=(Sound const&) -> Sound& = delete;
	auto operator=(Sound&&) -> Sound& = delete;

	explicit Sound(ma_engine& engine, Pcm const& pcm) : ma_sound({}) {
		auto config = ma_audio_buffer_config_init(ma_format_f32, pcm.channels, pcm.get_frame_count(),
												  pcm.samples.data(), nullptr);
		config.sampleRate = Pcm::sample_rate_v;
		auto result = ma_audio_buffer_init(&config, &m_buffer);
		if (result != MA_SUCCESS) {
			failed = true;
			return;
		}

		result = ma_sound_init_from_data_source(&engine, &m_buffer, 0, nullptr, this);
		if (result != MA_SUCCESS) {
			ma_audio_buffer_uninit(&m_buffer);
			failed = true;
		}
	}

	~Sound() {
		if (failed) { return; }
		ma_sound_stop(this);
		ma_sound_uninit(this);
		ma_audio_buffer_uninit(&m_buffer);
	}

	bool failed{};

  private:
	ma_audio_buffer m_buffer{};
};

class Source : public ISource {
  public:
	explicit Source(ma_engine& engine) : m_engine(&engine) {}

	[[nodiscard]] auto is_bound() const -> bool final { return m_sound != nullptr; }

	auto bind_to(Pcm const* target) -> bool final {
		if (target == nullptr) { return false; }
		if (m_sound && is_playing()) { stop(); }
		auto sound = std::make_unique<Sound>(*m_engine, *target);
		if (sound->failed) { return false; }

		m_sound = std::move(sound);
		set_cursor(0s);
		static auto const callback = +[](void* self, ma_sound* /*sound*/) { static_cast<Source*>(self)->on_end(); };
		ma_sound_set_end_callback(m_sound.get(), callback, this);
		return true;
	}

	auto bind_to(std::shared_ptr<Pcm const> target) -> bool final {
		if (!bind_to(target.get())) { return false; }
		m_pcm = std::move(target);
		return true;
	}

	void unbind() final { m_sound.reset(); }

	[[nodiscard]] auto is_playing() const -> bool final {
		if (!is_bound()) { return false; }
		return ma_sound_is_playing(m_sound.get()) == MA_TRUE;
	}

	void play() final {
		if (!is_bound()) { return; }
		m_ended.store(false);
		ma_sound_start(m_sound.get());
	}

	void stop() final {
		if (!is_bound()) { return; }
		ma_sound_stop(m_sound.get());
	}

	[[nodiscard]] auto at_end() const -> bool final {
		if (!is_bound()) { return true; }
		return ma_sound_at_end(m_sound.get()) == MA_TRUE;
	}

	[[nodiscard]] auto can_wait_until_ended() const -> bool final { return !is_looping() && is_playing(); }

	void wait_until_ended() final {
		if (!can_wait_until_ended()) { return; }
		m_ended.wait(false);
	}

	[[nodiscard]] auto get_duration() const -> std::chrono::duration<float> final {
		if (!is_bound()) { return -1s; }
		return get_float(&ma_sound_get_length_in_seconds);
	}

	[[nodiscard]] auto get_cursor() const -> std::chrono::duration<float> final {
		if (!is_bound()) { return -1s; }
		return get_float(&ma_sound_get_cursor_in_seconds);
	}

	void set_cursor(std::chrono::duration<float> const position) final {
		if (!is_bound()) { return; }
		ma_sound_seek_to_second(m_sound.get(), position.count());
	}

	[[nodiscard]] auto is_looping() const -> bool final {
		if (!is_bound()) { return false; }
		return ma_sound_is_looping(m_sound.get()) == MA_TRUE;
	}

	void set_looping(bool const looping) final {
		if (!is_bound()) { return; }
		ma_sound_set_looping(m_sound.get(), looping ? MA_TRUE : MA_FALSE);
	}

	[[nodiscard]] auto get_gain() const -> float final {
		if (!is_bound()) { return -1.0f; }
		return ma_sound_get_volume(m_sound.get());
	}

	void set_gain(float const gain) final {
		if (!is_bound()) { return; }
		ma_sound_set_volume(m_sound.get(), std::clamp(gain, 0.0f, 1.0f));
	}

	[[nodiscard]] auto is_spatialized() const -> bool final {
		if (!is_bound()) { return false; }
		return ma_sound_is_spatialization_enabled(m_sound.get()) == MA_TRUE;
	}

	void set_spatialized(bool const spatialized) final {
		if (!is_bound()) { return; }
		ma_sound_set_spatialization_enabled(m_sound.get(), spatialized ? MA_TRUE : MA_FALSE);
	}

	[[nodiscard]] auto get_position() const -> Vec3f final {
		if (!is_bound()) { return {}; }
		return std::bit_cast<Vec3f>(ma_sound_get_position(m_sound.get()));
	}

	void set_position(Vec3f const& pos) final {
		if (!is_bound()) { return; }
		ma_sound_set_position(m_sound.get(), pos.x, pos.y, pos.z);
	}

	[[nodiscard]] auto get_pan() const -> float final {
		if (!is_bound()) { return {}; }
		return ma_sound_get_pan(m_sound.get());
	}

	void set_pan(float const pan) final {
		if (!is_bound()) { return; }
		ma_sound_set_pan(m_sound.get(), pan);
	}

	[[nodiscard]] auto get_pitch() const -> float final {
		if (!is_bound()) { return -1.0f; }
		return ma_sound_get_pitch(m_sound.get());
	}

	void set_pitch(float const pitch) final {
		if (!is_bound()) { return; }
		ma_sound_set_pitch(m_sound.get(), std::max(pitch, 0.0f));
	}

	void set_fade_in(std::chrono::duration<float> duration, float const gain) final {
		if (!is_bound()) { return; }
		ma_sound_set_fade_in_milliseconds(m_sound.get(), 0.0f, gain, to_ms(duration));
	}

	void set_fade_out(std::chrono::duration<float> duration) final {
		if (!is_bound()) { return; }
		ma_sound_set_fade_in_milliseconds(m_sound.get(), -1.0f, 0.0f, to_ms(duration));
	}

  private:
	[[nodiscard]] static constexpr auto to_ms(std::chrono::duration<float> const duration) -> std::uint64_t {
		return std::uint64_t(std::chrono::duration_cast<std::chrono::milliseconds>(duration).count());
	}

	template <typename F>
	[[nodiscard]] auto get_float(F func) const -> std::chrono::duration<float> {
		auto ret = float{};
		auto const result = func(m_sound.get(), &ret);
		if (result != MA_SUCCESS) { return 0s; }
		return std::chrono::duration<float>{ret};
	}

	void on_end() {
		m_ended.store(true);
		m_ended.notify_one();
	}

	ma_engine* m_engine{};
	std::shared_ptr<Pcm const> m_pcm{};
	std::unique_ptr<Sound> m_sound{};
	std::atomic_bool m_ended{};
};

class Engine : public IEngine {
  public:
	Engine(Engine const&) = delete;
	Engine(Engine&&) = delete;
	auto operator=(Engine const&) -> Engine& = delete;
	auto operator=(Engine&&) -> Engine& = delete;

	Engine() = default;

	auto init() -> bool {
		auto const result = ma_engine_init(nullptr, &m_engine);
		return result == MA_SUCCESS;
	}

	~Engine() { ma_engine_uninit(&m_engine); }

	[[nodiscard]] auto get_engine() -> ma_engine& { return m_engine; }

	[[nodiscard]] auto create_source() -> std::unique_ptr<ISource> final { return std::make_unique<Source>(m_engine); }

	[[nodiscard]] auto get_position() const -> Vec3f final {
		return std::bit_cast<Vec3f>(ma_engine_listener_get_position(&m_engine, 0));
	}

	void set_position(Vec3f const& position) final {
		ma_engine_listener_set_position(&m_engine, 0, position.x, position.y, position.z);
	}

	[[nodiscard]] auto get_direction() const -> Vec3f final {
		return std::bit_cast<Vec3f>(ma_engine_listener_get_direction(&m_engine, 0));
	}

	void set_direction(Vec3f const& direction) final {
		ma_engine_listener_set_direction(&m_engine, 0, direction.x, direction.y, direction.z);
	}

	[[nodiscard]] auto get_world_up() const -> Vec3f final {
		return std::bit_cast<Vec3f>(ma_engine_listener_get_world_up(&m_engine, 0));
	}

	void set_world_up(Vec3f const& direction) final {
		ma_engine_listener_set_world_up(&m_engine, 0, direction.x, direction.y, direction.z);
	}

  private:
	ma_engine m_engine{};
};

auto build_test_pcm(std::chrono::duration<float> const duration) {
	auto ret = Pcm{.channels = 1};
	auto get_duration = [&ret] {
		return std::chrono::duration<float>{float(ret.samples.size()) / float(Pcm::sample_rate_v)};
	};
	for (auto rad = 0.0f; get_duration() < duration; rad += 0.05f) { ret.samples.push_back(0.1f * std::sin(rad)); }
	return ret;
}
} // namespace
} // namespace capo

auto capo::create_engine() -> std::unique_ptr<IEngine> {
	auto ret = std::make_unique<Engine>();
	if (!ret->init()) { return {}; }
	return ret;
}

auto capo::decode_bytes(std::span<std::byte const> bytes, std::optional<Encoding> const encoding) -> Pcm {
	auto ret = Pcm{};
	auto decoder = Decoder{bytes, encoding};
	if (decoder.failed || !decoder.decode(ret)) { return {}; }
	return ret;
}

auto capo::decode_file(char const* path, std::optional<Encoding> encoding) -> Pcm {
	auto file = std::ifstream{path, std::ios::binary | std::ios::ate};
	if (!file) { return {}; }
	if (!encoding) { encoding = guess_encoding(fs::path{path}.extension().generic_string()); }
	auto const size = file.tellg();
	file.seekg(std::ios::beg, {});
	auto bytes = std::vector<std::byte>{};
	bytes.resize(std::size_t(size));
	void* data = bytes.data();
	file.read(static_cast<char*>(data), size);
	return decode_bytes(bytes, encoding);
}

auto capo::format_duration(std::chrono::duration<float> dt) -> std::string { return std::format("{:%T}", dt); }

auto capo::format_bytes(std::uint64_t const bytes) -> std::string {
	auto fbytes = float(bytes);
	static constexpr auto suffixes_v = std::array{"B", "KiB", "MiB", "GiB"};
	auto suffix = std::string_view{"TiB"};
	for (auto const* unit : suffixes_v) {
		if (fbytes < 1024.0f) {
			suffix = unit;
			break;
		}
		fbytes /= 1024.0f;
	}
	return std::format("{:.1f}{}", fbytes, suffix);
}

#include <thread>

auto capo::run_test(char const* path) -> int {
	auto pcm = build_test_pcm(3s);
	auto pcm2 = decode_file(path);
	auto engine = create_engine();
	if (!engine) { return EXIT_FAILURE; }
	auto source = engine->create_source();
	if (!source) { return EXIT_FAILURE; }
	if (!source->bind_to(&pcm)) { return EXIT_FAILURE; }

	auto blank_pcm = Pcm{};
	source->bind_to(&pcm2);
	[[maybe_unused]] auto const expect_fail = source->bind_to(&blank_pcm);
	assert(!expect_fail);
	source->set_fade_in(0.5s, 0.4f);
	auto const duration = source->get_duration();
	source->play();
	// assert(source->can_wait_until_ended());
	// source->wait_until_ended();
	while (source->is_playing()) {
		auto const cursor = source->get_cursor();
		std::println("{:.1f} / {:.1f}", cursor.count(), duration.count());
		std::this_thread::sleep_for(100ms);
	}

	return EXIT_SUCCESS;
}
