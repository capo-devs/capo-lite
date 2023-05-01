#pragma once
#include <capo/clip.hpp>
#include <capo/state.hpp>
#include <capo/vec3.hpp>
#include <chrono>
#include <memory>

namespace capo {
namespace detail {
struct SoundSource;
} // namespace detail

///
/// \brief Audio Source that stores the entire clip in audio memory.
///
class SoundSource {
  public:
	SoundSource() = default;
	explicit SoundSource(std::unique_ptr<detail::SoundSource> impl);

	///
	/// \brief Set a clip for this source.
	///
	/// The clip is copied into a buffer during this, its storage
	/// need not be kept around after it returns.
	///
	void set_clip(Clip clip);

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

	explicit operator bool() const { return m_impl != nullptr; }

  private:
	struct Deleter {
		void operator()(detail::SoundSource const* ptr) const;
	};
	std::unique_ptr<detail::SoundSource, Deleter> m_impl{};
};
} // namespace capo
