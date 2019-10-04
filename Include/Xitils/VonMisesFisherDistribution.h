#pragma once

#include "Utils.h"
#include "Sampler.h"
#include "Vector.h"

namespace xitils {

	template<int _LobeNum> class VonMisesFisherDistribution {
	public:

		// 実装は以下を参考
		// Numerically stable sampling of the von Misesisher distribution on S^2 (and other tricks)
		// https://www.mitsuba-renderer.org/~wenzel/files/vmf.pdf

		static const int LobeNum = _LobeNum;
		static const int EncodedDataSize = LobeNum * 3 + LobeNum + (LobeNum - 1); // encode に必要な float の数 (byte ではない)

		// ローブごと
		std::vector<Vector3f> mu;
		std::vector<float> kappa;
		std::vector<float> alpha;

		VonMisesFisherDistribution() {
			mu.resize(LobeNum);
			kappa.resize(LobeNum);
			alpha.resize(LobeNum);

			for (int i = 0; i < LobeNum; ++i) {
				mu[i] = Vector3f(1, 0, 0);
				kappa[i] = 0.0f;
				alpha[i] = 0.0f;
			}
			alpha[0] = 1.0f;
		}

		void encode(float* dest) const {
			for (int n = 0; n < LobeNum; ++n) {
				dest[n * 5 + 0] = mu[n].x;
				dest[n * 5 + 1] = mu[n].y;
				dest[n * 5 + 2] = mu[n].z;
				dest[n * 5 + 3] = kappa[n];
				if (n < lobe - 1) { dest[n * 5 + 4] = alpha[n]; }
			}
		}

		void decode(const float* data) {
			float alphaAccum = 0.0f;
			for (int n = 0; n < LobeNum; ++n) {
				mu[n].x = data[n * 5 + 0];
				mu[n].y = data[n * 5 + 1];
				mu[n].z = data[n * 5 + 2];
				kappa[n] = data[n * 5 + 3];

				alpha[n] = (n < LobeNum - 1) ? data[n * 5 + 4] : 1.0f - alphaAccum;
				alphaAccum += alpha[n];
			}
		}

		float eval(const Vector3f& n) const {
			float val = 0.0f;
			for (int k = 0; k < LobeNum; ++k) {
				val += alpha[k] * eval(n, k);
			}
			return val;
		}

		static VonMisesFisherDistribution<_LobeNum> approximateBySEM(const std::vector<Vector3f>& samples, Sampler& sampler) {

			const float KappaLimit = 1000;
			const int StepMax = 100000;
			const float DiffThreshold = 1e-6;

			VonMisesFisherDistribution<_LobeNum> vmf;
			auto& mu = vmf.mu;
			auto& kappa = vmf.kappa;
			auto& alpha = vmf.alpha;

			if (samples.size() == 1) {
				for (int i = 0; i < LobeNum; ++i) {
					mu[i] = samples[0];
					kappa[i] = KappaLimit;
					alpha[i] = i == 0 ? 1.0f : 0.0f;
				}
				return vmf;
			}


			for (int j = 0; j < LobeNum; j++) {
				float u = sampler.randf();
				float v = sampler.randf();
				float theta = asinf(u); // [0, PI/2]
				float phai = 2.0f * (float)M_PI * v - M_PI; // [-PI, PI]

				mu[j] = Vector3f(cosf(theta) * cosf(phai), cosf(theta) * sinf(phai), sinf(theta));
				kappa[j] = 0.5f;
				alpha[j] = 1.0f / LobeNum;
			}

			float* z_data = new float[samples.size() * LobeNum];
			auto z = [&](int i, int j) -> float& { return z_data[i * LobeNum + j]; };

			int step = 0;
			VonMisesFisherDistribution<_LobeNum> lastVMF;

			for (step = 0; step < StepMax; ++step) {
				// E-step

				for (int i = 0; i < samples.size(); i++) {
					const auto& sample = samples[i];

					float gamma_sum = 0.0f;
					for (int j = 0; j < LobeNum; j++) {
						gamma_sum += vMF(mu[j], kappa[j], sample);
					}

					for (int j = 0; j < LobeNum; j++) {
						z(i, j) = gamma_sum > 0.0f ? vMF(mu[j], kappa[j], sample) / gamma_sum : 0.0f;
					}
				}

				// M-step
				for (int j = 0; j < LobeNum; j++) {
					alpha[j] = 0.0f;
					Vector3f r(0.0f, 0.0f, 0.0f);

					for (int i = 0; i < samples.size(); i++) {
						alpha[j] += z(i, j);
						r += z(i, j) * samples[i];
					}
					r /= alpha[j];
					alpha[j] /= samples.size();

					kappa[j] = (3 * r.length() - powf(r.length(), 3.0f)) / (1.0f - r.lengthSq());
					mu[j] = normalize(r);

					if (kappa[j] > KappaLimit) { kappa[j] = KappaLimit; }
					if (kappa[j] < 0.0f) { kappa[j] = 0.0f; }
				}

				if (step > 0) {
					float diff = 0.0f;
					for (int j = 0; j < LobeNum; ++j) {
						diff += (vmf.mu[j] - lastVMF.mu[j]).lengthSq();
						diff += (vmf.kappa[j] - lastVMF.kappa[j]) * 0.1f;
					}
					if (diff < DiffThreshold) { break; }
				}
				lastVMF = vmf;
			}
			delete[] z_data;

			for (int i = 0; i < LobeNum; ++i) {
				ASSERT(!std::isnan(kappa[i]));
				ASSERT(!mu[i].hasNan());
				ASSERT(!std::isnan(alpha[i]));
			}

			return vmf;
		}

		Vector3f sample(Sampler& sampler) const {
			int n = sampler.randiAlongNormalizedWeights(alpha);
			auto& mu = this->mu[n];
			auto& kappa = this->kappa[n];

			if (mu == Vector3f(0, 0, 0)) { return Vector3f(0, 0, 0); }

			Vector2f V;
			float W;

			{
				float u = sampler.randf();
				float v = sampler.randf();
				float r = sqrtf(u); // [0, 1]
				float theta = 2.0f * (float)M_PI * v - M_PI; // [-PI, PI]
				V = Vector2f(r * cosf(theta), r * sinf(theta));
			}
			{
				float u = sampler.randf();
				if (kappa > 0.0f) {
					W = 1.0f + powf(kappa, -1.0f) * logf(u + (1.0f - u) * exp(-2.0f * kappa));
				} else {
					W = 1.0f;
				}
			}

			Vector3f sampleLocal;
			float sqrt = safeSqrt(1.0f - powf(W, 2.0f));
			sampleLocal.x = W;
			sampleLocal.y = sqrt * V.x;
			sampleLocal.z = sqrt * V.y;

			BasisVectors basis(normalize(mu));

			Vector3f sampleGlobal = basis.toGlobal(sampleLocal.x, sampleLocal.y, sampleLocal.z).normalize();

			ASSERT(!sampleGlobal.hasNan());

			return sampleGlobal;
		};

	private:

		static float vMF(const Vector3f& mu, float kappa, const Vector3f& omega) {
			if (kappa == 0.0f) {
				return 1.0f / (4 * (float)M_PI);
			} else {
				return kappa / (2 * (float)M_PI * (1.0f - expf(-2 * kappa))) * expf(kappa * (dot(omega, mu) - 1.0f));
			}
		}

		float eval(const Vector3f& n, int lobe) const {
			const auto& mu = this->mu[lobe];
			const auto& kappa = this->kappa[lobe];
			return vMF(mu, kappa, n);
		}
	};

}