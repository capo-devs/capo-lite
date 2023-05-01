#include <capo/build_version.hpp>
#include <capo/capo.hpp>
#include <cassert>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <thread>

namespace {
bool music_test(char const* file_path, float const gain, int const rounds) {
	auto device = capo::Device{};
	if (!device) {
		std::cerr << "Couldn't create valid device." << std::endl;
		return false;
	}

	auto result = capo::Pcm::from_file(file_path);
	if (!result.pcm) {
		std::cerr << "Couldn't load audio file.\n";
		return false;
	}

	auto const& pcm = result.pcm;

	static constexpr std::string_view compression_str[] = {"??", "WAV", "MP3", "FLAC"};

	auto const duration = pcm.clip().duration();
	std::cout << file_path << " info:\n"
			  << "\t" << compression_str[static_cast<std::size_t>(result.compression)] << "\n"
			  << "\t" << std::fixed << std::setprecision(2) << duration.count() << "\n"
			  << "\t" << (pcm.clip().channels == capo::Channels::eMono ? "Mono" : "Stereo") << "\n"
			  << "\t" << pcm.clip().sample_rate << "Hz\n";

	auto source = device.make_stream_source();
	source.set_gain(gain);
	source.set_stream(pcm.clip());

	for (int round{}; round < rounds; ++round) {
		int done{};
		std::cout << "\r  ____________________\r  " << std::flush;
		source.play();
		while (source.state() == capo::State::ePlaying) {
			std::this_thread::yield();

			int const progress = static_cast<int>(20.0f * source.cursor() / duration);
			if (progress > done) {
				std::cout << "\r  ";
				int i = 0;
				for (; i < done; i++) { std::cout << '='; }
				done = progress;
				std::cout << std::flush;
			}
		}
	}
	assert(source.cursor() == capo::Duration{});
	std::cout << "=\ncapo-lite v" << capo::version_v << " ^^\n";
	return 0;
}
} // namespace

int main(int argc, char** argv) {
	if (argc < 2) {
		std::cerr << "Syntax: " << argv[0] << " <audio file path> [gain]" << std::endl;
		return EXIT_FAILURE;
	}

	float const gain = argc > 2 ? static_cast<float>(std::atof(argv[2])) : 1.0f;
	int const rounds = argc > 3 ? std::atoi(argv[3]) : 2;
	if (!music_test(argv[1], gain > 0.0f ? gain : 1.0f, rounds >= 1 ? rounds : 1)) { return EXIT_FAILURE; }
}
