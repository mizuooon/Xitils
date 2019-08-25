#pragma once

#include "AccelerationStructure.h"
#include "Camera.h"
#include "SkySphere.h"
#include "Utils.h"

namespace Xitils {

	class Scene {
	public:

		std::shared_ptr<Camera> camera;
		std::vector<std::shared_ptr<Object>> objects;
		std::shared_ptr<SkySphere> skySphere;

		void buildAccelerationStructure() {
			std::vector<Object*> tmp;
			map<std::shared_ptr<Object>, Object*>(objects, &tmp, [](const std::shared_ptr<Object>& obj) { return obj.get(); });
			accel = std::make_shared<AccelerationStructure>(tmp);
		}

		bool intersect(Ray& ray, SurfaceInteraction* isect) const {
			return accel->intersect(ray, isect);
		}

		bool intersectAny(const Ray& ray) const {
			return accel->intersectAny(ray);
		}

	private:
		std::shared_ptr<AccelerationStructure> accel;
	};

}