#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/norm.hpp>
#include <optional>

#include "Utils.h"
#include "Vector.h"

namespace Xitils {

	class Matrix4x4 {
	public:

		Matrix4x4():
			m({
				1, 0, 0, 0,
				0, 1, 0, 0,
				0, 0, 1, 0,
				0, 0, 0, 1
			})
		{}

		Matrix4x4(const Matrix4x4& mat)
			m({
				mat.m[0],  mat.m[1],  mat.m[2],  mat.m[3],
				mat.m[4],  mat.m[5],  mat.m[6],  mat.m[7],
				mat.m[8],  mat.m[9],  mat.m[10], mat.m[11],
				mat.m[12], mat.m[13], mat.m[14], mat.m[15]
				}) 
		{ ASSERT(!hasNan()); }

		Matrix4x4(const float *m) :
			m({
				m[0],  m[1],  m[2],  m[3],
				m[4],  m[5],  m[6],  m[7],
				m[8],  m[9],  m[10], m[11],
				m[12], m[13], m[14], m[15]
				})
		{ ASSERT(!hasNan()); }

		explicit Matrix4x4(const glm::mat4x4 mat) :
			m({
				mat[0][0], mat[0][1], mat[0][2], mat[0][3],
				mat[1][0], mat[1][1], mat[1][2], mat[1][3],
				mat[2][0], mat[2][1], mat[2][2], mat[2][3],
				mat[3][0], mat[3][1], mat[3][2], mat[3][3]
				})
		{}
		glm::mat4x4 glm() const {
			return glm::mat4x4({
				m[0],  m[1],  m[2],  m[3],
				m[4],  m[5],  m[6],  m[7],
				m[8],  m[9],  m[10], m[11],
				m[12], m[13], m[14], m[15]
				});
		}

		Matrix4x4& operator=(const Matrix4x4& mat) {
			float* a = m;
			const float* b = mat.m;
			a[0]  = b[0];   a[1]  = b[1];   a[2]  = b[2];   a[3]  = b[3];
			a[4]  = b[4];   a[5]  = b[5];   a[6]  = b[6];   a[7]  = b[7];
			a[8]  = b[8];   a[9]  = b[9];   a[10] = b[10];  a[11] = b[11];
			a[12] = b[12];  a[13] = b[13];  a[14] = b[14];  a[15] = b[15];
			ASSERT(!hasNan());
			return *this;
		}

		inline float& operator[](int i) const {
			ASSERT(i >= 0 && i <= 15);
			return m[i];
		}

		Matrix4x4(
			float m00, float m01, float m02, float m03,
			float m10, float m11, float m12, float m13,
			float m20, float m21, float m22, float m23,
			float m30, float m31, float m32, float m33
			) :
			m({ m00, m01, m02, m03,
				m10, m11, m12, m13,
				m20, m21, m22, m23,
				m30, m31, m32, m33
				})
		{}

		Matrix4x4 operator+(const Matrix4x4& mat) const {
			const float* a = m;
			const float* b = mat.m;
			return Matrix4x4(
				a[0]  + b[0],  a[1]  + b[1],  a[2]  + b[2],  a[3]  + b[3],
				a[4]  + b[4],  a[5]  + b[5],  a[6]  + b[6],  a[7]  + b[7],
				a[8]  + b[8],  a[9]  + b[9],  a[10] + b[10], a[11] + b[11],
				a[12] + b[12], a[13] + b[13], a[14] + b[14], a[15] + b[15]
			);
		}

		Matrix4x4& operator+=(const Matrix4x4& mat) {
			float* a = m;
			const float* b = mat.m;
			a[0]  += b[0];  a[1]  += b[1];  a[2]  += b[2];  a[3]  += b[3];
			a[4]  += b[4];  a[5]  += b[5];  a[6]  += b[6];  a[7]  += b[7];
			a[8]  += b[8];  a[9]  += b[9];  a[10] += b[10]; a[11] += b[11];
			a[12] += b[12]; a[13] += b[13]; a[14] += b[14]; a[15] += b[15];
			return *this;
		}

		Matrix4x4 operator-(const Matrix4x4& mat) const {
			const float* a = m;
			const float* b = mat.m;
			return Matrix4x4(
				a[0]  - b[0],  a[1]  - b[1],  a[2] -  b[2],  a[3]  - b[3],
				a[4]  - b[4],  a[5]  - b[5],  a[6] -  b[6],  a[7]  - b[7],
				a[8]  - b[8],  a[9]  - b[9],  a[10] - b[10], a[11] - b[11],
				a[12] - b[12], a[13] - b[13], a[14] - b[14], a[15] - b[15]
			);
		}
		Matrix4x4& operator-=(const Matrix4x4& mat) {
			float* a = m;
			const float* b = mat.m;
			a[0]  -= b[0];   a[1]  -= b[1];   a[2]  -= b[2];   a[3]  -= b[3];
			a[4]  -= b[4];   a[5]  -= b[5];   a[6]  -= b[6];   a[7]  -= b[7];
			a[8]  -= b[8];   a[9]  -= b[9];   a[10] -= b[10];  a[11] -= b[11];
			a[12] -= b[12];  a[13] -= b[13];  a[14] -= b[14];  a[15] -= b[15];
			return *this;
		}

		Matrix4x4 operator+() const { return Matrix4x4(m); }
		Matrix4x4 operator-() const {
			return Matrix4x4(
				-m[0],  -m[1],  -m[2],  -m[3],
				-m[4],  -m[5],  -m[6],  -m[7],
				-m[8],  -m[9],  -m[10], -m[11],
				-m[12], -m[13], -m[14], -m[15]
			);
		}


		Matrix4x4 operator*(float val) const {
			return Matrix4x4(
				val * m[0],  val * m[1],  val * m[2],  val * m[3],
				val * m[4],  val * m[5],  val * m[6],  val * m[7],
				val * m[8],  val * m[9],  val * m[10], val * m[11],
				val * m[12], val * m[13], val * m[14], val * m[15]
				);
		}

		Matrix4x4 operator*=(float val) {
			m[0]  *= val;  m[1]  *= val;  m[2]  *= val;  m[3]  *= val;
			m[4]  *= val;  m[5]  *= val;  m[6]  *= val;  m[7]  *= val;
			m[8]  *= val;  m[9]  *= val;  m[10] *= val;  m[11] *= val;
			m[12] *= val;  m[13] *= val;  m[14] *= val;  m[15] *= val;
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
			const float* a = m;
			const float* b = mat.m;
			return Matrix4x4(
				a[0]  * b[0] + a[1]  * b[4] + a[2]  * b[8] + a[3]  * b[12],   a[0]  * b[1] + a[1]  * b[5] + a[2]  * b[9] + a[3]  * b[13],   a[0]  * b[2] + a[1]  * b[6] + a[2]  * b[10] + a[3]  * b[14],   a[0]  * b[3] + a[1]  * b[7] + a[2]  * b[11] + a[3]  * b[15],
				a[4]  * b[0] + a[5]  * b[4] + a[6]  * b[8] + a[7]  * b[12],   a[4]  * b[1] + a[5]  * b[5] + a[6]  * b[9] + a[7]  * b[13],   a[4]  * b[2] + a[5]  * b[6] + a[6]  * b[10] + a[7]  * b[14],   a[4]  * b[3] + a[5]  * b[7] + a[6]  * b[11] + a[7]  * b[15],
				a[8]  * b[0] + a[9]  * b[4] + a[10] * b[8] + a[11] * b[12],   a[8]  * b[1] + a[9]  * b[5] + a[10] * b[9] + a[11] * b[13],   a[8]  * b[2] + a[9]  * b[6] + a[10] * b[10] + a[11] * b[14],   a[8]  * b[3] + a[9]  * b[7] + a[10] * b[11] + a[11] * b[15],
				a[12] * b[0] + a[13] * b[4] + a[14] * b[8] + a[15] * b[12],   a[12] * b[1] + a[13] * b[5] + a[14] * b[9] + a[15] * b[13],   a[12] * b[2] + a[13] * b[6] + a[14] * b[10] + a[15] * b[14],   a[12] * b[3] + a[13] * b[7] + a[14] * b[11] + a[15] * b[15],
				);
		}
		Matrix4x4& operator*=(const Matrix4x4& mat) {
			const float* a = m;
			const float* b = mat.m;
				a[0]  = a[0]  * b[0] + a[1]  * b[4] + a[2]  * b[8] + a[3]  * b[12];   a[1]  = a[0]  * b[1] + a[1]  * b[5] + a[2]  * b[9] + a[3]  * b[13];   a[2]  = a[0]  * b[2] + a[1]  * b[6] + a[2]  * b[10] + a[3]  * b[14];   a[3]  = a[0]  * b[3] + a[1]  * b[7] + a[2]  * b[11] + a[3]  * b[15];
				a[4]  = a[4]  * b[0] + a[5]  * b[4] + a[6]  * b[8] + a[7]  * b[12];   a[5]  = a[4]  * b[1] + a[5]  * b[5] + a[6]  * b[9] + a[7]  * b[13];   a[6]  = a[4]  * b[2] + a[5]  * b[6] + a[6]  * b[10] + a[7]  * b[14];   a[7]  = a[4]  * b[3] + a[5]  * b[7] + a[6]  * b[11] + a[7]  * b[15];
				a[8]  = a[8]  * b[0] + a[9]  * b[4] + a[10] * b[8] + a[11] * b[12];   a[9]  = a[8]  * b[1] + a[9]  * b[5] + a[10] * b[9] + a[11] * b[13];   a[10] = a[8]  * b[2] + a[9]  * b[6] + a[10] * b[10] + a[11] * b[14];   a[11] = a[8]  * b[3] + a[9]  * b[7] + a[10] * b[11] + a[11] * b[15];
				a[12] = a[12] * b[0] + a[13] * b[4] + a[14] * b[8] + a[15] * b[12];   a[13] = a[12] * b[1] + a[13] * b[5] + a[14] * b[9] + a[15] * b[13];   a[14] = a[12] * b[2] + a[13] * b[6] + a[14] * b[10] + a[15] * b[14];   a[15] = a[12] * b[3] + a[13] * b[7] + a[14] * b[11] + a[15] * b[15];
			return *this;
		}

		Vector4f operator*(const Vector4f& v) const {
			return Vector4f(
				m[0]  * v[0] + m[1]  * v[1] + m[2]  * v[2] + m[3]  * v[3],
				m[4]  * v[0] + m[5]  * v[1] + m[6]  * v[2] + m[7]  * v[3],
				m[8]  * v[0] + m[9]  * v[1] + m[10] * v[2] + m[11] * v[3],
				m[12] * v[0] + m[13] * v[1] + m[14] * v[2] + m[15] * v[3]
			);
		}

		bool operator==(const Matrix4x4& mat) const {
			const float* a = m;
			const float* b = mat.m;
			return
				a[0]  == b[0]  && a[1]  == b[1]  && a[2]  == b[2]  && a[3]  == b[3]  &&
				a[4]  == b[4]  && a[5]  == b[5]  && a[6]  == b[6]  && a[7]  == b[7]  &&
				a[8]  == b[8]  && a[9]  == b[9]  && a[10] == b[10] && a[11] == b[11] &&
				a[12] == b[12] && a[13] == b[13] && a[14] == b[14] && a[15] == b[15] ;
		}

		bool operator!=(const Matrix4x4& mat) const {
			return !(*this == mat);
		}

		bool hasNan() {
			for (int i = 0; i < 16; ++i) {
				if (std::isNan(m[i])) { return true; }
			}
			return false;
		}
		bool isZero() {
			for (int i = 0; i < 16; ++i) {
				if (m[i] != 0.0f) { return false; }
			}
			return true;
		}


		float m[16];

	};

	//--------------------------------------------------------------------------------------------------------------------------------------------------------------------

	Matrix4x4 operator*(float val, const Matrix4x4& mat) { return mat * val; }

}