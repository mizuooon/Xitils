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

		// �W�I���g���̏���\��
		// ���̏��͕ω����Ȃ�
		Vector3f n;
		Vector3f tangent, bitangent;

		struct Shading {
			Vector3f n;
			Vector3f tangent, bitangent;
		};
		// �V�F�[�f�B���O�Ɏg�p�������\��
		// �m�[�}���}�b�s���O�Ȃǂŕω�����
		Shading shading;
	};

}