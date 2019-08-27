#pragma once

#include "Utils.h"
#include "Vector.h"

namespace Xitils {

	class Ray {
	public:

		Vector3f o;
		Vector3f d;
		float tMax;

		Ray():
			tMax(Infinity)
		{}

		Ray(const Vector3f& o, const Vector3f& d, int depth = 0, float tMax = Infinity, float weight = 1.0f):
			o(o), d(d), tMax(tMax)
		{}

		Ray(const Ray& ray) :
			o(ray.o),
			d(ray.d),
			tMax(ray.tMax)
		{}

		Vector3f operator()(float t) const {
			return o + d * t;
		}

	};

}