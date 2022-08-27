#pragma once

#include "Utils.h"
#include "Vector.h"

namespace xitils {

	class Object;
	class Shape;
	class TriangleIndexed;

	class Interaction {
	public:
	};

	class SurfaceIntersection : public Interaction {
	public:
		Vector3f p;
		Vector3f wo;
		Vector2f texCoord;
		Vector3f n;
		Vector3f tangent, bitangent;
		const Object* object = nullptr;
		const Shape* shape = nullptr;
		const TriangleIndexed* tri = nullptr;

		struct Shading {
			Vector3f n;
			Vector3f tangent, bitangent;
		};
		Shading shading;
	};

}