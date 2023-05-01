#include <capo/device.hpp>
#include <detail/error_handler.hpp>
#include <detail/interfaces.hpp>

#if defined(CAPO_USE_OPENAL)
#include <openal/device.hpp>
#endif

namespace capo {
namespace {
struct NullSoundSource : detail::SoundSource {
	State state() const final { return State::eIdle; }
	void play() final {}
	void stop() final {}
	void pause() final {}

	bool is_looping() const final { return false; }
	void set_looping(bool) final {}
	float gain() const final { return {}; }
	void set_gain(float) final {}
	float pitch() const final { return {}; }
	void set_pitch(float) final {}
	Vec3 position() const final { return {}; }
	void set_position(Vec3 const&) final {}
	Vec3 velocity() const final { return {}; }
	void set_velocity(Vec3 const&) final {}
	float max_distance() const { return {}; }
	void set_max_distance(float) {}
	Duration cursor() const { return {}; }
	void seek(Duration) {}

	void set_clip(Clip) final {}
};

struct NullStreamSource : detail::StreamSource {
	State state() const final { return State::eIdle; }
	void play() final {}
	void stop() final {}
	void pause() final {}

	bool is_looping() const final { return false; }
	void set_looping(bool) final {}
	float gain() const final { return {}; }
	void set_gain(float) final {}
	float pitch() const final { return {}; }
	void set_pitch(float) final {}
	Vec3 position() const final { return {}; }
	void set_position(Vec3 const&) final {}
	Vec3 velocity() const final { return {}; }
	void set_velocity(Vec3 const&) final {}
	float max_distance() const { return {}; }
	void set_max_distance(float) {}
	Duration cursor() const { return {}; }
	void seek(Duration) {}

	void set_stream(Stream) final {}
};

struct NullDevice : detail::Device {
	bool init() final { return true; }
	std::unique_ptr<detail::SoundSource> make_sound_source() const final { return std::make_unique<NullSoundSource>(); }
	std::unique_ptr<detail::StreamSource> make_stream_source() const final { return std::make_unique<NullStreamSource>(); }
	float gain() const final { return {}; }
	void set_gain(float) final {}
	Vec3 velocity() const final { return {}; }
	void set_velocity(Vec3 const&) final {}
	Orientation orientation() const final { return {}; }
	void set_orientation(Orientation const&) final {}
};

std::unique_ptr<detail::Device> make_device() {
#if defined(CAPO_USE_OPENAL)
	return std::make_unique<openal::Device>();
#else
	return std::make_unique<NullDevice>();
#endif
}
} // namespace

void Device::Deleter::operator()(detail::Device const* ptr) const { delete ptr; }

Device::Device() : m_impl(make_device().release()) {
	if (m_impl && !m_impl->init()) {
		detail::dispatch_error("Failed to initialize audio library");
		m_impl = {};
	}
}

SoundSource Device::make_sound_source() const { return m_impl ? SoundSource{m_impl->make_sound_source()} : SoundSource{nullptr}; }

StreamSource Device::make_stream_source() const { return m_impl ? StreamSource{m_impl->make_stream_source()} : StreamSource{nullptr}; }

float Device::gain() const {
	if (!m_impl) { return {}; }
	return m_impl->gain();
}

void Device::set_gain(float value) {
	if (!m_impl) { return; }
	return m_impl->set_gain(value);
}

Vec3 Device::velocity() const {
	if (!m_impl) { return {}; }
	return m_impl->velocity();
}

void Device::set_velocity(Vec3 const& value) {
	if (!m_impl) { return; }
	return m_impl->set_velocity(value);
}

Orientation Device::orientation() const {
	if (!m_impl) { return {}; }
	return m_impl->orientation();
}

void Device::set_orientation(Orientation const& value) {
	if (!m_impl) { return; }
	return m_impl->set_orientation(value);
}
} // namespace capo
