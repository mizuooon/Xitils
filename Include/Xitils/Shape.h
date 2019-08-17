#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <optional>

#include "Utils.h"
#include "Vector.h"
#include "Bounds.h"
#include "Transform.h"
#include "Ray.h"
#include "Interaction.h"

namespace Xitils {

	class Shape {
	public:

		Transform objectToWorld;

		Shape(const Transform& objectToWorld):
			objectToWorld(objectToWorld)
		{}

		virtual Bounds3f bound() const = 0;
		virtual Bounds3f worldBound() const {
			return objectToWorld(bound());
		}

		virtual float surfaceArea() const = 0;

		virtual bool intersect(const Ray& ray, float* tHit, SurfaceInteraction* isect) const = 0;
		virtual bool intersectBool(const Ray& ray) const {
			float tHit = ray.tMax;
			SurfaceInteraction isect;
			return intersect(ray, &tHit, &isect);
		}

		bool intersectBound(const Ray& ray, float* t1, float* t2) const {
			return bound().intersect(objectToWorld.inverse(ray), t1, t2);
		}
	};

	//class Triangle : public Shape {
	//public:

	//	Vector3f p[3];

	//	Triangle(const Transform& objectToWorld) :
	//		Shape(objectToWorld)
	//	{}

	//	Bounds3f bound() const override {
	//		const auto p0 = objectToWorld(p[0]);
	//		const auto p1 = objectToWorld(p[1]);
	//		const auto p2 = objectToWorld(p[2]);
	//		return Bounds3f(p0, p1, p2);
	//	}

	//	float surfaceArea() const override {
	//		const auto p0 = objectToWorld(p[0]);
	//		const auto p1 = objectToWorld(p[1]);
	//		const auto p2 = objectToWorld(p[2]);
	//		auto v01 = p1 - p0;
	//		auto v02 = p2 - p0;
	//		return cross(v01, v02).length() / 2.0f;
	//	}

	//	bool intersect(const Ray& ray, float* tHit, SurfaceInteraction* isect) const override {

	//		Ray rayObj = objectToWorld.inverse(ray);

	//		const auto& o = rayObj.o;
	//		const auto& d = rayObj.d;

	//		const auto& p0 = p[0];
	//		const auto& p1 = p[1];
	//		const auto& p2 = p[2];

	//		auto v01 = p1 - p0;
	//		auto v02 = p2 - p0;
	//		auto v12 = p2 - p1;

	//		auto n = cross(v02, v01);
	//		if (!n.isZero()) { n.normalize(); }
	//		//if (dot(n, d) > 0.0f) { n *= -1.0f; }

	//		float d_dot_n = dot(d, n);
	//		if (fabs(d_dot_n) < 0.0000001f) { return false; }

	//		float t = dot(p0 - o, n) / d_dot_n;

	//		if (!inRange(t, 0.0f, rayObj.tMax)) { return false; }

	//		auto p = rayObj(t);

	//		const auto vop0 = p0 - o;
	//		const auto vop1 = p1 - o;
	//		const auto vop2 = p2 - o;

	//		float V0 = dot(p - o, cross(vop1, vop2));
	//		float V1 = dot(p - o, cross(vop2, vop0));
	//		float V2 = dot(p - o, cross(vop0, vop1));
	//		float V = V0 + V1 + V2;
	//		float invV = 1.0f / V;
	//		Vector3f w;
	//		w[0] = V0 * invV;
	//		w[1] = V1 * invV;
	//		w[2] = V2 * invV;

	//		if (!inRange01(w)) { return false; }

	//		if (tHit) { *tHit = t; }
	//		if (isect) {
	//			isect->p = objectToWorld(p);
	//			isect->n = objectToWorld.asNormal(n);
	//		}

	//		return true;
	//	}
	//};


	class ShapeLocal {
	public:

		ShapeLocal()
		{}

		virtual Bounds3f bound() const = 0;

		virtual float surfaceArea() const = 0;

		virtual bool intersect(const Ray& ray, float* tHit, SurfaceInteraction* isect) const = 0;
		virtual bool intersectBool(const Ray& ray) const {
			float tHit = ray.tMax;
			SurfaceInteraction isect;
			return intersect(ray, &tHit, &isect);
		}

		bool intersectBound(const Ray& ray, float* t1, float* t2) const {
			return bound().intersect(ray, t1, t2);
		}
	};

	class TriangleIndexed : public ShapeLocal {
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
			isect->n = cross(p2 - p0, p1 - p0).normalize();
			
			if (normals == nullptr) {
				isect->shading.n = isect->n;
			} else {
				isect->shading.n = lerp(normal(0), normal(1), normal(2), b0, b1).normalize();
			}

			return true;
		}
	};


}