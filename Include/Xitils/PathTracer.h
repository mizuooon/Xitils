#pragma once

#include "Interaction.h"
#include "Ray.h"
#include "Sampler.h"
#include "Scene.h"
#include "Utils.h"

namespace xitils {

	struct PathTracerEvalResult
	{
		Vector3f color;
		Vector3f albedo;
		Vector3f normal;
	};

	class PathTracer {
	public:
		virtual PathTracerEvalResult eval(const Scene& scene, Sampler& sampler, const Ray& ray) const = 0;
	};

	class DebugRayCaster : public PathTracer {
	public:

		DebugRayCaster(std::function<PathTracerEvalResult(const SurfaceIntersection&, Sampler&)> f) :
			f(f)
		{}

		PathTracerEvalResult eval(const Scene& scene, Sampler& sampler, const Ray& ray) const override {

			PathTracerEvalResult res;
			SurfaceIntersection isect;

			Ray tmpRay(ray);
			if (scene.intersect(tmpRay, &isect)) {
				res = f(isect, sampler);
			}

			return res;
		}

	private:
		std::function<PathTracerEvalResult(const SurfaceIntersection&, Sampler&)> f;
	};

	class NaivePathTracer : public PathTracer {
	public:

		PathTracerEvalResult eval(const Scene& scene, Sampler& sampler, const Ray& ray) const override {

			PathTracerEvalResult res;
			Vector3f& radiance = res.color;
			Vector3f& albedo = res.albedo;
			Vector3f& normal = res.normal;

			Vector3f weight(1.0f);

			Ray currentRay = ray;
			SurfaceIntersection isect;

			int pathLength = 1;

			Vector3f wi;
			while (true) {
				
				if (pathLength > russianRouletteLengthMin) {
					if(sampler.randf() >= russianRouletteProb){
						break;
					} else {
						weight /= russianRouletteProb;
					}
				}

				if (scene.intersect(currentRay, &isect)) {
					if(pathLength == 1)
					{
						albedo += isect.object->material->getAlbedo(isect);
						normal += isect.n;
					}

					if (isect.object->material->emissive) {
						radiance += weight * isect.object->material->getEmission(-currentRay.d, isect.n, isect.shading.n);
					}

					float pdf;
					weight *= isect.object->material->evalAndSample(isect, sampler, &wi, &pdf);

					if (weight.isZero()) { break; }

					currentRay.d = wi;
					currentRay.o = isect.p + rayOriginOffset * currentRay.d;
					currentRay.tMax = Infinity;

				} else if (scene.skySphere) {
					radiance += clampPositive( weight * scene.skySphere->getRadiance(currentRay.d) );
					break;
				}

				++pathLength;
			}

			return res;
		}

	private:
		float rayOriginOffset = 0.00001f;
		int russianRouletteLengthMin = 5;
		float russianRouletteProb = 0.9f;
	};

	class StandardPathTracer : public PathTracer {
	public:

		// NEE と BRDF サンプリングの MIS

		PathTracerEvalResult eval(const Scene& scene, Sampler& sampler, const Ray& ray) const override {

			PathTracerEvalResult res;
			Vector3f& radiance = res.color;
			Vector3f& albedo = res.albedo;
			Vector3f& normal = res.normal;

			Vector3f weight(1.0f);

			Ray currentRay = ray;
			Ray shadowRay;
			SurfaceIntersection isect;
			SurfaceIntersection nextIsect;

			int pathLength = 1;

			if (!scene.intersect(currentRay, &isect)) {
				if (scene.skySphere) {
					radiance += weight * scene.skySphere->getRadiance(currentRay.d);
				}
			}else{
				albedo += isect.object->material->getAlbedo(isect);
				normal += isect.n;

				++pathLength;

				if (isect.object->material->emissive) {
					radiance += weight * isect.object->material->getEmission(-currentRay.d, isect.n, isect.shading.n);
				}
				
				while (true) {

					if (pathLength > russianRouletteLengthMin) {
						if (sampler.randf() >= russianRouletteProb) {
							break;
						} else {
							weight /= russianRouletteProb;
						}
					}

					//-------------------------------------

					bool nextSampled = true;

					float pdf_bsdf_x_bsdf;
					Vector3f material_eval;
					material_eval = isect.object->material->evalAndSample(isect, sampler, &currentRay.d, &pdf_bsdf_x_bsdf);

					if (!material_eval.isZero()) {
						currentRay.o = isect.p + rayOriginOffset * currentRay.d;
						currentRay.tMax = Infinity;

						if (scene.intersect(currentRay, &nextIsect)) {

							if (nextIsect.object->material->emissive) {
								float misWeight;
								if (pdf_bsdf_x_bsdf >= 0.0f && scene.canSampleLight()) {
									float pdf_light_x_bsdf = scene.surfacePDF(nextIsect.p, nextIsect.object, nextIsect.shape, nextIsect.tri);
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
										* nextIsect.object->material->getEmission(-currentRay.d, nextIsect.n, nextIsect.shading.n)
										;
								}

							}

						} else {
							if (scene.skySphere) {
								radiance += weight * material_eval * scene.skySphere->getRadiance(currentRay.d);
							}
							nextSampled = false;
						}
					}

					//-------------------------------------

					if (scene.canSampleLight()) {
						float pdf_light_x_light;
						const auto& sampledLightSurface = scene.sampleSurface(sampler, &pdf_light_x_light);
						float sampledLightSurfaceDist = (sampledLightSurface.p - isect.p).length();
						shadowRay.d = (sampledLightSurface.p - isect.p) / sampledLightSurfaceDist;
						shadowRay.o = isect.p + rayOriginOffset * shadowRay.d;
						shadowRay.tMax = sampledLightSurfaceDist - shadowRayMargin;
						if (dot(shadowRay.d, sampledLightSurface.n) < 0 && !scene.intersectAny(shadowRay)) {
							float misWeight;
							float pdf_bsdf_x_light;
							float distSq = powf(sampledLightSurfaceDist, 2.0f);
							if (pdf_bsdf_x_bsdf >= 0.0f) {
								pdf_bsdf_x_light = isect.object->material->getPDF(isect, shadowRay.d);
								float cosLight = fabsf(dot(-shadowRay.d, sampledLightSurface.shadingN));
								pdf_bsdf_x_light *= cosLight / distSq;

								misWeight = powf(pdf_light_x_light, 2.0f) / (powf(pdf_bsdf_x_light, 2.0f) + powf(pdf_light_x_light, 2.0f));
							} else {
								misWeight = 0.0f;
							}

							if (misWeight > 0.0f) {
								float G = fabs(dot(-shadowRay.d, sampledLightSurface.shadingN)) / distSq; // bsdfCos にオブジェクト側のコサイン項は既に含まれている
								radiance +=
									weight * misWeight
									* isect.object->material->bsdfCos(isect, sampler, shadowRay.d)
									* sampledLightSurface.object->material->getEmission(-shadowRay.d, sampledLightSurface.n, sampledLightSurface.shadingN)
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

			return res;
		}

	private:
		float rayOriginOffset = 0.00001f;
		int russianRouletteLengthMin = 5;
		float russianRouletteProb = 0.9f;
		float shadowRayMargin = 0.0001f;
	};

	// シングルスキャッタリングのみ表示
	class DebugRayTracerSingleScattering : public PathTracer {
	public:

		PathTracerEvalResult eval(const Scene& scene, Sampler& sampler, const Ray& ray) const override {

			PathTracerEvalResult res;
			Vector3f& radiance = res.color;
			Vector3f& albedo = res.albedo;
			Vector3f& normal = res.normal;

			Vector3f weight(1.0f);

			Ray currentRay = ray;
			SurfaceIntersection isect;

			int pathLength = 1;

			Vector3f wi;
			while (true) {
				if (pathLength > 2) { break; }

				if (pathLength > russianRouletteLengthMin) {
					if (sampler.randf() >= russianRouletteProb) {
						break;
					} else {
						weight /= russianRouletteProb;
					}
				}

				if (scene.intersect(currentRay, &isect)) {
					if (isect.object->material->emissive) {
						radiance += weight * isect.object->material->getEmission(-currentRay.d, isect.n, isect.shading.n);
					}

					float pdf;
					weight *= isect.object->material->evalAndSample(isect, sampler, &wi, &pdf);

					if (weight.isZero()) { break; }

					currentRay.d = wi;
					currentRay.o = isect.p + rayOriginOffset * currentRay.d;
					currentRay.tMax = Infinity;

				} else if (scene.skySphere) {
					radiance += clampPositive(weight * scene.skySphere->getRadiance(currentRay.d));
					break;
				}

				++pathLength;
			}

			return res;
		}

	private:
		float rayOriginOffset = 0.00001f;
		int russianRouletteLengthMin = 5;
		float russianRouletteProb = 0.9f;
	};
}