#pragma once
#include <player/model.hpp>

namespace player {
class Controller {
  public:
	virtual ~Controller() = default;

	virtual Model const& get_model() const = 0;

	virtual void play_pause() = 0;
	virtual void stop() = 0;
	virtual void rewind() = 0;
	virtual void seek(capo::Duration point) = 0;
	virtual void set_volume(int value) = 0;
	virtual void set_repeat(bool value) = 0;
	virtual void quit() = 0;
	virtual void load_track(std::string_view path) = 0;
};
} // namespace player
