#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <optional>

#include "Utils.h"
#include "Vector.h"

namespace Xitils {

	class Ray {
	public:

		Vector3f o;
		Vector3f d;
		int depth;
		float tMax;

		Ray():
			depth(0),
			tMax(Infinity)
		{}

		Ray(const Vector3f& o, const Vector3f& d, int depth = 0, float tMax = Infinity):
			o(o), d(d), depth(depth), tMax(tMax)
		{}

		Ray(const Ray& ray) :
			o(ray.o),
			d(ray.d),
			depth(ray.depth),
			tMax(ray.tMax)
		{}

		Vector3f operator()(float t) const {
			return o + d * t;
		}

	};

}