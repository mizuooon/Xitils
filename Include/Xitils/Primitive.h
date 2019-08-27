#pragma once

#include "Utils.h"
#include "Vector.h"
#include "Bounds.h"
#include "Sampler.h"
#include "Transform.h"
#include "Ray.h"
#include "Interaction.h"

namespace Xitils {

	class Shape;

	class Primitive {
	public:

		virtual Bounds3f bound() const = 0;

		virtual float surfaceArea() const = 0;

		virtual bool intersect(const Ray& ray, float* tHit, SurfaceInteraction* isect) const = 0;
		virtual bool intersectAny(const Ray& ray) const = 0;

		virtual float surfacePDF(const Vector3f& p, const Shape* shape) const = 0;
	};

}