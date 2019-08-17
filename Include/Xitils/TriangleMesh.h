#pragma once

#include "Shape.h"
#include "AccelerationStructure.h"

namespace Xitils {

	class TriangleMesh : public Shape {
	public:

		TriangleMesh(const Transform& objectToWorld) :
			Shape(objectToWorld)
		{}

		~TriangleMesh(){
			for (auto* tri : triangles) {
				delete tri;
			}
		}

		void setGeometry(const Vector3f* positionData, int positionNum, int* indexData, int indexNum) {
			setPositions(positionData, positionNum);
			setIndices(indexData, indexNum);
			buildBVH();
		}

		void setGeometry(const Vector3f* positionData, int positionNum, const Vector3f* normalData, int normalNum, int* indexData, int indexNum) {
			setPositions(positionData, positionNum);
			setNormals(normalData, normalNum);
			setIndices(indexData, indexNum);
			buildBVH();
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
			float res = 0.0f;
			for (const auto& tri : triangles) {
				res += tri->surfaceArea();
			}
			return res;
		}

		bool intersect(const Ray& ray, float* tHit, SurfaceInteraction* isect) const override {
			Ray rayObj = objectToWorld.inverse(ray);
			if (bvh->intersect(rayObj, isect)) {
				isect->p = objectToWorld(isect->p);
				isect->n = objectToWorld.asNormal(isect->n);
				*tHit = (isect->p - ray.o).length();
				return true;
			}
			return false;
		}

		bool intersectBool(const Ray& ray) const override {
			Ray rayObj = objectToWorld.inverse(ray);
			return bvh->intersectBool(rayObj);
		}

	private:
		std::vector<Vector3f> positions;
		std::vector<Vector3f> normals;
		std::vector<int> indices;
		std::vector<ShapeLocal*> triangles;

		std::unique_ptr<AccelerationStructure> bvh;

		void buildBVH() {
			int faceNum = indices.size() / 3;

			triangles.clear();
			triangles.resize(faceNum);
			
			for (int i = 0; i < faceNum; ++i) {
				triangles[i] = new TriangleIndexed(positions.data(), !normals.empty() ? normals.data() : nullptr, indices.data(), i);
			}

			bvh = std::make_unique<BVHLocal>(triangles);
		}

		void setPositions(const Vector3f* data, int num){
			positions.clear();
			positions.resize(num);
			memcpy(positions.data(), data, sizeof(Vector3f) * num);
		}

		void setNormals(const Vector3f* data, int num) {
			normals.clear();
			normals.resize(num);
			memcpy(normals.data(), data, sizeof(Vector3f) * num);
		}

		void setIndices(int* data, int num) {
			indices.clear();
			indices.resize(num);
			memcpy(indices.data(), data, sizeof(int) * num);
		}

	};

}