#pragma once

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include "Utils.h"
#include "Vector.h"

namespace xitils {

	class Texture {
	public:

		bool warpClamp = true;
		bool filter = true;

		Texture(const std::string& filename) {
			float* tmp = stbi_loadf(filename.c_str(), &width, &height, &channel, STBI_rgb);
			data.resize(width * height * channel);
			memcpy_s(data.data(), data.size() * sizeof(float), tmp, data.size() * sizeof(float));

			stbi_image_free(tmp);
		}

		Texture(int width, int height):
			width(width),
			height(height),
			channel(3)
		{
			data.resize(width * height * channel);
			memset(data.data(), 0, sizeof(float) * data.size());
		}

		Texture(const Texture& tex) :
			width(tex.width),
			height(tex.height),
			channel(tex.channel),
			data(tex.data)
		{}

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
			ASSERT(channel == 4);
			return data[(x + y * width) * channel + 3];
		}

		Vector3f rgb(int x, int y) const {
			warp(x, y);
			int i = (x + y * width) * channel;
			return Vector3f(data[i + 0], data[i + 1], data[i + 2]);
		}

		Vector3f rgb(const Vector2f& uv) const {
			if (filter) {
				int x0 = floorf(uv.u * width - 0.5f);
				int y0 = floorf(uv.v * height - 0.5f);
				int x1 = x0 + 1;
				int y1 = y0 + 1;
				float wx1 = (uv.u * width - 0.5f) - x0;
				float wy1 = (uv.v * height - 0.5f) - y0;
				float wx0 = 1.0f - wx1;
				float wy0 = 1.0f - wy1;

				return wx0 * wy0 * rgb(x0, y0)
					+ wx0 * wy1 * rgb(x0, y1)
					+ wx1 * wy0 * rgb(x1, y0)
					+ wx1 * wy1 * rgb(x1, y1);
			} else {
				int x = uv.u * width;
				int y = uv.v * height;
				return rgb(x, y);
			}
		}

		Vector3f rgbDifferentialU(int x, int y) const {
			return (rgb(x + 1, y) - rgb(x - 1, y)) / 2.0f * width;
		}

		Vector3f rgbDifferentialV(int x, int y) const {
			return (rgb(x, y + 1) - rgb(x, y - 1)) / 2.0f * height;
		}

		Vector3f rgbDifferentialU(const Vector2f& uv) const {
			if (filter) {
				int x0 = floorf(uv.u * width - 0.5f);
				int y0 = floorf((1 - uv.v) * height - 0.5f);
				int x1 = x0 + 1;
				int y1 = y0 + 1;
				float wx1 = (uv.u * width - 0.5f) - x0;
				float wy1 = ((1 - uv.v) * height - 0.5f) - y0;
				float wx0 = 1.0f - wx1;
				float wy0 = 1.0f - wy1;

				return wx0 * wy0 * rgbDifferentialU(x0, y0)
					+ wx0 * wy1 * rgbDifferentialU(x0, y1)
					+ wx1 * wy0 * rgbDifferentialU(x1, y0)
					+ wx1 * wy1 * rgbDifferentialU(x1, y1);
			} else {
				int x = uv.u * width;
				int y = (1 - uv.v) * height;
				return rgbDifferentialU(x, y);
			}
		}

		Vector3f rgbDifferentialV(const Vector2f& uv) const {
			if (filter) {
				int x0 = floorf(uv.u * width - 0.5f);
				int y0 = floorf(uv.v * height - 0.5f);
				int x1 = x0 + 1;
				int y1 = y0 + 1;
				float wx1 = (uv.u * width - 0.5f) - x0;
				float wy1 = (uv.v * height - 0.5f) - y0;
				float wx0 = 1.0f - wx1;
				float wy0 = 1.0f - wy1;

				return wx0 * wy0 * rgbDifferentialV(x0, y0)
					+ wx0 * wy1 * rgbDifferentialV(x0, y1)
					+ wx1 * wy0 * rgbDifferentialV(x1, y0)
					+ wx1 * wy1 * rgbDifferentialV(x1, y1);
			} else {
				int x = uv.u * width;
				int y = uv.v * height;
				return rgbDifferentialV(x, y);
			}
		}

		int getWidth() const { return width; }
		int getHeight() const { return height; }
		int getChannel() const { return channel; }

		void warp(int& x, int& y) const {
			if (warpClamp) {
				x = clamp(x, 0, width - 1);
				y = clamp(y, 0, height - 1);
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

	private:
		int width, height, channel;
		std::vector<float> data;
	};

	//class TextureChecker : public Texture {
	//public:

	//	int split;
	//	Vector3f col0, col1;

	//	TextureChecker(int split, const Vector3f& col0, const Vector3f& col1) :
	//		split(split),
	//		col0(col0),
	//		col1(col1)
	//	{}

	//	Vector3f rgb(const Vector2f& uv) const override {
	//		Vector2f warpedUV = uv;
	//		warp(warpedUV.u, warpedUV.v);
	//		int x = warpedUV.u * split;
	//		int y = warpedUV.v * split;
	//		return (x + y) % 2 == 0 ? col0 : col1;
	//	}

	//	Vector3f rgbDifferentialU(const Vector2f& uv) const override {
	//		NOT_IMPLEMENTED;
	//	}

	//	Vector3f rgbDifferentialV(const Vector2f& uv) const override {
	//		NOT_IMPLEMENTED;
	//	}
	//};

	//class TextureNormalHump : public Texture {
	//public:

	//	int numU, numV;
	//	float scale;

	//	TextureNormalHump(int numU, int numV, float scale) :
	//		numU(numU),
	//		numV(numV),
	//		scale(scale)
	//	{}

	//	Vector3f rgb(const Vector2f& uv) const override {
	//		Vector2f warpedUV = uv;
	//		warp(warpedUV.u, warpedUV.v);
	//		warpedUV.u *= numU;
	//		warpedUV.v *= numV;
	//		warpedUV.u -= (int)warpedUV.u;
	//		warpedUV.v -= (int)warpedUV.v;

	//		Vector3f v;

	//		v.x = (uv.u - 0.5f) * 2;
	//		v.y = (uv.v - 0.5f) * 2;
	//		v.y *= -1;

	//		// x^2 + y^2 + (z*scale)^2 = 1;
	//		// z = sqrt(1 - x^2 - y^2) * scale

	//		float sq = 1 - v.x * v.x - v.y * v.y;
	//		if (sq >= 0.0f) {
	//			v.z = sqrtf(sq);
	//			v.z /= scale;
	//		} else {
	//			v = Vector3f(0, 0, 1);
	//		}

	//		Vector3f n = clamp01((v.normalize() + Vector3f(1.0f)) * 0.5f);

	//		return n;
	//	}

	//	Vector3f rgbDifferentialU(const Vector2f& uv) const override {
	//		NOT_IMPLEMENTED;
	//	}

	//	Vector3f rgbDifferentialV(const Vector2f& uv) const override {
	//		NOT_IMPLEMENTED;
	//	}
	//};
}