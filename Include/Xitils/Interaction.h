#pragma once

#include "Utils.h"
#include "Vector.h"

namespace xitils {

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
		Vector2f texCoord;
		Vector3f n;
		Vector3f tangent, bitangent;
		const Object* object = nullptr;
		const Shape* shape = nullptr;
		const Primitive* primitive = nullptr;

		struct Shading {
			Vector3f n;
			Vector3f tangent, bitangent;
		};
		Shading shading;
	};

}