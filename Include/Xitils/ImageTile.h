#pragma once

#include "Utils.h"
#include "Vector.h"

namespace Xitils {

	class ImageTile {
	public:
		static const int Width = 16;
		static const int Height = 16;
		ci::Surface* surface;
		Vector2i offset;
		std::shared_ptr<Sampler> sampler;

		Vector2i ImagePosition(const Vector2i p) const {
			return offset + p;
		}

		Vector2f GenerateFilmPosition(const Vector2i p) const {
			auto pFilm = Vector2f(ImagePosition(p));
			pFilm.x /= surface->getWidth();
			pFilm.y /= surface->getHeight();
			pFilm.y = 1.0f - pFilm.y;
			return pFilm;
		}
	};

	class ImageTileCollection {
	public:
		std::vector<ImageTile> tiles;
		int tileX, tileY;

		ImageTileCollection(ci::Surface& surface) {
			tileX = (surface.getWidth() + ImageTile::Width - 1) / ImageTile::Width;
			tileY = (surface.getHeight() + ImageTile::Height - 1) / ImageTile::Height;
			tiles.resize(tileX * tileY);
			for (int ty = 0; ty < tileY; ++ty) {
				for (int tx = 0; tx < tileX; ++tx) {
					int i = tx + ty * tileX;
					tiles[i].surface = &surface;
					tiles[i].offset = Vector2i(tx * ImageTile::Width, ty * ImageTile::Height);
					tiles[i].sampler = std::make_shared<Sampler>(i);
				}
			}
		}

		int size() const { return tiles.size(); }

		ImageTile& operator[](int i) { return tiles[i]; }
	};

}