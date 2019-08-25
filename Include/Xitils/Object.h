#pragma once

#include "Bounds.h"
#include "Interaction.h"
#include "Material.h"
#include "Transform.h"
#include "Utils.h"
#include "Primitive.h"
#include "Shape.h"
#include "Ray.h"
#include "Vector.h"

namespace Xitils {

	class Object : public Primitive {
	public:

		Transform objectToWorld;
		std::shared_ptr<Shape> shape;
		std::shared_ptr<Material> material;

		Object(const std::shared_ptr<Shape>& shape, const std::shared_ptr<Material>& material, const Transform& objectToWorld):
			shape(shape), material(material), objectToWorld(objectToWorld)
		{}

		Bounds3f bound() const override {
			return objectToWorld(shape->bound());
		}

		float surfaceArea() const override {

			// TODO Transformによる面積の変化を考慮
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

				isect->object = this;

				return true;
			}
			return false;
		}
		bool intersectAny(const Ray& ray) const override {
			return shape->intersectAny(objectToWorld.inverse(ray));
		}

	};

}