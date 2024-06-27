#include <detail/error_handler.hpp>
#include <openal/device.hpp>
#include <algorithm>
#include <cassert>

namespace capo::openal {
namespace {
template <std::same_as<State>... T>
constexpr bool one_of(State const target, T const... options) {
	return (... || (target == options));
}

bool al_check() {
	if (auto err = alGetError(); err != AL_NO_ERROR) {
		auto type = std::string_view{"Unknown Error"};
		switch (err) {
		case AL_INVALID_ENUM: type = "Invalid Enum"; break;
		case AL_INVALID_NAME: type = "Invalid Name"; break;
		case AL_INVALID_OPERATION: type = "Invalid Operation"; break;
		case AL_INVALID_VALUE: type = "Invalid Value"; break;
		default: break;
		}
		detail::dispatch_error(type);
		return false;
	}
	return true;
}

template <typename F>
bool al_check(F&& f) {
	f();
	return al_check();
}

constexpr ALenum to_al_format(Channels const in) {
	if (in == Channels::eStereo) { return AL_FORMAT_STEREO16; }
	return AL_FORMAT_MONO16;
}

template <typename T, typename F>
T al_get_source(ALuint source, ALenum param, F func) {
	auto ret = ALint{};
	al_check([&] { func(source, param, &ret); });
	return static_cast<T>(ret);
}

template <std::integral T = ALint>
T al_get_source(ALuint source, ALenum param) {
	return al_get_source<T>(source, param, &alGetSourcei);
}

template <std::floating_point T>
T al_get_source(ALuint source, ALenum param) {
	return al_get_source<T>(source, param, &alGetSourcef);
}
} // namespace

void Buffer::Deleter::operator()(ALuint const id) const {
	if (!id) { return; }
	al_check([&] { alDeleteBuffers(1u, &id); });
}

Buffer Buffer::generate() {
	auto id = ALuint{};
	al_check([&] { alGenBuffers(1u, &id); });
	return Buffer{.id = id};
}

void Buffer::write(Clip clip) {
	if (!id.get()) { return; }
	auto const size = static_cast<ALsizei>(clip.samples.size_bytes());
	al_check([&] { alBufferData(id, to_al_format(clip.channels), clip.samples.data(), size, static_cast<ALsizei>(clip.sample_rate)); });
}

StreamUpdater::StreamUpdater() {
	m_thread = std::thread(
		[this](std::atomic<bool> const& stop) {
			while (!stop.load()) {
				poll();
				std::this_thread::sleep_for(poll_rate_v);
			}
		},
		std::cref(m_stop));
}

StreamUpdater::~StreamUpdater() {
	m_stop = true;
	m_thread.join();
}

void StreamUpdater::track(Streamable* source) {
	if (!source) { return; }
	auto lock = std::scoped_lock{m_mutex};
	m_sources.insert(source);
}

void StreamUpdater::untrack(Streamable* source) {
	if (!source) { return; }
	auto lock = std::scoped_lock{m_mutex};
	m_sources.erase(source);
}

void StreamUpdater::poll() {
	auto lock = std::scoped_lock{m_mutex};
	for (auto* source : m_sources) { source->update(); }
}

void Source::Deleter::operator()(ALuint const id) const {
	if (!id) { return; }
	al_check([&] { alDeleteSources(1u, &id); });
}

Source Source::make() {
	auto id = ALuint{};
	al_check([&] { alGenSources(1u, &id); });
	return Source{.id = id};
}

State Source::state() const {
	switch (al_get_source<ALint>(id, AL_SOURCE_STATE)) {
	case AL_INITIAL: return State::eIdle;
	case AL_PLAYING: return State::ePlaying;
	case AL_PAUSED: return State::ePaused;
	case AL_STOPPED: return State::eStopped;
	default: return State::eUnknown;
	}
}

void Source::play() {
	al_check([&] { alSourcePlay(id); });
}

void Source::stop() {
	al_check([&] { alSourceStop(id); });
}

void Source::pause() {
	al_check([&] { alSourcePause(id); });
}

float Source::gain() const {
	auto value = float{};
	al_check([&] { alGetSourcef(id, AL_GAIN, &value); });
	return value;
}

void Source::set_gain(float value) {
	al_check([&] { alSourcef(id, AL_GAIN, value); });
}

float Source::pitch() const {
	auto value = float{};
	al_check([&] { alGetSourcef(id, AL_PITCH, &value); });
	return value;
}

void Source::set_pitch(float value) {
	al_check([&] { alSourcef(id, AL_PITCH, value); });
}

Vec3 Source::position() const {
	auto value = Vec3{};
	al_check([&] { alGetSource3f(id, AL_POSITION, &value.x, &value.y, &value.z); });
	return value;
}

void Source::set_position(Vec3 const& value) {
	al_check([&] { alSource3f(id, AL_POSITION, value.x, value.y, value.z); });
}

Vec3 Source::velocity() const {
	auto value = Vec3{};
	al_check([&] { alGetSource3f(id, AL_VELOCITY, &value.x, &value.y, &value.z); });
	return value;
}

void Source::set_velocity(Vec3 const& value) {
	al_check([&] { alSource3f(id, AL_VELOCITY, value.x, value.y, value.z); });
}

float Source::max_distance() const {
	auto value = float{};
	al_check([&] { alGetSourcef(id, AL_MAX_DISTANCE, &value); });
	return value;
}

void Source::set_max_distance(float value) {
	al_check([&] { alSourcef(id, AL_MAX_DISTANCE, value); });
}

State SoundSource::state() const { return m_source.state(); }

void SoundSource::play() { m_source.play(); }

void SoundSource::stop() { m_source.stop(); }

void SoundSource::pause() { m_source.pause(); }

float SoundSource::gain() const { return m_source.gain(); }

void SoundSource::set_gain(float value) { return m_source.set_gain(value); }

float SoundSource::pitch() const { return m_source.pitch(); }

void SoundSource::set_pitch(float value) { return m_source.set_pitch(value); }

Vec3 SoundSource::position() const { return m_source.position(); }

void SoundSource::set_position(Vec3 const& value) { return m_source.set_position(value); }

Vec3 SoundSource::velocity() const { return m_source.velocity(); }

void SoundSource::set_velocity(Vec3 const& value) { return m_source.set_velocity(value); }

float SoundSource::max_distance() const { return m_source.max_distance(); }

void SoundSource::set_max_distance(float radius) { return m_source.set_max_distance(radius); }

Duration SoundSource::cursor() const {
	auto ret = float{};
	al_check([&] { alGetSourcef(m_source.id, AL_SEC_OFFSET, &ret); });
	return Duration{ret};
}

void SoundSource::seek(Duration point) {
	al_check([&] { alSourcef(m_source.id, AL_SEC_OFFSET, point.count()); });
}

bool SoundSource::is_looping() const { return al_get_source<ALint>(m_source.id, AL_LOOPING) == AL_TRUE; }

void SoundSource::set_looping(bool value) {
	al_check([&] { alSourcei(m_source.id, AL_LOOPING, value ? 1 : 0); });
}

void SoundSource::set_clip(Clip clip) {
	stop();
	unbind();
	m_buffer.write(clip);
	bind();
}

void SoundSource::bind() {
	al_check([&] { alSourcei(m_source.id.get(), AL_BUFFER, static_cast<ALint>(m_buffer.id.get())); });
}

void SoundSource::unbind() {
	al_check([&] { alSourcei(m_source.id.get(), AL_BUFFER, 0); });
}

StreamSource::StreamSource(StreamUpdater* poll) : m_updater(poll) {
	poll->track(this);
	for (auto& buffer : m_buffers) { buffer = Buffer::generate(); }
}

StreamSource::~StreamSource() { m_updater->untrack(this); }

State StreamSource::state() const {
	auto lock = std::scoped_lock{m_mutex};
	return m_state;
}

void StreamSource::play() {
	auto lock = std::scoped_lock{m_mutex};
	if (!m_stream) { return; }
	if (m_state == State::ePlaying) { m_stream.seek_to_frame({}); }
	play(lock);
}

void StreamSource::stop() {
	auto lock = std::scoped_lock{m_mutex};
	if (!m_stream) { return; }
	if (one_of(m_state, State::eIdle, State::eStopped)) { return; }
	stop(lock);
}

void StreamSource::pause() {
	auto lock = std::scoped_lock{m_mutex};
	if (!m_stream) { return; }
	m_source.pause();
	m_state = m_source.state();
}

float StreamSource::gain() const { return m_source.gain(); }

void StreamSource::set_gain(float value) { return m_source.set_gain(value); }

float StreamSource::pitch() const { return m_source.pitch(); }

void StreamSource::set_pitch(float value) { return m_source.set_pitch(value); }

Vec3 StreamSource::position() const { return m_source.position(); }

void StreamSource::set_position(Vec3 const& value) { return m_source.set_position(value); }

Vec3 StreamSource::velocity() const { return m_source.velocity(); }

void StreamSource::set_velocity(Vec3 const& value) { return m_source.set_velocity(value); }

float StreamSource::max_distance() const { return m_source.max_distance(); }

void StreamSource::set_max_distance(float radius) { return m_source.set_max_distance(radius); }

Duration StreamSource::cursor() const {
	auto lock = std::scoped_lock{m_mutex};
	if (!m_stream) { return {}; }
	if (one_of(m_state, State::eIdle, State::eStopped)) {
		// playback hasn't started / has stopped
		return static_cast<float>(m_stream.next_frame_index()) / static_cast<float>(m_stream.frame_count()) * m_stream.duration();
	}
	assert(!m_queued.empty());
	// compute real time playback cursor
	auto const sample_offset = al_get_source<std::size_t>(m_source.id, AL_SAMPLE_OFFSET);
	auto const frame_offset = sample_offset / static_cast<std::size_t>(m_stream.clip().channels);
	auto const play_frame = m_queued.front() + frame_offset;
	auto const ratio = (static_cast<float>(play_frame) + 0.5f) / static_cast<float>(m_stream.frame_count());
	return ratio * m_stream.duration();
}

void StreamSource::seek(Duration point) {
	auto lock = std::scoped_lock{m_mutex};
	if (!m_stream) { return; }
	auto const previous_state = m_state;
	stop(lock);
	auto const ratio = std::clamp(point / m_stream.duration(), 0.0f, 1.0f);
	m_stream.seek_to_frame(static_cast<FrameIndex>(ratio * static_cast<float>(m_stream.frame_count())));
	if (previous_state == State::ePlaying) { play(lock); }
}

void StreamSource::set_stream(Stream stream) {
	if (!stream) { return; }

	auto lock = std::scoped_lock{m_mutex};
	stop(lock);
	m_stream = std::move(stream);
	m_samples = std::vector<Sample>(sample_count_for(frames_per_buffer_v, m_stream.clip().channels));
}

bool StreamSource::has_stream() const {
	auto lock = std::scoped_lock{m_mutex};
	return static_cast<bool>(m_stream);
}

bool StreamSource::update() {
	auto lock = std::scoped_lock{m_mutex};
	if (!m_stream || m_state != State::ePlaying) { return {}; }

	if (m_stream.at_end()) {
		if (!m_loop) {
			if (m_source.state() == State::eStopped && m_state != State::eStopped) { stop(lock); }
			return {};
		}
		m_stream.seek_to_frame({});
	}

	auto const processed_count = al_get_source<std::size_t>(m_source.id, AL_BUFFERS_PROCESSED);
	if (processed_count <= 0) { return {}; }

	auto unqueued = ALuint{};
	al_check([&] { alSourceUnqueueBuffers(m_source.id, 1u, &unqueued); });
	std::rotate(m_queued.begin(), m_queued.begin() + 1, m_queued.end());
	m_queued.pop_back();

	auto const it = std::find_if(m_buffers.begin(), m_buffers.end(), [unqueued](Buffer const& b) { return b.id.get() == unqueued; });
	assert(it != m_buffers.end());
	m_queued.push_back(m_stream.next_frame_index());
	it->write(m_stream.read(m_samples));

	al_check([&] { alSourceQueueBuffers(m_source.id, 1u, &unqueued); });

	return true;
}

void StreamSource::play(std::scoped_lock<std::mutex> const&) {
	if (one_of(m_state, State::eIdle, State::eStopped)) {
		m_queued.clear();
		auto buffer_ids = std::array<ALuint, buffering_v>{};
		auto buffer_count = std::size_t{};
		al_check([&] { alSourcei(m_source.id, AL_BUFFER, 0); });
		for (auto& buffer : m_buffers) {
			m_queued.push_back(m_stream.next_frame_index());
			buffer.write(m_stream.read(m_samples));
			buffer_ids[buffer_count++] = buffer.id;
			if (m_stream.at_end()) { break; }
		}
		al_check([&] { alSourceQueueBuffers(m_source.id, static_cast<ALsizei>(buffer_count), buffer_ids.data()); });
	}
	m_source.play();
	m_state = m_source.state();
}

void StreamSource::stop(std::scoped_lock<std::mutex> const&) {
	m_source.stop();
	auto processed_count = al_get_source<ALsizei>(m_source.id, AL_BUFFERS_PROCESSED);
	assert(processed_count <= static_cast<int>(buffering_v));
	auto processed = std::array<ALuint, buffering_v>{};
	alSourceUnqueueBuffers(m_source.id, processed_count, processed.data());
	m_queued.clear();
	m_stream.seek_to_frame({});
	m_state = m_source.state();
}

void Device::Deleter::operator()(ALCdevice* ptr) const { alcCloseDevice(ptr); }

void Device::Deleter::operator()(ALCcontext* ptr) const {
	al_check();
	alcMakeContextCurrent(nullptr);
	alcDestroyContext(ptr);
}

bool Device::init() {
	if (alcGetCurrentContext() != nullptr) { return false; }
	m_device = std::unique_ptr<ALCdevice, Deleter>{alcOpenDevice(nullptr)};
	if (!m_device) { return false; }
	m_context = std::unique_ptr<ALCcontext, Deleter>{alcCreateContext(m_device.get(), nullptr)};
	if (!m_context) { return false; }

	return al_check([&] { alcMakeContextCurrent(m_context.get()); });
}

float Device::gain() const {
	if (!m_context) { return {}; }
	auto ret = float{};
	al_check([&] { alGetListenerf(AL_GAIN, &ret); });
	return ret;
}

void Device::set_gain(float value) {
	if (!m_context) { return; }
	al_check([&] { alListenerf(AL_GAIN, value); });
}

Vec3 Device::velocity() const {
	if (!m_context) { return {}; }
	auto ret = Vec3{};
	al_check([&] { alGetListener3f(AL_VELOCITY, &ret.x, &ret.y, &ret.z); });
	return ret;
}

void Device::set_velocity(Vec3 const& value) {
	if (!m_context) { return; }
	al_check([&] { alListener3f(AL_VELOCITY, value.x, value.y, value.z); });
}

Vec3 Device::position() const {
	if (!m_context) { return {}; }
	auto ret = Vec3{};
	al_check([&] { alGetListener3f(AL_POSITION, &ret.x, &ret.y, &ret.z); });
	return ret;
}

void Device::set_position(Vec3 const& value) {
	if (!m_context) { return; }
	al_check([&] { alListener3f(AL_POSITION, value.x, value.y, value.z); });
}

Orientation Device::orientation() const {
	auto values = std::array<float, 6>{};
	al_check([&] { alGetListenerfv(AL_ORIENTATION, values.data()); });
	auto const at = Vec3{values[0], values[1], values[2]};
	auto const up = Vec3{values[3], values[4], values[5]};
	return {at, up};
}

void Device::set_orientation(Orientation const& value) {
	if (!m_context) { return; }
	auto const& at = value.look_at;
	auto const& up = value.up;
	auto const values = std::array{at.x, at.y, at.z, up.x, up.y, up.z};
	al_check([&] { alListenerfv(AL_ORIENTATION, values.data()); });
}
} // namespace capo::openal
