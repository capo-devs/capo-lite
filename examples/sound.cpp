#include <capo/build_version.hpp>
#include <capo/capo.hpp>
#include <cassert>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <thread>

namespace {
constexpr float travel_circurference_radius = 2.0f;
constexpr float travel_angular_speed = 2.0f;

// Uniform circular motion formulas
capo::Vec3 ucm_position(float angular_speed, float time, float radius) {
	return {std::cos(time * angular_speed) * radius, std::sin(time * angular_speed) * radius, 0};
}

capo::Vec3 ucm_velocity(float angular_speed, float time, float radius) {
	return {-radius * angular_speed * std::cos(time * angular_speed), radius * angular_speed * std::sin(time * angular_speed), 0};
}

bool sound_test(char const* file_path, float gain) {
	auto device = capo::Device{};
	if (!device) {
		std::cerr << "Couldn't create valid Device.\n";
		return false;
	}

	auto result = capo::Pcm::from_file(file_path);
	if (!result.pcm) {
		std::cerr << "Couldn't load audio file.\n";
		return false;
	}

	auto const clip = result.pcm.clip();
	auto source = device.make_sound_source();
	source.set_gain(gain);
	source.set_clip(clip);
	source.play();

	if (clip.channels == capo::Channels::eMono) {
		std::cout << "Travelling on a circurference around the listener; r=" << std::fixed << std::setprecision(1) << travel_circurference_radius
				  << ", angular speed=" << travel_angular_speed << "\n";
	} else {
		std::cout << "Input has more than one channel, positional audio is disabled\n";
	}

	static constexpr std::string_view compression_str[] = {"??", "WAV", "MP3", "FLAC"};

	std::cout << file_path << " info:\n"
			  << "\t" << compression_str[static_cast<std::size_t>(result.compression)] << "\n"
			  << "\t" << capo::format_duration(clip.duration()) << "\n"
			  << "\t" << (clip.channels == capo::Channels::eMono ? "Mono" : "Stereo") << "\n"
			  << "\t" << clip.sample_rate << "Hz\n";

	int done{};
	auto start = std::chrono::high_resolution_clock::now();

	std::cout << "  ____________________  " << std::flush;
	while (source.state() == capo::State::ePlaying) {
		std::this_thread::yield();
		auto now = std::chrono::high_resolution_clock::now();
		auto time = capo::Duration(now - start).count();

		capo::Vec3 position = ucm_position(travel_angular_speed, time, travel_circurference_radius);
		capo::Vec3 velocity = ucm_velocity(travel_angular_speed, time, travel_circurference_radius);

		source.set_position(position);
		source.set_velocity(velocity);

		int const progress = static_cast<int>(20.0f * source.cursor() / result.pcm.clip().duration());
		if (progress > done) {
			std::cout << "\r  ";
			for (int i = 0; i < progress; i++) { std::cout << '='; }
			done = progress;
			std::cout << std::flush;
		}
	}
	assert(source.cursor() == capo::Duration{});
	std::cout << "=\ncapo-lite v" << capo::version_v << " ^^\n";
	return true;
}
} // namespace

int main(int argc, char** argv) {
	if (argc < 2) {
		std::cerr << "Syntax: " << std::filesystem::path{argv[0]}.filename().string() << " <audio file path> [gain]" << std::endl;
		return EXIT_FAILURE;
	}

	float const gain = argc > 2 ? static_cast<float>(std::atof(argv[2])) : 1.0f;
	if (!sound_test(argv[1], gain > 0.0f ? gain : 1.0f)) { return EXIT_FAILURE; }
}
