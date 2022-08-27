﻿#pragma once

#include "Bounds.h"
#include "Interaction.h"
#include "Transform.h"
#include "TriangleIndexed.h"
#include "Utils.h"
#include "Ray.h"
#include "Vector.h"

namespace xitils {

	class Shape : public Geometry {
	public:

		struct SampledSurface {
			const Shape* shape;
			const TriangleIndexed* tri;
			Vector3f p;
			Vector3f n;
			Vector3f shadingN;
		};

		virtual float surfaceAreaScaling(const Transform& t) const = 0;

		virtual SampledSurface sampleSurface(Sampler& sampler, float* pdf) const = 0;
		virtual float surfacePDF(const Vector3f& p, const TriangleIndexed* tri) const = 0;
	};

	class Sphere : public Shape {
	public:

		Vector2f uvScale;

		Sphere(const Vector2f& uvScale = Vector2f(1.0f)):
			uvScale(uvScale)
		{}

		Bounds3f bound() const override {
			return Bounds3f(Vector3f(-1.0f), Vector3f(1.0f));
		}

		float surfaceArea() const override {
			return 4 * M_PI;
		}

		float surfaceAreaScaling(const Transform& t) const {
			// TODO
			NOT_IMPLEMENTED;

			return 0.0f;
		}

		bool intersect(const Ray& ray, float* tHit, SurfaceIntersection* isect) const override {

			float A = ray.d.lengthSq();
			float B = dot(ray.d, ray.o);
			float C = ray.o.lengthSq() - 1;

			float discriminant = B * B - A * C;
			if (discriminant < 0.0f) {
				return false;
			}

			bool back = false;
			float t = (-B - sqrtf(discriminant)) / A;
			if (t < 0.0f) {
				t = (-B + sqrtf(discriminant)) / A;
				if (t < 0.0f) {
					return false;
				}
				back = true;
			}

			*tHit = t;
			isect->p = ray(t).normalize();
			isect->n = isect->p;

			isect->wo = normalize(-ray.d);

			isect->texCoord.u = atan2f(isect->n.x, isect->n.z);
			if (isect->texCoord.u < 0.0f) { isect->texCoord.u += 2 * M_PI; }
			isect->texCoord.u = clamp01(isect->texCoord.u / (2 * M_PI));
			isect->texCoord.v = clamp01((-asinf(isect->n.y) + M_PI / 2) / M_PI);

			isect->texCoord *= uvScale;
			isect->texCoord.u = isect->texCoord.u - floorf(isect->texCoord.u);
			isect->texCoord.v = isect->texCoord.v - floorf(isect->texCoord.v);

			isect->tangent = cross(Vector3f(0, -1, 0) - isect->p, Vector3f(0, 1, 0) - isect->p).normalize();
			isect->bitangent = cross(isect->n, isect->tangent).normalize();
			isect->shading.tangent = isect->tangent;
			isect->shading.bitangent = isect->bitangent;

			isect->shading.n = isect->n;
			if (back) { isect->shading.n *= -1; }

			isect->shape = this;
			isect->tri = nullptr;

			return true;
		}

		SampledSurface sampleSurface(Sampler& sampler, float* pdf) const override {
			float r1 = sampler.randf();
			float r2 = sampler.randf();
			SampledSurface sample;
			sample.p.x = 2.0f * 1 * cosf(2.0f * M_PI * r1) * sqrtf(r2 * (1.0f - r2));
			sample.p.y = 2.0f * 1 * (1.0f - 2.0f * r2);
			sample.p.z = 2.0f * 1 * sinf(2.0f * M_PI * r1) * sqrtf(r2 * (1.0f - r2));
			sample.n = normalize(sample.p);
			sample.shadingN = sample.n;
			*pdf = 1.0f / surfaceArea();
			return sample;
		}

		float surfacePDF(const Vector3f& p, const TriangleIndexed* tri) const override {
			// tri �͎g�p���Ȃ�
			return 1.0f / surfaceArea();
		}

	};

}