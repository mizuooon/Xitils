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
						radiance += weight * isect.object->material->emission(-currentRay.d, isect.shading.n);
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
					radiance += clampPositive( weight * scene->skySphere->getRadiance(currentRay.d) );
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

	class NEEPathTracer {
	public:

		Vector3f eval(const std::shared_ptr<Scene>& scene, const std::shared_ptr<Sampler>& sampler, const Ray& ray) const {

			Vector3f radiance;
			Vector3f weight(1.0f);

			Ray currentRay = ray;
			Ray nextRay;
			Ray shadowRay;
			SurfaceInteraction isect;
			SurfaceInteraction nextIsect;

			int pathLength = 1;

			Vector3f wi;
			while (true) {

				if (pathLength > russianRouletteLengthMin && sampler->randf() >= russianRouletteProb) {
					break;
				} else {
					weight /= russianRouletteProb;
				}

				//-------------------------------------
				if (scene->intersect(currentRay, &isect)) {

					float pdf_bsdf_x_bsdf;

					if (isect.object->material->specular) {
						//weight *= isect.object->material->evalAndSampleSpecular(isect, sampler, &wi);
					} else {
						weight *= isect.object->material->evalAndSample(isect, sampler, &wi, &pdf_bsdf_x_bsdf);
					}

					if (weight.isZero()) { break; }

					nextRay.d = wi;
					nextRay.o = isect.p + rayOriginOffset * currentRay.d;
					nextRay.tMax = Infinity;

					if (scene->intersect(nextRay, &nextIsect)) {
						float pdf_light_x_bsdf = scene->surfacePDF(nextIsect.p, nextIsect.object, nextIsect.shape);
						float cosLight = fabsf(dot(currentRay.d, isect.n));
						float distSq = powf(nextRay.tMax, 2.0f);
						pdf_light_x_bsdf *= distSq / cosLight;

						if (nextIsect.object->material->emissive) {
							float misWeight = powf(pdf_bsdf_x_bsdf, 2.0f) / (powf(pdf_bsdf_x_bsdf, 2.0f) + powf(pdf_light_x_bsdf, 2.0f));
							radiance += weight *  misWeight
								* isect.object->material->eval(isect, wi)
								* clampPositive(dot(wi, isect.n))
								* nextIsect.object->material->emission(-wi, nextIsect.shading.n) / pdf_bsdf_x_bsdf;
						}
					}

				} else if (scene->skySphere) {
					//radiance += clampPositive(weight * scene->skySphere->getRadiance(currentRay.d));
					break;
				}

				//-------------------------------------
				float pdf_light_x_light;
				const auto& sampledLightSurface = scene->sampleSurface(sampler, &pdf_light_x_light);
				float sampledLightSurfaceDist = (sampledLightSurface.p - isect.p).length();
				shadowRay.d = (sampledLightSurface.p - isect.p) / sampledLightSurfaceDist;
				shadowRay.o = isect.p + rayOriginOffset * shadowRay.d;
				shadowRay.tMax = Infinity;
				if (!scene->intersectAny(shadowRay)) {
					float pdf_bsdf_x_light = isect.object->material->pdf(isect, shadowRay.d);

					float cosLight = fabsf(dot(shadowRay.d, sampledLightSurface.shadingN));
					float distSq = powf(sampledLightSurfaceDist, 2.0f);
					pdf_bsdf_x_light *= cosLight / distSq;

					float misWeight = powf(pdf_light_x_light, 2.0f) / (powf(pdf_bsdf_x_light, 2.0f) + powf(pdf_light_x_light, 2.0f));

					float G = fabs(dot(isect.shading.n, shadowRay.d) * dot(sampledLightSurface.shadingN, -shadowRay.d)) / distSq;
					radiance +=
						misWeight *
						isect.object->material->eval(isect, shadowRay.d)
						* sampledLightSurface.object->material->emission(-shadowRay.d, sampledLightSurface.shadingN) * G / pdf_light_x_light;
				}


				//-------------------------------------
				currentRay = nextRay;
				isect = nextIsect;

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