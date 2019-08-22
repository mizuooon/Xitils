#pragma once

#include "Utils.h"
#include "Vector.h"

namespace Xitils {

	class ImageBuffer {
	public:
		std::vector<Vector3f> data;
		int width;
		int height;

		ImageBuffer(int width, int height) :
			data(width* height),
			width(width),
			height(height)
		{}

		Vector3f& operator[](const Vector2i& p) {
			return data[p.x + p.y * width];
		}

		void clear() {
			memset(data.data(), 0, width * height * sizeof(Vector3f));
		}

		void toneMap(ci::Surface* surface, int sampleNum) {
#pragma omp parallel for schedule(dynamic, 1)
			for (int y = 0; y < height; ++y) {
#pragma omp parallel for schedule(dynamic, 1)
				for (int x = 0; x < width; ++x) {
					Vector3f color = (*this)[Vector2i(x, y)];
					color /= sampleNum;

					ci::ColorA8u colA8u;
					colA8u.r = Xitils::clamp((int)(color.x * 255), 0, 255);
					colA8u.g = Xitils::clamp((int)(color.y * 255), 0, 255);
					colA8u.b = Xitils::clamp((int)(color.z * 255), 0, 255);
					colA8u.a = 255;

					surface->setPixel(glm::ivec2(x, y), colA8u);
				}
			}
		}
	};

	class ImageTile {
	public:
		static const int Width = 16;
		static const int Height = 16;
		std::shared_ptr<ImageBuffer> image;
		Vector2i offset;
		std::shared_ptr<Sampler> sampler;

		Vector2i ImagePosition(const Vector2i p) const {
			return offset + p;
		}

		Vector2f GenerateFilmPosition(const Vector2i p, bool jitter = true) const {
			auto pFilm = Vector2f(ImagePosition(p));
			pFilm.x /= image->width;
			pFilm.y /= image->height;
			pFilm.y = 1.0f - pFilm.y;

			pFilm.x += (sampler->sample() - 0.5f) / image->width;
			pFilm.y += (sampler->sample() - 0.5f) / image->height;

			return pFilm;
		}
	};

	class ImageTileCollection {
	public:
		std::vector<ImageTile> tiles;
		int tileX, tileY;

		ImageTileCollection(const std::shared_ptr<ImageBuffer>& image) {
			tileX = (image->width + ImageTile::Width - 1) / ImageTile::Width;
			tileY = (image->height + ImageTile::Height - 1) / ImageTile::Height;
			tiles.resize(tileX * tileY);
			for (int ty = 0; ty < tileY; ++ty) {
				for (int tx = 0; tx < tileX; ++tx) {
					int i = tx + ty * tileX;
					tiles[i].image = image;
					tiles[i].offset = Vector2i(tx * ImageTile::Width, ty * ImageTile::Height);
					tiles[i].sampler = std::make_shared<Sampler>(i);
				}
			}
		}

		int size() const { return tiles.size(); }

		ImageTile& operator[](int i) { return tiles[i]; }
	};
}