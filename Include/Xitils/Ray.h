#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <optional>

#include "Utils.h"
#include "Vector.h"

namespace Xitils {

	class Ray {
	public:

		Ray():
			tMax(Infinity),
			depth(0)
		{}

		Ray(const Vector3f& o, const Vector3f& d, int depth = 0):
			o(o), d(d), depth(depth)
		{}

		Vector3f operator() const {
			return o + d * t;
		}

		float tMax;
		int depth;
		Vector3f o;
		Vector3f d;

	};

}