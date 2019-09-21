#pragma once

#include "Utils.h"
#include "Vector.h"

namespace xitils {

	class Object;
	class Shape;
	class TriangleIndexed;

	class Intersection {
	public:
	};

	class SurfaceIntersection : public Intersection {
	public:
		Vector3f p;
		Vector3f wo;
		Vector2f texCoord;
		const Object* object = nullptr;
		const Shape* shape = nullptr;
		const TriangleIndexed* tri = nullptr;

		// ジオメトリの情報を表す
		// この情報は変化しない
		Vector3f n;
		Vector3f tangent, bitangent;

		struct Shading {
			Vector3f n;
			Vector3f tangent, bitangent;
		};
		// シェーディングに使用する情報を表す
		// ノーマルマッピングなどで変化する
		Shading shading;
	};

}