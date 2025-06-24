#pragma once
#include <capo/buffer.hpp>
#include <capo/polymorphic.hpp>
#include <capo/stream.hpp>
#include <capo/vec3.hpp>
#include <chrono>
#include <memory>

namespace capo {
/// \brief Audio Source.
/// API for audio playback.
/// Open file for streaming or bind to existing Audio Buffer.
/// Supports 3D spatialization.
class ISource : public Polymorphic {
  public:
	/// \brief Check if source is bound to a stream or buffer.
	[[nodiscard]] virtual auto is_bound() const -> bool = 0;
	auto bind_to(std::nullptr_t) = delete;
	/// \brief Bind to existing buffer.
	/// Passed buffer must outlive this instance.
	/// \param buffer Audio Buffer to bind source to.
	/// \returns true on success.
	virtual auto bind_to(Buffer const* buffer) -> bool = 0;
	/// \brief Bind to existing buffer and increment its ref-count.
	/// \param buffer Audio Buffer to bind source to.
	/// \returns true on success.
	virtual auto bind_to(std::shared_ptr<Buffer const> buffer) -> bool = 0;
	/// \brief Bind to custom stream (data source).
	/// Passed stream must outlive this instance.
	/// \param custom_stream Stream to bind to.
	/// \returns true on success.
	virtual auto bind_to(IStream* custom_stream) -> bool = 0;
	/// \brief Bind to custom stream (data source).
	/// \param custom_stream Stream to bind to.
	/// \returns true on success.
	virtual auto bind_to(std::shared_ptr<IStream> custom_stream) -> bool = 0;
	/// \brief Open file stream and bind to it.
	/// The same encodings are supported as with capo::Buffer.
	/// \param path Path to audio file.
	/// \returns true on success.
	virtual auto open_file_stream(char const* path) -> bool = 0;
	/// \brief Detach buffer or stream if bound.
	virtual void unbind() = 0;

	[[nodiscard]] virtual auto is_playing() const -> bool = 0;
	virtual void play() = 0;
	virtual void stop() = 0;

	/// \brief Check if source is at end.
	/// \returns true after playback is complete, unless looping.
	[[nodiscard]] virtual auto at_end() const -> bool = 0;
	/// \brief Check if feasible to block until source is at end.
	/// \returns true if playing and not looping.
	[[nodiscard]] virtual auto can_wait_until_ended() const -> bool = 0;
	/// \brief Block until source is at end.
	/// The calling thread is blocked via atomic wait/notify (not spinlocking).
	/// Returns immediately if can_wait_until_ended() returns false.
	virtual void wait_until_ended() = 0;

	/// \brief Get duration of source.
	/// \returns -1s if not bound.
	[[nodiscard]] virtual auto get_duration() const -> std::chrono::duration<float> = 0;
	/// \brief Get position of playback cursor.
	/// \returns -1s if not bound.
	[[nodiscard]] virtual auto get_cursor() const -> std::chrono::duration<float> = 0;
	/// \brief Set position of playback cursor.
	virtual auto set_cursor(std::chrono::duration<float> position) -> bool = 0;

	/// \brief Check if spatialization is enabled.
	/// \returns true if bound and spatialized.
	[[nodiscard]] virtual auto is_spatialized() const -> bool = 0;
	/// \brief Toggle spatialization.
	/// \brief returns false if not bound.
	virtual auto set_spatialized(bool spatialized) -> bool = 0;

	/// \brief Set fade in parameters.
	/// \param duration Duration of fade.
	/// \param gain Gain to fade to.
	/// \returns false if not bound.
	virtual auto set_fade_in(std::chrono::duration<float> duration, float gain = -1.0f) -> bool = 0;
	/// \brief Set fade out parameters. Target gain is implicitly 0.
	/// \param duration Duration of fade.
	/// \returns false if not bound.
	virtual auto set_fade_out(std::chrono::duration<float> duration) -> bool = 0;

	[[nodiscard]] virtual auto is_looping() const -> bool = 0;
	virtual void set_looping(bool looping) = 0;

	[[nodiscard]] virtual auto get_gain() const -> float = 0;
	virtual void set_gain(float gain) = 0;

	[[nodiscard]] virtual auto get_position() const -> Vec3f = 0;
	virtual void set_position(Vec3f const& pos) = 0;

	[[nodiscard]] virtual auto get_pan() const -> float = 0;
	virtual void set_pan(float pan) = 0;

	[[nodiscard]] virtual auto get_pitch() const -> float = 0;
	virtual void set_pitch(float pitch) = 0;
};
} // namespace capo
