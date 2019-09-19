#pragma once


#include "Utils.h"
#include "Vector.h"
#include "Bounds.h"
#include "Interaction.h"

namespace xitils {

	class Geometry {
	public:

		virtual Bounds3f bound() const = 0;

		virtual float surfaceArea() const = 0;

		virtual bool intersect(const Ray& ray, float* tHit, SurfaceInteraction* isect) const = 0;
		virtual bool intersectAny(const Ray& ray) const {
			float tHit;
			SurfaceInteraction isect;
			return intersect(ray, &tHit, &isect);
		}

	};

}