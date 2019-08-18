#pragma once

#include "Utils.h"
#include "Vector.h"
#include "Bounds.h"
#include "Transform.h"
#include "Ray.h"
#include "Interaction.h"
#include "Shape.h"

namespace Xitils {

	class Object {
	public:

		Transform objectToWorld;
		std::shared_ptr<Shape> shape;
		//std::shared_ptr<Material> material;

		Object(std::shared_ptr<Shape> shape, const Transform& objectToWorld):
			shape(shape), objectToWorld(objectToWorld)
		{}

		Bounds3f bound() const {
			return objectToWorld(shape->bound());
		}

		float surfaceArea() const {

			// TODO TransformÇ…ÇÊÇÈñ êœÇÃïœâªÇçló∂
			NOT_IMPLEMENTED 

			return shape->surfaceArea();
		}

		bool intersect(const Ray& ray, float* tHit, SurfaceInteraction* isect) const {
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
		bool intersectBool(const Ray& ray) const {
			float tHit = ray.tMax;
			SurfaceInteraction isect;
			return intersect(objectToWorld.inverse(ray), &tHit, &isect);
		}

		//bool intersectBound(const Ray& ray, float* t1, float* t2) const {
		//	Ray rayObj = objectToWorld.inverse(ray);
		//	if (shape->bound().intersect(rayObj, t1, t2)) {
		//		// TODO çÇë¨âª
		//		*t1 = (objectToWorld(rayObj(*t1)) - ray.o).length();
		//		*t2 = (objectToWorld(rayObj(*t2)) - ray.o).length();
		//		return true;
		//	}
		//	return false;
		//}
	};

}