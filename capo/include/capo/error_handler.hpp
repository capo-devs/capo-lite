#pragma once
#include <functional>
#include <string_view>

namespace capo {
///
/// \brief Customization point for handling internal (OpenAL) errors.
///
/// By default the error type will be logged to stderr.
///
using ErrorHandler = std::function<void(std::string_view)>;

///
/// \brief Set a custom error handler.
///
/// Pass a default constructed instance to disable logging.
///
void set_error_handler(ErrorHandler handler);
} // namespace capo
