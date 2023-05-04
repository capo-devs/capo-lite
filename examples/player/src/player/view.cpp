#include <imgui_internal.h>
#include <player/view.hpp>
#include <algorithm>
#include <sstream>

namespace player {
std::optional<capo::Duration> SeekBar::update(capo::Duration actual_cursor, capo::Duration duration) {
	if (!seeking) { display_cursor = actual_cursor; }
	auto cursor_text = capo::format_duration(display_cursor);
	cursor_text += " / ";
	cursor_text += capo::format_duration(duration);
	auto cursor = display_cursor.count();
	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::SliderFloat("##cursor", &cursor, 0.0f, duration.count(), cursor_text.c_str())) {
		seeking = true;
		display_cursor = capo::Duration{cursor};
	} else if (!ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
		seeking = false;
	}
	if (!seeking && display_cursor != actual_cursor) { return display_cursor; }
	return {};
}

void View::render(ImVec2 extent) {
	static constexpr auto window_flags_v = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
	ImGui::SetNextWindowSize(extent);
	ImGui::SetNextWindowPos({}, ImGuiCond_Always);
	auto const title = m_controller->get_model().ready() ? m_controller->get_model().track.name_with_compression : std::string_view{"--"};
	ImGui::Begin(title.data(), nullptr, window_flags_v);
	playback();
	volume();
	repeat();
	tooltip();
	seek_bar();
	loading();
	ImGui::End();
}

void View::playback() {
	auto const ready = m_controller->get_model().ready();
	static constexpr ImVec2 button_size_v{65.0f, 40.0f};
	auto const main_button = [state = m_controller->get_model().state] {
		switch (state) {
		case capo::State::ePlaying: return ImGui::Button(" pause", button_size_v);
		default: return ImGui::Button(" play", button_size_v);
		}
	};
	if (main_button() && ready) { m_controller->play_pause(); }
	ImGui::SameLine();
	if (ImGui::Button("stop", button_size_v) && ready) { m_controller->stop(); }
	ImGui::SameLine();
	if (ImGui::Button("rewind", button_size_v) && ready) { m_controller->rewind(); }
}

void View::volume() {
	static constexpr float volume_width_v{150.0f};
	auto const text_width = ImGui::CalcTextSize("volume").x;
	ImGui::SameLine();
	ImGui::SetCursorPosX(ImGui::GetWindowWidth() - volume_width_v - 10.0f - text_width);
	ImGui::Text("volume");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(volume_width_v);
	auto volume = m_controller->get_model().volume;
	ImGui::SliderInt("##volume", &volume, 0, 100);
	if (ImGui::IsItemHovered() && ImGui::GetIO().MouseWheel != 0.0f) { volume = std::clamp(volume + static_cast<int>(ImGui::GetIO().MouseWheel), 0, 100); }
	if (volume != m_controller->get_model().volume) { m_controller->set_volume(volume); }
}

void View::repeat() {
	ImGui::NewLine();
	bool repeat = m_controller->get_model().repeat;
	if (ImGui::Checkbox("repeat", &repeat)) { m_controller->set_repeat(repeat); }
}

void View::tooltip() {
	ImGui::SameLine();
	ImGui::SeparatorEx(ImGuiSeparatorFlags_Vertical);
	ImGui::SameLine();
	ImGui::Text("(?)");
	if (ImGui::IsItemHovered() && ImGui::BeginTooltip()) {
		ImGui::Text("%s", "Drag an audio file to start playback");
		ImGui::EndTooltip();
	}
}

void View::seek_bar() {
	ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 30.0f);
	auto const& model = m_controller->get_model();
	auto const duration = model.ready() ? model.track.duration : capo::Duration{};
	if (auto cursor = m_seek_bar.update(model.cursor, duration)) { m_controller->seek(*cursor); }
}

void View::loading() {
	bool const is_open = ImGui::IsPopupOpen("Loading");
	if (!m_controller->get_model().loading.empty() && !is_open) { ImGui::OpenPopup("Loading"); }
	auto const centre = ImGui::GetMainViewport()->GetCenter();
	if (is_open) { ImGui::SetNextWindowPos(centre, ImGuiCond_None, {0.5f, 0.5f}); }
	if (ImGui::BeginPopupModal("Loading", {}, ImGuiWindowFlags_AlwaysAutoResize)) {
		ImGui::Text("Loading %s...", m_controller->get_model().loading.c_str());
		if (m_controller->get_model().loading.empty()) { ImGui::CloseCurrentPopup(); }
		ImGui::EndPopup();
	}
}
} // namespace player
