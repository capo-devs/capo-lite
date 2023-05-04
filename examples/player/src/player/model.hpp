#pragma once
#include <capo/pcm.hpp>
#include <capo/state.hpp>
#include <string>

namespace player {
struct Track {
	std::string name_with_compression{};
	capo::Duration duration{};
};

struct Model {
	Track track{};
	capo::State state{};
	capo::Duration cursor{};
	std::string loading{};
	int volume{80};
	bool repeat{};

	bool ready() const { return track.duration > 0s; }
};
} // namespace player
