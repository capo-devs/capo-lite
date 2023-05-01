#include <capo/stream_source.hpp>
#include <detail/interfaces.hpp>

namespace capo {
void StreamSource::Deleter::operator()(detail::StreamSource const* ptr) const { delete ptr; }

StreamSource::StreamSource(std::unique_ptr<detail::StreamSource> impl) : m_impl(impl.release()) {}

void StreamSource::set_stream(Stream stream) {
	if (!m_impl) { return; }
	m_impl->set_stream(std::move(stream));
}

State StreamSource::state() const {
	if (!m_impl) { return {}; }
	return m_impl->state();
}

void StreamSource::play() {
	if (!m_impl) { return; }
	return m_impl->play();
}

void StreamSource::stop() {
	if (!m_impl) { return; }
	return m_impl->stop();
}

void StreamSource::pause() {
	if (!m_impl) { return; }
	return m_impl->pause();
}

bool StreamSource::is_looping() const {
	if (!m_impl) { return {}; }
	return m_impl->is_looping();
}

void StreamSource::set_looping(bool value) {
	if (!m_impl) { return; }
	return m_impl->set_looping(value);
}

float StreamSource::gain() const {
	if (!m_impl) { return {}; }
	return m_impl->gain();
}

void StreamSource::set_gain(float value) {
	if (!m_impl) { return; }
	return m_impl->set_gain(value);
}

float StreamSource::pitch() const {
	if (!m_impl) { return {}; }
	return m_impl->pitch();
}

void StreamSource::set_pitch(float value) {
	if (!m_impl) { return; }
	return m_impl->set_pitch(value);
}

Vec3 StreamSource::position() const {
	if (!m_impl) { return {}; }
	return m_impl->position();
}

void StreamSource::set_position(Vec3 const& value) {
	if (!m_impl) { return; }
	return m_impl->set_position(value);
}

Vec3 StreamSource::velocity() const {
	if (!m_impl) { return {}; }
	return m_impl->velocity();
}

void StreamSource::set_velocity(Vec3 const& value) {
	if (!m_impl) { return; }
	return m_impl->set_velocity(value);
}

float StreamSource::max_distance() const {
	if (!m_impl) { return {}; }
	return m_impl->max_distance();
}

void StreamSource::set_max_distance(float radius) {
	if (!m_impl) { return; }
	return m_impl->set_max_distance(radius);
}

Duration StreamSource::cursor() const {
	if (!m_impl) { return {}; }
	return m_impl->cursor();
}

void StreamSource::seek(Duration point) {
	if (!m_impl) { return; }
	return m_impl->seek(point);
}

void StreamSource::rewind() {
	if (!m_impl) { return; }
	return m_impl->rewind();
}
} // namespace capo
