#pragma once

#include "Utils.h"
#include "Vector.h"
#include "Transform.h"
#include "Ray.h"
#include "Sampler.h"

namespace xitils {

	class Camera {
	public:
		virtual void setCurrentTime(float time) = 0;
		virtual Ray generateRay(const Vector2f pFilm, Sampler& sampler) const = 0;
	};

	class PinholeCamera : public Camera {
	public:

		struct State
		{
			float fov;
			float aspectRatio;
			Transform cameraToWorld;
		};

		struct KeyFrame
		{
			float time;
			State state;
		};
		PinholeCamera(const Transform& cameraToWorld, float fov, float aspectRatio)
		{
			addKeyFrame(0, cameraToWorld, fov, aspectRatio);
			setCurrentTime(0);
		}
		PinholeCamera()
		{}

		void setCurrentTime(float time) override
		{
			currentState = getState(time);
		}

		Ray generateRay(const Vector2f pFilm, Sampler& sampler) const override {
			Ray ray;
			ray.o = Vector3f(0,0,0);
			ray.d = Vector3f(
					 tanf(currentState.fov/2) * (pFilm.x - 0.5f) * 2,
					 tanf(currentState.fov/2 * currentState.aspectRatio) * (pFilm.y - 0.5f) * 2,
				     1
				).normalize();

			return currentState.cameraToWorld(ray);
		}

		// 最後尾にキーフレームを足すのしか受け付けてないので注意 (insert できない)
		void addKeyFrame(float time, const Transform& cameraToWorld, float fov = -1, float aspectRatio = -1)
		{
			KeyFrame keyFrame;
			keyFrame.time = time;
			keyFrame.state.cameraToWorld = cameraToWorld;
			keyFrame.state.fov = fov >= fov ? fov : keyFrames[keyFrames.size() - 1].state.fov;
			keyFrame.state.aspectRatio = aspectRatio >= 0 ? aspectRatio : keyFrames[keyFrames.size() - 1].state.aspectRatio;

			keyFrames.push_back(keyFrame);
		}

	private:
		std::vector<KeyFrame> keyFrames;
		State currentState;

		State getState(float time)
		{
			auto [f0, f1, t] = getKeyFrames(time);
			const auto& s0 = f0.state;
			const auto& s1 = f1.state;

			State s;
			s.fov = lerp(s0.fov, s1.fov, t);
			s.aspectRatio = lerp(s0.aspectRatio, s1.aspectRatio, t);
			const Matrix4x4 mat = (1.0f - t) * s0.cameraToWorld.mat + t * s1.cameraToWorld.mat;
			const Matrix4x4 matInv = (1.0f - t) * s0.cameraToWorld.matInv + t * s1.cameraToWorld.matInv;
			s.cameraToWorld = Transform(mat, matInv);

			return s;
		}

		std::tuple<const KeyFrame&, const KeyFrame&, float> getKeyFrames(float time)
		{
			if(keyFrames.size() == 1)
			{
				return {keyFrames[0], keyFrames[0], 0};
			}

			for(unsigned int i = 0; i< keyFrames.size() - 1; i++)
			{
				const auto& f0 = keyFrames[i];
				const auto& f1 = keyFrames[i + 1];
				if(f0.time <= time && time < f1.time)
				{
					return {f0, f1, (time - f0.time) / (f1.time - f0.time)};
				}
			}

			const auto& last = *(--keyFrames.end());
			return { last, last, 0 };
		}

	};

	// TODO : time 対応
	class OrthographicCamera : public Camera {
	public:

		Transform cameraToWorld;
		float width;
		float height;

		OrthographicCamera(const Transform& cameraToWorld, float width, float height) :
			cameraToWorld(cameraToWorld),
			width(width),
			height(height)
		{}

		void setCurrentTime(float time) override {}

		Ray generateRay(const Vector2f pFilm, Sampler& sampler) const override {
			Ray ray;
			ray.o = Vector3f((pFilm.x - 0.5f) * width, (pFilm.y - 0.5f) * height, 0);
			ray.d = Vector3f(0, 0, 1);

			return cameraToWorld(ray);
		}

	};
}