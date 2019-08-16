#pragma once

#include <stack>

#include "Utils.h"
#include "Geometry.h"
#include "Shape.h"
#include "Interaction.h"

namespace Xitils {

	//class AccelerationStructure;
	//class BVH;

	//class AccelerationStructureIterator {
	//public:
	//	virtual ~AccelerationStructureIterator() {};
	//	virtual const Shape* operator*() const = 0;
	//	virtual void next() = 0;
	//	virtual bool end() const = 0;
	//};
	class AccelerationStructure {
	public:
		//virtual ~AccelerationStructure() {};
		//virtual std::unique_ptr<AccelerationStructureIterator> traverse(const Ray& ray) = 0;
		virtual bool intersect(Ray& ray, SurfaceInteraction *isect) const = 0;
		virtual bool intersectBool(const Ray& ray) const {
			Ray ray2(ray);
			SurfaceInteraction isect;
			return intersect(ray2, &isect);
		}
	};

	//class NaiveAccelerationStructureIterator : public AccelerationStructureIterator {
	//public:
	//	NaiveAccelerationStructureIterator(const std::vector<Shape*>& shapes) :
	//		iterator(shapes.begin()), end_iterator(shapes.end()) {}
	//	virtual const Shape* operator*() const { return *iterator; }
	//	virtual void next() { ++iterator; }
	//	virtual bool end() const { return iterator == end_iterator; }
	//private:
	//	typename std::vector<Shape*>::const_iterator iterator;
	//	typename std::vector<Shape*>::const_iterator end_iterator;
	//};
	class NaiveAccelerationStructure : public AccelerationStructure {
	public:
		NaiveAccelerationStructure(const std::vector<Shape*>& shapes) : shapes(shapes) {}
		//virtual std::unique_ptr<AccelerationStructureIterator> traverse(const Ray& ray) { return std::make_unique<NaiveAccelerationStructureIterator>(shapes); }

		bool intersect(Ray& ray, SurfaceInteraction *isect) const override {
			bool hit = false;
			for(const auto& shape : shapes) {
				if (shape->intersect(ray, &ray.tMax, isect)) {
					hit = true;
				}
			}
			return hit;
		}

	private:
		std::vector<Shape*> shapes;
	};


	struct BVHNode {
		~BVHNode() {
		}
		int depth;
		Bounds3f aabb;
		BVHNode* parent;
		int localIndex; // 自身の親に対する子の中でのインデックス
		Shape* shape;
		BVHNode* children[2];

		// 交差判定用に使用するオブジェクトを返すイテレータではなく
		// BVHNode を見たいとき用のイテレータ
		//template<typename T> class Iterator {
		//public:
		//	Iterator(BVHNod* node) {
		//		currentNode = node;
		//	}

		//	virtual BVHNode* operator*() const { return currentNode; }
		//	Iterator& next() {
		//		if (!currentNode->shape) {
		//			nodeStack.push(currentNode->children[1]);
		//			currentNode = currentNode->children[0];
		//		} else {
		//			if (!nodeStack.empty()) {
		//				currentNode = nodeStack.top();
		//				nodeStack.pop();
		//			} else {
		//				currentNode = nullptr;
		//			}
		//		}

		//		return *this;
		//	}
		//	bool end() { return currentNode == nullptr; }
		//private:
		//	BVHNode* currentNode;
		//	std::stack<BVHNode*> nodeStack;
		//};

	};

	//class BVHIterator : public AccelerationStructureIterator {
	//public:
	//	BVHIterator(BVHNode* node, const Ray& ray)
	//		: currentNode(node), ray(ray) {
	//		findNextShape();
	//	}
	//	virtual const Shape* operator*() const { return *currentNode->shape; }
	//	virtual void next() { findNextShape(); }
	//	virtual bool end() const { return currentNode == nullptr; }

	//private:

	//	void findNextShape() {
	//		if (currentNode->shape) {
	//			if (!nodeStack.empty()) {
	//				currentNode = nodeStack.top();
	//				nodeStack.pop();
	//			} else {
	//				currentNode = nullptr;
	//				return;
	//			}
	//		}

	//		while (!currentNode->shape) {
	//			if (currentNode->children[0]->aabb.intersect(*ray, nullptr, &ray->tMax)) {
	//				if (currentNode->children[1]->aabb.intersect(*ray, nullptr, &ray->tMax)) {
	//					nodeStack.push(currentNode->children[1]);
	//				}
	//				currentNode = currentNode->children[0];
	//			} else {
	//				if (currentNode->children[1]->aabb.intersect(*ray, nullptr, &ray->tMax)) {
	//					currentNode = currentNode->children[1];
	//				} else {
	//					if (!nodeStack.empty()) {
	//						currentNode = nodeStack.top();
	//						nodeStack.pop();
	//					} else {
	//						currentNode = nullptr;
	//						return;
	//					}
	//				}
	//			}
	//		}
	//	}

	//	BVHNode* currentNode;
	//	std::stack<BVHNode*> nodeStack;
	//	std::optional<Ray> ray;
	//};

	class BVH : public AccelerationStructure {
	public:
		using AABBShape = std::pair<Bounds3f, Shape*>;

		BVH(const std::vector<Shape*>& shapes) {
			ASSERT(!shapes.empty());
			std::vector<AABBShape> aabbShapes;
			aabbShapes.reserve(shapes.size());
			for (auto& shape : shapes) { aabbShapes.push_back(std::make_pair(shape->bound(), shape)); }

			int treeDepth = cinder::log2ceil(shapes.size());
			nodes = (BVHNode*)calloc( pow(2, treeDepth + 1), sizeof(BVHNode));

			nodeRoot = &nodes[nodeCount++];
			nodeRoot->depth = 0;
			nodeRoot->localIndex = 0;
			buildBVH(aabbShapes.begin(), aabbShapes.end(), nodeRoot);
		}
		~BVH() {
			delete nodes;
		}

		//virtual std::unique_ptr<AccelerationStructureIterator> traverse(const Ray& ray) { return std::make_unique<BVHIterator>(nodeRoot, ray); }

		// オブジェクトではなく BVHNode についてのイテレータ
		//std::unique_ptr<BVHNode<T>::Iterator<T>> getNodeIterator() const { return std::make_unique<BVHNode<T>::Iterator<T>>(nodeRoot); }

		bool intersect(Ray& ray, SurfaceInteraction* isect) const override {
			return intersectSub(ray, isect, nodeRoot);
		}

		bool intersectBool(const Ray& ray) const override {
			return intersectBoolSub(ray, nodeRoot);
		}

	private:
		BVHNode* nodeRoot;
		BVHNode* nodes;
		int nodeCount = 0;

		bool intersectSub(Ray& ray, SurfaceInteraction* isect, BVHNode* node) const {
			if (!node->aabb.intersect(ray, nullptr, nullptr)) {
				return false;
			}

			if (node->shape) {
				return node->shape->intersect(ray, &ray.tMax, isect);
			}

			bool hit0 = intersectSub(ray, isect, node->children[0]);
			bool hit1 = intersectSub(ray, isect, node->children[1]);
			return hit0 || hit1;
		}

		bool intersectBoolSub(const Ray& ray, BVHNode* node) const {
			if (!node->aabb.intersect(ray, nullptr, nullptr)) {
				return false;
			}

			if (node->shape) {
				return node->shape->intersectBool(ray);
			}

			bool hit0 = intersectBoolSub(ray, node->children[0]);
			bool hit1 = intersectBoolSub(ray, node->children[1]);
			return hit0 || hit1;
		}

		void buildBVH(typename std::vector<AABBShape>::iterator begin, const typename std::vector<AABBShape>::iterator& end, BVHNode* node, int depth = 0, std::optional<Bounds3f> aabb = std::nullopt) {

			if (depth == 0) {
				auto getAABB = [](const std::vector<AABBShape>::const_iterator& begin, const std::vector<AABBShape>::const_iterator& end) {
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
				node->shape = begin->second;
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

				std::vector<AABBShape> xSortedObjs(begin, end);
				std::sort(xSortedObjs.begin(), xSortedObjs.end(), [](const AABBShape& a, const AABBShape& b) {
					return (a.first.min.x + a.first.max.x) < (b.first.min.x + b.first.max.x);
					});

				std::vector<AABBShape> ySortedObjs(begin, end);
				std::sort(ySortedObjs.begin(), ySortedObjs.end(), [](const AABBShape& a, const AABBShape& b) {
					return (a.first.min.y + a.first.max.y) < (b.first.min.y + b.first.max.y);
					});

				std::vector<AABBShape> zSortedObjs(begin, end);
				std::sort(zSortedObjs.begin(), zSortedObjs.end(), [](const AABBShape& a, const AABBShape& b) {
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

}
