#pragma once
#include <capo/clip.hpp>
#include <capo/orientation.hpp>
#include <capo/state.hpp>
#include <capo/stream.hpp>
#include <capo/vec3.hpp>

namespace capo::detail {
struct Source {
	virtual ~Source() = default;

	Source& operator=(Source&&) = delete;

	virtual State state() const = 0;
	virtual void play() = 0;
	virtual void stop() = 0;
	virtual void pause() = 0;

	virtual bool is_looping() const = 0;
	virtual void set_looping(bool value) = 0;
	virtual float gain() const = 0;
	virtual void set_gain(float value) = 0;
	virtual float pitch() const = 0;
	virtual void set_pitch(float value) = 0;
	virtual Vec3 position() const = 0;
	virtual void set_position(Vec3 const& value) = 0;
	virtual Vec3 velocity() const = 0;
	virtual void set_velocity(Vec3 const& value) = 0;
	virtual float max_distance() const = 0;
	virtual void set_max_distance(float radius) = 0;
	virtual Duration cursor() const = 0;
	virtual void seek(Duration point) = 0;
};

struct SoundSource : Source {
	virtual void set_clip(Clip clip) = 0;
};

struct StreamSource : Source {
	virtual void set_stream(Stream stream) = 0;
	virtual void rewind() { seek({}); }
};

struct Device {
	virtual ~Device() = default;

	Device& operator=(Device&&) = delete;

	virtual bool init() = 0;
	virtual std::unique_ptr<SoundSource> make_sound_source() const = 0;
	virtual std::unique_ptr<StreamSource> make_stream_source() const = 0;
	virtual float gain() const = 0;
	virtual void set_gain(float value) = 0;
	virtual Vec3 velocity() const = 0;
	virtual void set_velocity(Vec3 const& value) = 0;
	virtual Vec3 position() const = 0;
	virtual void set_position(Vec3 const& value) = 0;
	virtual Orientation orientation() const = 0;
	virtual void set_orientation(Orientation const& value) = 0;
};
} // namespace capo::detail
