﻿#pragma once

#include "Bounds.h"
#include "Geometry.h"
#include "Interaction.h"
#include "Material.h"
#include "Ray.h"
#include "Shape.h"
#include "Transform.h"
#include "TriangleIndexed.h"
#include "Utils.h"
#include "Vector.h"

namespace xitils {

	class Object : public Geometry {
	public:

		struct SampledSurface {
			const Object* object;
			const Shape* shape;
			const TriangleIndexed* tri;
			Vector3f p;
			Vector3f n;
			Vector3f shadingN;
		};

		Transform objectToWorld;
		std::shared_ptr<Shape> shape;
		std::shared_ptr<Material> material;

		Object(std::shared_ptr<Shape> shape, std::shared_ptr<Material> material, const Transform& objectToWorld):
			shape(shape), material(material), objectToWorld(objectToWorld)
		{}

		Bounds3f bound() const override {
			return objectToWorld(shape->bound());
		}

		float surfaceArea() const {
			return shape->surfaceArea() * shape->surfaceAreaScaling(objectToWorld);
		}

		bool intersect(const Ray& ray, float* tHit, SurfaceIntersection* isect) const override {
			if (shape->intersect(objectToWorld.inverse(ray), tHit, isect)) {
				isect->p = objectToWorld(isect->p);
				
				isect->n = objectToWorld.asNormal(isect->n);
				if (!isect->tangent.isZero()) {
					isect->tangent = objectToWorld.asNormal(isect->tangent);
				}
				if (!isect->bitangent.isZero()) {
					isect->bitangent = objectToWorld.asNormal(isect->bitangent);
				}

				isect->wo = objectToWorld.asNormal(isect->wo);

				// �m�[�}���}�b�v���ݒ肳��Ă����ꍇ�����K�p
				if (material->normalmap != nullptr) {
					// shading.n �͕ω������邪�Atangnet �� bitangent �͕ω������Ȃ��̂Œ���

					Vector3f n = material->normalmap->rgb(isect->texCoord) * 2 - Vector3f(1.0f);
					isect->shading.n =
						(     n.b * isect->shading.n
							+ n.r * isect->shading.tangent
							+ n.g * isect->shading.bitangent
						).normalize();
				}

				isect->shading.n = objectToWorld.asNormal(isect->shading.n);
				if (!isect->shading.tangent.isZero()) {
					isect->shading.tangent = objectToWorld.asNormal(isect->shading.tangent);
				}
				if (!isect->shading.bitangent.isZero()) {
					isect->shading.bitangent = objectToWorld.asNormal(isect->shading.bitangent);
				}

				// texCoord, tHit �͕ϊ����Ȃ��Ă悢

				isect->object = this;

				return true;
			}
			return false;
		}

		bool intersectAny(const Ray& ray) const override {
			return shape->intersectAny(objectToWorld.inverse(ray));
		}

		SampledSurface sampleSurface(Sampler& sampler, float* pdf) const {
			auto sampled = shape->sampleSurface(sampler, pdf);

			*pdf /= shape->surfaceAreaScaling(objectToWorld);

			SampledSurface res;
			res.object = this;
			res.shape = sampled.shape;
			res.tri = sampled.tri;
			res.p = objectToWorld(sampled.p);
			res.n = objectToWorld.asNormal(sampled.n);
			res.shadingN = objectToWorld.asNormal(sampled.shadingN);

			return res;
		}

		float surfacePDF(const Vector3f& p, const Shape* shape, const TriangleIndexed* tri) const {
			Vector3f scaling = objectToWorld.getScaling();
			return shape->surfacePDF(objectToWorld.inverse(p), tri) / shape->surfaceAreaScaling(objectToWorld);
		}

	};

}