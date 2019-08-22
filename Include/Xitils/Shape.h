#pragma once

#include "Utils.h"
#include "Vector.h"
#include "Bounds.h"
#include "Transform.h"
#include "Ray.h"
#include "Interaction.h"
#include "Primitive.h"

namespace Xitils {

	class Shape : public Primitive {
	public:
		virtual bool intersectAny(const Ray& ray) const override {
			float tHit = ray.tMax;
			SurfaceInteraction isect;
			return intersect(ray, &tHit, &isect);
		}
	};

	class TriangleIndexed : public Shape {
	public:

		const Vector3f* positions;
		const Vector3f* normals;
		const int* indices;
		int index;

		TriangleIndexed(const Vector3f* positions, const int *indices, int index) :
			positions(positions),
			normals(nullptr),
			indices(indices),
			index(index)
		{}

		TriangleIndexed(const Vector3f *positions, const Vector3f *normals, const int* indices, int index) :
			positions(positions),
			normals(normals),
			indices(indices),
			index(index)
		{}

		const Vector3f& position(int i) const { return positions[indices[index * 3 + i]]; }
		const Vector3f& normal(int i) const { return normals[indices[index * 3 + i]]; }

		Bounds3f bound() const override {
			const auto& p0 = position(0);
			const auto& p1 = position(1);
			const auto& p2 = position(2);
			return Bounds3f(p0, p1, p2);
		}

		float surfaceArea() const override {
			const auto& p0 = position(0);
			const auto& p1 = position(1);
			const auto& p2 = position(2);
			auto v01 = p1 - p0;
			auto v02 = p2 - p0;
			return cross(v01, v02).length() / 2.0f;
		}

		bool intersect(const Ray& ray, float* tHit, SurfaceInteraction* isect) const override {

			const auto& p0 = position(0);
			const auto& p1 = position(1);
			const auto& p2 = position(2);

			auto p0t = p0 - ray.o;
			auto p1t = p1 - ray.o;
			auto p2t = p2 - ray.o;

			int kz = abs(ray.d).maxDimension();
			int kx = kz + 1; if (kx == 3) { kx = 0; }
			int ky = kx + 1; if (ky == 3) { ky = 0; }
			auto d = permute(ray.d, kx, ky, kz);
			p0t = permute(p0t, kx, ky, kz);
			p1t = permute(p1t, kx, ky, kz);
			p2t = permute(p2t, kx, ky, kz);

			float sx = -d.x / d.z;
			float sy = -d.y / d.z;
			float sz = 1.0f / d.z;
			p0t.x += sx * p0t.z;
			p0t.y += sy * p0t.z;
			p1t.x += sx * p1t.z;
			p1t.y += sy * p1t.z;
			p2t.x += sx * p2t.z;
			p2t.y += sy * p2t.z;

			float e0 = p1t.x * p2t.y - p1t.y * p2t.x;
			float e1 = p2t.x * p0t.y - p2t.y * p0t.x;
			float e2 = p0t.x * p1t.y - p0t.y * p1t.x;

			if ((e0 < 0 || e1 < 0 || e2 < 0) && (e0 > 0 || e1 > 0 || e2 > 0)) {
				return false;
			}
			float det = e0 + e1 + e2;
			if (det == 0) { return false; }

			p0t.z *= sz;
			p1t.z *= sz;
			p2t.z *= sz;
			float tScaled = e0 * p0t.z + e1 * p1t.z + e2 * p2t.z;
			if (det < 0 && (tScaled >= 0 || tScaled < ray.tMax * det)) {
				return false;
			}
			if (det > 0 && (tScaled <= 0 || tScaled > ray.tMax * det)) {
				return false;
			}

			float invDet = 1 / det;
			float b0 = e0 * invDet;
			float b1 = e1 * invDet;
			float b2 = e2 * invDet;
			float t = tScaled * invDet;

			*tHit = t;
			isect->p = lerp(p0, p1, p2, b0, b1);
			isect->n = cross(p1 - p0, p2 - p0).normalize();
			
			if (normals == nullptr) {
				isect->shading.n = isect->n;
			} else {
				isect->shading.n = lerp(normal(0), normal(1), normal(2), b0, b1).normalize();
			}

			return true;
		}
	};


}