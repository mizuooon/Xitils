#pragma once

#include "Utils.h"
#include "Vector.h"
#include "Shape.h"

namespace Xitils {

	class Shape;

	class Interaction {
	public:
		Vector3f p;
		Vector3f wo;
	};

	class SurfaceInteraction : public Interaction {
	public:
		Vector3f n;
		const Shape* shape = nullptr;

		struct Shading {
			Vector3f n;
		};
		Shading shading;
	};

}