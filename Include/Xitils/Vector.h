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

		Vector2<T, T_SIMD, T_SIMDMASK> operator+(const Vector2<T, T_SIMD, T_SIMDMASK>& v) const { return Vector2<T, T_SIMD, T_SIMDMASK>(x + v.x, y + v.y); }
		Vector2<T, T_SIMD, T_SIMDMASK>& operator+=(const Vector2<T, T_SIMD, T_SIMDMASK>& v) {
			x += v.x;
			y += v.y;
			return *this;
		}
		Vector2<T, T_SIMD, T_SIMDMASK> operator-(const Vector2<T, T_SIMD, T_SIMDMASK>& v) const { return Vector2<T, T_SIMD, T_SIMDMASK>(x - v.x, y - v.y); }
		Vector2<T, T_SIMD, T_SIMDMASK>& operator-=(const Vector2<T, T_SIMD, T_SIMDMASK>& v) {
			x -= v.x;
			y -= v.y;
			return *this;
		}
		Vector2<T, T_SIMD, T_SIMDMASK> operator*(T val) { return Vector2<T, T_SIMD, T_SIMDMASK>(x * val, y * val); }

		Vector2<T, T_SIMD, T_SIMDMASK> operator+() const { return Vector2<T, T_SIMD, T_SIMDMASK>(x, y); }
		Vector2<T, T_SIMD, T_SIMDMASK> operator-() const { return Vector2<T, T_SIMD, T_SIMDMASK>(-x, -y); }

		Vector2<T, T_SIMD, T_SIMDMASK> operator*(T val) const { return Vector2<T, T_SIMD, T_SIMDMASK>(x * val, y * val); }
		Vector2<T, T_SIMD, T_SIMDMASK>& operator *=(T val) {
			x *= val;
			y *= val;
			return *this;
		}
		Vector2<T, T_SIMD, T_SIMDMASK> operator/(T val) const {
			ASSERT(val != 0);
			float inv = (float)1 / val;
			return Vector2<T, T_SIMD, T_SIMDMASK>(x * inv, y * inv);
		}
		Vector2<T, T_SIMD, T_SIMDMASK>& operator /=(T val) {
			ASSERT(val != 0);
			float inv = (float)1 / val;
			x *= inv;
			y *= inv;
			return *this;
		}

		bool operator==(const Vector2<T, T_SIMD, T_SIMDMASK>& v) { return x == v.x && y == v.y; }
		bool operator!=(const Vector2<T, T_SIMD, T_SIMDMASK>& v) { return !(*this == v); }

		float length() const { return sqrtf(x* x + y * y); }
		float lengthSq() const { return x* x + y * y; }

		Vector2<T, T_SIMD, T_SIMDMASK>& normalize() { *this /= length(); return *this; }

		T min() const { return std::min(x, y); }
		T max() const { return std::max(x, y); }
		int minDim() const { return (x < y) ? 0 : 1; }
		int maxDim() const { return (x > y) ? 0 : 1; }

		bool hasNan() { return std::isnan(x) || std::isnan(y); }
		bool isZero() { return x == 0 && y == 0; }

		union { T x, r, u; };
		union { T y, g, v; };
		float padding;
		float padding2;
	};
	using Vector2f = Vector2<float, simdpp::float32x4, simdpp::float32x4>;
	using Vector2i = Vector2<int, simdpp::int32x4, simdpp::mask_int32x4>;

	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector2<T, T_SIMD, T_SIMDMASK> operator*(float val, const Vector2<T, T_SIMD, T_SIMDMASK>& v) { return v * val; }

	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector2<T, T_SIMD, T_SIMDMASK> normalize(const Vector2<T, T_SIMD, T_SIMDMASK>& v) { return v / v.length(); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector2<T, T_SIMD, T_SIMDMASK> abs(const Vector2<T, T_SIMD, T_SIMDMASK>& v) { return Vector2(std::abs(v.x), std::abs(v.y)); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> T dot(const Vector2<T, T_SIMD, T_SIMDMASK>& v1, const Vector2<T, T_SIMD, T_SIMDMASK>& v2) { return (v1.x * v2.x) + (v1.y * v2.y); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector2<T, T_SIMD, T_SIMDMASK> permute(const Vector2<T, T_SIMD, T_SIMDMASK>& v, int x, int y) { return Vector2<T, T_SIMD, T_SIMDMASK>(v[x], v[y]); }

	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector2<T, T_SIMD, T_SIMDMASK> min(const Vector2<T, T_SIMD, T_SIMDMASK>& v1, const Vector2<T, T_SIMD, T_SIMDMASK>& v2) { return Vector2<T, T_SIMD, T_SIMDMASK>(std::min(v1.x, v2.x), std::min(v1.y, v2.y)); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector2<T, T_SIMD, T_SIMDMASK> min(const Vector2<T, T_SIMD, T_SIMDMASK>& v1, const Vector2<T, T_SIMD, T_SIMDMASK>& v2, const Vector2<T, T_SIMD, T_SIMDMASK>& v3) { return min(v1, v2); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector2<T, T_SIMD, T_SIMDMASK> max(const Vector2<T, T_SIMD, T_SIMDMASK>& v1, const Vector2<T, T_SIMD, T_SIMDMASK>& v2) { return Vector2<T, T_SIMD, T_SIMDMASK>(std::max(v1.x, v2.x), std::max(v1.y, v2.y)); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector2<T, T_SIMD, T_SIMDMASK> max(const Vector2<T, T_SIMD, T_SIMDMASK>& v1, const Vector2<T, T_SIMD, T_SIMDMASK>& v2, const Vector2<T, T_SIMD, T_SIMDMASK>& v3) { return max(v1, v2); }

	template <typename T, typename T_SIMD, typename T_SIMDMASK> bool inRange(const Vector2<T, T_SIMD, T_SIMDMASK>& v, T minVal, T maxVal) {
		return inRange(v.x, minVal, maxVal) && inRange(v.y, minVal, maxVal);
	}
	template <typename T, typename T_SIMD, typename T_SIMDMASK> bool inRange01(const Vector2<T, T_SIMD, T_SIMDMASK>& v) { return inRange(v, 0, 1); }

	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector2<T, T_SIMD, T_SIMDMASK> clamp(const Vector2<T, T_SIMD, T_SIMDMASK>& v, T minVal, T maxVal) {
		return Vector2<T, T_SIMD, T_SIMDMASK>(clamp(v.x, minVal, maxVal), clamp(v.y, minVal, maxVal));
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
			auto resVal = simdpp::add(val1, val2);

			Vector3 resVec;
			simdpp::store_u(&resVec, resVal);
			return std::move(resVec);
			//return Vector3<T, T_SIMD, T_SIMDMASK>(x + v.x, y + v.y, z + v.z);
		}
		Vector3<T, T_SIMD, T_SIMDMASK>& operator+=(const Vector3<T, T_SIMD, T_SIMDMASK>& v) {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&v);
			auto resVal = simdpp::add(val1, val2);
			simdpp::store_u(this, resVal);

			//x += v.x;
			//y += v.y;
			//z += v.z;
			return *this;
		}
		Vector3<T, T_SIMD, T_SIMDMASK> operator-(const Vector3<T, T_SIMD, T_SIMDMASK>& v) const {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&v);
			auto resVal = simdpp::sub(val1, val2);
			Vector3 resVec;
			simdpp::store_u(&resVec, resVal);
			return std::move(resVec);
			//return Vector3<T, T_SIMD, T_SIMDMASK>(x - v.x, y - v.y, z - v.z);
		}
		Vector3<T, T_SIMD, T_SIMDMASK>& operator-=(const Vector3<T, T_SIMD, T_SIMDMASK>& v) {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&v);
			auto resVal = simdpp::sub(val1, val2);
			simdpp::store_u(this, resVal);

			//x -= v.x;
			//y -= v.y;
			//z -= v.z;
			return *this;
		}
		Vector3<T, T_SIMD, T_SIMDMASK> operator*(T val) { return Vector3<T, T_SIMD, T_SIMDMASK>(x * val, y * val, z * val); }

		Vector3<T, T_SIMD, T_SIMDMASK> operator+() const { return Vector3<T, T_SIMD, T_SIMDMASK>(x, y, z); }
		Vector3<T, T_SIMD, T_SIMDMASK> operator-() const { return Vector3<T, T_SIMD, T_SIMDMASK>(-x, -y, -z); }

		Vector3<T, T_SIMD, T_SIMDMASK> operator*(T val) const { return Vector3<T, T_SIMD, T_SIMDMASK>(x * val, y * val, z * val); }
		Vector3<T, T_SIMD, T_SIMDMASK>& operator *=(T val) {
			x *= val;
			y *= val;
			z *= val;
			return *this;
		}
		Vector3<T, T_SIMD, T_SIMDMASK> operator/(T val) const {
			ASSERT(val != 0);
			float inv = (float)1 / val;
			return Vector3<T, T_SIMD, T_SIMDMASK>(x * inv, y * inv, z * inv);
		}
		Vector3<T, T_SIMD, T_SIMDMASK>& operator /=(T val) {
			ASSERT(val != 0);
			float inv = (float)1 / val;
			x *= inv;
			y *= inv;
			z *= inv;
			return *this;
		}

		bool operator==(const Vector3<T, T_SIMD, T_SIMDMASK>& v) {
			//auto val1 = simdpp::load_u<T_SIMD>(this);
			//auto val2 = simdpp::load_u<T_SIMD>(&v);
			//return simdpp::cmp_eq(val1, val2);
			return x == v.x && y == v.y && z == v.z;
		}
		bool operator!=(const Vector3<T, T_SIMD, T_SIMDMASK>& v) { return !(*this == v); }

		float length() const {
			auto val = simdpp::load_u<T_SIMD>(this);
			return sqrtf(simdpp::reduce_add(simdpp::mul(val, val)));
			//return sqrtf(x * x + y * y + z * z);
		}
		float lengthSq() const {
			auto val = simdpp::load_u<T_SIMD>(this);
			return simdpp::reduce_add(simdpp::mul(val, val));
			//return x * x + y * y + z * z;
		}

		Vector3<T, T_SIMD, T_SIMDMASK>& normalize() { *this /= length(); return *this; }

		T min() const {
			auto val = simdpp::load_u<T_SIMD>(this);
			return simdpp::reduce_min(val);
			//return std::min(x, y, z);
		}
		T max() const {
			auto val = simdpp::load_u<T_SIMD>(this);
			return simdpp::reduce_max(val);
			//return std::max(x, y, z);
		}
		int minDim() const { return (x < y && x < z) ? 0 : ((y < z) ? 1 : 2); }
		int maxDim() const { return (x > y && x > z) ? 0 : ((y > z) ? 1 : 2); }

		bool hasNan() { return std::isnan(x) || std::isnan(y) || std::isnan(z); }
		bool isZero() { return x == 0 && y == 0 && z == 0; }

		union { T x, r; };
		union { T y, g; };
		union { T z, b; };
		float padding = 0;

	};
	using Vector3f = Vector3<float, simdpp::float32x4, simdpp::mask_float32x4>;
	using Vector3i = Vector3<int, simdpp::int32x4, simdpp::mask_int32x4>;

	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector3<T, T_SIMD, T_SIMDMASK> operator*(float val, const Vector3<T, T_SIMD, T_SIMDMASK>& v) { return v * val; }

	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector3<T, T_SIMD, T_SIMDMASK> normalize(const Vector3<T, T_SIMD, T_SIMDMASK>& v) { return v / v.length(); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector3<T, T_SIMD, T_SIMDMASK> abs(const Vector3<T, T_SIMD, T_SIMDMASK>& v) { return Vector3(std::abs(v.x), std::abs(v.y), std::abs(v.z)); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> T dot(const Vector3<T, T_SIMD, T_SIMDMASK>& v1, const Vector3<T, T_SIMD, T_SIMDMASK>& v2) {
		auto val1 = simdpp::load_u<T_SIMD>(&v1);
		auto val2 = simdpp::load_u<T_SIMD>(&v2);

		return simdpp::reduce_add(simdpp::mul(val1, val2));

		//return (v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z);
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
		//return Vector3<T, T_SIMD, T_SIMDMASK>(std::min(v1.x, v2.x), std::min(v1.y, v2.y), std::min(v1.z, v2.z));
	}
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector3<T, T_SIMD, T_SIMDMASK> min(const Vector3<T, T_SIMD, T_SIMDMASK>& v1, const Vector3<T, T_SIMD, T_SIMDMASK>& v2, const Vector3<T, T_SIMD, T_SIMDMASK>& v3) {return min(v1, min(v2, v3)); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector3<T, T_SIMD, T_SIMDMASK> max(const Vector3<T, T_SIMD, T_SIMDMASK>& v1, const Vector3<T, T_SIMD, T_SIMDMASK>& v2) {
		T_SIMD val1 = simdpp::load_u<T_SIMD>(&v1);
		T_SIMD val2 = simdpp::load_u<T_SIMD>(&v2);
		Vector3<T, T_SIMD, T_SIMDMASK> res;
		simdpp::store_u(&res, simdpp::max(val1, val2));
		return std::move(res);
		//return Vector3<T, T_SIMD, T_SIMDMASK>(std::max(v1.x, v2.x), std::max(v1.y, v2.y), std::max(v1.z, v2.z));
	}
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector3<T, T_SIMD, T_SIMDMASK> max(const Vector3<T, T_SIMD, T_SIMDMASK>& v1, const Vector3<T, T_SIMD, T_SIMDMASK>& v2, const Vector3<T, T_SIMD, T_SIMDMASK>& v3) { return max(v1, max(v2, v3)); }

	template <typename T, typename T_SIMD, typename T_SIMDMASK> bool inRange(const Vector3<T, T_SIMD, T_SIMDMASK>& v, T minVal, T maxVal) {
		T_SIMD val = simdpp::load_u<T_SIMD>(&v);
		T_SIMDMASK mask = simdpp::cmp_ge(val, simdpp::load_splat<T_SIMD>(&minVal));
		mask = mask | simdpp::cmp_le(val, simdpp::load_splat<T_SIMD>(&maxVal));
		mask = mask | simdpp::to_mask( simdpp::make_float<T_SIMD>(0,0,0,1) );
		return simdpp::reduce_and(simdpp::bit_cast<simdpp::int32<4>, T_SIMDMASK>(mask));

		//return inRange(v.x, minVal, maxVal) && inRange(v.y, minVal, maxVal) && inRange(v.z, minVal, maxVal);
	}
	template <typename T, typename T_SIMD, typename T_SIMDMASK> bool inRange01(const Vector3<T, T_SIMD, T_SIMDMASK>& v) { return inRange(v, (T)0, (T)1); }

	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector3<T, T_SIMD, T_SIMDMASK> clamp(const Vector3<T, T_SIMD, T_SIMDMASK>& v, T minVal, T maxVal) {
		return Vector3<T, T_SIMD, T_SIMDMASK>(clamp(v.x, minVal, maxVal), clamp(v.y, minVal, maxVal), clamp(v.z, minVal, maxVal));
	}
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector3<T, T_SIMD, T_SIMDMASK> clamp01(const Vector3<T, T_SIMD, T_SIMDMASK>& v) { return clamp(v, 0, 1); }

}