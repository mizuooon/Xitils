#pragma once

#include "Utils.h"
#include "Vector.h"

namespace Xitils {

	class RenderTargetTileCollection;

	class RenderTarget {
	public:
		std::vector<Vector3f> data;
		int width;
		int height;
		std::shared_ptr<RenderTargetTileCollection> tiles;

		RenderTarget(int width, int height) :
			data(width* height),
			width(width),
			height(height),
			tiles(std::make_shared<RenderTargetTileCollection>(this))
		{}

		Vector3f& operator[](const Vector2i& p) {
			return data[p.x + p.y * width];
		}

		void clear() {
			memset(data.data(), 0, width * height * sizeof(Vector3f));
		}

		void render(const Scene& scene, int sampleNum, std::function<void(const Vector2f&, Sampler&, Vector3f&)> f);

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

	class RenderTargetTile {
	public:
		static const int Width = 16;
		static const int Height = 16;
		const RenderTarget* image;
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

			if (jitter) {
				pFilm.x += (sampler->randf() - 0.5f) / image->width;
				pFilm.y += (sampler->randf() - 0.5f) / image->height;
			}

			return pFilm;
		}
	};

	class RenderTargetTileCollection {
	public:
		std::vector<RenderTargetTile> tiles;
		int tileX, tileY;

		RenderTargetTileCollection(const RenderTarget* image) {
			tileX = (image->width + RenderTargetTile::Width - 1) / RenderTargetTile::Width;
			tileY = (image->height + RenderTargetTile::Height - 1) / RenderTargetTile::Height;
			tiles.resize(tileX * tileY);
			for (int ty = 0; ty < tileY; ++ty) {
				for (int tx = 0; tx < tileX; ++tx) {
					int i = tx + ty * tileX;
					tiles[i].image = image;
					tiles[i].offset = Vector2i(tx * RenderTargetTile::Width, ty * RenderTargetTile::Height);
					tiles[i].sampler = std::make_shared<Sampler>(i);
				}
			}
		}

		int size() const { return tiles.size(); }

		RenderTargetTile& operator[](int i) { return tiles[i]; }
	};



	void RenderTarget::render(const Scene& scene, int sampleNum, std::function<void(const Vector2f&, Sampler&, Vector3f&)> f) {
#pragma omp parallel for schedule(dynamic, 1)
		for (int i = 0; i < tiles->size(); ++i) {
			auto& tile = (*tiles)[i];
			for (int s = 0; s < sampleNum; ++s) {
				for (int ly = 0; ly < RenderTargetTile::Height; ++ly) {
					if (tile.offset.y + ly >= height) { continue; }
					for (int lx = 0; lx < RenderTargetTile::Width; ++lx) {
						if (tile.offset.x + lx >= width) { continue; }

						Vector2i localPos = Vector2i(lx, ly);
						auto pFilm = tile.GenerateFilmPosition(localPos, true);
						f(pFilm, *tile.sampler, (*this)[tile.ImagePosition(localPos)]);

					}
				}
			}
		}
	}

}