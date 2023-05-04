#include <player/app.hpp>
#include <iostream>

int main(int argc, char** argv) {
	try {
		auto app = player::App{};
		if (argc > 1) { app.load_track(argv[1]); }
		app.run();
	} catch (std::exception const& e) {
		std::cerr << "Fatal error: " << e.what() << "\n";
		return EXIT_FAILURE;
	}
}
