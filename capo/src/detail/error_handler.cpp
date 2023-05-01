#include <capo/error_handler.hpp>
#include <detail/error_handler.hpp>
#include <iostream>

namespace {
capo::ErrorHandler g_error_handler{[](std::string_view error) { std::cerr << error << '\n'; }};
}

void capo::set_error_handler(ErrorHandler handler) { g_error_handler = std::move(handler); }

void capo::detail::dispatch_error(std::string_view error) {
	if (!g_error_handler) { return; }
	g_error_handler(error);
}
