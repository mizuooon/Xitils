#pragma once

#include "Texture.h"
#include "Utils.h"
#include "Vector.h"

namespace xitils {

	class Material {
	public:
		bool emissive = false;

		std::shared_ptr<Texture> normalmap;

		// BSDF * cos の値を返す
		// スペキュラの物体ではデルタ関数になるので実装しない
		virtual Vector3f bsdfCos(const SurfaceIntersection& isect, Sampler& sampler, const Vector3f& wi) const {
			NOT_IMPLEMENTED;
			return Vector3f();
		}

		// BSDF * cos / getPDF の値を返し、wi のサンプリングも行う
		// スペキュラの物体ではデルタ関数になり、このとき getPDF は -1 として表される
		// 戻り値が 0 のときには wi と getPDF は有効ではない
		virtual Vector3f evalAndSample(const SurfaceIntersection& isect, Sampler& sampler, Vector3f* wi, float* pdf) const {
			NOT_IMPLEMENTED;
			return Vector3f();
		}

		// スペキュラの物体ではデルタ関数になるので実装しない
		virtual float getPDF(const SurfaceIntersection& isect, const Vector3f& wi) const {
			NOT_IMPLEMENTED;
		}

		// 輝度を返す
		// emissive のマテリアルのみで使用する
		virtual Vector3f getEmission(const Vector3f& wo, const Vector3f& n, const Vector3f& shadingN) const {
			NOT_IMPLEMENTED;
			return Vector3f();
		}

		// アルベドを返す
		// デノイザ用
		virtual Vector3f getAlbedo() const
		{
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

		float getPDF(const SurfaceIntersection& isect, const Vector3f& wi) const override {
			const auto& n = isect.shading.n;
			return clampPositive(dot(wi, n)) / M_PI;
		}

		Vector3f getAlbedo() const override {
			return albedo;
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

			*pdf = this->getPDF(isect, *wi);

			if (*pdf == 0.0f) { return Vector3f(0.0f); }

			return bsdfCos(isect, sampler, *wi) / *pdf;
		}


		float getPDF(const SurfaceIntersection& isect, const Vector3f& wi) const override {
			const auto& n = isect.shading.n;
			return (sharpness + 1) / (2 * M_PI) * powf(clampPositive(dot((isect.wo + wi).normalize(), n)), sharpness);
		}

		Vector3f getAlbedo() const override {
			return albedo;
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
				*pdf = (1.0f - highlightRate) * base->getPDF(isect, *wi) + highlightRate * tmppdf;
			} else {
				float tmppdf;
				Vector3f tmpeval = base->evalAndSample(isect, sampler, wi, &tmppdf);
				eval = (1.0f - highlightRate) * tmpeval + highlightRate * highlight->bsdfCos(isect, sampler, *wi);
				*pdf = (1.0f - highlightRate) * tmppdf + highlightRate * highlight->getPDF(isect, *wi);
			}
			return eval;
		}

		float getPDF(const SurfaceIntersection& isect, const Vector3f& wi) const override {
			return (1.0f - highlightRate) * base->getPDF(isect, wi) + highlightRate * highlight->getPDF(isect, wi);
		}
		
		Vector3f getAlbedo() const override {
			return base->getAlbedo();
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

	class Metal : public Material
	{
	public:
		float alpha;

		Vector3f f0;

		Metal(float roughness, const Vector3f& f0) :
			alpha(roughness * roughness),
			f0(f0)
		{
		}

		Vector3f F_Reflection(const Vector3f& eye, const Vector3f& h) const
		{
		return Vector3f(1);
			return f0 + (Vector3f(1.0f) - f0) * pow(1 - dot(eye, h), 5.0f);
		}

		float D_GGX(const Vector3f& x, const Vector3f& n) const
		{
			float alpha2 = alpha * alpha;
			float cosThetaH2 = pow(dot(n, x), 2);
			float cosThetaH4 = cosThetaH2 * cosThetaH2;
			float tanThetaH2 = 1 / cosThetaH2 - 1;

			return alpha2 * heavisideStep(dot(n, x)) / (M_PI* cosThetaH4  * pow(alpha2 + tanThetaH2, 2));
		}

		float D_V_GGX(const Vector3f& x, const Vector3f& n,  const Vector3f& eye) const
		{
			return G1_Smith(eye, n) * clampPositive(dot(eye, x)) * D_GGX(n, x) / dot(eye, n);
		}

		float G1_Smith(const Vector3f& x, const Vector3f& n) const
		{
			float alpha2 = alpha * alpha;
			float cosThetaV2 = pow(dot(n, x), 2);
			float lambda = (-1 + safeSqrt(1 + alpha2 / cosThetaV2)) / 2;
			return 1 / (1 + lambda);
		}

		float G_Smith(const Vector3f& wi, const Vector3f& wo, const Vector3f& n) const
		{
			return G1_Smith(wi, n) * G1_Smith(wo, n);
		}

		Vector3f sampleGGXVNDF(const Vector3f& eye, const Vector3f& n,  Sampler& sampler) const
		{
			auto basis = BasisVectors(n);
			Vector3f Ve = basis.fromGlobal(eye);
			Vector3f Vh = Vector3f(Ve.x, alpha * Ve.y, alpha * Ve.z).normalize();
			float lensq = Vh.y * Vh.y + Vh.z * Vh.z;
			Vector3f T1 = lensq > 0 ? Vector3f(0, Vh.z, -Vh.y) / safeSqrt(lensq) : Vector3f(0, 0, 1);
			Vector3f T2 = cross(Vh, T1);
			float U1 = sampler.randf();
			float U2 = sampler.randf();
			float r = sqrt(U1);
			float phi = 2.0 * M_PI * U2;
			float t1 = r * cos(phi);
			float t2 = r * sin(phi);
			float s = 0.5 * (1.0 + Vh.x);
			t2 = (1.0 - s) * sqrt(1.0 - t1 * t1) + s * t2;
			Vector3f Nh = t1 * T1 + t2 * T2 + safeSqrt(max(0.0, 1.0 - t1 * t1 - t2 * t2)) * Vh;
			Vector3f Ne = normalize(Vector3f(std::max<float>(0.0, Nh.x), alpha * Nh.y, alpha * Nh.z));
			Vector3f h = basis.toGlobal(Ne);
			return h;
		}

		Vector3f bsdfCos(const SurfaceIntersection& isect, Sampler& sampler, const Vector3f& wi) const override {
			const auto& wo = isect.wo;
			const auto& n = isect.shading.n;
			const auto h = (wi + wo).normalize();

			Vector3f Fr = F_Reflection(wo, h);
			float D = D_GGX(h, n);
			float G = G_Smith(wi, wo, n);
			return Fr * D * G / clampPositive(4 * dot(wo, n));
		}

		Vector3f evalAndSample(const SurfaceIntersection& isect, Sampler& sampler, Vector3f* wi, float* pdf) const override {
			const auto& wo = isect.wo;
			const auto& n = isect.shading.n;

			auto h = sampleGGXVNDF(wo, n, sampler);
			*wi = 2 * dot(wo, h) * h - wo;

			//*wi = sampleVectorFromCosinedHemiSphere(n, sampler);

			*pdf = getPDF(isect, *wi);

			return bsdfCos(isect, sampler, *wi) / *pdf;
		}

		float getPDF(const SurfaceIntersection& isect, const Vector3f& wi) const override {
			const auto& wo = isect.wo;
			const auto& n = isect.shading.n;
			const auto h = (wi + wo).normalize();
			//return dot(wi, n) / M_PI;
			return D_V_GGX(h, n, wo) / (4 * dot(wo, h));
		}

		Vector3f getAlbedo() const override {
			return f0;
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

		float getPDF(const SurfaceIntersection& isect, const Vector3f& wi) const override {
			return 0.0f;
		}

		Vector3f getEmission(const Vector3f& wo, const Vector3f& n, const Vector3f& shadingN) const override {
			return dot(wo,n) > 0.0f ? power : Vector3f();
		}
	};

}