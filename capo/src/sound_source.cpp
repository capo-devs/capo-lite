#include <capo/sound_source.hpp>
#include <detail/interfaces.hpp>

namespace capo {
void SoundSource::Deleter::operator()(detail::SoundSource const* ptr) const { delete ptr; }

SoundSource::SoundSource(std::unique_ptr<detail::SoundSource> impl) : m_impl(impl.release()) {}

void SoundSource::set_clip(Clip clip) {
	if (!m_impl) { return; }
	return m_impl->set_clip(std::move(clip));
}

State SoundSource::state() const {
	if (!m_impl) { return {}; }
	return m_impl->state();
}

void SoundSource::play() {
	if (!m_impl) { return; }
	return m_impl->play();
}

void SoundSource::stop() {
	if (!m_impl) { return; }
	return m_impl->stop();
}

void SoundSource::pause() {
	if (!m_impl) { return; }
	return m_impl->pause();
}

bool SoundSource::is_looping() const {
	if (!m_impl) { return {}; }
	return m_impl->is_looping();
}

void SoundSource::set_looping(bool value) {
	if (!m_impl) { return; }
	return m_impl->set_looping(value);
}

float SoundSource::gain() const {
	if (!m_impl) { return {}; }
	return m_impl->gain();
}

void SoundSource::set_gain(float value) {
	if (!m_impl) { return; }
	return m_impl->set_gain(value);
}

float SoundSource::pitch() const {
	if (!m_impl) { return {}; }
	return m_impl->pitch();
}

void SoundSource::set_pitch(float value) {
	if (!m_impl) { return; }
	return m_impl->set_pitch(value);
}

Vec3 SoundSource::position() const {
	if (!m_impl) { return {}; }
	return m_impl->position();
}

void SoundSource::set_position(Vec3 const& value) {
	if (!m_impl) { return; }
	return m_impl->set_position(value);
}

Vec3 SoundSource::velocity() const {
	if (!m_impl) { return {}; }
	return m_impl->velocity();
}

void SoundSource::set_velocity(Vec3 const& value) {
	if (!m_impl) { return; }
	return m_impl->set_velocity(value);
}

float SoundSource::max_distance() const {
	if (!m_impl) { return {}; }
	return m_impl->max_distance();
}

void SoundSource::set_max_distance(float radius) {
	if (!m_impl) { return; }
	return m_impl->set_max_distance(radius);
}

Duration SoundSource::cursor() const {
	if (!m_impl) { return {}; }
	return m_impl->cursor();
}

void SoundSource::seek(Duration point) {
	if (!m_impl) { return; }
	return m_impl->seek(point);
}
} // namespace capo
