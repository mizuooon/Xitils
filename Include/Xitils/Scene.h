#pragma once

#include "AccelerationStructure.h"
#include "Camera.h"
#include "Shape.h"
#include "SkySphere.h"
#include "Utils.h"

namespace xitils {

	class Scene {
	public:

		std::shared_ptr<Camera> camera;
		std::shared_ptr<SkySphere> skySphere;

		void addObject(std::shared_ptr<Object> object) {
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

		bool intersect(Ray& ray, SurfaceIntersection* isect) const {
			return accel->intersect(ray, isect);
		}

		bool intersectAny(const Ray& ray) const {
			return accel->intersectAny(ray);
		}

		bool canSampleLight() const { return !lights.empty(); }

		Object::SampledSurface sampleSurface(Sampler& sampler, float* pdf) const {
			// TODO: ���ׂĂ� Object ���瓙�m���ŃT���v�����O���Ă��邪�A
			//       �{���͖ʐςɔ�Ⴕ���m���őI�Ԃׂ�
			auto& light = sampler.select(lights);
			auto res = light->sampleSurface(sampler, pdf);
			*pdf /= lights.size();
			return res;
		}

		float surfacePDF(const Vector3f& p, const Object* object, const Shape* shape, const TriangleIndexed* tri) const {
			// TODO: ��L�𒼂����炱��������

			if (lights.empty()) { return 0.0f; }

			return object->surfacePDF(p, shape, tri) / lights.size();
		}

	private:
		std::shared_ptr<AccelerationStructure> accel;
		std::vector<std::shared_ptr<Object>> objects;
		std::vector<std::shared_ptr<Object>> lights;
	};

}