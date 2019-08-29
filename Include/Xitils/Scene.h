#pragma once

#include "AccelerationStructure.h"
#include "Camera.h"
#include "Shape.h"
#include "SkySphere.h"
#include "Utils.h"

namespace Xitils {

	class Scene {
	public:

		std::shared_ptr<Camera> camera;
		std::shared_ptr<SkySphere> skySphere;

		void addObject(const std::shared_ptr<Object>& object) {
			objects.push_back(object);
			if (object->material->emissive) {
				lights.push_back(object);
			}
		}

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

		bool canSampleLight() const { return !lights.empty(); }

		Object::SampledSurface sampleSurface(const std::shared_ptr<Sampler>& sampler, float* pdf) const {
			// TODO: すべての Object から等確率でサンプリングしているが、
			//       本当は面積に比例した確率で選ぶべき
			auto& light = sampler->select(lights);
			auto res = light->sampleSurface(sampler, pdf);
			*pdf /= lights.size();
			return res;
		}

		float surfacePDF(const Vector3f& p, const Object* object, const Shape* shape, const Primitive* primitive) const {
			// TODO: 上記を直したらこちらも直す

			if (lights.empty()) { return 0.0f; }

			return object->surfacePDF(p, shape, primitive) / lights.size();
		}

	private:
		std::shared_ptr<AccelerationStructure> accel;
		std::vector<std::shared_ptr<Object>> objects;
		std::vector<std::shared_ptr<Object>> lights;
	};

}