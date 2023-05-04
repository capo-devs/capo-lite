#include <capo/capo.hpp>
#include <gvdi/gvdi.hpp>
#include <player/app.hpp>
#include <player/input.hpp>
#include <player/view.hpp>
#include <filesystem>
#include <future>
#include <sstream>

namespace player {
namespace {
namespace fs = std::filesystem;

constexpr ImVec2 app_extent_v{450.0f, 150.0f};

std::string build_window_title() {
	auto ret = std::stringstream{};
	ret << "capo-lite v" << capo::version_v;
	return ret.str();
}
struct LoadRequest {
	std::future<capo::Pcm::Result> result{};
	std::string name{};
};

Track build_track(std::string_view track_name, capo::Duration duration, capo::Compression compression) {
	static constexpr std::string_view compression_text_v[] = {"??", "WAV", "MP3", "FLAC"};
	auto const compression_text = compression_text_v[static_cast<std::size_t>(compression)];
	auto str = std::stringstream{};
	str << track_name << " (" << compression_text << ")";
	return Track{.name_with_compression = str.str(), .duration = duration};
}
} // namespace

struct App::Impl : Controller {
	capo::Device capo{};
	gvdi::Instance gvdi{};

	capo::Pcm pcm{};
	capo::StreamSource source{};
	LoadRequest load_request{};

	Model model{};
	Input input{this};
	View view{this};

	Impl() : gvdi(app_extent_v, build_window_title().c_str()) {
		if (!capo || !gvdi) { throw std::runtime_error{"Failed to initialize capo / gvdi"}; }
		glfwSetWindowAttrib(gvdi.window(), GLFW_RESIZABLE, GLFW_FALSE);
		gvdi.set_event_handler(&input);
		source = capo.make_stream_source();

		set_volume(model.volume);
		set_repeat(model.repeat);
	}

	void update_load_request() {
		if (!load_request.result.valid()) {
			model.loading.clear();
			return;
		}
		if (load_request.result.wait_for(0s) == std::future_status::ready) {
			auto result = load_request.result.get();
			if (!result) { return; }
			pcm = std::move(result.pcm);
			model.track = build_track(std::move(load_request.name), pcm.clip().duration(), result.compression);
			model.loading.clear();
			source.set_stream(pcm.clip());
			source.play();
		}
	}

	void update_model() {
		model.state = source.state();
		model.cursor = source.cursor();
	}

	void run() {
		while (gvdi.is_running()) {
			auto frame = gvdi::Frame{gvdi};
			update_load_request();
			update_model();
			view.render(gvdi.framebuffer_extent());
		}
	}

	Model const& get_model() const final { return model; }

	void play_pause() final {
		if (!source) { return; }
		switch (source.state()) {
		case capo::State::ePlaying: source.pause(); break;
		default: source.play(); break;
		}
	}

	void stop() final { source.stop(); }

	void rewind() final { source.rewind(); }

	void seek(capo::Duration point) final {
		if (!source) { return; }
		source.seek(point);
	}

	void set_volume(int value) final {
		source.set_gain((static_cast<float>(value) + 0.5f) / 100.0f);
		model.volume = static_cast<int>(source.gain() * 100.0f);
	}

	void set_repeat(bool value) final {
		source.set_looping(value);
		model.repeat = source.is_looping();
	}

	void quit() final { gvdi.close(); }

	void load_track(std::string_view const path) final {
		load_request.name = fs::path{path}.stem().string();
		load_request.result = std::async([path = std::string{path}] { return capo::Pcm::from_file(path.c_str()); });
		model.loading = load_request.name;
	}
};

void App::Deleter::operator()(Impl const* ptr) const { delete ptr; }

App::App() : m_impl(new Impl{}) {}

void App::load_track(std::string_view const path) {
	if (!m_impl) { return; }
	m_impl->load_track(path);
}

void App::run() {
	if (!m_impl) { return; }
	m_impl->run();
}
} // namespace player
