#pragma once

#include <random>
#include "Utils.h"

namespace Xitils {

	class Sampler {
	public:
		virtual float randf() = 0;
		float randf(float max) {
			return randf() * max;
		}

		float randf(float min, float max) {
			float u = randf();
			return (1-u) * min + u * max;
		}
	};

	class SamplerMersenneTwister : public Sampler {
	public:
		
		SamplerMersenneTwister(int seed) :
			rand(seed)
		{}

		float randf() {
			return dist(rand);
		}

	private:
		std::mt19937 rand;
		std::uniform_real_distribution<float> dist;
	};

	class SamplerXORShift : public Sampler {
	public:

		SamplerXORShift(int seed) :
			x(seed)
		{}

		float randf() {
			x ^= x << 13;
			x ^= x >> 17;
			x ^= x << 5;
			return (float)x / (0xFFFFFFFF);
		}


	private:
		int x;
	};

}