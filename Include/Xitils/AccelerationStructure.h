#pragma once

#include "Utils.h"
#include "Geometry.h"
#include "Primitive.h"
#include "Interaction.h"
#include "Primitive.h"
#include "Object.h"
#include "Shape.h"

namespace Xitils {

	class AccelerationStructure {
	public:
		virtual bool intersect(Ray& ray, SurfaceInteraction *isect) const = 0;
		virtual bool intersectAny(const Ray& ray) const {
			Ray rayTmp(ray);
			SurfaceInteraction isect;
			return intersect(rayTmp, &isect);
		}
	};

	class NaiveAccelerationStructure : public AccelerationStructure {
	public:
		NaiveAccelerationStructure(const std::vector<Primitive*>& primitives) : primitives(primitives) {}

		bool intersect(Ray& ray, SurfaceInteraction *isect) const override {
			bool hit = false;
			for(const auto& prim : primitives) {
				if (prim->intersect(ray, &ray.tMax, isect)) {
					hit = true;
				}
			}
			return hit;
		}

	private:
		std::vector<Primitive*> primitives;
	};

	struct BVHNode {
		~BVHNode() {
		}
		int depth;
		Bounds3f aabb;
		BVHNode* parent;
		int axis;
		const Primitive* primitive;
		BVHNode* children[2];
	};

	class BVH : public AccelerationStructure {
	public:
		BVH(const std::vector<Primitive*>& primitives) {
			buildBVH((const Primitive**)primitives.data(), primitives.size());
		}

		BVH(const std::vector<Object*>& objects) {
			buildBVH((const Primitive**)objects.data(), objects.size());
		}

		BVH(const std::vector<Shape*>& shapes) {
			buildBVH((const Primitive**)shapes.data(), shapes.size());
		}

		~BVH() {
			delete nodes;
		}

		bool intersect(Ray& ray, SurfaceInteraction* isect) const override {
			return intersectSub(ray, isect, nodeRoot);
		}

		bool intersectAny(const Ray& ray) const override {
			return intersectAnySub(ray, nodeRoot);
		}

	private:
		BVHNode* nodeRoot;
		BVHNode* nodes;
		int nodeCount = 0;

		struct PrimBounds {
			const Primitive* primitive;
			Bounds3f bound;
			Vector3f center;

			PrimBounds(const Primitive* primitive, const Bounds3f& bound, const Vector3f& center):
				primitive(primitive),
				bound(bound),
				center(center)
			{}
		};

		void buildBVH(const Primitive** data, int primNum) {
			ASSERT(primNum > 0);
			std::vector<PrimBounds> aabbPrimitives;
			aabbPrimitives.reserve(primNum);
			for (int i = 0; i < primNum; ++i) {
				const Primitive* prim = data[i];
				Bounds3f bound = prim->bound();
				aabbPrimitives.push_back(PrimBounds(prim, bound, bound.center()));
			}

			int treeDepth = cinder::log2ceil(primNum);
			nodes = (BVHNode*)calloc(pow(2, treeDepth + 1), sizeof(BVHNode));

			nodeRoot = &nodes[nodeCount++];
			nodeRoot->depth = 0;
			nodeRoot->axis = -1;

			nodeRoot->aabb = Bounds3f();
			for (auto it = aabbPrimitives.begin(); it != aabbPrimitives.end(); ++it) {
				nodeRoot->aabb = merge(nodeRoot->aabb, it->bound);
			}

			buildBVH(aabbPrimitives.begin(), aabbPrimitives.end(), nodeRoot);
		}

		bool intersectSub(Ray& ray, SurfaceInteraction* isect, BVHNode* node) const {
			if (!node->aabb.intersect(ray, nullptr, nullptr)) {
				return false;
			}

			if (node->primitive) {
				return node->primitive->intersect(ray, &ray.tMax, isect);
			}

			bool hit0, hit1;
			if (ray.d[node->axis] > 0) {
				hit0 = intersectSub(ray, isect, node->children[0]);
				hit1 = intersectSub(ray, isect, node->children[1]);
			} else {
				hit1 = intersectSub(ray, isect, node->children[1]);
				hit0 = intersectSub(ray, isect, node->children[0]);
			}
			return hit0 || hit1;
		}

		bool intersectAnySub(const Ray& ray, BVHNode* node) const {
			if (!node->aabb.intersect(ray, nullptr, nullptr)) {
				return false;
			}

			if (node->primitive) {
				return node->primitive->intersectAny(ray);
			}

			bool hit0, hit1;
			if (ray.d[node->axis] > 0) {
				hit0 = intersectAnySub(ray, node->children[0]);
				hit1 = intersectAnySub(ray, node->children[1]);
			} else {
				hit1 = intersectAnySub(ray, node->children[1]);
				hit0 = intersectAnySub(ray, node->children[0]);
			}
		}

		void buildBVH(typename std::vector<PrimBounds>::iterator begin, const typename std::vector<PrimBounds>::iterator& end, BVHNode* node, int depth = 0, const Bounds3f& aabb = Bounds3f()) {

			if (end - begin == 1) {
				node->primitive = begin->primitive;
				node->aabb = begin->bound;
				return;
			}

			if (depth != 0) {
				node->aabb = aabb;
			}

			int axis = node->aabb.size().maxDimension();
			std::sort(begin, end, [axis](const PrimBounds& a, const PrimBounds& b) {
				return a.center[axis] < b.center[axis];
				});

			int splitIndex;
			Bounds3f splitAABB1, splitAABB2;
			if ((int)(end - begin) <= 8) {
				const int primNum = (int)(end - begin);
				splitIndex = primNum / 2;

				splitAABB1 = Bounds3f();
				for (int i = 0; i < splitIndex; ++i) { splitAABB1 = merge(splitAABB1, (begin + i)->bound); }
				splitAABB2 = Bounds3f();
				for (int i = splitIndex; i < primNum; ++i) { splitAABB2 = merge(splitAABB2, (begin + i)->bound); }

			}else{
				float bestSAH = Infinity;
				const int primNum = (int)(end - begin);

				auto calcSAH = [](const Bounds3f& aabb, const Bounds3f& aabb1, const Bounds3f& aabb2, int objNum1, int objNum2) {
					return  aabb1.surfaceArea() * objNum1 + aabb2.surfaceArea() * objNum2;
				};

				const int BucketNumMax = 16;

				const int bucketNum = min(BucketNumMax, primNum);
				std::vector<int> indices(bucketNum + 1);
				for (int i = 0; i <= bucketNum; ++i) {
					indices[i] = primNum * i / bucketNum;
				}

				std::vector<Bounds3f> bucketBounds1(bucketNum);
				std::vector<int> bucketCounts1(bucketNum, 0);
				for (int i = 1; i < bucketNum; ++i) {
					bucketBounds1[i] = bucketBounds1[i - 1];
					bucketCounts1[i] = bucketCounts1[i - 1];
					int K = indices[i] - indices[i - 1];
					auto bucketOffset = begin + indices[i - 1];
					for (int k = 0; k < K; ++k) {
						bucketBounds1[i] = merge(bucketBounds1[i], (bucketOffset + k)->bound);
					}
					bucketCounts1[i] += K;
				}

				std::vector<Bounds3f> bucketBounds2(bucketNum);
				std::vector<int> bucketCounts2(bucketNum, 0);
				for (int i = 1; i < bucketNum; ++i) {
					bucketBounds2[i] = bucketBounds2[i - 1];
					bucketCounts2[i] = bucketCounts2[i - 1];
					int K = indices[bucketNum - i + 1] - indices[bucketNum - i];
					auto bucketOffset = begin + indices[bucketNum - i];
					for (int k = 0; k < K; ++k) {
						bucketBounds2[i] = merge(bucketBounds2[i], (bucketOffset + k)->bound);
					}
					bucketCounts2[i] += K;
				}

				int bucketIndex;
				for (int i = 1; i < bucketNum; i++) {
					float sah = calcSAH(aabb, bucketBounds1[i], bucketBounds2[bucketNum - i], bucketCounts1[i], bucketCounts2[bucketNum - i]);
					if (sah < bestSAH) {
						splitIndex = bucketCounts1[i];
						bucketIndex = i;
						bestSAH = sah;
					}
				}
				splitAABB1 = bucketBounds1[bucketIndex];
				splitAABB2 = bucketBounds2[bucketNum - bucketIndex];
			}

			node->children[0] = &nodes[nodeCount++];
			node->children[1] = &nodes[nodeCount++];
			node->children[0]->depth = node->depth + 1;
			node->children[1]->depth = node->depth + 1;
			node->children[0]->axis = axis;
			node->children[1]->axis = axis;
			node->children[0]->parent = node;
			node->children[1]->parent = node;

			buildBVH(begin, begin + splitIndex, node->children[0], depth + 1, splitAABB1);
			buildBVH(begin + splitIndex, end, node->children[1], depth + 1, splitAABB2);
		}

	};

}
