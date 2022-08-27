#pragma once

#include "Utils.h"
#include "Vector.h"

namespace xitils {

	class Ray {
	public:

		Vector3f o;
		Vector3f d;
		float tMax;

		Ray():
			tMax(Infinity)
		{}

		Ray(const Vector3f& o, const Vector3f& d, float tMax = Infinity):
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