#pragma once

#include "Utils.h"
#include "Vector.h"

namespace Xitils {

	class Object;
	class Shape;
	class Primitive;

	class Interaction {
	public:
		Vector3f p;
		Vector3f wo;
	};

	class SurfaceInteraction : public Interaction {
	public:
		Vector3f n;
		const Object* object = nullptr;
		const Shape* shape = nullptr;
		const Primitive* primitive = nullptr;

		struct Shading {
			Vector3f n;
		};
		Shading shading;
	};

}