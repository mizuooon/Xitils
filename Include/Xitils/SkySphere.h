#pragma once

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "Utils.h"
#include "Vector.h"

namespace Xitils {

	class SkySphere {
	public: 
		virtual Vector3f getRadiance(const Vector3f& wi) = 0;
	};

	class SkySphereFromImage : public SkySphere {
	public:
		SkySphereFromImage(const std::string& filename) {
			
			float* tmp = stbi_loadf(filename.c_str(), &width, &height, &comp, STBI_rgb);
			data.resize(width * height);
			for (int i = 0; i < width * height; i++) {
				data[i].x = tmp[i * 3 + 0];
				data[i].y = tmp[i * 3 + 1];
				data[i].z = tmp[i * 3 + 2];
			}

			stbi_image_free(tmp);
		}

		Vector3f getRadiance(const Vector3f& wi) override {
			Vector3f d = -wi;

			float r = (1.0f / M_PI) * acosf(d.z) / sqrtf(d.x * d.x + d.y * d.y);
			float u = (d.x * r + 1.0f) / 2.0f;
			float v = (d.y * r + 1.0f) / 2.0f;

			int x0 = u * width + 0.5f;
			int y0 = v * height + 0.5f;
			int x1 = x0 + 1;
			int y1 = y0 + 1;
			float wx1 = (u * width + 0.5f) - x0;
			float wy1 = (v * height + 0.5f) - y0;
			float wx0 = 1.0f - wx1;
			float wy0 = 1.0f - wy1;

			x0 = clamp(x0, 0, width - 1);
			y0 = clamp(y0, 0, height - 1);
			x1 = clamp(x1, 0, width - 1);
			y1 = clamp(y1, 0, height - 1);

			auto val = [&](int x, int y) { return data[x + y * width]; };

			return wx0 * wy0 * val(x0, y0)
			  	 + wx0 * wy1 * val(x0, y1)
			 	 + wx1 * wy0 * val(x1, y0)
				 + wx1 * wy1 * val(x1, y1);
		}
	private:
		int width, height;
		int comp;
		std::vector<Vector3f> data;
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

