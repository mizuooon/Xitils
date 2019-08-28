#pragma once

#include "Bounds.h"
#include "Geometry.h"
#include "Interaction.h"
#include "Material.h"
#include "Transform.h"
#include "Utils.h"
#include "Primitive.h"
#include "Shape.h"
#include "Ray.h"
#include "Vector.h"

namespace Xitils {

	class Object : public Geometry {
	public:

		struct SampledSurface {
			const Object* object;
			const Shape* shape;
			const Primitive* primitive;
			Vector3f p;
			Vector3f n;
			Vector3f shadingN;
		};

		Transform objectToWorld;
		std::shared_ptr<Shape> shape;
		std::shared_ptr<Material> material;

		Object(const std::shared_ptr<Shape>& shape, const std::shared_ptr<Material>& material, const Transform& objectToWorld):
			shape(shape), material(material), objectToWorld(objectToWorld)
		{}

		Bounds3f bound() const override {
			return objectToWorld(shape->bound());
		}

		float surfaceArea() const {
			return shape->surfaceArea() * shape->surfaceAreaScaling(objectToWorld);
		}

		bool intersect(const Ray& ray, float* tHit, SurfaceInteraction* isect) const override {
			if (shape->intersect(objectToWorld.inverse(ray), tHit, isect)) {
				isect->p = objectToWorld(isect->p);
				isect->n = objectToWorld.asNormal(isect->n);
				isect->wo = objectToWorld.asNormal(isect->wo);
				isect->shading.n = objectToWorld.asNormal(isect->shading.n);
				*tHit = (isect->p - ray.o).length();

				isect->object = this;

				return true;
			}
			return false;
		}

		bool intersectAny(const Ray& ray) const override {
			return shape->intersectAny(objectToWorld.inverse(ray));
		}

		SampledSurface sampleSurface(const std::shared_ptr<Sampler>& sampler, float* pdf) const {
			auto sampled = shape->sampleSurface(sampler, pdf);

			*pdf /= shape->surfaceAreaScaling(objectToWorld);

			SampledSurface res;
			res.object = this;
			res.shape = sampled.shape;
			res.primitive = sampled.primitive;
			res.p = objectToWorld(sampled.p);
			res.n = objectToWorld.asNormal(sampled.n);
			res.shadingN = objectToWorld.asNormal(sampled.shadingN);

			return res;
		}

		float surfacePDF(const Vector3f& p, const Shape* shape, const Primitive* primitive) const {
			Vector3f scaling = objectToWorld.getScaling();
			return shape->surfacePDF(objectToWorld.inverse(p), primitive) / shape->surfaceAreaScaling(objectToWorld);
		}

	};

}