#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <optional>

#include "Utils.h"

namespace Xitils {

	template <typename T, typename T_SIMD, typename T_SIMDMASK> class Vector2 {
	public:

		Vector2() : x(0), y(0) {}
		explicit Vector2(T val) : x(val), y(val) {}
		Vector2(T x, T y) : x(x), y(y) { ASSERT(!hasNan()); }

		explicit Vector2(const glm::tvec2<T> v) : x(v.x), y(v.y) { ASSERT(!hasNan()); }
		glm::tvec2<T> glm() const { return glm::tvec2<T>(x, y); }

		template <typename U, typename U_SIMD, typename U_SIMDMASK> explicit Vector2(const Vector2<U, U_SIMD, U_SIMDMASK>& v) : x(v.x), y(v.y) { ASSERT(!hasNan()); }

		T& operator=(const T& v) { x = v.x; y = v.y; }
		T& operator=(const T&& v) noexcept { x = v.x; y = v.y; }

		T& operator[](int i) const {
			ASSERT(i >= 0 && i <= 1);
			if (i == 0) { return x; }
			return y;
		}

		Vector2<T, T_SIMD, T_SIMDMASK> operator+(const Vector2<T, T_SIMD, T_SIMDMASK>& v) const {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&v);
			Vector2<T, T_SIMD, T_SIMDMASK> res;
			simdpp::store_u(&res, simdpp::add(val1, val2));
			return std::move(res);
		}
		Vector2<T, T_SIMD, T_SIMDMASK>& operator+=(const Vector2<T, T_SIMD, T_SIMDMASK>& v) {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&v);
			auto resVal = simdpp::add(val1, val2);
			simdpp::store_u(this, resVal);
			return *this;
		}
		Vector2<T, T_SIMD, T_SIMDMASK> operator-(const Vector2<T, T_SIMD, T_SIMDMASK>& v) const {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&v);
			auto resVal = simdpp::sub(val1, val2);
			Vector2<T, T_SIMD, T_SIMDMASK> res;
			simdpp::store_u(&res, resVal);
			return std::move(res);
		}
		Vector2<T, T_SIMD, T_SIMDMASK>& operator-=(const Vector2<T, T_SIMD, T_SIMDMASK>& v) {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&v);
			auto resVal = simdpp::sub(val1, val2);
			simdpp::store_u(this, resVal);
			return *this;
		}

		Vector2<T, T_SIMD, T_SIMDMASK> operator+() const { return Vector2<T, T_SIMD, T_SIMDMASK>(x, y); }
		Vector2<T, T_SIMD, T_SIMDMASK> operator-() const { return Vector2<T, T_SIMD, T_SIMDMASK>(-x, -y); }

		Vector2<T, T_SIMD, T_SIMDMASK> operator*(T val) const {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_splat<T_SIMD>(&val);
			Vector2<T, T_SIMD, T_SIMDMASK> res;
			simdpp::store_u(&res, simdpp::mul(val1, val2));
			return std::move(res);
		}
		Vector2<T, T_SIMD, T_SIMDMASK>& operator *=(T val) {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_splat<T_SIMD>(&val);
			simdpp::store_u(this, simdpp::mul(val1, val2));
			return *this;
		}
		Vector2<T, T_SIMD, T_SIMDMASK> operator/(T val) const {
			ASSERT(val != 0);
			float inv = (float)1 / val;
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_splat<T_SIMD>(&inv);
			Vector2<T, T_SIMD, T_SIMDMASK> res;
			simdpp::store_u(&res, simdpp::mul(val1, val2));
			return std::move(res);
		}
		Vector2<T, T_SIMD, T_SIMDMASK>& operator /=(T val) {
			ASSERT(val != 0);
			float inv = (float)1 / val;
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_splat<T_SIMD>(&inv);
			simdpp::store_u(this, simdpp::mul(val1, val2));
			return *this;
		}

		bool operator==(const Vector2<T, T_SIMD, T_SIMDMASK>& v) {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&v);
			auto mask = simdpp::cmp_eq(val1, val2);
			return simdpp::reduce_and(simdpp::bit_cast<simdpp::uint32x4, T_SIMDMASK>(mask)) != 0;
		}
		bool operator!=(const Vector2<T, T_SIMD, T_SIMDMASK>& v) {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&v);
			auto mask = simdpp::cmp_neq(val1, val2);
			return simdpp::reduce_or(simdpp::bit_cast<simdpp::uint32x4, T_SIMDMASK>(mask)) != 0;
		}

		float length() const {
			auto val = simdpp::load_u<T_SIMD>(this);
			return sqrtf(simdpp::reduce_add(simdpp::mul(val, val)));
		}
		float lengthSq() const {
			auto val = simdpp::load_u<T_SIMD>(this);
			return simdpp::reduce_add(simdpp::mul(val, val));
		}

		Vector2<T, T_SIMD, T_SIMDMASK>& normalize() { *this /= length(); return *this; }

		T min() const { return std::min(x, y); }
		T max() const { return std::max(x, y); }
		int minDim() const { return (x < y) ? 0 : 1; }
		int maxDim() const { return (x > y) ? 0 : 1; }

		bool hasNan() {
			auto val = simdpp::load_u<T_SIMD>(this);
			auto mask = simdpp::isnan(val);
			return simdpp::reduce_or(simdpp::bit_cast<simdpp::uint32x4, T_SIMDMASK>(mask)) != 0;
		}
		bool isZero() {
			T_SIMD val1 = simdpp::load_u<T_SIMD>(this);
			T_SIMD val2 = simdpp::make_zero();
			auto mask = simdpp::cmp_eq(val1, val2);
			return simdpp::reduce_and(simdpp::bit_cast<simdpp::uint32x4, T_SIMDMASK>(mask)) != 0;
		}

		union { T x, r, u; };
		union { T y, g, v; };
		float padding;
		float padding2;
	};
	using Vector2f = Vector2<float, simdpp::float32x4, simdpp::float32x4>;
	using Vector2i = Vector2<int, simdpp::int32x4, simdpp::mask_int32x4>;

	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector2<T, T_SIMD, T_SIMDMASK> operator*(float val, const Vector2<T, T_SIMD, T_SIMDMASK>& v) { return v * val; }

	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector2<T, T_SIMD, T_SIMDMASK> normalize(const Vector2<T, T_SIMD, T_SIMDMASK>& v) { return v / v.length(); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector2<T, T_SIMD, T_SIMDMASK> abs(const Vector2<T, T_SIMD, T_SIMDMASK>& v) {
		auto val = simdpp::load_u<T_SIMD>(&v);
		Vector3<T, T_SIMD, T_SIMDMASK> res;
		simdpp::store_u(&res, simdpp::abs(val));
		return std::move(res);
	}
	template <typename T, typename T_SIMD, typename T_SIMDMASK> T dot(const Vector2<T, T_SIMD, T_SIMDMASK>& v1, const Vector2<T, T_SIMD, T_SIMDMASK>& v2) {
		auto val1 = simdpp::load_u<T_SIMD>(&v1);
		auto val2 = simdpp::load_u<T_SIMD>(&v2);
		return simdpp::reduce_add(simdpp::mul(val1, val2));
	}
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector2<T, T_SIMD, T_SIMDMASK> permute(const Vector2<T, T_SIMD, T_SIMDMASK>& v, int x, int y) { return Vector2<T, T_SIMD, T_SIMDMASK>(v[x], v[y]); }

	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector2<T, T_SIMD, T_SIMDMASK> min(const Vector2<T, T_SIMD, T_SIMDMASK>& v1, const Vector2<T, T_SIMD, T_SIMDMASK>& v2) {
		auto val1 = simdpp::load_u<T_SIMD>(&v1);
		auto val2 = simdpp::load_u<T_SIMD>(&v2);
		Vector2<T, T_SIMD, T_SIMDMASK> res;
		simdpp::store_u(&res, simdpp::min(val1, val2));
		return std::move(res);
	}
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector2<T, T_SIMD, T_SIMDMASK> min(const Vector2<T, T_SIMD, T_SIMDMASK>& v1, const Vector2<T, T_SIMD, T_SIMDMASK>& v2, const Vector2<T, T_SIMD, T_SIMDMASK>& v3) { return min(v1, v2); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector2<T, T_SIMD, T_SIMDMASK> max(const Vector2<T, T_SIMD, T_SIMDMASK>& v1, const Vector2<T, T_SIMD, T_SIMDMASK>& v2) {
		T_SIMD val1 = simdpp::load_u<T_SIMD>(&v1);
		T_SIMD val2 = simdpp::load_u<T_SIMD>(&v2);
		Vector2<T, T_SIMD, T_SIMDMASK> res;
		simdpp::store_u(&res, simdpp::max(val1, val2));
		return std::move(res);
	}
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector2<T, T_SIMD, T_SIMDMASK> max(const Vector2<T, T_SIMD, T_SIMDMASK>& v1, const Vector2<T, T_SIMD, T_SIMDMASK>& v2, const Vector2<T, T_SIMD, T_SIMDMASK>& v3) { return max(v1, max(v2, v3)); }

	template <typename T, typename T_SIMD, typename T_SIMDMASK> bool inRange(const Vector2<T, T_SIMD, T_SIMDMASK>& v, T minVal, T maxVal) {
		T_SIMD val = simdpp::load_u<T_SIMD>(&v);
		T_SIMDMASK mask = simdpp::cmp_ge(val, simdpp::load_splat<T_SIMD>(&minVal));
		mask = mask & simdpp::cmp_le(val, simdpp::load_splat<T_SIMD>(&maxVal));
		mask = mask | simdpp::to_mask(simdpp::make_float<T_SIMD>(0, 0, 0, 1));
		return simdpp::reduce_and(simdpp::bit_cast<simdpp::uint32x4, T_SIMDMASK>(mask)) != 0;
	}
	template <typename T, typename T_SIMD, typename T_SIMDMASK> bool inRange01(const Vector2<T, T_SIMD, T_SIMDMASK>& v) { return inRange(v, 0, 1); }

	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector2<T, T_SIMD, T_SIMDMASK> clamp(const Vector2<T, T_SIMD, T_SIMDMASK>& v, T minVal, T maxVal) {
		T_SIMD val1 = simdpp::load_u<T_SIMD>(&v1);
		T_SIMD val2 = simdpp::load_splat<T_SIMD>(&minVal);
		T_SIMD val3 = simdpp::load_splat<T_SIMD>(&maxVal);
		val1 = simdpp::blend(val1, val2, simdpp::cmp_lt(val1, val2));
		val1 = simdpp::blend(val1, val3, simdpp::cmp_gt(val1, val3));
		val1 = simdpp::insert<3, 4>(val1, 0);
		Vector2<T, T_SIMD, T_SIMDMASK> res;
		simdpp::store_u(&res, val1);
		return std::move(res);
	}
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector2<T, T_SIMD, T_SIMDMASK> clamp01(const Vector2<T, T_SIMD, T_SIMDMASK>& v) { return clamp(v, 0, 1); }




	template <typename T, typename T_SIMD, typename T_SIMDMASK> class Vector3 {
	public:
		Vector3() : x(0), y(0), z(0) {}
		explicit Vector3(T val) : x(val), y(val), z(val) {}
		Vector3(T x, T y, T z) : x(x), y(y), z(z) { ASSERT(!hasNan()); }

		explicit Vector3(const glm::tvec3<T> v) : x(v.x), y(v.y), z(v.z) { ASSERT(!hasNan()); }
		glm::tvec3<T> glm() const { return glm::tvec3<T>(x, y, z); }

		template <typename U, typename U_SIMD, typename U_SIMDMASK> explicit Vector3(const Vector3<U, U_SIMD, U_SIMDMASK>& v) : x(v.x), y(v.y), z(v.z) { ASSERT(!hasNan()); }

		T& operator=(const T& v) { x = v.x; y = v.y; z = v.z; }
		T& operator=(const T&& v) noexcept { x = v.x; y = v.y; z = v.z; }

		T& operator[](int i) {
			ASSERT(i >= 0 && i <= 2);
			if (i == 0) { return x; }
			if (i == 1) { return y; }
			return z;
		}

		Vector3<T, T_SIMD, T_SIMDMASK> operator+(const Vector3<T, T_SIMD, T_SIMDMASK>& v) const {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&v);
			Vector3<T, T_SIMD, T_SIMDMASK> res;
			simdpp::store_u(&res, simdpp::add(val1, val2));
			return std::move(res);
		}
		Vector3<T, T_SIMD, T_SIMDMASK>& operator+=(const Vector3<T, T_SIMD, T_SIMDMASK>& v) {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&v);
			auto resVal = simdpp::add(val1, val2);
			simdpp::store_u(this, resVal);
			return *this;
		}
		Vector3<T, T_SIMD, T_SIMDMASK> operator-(const Vector3<T, T_SIMD, T_SIMDMASK>& v) const {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&v);
			auto resVal = simdpp::sub(val1, val2);
			Vector3<T, T_SIMD, T_SIMDMASK> res;
			simdpp::store_u(&res, resVal);
			return std::move(res);
		}
		Vector3<T, T_SIMD, T_SIMDMASK>& operator-=(const Vector3<T, T_SIMD, T_SIMDMASK>& v) {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&v);
			auto resVal = simdpp::sub(val1, val2);
			simdpp::store_u(this, resVal);
			return *this;
		}

		Vector3<T, T_SIMD, T_SIMDMASK> operator+() const { return Vector3<T, T_SIMD, T_SIMDMASK>(x, y, z); }
		Vector3<T, T_SIMD, T_SIMDMASK> operator-() const { return Vector3<T, T_SIMD, T_SIMDMASK>(-x, -y, -z); }

		Vector3<T, T_SIMD, T_SIMDMASK> operator*(T val) const {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_splat<T_SIMD>(&val);
			Vector3<T, T_SIMD, T_SIMDMASK> res;
			simdpp::store_u(&res, simdpp::mul(val1, val2));
			return std::move(res);
		}
		Vector3<T, T_SIMD, T_SIMDMASK>& operator *=(T val) {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_splat<T_SIMD>(&val);
			simdpp::store_u(this, simdpp::mul(val1, val2));
			return *this;
		}
		Vector3<T, T_SIMD, T_SIMDMASK> operator/(T val) const {
			ASSERT(val != 0);
			float inv = (float)1 / val;
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_splat<T_SIMD>(&inv);
			Vector3<T, T_SIMD, T_SIMDMASK> res;
			simdpp::store_u(&res, simdpp::mul(val1, val2));
			return std::move(res);
		}
		Vector3<T, T_SIMD, T_SIMDMASK>& operator /=(T val) {
			ASSERT(val != 0);
			float inv = (float)1 / val;
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_splat<T_SIMD>(&inv);
			simdpp::store_u(this, simdpp::mul(val1, val2));
			return *this;
		}

		bool operator==(const Vector3<T, T_SIMD, T_SIMDMASK>& v) {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&v);
			auto mask = simdpp::cmp_eq(val1, val2);
			return simdpp::reduce_and(simdpp::bit_cast<simdpp::uint32x4, T_SIMDMASK>(mask)) != 0;
		}
		bool operator!=(const Vector3<T, T_SIMD, T_SIMDMASK>& v) {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&v);
			auto mask = simdpp::cmp_neq(val1, val2);
			return simdpp::reduce_or(simdpp::bit_cast<simdpp::uint32x4, T_SIMDMASK>(mask)) != 0;
		}

		float length() const {
			auto val = simdpp::load_u<T_SIMD>(this);
			return sqrtf(simdpp::reduce_add(simdpp::mul(val, val)));
		}
		float lengthSq() const {
			auto val = simdpp::load_u<T_SIMD>(this);
			return simdpp::reduce_add(simdpp::mul(val, val));
		}

		Vector3<T, T_SIMD, T_SIMDMASK>& normalize() { *this /= length(); return *this; }

		T min() const {
			auto val = simdpp::load_u<T_SIMD>(this);
			val = simdpp::insert<3, 4>(val, Infinity);
			return simdpp::reduce_min(val);
		}
		T max() const {
			auto val = simdpp::load_u<T_SIMD>(this);
			val = simdpp::insert<3, 4>(val, -Infinity);
			return simdpp::reduce_max(val);
		}
		int minDim() const { return (x < y && x < z) ? 0 : ((y < z) ? 1 : 2); }
		int maxDim() const { return (x > y && x > z) ? 0 : ((y > z) ? 1 : 2); }

		bool hasNan() {
			auto val = simdpp::load_u<T_SIMD>(this);
			auto mask = simdpp::isnan(val);
			return simdpp::reduce_or(simdpp::bit_cast<simdpp::uint32x4, T_SIMDMASK>(mask)) != 0;
		}
		bool isZero() {
			T_SIMD val1 = simdpp::load_u<T_SIMD>(this);
			T_SIMD val2 = simdpp::make_zero();
			auto mask = simdpp::cmp_eq(val1, val2);
			return simdpp::reduce_and(simdpp::bit_cast<simdpp::uint32x4, T_SIMDMASK>(mask)) != 0;
		}

		union { T x, r; };
		union { T y, g; };
		union { T z, b; };
		float padding = 0;

	};
	using Vector3f = Vector3<float, simdpp::float32x4, simdpp::mask_float32x4>;
	using Vector3i = Vector3<int, simdpp::int32x4, simdpp::mask_int32x4>;

	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector3<T, T_SIMD, T_SIMDMASK> operator*(float val, const Vector3<T, T_SIMD, T_SIMDMASK>& v) { return v * val; }

	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector3<T, T_SIMD, T_SIMDMASK> normalize(const Vector3<T, T_SIMD, T_SIMDMASK>& v) { return v / v.length(); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector3<T, T_SIMD, T_SIMDMASK> abs(const Vector3<T, T_SIMD, T_SIMDMASK>& v) {
		auto val = simdpp::load_u<T_SIMD>(&v);
		Vector3<T, T_SIMD, T_SIMDMASK> res;
		simdpp::store_u(&res, simdpp::abs(val));
		return std::move(res);
	}
	template <typename T, typename T_SIMD, typename T_SIMDMASK> T dot(const Vector3<T, T_SIMD, T_SIMDMASK>& v1, const Vector3<T, T_SIMD, T_SIMDMASK>& v2) {
		auto val1 = simdpp::load_u<T_SIMD>(&v1);
		auto val2 = simdpp::load_u<T_SIMD>(&v2);
		return simdpp::reduce_add(simdpp::mul(val1, val2));
	}
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector3<T, T_SIMD, T_SIMDMASK> cross(const Vector3<T, T_SIMD, T_SIMDMASK>& v1, const Vector3<T, T_SIMD, T_SIMDMASK>& v2) {
		simdpp::float64x4 val1 = simdpp::to_float64(simdpp::load_u<T_SIMD>(&v1));
		simdpp::float64x4 val2 = simdpp::to_float64(simdpp::load_u<T_SIMD>(&v2));

		simdpp::float64x4 val1_yzx = simdpp::shuffle4x2<1, 2, 0, 3>(val1, val1);
		simdpp::float64x4 val2_yzx = simdpp::shuffle4x2<1, 2, 0, 3>(val2, val2);

		simdpp::float64x4 a1 = simdpp::mul(val1, val2_yzx);
		simdpp::float64x4 a2 = simdpp::mul(val1_yzx, val2);
		simdpp::float64x4 b = simdpp::sub(a1, a2);
		b = simdpp::shuffle4x2<1, 2, 0, 3>(b, b);

		Vector3<T, T_SIMD, T_SIMDMASK> res;
		simdpp::store_u(&res, simdpp::to_float32(b));

		return std::move(res);
	}
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector3<T, T_SIMD, T_SIMDMASK> permute(const Vector3<T, T_SIMD, T_SIMDMASK>& v, int x, int y, int z) { return Vector3<T, T_SIMD, T_SIMDMASK>(v[x], v[y], v[z]); }

	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector3<T, T_SIMD, T_SIMDMASK> min(const Vector3<T, T_SIMD, T_SIMDMASK>& v1, const Vector3<T, T_SIMD, T_SIMDMASK>& v2) {
		auto val1 = simdpp::load_u<T_SIMD>(&v1);
		auto val2 = simdpp::load_u<T_SIMD>(&v2);
		Vector3<T, T_SIMD, T_SIMDMASK> res;
		simdpp::store_u(&res, simdpp::min(val1, val2));
		return std::move(res);
	}
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector3<T, T_SIMD, T_SIMDMASK> min(const Vector3<T, T_SIMD, T_SIMDMASK>& v1, const Vector3<T, T_SIMD, T_SIMDMASK>& v2, const Vector3<T, T_SIMD, T_SIMDMASK>& v3) {return min(v1, min(v2, v3)); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector3<T, T_SIMD, T_SIMDMASK> max(const Vector3<T, T_SIMD, T_SIMDMASK>& v1, const Vector3<T, T_SIMD, T_SIMDMASK>& v2) {
		T_SIMD val1 = simdpp::load_u<T_SIMD>(&v1);
		T_SIMD val2 = simdpp::load_u<T_SIMD>(&v2);
		Vector3<T, T_SIMD, T_SIMDMASK> res;
		simdpp::store_u(&res, simdpp::max(val1, val2));
		return std::move(res);
	}
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector3<T, T_SIMD, T_SIMDMASK> max(const Vector3<T, T_SIMD, T_SIMDMASK>& v1, const Vector3<T, T_SIMD, T_SIMDMASK>& v2, const Vector3<T, T_SIMD, T_SIMDMASK>& v3) { return max(v1, max(v2, v3)); }

	template <typename T, typename T_SIMD, typename T_SIMDMASK> bool inRange(const Vector3<T, T_SIMD, T_SIMDMASK>& v, T minVal, T maxVal) {
		T_SIMD val = simdpp::load_u<T_SIMD>(&v);
		T_SIMDMASK mask = simdpp::cmp_ge(val, simdpp::load_splat<T_SIMD>(&minVal));
		mask = mask & simdpp::cmp_le(val, simdpp::load_splat<T_SIMD>(&maxVal));
		mask = mask | simdpp::to_mask( simdpp::make_float<T_SIMD>(0,0,0,1) );
		return simdpp::reduce_and(simdpp::bit_cast<simdpp::uint32x4, T_SIMDMASK>(mask)) != 0;
	}
	template <typename T, typename T_SIMD, typename T_SIMDMASK> bool inRange01(const Vector3<T, T_SIMD, T_SIMDMASK>& v) { return inRange(v, (T)0, (T)1); }

	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector3<T, T_SIMD, T_SIMDMASK> clamp(const Vector3<T, T_SIMD, T_SIMDMASK>& v, T minVal, T maxVal) {
		T_SIMD val1 = simdpp::load_u<T_SIMD>(&v1);
		T_SIMD val2 = simdpp::load_splat<T_SIMD>(&minVal);
		T_SIMD val3 = simdpp::load_splat<T_SIMD>(&maxVal);
		val1 = simdpp::blend(val1, val2, simdpp::cmp_lt(val1, val2));
		val1 = simdpp::blend(val1, val3, simdpp::cmp_gt(val1, val3));
		val1 = simdpp::insert<3,4>(val1, 0);
		Vector3<T, T_SIMD, T_SIMDMASK> res;
		simdpp::store_u(&res, val1);
		return std::move(res);
	}
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector3<T, T_SIMD, T_SIMDMASK> clamp01(const Vector3<T, T_SIMD, T_SIMDMASK>& v) { return clamp(v, 0, 1); }




	template <typename T, typename T_SIMD, typename T_SIMDMASK> class Vector4 {
	public:
		Vector4() : x(0), y(0), z(0), w(0) {}
		explicit Vector4(T val) : x(val), y(val), z(val), w(val) {}
		Vector4(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) { ASSERT(!hasNan()); }

		explicit Vector4(const glm::tvec4<T> v) : x(v.x), y(v.y), z(v.z), w(v.w) { ASSERT(!hasNan()); }
		glm::tvec4<T> glm() const { return glm::tvec4<T>(x, y, z, w); }

		template <typename U, typename U_SIMD, typename U_SIMDMASK> explicit Vector4(const Vector4<U, U_SIMD, U_SIMDMASK>& v) : x(v.x), y(v.y), z(v.z), w(v.w) { ASSERT(!hasNan()); }

		T& operator=(const T& v) { x = v.x; y = v.y; z = v.z; w = v.w; }
		T& operator=(const T&& v) noexcept { x = v.x; y = v.y; z = v.z; w = v.w; }

		T& operator[](int i) {
			ASSERT(i >= 0 && i <= 3);
			if (i == 0) { return x; }
			if (i == 1) { return y; }
			if (i == 2) { return z; }
			return w;
		}

		Vector4<T, T_SIMD, T_SIMDMASK> operator+(const Vector4<T, T_SIMD, T_SIMDMASK>& v) const {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&v);
			Vector4<T, T_SIMD, T_SIMDMASK> res;
			simdpp::store_u(&res, simdpp::add(val1, val2));
			return std::move(res);
		}
		Vector4<T, T_SIMD, T_SIMDMASK>& operator+=(const Vector4<T, T_SIMD, T_SIMDMASK>& v) {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&v);
			auto resVal = simdpp::add(val1, val2);
			simdpp::store_u(this, resVal);
			return *this;
		}
		Vector4<T, T_SIMD, T_SIMDMASK> operator-(const Vector4<T, T_SIMD, T_SIMDMASK>& v) const {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&v);
			auto resVal = simdpp::sub(val1, val2);
			Vector4<T, T_SIMD, T_SIMDMASK> res;
			simdpp::store_u(&res, resVal);
			return std::move(res);
		}
		Vector4<T, T_SIMD, T_SIMDMASK>& operator-=(const Vector4<T, T_SIMD, T_SIMDMASK>& v) {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&v);
			auto resVal = simdpp::sub(val1, val2);
			simdpp::store_u(this, resVal);
			return *this;
		}

		Vector4<T, T_SIMD, T_SIMDMASK> operator+() const { return Vector4<T, T_SIMD, T_SIMDMASK>(x, y, z, w); }
		Vector4<T, T_SIMD, T_SIMDMASK> operator-() const { return Vector4<T, T_SIMD, T_SIMDMASK>(-x, -y, -z, -w); }

		Vector4<T, T_SIMD, T_SIMDMASK> operator*(T val) const {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_splat<T_SIMD>(&val);
			Vector4<T, T_SIMD, T_SIMDMASK> res;
			simdpp::store_u(&res, simdpp::mul(val1, val2));
			return std::move(res);
		}
		Vector4<T, T_SIMD, T_SIMDMASK>& operator *=(T val) {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_splat<T_SIMD>(&val);
			simdpp::store_u(this, simdpp::mul(val1, val2));
			return *this;
		}
		Vector4<T, T_SIMD, T_SIMDMASK> operator/(T val) const {
			ASSERT(val != 0);
			float inv = (float)1 / val;
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_splat<T_SIMD>(&inv);
			Vector4<T, T_SIMD, T_SIMDMASK> res;
			simdpp::store_u(&res, simdpp::mul(val1, val2));
			return std::move(res);
		}
		Vector4<T, T_SIMD, T_SIMDMASK>& operator /=(T val) {
			ASSERT(val != 0);
			float inv = (float)1 / val;
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_splat<T_SIMD>(&inv);
			simdpp::store_u(this, simdpp::mul(val1, val2));
			return *this;
		}

		bool operator==(const Vector4<T, T_SIMD, T_SIMDMASK>& v) {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&v);
			auto mask = simdpp::cmp_eq(val1, val2);
			return simdpp::reduce_and(simdpp::bit_cast<simdpp::uint32x4, T_SIMDMASK>(mask)) != 0;
		}
		bool operator!=(const Vector4<T, T_SIMD, T_SIMDMASK>& v) {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&v);
			auto mask = simdpp::cmp_neq(val1, val2);
			return simdpp::reduce_or(simdpp::bit_cast<simdpp::uint32x4, T_SIMDMASK>(mask)) != 0;
		}

		float length() const {
			auto val = simdpp::load_u<T_SIMD>(this);
			return sqrtf(simdpp::reduce_add(simdpp::mul(val, val)));
		}
		float lengthSq() const {
			auto val = simdpp::load_u<T_SIMD>(this);
			return simdpp::reduce_add(simdpp::mul(val, val));
		}

		Vector4<T, T_SIMD, T_SIMDMASK>& normalize() { *this /= length(); return *this; }

		T min() const {
			auto val = simdpp::load_u<T_SIMD>(this);
			return simdpp::reduce_min(val);
		}
		T max() const {
			auto val = simdpp::load_u<T_SIMD>(this);
			return simdpp::reduce_max(val);
		}
		int minDim() const {
			if (x < y && x < z && x < z) { return 0; }
			if (y < z && y < w) { return 1; }
			if (z < w) { return 2; }
			return 3;
		}
		int maxDim() const {
			if (x > y && x > z && x > z) { return 0; }
			if (y > z && y > w) { return 1; }
			if (z > w) { return 2; }
			return 3;
		}

		bool hasNan() {
			auto val = simdpp::load_u<T_SIMD>(this);
			auto mask = simdpp::isnan(val);
			return simdpp::reduce_or(simdpp::bit_cast<simdpp::uint32x4, T_SIMDMASK>(mask)) != 0;
		}
		bool isZero() {
			T_SIMD val1 = simdpp::load_u<T_SIMD>(this);
			T_SIMD val2 = simdpp::make_zero();
			auto mask = simdpp::cmp_eq(val1, val2);
			return simdpp::reduce_and(simdpp::bit_cast<simdpp::uint32x4, T_SIMDMASK>(mask)) != 0;
		}

		union { T x, r; };
		union { T y, g; };
		union { T z, b; };
		union { T w, a; };

	};
	using Vector4f = Vector4<float, simdpp::float32x4, simdpp::mask_float32x4>;
	using Vector4i = Vector4<int, simdpp::int32x4, simdpp::mask_int32x4>;

	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector4<T, T_SIMD, T_SIMDMASK> operator*(float val, const Vector4<T, T_SIMD, T_SIMDMASK>& v) { return v * val; }

	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector4<T, T_SIMD, T_SIMDMASK> normalize(const Vector4<T, T_SIMD, T_SIMDMASK>& v) { return v / v.length(); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector4<T, T_SIMD, T_SIMDMASK> abs(const Vector4<T, T_SIMD, T_SIMDMASK>& v) {
		auto val = simdpp::load_u<T_SIMD>(&v);
		Vector4<T, T_SIMD, T_SIMDMASK> res;
		simdpp::store_u(&res, simdpp::abs(val));
		return std::move(res);
	}
	template <typename T, typename T_SIMD, typename T_SIMDMASK> T dot(const Vector4<T, T_SIMD, T_SIMDMASK>& v1, const Vector4<T, T_SIMD, T_SIMDMASK>& v2) {
		auto val1 = simdpp::load_u<T_SIMD>(&v1);
		auto val2 = simdpp::load_u<T_SIMD>(&v2);
		return simdpp::reduce_add(simdpp::mul(val1, val2));
	}
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector4<T, T_SIMD, T_SIMDMASK> permute(const Vector4<T, T_SIMD, T_SIMDMASK>& v, int x, int y, int z, int w) { return Vector4<T, T_SIMD, T_SIMDMASK>(v[x], v[y], v[z], v[w]); }

	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector4<T, T_SIMD, T_SIMDMASK> min(const Vector4<T, T_SIMD, T_SIMDMASK>& v1, const Vector4<T, T_SIMD, T_SIMDMASK>& v2) {
		auto val1 = simdpp::load_u<T_SIMD>(&v1);
		auto val2 = simdpp::load_u<T_SIMD>(&v2);
		Vector4<T, T_SIMD, T_SIMDMASK> res;
		simdpp::store_u(&res, simdpp::min(val1, val2));
		return std::move(res);
	}
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector4<T, T_SIMD, T_SIMDMASK> min(const Vector4<T, T_SIMD, T_SIMDMASK>& v1, const Vector4<T, T_SIMD, T_SIMDMASK>& v2, const Vector4<T, T_SIMD, T_SIMDMASK>& v3) { return min(v1, min(v2, v3)); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector4<T, T_SIMD, T_SIMDMASK> max(const Vector4<T, T_SIMD, T_SIMDMASK>& v1, const Vector4<T, T_SIMD, T_SIMDMASK>& v2) {
		T_SIMD val1 = simdpp::load_u<T_SIMD>(&v1);
		T_SIMD val2 = simdpp::load_u<T_SIMD>(&v2);
		Vector4<T, T_SIMD, T_SIMDMASK> res;
		simdpp::store_u(&res, simdpp::max(val1, val2));
		return std::move(res);
	}
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector4<T, T_SIMD, T_SIMDMASK> max(const Vector4<T, T_SIMD, T_SIMDMASK>& v1, const Vector4<T, T_SIMD, T_SIMDMASK>& v2, const Vector4<T, T_SIMD, T_SIMDMASK>& v3) { return max(v1, max(v2, v3)); }

	template <typename T, typename T_SIMD, typename T_SIMDMASK> bool inRange(const Vector4<T, T_SIMD, T_SIMDMASK>& v, T minVal, T maxVal) {
		T_SIMD val = simdpp::load_u<T_SIMD>(&v);
		T_SIMDMASK mask = simdpp::cmp_ge(val, simdpp::load_splat<T_SIMD>(&minVal));
		mask = mask & simdpp::cmp_le(val, simdpp::load_splat<T_SIMD>(&maxVal));
		mask = mask | simdpp::to_mask(simdpp::make_float<T_SIMD>(0, 0, 0, 1));
		return simdpp::reduce_and(simdpp::bit_cast<simdpp::uint32x4, T_SIMDMASK>(mask)) != 0;
	}
	template <typename T, typename T_SIMD, typename T_SIMDMASK> bool inRange01(const Vector4<T, T_SIMD, T_SIMDMASK>& v) { return inRange(v, (T)0, (T)1); }

	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector4<T, T_SIMD, T_SIMDMASK> clamp(const Vector4<T, T_SIMD, T_SIMDMASK>& v, T minVal, T maxVal) {
		T_SIMD val1 = simdpp::load_u<T_SIMD>(&v1);
		T_SIMD val2 = simdpp::load_splat<T_SIMD>(&minVal);
		T_SIMD val3 = simdpp::load_splat<T_SIMD>(&maxVal);
		val1 = simdpp::blend(val1, val2, simdpp::cmp_lt(val1, val2));
		val1 = simdpp::blend(val1, val3, simdpp::cmp_gt(val1, val3));
		val1 = simdpp::insert<3, 4>(val1, 0);
		Vector4<T, T_SIMD, T_SIMDMASK> res;
		simdpp::store_u(&res, val1);
		return std::move(res);
	}
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector4<T, T_SIMD, T_SIMDMASK> clamp01(const Vector4<T, T_SIMD, T_SIMDMASK>& v) { return clamp(v, 0, 1); }

}