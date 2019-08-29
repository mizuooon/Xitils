#pragma once

#include "Texture.h"
#include "Utils.h"
#include "Vector.h"

namespace Xitils {

	class SkySphere {
	public: 
		virtual Vector3f getRadiance(const Vector3f& wi) = 0;
	};

	class SkySphereFromImage : public SkySphere {
	public:
		SkySphereFromImage(const std::string& filename):
			tex(filename)
		{}

		Vector3f getRadiance(const Vector3f& wi) override {
			Vector3f d = normalize(-wi);
			float sq = sqrtf(d.x * d.x + d.y * d.y);
			if (sq == 0.0f) {
				return Vector3f();
			}
			float r = (1.0f / M_PI) * acosf(d.z) / sq;
			float u = (d.x * r + 1.0f) / 2.0f;
			float v = (d.y * r + 1.0f) / 2.0f;

			return tex.rgb(Vector2f(u, v));
		}
	private:
		Texture tex;
	};

	class SkySphereUniform : SkySphere {
	public:
		SkySphereUniform(const Vector3f& color) : color(color) {}

		Vector3f getRadiance(const Vector3f& d) override {
			return color;
		}
	private:
		Vector3f color;
	};

}

