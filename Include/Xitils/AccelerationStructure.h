#pragma once

#include "Utils.h"
#include "Geometry.h"
#include "Primitive.h"
#include "Interaction.h"
#include "Primitive.h"
#include "Object.h"
#include "Shape.h"

namespace Xitils {

	class _NaiveAccelerationStructure;
	class _BVH;

	using AccelerationStructure = _BVH;

	//---------------------------------------------------

	class _AccelerationStructure {
	public:
		virtual bool intersect(Ray& ray, SurfaceInteraction *isect) const = 0;
		virtual bool intersectAny(const Ray& ray) const {
			Ray rayTmp(ray);
			SurfaceInteraction isect;
			return intersect(rayTmp, &isect);
		}
	};

	class _NaiveAccelerationStructure : public _AccelerationStructure {
	public:
		_NaiveAccelerationStructure(const std::vector<Primitive*>& primitives) {
			map<Primitive*, Geometry*>(primitives, &geometries, [](const Geometry* primitive) { return (Geometry*)primitive; });
		}
		_NaiveAccelerationStructure(const std::vector<Object*>& objects) {
			map<Object*, Geometry*>(objects, &geometries, [](const Geometry* object) { return (Geometry*)object; });
		}
		_NaiveAccelerationStructure(const std::vector<Shape*>& shapes) {
			map<Shape*, Geometry*>(shapes, &geometries, [](const Geometry* shape) { return (Geometry*)shape; });
		}

		bool intersect(Ray& ray, SurfaceInteraction *isect) const override {
			bool hit = false;
			for(const auto& prim : geometries) {
				if (prim->intersect(ray, &ray.tMax, isect)) {
					hit = true;
				}
			}
			return hit;
		}

	private:
		std::vector<Geometry*> geometries;
	};

	struct BVHNode {
		~BVHNode() {
		}
		int depth;
		Bounds3f aabb;
		BVHNode* parent;
		int axis;
		const Geometry* geometry;
		BVHNode* children[2];
	};

	class _BVH : public _AccelerationStructure {
	public:
		_BVH(const std::vector<Primitive*>& primitives) {
			buildBVH((const Geometry**)primitives.data(), primitives.size());
		}

		_BVH(const std::vector<Object*>& objects) {
			buildBVH((const Geometry**)objects.data(), objects.size());
		}

		_BVH(const std::vector<Shape*>& shapes) {
			buildBVH((const Geometry**)shapes.data(), shapes.size());
		}

		~_BVH() {
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
			const Geometry* geometry;
			Bounds3f bound;
			Vector3f center;

			PrimBounds(const Geometry* geometry, const Bounds3f& bound, const Vector3f& center):
				geometry(geometry),
				bound(bound),
				center(center)
			{}
		};

		bool intersectSub(Ray& ray, SurfaceInteraction* isect, BVHNode* node) const {
			if (!node->aabb.intersect(ray, nullptr, nullptr)) {
				return false;
			}

			if (node->geometry) {
				return node->geometry->intersect(ray, &ray.tMax, isect);
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

			if (node->geometry) {
				return node->geometry->intersectAny(ray);
			}

			bool hit0, hit1;
			if (ray.d[node->axis] > 0) {
				hit0 = intersectAnySub(ray, node->children[0]);
				hit1 = intersectAnySub(ray, node->children[1]);
			} else {
				hit1 = intersectAnySub(ray, node->children[1]);
				hit0 = intersectAnySub(ray, node->children[0]);
			}
			return hit0 || hit1;
		}

		struct BucketComputation {
			std::vector<Bounds3f> bucketBounds, bucketBounds1, bucketBounds2;
			std::vector<int> bucketSizes, bucketSizes1, bucketSizes2;
		};

		void buildBVH(const Geometry** data, int primNum) {
			ASSERT(primNum > 0);
			std::vector<PrimBounds> aabbPrimitives;
			aabbPrimitives.reserve(primNum);
			for (int i = 0; i < primNum; ++i) {
				const Geometry* prim = data[i];
				Bounds3f bound = prim->bound();
				aabbPrimitives.push_back(PrimBounds(prim, bound, bound.center()));
			}

			int treeDepth = ceil(log2((float)primNum));
			nodes = (BVHNode*)calloc(pow(2, treeDepth + 1), sizeof(BVHNode));

			nodeRoot = &nodes[nodeCount++];
			nodeRoot->depth = 0;

			nodeRoot->aabb = Bounds3f();
			for (auto it = aabbPrimitives.begin(); it != aabbPrimitives.end(); ++it) {
				nodeRoot->aabb = merge(nodeRoot->aabb, it->bound);
			}

			BucketComputation bucket;

			buildBVHSub(aabbPrimitives.begin(), aabbPrimitives.end(), nodeRoot, bucket);
		}

		void buildBVHSub(typename std::vector<PrimBounds>::iterator begin, const typename std::vector<PrimBounds>::iterator& end, BVHNode* node, BucketComputation& bucket, int depth = 0, const Bounds3f& aabb = Bounds3f()) {

			if (end - begin == 1) {
				node->geometry = begin->geometry;
				node->aabb = begin->bound;
				return;
			}

			if (depth != 0) {
				node->aabb = aabb;
			}

			int axis = node->aabb.size().maxDimension();
			node->axis = axis;
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
			node->children[0]->parent = node;
			node->children[1]->parent = node;

			buildBVHSub(begin, begin + splitIndex, node->children[0], bucket, depth + 1, std::move(splitAABB1));
			buildBVHSub(begin + splitIndex, end, node->children[1], bucket, depth + 1, std::move(splitAABB2));
		}

	};

}
