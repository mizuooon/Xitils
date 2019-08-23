#pragma once

//#include <glm/glm.hpp>
//#include <glm/gtx/norm.hpp>
//#include <optional>

#include "Utils.h"
#include "Vector.h"
#include "Bounds.h"

namespace Xitils::Geometry {

	//struct Sphere {
	//	Vector3f center;
	//	float radius;
	//};

	//struct Segment {
	//	Vector3f p[2];
	//};

	//struct Triangle {
	//	Vector3f p[3];
	//};

	//inline Bounds3f getBoudingBox(const Triangle& triangle) {
	//	return Bounds3f(triangle.p[0], triangle.p[1], triangle.p[2]);
	//}

	//inline Bounds3f getBoudingBox(const Sphere& sphere) {
	//	return Bounds3f(
	//		sphere.center - Vector3f(sphere.radius, sphere.radius, sphere.radius),
	//		sphere.center + Vector3f(sphere.radius, sphere.radius, sphere.radius)
	//	);
	//}


	//namespace Intersection {

	//	namespace PointAABB {
	//		struct Intersection {
	//		};

	//		inline std::optional<Intersection> getIntersection(const Vector3f& p, const Bounds3f& aabb) {
	//			if (inRange(p.x, aabb.min.x, aabb.max.x) &&
	//				inRange(p.y, aabb.min.y, aabb.max.y) &&
	//				inRange(p.z, aabb.min.z, aabb.max.z)) {
	//				return Intersection();
	//			}
	//			return std::nullopt;
	//		}
	//	};


	//	namespace AABBAABB {
	//		struct Intersection {
	//		};

	//		inline std::optional<Intersection> getIntersection(const Bounds3f& aabb1, const Bounds3f& aabb2) {

	//			if (inRange(aabb1.min.x, aabb2.min.x, aabb2.max.x) &&
	//				inRange(aabb1.min.y, aabb2.min.y, aabb2.max.y) &&
	//				inRange(aabb1.min.z, aabb2.min.z, aabb2.max.z) ||
	//				inRange(aabb2.min.x, aabb1.min.x, aabb1.max.x) &&
	//				inRange(aabb2.min.y, aabb1.min.y, aabb1.max.y) &&
	//				inRange(aabb2.min.z, aabb1.min.z, aabb1.max.z)) {
	//				return Intersection();
	//			}

	//			if (aabb1.min.x <= aabb2.max.x &&
	//				aabb1.max.x >= aabb2.min.x &&
	//				aabb1.min.y >= aabb2.max.y &&
	//				aabb1.max.y <= aabb2.min.y &&
	//				aabb1.min.z >= aabb2.max.z &&
	//				aabb1.max.z <= aabb2.min.z) {
	//				return Intersection();
	//			}
	//			return std::nullopt;
	//		}
	//	};

	//	namespace RayAABB {
	//		struct Intersection {
	//			float t;
	//			Vector3f p;
	//			Vector3f n;
	//			float depth;
	//		};
	//		enum Option {
	//			None = 0b0000,
	//			IgnoreBack = 0b0001
	//		};

	//		inline std::optional<Intersection> getIntersection(const Ray& ray, const Bounds3f& aabb, Option opt = None) {

	//			const auto& o = ray.o;
	//			const auto& d = ray.d;
	//			Intersection result;

	//			bool back = false;
	//			if (inRange(o.x, aabb.min.x, aabb.max.x) && inRange(o.y, aabb.min.y, aabb.max.y) && inRange(o.z, aabb.min.z, aabb.max.z)) {
	//				back = true;
	//				if (opt & IgnoreBack) { return std::nullopt; }
	//			}

	//			bool xSide = (d.x < 0.0f) ^ back;
	//			bool ySide = (d.y < 0.0f) ^ back;
	//			bool zSide = (d.z < 0.0f) ^ back;
	//			float x0 = xSide ? aabb.max.x : aabb.min.x;
	//			float y0 = ySide ? aabb.max.y : aabb.min.y;
	//			float z0 = zSide ? aabb.max.z : aabb.min.z;
	//			float x1 = !xSide ? aabb.max.x : aabb.min.x;
	//			float y1 = !ySide ? aabb.max.y : aabb.min.y;
	//			float z1 = !zSide ? aabb.max.z : aabb.min.z;

	//			float validX = fabs(d.x) > 0.00001f;
	//			float validY = fabs(d.y) > 0.00001f;
	//			float validZ = fabs(d.z) > 0.00001f;

	//			float tx0 = validX ? (x0 - o.x) / d.x : -1.0f;
	//			float ty0 = validY ? (y0 - o.y) / d.y : -1.0f;
	//			float tz0 = validZ ? (z0 - o.z) / d.z : -1.0f;
	//			float tx1 = validX ? (x1 - o.x) / d.x : Infinity;
	//			float ty1 = validY ? (y1 - o.y) / d.y : Infinity;
	//			float tz1 = validZ ? (z1 - o.z) / d.z : Infinity;
	//			float t1 = min(tx1, ty1, tz1);

	//			Vector3f px = o + tx0 * d;
	//			Vector3f py = o + ty0 * d;
	//			Vector3f pz = o + tz0 * d;

	//			if (validX && tx0 >= 0.0f && inRange(px.y, aabb.min.y, aabb.max.y) && inRange(px.z, aabb.min.z, aabb.max.z)) {
	//				result.t = tx0;
	//				result.p = px;
	//				result.n = xSide ? Vector3f(1, 0, 0) : Vector3f(-1, 0, 0);
	//				result.depth = t1 - result.t;
	//				return std::move(result);
	//			}
	//			if (validY && ty0 >= 0.0f && inRange(py.z, aabb.min.z, aabb.max.z) && inRange(py.x, aabb.min.x, aabb.max.x)) {
	//				result.t = ty0;
	//				result.p = py;
	//				result.n = ySide ? Vector3f(0, 1, 0) : Vector3f(0, -1, 0);
	//				result.depth = t1 - result.t;
	//				return std::move(result);
	//			}
	//			if (validZ && tz0 >= 0.0f && inRange(pz.x, aabb.min.x, aabb.max.x) && inRange(pz.y, aabb.min.y, aabb.max.y)) {
	//				result.t = tz0;
	//				result.p = pz;
	//				result.n = zSide ? Vector3f(0, 0, 1) : Vector3f(0, 0, -1);
	//				result.depth = t1 - result.t;
	//				return std::move(result);
	//			}

	//			return std::nullopt;
	//		}
	//	};


	//	namespace RaySpehere {
	//		struct Intersection {
	//			float t;
	//			Vector3f p;
	//			Vector3f n;
	//		};
	//		enum Option {
	//			None = 0b0000,
	//			IgnoreBack = 0b0001
	//		};

	//		inline std::optional<Intersection> getIntersection(const Ray& ray, const Sphere& sphere, Option option = None) {

	//			float A = ray.d.lengthSq();
	//			float B = dot(ray.d, ray.o - sphere.center);
	//			float C = (ray.o - sphere.center).lengthSq() - sphere.radius * sphere.radius;

	//			float discriminant = B * B - A * C;
	//			if (discriminant < 0.0f) {
	//				return std::nullopt;
	//			}

	//			float t = (-B - sqrtf(discriminant)) / A;
	//			if (t < 0.0f) {
	//				t = (-B + sqrtf(discriminant)) / A;
	//				if (t < 0.0f) {
	//					return std::nullopt;
	//				}
	//				if (option & IgnoreBack) {
	//					return std::nullopt;
	//				}
	//			}

	//			Intersection result;
	//			result.p = ray.o + t * ray.d;
	//			result.t = t;
	//			result.n = normalize(result.p - sphere.center);

	//			return std::move(result);
	//		}
	//	}


	//	namespace RayTriangle {
	//		struct Intersection {
	//			float t;
	//			Vector3f p;
	//			Vector3f n;
	//			Vector3f w;
	//		};
	//		enum Option {
	//			None = 0b0000,
	//			//IgnoreBack = 0b0001
	//		};

	//		inline std::optional<Intersection> getIntersection(const Ray& ray, const Triangle& triangle, Option option = None) {

	//			const auto& o = ray.o;
	//			const auto& d = ray.d;
	//			const auto& p0 = triangle.p[0];
	//			const auto& p1 = triangle.p[1];
	//			const auto& p2 = triangle.p[2];

	//			auto v01 = p1 - p0;
	//			auto v02 = p2 - p0;
	//			auto v12 = p2 - p1;

	//			auto n = cross(v01, v02);
	//			if (!n.isZero()) { n.normalize(); }
	//			if (dot(n, d) > 0.0f) { n *= -1.0f; }

	//			float d_dot_n = dot(d, n);
	//			if (fabs(d_dot_n) < 0.0000001f) { return std::nullopt; }

	//			float t = dot(p0 - o, n) / d_dot_n;

	//			if (t < 0.0f) { return std::nullopt; }

	//			auto p = o + t * d;

	//			const auto vop0 = p0 - o;
	//			const auto vop1 = p1 - o;
	//			const auto vop2 = p2 - o;

	//			float V0 = dot(p - o, cross(vop1, vop2));
	//			float V1 = dot(p - o, cross(vop2, vop0));
	//			float V2 = dot(p - o, cross(vop0, vop1));
	//			float V = V0 + V1 + V2;
	//			Vector3f w;
	//			w[0] = V0 / V;
	//			w[1] = V1 / V;
	//			w[2] = V2 / V;

	//			if (!inRange01(w)) { return std::nullopt; }

	//			Intersection result;
	//			result.p = std::move(p);
	//			result.t = t;
	//			result.n = n;
	//			result.w = w;

	//			return std::move(result);
	//		}
	//	}

	//}

}