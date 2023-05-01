#pragma once
#include <capo/vec3.hpp>

namespace capo {
///
/// \brief 3D orientation expressed as "look_at" and "up" vectors.
///
struct Orientation {
	Vec3 look_at{0.0f, 0.0f, 1.0f};
	Vec3 up{0.0f, 1.0f, 0.0f};
};
} // namespace capo
