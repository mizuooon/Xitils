#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <optional>

#include "Utils.h"
#include "Vector.h"
#include "Matrix.h"
#include "Ray.h"

namespace Xitils {

	class Transform {
	public:

		Matrix4x4 mat;
		Matrix4x4 matInv;

		Transform():
			mat(),
			matInv()
		{}

		Transform(const Matrix4x4& mat):
			mat(mat),
			matInv(transpose(mat))
		{}

		Transform(const Matrix4x4 & mat, const Matrix4x4& matInv) :
			mat(mat),
			matInv(matInv)
		{}

		Transform(
			float m00, float m01, float m02, float m03,
			float m10, float m11, float m12, float m13,
			float m20, float m21, float m22, float m23,
			float m30, float m31, float m32, float m33):
			mat( m00, m01, m02, m03,
				 m10, m11, m12, m13,
				 m20, m21, m22, m23,
				 m30, m31, m32, m33 ),
			matInv( m00, m10, m20, m30,
				    m01, m11, m21, m31,
				    m02, m12, m22, m32,
				    m03, m13, m23, m33)
		{}

		Transform(const Transform& t):
			mat(t.mat),
			matInv(t.matInv)
		{}

		Transform operator*(const Transform& t) const {
			return Transform(mat * t.mat, t.matInv * matInv);
		}

		Transform operator*=(const Transform& t) {
			mat = mat * t.mat;
			matInv = t.matInv * matInv;
		}

		template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector3<T, T_SIMD, T_SIMDMASK> operator()(const Vector3<T, T_SIMD, T_SIMDMASK>& v) const {
			T xp = mat[0][0] * v[0] + mat[0][1] * v[1] + mat[0][2] * v[2] + mat[0][3];
			T yp = mat[1][0] * v[0] + mat[1][1] * v[1] + mat[1][2] * v[2] + mat[1][3];
			T zp = mat[2][0] * v[0] + mat[2][1] * v[1] + mat[2][2] * v[2] + mat[2][3];
			T wp = mat[3][0] * v[0] + mat[3][1] * v[1] + mat[3][2] * v[2] + mat[3][3];
			if (wp == 1) {
				return Vector3<T>(xp, yp, zp);
			} else {
				return Vector3<T>(xp, yp, zp) / wp;
			}
		}

		template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector4<T, T_SIMD, T_SIMDMASK> operator()(const Vector4<T, T_SIMD, T_SIMDMASK>& v) const {
			T xp = mat[0][0] * v[0] + mat[0][1] * v[1] + mat[0][2] * v[2] + mat[0][3] * v[3];
			T yp = mat[1][0] * v[0] + mat[1][1] * v[1] + mat[1][2] * v[2] + mat[1][3] * v[3];
			T zp = mat[2][0] * v[0] + mat[2][1] * v[1] + mat[2][2] * v[2] + mat[2][3] * v[3];
			T wp = mat[3][0] * v[0] + mat[3][1] * v[1] + mat[3][2] * v[2] + mat[3][3] * v[3];
			return Vector4<T>(xp, yp, zp, wp);
		}

		template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector3<T, T_SIMD, T_SIMDMASK> asVector(const Vector3<T, T_SIMD, T_SIMDMASK>& v) const {
			T xp = mat[0][0] * v[0] + mat[0][1] * v[1] + mat[0][2] * v[2];
			T yp = mat[1][0] * v[0] + mat[1][1] * v[1] + mat[1][2] * v[2];
			T zp = mat[2][0] * v[0] + mat[2][1] * v[1] + mat[2][2] * v[2];
			return Vector3<T>(xp, yp, zp);
		}

		template <typename T, typename T_SIMD, typename T_SIMDMASK> Vector3<T, T_SIMD, T_SIMDMASK> asNormal(const Vector3<T, T_SIMD, T_SIMDMASK>& v) const {
			T xp = mat[0][0] * v[0] + mat[0][1] * v[1] + mat[0][2] * v[2];
			T yp = mat[1][0] * v[0] + mat[1][1] * v[1] + mat[1][2] * v[2];
			T zp = mat[2][0] * v[0] + mat[2][1] * v[1] + mat[2][2] * v[2];
			return Vector3<T>(xp, yp, zp);
		}

		Ray operator()(const Ray& r) const {
			return Ray( (*this)(r.o), asVector(r.d), r.depth, r.tMax );
		}

		bool swapsHandedness() const {
			float det =
				mat.m[0][0] * (mat.m[1][1] * mat.m[2][2] - mat.m[1][2] * mat.m[2][1]) -
				mat.m[0][1] * (mat.m[1][0] * mat.m[2][2] - mat.m[1][2] * mat.m[2][0]) +
				mat.m[0][2] * (mat.m[1][0] * mat.m[2][1] - mat.m[1][1] * mat.m[2][0]);
			return det < 0;
		}
		
	};

	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------

	Transform transpose(const Transform& t) {
		return Transform(t.matInv, t.mat);
	}

	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------

	Transform translate(float x, float y, float z) {
		Matrix4x4 mat(
			1, 0, 0, x,
			0, 1, 0, y,
			0, 0, 1, z,
			0, 0, 0, 1
		);
		Matrix4x4 matInv(
			1, 0, 0, -x,
			0, 1, 0, -y,
			0, 0, 1, -z,
			0, 0, 0, 1
		);
		return Transform(mat, matInv);
	}

	Transform translate(const Vector3f& v) {
		return translate(v.x, v.y, v.z);
	}

	Transform scale(float x, float y, float z) {
		Matrix4x4 mat(
			x, 0, 0, 0,
			0, y, 0, 0,
			0, 0, z, 0,
			0, 0, 0, 1
		);
		Matrix4x4 matInv(
			1/x, 0, 0, 0,
			0, 1/y, 0, 0,
			0, 0, 1/z, 0,
			0, 0, 0, 1
		);
		return Transform(mat, matInv);
	}

	Transform scale(const Vector3f& v) {
		return scale(v.x, v.y, v.z);
	}

	Transform rotateX(float theta) {
		float thetaRad = theta * ToRad;
		float sinTheta = std::sin(thetaRad);
		float cosTheta = std::cos(thetaRad);
		return Transform(
			1, 0, 0, 0,
			0, cosTheta, -sinTheta, 0,
			0, sinTheta, cosTheta, 0,
			0, 0, 0, 1
		);
	}

	Transform rotateY(float theta) {
		float thetaRad = theta * ToRad;
		float sinTheta = std::sin(thetaRad);
		float cosTheta = std::cos(thetaRad);
		return Transform(
			cosTheta, 0, sinTheta, 0,
			0, 1, 0, 0,
			-sinTheta, 0, cosTheta, 0,
			0, 0, 0, 1
		);
	}

	Transform rotateZ(float theta) {
		float thetaRad = theta * ToRad;
		float sinTheta = std::sin(thetaRad);
		float cosTheta = std::cos(thetaRad);
		return Transform(
			cosTheta, -sinTheta, 0, 0,
			sinTheta, cosTheta, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1
		);
	}

	Transform rotate(float theta, const Vector3f& axis) {
		Vector3f a = normalize(axis);
		float thetaRad = theta * ToRad;
		float sinTheta = std::sin(thetaRad);
		float cosTheta = std::cos(thetaRad);
		Matrix4x4 mat;
		float ax2 = a.x * a.x;
		float ay2 = a.y * a.y;
		float az2 = a.z * a.z;
		float axy1Cos = a.x * a.y * (1 - cosTheta);
		float ayz1Cos = a.y * a.z * (1 - cosTheta);
		float azx1Cos = a.z * a.x * (1 - cosTheta);
		float axSin = a.x * sinTheta;
		float aySin = a.y * sinTheta;
		float azSin = a.z * sinTheta;
		mat[0][0] = ax2 + (1 - ax2) * cosTheta;
		mat[0][1] = axy1Cos - azSin;
		mat[0][2] = azx1Cos + aySin;
		mat[0][3] = 0;
		mat[1][0] = axy1Cos + azSin;
		mat[1][1] = ay2 + (1 - ay2) * cosTheta;
		mat[1][2] = ayz1Cos - axSin;
		mat[1][3] = 0;
		mat[2][0] = azx1Cos - aySin;
		mat[2][1] = ayz1Cos + axSin;
		mat[2][2] = az2 + (1 - az2) * cosTheta;
		mat[2][3] = 0;
		return Transform(mat);
	}

	Transform lookAt(const Vector3f& pos, const Vector3f& look, const Vector3f& up) {
		Matrix4x4 mat;
		mat[0][3] = pos.x;
		mat[1][3] = pos.y;
		mat[2][3] = pos.z;
		mat[3][3] = 1;

		Vector3f dir = normalize(look - pos);
		Vector3f right = normalize(cross(normalize(up), dir));
		Vector3f newUp = cross(dir, right);
		mat[0][0] = right.x;
		mat[1][0] = right.y;
		mat[2][0] = right.z;
		mat[3][0] = 0.;
		mat[0][1] = newUp.x;
		mat[1][1] = newUp.y;
		mat[2][1] = newUp.z;
		mat[3][1] = 0.;
		mat[0][2] = dir.x;
		mat[1][2] = dir.y;
		mat[2][2] = dir.z;
		mat[3][2] = 0.;

		return Transform(mat);
	}

	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------

}