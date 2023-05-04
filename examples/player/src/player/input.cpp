#include <GLFW/glfw3.h>
#include <player/input.hpp>
#include <cassert>

namespace player {
void Input::on_key(int key, int action, int mods) {
	switch (action) {
	case GLFW_PRESS: on_press(key, mods); break;
	case GLFW_REPEAT: on_repeat(key, mods); break;
	case GLFW_RELEASE: on_release(key, mods); break;
	default: break;
	}
}

void Input::on_drop(std::span<std::string_view const> drops) {
	if (drops.empty()) { return; }
	m_controller->load_track(drops.front());
}

void Input::on_press(int key, int mods) {
	seek(key, mods);
	volume(key, mods);
}

void Input::on_repeat(int key, int mods) {
	seek(key, mods);
	volume(key, mods);
}

void Input::on_release(int key, int mods) {
	if (!mods) {
		switch (key) {
		case GLFW_KEY_SPACE: m_controller->play_pause(); break;
		case GLFW_KEY_ESCAPE: m_controller->quit(); break;
		case GLFW_KEY_S: m_controller->stop(); break;
		case GLFW_KEY_R: m_controller->rewind(); break;
		default: break;
		}
	}
}

void Input::volume(int key, int mods) {
	auto const delta = ((mods & GLFW_MOD_CONTROL) ? 10 : 1);
	switch (key) {
	case GLFW_KEY_UP: m_controller->set_volume(m_controller->get_model().volume + delta); break;
	case GLFW_KEY_DOWN: m_controller->set_volume(m_controller->get_model().volume - delta); break;
	default: break;
	}
}

void Input::seek(int key, int mods) {
	auto const delta = [mods] {
		if (mods & GLFW_MOD_CONTROL) { return 30s; }
		if (mods & GLFW_MOD_ALT) { return 10s; }
		return 3s;
	}();
	switch (key) {
	case GLFW_KEY_RIGHT: m_controller->seek(m_controller->get_model().cursor + delta); break;
	case GLFW_KEY_LEFT: m_controller->seek(m_controller->get_model().cursor - delta); break;
	default: break;
	}
}
} // namespace player
