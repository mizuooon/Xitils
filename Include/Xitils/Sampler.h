#pragma once

#include <random>
#include "Utils.h"

namespace Xitils {

	class _SamplerMersenneTwister;
	class _SamplerXORShift;

	using Sampler = _SamplerMersenneTwister;

	//---------------------------------------------------

	class _Sampler {
	public:
		float randf() {
			return (float)rand() / 0xFFFFFFFF;
		}

		float randf(float max) {
			return randf() * max;
		}

		float randf(float min, float max) {
			float u = randf();
			return (1-u) * min + u * max;
		}

		int randi(int max) {
			return rand() % max;
		}

		template<typename T> T& select(std::vector<T>& v) {
			return v[randi(v.size())];
		}

		template<typename T> const T& select(const std::vector<T>& v) {
			return v[randi(v.size())];
		}

		// ウェイトに比例した確率で整数値を取得
		const int randiAlongWeights(const std::vector<float>& weights) {
			std::vector<float> normalizedWeights = weights;
			float weightSum = sum(normalizedWeights);
			for (auto& w : normalizedWeights) {
				w /= weightSum;
			}
			return randiAlongNormalizedWeights(normalizedWeights);
		}

		const int randiAlongNormalizedWeights(const std::vector<float>& weights, bool omitLast = false) {
			float u = randf();
			float accum = 0.0f;
			int i = 0;
			for (; i < (omitLast ? weights.size() : weights.size() - 1); ++i) {
				accum += weights[i];
				if (u <= accum) { break; }
			}
			return i;
		}

		template<typename T> const T& selectAlongWeights(const std::vector<T>& v, const std::vector<float>& weights) {			
			ASSERT(v.size() == weights.size());
			return v[randiAlongWeights(weights)];
		}

		template<typename T> const T& selectAlongNormalizedWeights(const std::vector<T>& v, const std::vector<float>& weights) {
			ASSERT(v.size() == weights.size() || v.size() == weights.size() + 1);
			return v[randiAlongNormalizedWeights(weigths, v.size() == weights.size() + 1)];
		}

	protected:
		virtual unsigned int rand() = 0;
	};

	class _SamplerMersenneTwister : public _Sampler {
	public:
		
		_SamplerMersenneTwister(int seed) :
			gen(seed),
			dist(0, 0xFFFFFFFF - 1)
		{}

	protected:
		unsigned int rand() override {
			return dist(gen);
		}

	private:
		std::mt19937 gen;
		std::uniform_int_distribution<unsigned int> dist;
	};

	// TODO: バグってる
	class _SamplerXORShift : public _Sampler {
	public:

		_SamplerXORShift(int seed) :
			x(seed)
		{}

	protected:
		unsigned int rand() override {
			x ^= x << 13;
			x ^= x >> 17;
			x ^= x << 5;
			return (x != 0xFFFFFFFF) ? x : randf(); // 0xFFFFFFFF は含まないようにする
		}

	private:
		unsigned int x;
	};

}