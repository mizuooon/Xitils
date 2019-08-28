#pragma once

#include "Utils.h"
#include "Vector.h"
#include "Bounds.h"
#include "Transform.h"
#include "Ray.h"
#include "Interaction.h"
#include "Primitive.h"

namespace Xitils {

	class Shape : public Geometry {
	public:

		struct SampledSurface {
			const Shape* shape;
			const Primitive* primitive;
			Vector3f p;
			Vector3f n;
			Vector3f shadingN;
		};

		virtual float surfaceAreaScaling(const Transform& t) const = 0;

		virtual SampledSurface sampleSurface(const std::shared_ptr<Sampler>& sampler, float* pdf) const = 0;
		virtual float surfacePDF(const Vector3f& p, const Primitive* primitive) const = 0;
	};

}