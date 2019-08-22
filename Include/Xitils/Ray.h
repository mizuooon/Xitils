#pragma once

#include "Utils.h"
#include "Vector.h"

namespace Xitils {

	class Ray {
	public:

		Vector3f o;
		Vector3f d;
		int depth;
		float tMax;
		float weight;

		Ray():
			depth(0),
			tMax(Infinity),
			weight(1.0f)
		{}

		Ray(const Vector3f& o, const Vector3f& d, int depth = 0, float tMax = Infinity, float weight = 1.0f):
			o(o), d(d), depth(depth), tMax(tMax), weight(1.0f)
		{}

		Ray(const Ray& ray) :
			o(ray.o),
			d(ray.d),
			depth(ray.depth),
			tMax(ray.tMax),
			weight(ray.weight)
		{}

		Vector3f operator()(float t) const {
			return o + d * t;
		}

	};

}