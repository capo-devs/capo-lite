#pragma once
#include <capo/duration.hpp>
#include <capo/state.hpp>
#include <capo/stream.hpp>
#include <capo/vec3.hpp>
#include <chrono>
#include <memory>

namespace capo {
namespace detail {
struct StreamSource;
} // namespace detail

///
/// \brief An Audio Source that streams data in chunks periodically.
///
class StreamSource {
  public:
	StreamSource() = default;
	explicit StreamSource(std::unique_ptr<detail::StreamSource> impl);

	///
	/// \brief Set a clip for this source.
	///
	/// The underlying storage for the stream / clip (the Pcm instance) must
	/// outlive this Stream Source.
	///
	void set_stream(Stream stream);

	State state() const;
	void play();
	void stop();
	void pause();

	bool is_looping() const;
	void set_looping(bool value);
	float gain() const;
	void set_gain(float value);
	float pitch() const;
	void set_pitch(float value);
	Vec3 position() const;
	void set_position(Vec3 const& value);
	Vec3 velocity() const;
	void set_velocity(Vec3 const&);
	float max_distance() const;
	void set_max_distance(float radius);
	Duration cursor() const;
	void seek(Duration point);
	void rewind();

	explicit operator bool() const { return m_impl != nullptr; }

  private:
	struct Deleter {
		void operator()(detail::StreamSource const* ptr) const;
	};
	std::unique_ptr<detail::StreamSource, Deleter> m_impl{};
};
} // namespace capo
