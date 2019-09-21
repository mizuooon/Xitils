#pragma once

#include "Utils.h"
#include "Bounds.h"
#include "Sampler.h"
#include "Vector.h"

namespace Xitils {

	using TableBinIndex = unsigned int;

	class Parameter {
		virtual unsigned int size() const = 0;
	};

	class Parameter1D : public Parameter {
	public:
		std::function<float(float)> f; // R -> [0, 1] 狭義単調増加である必要がある
		std::function<float(float)> fInv; // [0, 1] -> R
		float min;
		float max;
		int resolution;

		enum Loop {
			Clamp,
			Repeat,
			Reverse
		};
		Loop loop;

		static Parameter1D createLinear(float min, float max, int resolution, Loop loop = Clamp) {
			Parameter1D param;
			param.min = min;
			param.max = max;
			param.resolution = resolution;
			param.loop = loop;
			param.f = [](float x) { return x; };
			param.fInv = [](float x) { return x; };
			return std::move(param);
		}

		static Parameter1D createCosined(float min, float max, int resolution, Loop loop = Clamp) {
			Parameter1D param;
			param.min = min;
			param.max = max;
			param.resolution = resolution;
			param.loop = loop;
			param.f = cosf;
			param.fInv = acosf;
			return std::move(param);
		}

		static Parameter1D createSined(float min, float max, int resolution, Loop loop = Clamp) {
			Parameter1D param;
			param.min = min;
			param.max = max;
			param.resolution = resolution;
			param.loop = loop;
			param.f = sinf;
			param.fInv = asinf;
			return std::move(param);
		}

		virtual unsigned int size() const override {
			return resolution;
		}

		TableBinIndex evalValue(float x) const {
			auto val = normalizeValue(x);
			return clamp<unsigned int>((unsigned int)(val * size()), 0, size() - 1);
		}

		float evalIndex(TableBinIndex index, bool randomize, Sampler& sampler) const {
			float cellSize = (max - min) / resolution;
			float cellMin = min + cellSize * index;

			return (randomize ? sampler.randf() : 0.5f) * cellSize + cellMin;
		}

		float normalizeValue(float x) const {
			float valMin = f(min);
			float valMax = f(max);
			float y = (f(x) - valMin) / (valMin + valMax); // [0, 1]

			switch (loop) {
			case Parameter1D::Loop::Clamp:
				y = clamp01(y);
				break;
			case Parameter1D::Loop::Repeat:
				y = clamp01(y - floorf(y));
				break;
			case Parameter1D::Loop::Reverse:
				if (y < 0) { y += 2 * (-floor(y)); } // 適当な偶数を足して正にする
				y = clamp01(((int)y) % 2 == 0 ?
					y - floorf(y) // 整数部が偶数の場合 Repeat と同じ
					: 1.0f - (y - floorf(y)) // 整数部が奇数の場合 反転する
				);
				break;
			}
			return y;
		}
	};

	class Parameter2D : public Parameter {
	public:

		std::array<Parameter1D, 2> params;

		virtual unsigned int size() const override {
			return params[0].size() * params[1].size();
		}

		TableBinIndex evalValue(const Vector2f& v) const {
			auto x0 = params[0].normalizeValue(v.x);
			auto x1 = params[1].normalizeValue(v.y);
			return clamp<unsigned int>((unsigned int)(x0 * params[0].size()), 0, params[0].size() - 1) * params[1].size() +
				   clamp<unsigned int>((unsigned int)(x1 * params[1].size()), 0, params[1].size() - 1);
		}

		Vector2f evalIndex(TableBinIndex index, bool randomize, Sampler& sampler) const {
			TableBinIndex i0 = (TableBinIndex)index / params[1].size();
			TableBinIndex i1 = (TableBinIndex)index % params[1].size();
			auto v0 = params[0].evalIndex(i0, randomize, sampler);
			auto v1 = params[1].evalIndex(i1, randomize, sampler);
			return Vector2f( v0, v1 );
		}

	};

	class Parameter2DDirection : public Parameter2D {
	public:

		bool sphericallySymmetric = false;

		TableBinIndex evalValue(const Vector3f& v) const {
			Vector2f a = toAngle(normalize(v));

			if (sphericallySymmetric) {
				// theta < 0 の領域なら x0,x1 の指すベクトルを反転
				float theta = a.x;
				float valMin = -(float)M_PI / 2;
				float valMax = (float)M_PI / 2;
				float y = (params[0].f(theta) - valMin) / (valMin + valMax); // [0, 1]
				if (y < 0) { y += 2 * (-floor(y)); }
				y = clamp01(((int)y) % 2 == 0 ?
					y - floorf(y)
					: 1.0f - (y - floorf(y))
				);
				if (y < 0.5f) {
					a.x = -1 * a.x;
					a.y = modff(a.y + 0.5f, NULL);
				}
			}

			return Parameter2D::evalValue(a);
		}

		Vector3f evalIndex(TableBinIndex index, bool randomize, Sampler& sampler) const {
			TableBinIndex i0 = (TableBinIndex)index / params[1].size();
			TableBinIndex i1 = (TableBinIndex)index % params[1].size();
			auto v0 = params[0].evalIndex(i0, randomize, sampler);
			auto v1 = params[1].evalIndex(i1, randomize, sampler);

			if (sphericallySymmetric) {
				// TODO  *************************
			}

			return toVector(Vector2f(v0, v1));
		}

		static Vector2f toAngle(const Vector3f& vec) {
			auto n = normalize(vec);
			float theta = asinf(n.z); // [-PI/2, PI/2]
			float phai = (cos(theta) != 0.0f) ? atan2(n.y, n.x) : 0.0f; // [-PI, PI]
			return Vector2f( theta, phai );
		}

		static Vector3f toVector(const Vector2f& v) {
			return Vector3f(cosf(v[0]) * cosf(v[1]), cosf(v[0]) * sinf(v[1]), sinf(v[0]));
		}

		static Parameter2DDirection createSpherical(int resolutionPhai, bool symmetric = false) {
			Parameter2DDirection result;
			result.sphericallySymmetric = symmetric;
			result.params[0] = // theta
				!result.sphericallySymmetric ?
				Parameter1D::createSined(-(float)M_PI / 2.0f, (float)M_PI / 2.0f, resolutionPhai / 2, Parameter1D::Loop::Reverse) :
				Parameter1D::createSined(0.0f, (float)M_PI / 2.0f, resolutionPhai / 4, Parameter1D::Loop::Reverse);
			result.params[1] = Parameter1D::createLinear(-(float)M_PI, (float)M_PI, resolutionPhai, Parameter1D::Loop::Repeat); // phai
			return result;
		}
	};

	class Parameter3D : public Parameter {
	public:

		std::array<Parameter1D, 3> params;

		virtual unsigned int size() const override {
			return params[0].size() * params[1].size() * params[2].size();
		}

		TableBinIndex evalValue(const Vector3f& v) const {
			auto x0 = params[0].evalValue(v.x);
			auto x1 = params[1].evalValue(v.y);
			auto x2 = params[2].evalValue(v.z);
			return x0 * params[1].size() * params[2].size() +
				   x1 * params[2].size() +
				   x2;
		}

		Vector3f evalIndex(TableBinIndex index, bool randomize, Sampler& sampler) const {
			TableBinIndex i0 = (TableBinIndex)index / (params[1].size() * params[2].size());
			TableBinIndex i1 = ((TableBinIndex)index / params[2].size()) % params[1].size();
			TableBinIndex i2 = (TableBinIndex)index % params[2].size();
			auto v0 = params[0].evalIndex(i0, randomize, sampler);
			auto v1 = params[1].evalIndex(i1, randomize, sampler);
			auto v2 = params[2].evalIndex(i2, randomize, sampler);
			return Vector3f( v0, v1, v2 );
		}

		static Parameter3D grid(float min_x, float max_x, int resolution_x,
			float min_y, float max_y, int resolution_y,
			float min_z, float max_z, int resolution_z) {
			Parameter3D result;
			result.params[0] = Parameter1D::createLinear(min_x, max_x, resolution_x, Parameter1D::Loop::Clamp);
			result.params[1] = Parameter1D::createLinear(min_y, max_y, resolution_y, Parameter1D::Loop::Clamp);
			result.params[2] = Parameter1D::createLinear(min_z, max_z, resolution_z, Parameter1D::Loop::Clamp);
			return std::move(result);
		}
	};

	template <typename... Params>
	class Iterator {
	public:
		constexpr static int N = std::tuple_size<std::tuple<Params...>>::value;

		Iterator(const Params* ... params) : _params(params...) {
			initializeState(std::make_index_sequence<N>{});
		}

		std::array<TableBinIndex, N> index() const {
			return _state;
		}

		bool next() {
			return nextSub<N - 1>();
		}
		decltype(auto) value(bool randomize = false) const {
			std::tuple<Params::Value...> tuple;
			valueSub < 0, decltype(tuple)> ::func(this, &tuple, randomize);
			return tuple;
		}

	private:
		template<std::size_t... indices>
		void initializeState(std::index_sequence<indices...>) {
			using swallow = int[];
			(void)swallow {
				(_state[indices] = 0, 0)...
			};
		}

		template <int M>
		bool nextSub() {
			++_state[M];
			if (_state[M] >= std::get<M>(_params)->size()) {
				_state[M] = 0;
				return nextSub<M - 1>();
			}
			return true;
		}
		template <>
		bool nextSub<-1>() {
			return false;
		}

		template <int M, typename Tuple>
		struct valueSub {
			static void func(const Iterator<Params...>* it, Tuple* tuple, bool randomize) {
				std::get<M>(*tuple) = *std::get<M>(it->_params)->evalIndex(it->_state[M], randomize);
				valueSub<M + 1, Tuple>::func(it, tuple, randomize);
			}

		};
		template <typename Tuple>
		struct valueSub<std::tuple_size<std::tuple<Params...>>::value, Tuple> {
			static void func(const Iterator<Params...>* it, Tuple* tuple, bool randomize) {
			}
		};

		std::tuple<const Params* ...> _params;
		std::array<TableBinIndex, N> _state;

	};

	template <typename ValueType, typename... Params>
	class Table {
	public:
		constexpr static int N = std::tuple_size<std::tuple<Params...>>::value;

		Table(Params... params) : _params(params...) {
			initializeData(std::make_index_sequence<N>{});
		}

		//template<typename T> T& operator() ( const std::array<float, D> &args ) {
		//	Index index;
		//	zip( args, params, &index, []( float x, Parameter p ) { return clamp( (int) p.f( x )*p.resolution, 0, p.resolution - 1 ); } );
		//	return _data[flattenIndex( index )];
		//}
		//template <typename... Args> std::optional<T> eval( Args... args ) { // 補間あり
		//	std::vector<TableBinIndex>index;
		//	return access( std::move( index ), args... );
		//}
		template <typename... Args> std::optional<ValueType*> access(Args... args) { // 補間なし テーブルの値に書き込み可能
			return access(std::make_index_sequence<N>{}, args...);
		}

		ValueType& operator[] (const std::vector<TableBinIndex>& index) {
			return _data[flattenIndex(index)];
		}
		const std::vector<ValueType>& data() const {
			return _data;
		}
		const std::tuple<Params...>& params() const {
			return _params;
		}

		template<std::size_t... i>
		decltype(auto) iterator(std::index_sequence<i...>) {
			return Iterator(&std::get<i>(_params)...);
		}

	private:
		template<std::size_t... i>
		void initializeData(std::index_sequence<i...>) {
			unsigned int size = 1;
			using swallow = int[];
			(void)swallow {
				(size *= std::get<i>(_params).size(), 0)...
			};
			_data.resize(size);
		}

		template<std::size_t... i, typename... Args>
		std::optional<ValueType*> access(std::index_sequence<i...>, Args... args) {
			std::tuple<Args...> a{ args... };
			std::array<std::optional<TableBinIndex>, N> indices{
				(std::get<i>(_params).evalValue(std::get<i>(a)) , 0)...
			};
			auto flatten = flattenIndex(indices);
			if (!flatten) { return std::nullopt; }
			return &_data[*flatten];
		}

		std::optional<TableBinIndex> flattenIndex(const std::array<std::optional<TableBinIndex>, N>& indices) const {
			return flattenIndex(std::make_index_sequence<N>{}, indices);
		}
		template<std::size_t... i>
		std::optional<TableBinIndex> flattenIndex(std::index_sequence<i...>, const std::array<std::optional<TableBinIndex>, N>& indices) const {
			bool valid = true;
			using swallow = int[];
			(void)swallow {
				(valid &= indices[i].has_value(), 0)...
			};
			if (!valid) { return std::nullopt; }

			unsigned int result = *indices[0];
			(void)swallow {
				(result *= (i > 0) ? std::get<i>(_params).size() : 1, result += (i > 0) ? *indices[i] : 0, 0)...
			};

			return result;
		}

		std::tuple<Params...> _params;
		std::vector<ValueType> _data;
	};
}