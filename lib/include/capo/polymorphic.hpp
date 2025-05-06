#pragma once

namespace capo {
/// \brief Base class for polymorphic types.
class Polymorphic {
  public:
	Polymorphic() = default;
	virtual ~Polymorphic() = default;

	Polymorphic(Polymorphic const&) = default;
	Polymorphic(Polymorphic&&) = default;
	auto operator=(Polymorphic const&) -> Polymorphic& = default;
	auto operator=(Polymorphic&&) -> Polymorphic& = default;
};
} // namespace capo
