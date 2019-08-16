#pragma once

#include "Shape.h"

namespace Xitils {

	class TriangleMesh : public Shape {
	public:

		std::vector<Vector3f> positions;
		std::vector<Vector3f> normals;
		std::vector<int> indices;

	};

}