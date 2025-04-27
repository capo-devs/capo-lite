#pragma once
#include <capo/source.hpp>
#include <capo/version.hpp>
#include <memory>

namespace capo {
class IEngine : public Polymorphic {
  public:
	[[nodiscard]] virtual auto create_source() -> std::unique_ptr<ISource> = 0;

	[[nodiscard]] virtual auto get_position() const -> Vec3f = 0;
	virtual void set_position(Vec3f const& position) = 0;

	[[nodiscard]] virtual auto get_direction() const -> Vec3f = 0;
	virtual void set_direction(Vec3f const& direction) = 0;

	[[nodiscard]] virtual auto get_world_up() const -> Vec3f = 0;
	virtual void set_world_up(Vec3f const& direction) = 0;
};

[[nodiscard]] auto create_engine() -> std::unique_ptr<IEngine>;
} // namespace capo
