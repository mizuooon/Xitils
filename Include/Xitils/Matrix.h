#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <optional>

#include "Utils.h"
#include "Vector.h"

namespace Xitils {

	class Matrix4x4 {
	public:

		float m[4][4];

		Matrix4x4():
			m{
				{ 1, 0, 0, 0 },
				{ 0, 1, 0, 0 },
				{ 0, 0, 1, 0 },
				{ 0, 0, 0, 1 }
			}
		{}

		Matrix4x4(const Matrix4x4& mat):
			m{
				{ mat.m[0][0], mat.m[0][1], mat.m[0][2], mat.m[0][3] },
				{ mat.m[1][0], mat.m[1][1], mat.m[1][2], mat.m[1][3] },
				{ mat.m[2][0], mat.m[2][1], mat.m[2][2], mat.m[2][3] },
				{ mat.m[3][0], mat.m[3][1], mat.m[3][2], mat.m[3][3] }
				}
		{ ASSERT(!hasNan()); }

		Matrix4x4(const float *m):
			m{
				{ m[0],  m[1],  m[2] , m[3] },
				{ m[4],  m[5],  m[6],  m[7] },
				{ m[8],  m[9],  m[10], m[11] },
				{ m[12], m[13], m[14], m[15] }
				}
		{ ASSERT(!hasNan()); }

		Matrix4x4(const float m[4][4]) :
			m{
				{ m[0][0], m[0][1], m[0][2], m[0][3] },
				{ m[1][0], m[1][1], m[1][2], m[1][3] },
				{ m[2][0], m[2][1], m[2][2], m[2][3] },
				{ m[3][0], m[3][1], m[3][2], m[3][3] }
			}
		{ ASSERT(!hasNan()); }

		explicit Matrix4x4(const glm::mat4x4 mat):
			m{
				{ mat[0][0], mat[0][1], mat[0][2], mat[0][3] },
				{ mat[1][0], mat[1][1], mat[1][2], mat[1][3] },
				{ mat[2][0], mat[2][1], mat[2][2], mat[2][3] },
				{ mat[3][0], mat[3][1], mat[3][2], mat[3][3] }
				}
		{}
		glm::mat4x4 glm() const {
			return glm::mat4x4({
				m[0][0], m[0][1], m[0][2], m[0][3],
				m[1][0], m[1][1], m[1][2], m[1][3],
				m[2][0], m[2][1], m[2][2], m[2][3],
				m[3][0], m[3][1], m[3][2], m[3][3]
				});
		}

		Matrix4x4& operator=(const Matrix4x4& mat) {
			auto a = m;
			const auto b = mat.m;
			a[0][0] = b[0][0]; a[0][1] = b[0][1]; a[0][2] = b[0][2]; a[0][3] = b[0][3];
			a[1][0] = b[1][0]; a[1][1] = b[1][1]; a[1][2] = b[1][2]; a[1][3] = b[1][3];
			a[2][0] = b[2][0]; a[2][1] = b[2][1]; a[2][2] = b[2][2]; a[2][3] = b[2][3];
			a[3][0] = b[3][0]; a[3][1] = b[3][1]; a[3][2] = b[3][2]; a[3][3] = b[3][3];
			ASSERT(!hasNan());
			return *this;
		}

		Matrix4x4(
			float m00, float m01, float m02, float m03,
			float m10, float m11, float m12, float m13,
			float m20, float m21, float m22, float m23,
			float m30, float m31, float m32, float m33
			) :
			m{  m00, m01, m02, m03,
				m10, m11, m12, m13,
				m20, m21, m22, m23,
				m30, m31, m32, m33
				}
		{}

		inline float* operator[](int i) {
			ASSERT(i >= 0 && i < 4);
			return m[i];
		}

		inline float* data() { return &m[0][0][0][0]; }

		Matrix4x4 operator+(const Matrix4x4& mat) const {
			const auto a = m;
			const auto b = mat.m;
			return Matrix4x4(
				a[0][0] + b[0][0], a[0][1] + b[0][1], a[0][2] + b[0][2], a[0][3] + b[0][3],
				a[1][0] + b[1][0], a[1][1] + b[1][1], a[1][2] + b[1][2], a[1][3] + b[1][3],
				a[2][0] + b[2][0], a[2][1] + b[2][1], a[2][2] + b[2][2], a[2][3] + b[2][3],
				a[3][0] + b[3][0], a[3][1] + b[3][1], a[3][2] + b[3][2], a[3][3] + b[3][3]
			);
		}

		Matrix4x4& operator+=(const Matrix4x4& mat) {
			auto a = m;
			const auto b = mat.m;
			a[0][0] += b[0][0]; a[0][1] += b[0][1]; a[0][2] += b[0][2]; a[0][3] += b[0][3];
			a[1][0] += b[1][0]; a[1][1] += b[1][1]; a[1][2] += b[1][2]; a[1][3] += b[1][3];
			a[2][0] += b[2][0]; a[2][1] += b[2][1]; a[2][2] += b[2][2]; a[2][3] += b[2][3];
			a[3][0] += b[3][0]; a[3][1] += b[3][1]; a[3][2] += b[3][2]; a[3][3] += b[3][3];
			return *this;
		}

		Matrix4x4 operator-(const Matrix4x4& mat) const {
			const auto a = m;
			const auto b = mat.m;
			return Matrix4x4(
				a[0][0] - b[0][0], a[0][1] - b[0][1], a[0][2] - b[0][2], a[0][3] - b[0][3],
				a[1][0] - b[1][0], a[1][1] - b[1][1], a[1][2] - b[1][2], a[1][3] - b[1][3],
				a[2][0] - b[2][0], a[2][1] - b[2][1], a[2][2] - b[2][2], a[2][3] - b[2][3],
				a[3][0] - b[3][0], a[3][1] - b[3][1], a[3][2] - b[3][2], a[3][3] - b[3][3]
			);
		}
		Matrix4x4& operator-=(const Matrix4x4& mat) {
			auto a = m;
			const auto b = mat.m;
			a[0][0] -= b[0][0];  a[0][1] -= b[0][1];  a[0][2] -= b[0][2];  a[0][3]  -= b[0][3];
			a[1][0] -= b[1][0];  a[1][1] -= b[1][1];  a[1][2] -= b[1][2];  a[1][3]  -= b[1][3];
			a[2][0] -= b[2][0];  a[2][1] -= b[2][1];  a[2][2] -= b[2][2];  a[2][3] -= b[2][3];
			a[3][0] -= b[3][0];  a[3][1] -= b[3][1];  a[3][2] -= b[3][2];  a[3][3] -= b[3][3];
			return *this;
		}

		Matrix4x4 operator+() const { return Matrix4x4(m); }
		Matrix4x4 operator-() const {
			return Matrix4x4(
				-m[0][0], -m[0][1], -m[0][2], -m[0][3],
				-m[1][0], -m[1][1], -m[1][2], -m[1][3],
				-m[2][0], -m[2][1], -m[2][2], -m[2][3],
				-m[3][0], -m[3][1], -m[3][2], -m[3][3]
			);
		}


		Matrix4x4 operator*(float val) const {
			return Matrix4x4(
				val * m[0][0], val * m[0][1], val * m[0][2], val * m[0][3],
				val * m[1][0], val * m[1][1], val * m[1][2], val * m[1][3],
				val * m[2][0], val * m[2][1], val * m[2][2], val * m[2][3],
				val * m[3][0], val * m[3][1], val * m[3][2], val * m[3][3]
				);
		}

		Matrix4x4 operator*=(float val) {
			m[0][0] *= val;  m[0][1] *= val;  m[0][2] *= val;  m[0][3] *= val;
			m[1][0] *= val;  m[1][1] *= val;  m[1][2] *= val;  m[1][3] *= val;
			m[2][0] *= val;  m[2][1] *= val;  m[2][2] *= val;  m[2][3] *= val;
			m[3][0] *= val;  m[3][1] *= val;  m[3][2] *= val;  m[3][3] *= val;
			return *this;
		}

		Matrix4x4 operator/(float val) const {
			ASSERT(val != 0);
			float inv = (float)1 / val;
			return (*this) * inv;
		}

		Matrix4x4 operator/=(float val) {
			ASSERT(val != 0);
			float inv = (float)1 / val;
			*this *= inv;
			return *this;
		}

		Matrix4x4 operator*(const Matrix4x4& mat) const {
			const auto a = m;
			const auto b = mat.m;
			return Matrix4x4(
				a[0][0] * b[0][0] + a[0][1] * b[1][0] + a[0][2] * b[2][0] + a[0][3] * b[3][0],   a[0][0] * b[0][1] + a[0][1] * b[1][1] + a[0][2] * b[2][1] + a[0][3] * b[3][1],   a[0][0] * b[0][2] + a[0][1] * b[1][2] + a[0][2] * b[2][2] + a[0][3] * b[3][2],   a[0][0] * b[0][3] + a[0][1] * b[1][3] + a[0][2] * b[2][3] + a[0][3] * b[3][3],
				a[1][0] * b[0][0] + a[1][1] * b[1][0] + a[1][2] * b[2][0] + a[1][3] * b[3][0],   a[1][0] * b[0][1] + a[1][1] * b[1][1] + a[1][2] * b[2][1] + a[1][3] * b[3][1],   a[1][0] * b[0][2] + a[1][1] * b[1][2] + a[1][2] * b[2][2] + a[1][3] * b[3][2],   a[1][0] * b[0][3] + a[1][1] * b[1][3] + a[1][2] * b[2][3] + a[1][3] * b[3][3],
				a[2][0] * b[0][0] + a[2][1] * b[1][0] + a[2][2] * b[2][0] + a[2][3] * b[3][0],   a[2][0] * b[0][1] + a[2][1] * b[1][1] + a[2][2] * b[2][1] + a[2][3] * b[3][1],   a[2][0] * b[0][2] + a[2][1] * b[1][2] + a[2][2] * b[2][2] + a[2][3] * b[3][2],   a[2][0] * b[0][3] + a[2][1] * b[1][3] + a[2][2] * b[2][3] + a[2][3] * b[3][3],
				a[3][0] * b[0][0] + a[3][1] * b[1][0] + a[3][2] * b[2][0] + a[3][3] * b[3][0],   a[3][0] * b[0][1] + a[3][1] * b[1][1] + a[3][2] * b[2][1] + a[3][3] * b[3][1],   a[3][0] * b[0][2] + a[3][1] * b[1][2] + a[3][2] * b[2][2] + a[3][3] * b[3][2],   a[3][0] * b[0][3] + a[3][1] * b[1][3] + a[3][2] * b[2][3] + a[3][3] * b[3][3]
				);
		}
		Matrix4x4& operator*=(const Matrix4x4& mat) {
			auto a = m;
			const auto b = mat.m;
				a[0][0] = a[0][0] * b[0][0] + a[0][1] * b[1][0] + a[0][2] * b[2][0] + a[0][3] * b[3][0];   a[0][1] = a[0][0] * b[0][1] + a[0][1] * b[1][1] + a[0][2] * b[2][1] + a[0][3] * b[3][1];   a[0][2] = a[0][0] * b[0][2] + a[0][1] * b[1][2] + a[0][2] * b[2][2] + a[0][3] * b[3][2];   a[0][3] = a[0][0] * b[0][3] + a[0][1] * b[1][3] + a[0][2] * b[2][3] + a[0][3] * b[3][3];
				a[1][0] = a[1][0] * b[0][0] + a[1][1] * b[1][0] + a[1][2] * b[2][0] + a[1][3] * b[3][0];   a[1][1] = a[1][0] * b[0][1] + a[1][1] * b[1][1] + a[1][2] * b[2][1] + a[1][3] * b[3][1];   a[1][2] = a[1][0] * b[0][2] + a[1][1] * b[1][2] + a[1][2] * b[2][2] + a[1][3] * b[3][2];   a[1][3] = a[1][0] * b[0][3] + a[1][1] * b[1][3] + a[1][2] * b[2][3] + a[1][3] * b[3][3];
				a[2][0] = a[2][0] * b[0][0] + a[2][1] * b[1][0] + a[2][2] * b[2][0] + a[2][3] * b[3][0];   a[2][1] = a[2][0] * b[0][1] + a[2][1] * b[1][1] + a[2][2] * b[2][1] + a[2][3] * b[3][1];   a[2][2] = a[2][0] * b[0][2] + a[2][1] * b[1][2] + a[2][2] * b[2][2] + a[2][3] * b[3][2];   a[2][3] = a[2][0] * b[0][3] + a[2][1] * b[1][3] + a[2][2] * b[2][3] + a[2][3] * b[3][3];
				a[3][0] = a[3][0] * b[0][0] + a[3][1] * b[1][0] + a[3][2] * b[2][0] + a[3][3] * b[3][0];   a[3][1] = a[3][0] * b[0][1] + a[3][1] * b[1][1] + a[3][2] * b[2][1] + a[3][3] * b[3][1];   a[3][2] = a[3][0] * b[0][2] + a[3][1] * b[1][2] + a[3][2] * b[2][2] + a[3][3] * b[3][2];   a[3][3] = a[3][0] * b[0][3] + a[3][1] * b[1][3] + a[3][2] * b[2][3] + a[3][3] * b[3][3];
			return *this;
		}

		Vector4f operator*(const Vector4f& v) const {
			const auto a = m;
			const auto b = v.data();
			return Vector4f(
				m[0][0] * b[0] + m[0][1] * b[1] + m[0][2] * b[2] + m[0][3] * b[3],
				m[1][0] * b[0] + m[1][1] * b[1] + m[1][2] * b[2] + m[1][3] * b[3],
				m[2][0] * b[0] + m[2][1] * b[1] + m[2][2] * b[2] + m[2][3] * b[3],
				m[3][0] * b[0] + m[3][1] * b[1] + m[3][2] * b[2] + m[3][3] * b[3]
			);
		}

		bool operator==(const Matrix4x4& mat) const {
			auto a = m;
			const auto b = mat.m;
			return
				a[0][0] == b[0][0] && a[0][1] == b[0][1] && a[0][2] == b[0][2] && a[0][3] == b[0][3] &&
				a[1][0] == b[1][0] && a[1][1] == b[1][1] && a[1][2] == b[1][2] && a[1][3] == b[1][3] &&
				a[2][0] == b[2][0] && a[2][1] == b[2][1] && a[2][2] == b[2][2] && a[2][3] == b[2][3] &&
				a[3][0] == b[3][0] && a[3][1] == b[3][1] && a[3][2] == b[3][2] && a[3][3] == b[3][3] ;
		}

		bool operator!=(const Matrix4x4& mat) const {
			return !(*this == mat);
		}

		bool hasNan() {
			for (int i = 0; i < 16; ++i) {
				if (std::isnan(data()[i])) { return true; }
			}
			return false;
		}
		bool isZero() {
			for (int i = 0; i < 16; ++i) {
				if (data()[i] != 0.0f) { return false; }
			}
			return true;
		}

	};

	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------

	Matrix4x4 operator*(float val, const Matrix4x4& mat) { return mat * val; }

	Matrix4x4 transpose(const Matrix4x4& mat) {
		return Matrix4x4(
			mat.m[0][0], mat.m[1][0], mat.m[2][0],  mat.m[3][0],
			mat.m[0][1], mat.m[1][1], mat.m[2][1],  mat.m[3][1],
			mat.m[0][2], mat.m[1][2], mat.m[2][2], mat.m[3][2],
			mat.m[0][3], mat.m[1][3], mat.m[2][3], mat.m[3][3]);
	}

}