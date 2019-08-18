#pragma once

#include "Utils.h"
#include "Vector.h"
#include "Bounds.h"
#include "Transform.h"
#include "Ray.h"
#include "Interaction.h"
#include "Primitive.h"
#include "Shape.h"

namespace Xitils {

	class Object : public Primitive {
	public:

		Transform objectToWorld;
		std::shared_ptr<Shape> shape;
		//std::shared_ptr<Material> material;

		Object(std::shared_ptr<Shape> shape, const Transform& objectToWorld):
			shape(shape), objectToWorld(objectToWorld)
		{}

		Bounds3f bound() const override {
			return objectToWorld(shape->bound());
		}

		float surfaceArea() const override {

			// TODO Transform‚É‚æ‚é–ÊÏ‚Ì•Ï‰»‚ðl—¶
			NOT_IMPLEMENTED 

			return shape->surfaceArea();
		}

		bool intersect(const Ray& ray, float* tHit, SurfaceInteraction* isect) const override {
			if (shape->intersect(objectToWorld.inverse(ray), tHit, isect)) {
				isect->p = objectToWorld(isect->p);
				isect->n = objectToWorld.asNormal(isect->n);
				isect->wo = objectToWorld.asNormal(isect->wo);
				isect->shading.n = objectToWorld.asNormal(isect->shading.n);
				*tHit = (isect->p - ray.o).length();
				return true;
			}
			return false;
		}
		bool intersectBool(const Ray& ray) const override {
			float tHit = ray.tMax;
			SurfaceInteraction isect;
			return intersect(objectToWorld.inverse(ray), &tHit, &isect);
		}

	};

}