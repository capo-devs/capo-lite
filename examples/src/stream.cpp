#include <capo/engine.hpp>
#include <algorithm>
#include <atomic>
#include <cassert>
#include <filesystem>
#include <print>

namespace fs = std::filesystem;

namespace example {
namespace {
// this example stream just wraps capo::Buffer and a cursor index.
// a more useful one would wrap a custom decoder and generate samples on the fly.
class Stream : public capo::IStream {
  public:
	explicit Stream(capo::Buffer buffer) : m_buffer(std::move(buffer)) { assert(m_buffer.is_loaded()); }

  private:
	[[nodiscard]] auto read_samples(std::span<float> out) -> std::size_t final {
		auto const samples = m_buffer.get_samples();
		// cache the current value of m_index.
		auto index = m_index.load();
		if (index >= samples.size()) {
			// finished playback.
			return 0;
		}
		// trim front to the current index.
		auto src = samples.subspan(index);
		// obtain the number of samples to copy.
		auto const count = std::min(src.size(), out.size());
		// trim to exact size to copy.
		src = src.subspan(0, count);
		// fill output buffer.
		std::ranges::copy(src, out.begin());
		// increment cursor unless modified via seek_to_sample() in the meanwhile.
		m_index.compare_exchange_strong(index, index + count);
		// return count of samples read.
		return count;
	}

	[[nodiscard]] auto get_sample_rate() const -> std::uint32_t final { return capo::Buffer::sample_rate_v; }
	[[nodiscard]] auto get_channels() const -> std::uint8_t final { return m_buffer.get_channels(); }

	[[nodiscard]] auto seek_to_sample(std::size_t const index) -> bool final {
		m_index = std::min(index, m_buffer.get_samples().size());
		return true;
	}

	[[nodiscard]] auto get_cursor() const -> std::optional<std::size_t> final { return m_index.load(); }

	[[nodiscard]] auto get_sample_count() const -> std::size_t final { return m_buffer.get_samples().size(); }

	// the buffer is read-only after construction, does not need synchronization.
	capo::Buffer m_buffer{};
	// the cursor index may be accessed on another thread while also being updated in read_samples().
	std::atomic<std::size_t> m_index{};
};

void run(fs::path const& path) {
	// create an Engine instance.
	auto engine = capo::create_engine();
	// fatal error if no Engine.
	if (!engine) { throw std::runtime_error{"Failed to create Engine"}; }

	// create an Audio Source.
	auto source = engine->create_source();
	// fatal error if no Source.
	if (!source) { throw std::runtime_error{"Failed to create Source"}; }

	auto buffer = capo::Buffer{};
	if (!buffer.decode_file(path.string().c_str())) {
		throw std::runtime_error{"Failed to decode file: " + path.generic_string()};
	}

	// bind Stream to Source. fatal error if not bound.
	if (!source->bind_to(std::make_shared<Stream>(std::move(buffer)))) {
		throw std::runtime_error{"Failed to bind Source to Stream"};
	}

	// start playback.
	source->play();
	// wait until playback has ended.
	source->wait_until_ended();
}
} // namespace
} // namespace example

auto main(int argc, char** argv) -> int {
	try {
		auto args = std::span{argv, std::size_t(argc)};
		auto exe_name = std::string{"<exe>"};
		assert(!args.empty());
		exe_name = fs::path{args.front()}.stem().string();
		args = args.subspan(1);

		if (args.empty()) {
			std::println("Usage: {} <path/to/audio/file>", exe_name);
			return EXIT_FAILURE;
		}

		example::run(args.front());
	} catch (std::exception const& e) {
		std::println("PANIC: {}", e.what());
		return EXIT_FAILURE;
	} catch (...) {
		std::println("PANIC!");
		return EXIT_FAILURE;
	}
}
