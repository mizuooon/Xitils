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
			return v[randi(v.size()-1)];
		}

		template<typename T> const T& select(const std::vector<T>& v) {
			return v[randi(v.size() - 1)];
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
			return (x != 0xFFFFFFFF) ? x : randf(); // 0xFFFFFFFF ‚ÍŠÜ‚Ü‚È‚¢‚æ‚¤‚É‚·‚é
		}

	private:
		unsigned int x;
	};

}