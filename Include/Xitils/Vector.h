#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <optional>

#include "Utils.h"

namespace Xitils {

	template <typename T> class Vector2 {
	public:

		Vector2() : x(0), y(0) {}
		explicit Vector2(T val) : x(val), y(val) {}
		Vector2(T x, T y) : x(x), y(y) { ASSERT(!hasNan()); }

		explicit Vector2(const glm::tvec2<T> v) : x(v.x), y(v.y) { ASSERT(!hasNan()); }
		glm::tvec2<T> glm() const { return glm::tvec2<T>(x, y); }

		template <typename U> explicit Vector2(const Vector2<U>& v) : x(v.x), y(v.y) { ASSERT(!hasNan()); }

		T& operator=(const T& v) { x = v.x; y = v.y; }
		T& operator=(const T&& v) noexcept { x = v.x; y = v.y; }

		T& operator[](int i) const {
			ASSERT(i >= 0 && i <= 1);
			if (i == 0) { return x; }
			return y;
		}

		Vector2<T> operator+(const Vector2<T>& v) const { return Vector2<T>(x + v.x, y + v.y); }
		Vector2<T>& operator+=(const Vector2<T>& v) {
			x += v.x;
			y += v.y;
			return *this;
		}
		Vector2<T> operator-(const Vector2<T>& v) const { return Vector2<T>(x - v.x, y - v.y); }
		Vector2<T>& operator-=(const Vector2<T>& v) {
			x -= v.x;
			y -= v.y;
			return *this;
		}
		Vector2<T> operator*(T val) { return Vector2<T>(x * val, y * val); }

		Vector2<T> operator+() const { return Vector2<T>(x, y); }
		Vector2<T> operator-() const { return Vector2<T>(-x, -y); }

		Vector2<T> operator*(T val) const { return Vector2<T>(x * val, y * val); }
		Vector2<T>& operator *=(T val) {
			x *= val;
			y *= val;
			return *this;
		}
		Vector2<T> operator/(T val) const {
			ASSERT(val != 0);
			float inv = (float)1 / val;
			return Vector2<T>(x * inv, y * inv);
		}
		Vector2<T>& operator /=(T val) {
			ASSERT(val != 0);
			float inv = (float)1 / val;
			x *= inv;
			y *= inv;
			return *this;
		}

		bool operator==(const Vector2<T>& v) { return x == v.x && y == v.y; }
		bool operator!=(const Vector2<T>& v) { return !(*this == v); }

		float length() const { return sqrtf(x* x + y * y); }
		float lengthSq() const { return x* x + y * y; }

		Vector2<T>& normalize() { *this /= length(); return *this; }

		T min() const { return std::min(x, y); }
		T max() const { return std::max(x, y); }
		int minDim() const { return (x < y) ? 0 : 1; }
		int maxDim() const { return (x > y) ? 0 : 1; }

		bool hasNan() { return std::isnan(x) || std::isnan(y); }
		bool isZero() { return x == 0 && y == 0; }

		union { T x, r, u; };
		union { T y, g, v; };

	};

	template <typename T> Vector2<T> operator*(float val, const Vector2<T>& v) { return v * val; }

	template <typename T> Vector2<T> normalize(const Vector2<T>& v) { return v / v.length(); }
	template <typename T> Vector2<T> abs(const Vector2<T>& v) { return Vector2(std::abs(v.x), std::abs(v.y)); }
	template <typename T> T dot(const Vector2<T>& v1, const Vector2<T>& v2) { return (v1.x * v2.x) + (v1.y * v2.y); }
	template <typename T> Vector2<T> permute(const Vector2<T>& v, int x, int y) { return Vector2<T>(v[x], v[y]); }

	template <typename T> Vector2<T> min(const Vector2<T>& v1, const Vector2<T>& v2) { return Vector2<T>(std::min(v1.x, v2.x), std::min(v1.y, v2.y)); }
	template <typename T> Vector2<T> min(const Vector2<T>& v1, const Vector2<T>& v2, const Vector2<T>& v3) { return min(v1, v2); }
	template <typename T> Vector2<T> max(const Vector2<T>& v1, const Vector2<T>& v2) { return Vector2<T>(std::max(v1.x, v2.x), std::max(v1.y, v2.y)); }
	template <typename T> Vector2<T> max(const Vector2<T>& v1, const Vector2<T>& v2, const Vector2<T>& v3) { return max(v1, v2); }

	template <typename T> bool inRange(const Vector2<T>& v, T minVal, T maxVal) {
		return inRange(v.x, minVal, maxVal) && inRange(v.y, minVal, maxVal);
	}
	template <typename T> bool inRange01(const Vector2<T>& v) { return inRange(v, 0, 1); }

	template <typename T> Vector2<T> clamp(const Vector2<T>& v, T minVal, T maxVal) {
		return Vector2<T>(clamp(v.x, minVal, maxVal), clamp(v.y, minVal, maxVal));
	}
	template <typename T> Vector2<T> clamp01(const Vector2<T>& v) { return clamp(v, 0, 1); }


	using Vector2f = Vector2<float>;
	using Vector2i = Vector2<int>;



	template <typename T> class Vector3 {
	public:
		Vector3() : x(0), y(0), z(0) {}
		explicit Vector3(T val) : x(val), y(val), z(val) {}
		Vector3(T x, T y, T z) : x(x), y(y), z(z) { ASSERT(!hasNan()); }

		explicit Vector3(const glm::tvec3<T> v) : x(v.x), y(v.y), z(v.z) { ASSERT(!hasNan()); }
		glm::tvec3<T> glm() const { return glm::tvec3<T>(x, y, z); }

		template <typename U> explicit Vector3(const Vector3<U>& v) : x(v.x), y(v.y), z(v.z) { ASSERT(!hasNan()); }

		T& operator=(const T& v) { x = v.x; y = v.y; z = v.z; }
		T& operator=(const T&& v) noexcept { x = v.x; y = v.y; z = v.z; }

		T& operator[](int i) {
			ASSERT(i >= 0 && i <= 2);
			if (i == 0) { return x; }
			if (i == 1) { return y; }
			return z;
		}

		Vector3<T> operator+(const Vector3<T>& v) const { return Vector3<T>(x + v.x, y + v.y, z + v.z); }
		Vector3<T>& operator+=(const Vector3<T>& v) {
			x += v.x;
			y += v.y;
			z += v.z;
			return *this;
		}
		Vector3<T> operator-(const Vector3<T>& v) const { return Vector3<T>(x - v.x, y - v.y, z - v.z); }
		Vector3<T>& operator-=(const Vector3<T>& v) {
			x -= v.x;
			y -= v.y;
			z -= v.z;
			return *this;
		}
		Vector3<T> operator*(T val) { return Vector3<T>(x * val, y * val, z * val); }

		Vector3<T> operator+() const { return Vector3<T>(x, y, z); }
		Vector3<T> operator-() const { return Vector3<T>(-x, -y, -z); }

		Vector3<T> operator*(T val) const { return Vector3<T>(x * val, y * val, z * val); }
		Vector3<T>& operator *=(T val) {
			x *= val;
			y *= val;
			z *= val;
			return *this;
		}
		Vector3<T> operator/(T val) const {
			ASSERT(val != 0);
			float inv = (float)1 / val;
			return Vector3<T>(x * inv, y * inv, z * inv);
		}
		Vector3<T>& operator /=(T val) {
			ASSERT(val != 0);
			float inv = (float)1 / val;
			x *= inv;
			y *= inv;
			z *= inv;
			return *this;
		}

		bool operator==(const Vector3<T>& v) { return x == v.x && y == v.y && z == v.z; }
		bool operator!=(const Vector3<T>& v) { return !(*this == v); }

		float length() const { return sqrtf(x * x + y * y + z * z); }
		float lengthSq() const { return x * x + y * y + z * z; }

		Vector3<T>& normalize() { *this /= length(); return *this; }

		T min() const { return std::min(x, y, z); }
		T max() const { return std::max(x, y, z); }
		int minDim() const { return (x < y && x < z) ? 0 : ((y < z) ? 1 : 2); }
		int maxDim() const { return (x > y && x > z) ? 0 : ((y > z) ? 1 : 2); }

		bool hasNan() { return std::isnan(x) || std::isnan(y) || std::isnan(z); }
		bool isZero() { return x == 0 && y == 0 && z == 0; }

		union { T x, r; };
		union { T y, g; };
		union { T z, b; };

	};

	template <typename T> Vector3<T> operator*(float val, const Vector3<T>& v) { return v * val; }

	template <typename T> Vector3<T> normalize(const Vector3<T>& v) { return v / v.length(); }
	template <typename T> Vector3<T> abs(const Vector3<T>& v) { return Vector3(std::abs(v.x), std::abs(v.y), std::abs(v.z)); }
	template <typename T> T dot(const Vector3<T>& v1, const Vector3<T>& v2) { return (v1.x * v2.x) + (v1.y * v2.y) + (v1.z * v2.z); }
	template <typename T> Vector3<T> cross(const Vector3<T>& v1, const Vector3<T>& v2) {
		double v1x = v1.x, v1y = v1.y, v1z = v1.z;
		double v2x = v2.x, v2y = v2.y, v2z = v2.z;
		return Vector3<T>(
			(v1y * v2z) - (v1z * v2y),
			(v1z * v2x) - (v1x * v2z),
			(v1x * v2y) - (v1y * v2x));
	}
	template <typename T> Vector3<T> permute(const Vector3<T>& v, int x, int y, int z) { return Vector3<T>(v[x], v[y], v[z]); }

	template <typename T> Vector3<T> min(const Vector3<T>& v1, const Vector3<T>& v2) { return Vector3<T>(std::min(v1.x, v2.x), std::min(v1.y, v2.y), std::min(v1.z, v2.z)); }
	template <typename T> Vector3<T> min(const Vector3<T>& v1, const Vector3<T>& v2, const Vector3<T>& v3) { return min(v1, min(v2, v3)); }
	template <typename T> Vector3<T> max(const Vector3<T>& v1, const Vector3<T>& v2) { return Vector3<T>(std::max(v1.x, v2.x), std::max(v1.y, v2.y), std::max(v1.z, v2.z)); }
	template <typename T> Vector3<T> max(const Vector3<T>& v1, const Vector3<T>& v2, const Vector3<T>& v3) { return max(v1, max(v2, v3)); }

	template <typename T> bool inRange(const Vector3<T>& v, T minVal, T maxVal) {
		return inRange(v.x, minVal, maxVal) && inRange(v.y, minVal, maxVal) && inRange(v.z, minVal, maxVal);
	}
	template <typename T> bool inRange01(const Vector3<T>& v) { return inRange(v, (T)0, (T)1); }

	template <typename T> Vector3<T> clamp(const Vector3<T>& v, T minVal, T maxVal) {
		return Vector3<T>(clamp(v.x, minVal, maxVal), clamp(v.y, minVal, maxVal), clamp(v.z, minVal, maxVal));
	}
	template <typename T> Vector3<T> clamp01(const Vector3<T>& v) { return clamp(v, 0, 1); }


	using Vector3f = Vector3<float>;
	using Vector3i = Vector3<int>;

}