#pragma once
#include <imgui.h>
#include <player/controller.hpp>
#include <optional>

namespace player {
struct SeekBar {
	capo::Duration display_cursor{};
	bool seeking{};

	std::optional<capo::Duration> update(capo::Duration actual_cursor, capo::Duration duration);
};

class View {
  public:
	View(Controller* controller) : m_controller(controller) {}

	void render(ImVec2 extent);

  private:
	void playback();
	void volume();
	void repeat();
	void tooltip();
	void seek_bar();
	void playlist();
	void loading();

	Controller* m_controller{};
	SeekBar m_seek_bar{};
	std::size_t m_selected_index{};
};
} // namespace player
