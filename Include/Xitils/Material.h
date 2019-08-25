#pragma once

#include "Utils.h"
#include "Vector.h"

namespace Xitils {

	class Material {
	public:
		bool specular;

		// bsdf_cos の値を返す
		// スペキュラの物体ではデルタ関数になるので実装しない
		virtual Vector3f eval(const SurfaceInteraction& isect, const Vector3f& wi) const {
			NOT_IMPLEMENTED;
			return Vector3f();
		}

		// bsdf_cos の値を返し、wi のサンプリングも行う
		// スペキュラの物体ではデルタ関数になるので実装しない
		virtual Vector3f evalAndSample(const SurfaceInteraction& isect, const std::shared_ptr<Sampler>& sampler, Vector3f* wi, float* pdf) const {
			NOT_IMPLEMENTED;
			return Vector3f();
		}

		// wi のサンプリングのみを行う
		// スペキュラの物体では pdf がデルタ関数になるので実装しない
		virtual void sample(const SurfaceInteraction& isect, const std::shared_ptr<Sampler>& sampler, Vector3f* wi, float* pdf) const {
			NOT_IMPLEMENTED;
		}

		// スペキュラの物体でのみ実装する
		// BSDF * cos / pdf の値を返す
		virtual Vector3f evalAndSampleSpecular(const SurfaceInteraction& isect, const std::shared_ptr<Sampler>& sampler, Vector3f* wi) const {
			NOT_IMPLEMENTED;
			return Vector3f();
		}

		// スペキュラの物体でのみ実装する
		// wi のサンプリングのみを行う
		virtual void sampleSpecular(const SurfaceInteraction& isect, const std::shared_ptr<Sampler>& sampler, Vector3f* wi) const = 0;
	};

	class Diffuse : public Material {
	public:
		Vector3f albedo;

		Diffuse (const Vector3f& albedo):
			albedo(albedo)
		{
			specular = false;
		}

		Vector3f eval(const SurfaceInteraction& isect, const Vector3f& wi) const override {
			const auto& n = isect.shading.n;
			return albedo * clampPositive(dot(n, wi)) / M_PI;
		}

		Vector3f evalAndSample(const SurfaceInteraction& isect, const std::shared_ptr<Sampler>& sampler, Vector3f* wi, float* pdf) const override {
			const auto& n = isect.shading.n;
			*wi = sampleVectorFromCosinedHemiSphere(n, sampler);
			*pdf = dot(*wi, n);
			return albedo * clampPositive(dot(n, *wi)) / M_PI;
		}

		void sample(const SurfaceInteraction& isect, const std::shared_ptr<Sampler>& sampler, Vector3f* wi, float* pdf) const override {
			const auto& n = isect.shading.n;
			*wi = sampleVectorFromCosinedHemiSphere(n, sampler);
			*pdf = dot(*wi, n);
		}
	};

	class Specular : public Material {
	public:
		Vector3f albedo;

		Specular(const Vector3f& albedo) :
			albedo(albedo)
		{
			specular = true;
		}

		Vector3f evalAndSampleSpecular(const SurfaceInteraction& isect, const std::shared_ptr<Sampler>& sampler, Vector3f* wi) const {
			const auto& wo = isect.wo;
			const auto& n = isect.shading.n;
			*wi = -(wo - 2 * dot(n, wo) * n).normalize();
			return albedo;
		}

		void sampleSpecular(const SurfaceInteraction& isect, const std::shared_ptr<Sampler>& sampler, Vector3f* wi) const override {
			const auto& wo = isect.wo;
			const auto& n = isect.shading.n;
			*wi = -( wo - 2 * dot(n, wo) * n ).normalize();
		}
	};


}