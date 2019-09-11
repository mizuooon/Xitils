#pragma once

#include "Utils.h"
#include "Vector.h"
#include "Ray.h"

namespace Xitils {

	template <typename T, typename T_SIMD, typename T_SIMDMASK> class Bounds2 {
	public:

		using _B = Bounds2<T, T_SIMD, T_SIMDMASK>;
		using _V = Vector2<T, T_SIMD, T_SIMDMASK>;

		_V min;
		_V max;

		Bounds2(){
			T minNum = std::numeric_limits<T>::lowest();
			T maxNum = std::numeric_limits<T>::max();
			min = _V(maxNum, maxNum);
			max = _V(minNum, minNum);
		}

		Bounds2(const _V& p): min(p), max(p) {}

		Bounds2(const _V& p1, const _V& p2) : min(Xitils::min(p1,p2)), max(Xitils::max(p1, p2)) {}

		const _V& operator[](int i) const {
			ASSERT(i == 0 || i == 1);
			return i == 0 ? min : max;
		}

		_V& operator[](int i) {
			ASSERT(i == 0 || i == 1);
			return i == 0 ? min : max;
		}

		_V corner(int corner) const {
			ASSERT(corner >= 0 && corner < 4);
			return _V(
					(*this)[(corner & 1)].x,
					(*this)[(corner & 2) ? 1 : 0].y
				);
		}

		_B expand(const _B& b, T delta) {
			min -= _V(delta, delta);
			max += _V(delta, delta);
		}

		_V size() const {
			return max - min;
		}

		_V center() const {
			return (max + min) / 2;
		}

		T area() const{
			_V s = size();
			return s.x * s.y;
		}

		int maximumExtent() const {
			_V s = size();
			if (s.x > s.y) { return 0; }
			return 1;
		}

		_V lerp(const Vector2f& t) const {
			return _V(
				Xitils::lerp(min.x, max.x, t.x),
				Xitils::lerp(min.y, max.y, t.y));
		}

		_V offset(const _V& p) const {
			_V s = size();
			Vector2f q = p - min;
			if (max.x > min.x) { q.x /= max.x - min.x; }
			if (max.y > min.y) { q.y /= max.y - min.y; }
			return q;
		}

	};

	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------

	template <typename T, typename T_SIMD, typename T_SIMDMASK> class Bounds3 {
	public:

		using _B = Bounds3<T, T_SIMD, T_SIMDMASK>;
		using _V = Vector3<T, T_SIMD, T_SIMDMASK>;

		_V min;
		_V max;

		Bounds3() {
			T minNum = std::numeric_limits<T>::lowest();
			T maxNum = std::numeric_limits<T>::max();
			min = _V(maxNum, maxNum, maxNum);
			max = _V(minNum, minNum, minNum);
		}

		Bounds3(const _V& p) : min(p), max(p) {}

		Bounds3(const _V& p1, const _V& p2) : min(Xitils::min(p1, p2)), max(Xitils::max(p1, p2)) {}

		Bounds3(const _V& p1, const _V& p2, const _V& p3) : min(Xitils::min(p1, p2, p3)), max(Xitils::max(p1, p2, p3)) {}

		const _V& operator[](int i) const {
			ASSERT(i == 0 || i == 1);
			return i == 0 ? min : max;
		}

		_V& operator[](int i) {
			ASSERT(i == 0 || i == 1);
			return i == 0 ? min : max;
		}

		_V corner(int corner) const {
			ASSERT(corner >= 0 && corner < 8);
			return _V(
				(*this)[(corner & 1)].x,
				(*this)[(corner & 2) ? 1 : 0].y,
				(*this)[(corner & 4) ? 1 : 0].z
				);
		}

		_B& expand(T delta) {
			min -= _V(delta, delta, delta);
			max += _V(delta, delta, delta);
			return *this;
		}

		_V size() const {
			return max - min;
		}

		_V center() const {
			return (max + min) / 2;
		}

		T surfaceArea() const {
			_V s = size();
			return 2 * s.x * s.y * s.z;
		}

		T volume() const {
			_V s = size();
			return s.x * s.y * s.z;
		}

		int maximumExtent() const {
			_V s = size();
			if (s.x > s.y) { return 0; }
			return 1;
		}

		_V lerp(const Vector3f& t) const {
			return _V(
				lerp(min.x, max.x, t.x),
				lerp(min.y, max.y, t.y),
				lerp(min.z, max.z, t.z));
		}

		_V offset(const _V& p) const {
			_V s = size();
			Vector3f q = p - min;
			if (max.x > min.x) { q.x /= max.x - min.x; }
			if (max.y > min.y) { q.y /= max.y - min.y; }
			if (max.z > min.z) { q.z /= max.z - min.z; }
			return q;
		}

		bool intersect(const Ray& ray, float* hitt1, float* hitt2) const {
			float t1 = 0;
			float t2 = ray.tMax;
			for (int i = 0; i < 3; ++i) {
				float invD = 1.0f / ray.d[i];
				float tNear = (min[i] - ray.o[i]) * invD;
				float tFar  = (max[i] - ray.o[i]) * invD;
				if (tNear > tFar) { std::swap(tNear, tFar); }
				t1 = tNear > t1 ? tNear : t1;
				t2 = tFar < t2 ? tFar : t2;
				if (t1 > t2) { return false; }
			}
			if (hitt1) { *hitt1 = t1; }
			if (hitt2) { *hitt2 = t1; }
			return true;
		}

	};

	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------

	using Bounds2f = Bounds2<float, simdpp::float32x4, simdpp::mask_float32x4>;
	using Bounds2i = Bounds2<int, simdpp::int32x4, simdpp::mask_int32x4>;

	using Bounds3f = Bounds3<float, simdpp::float32x4, simdpp::mask_float32x4>;
	using Bounds3i = Bounds3<int, simdpp::int32x4, simdpp::mask_int32x4>;

	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------

	template<typename B> B _merge(const B& b1, const B& b2) {
		return B(
			Xitils::min(b1.min, b2.min),
			Xitils::max(b1.max, b2.max));
	}

	template <typename T, typename T_SIMD, typename T_SIMDMASK> Bounds2<T, T_SIMD, T_SIMDMASK> merge(const Bounds2<T, T_SIMD, T_SIMDMASK>& b1, const Bounds2<T, T_SIMD, T_SIMDMASK>& b2) { return _merge(b1, b2); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Bounds3<T, T_SIMD, T_SIMDMASK> merge(const Bounds3<T, T_SIMD, T_SIMDMASK>& b1, const Bounds3<T, T_SIMD, T_SIMDMASK>& b2) { return _merge(b1, b2); }

	template<typename B, typename V> B _merge(const B& b, const V& p) {
		return B(
			Xitils::min(b.min, p),
			Xitils::max(b.max, p));
	}

	template <typename T, typename T_SIMD, typename T_SIMDMASK> Bounds2<T, T_SIMD, T_SIMDMASK> merge(const Bounds2<T, T_SIMD, T_SIMDMASK>& b, const Vector2<T, T_SIMD, T_SIMDMASK>& p) { return _merge(b, p); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Bounds3<T, T_SIMD, T_SIMDMASK> merge(const Bounds3<T, T_SIMD, T_SIMDMASK>& b, const Vector3<T, T_SIMD, T_SIMDMASK>& p) { return _merge(b, p); }


	template <typename T, typename T_SIMD, typename T_SIMDMASK> bool overlap(const Bounds2<T, T_SIMD, T_SIMDMASK>& b1, const Bounds2<T, T_SIMD, T_SIMDMASK>& b2) {
		return b1.max.x >= b2.min.x && b1.min.x <= b2.max.x &&
			   b1.max.y >= b2.min.y && b1.min.y <= b2.max.y ;
	}

	template <typename T, typename T_SIMD, typename T_SIMDMASK> bool inside(const Bounds2<T, T_SIMD, T_SIMDMASK>& b, const Vector2<T, T_SIMD, T_SIMDMASK>& p) {
		return b.min.x <= p.x && p.x <= b.max.x &&
			   b.min.y <= p.y && p.y <= b.max.y ;
	}

	template <typename T, typename T_SIMD, typename T_SIMDMASK> bool inside(const Bounds3<T, T_SIMD, T_SIMDMASK>& b, const Vector3<T, T_SIMD, T_SIMDMASK>& p) {
		return b.min.x <= p.x && p.x <= b.max.x &&
			   b.min.y <= p.y && p.y <= b.max.y &&
			   b.min.z <= p.z && p.z <= b.max.z;
	}

	template <typename T, typename T_SIMD, typename T_SIMDMASK> bool insideExclusive(const Bounds2<T, T_SIMD, T_SIMDMASK>& b, const Vector2<T, T_SIMD, T_SIMDMASK>& p) {
		return b.min.x <= p.x && p.x < b.max.x &&
			   b.min.y <= p.y && p.y < b.max.y;
	}

	template <typename T, typename T_SIMD, typename T_SIMDMASK> bool insideExclusive(const Bounds3<T, T_SIMD, T_SIMDMASK>& b, const Vector3<T, T_SIMD, T_SIMDMASK>& p) {
		return b.min.x <= p.x && p.x < b.max.x &&
			   b.min.y <= p.y && p.y < b.max.y &&
			   b.min.z <= p.z && p.z < b.max.z;
	}

	template<typename B, typename V> B _expand(const B& b, const typename V::_T& delta) {
		return B( b.min - V(delta),
				  b.min + V(delta));
	}

	template <typename T, typename T_SIMD, typename T_SIMDMASK> Bounds2<T, T_SIMD, T_SIMDMASK> expand(const Bounds2<T, T_SIMD, T_SIMDMASK>& b, T delta) { return _expand(b, delta); }
	template <typename T, typename T_SIMD, typename T_SIMDMASK> Bounds3<T, T_SIMD, T_SIMDMASK> expand(const Bounds3<T, T_SIMD, T_SIMDMASK>& b, T delta) { return _expand(b, delta); }

}