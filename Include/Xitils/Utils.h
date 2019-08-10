#pragma once

#define _USE_MATH_DEFINES
#define SIMDPP_ARCH_X86_AVX2
#define GLM_FORCE_AVX2
//#define GLM_FORCE_INLNIE

#include <vector>
#include <random>
#include <math.h>
#include <functional>
#include <glm/glm.hpp>
#include <simdpp/simd.h>

#undef INFINITY


#ifdef DEBUG
#define ASSERT(x) assert(x);
#else
#define ASSERT ;
#endif


namespace Xitils {

	static const float Pi = (float)M_PI;
	static const float ToDeg = 180.0f / (float)Pi;
	static const float ToRad = (float)Pi / 180.0f;
	static const float Infinity = std::numeric_limits<float>::infinity();

	template <typename ValueType> ValueType min(const ValueType& x, const ValueType& y) { return x < y ? x : y; }
	template <typename ValueType> ValueType max(const ValueType& x, const ValueType& y) { return x > y ? x : y; }
	template <typename ValueType> ValueType min(const ValueType& x, const ValueType& y, const ValueType& z) { return min(x, min(y, z)); }
	template <typename ValueType> ValueType max(const ValueType& x, const ValueType& y, const ValueType& z) { return max(x, max(y, z)); }

	template <typename ValueType> ValueType clamp(const ValueType& x, const ValueType& minVal, const ValueType& maxVal) { return max(minVal, min(maxVal, x)); }
	template <typename ValueType> ValueType clamp01(const ValueType& x) { return clamp(x, (ValueType)0.0f, (ValueType)1.0f); }
	template <typename ValueType> ValueType clampPositive(const ValueType& x) { return max(x, (ValueType)0.0f); }

	inline bool inRange(float x, float minVal, float maxVal) { return minVal <= x && x <= maxVal; }
	inline bool inRange01(float x) { return inRange(x, 0.0f, 1.0f); }

	template <typename A, typename ValueType, typename U> void map(const A& src, A* dest, const std::function<U(ValueType)>& f) {
		dest->clear();
		dest->reserve(src->size());
		std::transform(src.begin(), src.end(), dest->begin(), f);
	}

	template <typename A, typename ValueType> void map(const A& src, A* dest, const std::function<ValueType(ValueType, ValueType)>& f) {
		dest->clear();
		dest->reserve(src->size());
		std::transform(src.begin(), src.end(), dest->begin(), f);
	}

	template <typename A, typename ValueType, typename U> U reduce(const A& src, U initialValue, const std::function<U(U, ValueType)>& f) {
		return std::accumulate(src.begin(), src.end(), initialValue, f);
	}

	template <typename A, typename ValueType> void filter(const A& src, A* dest, const std::function<bool(ValueType)>& f) {
		dest->clear();
		std::copy_if(src.begin(), src.end(), std::back_inserter(*dest), f);
	}

	template <typename A, typename B, typename C, typename ValueType, typename U, typename V> void zip(const A& src0, const B& src1, C* dest, const std::function<V(ValueType, U)>& f) {
		dest->clear();
		for (int i = 0; i < src0.size() && i < src1.size(); i++) {
			dest->push_back(f(src0[i], src1[i]));
		}
	}

	template <typename A> A interpolate(A x, A y, float t) {
		return (1.0f - t) * x + t * y;
	}

	template <typename ValueType> ValueType id(ValueType x) { return x; }

}