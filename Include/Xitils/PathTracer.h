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
				
				if (pathLength > russianRouletteLengthMin) {
					if(sampler->randf() >= russianRouletteProb){
						break;
					} else {
						weight /= russianRouletteProb;
					}
				}

				if (scene->intersect(currentRay, &isect)) {
					if (isect.object->material->emissive) {
						radiance += weight * isect.object->material->emission(-currentRay.d, isect.n, isect.shading.n);
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
		float rayOriginOffset = 0.00001f;
		int russianRouletteLengthMin = 5;
		float russianRouletteProb = 0.9f;
	};

	class StandardPathTracer {
	public:

		// NEE ‚Æ BRDF ƒTƒ“ƒvƒŠƒ“ƒO‚Ì MIS

		Vector3f eval(const std::shared_ptr<Scene>& scene, const std::shared_ptr<Sampler>& sampler, const Ray& ray) const {

			Vector3f radiance;
			Vector3f weight(1.0f);

			Ray currentRay = ray;
			Ray shadowRay;
			SurfaceInteraction isect;
			SurfaceInteraction nextIsect;

			int pathLength = 1;

			if (!scene->intersect(currentRay, &isect)) {
				if (scene->skySphere) {
					radiance += weight * scene->skySphere->getRadiance(currentRay.d);
				}
			}else{

				++pathLength;

				if (isect.object->material->emissive) {
					radiance += weight * isect.object->material->emission(-currentRay.d, isect.n, isect.shading.n);
				}
				
				while (true) {

					if (pathLength > russianRouletteLengthMin) {
						if (sampler->randf() >= russianRouletteProb) {
							break;
						} else {
							weight /= russianRouletteProb;
						}
					}

					//-------------------------------------

					bool nextSampled = true;

					float pdf_bsdf_x_bsdf;
					Vector3f material_eval;
					if (!isect.object->material->specular) {
						material_eval = isect.object->material->evalAndSample(isect, sampler, &currentRay.d, &pdf_bsdf_x_bsdf);
					} else {
						material_eval = isect.object->material->evalAndSampleSpecular(isect, sampler, &currentRay.d);
					}

					if (!material_eval.isZero()) {
						currentRay.o = isect.p + rayOriginOffset * currentRay.d;
						currentRay.tMax = Infinity;

						if (scene->intersect(currentRay, &nextIsect)) {

							if (nextIsect.object->material->emissive) {
								float misWeight;
								if (!isect.object->material->specular && scene->canSampleLight()) {
									float pdf_light_x_bsdf = scene->surfacePDF(nextIsect.p, nextIsect.object, nextIsect.shape, nextIsect.primitive);
									float cosLight = fabsf(dot(currentRay.d, nextIsect.shading.n));
									float distSq = powf(currentRay.tMax, 2.0f);
									pdf_light_x_bsdf *= distSq / cosLight;
									misWeight = powf(pdf_bsdf_x_bsdf, 2.0f) / (powf(pdf_bsdf_x_bsdf, 2.0f) + powf(pdf_light_x_bsdf, 2.0f));
								} else {
									misWeight = 1.0f;
								}

								if (misWeight > 0.0f) {
									radiance += weight * misWeight
										* material_eval
										* nextIsect.object->material->emission(-currentRay.d, nextIsect.n, nextIsect.shading.n)
										;
								}

							}

						} else {
							if (scene->skySphere) {
								radiance += weight * material_eval * scene->skySphere->getRadiance(currentRay.d);
							}
							nextSampled = false;
						}
					}

					//-------------------------------------

					if (scene->canSampleLight()) {
						float pdf_light_x_light;
						const auto& sampledLightSurface = scene->sampleSurface(sampler, &pdf_light_x_light);
						float sampledLightSurfaceDist = (sampledLightSurface.p - isect.p).length();
						shadowRay.d = (sampledLightSurface.p - isect.p) / sampledLightSurfaceDist;
						shadowRay.o = isect.p + rayOriginOffset * shadowRay.d;
						shadowRay.tMax = sampledLightSurfaceDist - shadowRayMargin;
						if (dot(shadowRay.d, sampledLightSurface.n) < 0 && !scene->intersectAny(shadowRay)) {
							float misWeight;
							float pdf_bsdf_x_light;
							float distSq = powf(sampledLightSurfaceDist, 2.0f);
							if (!isect.object->material->specular) {
								pdf_bsdf_x_light = isect.object->material->pdf(isect, shadowRay.d);
								float cosLight = fabsf(dot(-shadowRay.d, sampledLightSurface.shadingN));
								pdf_bsdf_x_light *= cosLight / distSq;

								misWeight = powf(pdf_light_x_light, 2.0f) / (powf(pdf_bsdf_x_light, 2.0f) + powf(pdf_light_x_light, 2.0f));
							} else {
								misWeight = 0.0f;
							}

							if (misWeight > 0.0f) {
								float G = fabs(dot(shadowRay.d, isect.shading.n) * dot(-shadowRay.d, sampledLightSurface.shadingN)) / distSq;
								radiance +=
									weight * misWeight
									* isect.object->material->bsdf(isect, shadowRay.d)
									* sampledLightSurface.object->material->emission(-shadowRay.d, sampledLightSurface.n, sampledLightSurface.shadingN)
									* G / pdf_light_x_light;
							}

						}
					}

					//-------------------------------------
					
					if (!nextSampled) { break; }

					isect = nextIsect;
					weight *= material_eval;
					++pathLength;

					if (weight.isZero()) {
						break;
					}
				}

			}

			return radiance;
		}

	private:
		float rayOriginOffset = 0.00001f;
		int russianRouletteLengthMin = 5;
		float russianRouletteProb = 0.9f;
		float shadowRayMargin = 0.0001f;
	};

}