#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <optional>

#include "Utils.h"
#include "Vector.h"
#include "Bounds.h"
#include "Transform.h"
#include "Ray.h"

namespace Xitils {

	class Interaction {
	public:

	};

	class SurfaceInteraction : Interaction {
	public:
		Vector3f p;
		Vector3f n;
	};

}