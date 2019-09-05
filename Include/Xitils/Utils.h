#pragma once

#define _USE_MATH_DEFINES
#define SIMDPP_ARCH_X86_AVX2
#define GLM_FORCE_AVX2
//#define GLM_FORCE_INLNIE

#include <vector>
#include <random>
#include <math.h>
#include <functional>
#include <numeric>
#include <glm/glm.hpp>
#include <simdpp/simd.h>

#undef INFINITY


#ifdef DEBUG
#define ASSERT(x) assert(x);
#else
#define ASSERT ;
#endif

#define NOT_IMPLEMENTED throw "Not implemented";


namespace Xitils {

	static const float Pi = (float)M_PI;
	static const float ToDeg = 180.0f / (float)Pi;
	static const float ToRad = (float)Pi / 180.0f;
	static const float Infinity = std::numeric_limits<float>::infinity();

	template <typename T> T min(const T& x, const T& y) { return x < y ? x : y; }
	template <typename T> T max(const T& x, const T& y) { return x > y ? x : y; }
	template <typename T> T min(const T& x, const T& y, const T& z) { return min(x, min(y, z)); }
	template <typename T> T max(const T& x, const T& y, const T& z) { return max(x, max(y, z)); }

	template <typename T> T clamp(const T& x, const T& minVal, const T& maxVal) { return max(minVal, min(maxVal, x)); }
	template <typename T> T clamp01(const T& x) { return clamp(x, (T)0.0f, (T)1.0f); }
	template <typename T> T clampPositive(const T& x) { return max(x, (T)0.0f); }

	inline bool inRange(float x, float minVal, float maxVal) { return minVal <= x && x <= maxVal; }
	inline bool inRange01(float x) { return inRange(x, 0.0f, 1.0f); }

	template <typename T, typename U> void map(const std::vector<T>& src, std::vector<U>* dest, const std::function<U(const T&)>& f) {
		dest->clear();
		dest->resize(src.size());
		std::transform(src.begin(), src.end(), dest->begin(), f);
	}


	template <typename T> T reduce(const std::vector<T>& src, T initialValue, const std::function<T(T, T)>& f) {
		return std::accumulate(src.begin(), src.end(), initialValue, f);
	}

	template <typename T> T sum(const std::vector<T>& src) {
		return std::accumulate(src.begin(), src.end(), 0, [](T a, T b) { return a + b; });
	}

	template <typename A, typename T> void filter(const A& src, A* dest, const std::function<bool(T)>& f) {
		dest->clear();
		std::copy_if(src.begin(), src.end(), std::back_inserter(*dest), f);
	}

	template <typename A, typename B, typename C, typename T, typename U, typename V> void zip(const A& src0, const B& src1, C* dest, const std::function<V(T, U)>& f) {
		dest->clear();
		for (int i = 0; i < src0.size() && i < src1.size(); i++) {
			dest->push_back(f(src0[i], src1[i]));
		}
	}

	template <typename A> A lerp(A x, A y, float t) {
		return (1.0f - t) * x + t * y;
	}

	template <typename A> A lerp(A x, A y, A z, float t1, float t2) {
		return t1 * x + t2 * y + (1.0f - t1 - t2) * z;
	}

	template <typename T> T id(T x) { return x; }


	float safeSqrt(float x) { return sqrtf(clampPositive(x)); }
}