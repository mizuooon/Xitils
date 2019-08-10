#pragma once

#include <stack>

#include "Utils.h"
#include "Geometry.h"

namespace Xitils::Geometry {

	template<typename T> class AccelerationStructure;
	template <typename T> class BVH;

	template<typename T>
	class AccelerationStructureIterator {
	public:
		virtual ~AccelerationStructureIterator() {};
		virtual const T& operator*() const = 0;
		virtual AccelerationStructureIterator<T>& next() = 0;
		virtual bool end() const = 0;
		virtual void select(float t) {}
	};
	template<typename T>
	class AccelerationStructure {
	public:
		virtual ~AccelerationStructure() {};
		virtual std::unique_ptr<AccelerationStructureIterator<T>> traverse(const Ray& ray) = 0;
	};

	template<typename T>
	class NaiveAccelerationStructureIterator : public AccelerationStructureIterator<T> {
	public:
		NaiveAccelerationStructureIterator(const std::vector<T>& objects) :
			iterator(objects.begin()), end_iterator(objects.max()) {}
		virtual const T& operator*() const { return *iterator; }
		virtual AccelerationStructureIterator<T>& next() { ++iterator; return *this; }
		virtual bool end() const { return iterator == end_iterator; }
	private:
		typename std::vector<T>::const_iterator iterator;
		typename std::vector<T>::const_iterator end_iterator;
	};
	template<typename T>
	class NaiveAccelerationStructure : public AccelerationStructure<T> {
	public:
		NaiveAccelerationStructure(const std::vector<T>& objects) : objects(objects) {}
		virtual std::unique_ptr<AccelerationStructureIterator<T>> traverse(const Ray& ray) { return std::make_unique<NaiveAccelerationStructureIterator<T>>(objects); }
	private:
		std::vector<T> objects;
	};


	template <typename T>
	struct BVHNode {
		~BVHNode() {
		}
		int depth;
		AABB aabb;
		BVHNode<T>* parent;
		int localIndex; // 自身の親に対する子の中でのインデックス
		std::optional<T> object;
		BVHNode<T>* children[2];

		// 交差判定用に使用するオブジェクトを返すイテレータではなく
		// BVHNode を見たいとき用のイテレータ
		template<typename T> class Iterator {
		public:
			Iterator(BVHNode<T>* node) {
				currentNode = node;
			}

			virtual BVHNode<T>* operator*() const { return currentNode; }
			Iterator& next() {
				if (!currentNode->object) {
					nodeStack.push(currentNode->children[1]);
					currentNode = currentNode->children[0];
				} else {
					if (!nodeStack.empty()) {
						currentNode = nodeStack.top();
						nodeStack.pop();
					} else {
						currentNode = nullptr;
					}
				}

				return *this;
			}
			bool end() { return currentNode == nullptr; }
		private:
			BVHNode<T>* currentNode;
			std::stack<BVHNode<T>*> nodeStack;
		};

	};

	template <typename T>
	class BVHIterator : public AccelerationStructureIterator<T> {
	public:
		BVHIterator(BVHNode<T>* node, const Ray& ray)
			: currentNode(node), ray(ray) {
			findNextObject();
		}
		virtual const T& operator*() const { return *currentNode->object; }
		virtual AccelerationStructureIterator<T>& next() { findNextObject(); return *this; }
		virtual bool end() const { return currentNode == nullptr; }
		virtual void select(float t) { maxT = t; }

	private:

		void findNextObject() {
			if (currentNode->object) {
				if (!objStack.empty()) {
					currentNode = objStack.top();
					objStack.pop();
				} else {
					currentNode = nullptr;
					return;
				}
			}

			while (!currentNode->object) {
				auto intsct = Intersection::RayAABB::getIntersection(*ray, currentNode->children[0]->aabb);
				if (intsct && (!maxT || intsct->t < maxT)) {
					auto intsct = Intersection::RayAABB::getIntersection(*ray, currentNode->children[1]->aabb);
					if (intsct && (!maxT || intsct->t < maxT)) {
						objStack.push(currentNode->children[1]);
					}
					currentNode = currentNode->children[0];
				} else {
					auto intsct = Intersection::RayAABB::getIntersection(*ray, currentNode->children[1]->aabb);
					if (intsct && (!maxT || intsct->t < maxT)) {
						currentNode = currentNode->children[1];
					} else {
						if (!objStack.empty()) {
							currentNode = objStack.top();
							objStack.pop();
						} else {
							currentNode = nullptr;
							return;
						}
					}
				}
			}
		}

		BVHNode<T>* currentNode;
		std::stack<BVHNode<T>*> objStack;
		std::optional<Ray> ray;
		std::optional<float> maxT;
	};

	template <typename T>
	class BVH : public AccelerationStructure<T> {
	public:
		using AABBObj = std::pair<AABB, T>;

		BVH(const std::vector<T>& objects, std::function<AABB(const T&)> calcAABB) : calcAABB(calcAABB) {
			std::vector<AABBObj> aabbObjects;
			aabbObjects.reserve(objects.size());
			for (auto& obj : objects) { aabbObjects.push_back(std::make_pair(calcAABB(obj), obj)); }

			int treeDepth = log2ceil(objects.size());
			nodes = (BVHNode<T>*)malloc( sizeof(BVHNode<T>) * (pow(2, treeDepth + 1)) );

			nodeRoot = &nodes[nodeCount++];
			nodeRoot->depth = 0;
			nodeRoot->localIndex = 0;
			buildBVH(aabbObjects.begin(), aabbObjects.end(), nodeRoot);
		}
		~BVH() {
			delete nodes;
		}

		virtual std::unique_ptr<AccelerationStructureIterator<T>> traverse(const Ray& ray) { return std::make_unique<BVHIterator<T>>(nodeRoot, ray); }

		// オブジェクトではなく BVHNode についてのイテレータ
		std::unique_ptr<BVHNode<T>::Iterator<T>> getNodeIterator() const { return std::make_unique<BVHNode<T>::Iterator<T>>(nodeRoot); }

	private:
		BVHNode<T>* nodeRoot;
		BVHNode<T>* nodes;
		int nodeCount = 0;
		std::function<AABB(const T&)> calcAABB;

		void buildBVH(typename std::vector<AABBObj>::iterator begin, const typename std::vector<AABBObj>::iterator& end, BVHNode<T>* node, int depth = 0, std::optional<AABB> aabb = std::nullopt) {

			if (depth == 0) {
				auto getAABB = [](const std::vector<AABBObj>::const_iterator& begin, const std::vector<AABBObj>::const_iterator& end) {
					AABB aabb = begin->first;
					for (auto it = begin; it != end; it++) {
						aabb |= it->first;
					}
					return std::move(aabb);
				};
				node->aabb = getAABB(begin, end);
			} else {
				node->aabb = *aabb;
			}

			if (end - begin == 1) {
				node->object = begin->second;
				return;
			}

			int bestIndex;
			AABB bestAABB1, bestAABB2;
			{
				int bestAxis = -1;
				float bestSAH;
				AABB aabb1;
				std::vector<AABB> aabb2;
				const int objNum = (int)(end - begin);

				std::vector<AABBObj> xSortedObjs(begin, end);
				std::sort(xSortedObjs.begin(), xSortedObjs.end(), [](const AABBObj& a, const AABBObj& b) {
					return (a.first.min.x + a.first.max.x) < (b.first.min.x + b.first.max.x);
					});

				std::vector<AABBObj> ySortedObjs(begin, end);
				std::sort(ySortedObjs.begin(), ySortedObjs.end(), [](const AABBObj& a, const AABBObj& b) {
					return (a.first.min.y + a.first.max.y) < (b.first.min.y + b.first.max.y);
					});

				std::vector<AABBObj> zSortedObjs(begin, end);
				std::sort(zSortedObjs.begin(), zSortedObjs.end(), [](const AABBObj& a, const AABBObj& b) {
					return (a.first.min.z + a.first.max.z) < (b.first.min.z + b.first.max.z);
					});

				auto A = [](const AABB& aabb) {
					auto size = aabb.max - aabb.min;
					return 2.0f * (size.x * size.y + size.y * size.z + size.z * size.x);
					//if ( size.x < size.y && size.x < size.z ) {
					//	return size.y * size.z;
					//}
					//if ( size.y < size.z && size.y < size.x ) {
					//	return size.z * size.x;
					//}
					//return size.x * size.y;
				};
				auto calcSAH = [&A](const AABB& aabb, const AABB& aabb1, const AABB& aabb2, int objNum1, int objNum2) {
					const float T_AABB = 1.0f;
					//const float T_tri = 1.0f;
					//float A_S = A(aabb);
					//return 2 * T_AABB + A(aabb1) / A_S * objNum1 * T_tri + A(aabb2) / A_S * objNum2 * T_tri;
					return A(aabb1) * objNum1 + A(aabb2) * objNum2;
				};

				aabb2.reserve(objNum);

				aabb1 = calcAABB(xSortedObjs[0].second);
				aabb2.push_back(xSortedObjs.rbegin()->first);
				for (auto it = xSortedObjs.rbegin() + 1; it != xSortedObjs.rend() - 1; it++) { aabb2.push_back(aabb2[aabb2.size() - 1] | it->first); }
				for (int i = 1; i < xSortedObjs.size(); i++) {
					aabb1 |= xSortedObjs[i].first;
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

				aabb1 = calcAABB(ySortedObjs[0].second);
				aabb2.push_back(ySortedObjs.rbegin()->first);
				for (auto it = ySortedObjs.rbegin() + 1; it != ySortedObjs.rend() - 1; it++) { aabb2.push_back(aabb2[aabb2.size() - 1] | it->first); }
				for (int i = 1; i < ySortedObjs.size(); i++) {
					aabb1 |= ySortedObjs[i].first;
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

				aabb1 = calcAABB(zSortedObjs[0].second);
				aabb2.push_back(zSortedObjs.rbegin()->first);
				for (auto it = zSortedObjs.rbegin() + 1; it != zSortedObjs.rend() - 1; it++) { aabb2.push_back(aabb2[aabb2.size() - 1] | it->first); }
				for (int i = 1; i < zSortedObjs.size(); i++) {
					aabb1 |= zSortedObjs[i].first;
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
