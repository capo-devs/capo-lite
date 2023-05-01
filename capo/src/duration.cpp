#include <capo/duration.hpp>
#include <iomanip>
#include <sstream>

std::string capo::format_duration(Duration duration, std::string_view separator) {
	auto ret = std::stringstream{};
	ret << std::setw(1) << std::setfill('0');
	auto const hrs = std::chrono::duration_cast<std::chrono::hours>(duration);
	auto const mins = std::chrono::duration_cast<std::chrono::minutes>(duration) - hrs;
	auto const secs = std::chrono::duration_cast<std::chrono::seconds>(duration) - mins;
	if (hrs > std::chrono::hours{}) { ret << hrs.count() << separator; }
	ret << mins.count() << separator << std::setw(2) << secs.count();
	return ret.str();
}
