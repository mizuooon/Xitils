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

		struct BucketComputation {
			std::vector<Bounds3f> bucketBounds, bucketBounds1, bucketBounds2;
			std::vector<int> bucketSizes, bucketSizes1, bucketSizes2;
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

			BucketComputation bucket;

			buildBVHSub(aabbPrimitives.begin(), aabbPrimitives.end(), nodeRoot, bucket);
		}

		void buildBVHSub(typename std::vector<PrimBounds>::iterator begin, const typename std::vector<PrimBounds>::iterator& end, BVHNode* node, BucketComputation& bucket, int depth = 0, const Bounds3f& aabb = Bounds3f()) {

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

				static auto calcSAH = [](const Bounds3f& aabb, const Bounds3f& aabb1, const Bounds3f& aabb2, int objNum1, int objNum2) {
					return  aabb1.surfaceArea() * objNum1 + aabb2.surfaceArea() * objNum2;
				};

				const int BucketNumMax = 8;

				bucket.bucketBounds.clear();
				bucket.bucketBounds1.clear();
				bucket.bucketBounds2.clear();
				bucket.bucketSizes.clear();
				bucket.bucketSizes1.clear();
				bucket.bucketSizes2.clear();

				const int bucketNum = min(BucketNumMax, primNum);

				int bucketBegin = 0;
				for (int i = 0; i < bucketNum; ++i) {
					Bounds3f bound;
					int bucketEnd = primNum * (i+1) / bucketNum;
					int bucketSize = bucketEnd - bucketBegin;
					auto bucketOffset = begin + bucketBegin;
					for (int k = 0; k < bucketSize; ++k) {
						bound = merge(bound, (bucketOffset + k)->bound);
					}
					bucket.bucketBounds.push_back(bound);
					bucket.bucketSizes.push_back(bucketSize);
					bucketBegin = bucketEnd;
				}

				for (int i = 0; i < bucketNum - 1; ++i) {
					bucket.bucketBounds1.push_back(bucket.bucketBounds[i]);
					bucket.bucketSizes1.push_back(bucket.bucketSizes[i]);
					if (i > 0) {
						bucket.bucketBounds1[i] = merge(bucket.bucketBounds1[i], bucket.bucketBounds1[i - 1]);
						bucket.bucketSizes1[i] += bucket.bucketSizes1[i - 1];
					}
				}

				for (int i = 0; i < bucketNum - 1; ++i) {
					bucket.bucketBounds2.push_back(bucket.bucketBounds[bucketNum - 1 - i]);
					bucket.bucketSizes2.push_back(bucket.bucketSizes[bucketNum - 1 - i]);
					if (i > 0) {
						bucket.bucketBounds2[i] = merge(bucket.bucketBounds2[i], bucket.bucketBounds2[i - 1]);
						bucket.bucketSizes2[i] += bucket.bucketSizes2[i - 1];
					}
				}

				int bucketIndex;
				for (int i = 0; i < bucketNum - 1; i++) {
					float sah = calcSAH(aabb, bucket.bucketBounds1[i], bucket.bucketBounds2[bucketNum - 2 - i], bucket.bucketSizes1[i], bucket.bucketSizes2[bucketNum - 2 - i]);
					if (sah < bestSAH) {
						splitIndex = bucket.bucketSizes1[i];
						bucketIndex = i;
						bestSAH = sah;
					}
				}
				splitAABB1 = bucket.bucketBounds1[bucketIndex];
				splitAABB2 = bucket.bucketBounds2[bucketNum - 2 - bucketIndex];
			}

			node->children[0] = &nodes[nodeCount++];
			node->children[1] = &nodes[nodeCount++];
			node->children[0]->depth = node->depth + 1;
			node->children[1]->depth = node->depth + 1;
			node->children[0]->axis = axis;
			node->children[1]->axis = axis;
			node->children[0]->parent = node;
			node->children[1]->parent = node;

			buildBVHSub(begin, begin + splitIndex, node->children[0], bucket, depth + 1, std::move(splitAABB1));
			buildBVHSub(begin + splitIndex, end, node->children[1], bucket, depth + 1, std::move(splitAABB2));
		}

	};

}
