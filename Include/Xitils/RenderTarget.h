#pragma once

#include "Utils.h"
#include "Vector.h"

namespace xitils {

	template<typename T> class RenderTargetTileCollection;

	template<typename T>
	class RenderTarget {
	public:
		std::vector<T> data;
		int width;
		int height;
		std::shared_ptr<RenderTargetTileCollection<T>> tiles;

		RenderTarget(int width, int height) :
			data(width* height),
			width(width),
			height(height),
			tiles(std::make_shared<RenderTargetTileCollection<T>>(this))
		{}

		T& operator[](const Vector2i& p) {
			return data[p.x + p.y * width];
		}

		void clear() {
			memset(data.data(), 0, width * height * sizeof(T));
		}

		void render(const Scene& scene, int sampleNum, std::function<void(const Vector2f&, Sampler&, T&)> f);
		void normalize(int sampleNum, std::function<void(T&, int)> f);

		void toneMap(ci::Surface* surface, int sampleNum, std::function<ci::ColorA8u(const T&, int)> f) {
#pragma omp parallel for schedule(dynamic, 1)
			for (int y = 0; y < height; ++y) {
#pragma omp parallel for schedule(dynamic, 1)
				for (int x = 0; x < width; ++x) {
					surface->setPixel(glm::ivec2(x, y), f((*this)[Vector2i(x, y)], sampleNum));
				}
			}
		}
	};

	template<typename T>
	class RenderTargetTile {
	public:
		static const int Width = 16;
		static const int Height = 16;
		const RenderTarget<T>* image;
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

	template<typename T>
	class RenderTargetTileCollection {
	public:
		std::vector<RenderTargetTile<T>> tiles;
		int tileX, tileY;

		RenderTargetTileCollection(const RenderTarget<T>* image) {
			tileX = (image->width + RenderTargetTile<T>::Width - 1) / RenderTargetTile<T>::Width;
			tileY = (image->height + RenderTargetTile<T>::Height - 1) / RenderTargetTile<T>::Height;
			tiles.resize(tileX * tileY);
			for (int ty = 0; ty < tileY; ++ty) {
				for (int tx = 0; tx < tileX; ++tx) {
					int i = tx + ty * tileX;
					tiles[i].image = image;
					tiles[i].offset = Vector2i(tx * RenderTargetTile<T>::Width, ty * RenderTargetTile<T>::Height);
					tiles[i].sampler = std::make_shared<Sampler>(i);
				}
			}
		}

		int size() const { return tiles.size(); }

		RenderTargetTile<T>& operator[](int i) { return tiles[i]; }
	};


	template<typename T>
	void RenderTarget<T>::render(const Scene& scene, int sampleNum, std::function<void(const Vector2f&, Sampler&, T&)> f) {
#pragma omp parallel for schedule(dynamic, 1)
		for (int i = 0; i < tiles->size(); ++i) {
			auto& tile = (*tiles)[i];
			for (int s = 0; s < sampleNum; ++s) {
				for (int ly = 0; ly < RenderTargetTile<T>::Height; ++ly) {
					if (tile.offset.y + ly >= height) { continue; }
					for (int lx = 0; lx < RenderTargetTile<T>::Width; ++lx) {
						if (tile.offset.x + lx >= width) { continue; }

						Vector2i localPos = Vector2i(lx, ly);
						auto pFilm = tile.GenerateFilmPosition(localPos, true);
						f(pFilm, *tile.sampler, (*this)[tile.ImagePosition(localPos)]);
					}
				}
			}
		}
	}

	template<typename T>
	void RenderTarget<T>::normalize(int sampleNum, std::function<void(T&, int)> f) {
#pragma omp parallel for schedule(dynamic, 1)
		for (int i = 0; i < tiles->size(); ++i) {
			auto& tile = (*tiles)[i];
			for (int s = 0; s < sampleNum; ++s) {
				for (int ly = 0; ly < RenderTargetTile<T>::Height; ++ly) {
					if (tile.offset.y + ly >= height) { continue; }
					for (int lx = 0; lx < RenderTargetTile<T>::Width; ++lx) {
						if (tile.offset.x + lx >= width) { continue; }

						Vector2i localPos = Vector2i(lx, ly);
						f((*this)[tile.ImagePosition(localPos)], sampleNum);
					}
				}
			}
		}
	}

	using SimpleRenderTarget = RenderTarget<Vector3f>;

	struct DenoisableRenderTargetPixel
	{
		Vector3f color;
		Vector3f albedo;
		Vector3f normal;

		DenoisableRenderTargetPixel& operator/=(int sampleNum)
		{
			color /= sampleNum;
			albedo /= sampleNum;
			normal /= sampleNum;
			return *this;
		}
	};
	using DenoisableRenderTarget = RenderTarget<DenoisableRenderTargetPixel>;

}