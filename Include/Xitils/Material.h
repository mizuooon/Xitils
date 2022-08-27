#pragma once

#include "Texture.h"
#include "Utils.h"
#include "Vector.h"

namespace xitils {

	class Material {
	public:
		bool emissive = false;

		std::shared_ptr<Texture> normalmap;

		// BSDF * cos �̒l��Ԃ�
		// �X�y�L�����̕��̂ł̓f���^�֐��ɂȂ�̂Ŏ������Ȃ�
		virtual Vector3f bsdfCos(const SurfaceIntersection& isect, Sampler& sampler, const Vector3f& wi) const {
			NOT_IMPLEMENTED;
			return Vector3f();
		}

		// BSDF * cos / pdf �̒l��Ԃ��Awi �̃T���v�����O��s��
		// �X�y�L�����̕��̂ł̓f���^�֐��ɂȂ�A���̂Ƃ� pdf �� -1 �Ƃ��ĕ\�����
		// �߂�l�� 0 �̂Ƃ��ɂ� wi �� pdf �͗L���ł͂Ȃ�
		virtual Vector3f evalAndSample(const SurfaceIntersection& isect, Sampler& sampler, Vector3f* wi, float* pdf) const {
			NOT_IMPLEMENTED;
			return Vector3f();
		}

		// �X�y�L�����̕��̂ł̓f���^�֐��ɂȂ�̂Ŏ������Ȃ�
		virtual float pdf(const SurfaceIntersection& isect, const Vector3f& wi) const {
			NOT_IMPLEMENTED;
		}

		// �P�x��Ԃ�
		// emissive �̃}�e���A���݂̂Ŏg�p����
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

		Vector3f bsdfCos(const SurfaceIntersection& isect, Sampler& sampler, const Vector3f& wi) const override {
			if (texture == nullptr) {
				return albedo / M_PI * clampPositive(dot(isect.shading.n, wi));
			} else {
				return albedo / M_PI * texture->rgb(isect.texCoord) * clampPositive(dot(isect.shading.n, wi));
			}
		}

		Vector3f evalAndSample(const SurfaceIntersection& isect, Sampler& sampler, Vector3f* wi, float* pdf) const override {
			const auto& n = isect.shading.n;
			*wi = sampleVectorFromCosinedHemiSphere(n, sampler);
			*pdf = dot(*wi, n) / M_PI;
			if (texture == nullptr) {
				return albedo;
			} else {
				return albedo * texture->rgb(isect.texCoord);
			}
		}

		float pdf(const SurfaceIntersection& isect, const Vector3f& wi) const override {
			const auto& n = isect.shading.n;
			return clampPositive(dot(wi, n)) / M_PI;
		}
	};

	struct Glossy : public Material {
	public:

		Vector3f albedo;
		float sharpness;

		Glossy(const Vector3f& albedo, float sharpness) : 
			albedo(albedo),
			sharpness(sharpness)
		{}

		Vector3f bsdfCos(const SurfaceIntersection& isect, Sampler& sampler, const Vector3f& wi) const override {
			const auto& n = isect.shading.n;
			float N = 2 * M_PI / (sharpness + 2);
			return albedo * powf(dot((isect.wo + wi).normalize(), n), sharpness) / N;
		}

		Vector3f evalAndSample(const SurfaceIntersection& isect, Sampler& sampler, Vector3f* wi, float* pdf) const override {
			float r1 = sampler.randf();
			float r2 = sampler.randf();

			const Vector3f& n = isect.shading.n;

			BasisVectors basis(n);
			float sqrt = safeSqrt(1 - powf(r2, 2 / (sharpness + 1.0f)));
			Vector3f h = basis.toGlobal(powf(r2, 1 / (sharpness + 1.0f)), cosf(2 * M_PI * r1) * sqrt, sinf(2 * M_PI * r1) * sqrt).normalize();
			*wi = -(isect.wo - 2.0f * dot(isect.wo, h) * h).normalize();

			*pdf = this->pdf(isect, *wi);

			if (*pdf == 0.0f) { return Vector3f(0.0f); }

			return bsdfCos(isect, sampler, *wi) / *pdf;
		}


		float pdf(const SurfaceIntersection& isect, const Vector3f& wi) const override {
			const auto& n = isect.shading.n;
			return (sharpness + 1) / (2 * M_PI) * powf(clampPositive(dot((isect.wo + wi).normalize(), n)), sharpness);
		}

	};

	struct GlossyWithHighLight : public Material {
	public:
		float highlightRate;

		GlossyWithHighLight(const Vector3f& albedo, const Vector3f& highlight, float sharpness, float sharpnessHighLight, float highlightRate) :
			base(new Glossy(albedo, sharpness)),
			highlight(new Glossy(highlight, sharpnessHighLight)),
			highlightRate(highlightRate)
		{}

		Vector3f bsdfCos(const SurfaceIntersection& isect, Sampler& sampler, const Vector3f& wi) const override {
			return (1.0f - highlightRate) * base->bsdfCos(isect, sampler, wi) + highlightRate * highlight->bsdfCos(isect, sampler, wi);
		}

		Vector3f evalAndSample(const SurfaceIntersection& isect, Sampler& sampler, Vector3f* wi, float* pdf) const override {
			Vector3f eval;
			if (sampler.randf() <= highlightRate) {
				float tmppdf;
				Vector3f tmpeval = highlight->evalAndSample(isect, sampler, wi, &tmppdf);
				eval = (1.0f - highlightRate)* base->bsdfCos(isect, sampler, *wi) + highlightRate * tmpeval;
				*pdf = (1.0f - highlightRate) * base->pdf(isect, *wi) + highlightRate * tmppdf;
			} else {
				float tmppdf;
				Vector3f tmpeval = base->evalAndSample(isect, sampler, wi, &tmppdf);
				eval = (1.0f - highlightRate) * tmpeval + highlightRate * highlight->bsdfCos(isect, sampler, *wi);
				*pdf = (1.0f - highlightRate) * tmppdf + highlightRate * highlight->pdf(isect, *wi);
			}
			return eval;
		}

		float pdf(const SurfaceIntersection& isect, const Vector3f& wi) const override {
			return (1.0f - highlightRate) * base->pdf(isect, wi) + highlightRate * highlight->pdf(isect, wi);
		}

	private:
		std::shared_ptr<Glossy> base;
		std::shared_ptr<Glossy> highlight;
	};

	class SpecularReflection : public Material {
	public:

		Vector3f evalAndSample(const SurfaceIntersection& isect, Sampler& sampler, Vector3f* wi, float* pdf) const override {
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

		Vector3f evalAndSample(const SurfaceIntersection& isect, Sampler& sampler, Vector3f* wi, float* pdf) const override {
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

		Vector3f evalAndSample(const SurfaceIntersection& isect, Sampler& sampler, Vector3f* wi, float* pdf) const override {
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

			if (F_r(wo, n, eta_o, eta_i) >= sampler.randf()) {
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

		Vector3f bsdfCos(const SurfaceIntersection& isect, Sampler& sampler, const Vector3f& wi) const override {
			return Vector3f();
		}

		Vector3f evalAndSample(const SurfaceIntersection& isect, Sampler& sampler, Vector3f* wi, float* pdf) const override {
			return Vector3f();
		}

		float pdf(const SurfaceIntersection& isect, const Vector3f& wi) const override {
			return 0.0f;
		}

		Vector3f emission(const Vector3f& wo, const Vector3f& n, const Vector3f& shadingN) const override {
			return dot(wo,n) > 0.0f ? power : Vector3f();
		}
	};

}