#pragma once

#include <stack>

#include "Utils.h"
#include "Geometry.h"
#include "Shape.h"
#include "Object.h"
#include "Interaction.h"

namespace Xitils {

	class AccelerationStructure {
	public:
		virtual bool intersect(Ray& ray, SurfaceInteraction *isect) const = 0;
		virtual bool intersectBool(const Ray& ray) const {
			Ray rayTmp(ray);
			SurfaceInteraction isect;
			return intersect(rayTmp, &isect);
		}
	};

	template <typename T> class _NaiveAccelerationStructure : public AccelerationStructure {
	public:
		_NaiveAccelerationStructure(const std::vector<T*>& primitives) : primitives(primitives) {}

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
		std::vector<T*> primitives;
	};

	using NaiveAccelerationStructure = _NaiveAccelerationStructure<Object>;
	using NaiveAccelerationStructureLocal = _NaiveAccelerationStructure<Shape>;


	template <typename T> struct BVHNode {
		~BVHNode() {
		}
		int depth;
		Bounds3f aabb;
		BVHNode* parent;
		int localIndex; // 自身の親に対する子の中でのインデックス
		T* primitive;
		BVHNode* children[2];
	};

	template <typename T> class _BVH : public AccelerationStructure {
	public:
		using AABBPrimitive = std::pair<Bounds3f, T*>;

		_BVH(const std::vector<T*>& primitives) {
			ASSERT(!primitives.empty());
			std::vector<AABBPrimitive> aabbPrimitives;
			aabbPrimitives.reserve(primitives.size());
			for (auto& prim : primitives) { aabbPrimitives.push_back(std::make_pair(prim->bound(), prim)); }

			int treeDepth = cinder::log2ceil(primitives.size());
			nodes = (BVHNode<T>*)calloc( pow(2, treeDepth + 1), sizeof(BVHNode<T>));

			nodeRoot = &nodes[nodeCount++];
			nodeRoot->depth = 0;
			nodeRoot->localIndex = 0;
			buildBVH(aabbPrimitives.begin(), aabbPrimitives.end(), nodeRoot);
		}
		~_BVH() {
			delete nodes;
		}

		bool intersect(Ray& ray, SurfaceInteraction* isect) const override {
			return intersectSub(ray, isect, nodeRoot);
		}

		bool intersectBool(const Ray& ray) const override {
			return intersectBoolSub(ray, nodeRoot);
		}

	private:
		BVHNode<T>* nodeRoot;
		BVHNode<T>* nodes;
		int nodeCount = 0;

		bool intersectSub(Ray& ray, SurfaceInteraction* isect, BVHNode<T>* node) const {
			if (!node->aabb.intersect(ray, nullptr, nullptr)) {
				return false;
			}

			if (node->primitive) {
				return node->primitive->intersect(ray, &ray.tMax, isect);
			}

			bool hit0 = intersectSub(ray, isect, node->children[0]);
			bool hit1 = intersectSub(ray, isect, node->children[1]);
			return hit0 || hit1;
		}

		bool intersectBoolSub(const Ray& ray, BVHNode<T>* node) const {
			if (!node->aabb.intersect(ray, nullptr, nullptr)) {
				return false;
			}

			if (node->primitive) {
				return node->primitive->intersectBool(ray);
			}

			bool hit0 = intersectBoolSub(ray, node->children[0]);
			bool hit1 = intersectBoolSub(ray, node->children[1]);
			return hit0 || hit1;
		}

		void buildBVH(typename std::vector<AABBPrimitive>::iterator begin, const typename std::vector<AABBPrimitive>::iterator& end, BVHNode<T>* node, int depth = 0, std::optional<Bounds3f> aabb = std::nullopt) {

			if (depth == 0) {
				auto getAABB = [](const std::vector<AABBPrimitive>::const_iterator& begin, const std::vector<AABBPrimitive>::const_iterator& end) {
					Bounds3f aabb = begin->first;
					for (auto it = begin; it != end; it++) {
						aabb = merge(aabb, it->first);
					}
					return std::move(aabb);
				};
				node->aabb = getAABB(begin, end);
			} else {
				node->aabb = *aabb;
			}

			if (end - begin == 1) {
				node->primitive = begin->second;
				return;
			}

			int bestIndex;
			Bounds3f bestAABB1, bestAABB2;
			{
				int bestAxis = -1;
				float bestSAH;
				Bounds3f aabb1;
				std::vector<Bounds3f> aabb2;
				const int objNum = (int)(end - begin);

				std::vector<AABBPrimitive> xSortedObjs(begin, end);
				std::sort(xSortedObjs.begin(), xSortedObjs.end(), [](const AABBPrimitive& a, const AABBPrimitive& b) {
					return (a.first.min.x + a.first.max.x) < (b.first.min.x + b.first.max.x);
					});

				std::vector<AABBPrimitive> ySortedObjs(begin, end);
				std::sort(ySortedObjs.begin(), ySortedObjs.end(), [](const AABBPrimitive& a, const AABBPrimitive& b) {
					return (a.first.min.y + a.first.max.y) < (b.first.min.y + b.first.max.y);
					});

				std::vector<AABBPrimitive> zSortedObjs(begin, end);
				std::sort(zSortedObjs.begin(), zSortedObjs.end(), [](const AABBPrimitive& a, const AABBPrimitive& b) {
					return (a.first.min.z + a.first.max.z) < (b.first.min.z + b.first.max.z);
					});

				auto calcSAH = [](const Bounds3f& aabb, const Bounds3f& aabb1, const Bounds3f& aabb2, int objNum1, int objNum2) {
					//const float T_AABB = 1.0f;
					//const float T_tri = 1.0f;
					//float A_S = A(aabb);
					//return 2 * T_AABB + A(aabb1) / A_S * objNum1 * T_tri + A(aabb2) / A_S * objNum2 * T_tri;
					return  aabb1.surfaceArea() * objNum1 + aabb2.surfaceArea() * objNum2;
				};

				aabb2.reserve(objNum);

				aabb1 = xSortedObjs[0].first;
				aabb2.push_back(xSortedObjs.rbegin()->first);
				for (auto it = xSortedObjs.rbegin() + 1; it != xSortedObjs.rend() - 1; it++) { aabb2.push_back(merge(aabb2[aabb2.size() - 1], it->first)); }
				for (int i = 1; i < xSortedObjs.size(); i++) {
					aabb1 = merge(aabb1, xSortedObjs[i].first);
					float sah = calcSAH(*aabb, aabb1, aabb2[aabb2.size() - i], i, objNum - i);
					if (bestAxis == -1 || sah < bestSAH) {
						bestAxis = 0;
						bestIndex = i;
						bestSAH = sah;
						bestAABB1 = aabb1;
						bestAABB2 = aabb2[aabb2.size() - i];
					}
				}
				aabb2.clear();

				aabb1 = ySortedObjs[0].first;
				aabb2.push_back(ySortedObjs.rbegin()->first);
				for (auto it = ySortedObjs.rbegin() + 1; it != ySortedObjs.rend() - 1; it++) { aabb2.push_back(merge(aabb2[aabb2.size() - 1], it->first)); }
				for (int i = 1; i < ySortedObjs.size(); i++) {
					aabb1 = merge(aabb1, ySortedObjs[i].first);
					float sah = calcSAH(*aabb, aabb1, aabb2[aabb2.size() - i], i, objNum - i);
					if (bestAxis == -1 || sah < bestSAH) {
						bestAxis = 1;
						bestIndex = i;
						bestSAH = sah;
						bestAABB1 = aabb1;
						bestAABB2 = aabb2[aabb2.size() - i];
					}
				}
				aabb2.clear();

				aabb1 = zSortedObjs[0].first;
				aabb2.push_back(zSortedObjs.rbegin()->first);
				for (auto it = zSortedObjs.rbegin() + 1; it != zSortedObjs.rend() - 1; it++) { aabb2.push_back(merge(aabb2[aabb2.size() - 1], it->first)); }
				for (int i = 1; i < zSortedObjs.size(); i++) {
					aabb1 = merge(aabb1, zSortedObjs[i].first);
					float sah = calcSAH(*aabb, aabb1, aabb2[aabb2.size() - i], i, objNum - i);
					if (bestAxis == -1 || sah < bestSAH) {
						bestAxis = 2;
						bestIndex = i;
						bestSAH = sah;
						bestAABB1 = aabb1;
						bestAABB2 = aabb2[aabb2.size() - i];
					}
				}

				auto& bestSortedObjs = bestAxis == 0 ? xSortedObjs : (bestAxis == 1 ? ySortedObjs : zSortedObjs);
				std::move(bestSortedObjs.begin(), bestSortedObjs.end(), begin);
			}

			node->children[0] = &nodes[nodeCount++];
			node->children[1] = &nodes[nodeCount++];
			node->children[0]->depth = node->depth + 1;
			node->children[1]->depth = node->depth + 1;

			node->children[0]->parent = node;
			node->children[1]->parent = node;
			node->children[0]->localIndex = 0;
			node->children[1]->localIndex = 1;

			buildBVH(begin, begin + bestIndex, node->children[0], depth + 1, bestAABB1);
			buildBVH(begin + bestIndex, end, node->children[1], depth + 1, bestAABB2);
		}

	};

	using BVH = _BVH<Object>;
	using BVHLocal = _BVH<Shape>;

}
