#pragma once

#include "Utils.h"
#include "Vector.h"
#include "Transform.h"
#include "Ray.h"
#include "Sampler.h"

namespace Xitils {

	class Camera {
	public:

		Transform cameraToWorld;

		Camera(const Transform& cameraToWorld):
			cameraToWorld(cameraToWorld)
		{}

		virtual Ray GenerateRay(const Vector2f pFilm, Sampler& sampler) const = 0;

	};

	class PinholeCamera : public Camera {
	public:

		float fov;
		float aspectRatio;

		PinholeCamera(const Transform& cameraToWorld, float fov, float aspectRatio) :
			Camera(cameraToWorld),
			fov(fov),
			aspectRatio(aspectRatio)
		{}

		Ray GenerateRay(const Vector2f pFilm, Sampler& sampler) const override {
			Ray ray;
			ray.o = Vector3f(0,0,0);
			ray.d = Vector3f(
					 tanf(fov/2) * (pFilm.x - 0.5f) * 2,
					 tanf(fov/2 * aspectRatio) * (pFilm.y - 0.5f) * 2,
				     1
				).normalize();

			return cameraToWorld(ray);
		}

	};

	class OrthographicCamera : public Camera {
	public:

		float width;
		float height;

		OrthographicCamera(const Transform& cameraToWorld, float width, float height) :
			Camera(cameraToWorld),
			width(width),
			height(height)
		{}

		Ray GenerateRay(const Vector2f pFilm, Sampler& sampler) const override {
			Ray ray;
			ray.o = Vector3f((pFilm.x - 0.5f) * width, (pFilm.y - 0.5f) * height, 0);
			ray.d = Vector3f(0, 0, 1);

			return cameraToWorld(ray);
		}

	};
}