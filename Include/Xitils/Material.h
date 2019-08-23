#pragma once

#include "Utils.h"
#include "Vector.h"

namespace Xitils {

	class Material {
	public:
		bool specular;

		// bsdf_cos の値を返す
		// スペキュラの物体ではデルタ関数になるので実装しない
		virtual Vector3f eval(const Vector3f& wo, const Vector3f& wi, const Vector3f& n) const {
			NOT_IMPLEMENTED;
			return Vector3f();
		}

		// bsdf_cos の値を返し、wi のサンプリングも行う
		// スペキュラの物体ではデルタ関数になるので実装しない
		virtual Vector3f evalAndSample(const Vector3f& wo, Vector3f* wi, const Vector3f& n, float* pdf, Sampler& sampler) const {
			NOT_IMPLEMENTED;
			return Vector3f();
		}

		// wi のサンプリングのみを行う
		// スペキュラの物体では pdf がデルタ関数になるので実装しない
		virtual void sample(const Vector3f& wo, Vector3f* wi, const Vector3f& n, float* pdf, Sampler& sampler) const {
			NOT_IMPLEMENTED;
		}

		// スペキュラの物体でのみ実装する
		// BSDF * cos / pdf の値を返す
		virtual Vector3f evalAndSampleSpecular(const Vector3f& wo, Vector3f* wi, const Vector3f& n, Sampler& sampler) const {
			NOT_IMPLEMENTED;
			return Vector3f();
		}

		// スペキュラの物体でのみ実装する
		// wi のサンプリングのみを行う
		virtual void sampleSpecular(const Vector3f& wo, Vector3f* wi, const Vector3f& n, Sampler& sampler) const = 0;
	};

	class Diffuse : public Material {
	public:
		Vector3f albedo;

		Diffuse (const Vector3f& albedo):
			albedo(albedo)
		{
			specular = false;
		}

		Vector3f eval(const Vector3f& wo, const Vector3f& wi, const Vector3f& n) const override {
			Vector3f fn = faceForward(n, wo);
			return albedo * fabs(dot(fn, wi)) * dot(wi, n) / M_PI;
		}

		Vector3f evalAndSample(const Vector3f& wo, Vector3f* wi, const Vector3f& n, float* pdf, Sampler& sampler) const override {
			Vector3f fn = faceForward(n, wo);
			*wi = sampleVectorFromCosinedHemiSphere(fn, sampler);
			*pdf = dot(*wi, fn);
			return albedo * fabs(dot(fn, *wi)) * dot(*wi, fn) / M_PI;
		}

		void sample(const Vector3f& wo, Vector3f* wi, const Vector3f& n, float* pdf, Sampler& sampler) const override {
			Vector3f fn = faceForward(n, wo);
			*wi = sampleVectorFromCosinedHemiSphere(fn, sampler);
			*pdf = dot(*wi, fn);
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

		Vector3f evalAndSampleSpecular(const Vector3f& wo, Vector3f* wi, const Vector3f& n, Sampler& sampler) const {
			Vector3f fn = faceForward(n, wo);
			*wi = -(wo - 2 * dot(fn, wo) * fn).normalize();
			return albedo;
		}

		void sampleSpecular(const Vector3f& wo, Vector3f* wi, const Vector3f& n, Sampler& sampler) const override {
			Vector3f fn = faceForward(n, wo);
			*wi = -( wo - 2 * dot(fn, wo) * fn ).normalize();
		}
	};


}