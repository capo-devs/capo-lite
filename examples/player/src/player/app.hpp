#pragma once
#include <memory>
#include <string_view>

namespace player {
class App {
  public:
	App();

	void load_track(std::string_view path);
	void run();

  private:
	struct Impl;
	struct Deleter {
		void operator()(Impl const* ptr) const;
	};

	std::unique_ptr<Impl, Deleter> m_impl{};
};
} // namespace player
