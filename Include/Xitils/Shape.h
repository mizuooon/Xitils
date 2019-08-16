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

	class Triangle : public Shape {
	public:
		Triangle(const Transform& objectToWorld) :
			Shape(objectToWorld)
		{}

		Vector3f p[3];

		Bounds3f bound() const override {
			return Bounds3f(p[0], p[1], p[2]);
		}

		float surfaceArea() const override {
			auto v01 = p[1] - p[0];
			auto v02 = p[2] - p[0];
			return cross(v01, v02).length() / 2.0f;
		}

		bool intersect(const Ray& ray, float* tHit, SurfaceInteraction* isect) const override {

			Ray rayObj = objectToWorld.inverse(ray);

			const auto& o = rayObj.o;
			const auto& d = rayObj.d;

			const auto& p0 = p[0];
			const auto& p1 = p[1];
			const auto& p2 = p[2];

			auto v01 = p1 - p0;
			auto v02 = p2 - p0;
			auto v12 = p2 - p1;

			auto n = cross(v02, v01);
			if (!n.isZero()) { n.normalize(); }
			//if (dot(n, d) > 0.0f) { n *= -1.0f; }

			float d_dot_n = dot(d, n);
			if (fabs(d_dot_n) < 0.0000001f) { return false; }

			float t = dot(p0 - o, n) / d_dot_n;

			if (!inRange(t, 0.0f, rayObj.tMax)) { return false; }

			auto p = rayObj(t);

			const auto vop0 = p0 - o;
			const auto vop1 = p1 - o;
			const auto vop2 = p2 - o;

			float V0 = dot(p - o, cross(vop1, vop2));
			float V1 = dot(p - o, cross(vop2, vop0));
			float V2 = dot(p - o, cross(vop0, vop1));
			float V = V0 + V1 + V2;
			float invV = 1.0f / V;
			Vector3f w;
			w[0] = V0 * invV;
			w[1] = V1 * invV;
			w[2] = V2 * invV;

			if (!inRange01(w)) { return false; }

			if (tHit) { *tHit = t; }
			if (isect) {
				isect->p = objectToWorld(p);
				isect->n = objectToWorld.asNormal(n);
			}

			return true;
		}
	};
}