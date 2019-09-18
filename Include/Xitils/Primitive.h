#pragma once

#include "Utils.h"
#include "Vector.h"
#include "Bounds.h"
#include "Geometry.h"
#include "Sampler.h"
#include "Transform.h"
#include "Ray.h"
#include "Interaction.h"
#include "Texture.h"

namespace Xitils {

	class Primitive : public Geometry {
	public:

		struct SampledSurface {
			const Primitive* primitive;
			Vector3f p;
			Vector3f n;
			Vector3f shadingN;
		};

		virtual float surfaceAreaScaling(const Transform& t) const = 0;

		virtual SampledSurface sampleSurface(Sampler& sampler, float* pdf) const = 0;
		virtual float surfacePDF(const Vector3f& p) const = 0;
	};

	class TriangleIndexed : public Primitive {
	public:

		const Vector3f* positions;
		const Vector2f* texCoords;
		const Vector3f* normals;
		const Vector3f* tangents;
		const Vector3f* bitangents;
		const int* indices;
		int index;

		TriangleIndexed(const Vector3f* positions, const Vector2f* texCoords, const Vector3f* normals, const Vector3f* tangents, const Vector3f* bitangents, const int* indices, int index) :
			positions(positions),
			texCoords(texCoords),
			normals(normals),
			tangents(tangents),
			bitangents(bitangents),
			indices(indices),
			index(index)
		{}

		const Vector3f& position(int i) const { return positions[indices[index * 3 + i]]; }
		const Vector2f& texCoord(int i) const { return texCoords[indices[index * 3 + i]]; }
		const Vector3f& normal(int i) const { return normals[indices[index * 3 + i]]; }
		const Vector3f& tangent(int i) const { return tangents[indices[index * 3 + i]]; }
		const Vector3f& bitangent(int i) const { return bitangents[indices[index * 3 + i]]; }

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

		float surfaceAreaScaling(const Transform& t) const {
			// TODO: çÇë¨âª
			const auto& p0 = t(position(0));
			const auto& p1 = t(position(1));
			const auto& p2 = t(position(2));
			auto v01 = p1 - p0;
			auto v02 = p2 - p0;
			return cross(v01, v02).length() / 2.0f / surfaceArea();
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

			Vector2f texCoordTmp;
			if (texCoords != nullptr) {
				texCoordTmp = lerp(texCoord(0), texCoord(1), texCoord(2), b0, b1);
			} else {
				texCoordTmp = Vector2f();
			}

			if (discardByAlpha(texCoordTmp)) {
				return false;
			}

			isect->texCoord = texCoordTmp;

			*tHit = t;
			isect->p = lerp(p0, p1, p2, b0, b1);
			isect->n = cross(p1 - p0, p2 - p0).normalize();

			isect->wo = normalize(-ray.d);

			if (normals != nullptr) {
				isect->shading.n = lerp(normal(0), normal(1), normal(2), b0, b1).normalize();
			} else {
				isect->shading.n = isect->n;
			}

			if (tangents != nullptr) {
				isect->tangent = lerp(tangent(0), tangent(1), tangent(2), b0, b1).normalize();
				isect->bitangent = lerp(bitangent(0), bitangent(1), bitangent(2), b0, b1).normalize();
				isect->shading.tangent = isect->tangent;
				isect->shading.bitangent = isect->bitangent;
			} else {
				isect->tangent = Vector3f();
				isect->bitangent = Vector3f();
				isect->shading.tangent = Vector3f();
				isect->shading.bitangent = Vector3f();
			}

			isect->shading.n = faceForward(isect->shading.n, isect->wo);

			isect->primitive = this;

			perturbIntersection(*isect);

			return true;
		}

		SampledSurface sampleSurface(Sampler& sampler, float* pdf) const override {
			float t0;
			float t1;
			float t2;
			float s;
			do {
				t0 = sampler.randf();
				t1 = sampler.randf();
				t2 = sampler.randf();
				s = t0 + t1 + t2;
			} while (s == 0.0f);
			float inv = 1.0f / s;
			t0 *= inv;
			t1 *= inv;

			*pdf = 1.0f / surfaceArea();

			const Vector3f& p0 = position(0);
			const Vector3f& p1 = position(1);
			const Vector3f& p2 = position(2);
			SampledSurface res;
			res.p = lerp(p0, p1, p2, t0, t1);
			res.n = cross(p1 - p0, p2 - p0).normalize();
			if (normals == nullptr) {
				res.shadingN = res.n;
			} else {
				res.shadingN = lerp(normal(0), normal(1), normal(2), t0, t1).normalize();
			}
			res.primitive = this;
			return res;
		}

		float surfacePDF(const Vector3f& p) const override {
			return 1.0f / surfaceArea();
		}

	protected:
		virtual bool discardByAlpha(const Vector2f& texCoord) const { return false; }
		virtual void perturbIntersection(SurfaceInteraction& isect) const {}
	};


	class TriangleIndexedWithShellMapping : public TriangleIndexed {
	public:
		float height; // [0,1] ÇÃîÕàÕ
		float displacementScale;
		const Texture* displacementMapTexture;

		TriangleIndexedWithShellMapping(const Vector3f* positions, const Vector2f* texCoords, const Vector3f* normals, const Vector3f* tangents, const Vector3f* bitangents, const int* indices, int index, const Texture* displacementMapTexture, float displacementScale, float height) :
			TriangleIndexed(positions, texCoords, normals, tangents, bitangents, indices, index),
			displacementMapTexture(displacementMapTexture),
			displacementScale(displacementScale),
			height(height)
		{}

	protected:

		virtual bool discardByAlpha(const Vector2f& texCoord) const override {
			return displacementMapTexture->rgb(texCoord).x < height;
		}

		virtual void perturbIntersection(SurfaceInteraction& isect) const override {

			Vector3f a;
			a.x = -displacementMapTexture->rgbDifferentialU(isect.texCoord).x * displacementScale;
			a.y = -displacementMapTexture->rgbDifferentialV(isect.texCoord).x * displacementScale;
			a.z = 1;
			a.normalize();

			isect.shading.n = (a.x * isect.shading.tangent + a.y * isect.shading.bitangent + a.z * isect.n).normalize();

			// TODO: tangent Ç∆ bitangent
			isect.shading.tangent = Vector3f();
			isect.shading.bitangent = Vector3f();

			//isect.shading.n = faceForward(isect.shading.n, isect.wo);
		}
	};

}