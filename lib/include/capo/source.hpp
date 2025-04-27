#pragma once
#include <capo/pcm.hpp>
#include <capo/polymorphic.hpp>
#include <capo/vec3.hpp>
#include <chrono>
#include <memory>

namespace capo {
class ISource : public Polymorphic {
  public:
	[[nodiscard]] virtual auto is_bound() const -> bool = 0;
	auto bind_to(std::nullptr_t) = delete;
	virtual auto bind_to(Pcm const* pcm) -> bool = 0;
	virtual auto bind_to(std::shared_ptr<Pcm const> pcm) -> bool = 0;
	virtual auto open_stream(char const* path) -> bool = 0;
	virtual void unbind() = 0;

	[[nodiscard]] virtual auto is_playing() const -> bool = 0;
	virtual void play() = 0;
	virtual void stop() = 0;

	[[nodiscard]] virtual auto at_end() const -> bool = 0;
	[[nodiscard]] virtual auto can_wait_until_ended() const -> bool = 0;
	virtual void wait_until_ended() = 0;

	[[nodiscard]] virtual auto get_duration() const -> std::chrono::duration<float> = 0;
	[[nodiscard]] virtual auto get_cursor() const -> std::chrono::duration<float> = 0;
	virtual void set_cursor(std::chrono::duration<float> position) = 0;

	[[nodiscard]] virtual auto is_looping() const -> bool = 0;
	virtual void set_looping(bool looping) = 0;

	[[nodiscard]] virtual auto get_gain() const -> float = 0;
	virtual void set_gain(float gain) = 0;

	[[nodiscard]] virtual auto is_spatialized() const -> bool = 0;
	virtual void set_spatialized(bool spatialized) = 0;

	[[nodiscard]] virtual auto get_position() const -> Vec3f = 0;
	virtual void set_position(Vec3f const& pos) = 0;

	[[nodiscard]] virtual auto get_pan() const -> float = 0;
	virtual void set_pan(float pan) = 0;

	[[nodiscard]] virtual auto get_pitch() const -> float = 0;
	virtual void set_pitch(float pitch) = 0;

	virtual void set_fade_in(std::chrono::duration<float> duration, float gain = -1.0f) = 0;
	virtual void set_fade_out(std::chrono::duration<float> duration) = 0;
};
} // namespace capo
