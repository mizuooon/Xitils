#pragma once

#include "Interaction.h"
#include "Ray.h"
#include "Sampler.h"
#include "Scene.h"
#include "Utils.h"

namespace Xitils {

	class NaivePathTracer {
	public:

		Vector3f eval(const std::shared_ptr<Scene>& scene, const std::shared_ptr<Sampler>& sampler, const Ray& ray) const {

			Vector3f radiance;
			Vector3f weight(1.0f);

			Ray currentRay = ray;
			SurfaceInteraction isect;

			int pathLength = 1;

			Vector3f wi;
			while (true) {
				
				if (pathLength > russianRouletteLengthMin && sampler->randf() >= russianRouletteProb) {
					break;
				} else {
					weight /= russianRouletteProb;
				}

				if (scene->intersect(currentRay, &isect)) {
					if (isect.object->material->emissive) {
						radiance += weight * isect.object->material->emission(isect);
					}

					if (isect.object->material->specular) {
						weight *= isect.object->material->evalAndSampleSpecular(isect, sampler, &wi);
					} else {
						float pdf;
						weight *= isect.object->material->evalAndSample(isect, sampler, &wi, &pdf);
					}

					if (weight.isZero()) { break; }

					currentRay.d = wi;
					currentRay.o = isect.p + rayOriginOffset * currentRay.d;
					currentRay.tMax = Infinity;

				} else if (scene->skySphere) {
					radiance += weight * scene->skySphere->getRadiance(currentRay.d);
					break;
				}

				++pathLength;
			}

			return radiance;
		}

	private:
		float rayOriginOffset = 0.0001f;
		int russianRouletteLengthMin = 5;
		float russianRouletteProb = 0.9f;
	};

}