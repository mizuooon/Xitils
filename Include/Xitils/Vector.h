#pragma once

#include "Utils.h"
#include "Sampler.h"

namespace Xitils {

	template <typename T, typename T_SIMD, typename T_SIMDMASK> class Vector2;
	template <typename T, typename T_SIMD, typename T_SIMDMASK> class Vector3;
	template <typename T, typename T_SIMD, typename T_SIMDMASK> class Vector4;

	template<typename T_SIMD, typename T_SIMDMASK> bool _hasNan(const Vector2<float, T_SIMD, T_SIMDMASK>& v) {
		return std::isnan(v.x) || std::isnan(v.y);

		//auto val = simdpp::load_u<T_SIMD>(&v);
		//auto mask = simdpp::isnan(val);
		//return simdpp::reduce_or(simdpp::bit_cast<simdpp::uint32x4, T_SIMDMASK>(mask)) != 0;
	}

	template<typename T_SIMD, typename T_SIMDMASK> bool _hasNan(const Vector2<int, T_SIMD, T_SIMDMASK>& v) {
		return false;
	}

	template<typename T_SIMD, typename T_SIMDMASK> bool _hasNan(const Vector3<float, T_SIMD, T_SIMDMASK>& v) {
		return std::isnan(v.x) || std::isnan(v.y) || std::isnan(v.z);

		//auto val = simdpp::load_u<T_SIMD>(&v);
		//auto mask = simdpp::isnan(val);
		//return simdpp::reduce_or(simdpp::bit_cast<simdpp::uint32x4, T_SIMDMASK>(mask)) != 0;
	}

	template<typename T_SIMD, typename T_SIMDMASK> bool _hasNan(const Vector3<int, T_SIMD, T_SIMDMASK>& v) {
		return false;
	}

	template<typename T_SIMD, typename T_SIMDMASK> bool _hasNan(const Vector4<float, T_SIMD, T_SIMDMASK>& v) {
		return std::isnan(v.x) || std::isnan(v.y) || std::isnan(v.z) || std::isnan(v.w);

		//auto val = simdpp::load_u<T_SIMD>(&v);
		//auto mask = simdpp::isnan(val);
		//return simdpp::reduce_or(simdpp::bit_cast<simdpp::uint32x4, T_SIMDMASK>(mask)) != 0;
	}

	template<typename T_SIMD, typename T_SIMDMASK> bool _hasNan(const Vector4<int, T_SIMD, T_SIMDMASK>& v) {
		return false;
	}

	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------

	template <typename T, typename T_SIMD, typename T_SIMDMASK> class Vector2 {
	public:

		using _V = Vector2<T, T_SIMD, T_SIMDMASK>;
		using _T = T;
		using _T_SIMD = T_SIMD;
		using _T_SIMDMASK = T_SIMDMASK;

		Vector2() : x(0), y(0) {}
		explicit Vector2(T val) : x(val), y(val) {}
		Vector2(T x, T y) : x(x), y(y) { ASSERT(!hasNan()); }

		explicit Vector2(const glm::tvec2<T> v) : x(v.x), y(v.y) { ASSERT(!hasNan()); }
		glm::tvec2<T> glm() const { return glm::tvec2<T>(x, y); }

		template <typename U, typename U_SIMD, typename U_SIMDMASK> explicit Vector2(const Vector2<U, U_SIMD, U_SIMDMASK>& v) : x(v.x), y(v.y) { ASSERT(!hasNan()); }

		_V& operator=(const _V& v) {
			x = v.x; y = v.y;
			ASSERT(!hasNan());
			return *this;
		}

		const T& operator[](int i) const {
			ASSERT(i >= 0 && i <= 1);
			if (i == 0) { return x; }
			return y;
		}

		T& operator[](int i) {
			ASSERT(i >= 0 && i <= 1);
			if (i == 0) { return x; }
			return y;
		}

		T* data() const { return &x; }

		_V operator+(const _V& v) const {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&v);
			_V res;
			simdpp::store_u(&res, simdpp::add(val1, val2));
			return std::move(res);
		}
		_V& operator+=(const _V& v) {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&v);
			simdpp::store_u(this, simdpp::add(val1, val2));
			return *this;
		}
		_V operator-(const _V& v) const {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&v);
			_V res;
			simdpp::store_u(&res, simdpp::sub(val1, val2));
			return std::move(res);
		}
		_V& operator-=(const _V& v) {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&v);
			simdpp::store_u(this, simdpp::sub(val1, val2));
			return *this;
		}

		_V operator+() const { return _V(x, y); }
		_V operator-() const { return _V(-x, -y); }

		_V operator*(const _V& val) const {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&val);
			_V res;
			simdpp::store_u(&res, simdpp::mul(val1, val2));
			return std::move(res);
		}
		_V& operator *=(const _V& val) {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&val);
			simdpp::store_u(this, simdpp::mul(val1, val2));
			return *this;
		}
		_V operator*(T val) const {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_splat<T_SIMD>(&val);
			_V res;
			simdpp::store_u(&res, simdpp::mul(val1, val2));
			return std::move(res);
		}
		_V& operator *=(T val) {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_splat<T_SIMD>(&val);
			simdpp::store_u(this, simdpp::mul(val1, val2));
			return *this;
		}
		_V operator/(const _V& val) const {
			ASSERT(val.x != 0.0f && val.y != 0.0f);
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&val);
			_V res;
			simdpp::store_u(&res, simdpp::div(val1, val2));
			return std::move(res);
		}
		_V& operator /=(const _V& val) {
			ASSERT(val.x != 0.0f && val.y != 0.0f);
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&val);
			simdpp::store_u(this, simdpp::div(val1, val2));
			return *this;
		}
		_V operator/(T val) const {
			ASSERT(val != 0);
			float inv = (float)1 / val;
			return *this * inv;
		}
		_V& operator /=(T val) {
			ASSERT(val != 0);
			float inv = (float)1 / val;
			*this *= inv;
			return *this;
		}

		bool operator==(const _V& v) const {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&v);
			auto mask = simdpp::cmp_eq(val1, val2);
			return simdpp::reduce_and(simdpp::bit_cast<simdpp::uint32x4, T_SIMDMASK>(mask)) != 0;
		}
		bool operator!=(const _V& v) const {
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

		_V& normalize() { *this /= length(); return *this; }

		T min() const { return std::min(x, y); }
		T max() const { return std::max(x, y); }
		int minDim() const { return (x < y) ? 0 : 1; }
		int maxDimension() const { return (x > y) ? 0 : 1; }

		bool hasNan() const { return _hasNan(*this); }
		bool isZero() const {
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

	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------

	template <typename T, typename T_SIMD, typename T_SIMDMASK> class Vector3 {
	public:

		using _V = Vector3<T, T_SIMD, T_SIMDMASK>;
		using _T = T;
		using _T_SIMD = T_SIMD;
		using _T_SIMDMASK = T_SIMDMASK;

		Vector3() : x(0), y(0), z(0) {}
		explicit Vector3(T val) : x(val), y(val), z(val) {}
		Vector3(T x, T y, T z) : x(x), y(y), z(z) { ASSERT(!hasNan()); }

		explicit Vector3(const glm::tvec3<T> v) : x(v.x), y(v.y), z(v.z) { ASSERT(!hasNan()); }
		glm::tvec3<T> glm() const { return glm::tvec3<T>(x, y, z); }

		template <typename U, typename U_SIMD, typename U_SIMDMASK> explicit Vector3(const Vector3<U, U_SIMD, U_SIMDMASK>& v) : x(v.x), y(v.y), z(v.z) { ASSERT(!hasNan()); }
		
		_V& operator=(const _V& v) {
			x = v.x; y = v.y; z = v.z;
			ASSERT(!hasNan());
			return *this;
		}

		const T& operator[](int i) const {
			ASSERT(i >= 0 && i <= 2);
			if (i == 0) { return x; }
			if (i == 1) { return y; }
			return z;
		}

		T& operator[](int i) {
			ASSERT(i >= 0 && i <= 2);
			if (i == 0) { return x; }
			if (i == 1) { return y; }
			return z;
		}

		T* data() const { return &x; }

		_V operator+(const _V& v) const {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&v);
			_V res;
			simdpp::store_u(&res, simdpp::add(val1, val2));
			return std::move(res);
		}
		_V& operator+=(const _V& v) {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&v);
			simdpp::store_u(this, simdpp::add(val1, val2));
			return *this;
		}
		_V operator-(const _V& v) const {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&v);
			_V res;
			simdpp::store_u(&res, simdpp::sub(val1, val2));
			return std::move(res);
		}
		_V& operator-=(const _V& v) {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&v);
			simdpp::store_u(this, simdpp::sub(val1, val2));
			return *this;
		}

		_V operator+() const { return _V(x, y, z); }
		_V operator-() const { return _V(-x, -y, -z); }

		_V operator*(const _V& val) const {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&val);
			_V res;
			simdpp::store_u(&res, simdpp::mul(val1, val2));
			return std::move(res);
		}
		_V& operator *=(const _V& val) {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&val);
			simdpp::store_u(this, simdpp::mul(val1, val2));
			return *this;
		}
		_V operator*(T val) const {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_splat<T_SIMD>(&val);
			_V res;
			simdpp::store_u(&res, simdpp::mul(val1, val2));
			return std::move(res);
		}
		_V& operator *=(T val) {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_splat<T_SIMD>(&val);
			simdpp::store_u(this, simdpp::mul(val1, val2));
			return *this;
		}
		_V operator/(const _V& val) const {
			ASSERT(val.x != 0.0f && val.y != 0.0f && val.z != 0.0f);
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&val);
			_V res;
			simdpp::store_u(&res, simdpp::div(val1, val2));
			return std::move(res);
		}
		_V& operator /=(const _V& val) {
			ASSERT(val.x != 0.0f && val.y != 0.0f && val.z != 0.0f);
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&val);
			simdpp::store_u(this, simdpp::div(val1, val2));
			return *this;
		}
		_V operator/(T val) const {
			ASSERT(val != 0);
			float inv = (float)1 / val;
			return *this * inv;
		}
		_V& operator /=(T val) {
			ASSERT(val != 0);
			float inv = (float)1 / val;
			*this *= inv;
			return *this;
		}

		bool operator==(const _V& v) const {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&v);
			auto mask = simdpp::cmp_eq(val1, val2);
			return simdpp::reduce_and(simdpp::bit_cast<simdpp::uint32x4, T_SIMDMASK>(mask)) != 0;
		}
		bool operator!=(const _V& v) const {
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

		_V& normalize() { *this /= length(); return *this; }

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
		int maxDimension() const { return (x > y && x > z) ? 0 : ((y > z) ? 1 : 2); }

		bool hasNan() const { return _hasNan(*this); }
		bool isZero() const {
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


	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------

	template <typename T, typename T_SIMD, typename T_SIMDMASK> class Vector4 {
	public:

		using _V = Vector4<T, T_SIMD, T_SIMDMASK>;
		using _T = T;
		using _T_SIMD = T_SIMD;
		using _T_SIMDMASK = T_SIMDMASK;

		Vector4() : x(0), y(0), z(0), w(0) {}
		explicit Vector4(T val) : x(val), y(val), z(val), w(val) {}
		Vector4(T x, T y, T z, T w) : x(x), y(y), z(z), w(w) { ASSERT(!hasNan()); }

		explicit Vector4(const glm::tvec4<T> v) : x(v.x), y(v.y), z(v.z), w(v.w) { ASSERT(!hasNan()); }
		glm::tvec4<T> glm() const { return glm::tvec4<T>(x, y, z, w); }

		template <typename U, typename U_SIMD, typename U_SIMDMASK> explicit Vector4(const Vector4<U, U_SIMD, U_SIMDMASK>& v) : x(v.x), y(v.y), z(v.z), w(v.w) { ASSERT(!hasNan()); }

		_V& operator=(const _V& v) {
			x = v.x; y = v.y; z = v.z; w = v.w;
			ASSERT(!hasNan());
			return *this;
		}

		const T& operator[](int i) const {
			ASSERT(i >= 0 && i <= 3);
			if (i == 0) { return x; }
			if (i == 1) { return y; }
			if (i == 2) { return z; }
			return w;
		}

		T& operator[](int i) {
			ASSERT(i >= 0 && i <= 3);
			if (i == 0) { return x; }
			if (i == 1) { return y; }
			if (i == 2) { return z; }
			return w;
		}

		_V operator+(const _V& v) const {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&v);
			_V res;
			simdpp::store_u(&res, simdpp::add(val1, val2));
			return std::move(res);
		}
		_V& operator+=(const _V& v) {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&v);
			simdpp::store_u(this, simdpp::add(val1, val2));
			return *this;
		}
		_V operator-(const _V& v) const {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&v);
			_V res;
			simdpp::store_u(&res, simdpp::sub(val1, val2));
			return std::move(res);
		}
		_V& operator-=(const _V& v) {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&v);
			simdpp::store_u(this, simdpp::sub(val1, val2));
			return *this;
		}

		_V operator+() const { return _V(x, y, z, w); }
		_V operator-() const { return _V(-x, -y, -z, -w); }

		_V operator*(const _V& val) const {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&val);
			_V res;
			simdpp::store_u(&res, simdpp::mul(val1, val2));
			return std::move(res);
		}
		_V& operator *=(const _V& val) {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&val);
			simdpp::store_u(this, simdpp::mul(val1, val2));
			return *this;
		}
		_V operator*(T val) const {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_splat<T_SIMD>(&val);
			_V res;
			simdpp::store_u(&res, simdpp::mul(val1, val2));
			return std::move(res);
		}
		_V& operator *=(T val) {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_splat<T_SIMD>(&val);
			simdpp::store_u(this, simdpp::mul(val1, val2));
			return *this;
		}
		_V operator/(const _V& val) const {
			ASSERT(val.x != 0.0f && val.y != 0.0f && val.z != 0.0f && val.w != 0.0f);
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&val);
			_V res;
			simdpp::store_u(&res, simdpp::div(val1, val2));
			return std::move(res);
		}
		_V& operator /=(const _V& val) {
			ASSERT(val.x != 0.0f && val.y != 0.0f && val.z != 0.0f && val.w != 0.0f);
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&val);
			simdpp::store_u(this, simdpp::div(val1, val2));
			return *this;
		}
		_V operator/(T val) const {
			ASSERT(val != 0);
			float inv = (float)1 / val;
			return *this * inv;
		}
		_V& operator /=(T val) {
			ASSERT(val != 0);
			float inv = (float)1 / val;
			*this *= inv;
			return *this;
		}

		bool operator==(const _V& v) const {
			auto val1 = simdpp::load_u<T_SIMD>(this);
			auto val2 = simdpp::load_u<T_SIMD>(&v);
			auto mask = simdpp::cmp_eq(val1, val2);
			return simdpp::reduce_and(simdpp::bit_cast<simdpp::uint32x4, T_SIMDMASK>(mask)) != 0;
		}
		bool operator!=(const _V& v) const {
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

		_V& normalize() { *this /= length(); return *this; }

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
		int maxDimension() const {
			if (x > y && x > z && x > z) { return 0; }
			if (y > z && y > w) { return 1; }
			if (z > w) { return 2; }
			return 3;
		}

		bool hasNan() const { return _hasNan(*this); }
		bool isZero() const {
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


	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------

	using Vector2f = Vector2<float, simdpp::float32x4, simdpp::mask_float32x4>;
	using Vector2i = Vector2<int, simdpp::int32x4, simdpp::mask_int32x4>;

	using Vector3f = Vector3<float, simdpp::float32x4, simdpp::mask_float32x4>;
	using Vector3i = Vector3<int, simdpp::int32x4, simdpp::mask_int32x4>;

	using Vector4f = Vector4<float, simdpp::float32x4, simdpp::mask_float32x4>;
	using Vector4i = Vector4<int, simdpp::int32x4, simdpp::mask_int32x4>;

	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------

	// min, max など一部の関数はテンプレートとして一般のクラスに対して既に定義されているが、
	// Vector 系のクラスを渡す際には以下で定義した関数群が優先的に使用される。
	// このオーバーロードを行うために Vector2/3/4 ごとに関数のインタフェースを定義しつつ
	// その実体は Vector 系クラス間で使い回せるようテンプレート化しているので
	// やや冗長な書き方になっている。


	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector2<T, T_SIMD, T_SIMDMASK> operator*(T val, const Vector2<T, T_SIMD, T_SIMDMASK>& v) { return v * val; }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector3<T, T_SIMD, T_SIMDMASK> operator*(T val, const Vector3<T, T_SIMD, T_SIMDMASK>& v) { return v * val; }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector4<T, T_SIMD, T_SIMDMASK> operator*(T val, const Vector4<T, T_SIMD, T_SIMDMASK>& v) { return v * val; }

	template <typename V> inline V _normalize(const V& v) { return v / v.length(); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector2<T, T_SIMD, T_SIMDMASK> normalize(const Vector2<T, T_SIMD, T_SIMDMASK>& v) { return _normalize(v); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector3<T, T_SIMD, T_SIMDMASK> normalize(const Vector3<T, T_SIMD, T_SIMDMASK>& v) { return _normalize(v); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector4<T, T_SIMD, T_SIMDMASK> normalize(const Vector4<T, T_SIMD, T_SIMDMASK>& v) { return _normalize(v); }

	template <typename V> inline V _abs(const V& v) {
		auto val = simdpp::load_u<V::_T_SIMD>(&v);
		V res;
		simdpp::store_u(&res, simdpp::abs(val));
		return std::move(res);
	}
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector2<T, T_SIMD, T_SIMDMASK> abs(const Vector2<T, T_SIMD, T_SIMDMASK>& v) { return _abs(v); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector3<T, T_SIMD, T_SIMDMASK> abs(const Vector3<T, T_SIMD, T_SIMDMASK>& v) { return _abs(v); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector4<T, T_SIMD, T_SIMDMASK> abs(const Vector4<T, T_SIMD, T_SIMDMASK>& v) { return _abs(v); }

	template <typename V> inline typename V::_T _dot(const V& v1, const V& v2) {
		auto val1 = simdpp::load_u<V::_T_SIMD>(&v1);
		auto val2 = simdpp::load_u<V::_T_SIMD>(&v2);
		return simdpp::reduce_add(simdpp::mul(val1, val2));
	}
	template <typename T, typename T_SIMD, typename T_SIMDMASK> T dot(const Vector2<T, T_SIMD, T_SIMDMASK>& v1, const Vector2<T, T_SIMD, T_SIMDMASK>& v2) { return _dot(v1, v2); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> T dot(const Vector3<T, T_SIMD, T_SIMDMASK>& v1, const Vector3<T, T_SIMD, T_SIMDMASK>& v2) { return _dot(v1, v2); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> T dot(const Vector4<T, T_SIMD, T_SIMDMASK>& v1, const Vector4<T, T_SIMD, T_SIMDMASK>& v2) { return _dot(v1, v2); }

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

	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector2<T, T_SIMD, T_SIMDMASK> permute(const Vector2<T, T_SIMD, T_SIMDMASK>& v, int x, int y) { return Vector2<T, T_SIMD, T_SIMDMASK>(v[x], v[y]); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector3<T, T_SIMD, T_SIMDMASK> permute(const Vector3<T, T_SIMD, T_SIMDMASK>& v, int x, int y, int z) { return Vector3<T, T_SIMD, T_SIMDMASK>(v[x], v[y], v[z]); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector4<T, T_SIMD, T_SIMDMASK> permute(const Vector4<T, T_SIMD, T_SIMDMASK>& v, int x, int y, int z, int w) { return Vector4<T, T_SIMD, T_SIMDMASK>(v[x], v[y], v[z], v[w]); }

	template <typename V> inline V _min(const V& v1, const V& v2) {
		auto val1 = simdpp::load_u<V::_T_SIMD>(&v1);
		auto val2 = simdpp::load_u<V::_T_SIMD>(&v2);
		V res;
		simdpp::store_u(&res, simdpp::min(val1, val2));
		return std::move(res);
	}
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector2<T, T_SIMD, T_SIMDMASK> min(const Vector2<T, T_SIMD, T_SIMDMASK>& v1, const Vector2<T, T_SIMD, T_SIMDMASK>& v2) { return _min(v1, v2); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector3<T, T_SIMD, T_SIMDMASK> min(const Vector3<T, T_SIMD, T_SIMDMASK>& v1, const Vector3<T, T_SIMD, T_SIMDMASK>& v2) { return _min(v1, v2); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector4<T, T_SIMD, T_SIMDMASK> min(const Vector4<T, T_SIMD, T_SIMDMASK>& v1, const Vector4<T, T_SIMD, T_SIMDMASK>& v2) { return _min(v1, v2); }

	template <typename V> inline V _min(const V& v1, const V& v2, const V& v3) { return min(v1, min(v2, v3)); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector2<T, T_SIMD, T_SIMDMASK> min(const Vector2<T, T_SIMD, T_SIMDMASK>& v1, const Vector2<T, T_SIMD, T_SIMDMASK>& v2, const Vector2<T, T_SIMD, T_SIMDMASK>& v3) { return _min(v1, v2, v3); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector2<T, T_SIMD, T_SIMDMASK> min(const Vector2<T, T_SIMD, T_SIMDMASK>& v1, const Vector2<T, T_SIMD, T_SIMDMASK>& v2, const Vector3<T, T_SIMD, T_SIMDMASK>& v3) { return _min(v1, v2, v3); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector2<T, T_SIMD, T_SIMDMASK> min(const Vector2<T, T_SIMD, T_SIMDMASK>& v1, const Vector2<T, T_SIMD, T_SIMDMASK>& v2, const Vector4<T, T_SIMD, T_SIMDMASK>& v3) { return _min(v1, v2, v3); }

	template <typename V> inline V _max(const V& v1, const V& v2) {
		auto val1 = simdpp::load_u<V::_T_SIMD>(&v1);
		auto val2 = simdpp::load_u<V::_T_SIMD>(&v2);
		V res;
		simdpp::store_u(&res, simdpp::max(val1, val2));
		return std::move(res);
	}
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector2<T, T_SIMD, T_SIMDMASK> max(const Vector2<T, T_SIMD, T_SIMDMASK>& v1, const Vector2<T, T_SIMD, T_SIMDMASK>& v2) { return _max(v1, v2); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector3<T, T_SIMD, T_SIMDMASK> max(const Vector3<T, T_SIMD, T_SIMDMASK>& v1, const Vector3<T, T_SIMD, T_SIMDMASK>& v2) { return _max(v1, v2); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector4<T, T_SIMD, T_SIMDMASK> max(const Vector4<T, T_SIMD, T_SIMDMASK>& v1, const Vector4<T, T_SIMD, T_SIMDMASK>& v2) { return _max(v1, v2); }

	template <typename V> inline V _max(const V& v1, const V& v2, const V& v3) { return max(v1, max(v2, v3)); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector2<T, T_SIMD, T_SIMDMASK> max(const Vector2<T, T_SIMD, T_SIMDMASK>& v1, const Vector2<T, T_SIMD, T_SIMDMASK>& v2, const Vector2<T, T_SIMD, T_SIMDMASK>& v3) { return _max(v1, v2, v3); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector2<T, T_SIMD, T_SIMDMASK> max(const Vector2<T, T_SIMD, T_SIMDMASK>& v1, const Vector2<T, T_SIMD, T_SIMDMASK>& v2, const Vector3<T, T_SIMD, T_SIMDMASK>& v3) { return _max(v1, v2, v3); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector2<T, T_SIMD, T_SIMDMASK> max(const Vector2<T, T_SIMD, T_SIMDMASK>& v1, const Vector2<T, T_SIMD, T_SIMDMASK>& v2, const Vector4<T, T_SIMD, T_SIMDMASK>& v3) { return _max(v1, v2, v3); }

	template <typename V> inline bool _inRange(const V& v, typename V::_T minVal, typename V::_T maxVal) {
		V::_T_SIMD val = simdpp::load_u<V::_T_SIMD>(&v);
		V::_T_SIMDMASK mask = simdpp::cmp_ge(val, simdpp::load_splat<V::_T_SIMD>(&minVal));
		mask = mask & simdpp::cmp_le(val, simdpp::load_splat<V::_T_SIMD>(&maxVal));
		mask = mask | simdpp::to_mask(simdpp::make_float<V::_T_SIMD>(0, 0, 0, 1));
		return simdpp::reduce_and(simdpp::bit_cast<simdpp::uint32x4, V::_T_SIMDMASK>(mask)) != 0;
	}
	template <typename T, typename T_SIMD, typename T_SIMDMASK> bool inRange(const Vector2<T, T_SIMD, T_SIMDMASK>& v, T_SIMD minVal, T_SIMD maxVal) { return _inRange(v, minVal, maxVal); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> bool inRange(const Vector3<T, T_SIMD, T_SIMDMASK>& v, T_SIMD minVal, T_SIMD maxVal) { return _inRange(v, minVal, maxVal); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> bool inRange(const Vector4<T, T_SIMD, T_SIMDMASK>& v, T_SIMD minVal, T_SIMD maxVal) { return _inRange(v, minVal, maxVal); }

	template <typename V> inline bool _inRange01(const V& v) { return _inRange(v, (typename V::_T)0, (typename V::_T)1); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> bool inRange01(const Vector2<T, T_SIMD, T_SIMDMASK>& v) { return _inRange01(v); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> bool inRange01(const Vector3<T, T_SIMD, T_SIMDMASK>& v) { return _inRange01(v); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> bool inRange01(const Vector4<T, T_SIMD, T_SIMDMASK>& v) { return _inRange01(v); }


	template <typename V> inline V _clamp(const V& v, typename V::_T minVal, typename V::_T maxVal) {
		auto val1 = simdpp::load_u<V::_T_SIMD>(&v);
		auto val2 = simdpp::load_splat<V::_T_SIMD>(&minVal);
		auto val3 = simdpp::load_splat<V::_T_SIMD>(&maxVal);
		val1 = simdpp::blend(val1, val2, simdpp::cmp_gt(val1, val2));
		val1 = simdpp::blend(val1, val3, simdpp::cmp_lt(val1, val3));
		val1 = simdpp::insert<3, 4>(val1, 0);
		V res;
		simdpp::store_u(&res, val1);
		return std::move(res);
	}
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector2<T, T_SIMD, T_SIMDMASK> clamp(const Vector2<T, T_SIMD, T_SIMDMASK>& v, T_SIMD minVal, T_SIMD maxVal) { return _clamp(v, minVal, maxVal); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector3<T, T_SIMD, T_SIMDMASK> clamp(const Vector3<T, T_SIMD, T_SIMDMASK>& v, T_SIMD minVal, T_SIMD maxVal) { return _clamp(v, minVal, maxVal); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector4<T, T_SIMD, T_SIMDMASK> clamp(const Vector4<T, T_SIMD, T_SIMDMASK>& v, T_SIMD minVal, T_SIMD maxVal) { return _clamp(v, minVal, maxVal); }

	template <typename V> inline V _clamp01(const V& v) { return _clamp(v, (typename V::_T)0, (typename V::_T)1); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector2<T, T_SIMD, T_SIMDMASK> clamp01(const Vector2<T, T_SIMD, T_SIMDMASK>& v) { return _clamp01(v); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector3<T, T_SIMD, T_SIMDMASK> clamp01(const Vector3<T, T_SIMD, T_SIMDMASK>& v) { return _clamp01(v); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector4<T, T_SIMD, T_SIMDMASK> clamp01(const Vector4<T, T_SIMD, T_SIMDMASK>& v) { return _clamp01(v); }

	template <typename V> inline V _lerp2(const V& v1, const V& v2, float t) {
		//auto val1 = simdpp::load_u<V::_T_SIMD>(&v1);
		//auto val2 = simdpp::load_u<V::_T_SIMD>(&v2);
		//V res;
		//simdpp::store_u(&res, simdpp::fmadd(val1, simdpp::splat<V::_T_SIMD>(1.0f - t), simdpp::mul(val2, simdpp::splat<V::_T_SIMD>(t))));
		//return std::move(res);
		return (1.0f - t) * v1 + t * v2;
	}
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector2<T, T_SIMD, T_SIMDMASK> lerp(const Vector2<T, T_SIMD, T_SIMDMASK>& v1, const Vector2<T, T_SIMD, T_SIMDMASK>& v2, float t) { return _lerp2(v1, v2, t); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector3<T, T_SIMD, T_SIMDMASK> lerp(const Vector3<T, T_SIMD, T_SIMDMASK>& v1, const Vector3<T, T_SIMD, T_SIMDMASK>& v2, float t) { return _lerp2(v1, v2, t); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector4<T, T_SIMD, T_SIMDMASK> lerp(const Vector4<T, T_SIMD, T_SIMDMASK>& v1, const Vector4<T, T_SIMD, T_SIMDMASK>& v2, float t) { return _lerp2(v1, v2, t); }

	template <typename V> inline V _lerp3(const V& v1, const V& v2, const V& v3, float t1, float t2) {
		//auto val1 = simdpp::load_u<V::_T_SIMD>(&v1);
		//auto val2 = simdpp::load_u<V::_T_SIMD>(&v2);
		//auto val3 = simdpp::load_u<V::_T_SIMD>(&v3);
		//V res;
		//simdpp::store_u(&res, 
		//	simdpp::fmadd(val1, simdpp::splat<V::_T_SIMD>(t1),
		//		simdpp::fmadd(val2, simdpp::splat<V::_T_SIMD>(t2), simdpp::mul(val3, simdpp::splat<V::_T_SIMD>(1.0f - t1 - t2)))
		//		)
		//	);
		//return std::move(res);
		return t1 * v1 + t2 * v2 + (1.0f - t1 - t2) * v3;
	}
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector2<T, T_SIMD, T_SIMDMASK> lerp(const Vector2<T, T_SIMD, T_SIMDMASK>& v1, const Vector2<T, T_SIMD, T_SIMDMASK>& v2, const Vector2<T, T_SIMD, T_SIMDMASK>& v3, float t1, float t2) { return _lerp3(v1, v2, v3, t1, t2); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector3<T, T_SIMD, T_SIMDMASK> lerp(const Vector3<T, T_SIMD, T_SIMDMASK>& v1, const Vector3<T, T_SIMD, T_SIMDMASK>& v2, const Vector3<T, T_SIMD, T_SIMDMASK>& v3, float t1, float t2) { return _lerp3(v1, v2, v3, t1, t2); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector4<T, T_SIMD, T_SIMDMASK> lerp(const Vector4<T, T_SIMD, T_SIMDMASK>& v1, const Vector4<T, T_SIMD, T_SIMDMASK>& v2, const Vector4<T, T_SIMD, T_SIMDMASK>& v3, float t1, float t2) { return _lerp3(v1, v2, v3, t1, t2); }

	template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector3<T, T_SIMD, T_SIMDMASK> faceForward(const Vector3<T, T_SIMD, T_SIMDMASK>& n, const Vector3<T, T_SIMD, T_SIMDMASK>& v) { return (dot(n, v) < 0.0f) ? -n : n; }

	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------

	struct BasisVectors {
		BasisVectors(const Vector3f& v) {
			e1 = normalize(v);
			e2 = cross(v,
				fabsf(v.x) < 0.5f ? Vector3f(1, 0, 0) :
				(fabsf(v.y) < 0.5f ? Vector3f(0, 1, 0) : Vector3f(0, 0, 1))).normalize();
			e3 = cross(e1, e2).normalize();
		}

		Vector3f e1, e2, e3;

		Vector3f fromGlobal(const Vector3f& v) { return Vector3f(dot(v, e1), dot(v, e2), dot(v, e3)); }
		Vector3f toGlobal(float x, float y, float z) { return e1 * x + e2 * y + e3 * z; }
		Vector3f toGlobal(const Vector3f& v) { return e1 * v.x + e2 * v.y + e3 * v.z; }
	};

	Vector3f sampleVectorFromHemiSphere(const Vector3f& v, Sampler& sampler) {
		BasisVectors basis(v);
		float phai = asinf(sampler.randf(1.0f));
		float theta = sampler.randf(2.0f * Pi);
		return basis.toGlobal(sinf(phai), cosf(phai) * cosf(theta), cosf(phai) * sinf(theta));
	}

	Vector3f sampleVectorFromSphere(Sampler& sampler) {
		float phai = asinf(sampler.randf(-1.0f, 1.0f));
		float theta = sampler.randf(2.0f * Pi);
		return Vector3f(sinf(phai), cosf(phai) * cosf(theta), cosf(phai) * sinf(theta));
	}

	Vector3f sampleVectorFromCosinedHemiSphere(const Vector3f& v, Sampler& sampler) {
		BasisVectors basis(v);
		
		float u = sampler.randf(1.0f);
		float r = sqrtf(u);
		float theta = sampler.randf(2.0f * Pi);
		return basis.toGlobal(safeSqrt(1.0f - u), r * cosf(theta), r * sinf(theta));
	}

}