#pragma once
#include <capo/orientation.hpp>
#include <capo/sound_source.hpp>
#include <capo/stream_source.hpp>
#include <memory>

namespace capo {
namespace detail {
struct Device;
}

///
/// \brief An Audio Device capable of creating Audio Sources.
///
/// Also functions as the Audio Listener associated with this device / context.
///
class Device {
  public:
	explicit Device();

	SoundSource make_sound_source() const;
	StreamSource make_stream_source() const;

	float gain() const;
	void set_gain(float value);
	Vec3 velocity() const;
	void set_velocity(Vec3 const& value);
	Orientation orientation() const;
	void set_orientation(Orientation const& value);

	explicit operator bool() const { return m_impl != nullptr; }

  private:
	struct Deleter {
		void operator()(detail::Device const* ptr) const;
	};
	std::unique_ptr<detail::Device, Deleter> m_impl{};
};
} // namespace capo
