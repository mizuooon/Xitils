#pragma once

#include <random>
#include "Utils.h"

namespace Xitils {

	class SamplerMersenneTwister {
	public:
		
		SamplerMersenneTwister(int seed) :
			rand(seed)
		{}

		float sample() {
			return dist(rand);
		}

	private:
		std::mt19937 rand;
		std::uniform_real_distribution<float> dist;
	};

	class SamplerXORShift {
	public:

		SamplerXORShift(int seed) :
			x(seed)
		{}

		float sample() {
			x ^= x << 13;
			x ^= x >> 17;
			x ^= x << 5;
			return (float)x / (0xFFFFFFFF);
		}

	private:
		int x;
	};

	//using Sampler = SamplerMersenneTwister;
	using Sampler = SamplerXORShift;

}