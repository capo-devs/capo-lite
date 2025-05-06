#pragma once
#include <capo/source.hpp>
#include <capo/version.hpp>
#include <memory>

namespace capo {
/// \brief Audio Engine.
/// API to create Audio Sources.
/// Represents 3D spatialized listener.
/// Uses default audio device.
class IEngine : public Polymorphic {
  public:
	/// \brief Create an Audio Source.
	/// \returns null on failure.
	[[nodiscard]] virtual auto create_source() -> std::unique_ptr<ISource> = 0;

	/// \brief Obtain the listener's 3D position.
	[[nodiscard]] virtual auto get_position() const -> Vec3f = 0;
	/// \brief Set the listener's 3D position.
	virtual void set_position(Vec3f const& position) = 0;

	/// \brief Obtain the listener's direction as a unit vector.
	[[nodiscard]] virtual auto get_direction() const -> Vec3f = 0;
	/// \brief Set the listener's direction as a unit vector.
	virtual void set_direction(Vec3f const& direction) = 0;

	/// \brief Obtain the world up as a unit vector.
	[[nodiscard]] virtual auto get_world_up() const -> Vec3f = 0;
	/// \brief Set the world up as a unit vector.
	virtual void set_world_up(Vec3f const& direction) = 0;
};

/// \brief Create an Engine instance.
/// \returns null on failure.
[[nodiscard]] auto create_engine() -> std::unique_ptr<IEngine>;
} // namespace capo
