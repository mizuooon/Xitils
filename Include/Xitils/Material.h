#pragma once

#include "Texture.h"
#include "Utils.h"
#include "Vector.h"

namespace Xitils {

	class Material {
	public:
		bool emissive = false;

		std::shared_ptr<Texture> normalmap;

		// BSDF * cos の値を返す
		// スペキュラの物体ではデルタ関数になるので実装しない
		virtual Vector3f bsdfCos(const SurfaceInteraction& isect, const std::shared_ptr<Sampler>& sampler, const Vector3f& wi) const {
			NOT_IMPLEMENTED;
			return Vector3f();
		}

		// BSDF * cos / pdf の値を返し、wi のサンプリングも行う
		// スペキュラの物体ではデルタ関数になり、このとき pdf は -1 として表される
		// 戻り値が 0 のときには wi と pdf は有効ではない
		virtual Vector3f evalAndSample(const SurfaceInteraction& isect, const std::shared_ptr<Sampler>& sampler, Vector3f* wi, float* pdf) const {
			NOT_IMPLEMENTED;
			return Vector3f();
		}

		// スペキュラの物体ではデルタ関数になるので実装しない
		virtual float pdf(const SurfaceInteraction& isect, const Vector3f& wi) const {
			NOT_IMPLEMENTED;
		}

		// 輝度を返す
		// emissive のマテリアルのみで使用する
		virtual Vector3f emission(const Vector3f& wo, const Vector3f& n, const Vector3f& shadingN) const {
			NOT_IMPLEMENTED;
			return Vector3f();
		}

	};

	class Diffuse : public Material {
	public:
		Vector3f albedo;
		std::shared_ptr<Texture> texture;

		Diffuse (const Vector3f& albedo):
			albedo(albedo)
		{}

		Vector3f bsdfCos(const SurfaceInteraction& isect, const std::shared_ptr<Sampler>& sampler, const Vector3f& wi) const override {
			if (texture == nullptr) {
				return albedo / M_PI * clampPositive(dot(isect.shading.n, wi));
			} else {
				return albedo / M_PI * texture->rgb(isect.texCoord) * clampPositive(dot(isect.shading.n, wi));
			}
		}

		Vector3f evalAndSample(const SurfaceInteraction& isect, const std::shared_ptr<Sampler>& sampler, Vector3f* wi, float* pdf) const override {
			const auto& n = isect.shading.n;
			*wi = sampleVectorFromCosinedHemiSphere(n, sampler);
			*pdf = dot(*wi, n) / M_PI;
			if (texture == nullptr) {
				return albedo;
			} else {
				return albedo * texture->rgb(isect.texCoord);
			}
		}

		float pdf(const SurfaceInteraction& isect, const Vector3f& wi) const override {
			const auto& n = isect.shading.n;
			return clampPositive(dot(wi, n)) / M_PI;
		}
	};

	class SpecularReflection : public Material {
	public:

		Vector3f evalAndSample(const SurfaceInteraction& isect, const std::shared_ptr<Sampler>& sampler, Vector3f* wi, float* pdf) const override {
			const auto& wo = isect.wo;

			const auto& n = isect.shading.n;
			*wi = -(wo - 2 * dot(n, wo) * n).normalize();
			*pdf = -1.0f;
			return Vector3f(1.0f);
		}

	};

	class SpecularRefraction : public Material {
	public:

		float index = 1.0f;

		Vector3f evalAndSample(const SurfaceInteraction& isect, const std::shared_ptr<Sampler>& sampler, Vector3f* wi, float* pdf) const override {
			const auto& wo = isect.wo;
			const auto& n = isect.shading.n;

			float eta_i;
			float eta_o;
			if (dot(isect.n, wo) > 0.0f) {
				eta_o = 1.0f;
				eta_i = index;
			} else {
				eta_o = index;
				eta_i = 1.0f;
			}

			float rindex = eta_o / eta_i;
			float cos = dot(-wo, n);
			*wi = rindex * (-wo - cos * n) - safeSqrt(1 - powf(rindex, 2) * (1 - powf(cos, 2))) * n;
			*pdf = -1.0f;

			return Vector3f(1.0f);
		}

	};

	class SpecularFresnel : public Material {
	public:

		float index = 1.0f;

		Vector3f evalAndSample(const SurfaceInteraction& isect, const std::shared_ptr<Sampler>& sampler, Vector3f* wi, float* pdf) const override {
			const auto& wo = isect.wo;
			const auto& n = isect.shading.n;

			float eta_i;
			float eta_o;
			if (dot(isect.n, wo) > 0.0f) {
				eta_o = 1.0f;
				eta_i = index;
			} else {
				eta_o = index;
				eta_i = 1.0f;
			}

			if (F_r(wo, n, eta_o, eta_i) >= sampler->randf()) {
				*wi = -(wo - 2 * dot(n, wo) * n).normalize();
			} else {
				float rindex = eta_o / eta_i;
				float cos = dot(-wo, n);
				*wi = rindex * (-wo - cos * n) - safeSqrt(1 - powf(rindex, 2) * (1 - powf(cos, 2))) * n;
			}

			*pdf = -1.0f;

			return Vector3f(1.0f);
		}

	private:
		//float F_r(const Vector3f& wo, const Vector3f& wi, const Vector3f& n, float eta_o, float eta_i) const {
		//	float cos_theta_o = dot(-wo, n);
		//	float cos_theta_i = dot(wi, n);

		//	if (1 - powf(eta_o / eta_i, 2) * (1 - powf(cos_theta_o, 2)) < 0) { return 1.0f; }

		//	float rho1 = (eta_o * cos_theta_i - eta_i * cos_theta_o) / (eta_o * cos_theta_i + eta_i * cos_theta_o);
		//	float rho2 = (eta_o * cos_theta_o - eta_i * cos_theta_i) / (eta_o * cos_theta_o + eta_i * cos_theta_i);

		//	return (powf(rho1, 2) + powf(rho2, 2)) / 2.0f;
		//}

		float F_r( const Vector3f& wo, const Vector3f& n, float eta_o, float eta_i ) const {
		const float c = fabsf( dot( wo, n ) );
		float g_sq = powf( eta_o / eta_i, 2.0f ) - 1.0f + c * c;
		if ( g_sq <= 0.0f ) { return 1.0f; }
		float g = sqrtf( g_sq );
		return 0.5f * powf((g - c) / (g + c), 2.0f) * (1.0f + powf((c * (g + c) - 1.0f) / (c * (g - c) + 1.0f), 2.0f));
		}

	};

	class Emission : public Material {
	public:
		Vector3f power;

		Emission(const Vector3f power):
			power(power)
		{
			emissive = true;
		}

		Vector3f bsdfCos(const SurfaceInteraction& isect, const std::shared_ptr<Sampler>& sampler, const Vector3f& wi) const override {
			return Vector3f();
		}

		Vector3f evalAndSample(const SurfaceInteraction& isect, const std::shared_ptr<Sampler>& sampler, Vector3f* wi, float* pdf) const override {
			return Vector3f();
		}

		float pdf(const SurfaceInteraction& isect, const Vector3f& wi) const override {
			return 0.0f;
		}

		Vector3f emission(const Vector3f& wo, const Vector3f& n, const Vector3f& shadingN) const override {
			return dot(wo,n) > 0.0f ? power : Vector3f();
		}
	};

}