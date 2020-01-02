#pragma once

#include "Texture.h"
#include "Utils.h"
#include "Vector.h"


namespace xitils {

	template <int _L> 
	class SphericalHarmonics {
	public: 

		static const int L = _L;
		static const int CoeffNum = (L + 1) * (L + 1);
		static const int PNum = (L + 1) * (L + 2) / 2;
		std::function<float(float)> P[PNum];

		SphericalHarmonics(int samples, const std::function<float(const Vector3f&)>& func, Sampler& sampler) {

			// TODO: P をコンパイル時に生成

			P[0] = [](float t) { return 1.0f; };

			for (int l = 1; l <= L; ++l) {
				int i = pIndex(l, l);
				P[i] = [l](float t) {
					return (l % 2 == 0 ? 1 : -1) * dfact(2 * l - 1) * powf(1 - t * t, l / 2.0f);
				};
			}
			for (int l = 1; l <= L; ++l) {
				int m = l - 1;
				int i = pIndex(l, m);
				P[i] = [this, l, m](float t) {
					return t * (2 * m + 1) * P[pIndex(m, m)](t);
				};
			}

			for (int l = 2; l <= L; ++l) {
				for (int m = 0; m <= l - 2; ++m) {
					int i = pIndex(l, m);
					P[i] = [this, l, m](float t) {
						return (t * (2 * l - 1) * P[pIndex(l - 1, m)](t) - (l + m - 1) * P[pIndex(l - 2, m)](t)) / (float)(l - m);
					};
				}
			}

			estimateCoeff(samples, func, sampler);
		}

		float eval(const Vector2f& angles) const {
			float ret = 0.0f;
			float theta = angles.x;
			float phi = angles.y;

			for (int l = 0; l <= L; ++l) {
				for (int m = -l; m <= l; ++m) {
					ret += coeff(l, m) * Y(l, m, theta, phi);
				}
			}
			return ret;
		}

		float eval(const Vector3f& v) const {
			return eval(vectorToAngles(v));
		}

	private:

		float coeffs[CoeffNum];

		static int pIndex(int l, int m) {
			assert(m >= 0);
			int offset = l * (l + 1) / 2;
			return offset + m;
		}

		static int coeffIndex(int l, int m) {
			int l2 = l * l;
			int offsetm = ((l + 1) * (l + 1) - l2 - 1) / 2;
			return l2 + offsetm + m;
		}

		float& coeff(int l, int m) {
			return coeffs[coeffIndex(l, m)];
		}

		const float& coeff(int l, int m) const {
			return coeffs[coeffIndex(l,m)];
		}

		void estimateCoeff(int samples, const std::function<float(const Vector3f&)>& func, Sampler& sampler) {
			// TODO: インポータンスサンプリング			

			for (int l = 0; l <= L; ++l) {
				for (int m = -l; m <= l; ++m) {
					float c = 0;

					for (int n = 0; n < samples; ++n) {
						Vector2f angles = sampleAnglesFromSphere(sampler);
						float theta = angles.x;
						float phi = angles.y;

						Vector3f v = anglesToVector(angles);
						c += func(v) * Y(l, m, theta, phi);
					}
					c *= 4 * Pi / samples;

					coeff(l, m) = c;
				}
			}
		}

		float Y(int l, int m, float theta, float phi) const {
			float f = 1;
			for (int i = l - fabsf(m) + 1; i <= l + fabsf(m); ++i) {
				f *= i;
			}
			float K = sqrtf((2 * l + 1) / (4 * Pi * f));

			if (m == 0) {
				return K * P[pIndex(l, 0)](cosf(theta));
			} else if (m > 0) {
				return sqrtf(2) * K * P[pIndex(l, m)](cosf(theta)) * cosf(m * phi);
			} else {
				return sqrtf(2) * K * P[pIndex(l, -m)](cosf(theta)) * sinf(-m * phi);
			}
		}


	};
}

