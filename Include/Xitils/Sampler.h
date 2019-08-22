#pragma once

#include <random>
#include "Utils.h"

namespace Xitils {

	class Sampler {
	public:
		
		Sampler(int seed) :
			rand(seed)
		{}

		float sample() {
			return dist(rand);
		}

	private:
		std::mt19937 rand;
		std::uniform_real_distribution<float> dist;
	};

}