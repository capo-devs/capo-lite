#pragma once
#include <capo/stream_source.hpp>
#include <gvdi/event_handler.hpp>
#include <player/controller.hpp>

namespace player {
class Input : public gvdi::EventHandler::Default {
  public:
	Input(Controller* controller) : m_controller(controller) {}

  private:
	void on_key(int key, int action, int mods) final;
	void on_drop(std::span<std::string_view const> drops) final;

	void on_press(int key, int mods);
	void on_repeat(int key, int mods);
	void on_release(int key, int mods);

	void volume(int key, int mods);
	void seek(int key, int mods);

	Controller* m_controller{};
};
} // namespace player
