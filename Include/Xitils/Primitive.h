#pragma once

#include "Utils.h"
#include "Vector.h"
#include "Bounds.h"
#include "Transform.h"
#include "Ray.h"
#include "Interaction.h"

namespace Xitils {

	class Primitive {
	public:

		virtual Bounds3f bound() const = 0;

		virtual float surfaceArea() const = 0;

		virtual bool intersect(const Ray& ray, float* tHit, SurfaceInteraction* isect) const = 0;
		virtual bool intersectBool(const Ray& ray) const = 0;

	};

}