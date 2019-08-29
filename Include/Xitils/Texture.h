#pragma once

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "Utils.h"
#include "Vector.h"

namespace Xitils {

	class Texture {
	public:

		bool filter = true;
		bool warpClamp = true;
		
		Texture(const std::string& filename) {
			float* tmp = stbi_loadf(filename.c_str(), &width, &height, &channel, STBI_rgb);
			data.resize(width * height * channel);
			memcpy_s(data.data(), data.size()*sizeof(float), tmp, data.size()*sizeof(float));

			stbi_image_free(tmp);
		}

		float& operator[](int i) { return data[i]; }
		float& r(int x, int y) {
			warp(x, y);
			return data[(x + y * width) * channel + 0];
		}
		float& g(int x, int y) {
			warp(x, y);
			return data[(x + y * width) * channel + 1];
		}
		float& b(int x, int y) {
			warp(x, y);
			return data[(x + y * width) * channel + 2];
		}
		float& a(int x, int y) {
			warp(x, y);
			ASSERT(channel==4);
			return data[(x + y * width) * channel + 3];
		}

		Vector3f rgb(int x, int y) const {
			warp(x, y);
			int i = (x + y * width) * channel;
			return Vector3f(data[i + 0], data[i + 1], data[i + 2]);
		}

		Vector3f rgb(Vector2f uv) const {
			if (filter) {
				int x0 = uv.u * width + 0.5f;
				int y0 = uv.v * height + 0.5f;
				int x1 = x0 + 1;
				int y1 = y0 + 1;
				float wx1 = (uv.u * width + 0.5f) - x0;
				float wy1 = (uv.v * height + 0.5f) - y0;
				float wx0 = 1.0f - wx1;
				float wy0 = 1.0f - wy1;

				warp(x0, y0);
				warp(x1, y1);

				return wx0 * wy0 * rgb(x0, y0)
					+ wx0 * wy1 * rgb(x0, y1)
					+ wx1 * wy0 * rgb(x1, y0)
					+ wx1 * wy1 * rgb(x1, y1);
			} else {
				int x = uv.u * width;
				int y = uv.v * height;
				warp(x, y);
				return rgb(x, y);
			}
		}

		int getWidth() const { return width; }
		int getHeight() const { return height; }
		int getChannel() const { return channel; }

	private:
		int width, height, channel;
		std::vector<float> data;

		void warp(int& x, int& y) const {
			if (warpClamp) {
				x = clamp(x, 0, width - 1);
				y = clamp(y, 0, width - 1);
			} else {
				x %= width;
				y %= height;
				if (x < 0) { x += width; }
				if (y < 0) { y += height; }
			}
		}

		void warp(float& u, float& v) const {
			if (warpClamp) {
				u = clamp(u, 0.0f, 1.0f);
				v = clamp(v, 0.0f, 1.0f);
			} else {
				u = u - floorf(u);
				v = v - floorf(v);
			}
		}
	};

}