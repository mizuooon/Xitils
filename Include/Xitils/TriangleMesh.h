#pragma once

#include "Shape.h"
#include "AccelerationStructure.h"

namespace Xitils {

	class TriangleMesh : public Shape {
	public:

		~TriangleMesh(){
			for (auto* tri : triangles) {
				delete tri;
			}
		}

		void setGeometry(const std::shared_ptr<const cinder::TriMesh>& mesh, const std::shared_ptr<Texture> displacement = nullptr, float displacemntMax =0.0f) {

			std::shared_ptr <cinder::TriMesh> tmpMesh((cinder::TriMesh*)mesh->clone());

			if (displacement) {
				for (int i = 0; i < tmpMesh->getNumVertices(); ++i) {
					tmpMesh->getPositions<3>()[i] += 
						displacemntMax * displacement->rgb(Vector2f(tmpMesh->getTexCoords0<2>()[i])).x * tmpMesh->getNormals()[i];
				}
				tmpMesh->recalculateNormals();
			}

			positions.resize(tmpMesh->getNumVertices());
			for (int i = 0; i < positions.size(); ++i) {
				positions[i] = Vector3f(tmpMesh->getPositions<3>()[i]);
			}
			normals.resize(tmpMesh->getNumVertices());
			for (int i = 0; i < normals.size(); ++i) {
				normals[i] = Vector3f(tmpMesh->getNormals()[i]);
			}
			texCoords.resize(tmpMesh->getNumVertices());
			for (int i = 0; i < texCoords.size(); ++i) {
				texCoords[i] = Vector2f(tmpMesh->getTexCoords0<2>()[i]);
			}

			//tmpMesh->recalculateTangents();
			//tmpMesh->recalculateBitangents();

			//tangents.resize(tmpMesh->getTangents().size());
			//for (int i = 0; i < tangents.size(); ++i) {
			//	tangents[i] = Vector3f(tmpMesh->getTangents()[i]);
			//}
			//bitangents.resize(tmpMesh->getBitangents().size());
			//for (int i = 0; i < bitangents.size(); ++i) {
			//	bitangents[i] = Vector3f(tmpMesh->getBitangents()[i]);
			//}
			
			indices.resize(tmpMesh->getNumTriangles() * 3);
			for (int i = 0; i < indices.size(); i += 3) {
				indices[i + 0] = tmpMesh->getIndices()[i + 0];
				indices[i + 1] = tmpMesh->getIndices()[i + 1];
				indices[i + 2] = tmpMesh->getIndices()[i + 2];
			}

			buildBVH();
			calcSurfaceArea();
		}

		void setGeometry(const Vector3f* positionData, int positionNum, int* indexData, int indexNum) {
			setPositions(positionData, positionNum);
			setIndices(indexData, indexNum);
			buildBVH();
			calcSurfaceArea();
		}

		void setGeometry(const Vector3f* positionData, int positionNum, const Vector3f* normalData, int normalNum, int* indexData, int indexNum) {
			setPositions(positionData, positionNum);
			setNormals(normalData, normalNum);
			setIndices(indexData, indexNum);
			buildBVH();
			calcSurfaceArea();
		}

		int triangleNum() const { return triangles.size(); }

		Bounds3f bound() const override {
			Bounds3f res;
			for (const auto& p : positions) {
				res = merge(res, p);
			}
			return res;
		}

		float surfaceArea() const override {
			return area;
		}

		float surfaceAreaScaling(const Transform& t)  const override {
			// TODO: 高速化
			float scaledArea = 0.0f;
			for (const auto& tri : triangles) {
				scaledArea += tri->surfaceArea() * tri->surfaceAreaScaling(t);
			}
			return scaledArea / area;
		}

		bool intersect(const Ray& ray, float* tHit, SurfaceInteraction* isect) const override {
			Ray rayTmp = Ray(ray);
			if (accel->intersect(rayTmp, isect)) {
				isect->shape = this;
				*tHit = rayTmp.tMax;
				return true;
			}
			return false;
		}

		bool intersectAny(const Ray& ray) const override {
			return accel->intersectAny(ray);
		}

		SampledSurface sampleSurface(const std::shared_ptr<Sampler>& sampler, float* pdf) const override {
			// TODO: すべての Triangle から等確率でサンプリングしているが、
			//       本当は面積に比例した確率で選ぶべき
			auto& tri = sampler->select(triangles);
			auto sampled = tri->sampleSurface(sampler, pdf);
			*pdf /= triangleNum();

			SampledSurface res;
			res.shape = this;
			res.primitive = sampled.primitive;
			res.p = sampled.p;
			res.n = sampled.n;
			res.shadingN = sampled.shadingN;
			return res;
		}

		float surfacePDF(const Vector3f& p, const Primitive* primitive) const override {
			// TODO: 上記を直したらこちらも直す
			return primitive->surfacePDF(p) / triangleNum();
		}

	private:
		std::vector<Vector3f> positions;
		std::vector<Vector3f> normals;
		std::vector<Vector2f> texCoords;
		std::vector<Vector3f> tangents;
		std::vector<Vector3f> bitangents;
		std::vector<int> indices;
		std::vector<Primitive*> triangles;

		std::unique_ptr<AccelerationStructure> accel;

		float area;

		void calcSurfaceArea() {
			area = 0.0f;
			for (const auto& tri : triangles) {
				area += tri->surfaceArea();
			}
		}

		void buildBVH() {
			int faceNum = indices.size() / 3;

			triangles.clear();
			triangles.resize(faceNum);
			
			for (int i = 0; i < faceNum; ++i) {
				triangles[i] =
					new TriangleIndexed(
						positions.data(),
						!texCoords.empty() ? texCoords.data() : nullptr,
						!normals.empty() ? normals.data() : nullptr,
						!tangents.empty() ? tangents.data() : nullptr,
						!bitangents.empty() ? bitangents.data() : nullptr,
						indices.data(), i);
			}

			accel = std::make_unique<AccelerationStructure>(triangles);
		}

		void setPositions(const Vector3f* data, int num){
			positions.clear();
			positions.resize(num);
			memcpy(positions.data(), data, sizeof(Vector3f) * num);
		}

		void setNormals(const Vector3f* data, int num) {
			texCoords.clear();
			texCoords.resize(num);
			memcpy(texCoords.data(), data, sizeof(Vector2f) * num);
		}

		void setTexCoordinates(const Vector2f* data, int num) {
			indices.clear();
			indices.resize(num);
			memcpy(indices.data(), data, sizeof(int) * num);
		}

		void setIndices(int* data, int num) {
			indices.clear();
			indices.resize(num);
			memcpy(indices.data(), data, sizeof(int) * num);
		}

	};

	class Cube : public TriangleMesh {
	public:

		Cube() {
			std::array<Vector3f,8> positions;
			for (int i = 0; i < 8; ++i) {
				positions[i] = Vector3f( i & 1 ? -0.5f : 0.5f,
										 i & 2 ? -0.5f : 0.5f,
									     i & 4 ? -0.5f : 0.5f);
			}
			std::array<int, 3 * 6 * 2> indices{
				0,1,2, 1,3,2,
				2,3,6, 3,7,6,
				0,4,1, 4,5,1,
				0,2,6, 0,6,4,
				1,5,3, 5,7,3,
				4,6,5, 6,7,5
			};

			setGeometry(positions.data(), positions.size(), indices.data(), indices.size());

		}

	};

	class Plane : public TriangleMesh {
	public:

		Plane() {
			std::array<Vector3f, 4> positions;
			for (int i = 0; i < 4; ++i) {
				positions[i] = Vector3f(i & 1 ? -0.5f : 0.5f,
					i & 2 ? -0.5f : 0.5f,
					0);
			}
			std::array<int, 3 * 2> indices{
				0,2,1, 1,2,3
			};

			setGeometry(positions.data(), positions.size(), indices.data(), indices.size());

		}

	};

}