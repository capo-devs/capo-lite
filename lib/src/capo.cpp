#include <miniaudio.h>
#include <capo/buffer.hpp>
#include <capo/engine.hpp>
#include <capo/format.hpp>
#include <capo/stream_pipe.hpp>
#include <algorithm>
#include <array>
#include <atomic>
#include <bit>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <ranges>
#include <span>
#include <variant>
#include <vector>

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

constexpr auto guess_encoding_from_extension(std::string_view const extension) -> std::optional<Encoding> {
	if (extension == ".wav") { return Encoding::Wav; }
	if (extension == ".mp3") { return Encoding::Mp3; }
	if (extension == ".flac") { return Encoding::Flac; }
	return {};
}

class Decoder : public ma_decoder {
  public:
	Decoder(Decoder const&) = delete;
	Decoder(Decoder&&) = delete;
	auto operator=(Decoder const&) -> Decoder& = delete;
	auto operator=(Decoder&&) -> Decoder& = delete;

	explicit Decoder(std::span<std::byte const> bytes, std::optional<Encoding> const encoding) : ma_decoder({}) {
		auto config = ma_decoder_config_init(ma_format_f32, 0, Buffer::sample_rate_v);
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
		m_input_size = bytes.size();
	}

	~Decoder() { ma_decoder_uninit(this); }

	[[nodiscard]] auto decode(std::vector<float>& samples, std::uint8_t& channels) -> bool {
		auto frames = ma_uint64{};
		ma_decoder_get_length_in_pcm_frames(this, &frames);

		channels = m_channels;
		samples.clear();
		samples.reserve(get_reserve_size());
		static constexpr auto buffer_size_v = 128uz /*KiB*/ * 1024uz /*B*/ / sizeof(float);
		auto buffer = std::vector<float>(buffer_size_v);
		while (true) {
			auto const frames_to_read = buffer.size() / m_channels;
			auto frames_read = ma_uint64{};
			auto const result = ma_decoder_read_pcm_frames(this, buffer.data(), frames_to_read, &frames_read);
			if (result != MA_SUCCESS || frames_read == 0) { return false; }

			auto const frames = std::span{buffer.data(), frames_read * m_channels};
			for (auto const sample : frames) { samples.push_back(sample); }
			if (frames_read < frames_to_read) { break; }
		}
		return true;
	}

	[[nodiscard]] auto get_channels() const -> std::uint8_t { return m_channels; }

	bool failed{};

  private:
	[[nodiscard]] auto get_reserve_size() -> std::size_t {
		auto frames = ma_uint64{};
		// if the decoder returns the frame count, return exact value.
		if (ma_decoder_get_length_in_pcm_frames(this, &frames) == MA_SUCCESS && frames > 0) {
			// MP3s and FLACs decode one extra frame (?)...
			return std::size_t((frames + 1) * m_channels);
		}

		// WAV input will be larger than "decoded" PCM, assume 80% shrinkage.
		static constexpr auto input_coefficient_v = 0.8f;
		auto const input_based = std::size_t(input_coefficient_v * float(m_input_size));

		// Rely on malloc allocating pages after this point.
		static constexpr auto max_reserve_v = 128uz /*KiB*/ * 1024uz /*B*/;
		return std::min(input_based, max_reserve_v);
	}

	std::uint8_t m_channels{};
	std::size_t m_input_size{};
};

class AudioBuffer : public ma_audio_buffer {
  public:
	AudioBuffer(AudioBuffer const&) = delete;
	AudioBuffer(AudioBuffer&&) = delete;
	auto operator=(AudioBuffer const&) -> AudioBuffer& = delete;
	auto operator=(AudioBuffer&&) -> AudioBuffer& = delete;

	explicit AudioBuffer(Buffer const& buffer) : ma_audio_buffer({}) {
		auto config = ma_audio_buffer_config_init(ma_format_f32, buffer.get_channels(), buffer.get_frame_count(),
												  buffer.get_samples().data(), nullptr);
		config.sampleRate = Buffer::sample_rate_v;
		auto result = ma_audio_buffer_init(&config, this);
		if (result != MA_SUCCESS) {
			failed = true;
			return;
		}
	}

	~AudioBuffer() {
		if (failed) { return; }
		ma_audio_buffer_uninit(this);
	}

	bool failed{};
};

class StreamSource : public ma_data_source_base {
  public:
	StreamSource(StreamSource const&) = delete;
	StreamSource(StreamSource&&) = delete;
	auto operator=(StreamSource const&) = delete;
	auto operator=(StreamSource&&) = delete;

	explicit StreamSource(IStream& stream)
		: ma_data_source_base({}), m_stream(stream), m_channels(stream.get_channels()) {
		auto config = ma_data_source_config_init();
		config.vtable = &s_vtable;
		auto const result = ma_data_source_init(&config, this);
		if (result != MA_SUCCESS) {
			failed = true;
			return;
		}
	}

	~StreamSource() {
		if (failed) { return; }
		ma_data_source_uninit(this);
	}

	bool failed{};

  private:
	auto on_read(void* out, ma_uint64 count, ma_uint64& frames_read) {
		auto const span = std::span{static_cast<float*>(out), std::size_t(count) * m_channels};
		frames_read = ma_uint64(m_stream.read_samples(span) / m_channels);
		if (frames_read == 0) { return MA_AT_END; }
		return MA_SUCCESS;
	}

	auto on_seek(ma_uint64 const frame) {
		return m_stream.seek_to_sample(frame * m_channels) ? MA_SUCCESS : MA_NOT_IMPLEMENTED;
	}

	auto get_data_format(ma_format& fmt, ma_uint32& channels, ma_uint32& sample_rate, ma_channel* ch_map,
						 std::size_t const max_ch) {
		auto const in_channels = m_stream.get_channels();
		auto const in_sample_rate = m_stream.get_sample_rate();
		if (in_channels == 0 || in_sample_rate == 0) { return MA_NOT_IMPLEMENTED; }

		fmt = ma_format_f32;
		channels = ma_uint32(in_channels);
		sample_rate = ma_uint32(in_sample_rate);
		auto const span = std::span{ch_map, max_ch};
		static constexpr auto channels_stereo_v = std::array{MA_CHANNEL_LEFT, MA_CHANNEL_RIGHT};
		if (!span.empty()) {
			if (channels == 1) {
				span[0] = MA_CHANNEL_MONO;
			} else {
				for (auto const [in, out] : std::views::zip(channels_stereo_v, span)) { out = in; }
			}
		}

		return MA_SUCCESS;
	}

	auto get_cursor(ma_uint64& out) const {
		auto const ret = m_stream.get_cursor();
		if (!ret) {
			out = 0;
			return MA_NOT_IMPLEMENTED;
		}
		out = ma_uint64(*ret);
		return MA_SUCCESS;
	}

	auto get_frame_count(ma_uint64& out) const {
		auto const ret = m_stream.get_sample_count();
		if (ret == 0) {
			out = 0;
			return MA_NOT_IMPLEMENTED;
		}
		out = ma_uint64(ret * m_channels);
		return MA_SUCCESS;
	}

	auto set_looping(ma_bool32 const loop) {
		return m_stream.set_looping(loop == MA_TRUE) ? MA_SUCCESS : MA_NOT_IMPLEMENTED;
	}

	static ma_data_source_vtable const s_vtable;

	IStream& m_stream;
	std::uint8_t m_channels{};
};

ma_data_source_vtable const StreamSource::s_vtable = {
	.onRead = [](ma_data_source* base, void* out, ma_uint64 count, ma_uint64* frames_read) -> ma_result {
		return static_cast<StreamSource*>(base)->on_read(out, count, *frames_read);
	},
	.onSeek = [](ma_data_source* base, ma_uint64 frame) -> ma_result {
		return static_cast<StreamSource*>(base)->on_seek(frame);
	},
	.onGetDataFormat = [](ma_data_source* base, ma_format* fmt, ma_uint32* channels, ma_uint32* sample_rate,
						  ma_channel* ch_map, std::size_t max_ch) -> ma_result {
		return static_cast<StreamSource*>(base)->get_data_format(*fmt, *channels, *sample_rate, ch_map, max_ch);
	},
	.onGetCursor = [](ma_data_source* base, ma_uint64* cursor) -> ma_result {
		return static_cast<StreamSource*>(base)->get_cursor(*cursor);
	},
	.onGetLength = [](ma_data_source* base, ma_uint64* out_length) -> ma_result {
		return static_cast<StreamSource*>(base)->get_frame_count(*out_length);
	},
	.onSetLooping = [](ma_data_source* base, ma_bool32 loop) -> ma_result {
		return static_cast<StreamSource*>(base)->set_looping(loop);
	},
	.flags = {},
};

class Sound : public ma_sound {
  public:
	Sound(Sound const&) = delete;
	Sound(Sound&&) = delete;
	auto operator=(Sound const&) -> Sound& = delete;
	auto operator=(Sound&&) -> Sound& = delete;

	explicit Sound(ma_engine& engine, Buffer const& buffer)
		: ma_sound({}), m_storage(std::in_place_type_t<AudioBuffer>{}, buffer) {
		if (std::get<AudioBuffer>(m_storage).failed) {
			failed = true;
			return;
		}

		auto const result =
			ma_sound_init_from_data_source(&engine, &std::get<AudioBuffer>(m_storage), 0, nullptr, this);
		if (result != MA_SUCCESS) { failed = true; }
	}

	explicit Sound(ma_engine& engine, char const* path) : ma_sound({}) {
		static constexpr auto flags_v = MA_SOUND_FLAG_STREAM;
		auto const result = ma_sound_init_from_file(&engine, path, flags_v, nullptr, nullptr, this);
		if (result != MA_SUCCESS) { failed = true; }
	}

	explicit Sound(ma_engine& engine, IStream& stream)
		: ma_sound({}), m_storage(std::in_place_type_t<StreamSource>{}, stream) {
		if (std::get<StreamSource>(m_storage).failed) {
			failed = true;
			return;
		}
		auto const result =
			ma_sound_init_from_data_source(&engine, &std::get<StreamSource>(m_storage), 0, nullptr, this);
		if (result != MA_SUCCESS) { failed = true; }
	}

	~Sound() {
		if (failed) { return; }
		ma_sound_stop(this);
		ma_sound_uninit(this);
	}

	bool failed{};

  private:
	std::variant<std::monostate, AudioBuffer, StreamSource> m_storage{};
};

class Source : public ISource {
  public:
	explicit Source(ma_engine& engine) : m_engine(engine) {}

	[[nodiscard]] auto is_bound() const -> bool final { return m_sound != nullptr; }

	auto bind_to(Buffer const* target) -> bool final {
		if (target == nullptr || !target->is_loaded()) { return false; }
		return try_create_sound(*target);
	}

	auto bind_to(std::shared_ptr<Buffer const> target) -> bool final {
		if (!bind_to(target.get())) { return false; }
		m_ref = std::move(target);
		return true;
	}

	auto bind_to(IStream* target) -> bool final {
		if (target == nullptr || target->get_channels() == 0 || target->get_sample_rate() == 0) { return false; }
		return try_create_sound(*target);
	}

	auto bind_to(std::shared_ptr<IStream> custom_stream) -> bool final {
		if (!bind_to(custom_stream.get())) { return false; }
		m_ref = std::move(custom_stream);
		return true;
	}

	auto open_file_stream(char const* path) -> bool final {
		if (path == nullptr || *path == '\0') { return false; }
		return try_create_sound(path);
	}

	void unbind() final {
		m_sound.reset();
		m_ref.reset();
	}

	[[nodiscard]] auto is_playing() const -> bool final {
		return is_bound() && ma_sound_is_playing(m_sound.get()) == MA_TRUE;
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

	auto set_cursor(std::chrono::duration<float> const position) -> bool final {
		if (!is_bound() || position < 0s || position > get_duration()) { return false; }
		ma_sound_seek_to_second(m_sound.get(), position.count());
		return true;
	}

	[[nodiscard]] auto is_spatialized() const -> bool final {
		return is_bound() && ma_sound_is_spatialization_enabled(m_sound.get()) == MA_TRUE;
	}

	auto set_spatialized(bool const spatialized) -> bool final {
		if (!is_bound()) { return false; }
		ma_sound_set_spatialization_enabled(m_sound.get(), spatialized ? MA_TRUE : MA_FALSE);
		return true;
	}

	auto set_fade_in(std::chrono::duration<float> duration, float const gain) -> bool final {
		if (!is_bound()) { return false; }
		ma_sound_set_fade_in_milliseconds(m_sound.get(), 0.0f, gain, to_ms(duration));
		return true;
	}

	auto set_fade_out(std::chrono::duration<float> duration) -> bool final {
		if (!is_bound()) { return false; }
		ma_sound_set_fade_in_milliseconds(m_sound.get(), -1.0f, 0.0f, to_ms(duration));
		return true;
	}

	[[nodiscard]] auto is_looping() const -> bool final { return m_state.looping; }

	void set_looping(bool const looping) final {
		m_state.looping = looping;
		if (!is_bound()) { return; }
		ma_sound_set_looping(m_sound.get(), looping ? MA_TRUE : MA_FALSE);
	}

	[[nodiscard]] auto get_gain() const -> float final { return m_state.gain; }

	void set_gain(float const gain) final {
		m_state.gain = std::clamp(gain, 0.0f, 1.0f);
		if (!is_bound()) { return; }
		ma_sound_set_volume(m_sound.get(), m_state.gain);
	}

	[[nodiscard]] auto get_position() const -> Vec3f final { return m_state.position; }

	void set_position(Vec3f const& pos) final {
		m_state.position = pos;
		if (!is_bound()) { return; }
		ma_sound_set_position(m_sound.get(), pos.x, pos.y, pos.z);
	}

	[[nodiscard]] auto get_pan() const -> float final { return m_state.pan; }

	void set_pan(float const pan) final {
		m_state.pan = pan;
		if (!is_bound()) { return; }
		ma_sound_set_pan(m_sound.get(), pan);
	}

	[[nodiscard]] auto get_pitch() const -> float final { return m_state.pitch; }

	void set_pitch(float const pitch) final {
		m_state.pitch = std::max(pitch, 0.0f);
		if (!is_bound()) { return; }
		ma_sound_set_pitch(m_sound.get(), m_state.pitch);
	}

  private:
	struct State {
		Vec3f position{};
		float gain{1.0f};
		float pan{0.0f};
		float pitch{0.0f};
		bool looping{};
	};

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

	template <typename... Args>
	auto try_create_sound(Args&&... args) -> bool {
		if (m_sound && is_playing()) { stop(); }
		auto sound = std::make_unique<Sound>(m_engine, std::forward<Args>(args)...);
		if (sound->failed) { return false; }

		m_sound = std::move(sound);
		m_ref.reset();
		copy_state_to_ma();
		set_cursor(0s);
		static auto const callback = +[](void* self, ma_sound* /*sound*/) { static_cast<Source*>(self)->on_end(); };
		ma_sound_set_end_callback(m_sound.get(), callback, this);
		return true;
	}

	void on_end() {
		m_ended.store(true);
		m_ended.notify_one();
	}

	void copy_state_to_ma() const {
		assert(is_bound());
		ma_sound_set_volume(m_sound.get(), m_state.gain);
		ma_sound_set_looping(m_sound.get(), m_state.looping ? MA_TRUE : MA_FALSE);
		auto const& pos = m_state.position;
		ma_sound_set_position(m_sound.get(), pos.x, pos.y, pos.z);
		ma_sound_set_pan(m_sound.get(), m_state.pan);
		ma_sound_set_pitch(m_sound.get(), m_state.pitch);
	}

	ma_engine& m_engine;
	std::shared_ptr<void const> m_ref{};
	std::unique_ptr<Sound> m_sound{};
	State m_state{};
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
} // namespace

void Buffer::set_frames(std::vector<float> samples, std::uint8_t const channels) {
	m_samples = std::move(samples);
	m_channels = channels;
}

auto Buffer::decode_bytes(std::span<std::byte const> bytes, std::optional<Encoding> const encoding) -> bool {
	auto decoder = Decoder{bytes, encoding};
	return !decoder.failed && decoder.decode(m_samples, m_channels);
}

auto Buffer::decode_file(char const* path, std::optional<Encoding> encoding) -> bool {
	if (!encoding) { encoding = guess_encoding(path); }
	auto const bytes = file_to_bytes(path);
	if (bytes.empty()) { return false; }
	return decode_bytes(bytes, encoding);
}

auto IStreamPipe::read_samples(std::span<float> out) -> std::size_t {
	auto ret = drain_buffer(out);
	assert(ret <= out.size());
	if (ret == out.size()) { return ret; }
	out = out.subspan(ret);

	while (m_buffer.size() < out.size()) {
		auto const prev_size = m_buffer.size();
		push_samples(m_buffer);
		auto const at_end = m_buffer.size() == prev_size;
		if (at_end) { break; }
	}

	ret += drain_buffer(out);
	return ret;
}

auto IStreamPipe::drain_buffer(std::span<float> out) -> std::size_t {
	if (m_buffer.empty()) { return 0; }
	auto const size = std::min(out.size(), m_buffer.size());
	auto const src = std::span{m_buffer}.subspan(0, size);
	auto const remain = std::span{m_buffer}.subspan(size);
	std::ranges::copy(src, out.begin());
	if (remain.empty()) {
		m_buffer.clear();
	} else {
		m_staging.clear();
		std::ranges::copy(remain, std::back_inserter(m_staging));
		m_buffer.clear();
		std::ranges::copy(m_staging, std::back_inserter(m_buffer));
	}
	return size;
}
} // namespace capo

auto capo::guess_encoding(std::string_view const path) -> std::optional<Encoding> {
	return guess_encoding_from_extension(fs::path{path}.extension().generic_string());
}

auto capo::file_to_bytes(char const* path) -> std::vector<std::byte> {
	auto file = std::ifstream{path, std::ios::binary | std::ios::ate};
	if (!file) { return {}; }
	auto const size = file.tellg();
	file.seekg(std::ios::beg, {});
	auto ret = std::vector<std::byte>{};
	ret.resize(std::size_t(size));
	void* data = ret.data();
	file.read(static_cast<char*>(data), size);
	return ret;
}

auto capo::create_engine() -> std::unique_ptr<IEngine> {
	auto ret = std::make_unique<Engine>();
	if (!ret->init()) { return {}; }
	return ret;
}

void capo::format_duration_to(std::string& out, std::chrono::duration<float> const dt) {
	if (dt < 1h) {
		std::format_to(std::back_inserter(out), "{:%M:%S}", dt);
		return;
	}
	std::format_to(std::back_inserter(out), "{:%T}", dt);
}

auto capo::format_duration(std::chrono::duration<float> const dt) -> std::string {
	auto ret = std::string{};
	format_duration_to(ret, dt);
	return ret;
}

void capo::format_bytes_to(std::string& out, std::uint64_t const bytes) {
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
	std::format_to(std::back_inserter(out), "{:.1f}{}", fbytes, suffix);
}

auto capo::format_bytes(std::uint64_t const bytes) -> std::string {
	auto ret = std::string{};
	format_bytes_to(ret, bytes);
	return ret;
}
