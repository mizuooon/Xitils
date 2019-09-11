

#include <Xitils/AccelerationStructure.h>
#include <Xitils/App.h>
#include <Xitils/Camera.h>
#include <Xitils/Geometry.h>
#include <Xitils/PathTracer.h>
#include <Xitils/Scene.h>
#include <Xitils/TriangleMesh.h>
#include <Xitils/RenderTarget.h>
#include <Xitils/VonMisesFisherDistribution.h>
#include <CinderImGui.h>


using namespace Xitils;
using namespace ci;
using namespace ci::app;
using namespace ci::geom;

const int DownSamplingRate = 16;
const int VMFLobeNum = 6;
const int ShellMappingLayerNum = 16;



// TODO
// - テクスチャ解像度に合わせたポリゴン分割をしている plane の作成
//     - その plane はテクスチャをループさせる
// - テクスチャの指定した uv 領域での 実効BRDF 測定 (低解像度と高解像度)
// - 評価したデータの保存、読み込み
// - 処理の並列化

// plane をシーン化して、TextureDownsampled に渡す
// シーンはスレッドセーフなので、DownsampledTexel 間で共有できる

// the concentric mapping

// angular resolution 15^2
// spatial resolution 4^2

class PlaneDisplaceMapped : public TriangleMesh {
public:

	PlaneDisplaceMapped(std::shared_ptr<Texture> displacement, float displacementScale) {

		// xy と uv の座標が一致して、かつ z 方向を向くように平面を生成
		// (0,0) - (1,1) の領域を中心として、uv 方向それぞれ 3 ループずつさせている

		std::array<Vector3f, 4> positions;
		for (int i = 0; i < 4; ++i) {
			positions[i] = Vector3f(i & 1 ? -1.0f : 2.0f,
				i & 2 ? -1.0f : 2.0f,
				0);
		}
		std::array<Vector2f, 4> texCoords;
		for (int i = 0; i < 4; ++i) {
			texCoords[i] = Vector2f(i & 1 ? -1.0f : 2.0f,
				i & 2 ? -1.0f : 2.0f);
		}
		std::array<Vector3f, 4> normals;
		for (int i = 0; i < 4; ++i) {
			normals[i] = Vector3f(0, 0, 1);
		}
		std::array<Vector3f, 4> tangents;
		for (int i = 0; i < 4; ++i) {
			tangents[i] = Vector3f(1, 0, 0);
		}
		std::array<Vector3f, 4> bitangents;
		for (int i = 0; i < 4; ++i) {
			bitangents[i] = Vector3f(0, 1, 0);
		}
		std::array<int, 3 * 2> indices{
			0,1,2, 1,3,2
		};

		setGeometryWithShellMapping(positions.size(),
			positions.data(),
			texCoords.data(),
			normals.data(),
			tangents.data(),
			bitangents.data(),
			indices.size(), indices.data(),
			displacement, displacementScale, ShellMappingLayerNum);
	}

};


class AngularParam {
public:

	AngularParam(int resolutionTheta, int resolutionPhai):
		resolutionTheta(resolutionTheta),
		resolutionPhai(resolutionPhai)
	{}

	int vectorToIndex(const Vector3f& v) const {
		return angleToIndex(vectorToAngle(v));
	}

	void vectorToIndexWeighted(const Vector3f& v, int* indices, float* weights) const {
		angleToIndexWeighted(vectorToAngle(v), indices, weights);
	}

	Vector3f indexToVector(int index) const {
		return angleToVector(indexToAngle(index));
	}
	
	int size() const { return resolutionTheta * resolutionPhai; }

private:
	Vector2f vectorToAngle(const Vector3f& v) const {
		Vector3f n = normalize(v);
		if (n.z < 0) { n.z *= -1; }
		float theta = asinf(n.z); // [0, PI/2]
		float phai = atan2(n.y, n.x); // [-PI, PI];
		if (phai < 0.0f) { phai += M_PI; } // [0, 2PI]
		return Vector2f(theta, phai);
	}

	Vector3f angleToVector(const Vector2f& a) const {
		float theta = a.x;
		float phai = a.y;
		return Vector3f( cosf(theta) * cosf(phai), cosf(theta) * sinf(phai), sinf(theta));
	}

	Vector2f indexToAngle(int index) const {
		int thetaIndex = index / resolutionPhai;
		int phaiIndex = index % resolutionPhai;
		float theta = (thetaIndex + 0.5f) / resolutionTheta;
		float phai = (phaiIndex + 0.5f) / resolutionPhai;
		return Vector2f(theta, phai);
	}

	int angleToIndex(const Vector2f& a) const {
		float theta = a.x;
		float phai = a.y;
		int thetaIndex = (int)(theta * resolutionTheta);
		int phaiIndex = (int)(phai * resolutionPhai);
		thetaIndex = Xitils::clamp(thetaIndex, 0, resolutionTheta - 1);
		phaiIndex = Xitils::clamp(phaiIndex, 0, resolutionPhai - 1);
		return thetaIndex * resolutionPhai + phaiIndex;
	}

	void angleToIndexWeighted(const Vector2f& a, int* indices, float* weights) const {
		float theta = a.x;
		float phai = a.y;

		int thetaIndex0 = (int)(theta * resolutionTheta - 0.5f);
		int phaiIndex0 = (int)(phai * resolutionPhai - 0.5f);
		int thetaIndex1 = thetaIndex0 + 1;
		int phaiIndex1 = phaiIndex0 + 1;

		float wTheta1 = (theta * resolutionTheta - 0.5f) - thetaIndex0;
		float wPhai1 = (phai * resolutionPhai - 0.5f) - phaiIndex0;
		float wTheta0 = 1.0f - wTheta0;
		float wPhai0 = 1.0f - wPhai0;

		if (thetaIndex0 < 0) { thetaIndex0 = resolutionTheta - 1; }
		if (thetaIndex1 >= resolutionTheta) { thetaIndex0 = 0; }
		phaiIndex0 = Xitils::clamp(phaiIndex0, 0, resolutionPhai - 1);
		phaiIndex1 = Xitils::clamp(phaiIndex1, 0, resolutionPhai - 1);

		indices[0] = thetaIndex0 * resolutionPhai + phaiIndex0;
		weights[0] = wTheta0 * wPhai0;
		indices[1] = thetaIndex0 * resolutionPhai + phaiIndex1;
		weights[1] = wTheta0 * wPhai1;
		indices[2] = thetaIndex1 * resolutionPhai + phaiIndex0;
		weights[2] = wTheta1 * wPhai0;
		indices[3] = thetaIndex1 * resolutionPhai + phaiIndex1;
		weights[3] = wTheta1 * wPhai1;
	}

	int resolutionTheta;
	int resolutionPhai;
};

class SpatialParam {
public:

	SpatialParam(int resolutionU, int resolutionV):
		resolutionU(resolutionU),
		resolutionV(resolutionV)
	{}

	int positionToIndex(const Vector2f& p) const {
		int uIndex = Xitils::clamp((int)(p.u * resolutionU), 0, resolutionU - 1);
		int vIndex = Xitils::clamp((int)(p.v * resolutionV), 0, resolutionV - 1);
		return uIndex * resolutionV + vIndex;
	}

	void positionToIndexWeighted(const Vector2f& p, int* indices, float* weights) const {
		int uIndex0 = (int)(p.u * resolutionU - 0.5f);
		int vIndex0 = (int)(p.v * resolutionV - 0.5f);
		int uIndex1 = uIndex0 + 1;
		int vIndex1 = vIndex0 + 1;

		float wu1 = (p.u * resolutionU - 0.5f) - uIndex0;
		float wv1 = (p.v * resolutionV - 0.5f) - vIndex0;
		float wu0 = 1.0f - wu1;
		float wv0 = 1.0f - wv1;

		if (uIndex0 < 0) { uIndex0 = resolutionU - 1; }
		if (uIndex1 >= resolutionU) { uIndex1 = 0; }
		if (vIndex0 < 0) { vIndex0 = resolutionV - 1; }
		if (vIndex1 >= resolutionV) { vIndex1 = 0; }

		indices[0] = uIndex0 * resolutionV + vIndex0;
		weights[0] = wu0 * wv0;
		indices[1] = uIndex0 * resolutionV + vIndex1;
		weights[1] = wu0 * wv1;
		indices[2] = uIndex1 * resolutionV + vIndex0;
		weights[2] = wu1 * wv0;
		indices[3] = uIndex1 * resolutionV + vIndex1;
		weights[3] = wu1 * wv1;
	}

	Vector2f indexToPosition(int index) const {
		int uIndex = index / resolutionV;
		int vIndex = index % resolutionV;
		return Vector2f( (uIndex + 0.5f) / resolutionU, (vIndex + 0.5f) / resolutionV );
	}

	int size() const { return resolutionU * resolutionV; }

private:
	int resolutionU;
	int resolutionV;
};

class AngularTable {
public:

	AngularTable(int resolutionTheta, int resolutionPhai):
		paramWi(resolutionTheta, resolutionPhai),
		paramWo(resolutionTheta, resolutionPhai)
	{
		data.resize(size());
	}

	Vector3f& operator[](int i) { return data[i]; }

	// テーブルの bin の参照を得る
	// 書き込み用
	Vector3f& at(const Vector3f& wi, const Vector3f& wo) {
		return data[vectorToIndex(wi, wo)];
	}

	// テーブル上の値から補間をして新しく値を得る
	// (実装上では、確率的に bin を読んでいるだけ)
	// 読み出し用
	Vector3f eval(const Vector3f& wi, const Vector3f& wo, Sampler& sampler) const {
		std::vector<int> indicesWi(4);
		std::vector<float> weightsWi(4);
		paramWi.vectorToIndexWeighted(wi, indicesWi.data(), weightsWi.data());
		int indexWi = sampler.selectAlongWeights(indicesWi, weightsWi);

		std::vector<int> indicesWo(4);
		std::vector<float> weightsWo(4);
		paramWi.vectorToIndexWeighted(wi, indicesWo.data(), weightsWo.data());
		int indexWo = sampler.selectAlongWeights(indicesWo, weightsWo);

		return data[indexWi * paramWo.size() + indexWo];
	}

	int vectorToIndex(const Vector3f& wi, const Vector3f& wo) const {
		return paramWi.vectorToIndex(wi) * paramWo.size() + paramWo.vectorToIndex(wo);
	}

	void indexToVector(int index, Vector3f* wi, Vector3f* wo) const {
		int indexWi = index / paramWo.size();
		int indexWo = index % paramWo.size();
		*wi = paramWi.indexToVector(indexWi);
		*wo = paramWo.indexToVector(indexWo);
	}

	 int size() const {
		 return paramWi.size() * paramWo.size();
	 }

private:
	std::vector<Vector3f> data;
	AngularParam paramWi, paramWo;
};

class SpatialTable {
public:

	SpatialTable(int resolutionU, int resolutionV):
		paramPos(resolutionU, resolutionV)
	{
		data.resize(size());
	}

	Vector3f& operator[](int i) { return data[i]; }

	// テーブルの bin の参照を得る
	// 書き込み用
	Vector3f& at(const Vector2f& p) {
		return data[positionToIndex(p)];
	}

	// テーブル上の値から補間をした値を得る
	// (実装上では、確率的に bin を読んでいるだけ)
	// 読み出し用
	Vector3f eval(const Vector2f& p, Sampler &sampler) const {
		std::vector<int> indices(4);
		std::vector<float> weights(4);
		paramPos.positionToIndexWeighted(p, indices.data(), weights.data());
		return data[sampler.selectAlongWeights(indices, weights)];
	}

	int positionToIndex(const Vector2f& p) const{
		return paramPos.positionToIndex(p);
	}

	Vector2f indexToPosition(int index) const {
		return paramPos.indexToPosition(index);
	}

	int size() const {
		return paramPos.size();
	}

private:
	std::vector<Vector3f> data;
	SpatialParam paramPos;
};

class ScalingFunction {
public:

	ScalingFunction(Bounds2f texCoordsBound, int N = 4, int M = 1):
		texCoordsBound(texCoordsBound),
		N(N),
		M(M),
		S(N/2, N),
		T(M, M)
	{
	}

	void precalc(const Vector3f& C, const Scene& sceneLow, const Scene& sceneOrig, Sampler& sampler) {
		estimateT(C, sceneLow, sceneOrig, sampler);
		estimateS(sceneLow, sceneOrig, sampler);
	}

	Vector3f evalRir(const Vector2f& texCoord, const Vector3f& wi, const Vector3f& wo, Sampler& sampler) const {
		return T.eval(texCoord, sampler)* S.eval(wi, wo, sampler);
	}

	Vector3f estimateRir(const Vector2f& p, const Vector3f& wi, const Vector3f& wo, const Scene& sceneLow, const Scene& sceneOrig, Sampler& sampler) const {
		Vector3f f_eff_low = estimateEffectiveBRDFLow(p, wi, wo, sceneLow, sampler);
		Vector3f f_eff_ir_orig = estimateEffectiveBRDFIROrig(p, wi, wo, sceneLow, sceneOrig, sampler);
		return Vector3f(
			f_eff_low.x > 1e-6 ? f_eff_ir_orig.x / f_eff_low.x : 0.0f,
			f_eff_low.y > 1e-6 ? f_eff_ir_orig.y / f_eff_low.y : 0.0f,
			f_eff_low.z > 1e-6 ? f_eff_ir_orig.z / f_eff_low.z : 0.0f
		);
	}

private:
	Bounds2f texCoordsBound;
	int N;
	int M;
	SpatialTable T;
	AngularTable S;

	void estimateT(const Vector3f& C, const Scene& sceneLow, const Scene& sceneOrig, Sampler& sampler) {
		//printf("estimate T\n");

		//int SampleNum = 5000;
		int SampleNum = 50;
		Vector3f n = Vector3f(0, 0, 1);
		for (int i = 0; i < T.size(); ++i) {
			T[i] = Vector3f(0.0f);
			for (int sample = 0; sample < SampleNum; ++sample) {
				Vector2f p = T.indexToPosition(i);

				Vector3f wi = sampleVectorFromCosinedHemiSphere(n, sampler);
				Vector3f wo = sampleVectorFromCosinedHemiSphere(n, sampler);
				float prob_wi = clampPositive(dot(wi, n)) / M_PI;
				float prob_wo = clampPositive(dot(wo, n)) / M_PI;

				T[i] += estimateRir(p, wi, wo, sceneLow, sceneOrig, sampler) / (prob_wi * prob_wo);
			}
			T[i] /= SampleNum;
			T[i] /= C;
		}
	}

	void estimateS(const Scene& sceneLow, const Scene& sceneOrig, Sampler& sampler) {
		//printf("estimate S\n");

		//int SampleNum = 2500;
		int SampleNum = 25;

		for (int i = 0; i < S.size(); ++i) {
			S[i] = Vector3f(0.0f);
			for (int sample = 0; sample < SampleNum; ++sample){
				Vector3f wi, wo;
				S.indexToVector(i, &wi, &wo);
				
				Vector2f t = Vector2f(((i / M) % M + sampler.randf()) / M, ((i % M) + sampler.randf()) / M);
				Vector2f p = texCoordsBound.lerp(t);
				float prob_p = 1.0f / texCoordsBound.area();

				if (!(inRange(p.u, texCoordsBound.min.u, texCoordsBound.max.u) && inRange(p.v, texCoordsBound.min.v, texCoordsBound.max.v))) {
					printf("%d | %f %f | %f %f - %f %f | %f %f\n", i, t.u, t.v, texCoordsBound.min.u, texCoordsBound.min.v, texCoordsBound.max.u, texCoordsBound.max.v, p.u, p.v);
				}

				S[i] += estimateRir(p, wi, wo, sceneLow, sceneOrig, sampler) / prob_p;
			}
			S[i] /= SampleNum;
		}
	}

	Vector3f estimateEffectiveBRDFLow(const Vector2f& texelPos, const Vector3f& wi, const Vector3f& wo, const Scene& sceneLow, Sampler& sampler) const {
		// IR は考慮しない

		// p を含むパッチ
		Bounds2f patch;
		Vector2f patchSize = texCoordsBound.size() / M;
		int patchIndexU = (int)((texelPos.u - texCoordsBound.min.u) * M);
		int patchIndexV = (int)((texelPos.v - texCoordsBound.min.v) * M);
		patchIndexU = Xitils::clamp(patchIndexU, 0, M - 1);
		patchIndexV = Xitils::clamp(patchIndexV, 0, M - 1);
		patch.min.u = ((float)patchIndexU / M) * patchSize.u + texCoordsBound.min.u;
		patch.min.v = ((float)patchIndexV / M) * patchSize.v + texCoordsBound.min.v;
		patch.max = patch.min + patchSize;

		Vector3f res;

		//const int Sample = 100; // 何回評価する？
		const int Sample = 10;
		const float RayOffset = 1e-6;
		for (int s = 0; s < Sample; ++s) {

			// texelPos を含むパッチ内で始点となる位置をサンプル
			Vector2f p(sampler.randf(patch.min.u, patch.max.u), sampler.randf(patch.min.v, patch.max.v));
			float prob_p = 1.0f / patch.area();

			Xitils::Ray ray;
			ray.o = Vector3f(p.u, p.v, 99);
			ray.d = Vector3f(0, 0, -1);
			SurfaceInteraction isect;
			bool hit = sceneLow.intersect(ray, &isect);
			if (!hit) { printf("%f %f - %f %f | %f %f\n", patch.min.u, patch.max.u, patch.min.v, patch.max.v, p.u,  p.v); }
			ASSERT(hit);

			Xitils::Ray rayWo;
			rayWo.d = wo;
			rayWo.o = isect.p + rayWo.d * RayOffset;
			if (sceneLow.intersectAny(rayWo)) { continue; }

			Xitils::Ray rayWi;
			rayWi.d = wi;
			rayWi.o = isect.p + rayWi.d * RayOffset;
			if (sceneLow.intersectAny(rayWi)) { continue; }

			res += isect.object->material->bsdfCos(isect, sampler, wi) / prob_p;
		}
		res /= Sample;

		return res;
	}

	Vector3f estimateEffectiveBRDFIROrig(const Vector2f& texelPos, const Vector3f& wi, const Vector3f& wo, const Scene& sceneLow, const Scene& sceneOrig, Sampler& sampler) const {
		// IR を考慮する

		// x を含むパッチ内で始点となる位置をサンプル

		// p を含むパッチ
		Bounds2f patch;
		Vector2f patchSize = texCoordsBound.size() / M;
		int patchIndexU = (int)((texelPos.u - texCoordsBound.min.u) * M);
		int patchIndexV = (int)((texelPos.v - texCoordsBound.min.v) * M);
		patchIndexU = Xitils::clamp(patchIndexU, 0, M - 1);
		patchIndexV = Xitils::clamp(patchIndexV, 0, M - 1);
		patch.min.u = ((float)patchIndexU / M) * patchSize.u + texCoordsBound.min.u;
		patch.min.v = ((float)patchIndexV / M) * patchSize.v + texCoordsBound.min.v;
		patch.max = patch.min + patchSize;

		//printf("------------\n");
		//printf("%f %f\n", texelPos.u, texelPos.v);
		//printf("%f %f - %f %f\n", texCoordsBound.min.u, texCoordsBound.min.v, texCoordsBound.max.u, texCoordsBound.max.v);
		//printf("%f %f - %f %f\n", patch.min.u, patch.min.v, patch.max.u, patch.max.v);

		Vector3f res;

		//const int Sample = 2500;
		const int Sample = 1;
		const float RayOffset = 1e-6;
		for (int s = 0; s < Sample; ++s) {

			// texelPos を含むパッチ内で始点となる位置をサンプル
			Vector2f p(sampler.randf(patch.min.u, patch.max.u), sampler.randf(patch.min.v, patch.max.v));
			float prob_p = 1.0f / patch.area();

			Xitils::Ray ray;
			ray.o = Vector3f(p.u, p.v, 99);
			ray.d = Vector3f(0, 0, -1);
			SurfaceInteraction isect;
			bool hit = sceneOrig.intersect(ray, &isect);
			ASSERT(hit);

			Xitils::Ray rayWo;
			rayWo.d = wo;
			rayWo.o = isect.p + rayWo.d * RayOffset;
			if (sceneOrig.intersectAny(rayWo)) { continue; }

			ray.tMax = Infinity;
			SurfaceInteraction isectLow;
			bool hitLow = sceneLow.intersect(ray, &isectLow);
			ASSERT(hitLow);

			const Vector3f& wg = isect.n;
			const Vector3f& wn = isect.shading.n;
			float A_G_wo = clampPositive(dot(wo, wn)) / clampPositive(dot(wg, wn));
			if (A_G_wo > 1e-6) {
				Vector3f weight(1.0f);
				int pathLength = 1;

				while (true) {
					//printf("%d\n",pathLength);

					Xitils::Ray rayWi;
					rayWi.d = wi;
					rayWi.o = isect.p + rayWi.d * RayOffset;
					if (!sceneOrig.intersectAny(rayWi)) {
						const Vector3f& wm = isect.shading.n;
						float A_G_p_wo = clampPositive(dot(wo, wm)) / clampPositive(dot(wg, wm)); // V 項は除いた値
						res += weight * isect.object->material->bsdfCos(isect, sampler, wi) * A_G_p_wo / A_G_wo;
					}

					Xitils::Ray rayNext;
					float pdf;
					weight *= isect.object->material->evalAndSample(isect, sampler, &rayNext.d, &pdf);
					rayNext.o = isect.p + rayNext.d * RayOffset;
					if (!sceneOrig.intersect(rayNext, &isect)) { break; }

					++pathLength;
					if (pathLength > 10 || weight.isZero()) { break; }
				}
			}

		}

		return res;
	}
};

class SVNDF {
public:
	int resolutionU, resolutionV;
	std::vector<VonMisesFisherDistribution<VMFLobeNum>> vmfs;
	
	Vector3f sampleNormal(const Vector2f& texCoord, Sampler& sampler) const {
		int x0 = texCoord.u * resolutionU + 0.5f;
		int y0 = (1 - texCoord.v) * resolutionV + 0.5f;
		int x1 = x0 + 1;
		int y1 = y0 + 1;
		float wx1 = (texCoord.u * resolutionU + 0.5f) - x0;
		float wy1 = ((1 - texCoord.v) * resolutionV + 0.5f) - y0;

		x0 = Xitils::clamp(x0, 0, resolutionU - 1);
		x1 = Xitils::clamp(x1, 0, resolutionU - 1);
		y0 = Xitils::clamp(y0, 0, resolutionV - 1);
		y1 = Xitils::clamp(y1, 0, resolutionV - 1);

		int x = (sampler.randf() <= wx1) ? x1 : x0;
		int y = (sampler.randf() <= wy1) ? y1 : y0;

		return vmfs[y * resolutionU + x].sample(sampler);
	}
};

std::shared_ptr<Texture> downsampleDisplacementTexture(std::shared_ptr<Texture> texOrig, float displacementScale, std::shared_ptr<SVNDF> svndf) {

	Sampler sampler(0);

	auto texLow = std::make_shared<Texture>(
		(texOrig->getWidth() + DownSamplingRate - 1) / DownSamplingRate,
		(texOrig->getHeight() + DownSamplingRate - 1) / DownSamplingRate);

	svndf->resolutionU = texLow->getWidth();
	svndf->resolutionV = texLow->getHeight();
	svndf->vmfs.clear();
	svndf->vmfs.resize(texLow->getWidth() * texLow->getHeight());

	// texLow の初期値は普通に texOrig をダウンサンプリングした値

	std::vector<Vector2f> ave_slope_orig(texLow->getWidth() * texLow->getHeight());
	for (int py = 0; py < texLow->getHeight(); ++py) {
		for (int px = 0; px < texLow->getWidth(); ++px) {

			int count = 0;
			std::vector<Vector3f> normals;
			normals.reserve(DownSamplingRate * DownSamplingRate);

			for (int ly = 0; ly < DownSamplingRate; ++ly) {
				int y = py * DownSamplingRate + ly;
				if (y >= texOrig->getHeight()) { continue; }
				for (int lx = 0; lx < DownSamplingRate; ++lx) {
					int x = px * DownSamplingRate + lx;
					if (x >= texOrig->getWidth()) { continue; }

					Vector2f slope = Vector2f(texOrig->rgbDifferentialU(x, y).x, texOrig->rgbDifferentialV(x, y).x);
					Vector3f n = Vector3f(-slope.u * displacementScale, -slope.v * displacementScale, 1).normalize();

					ave_slope_orig[py * texLow->getWidth() + px] += slope;
					texLow->r(px, py) += texOrig->r(x, y);

					normals.push_back(n);

					++count;
				}
			}
			ave_slope_orig[py * texLow->getWidth() + px] /= count;
			texLow->r(px, py) /= count;

			svndf->vmfs[py * texLow->getWidth() + px] = VonMisesFisherDistribution<VMFLobeNum>::approximateBySEM(normals, sampler);
		}
	}

	auto L = [&](int px, int py, float dh) {
		const float w = 0.01f;

		int x0 = px * DownSamplingRate;
		int y0 = py * DownSamplingRate;
		int x1 = Xitils::min(px + DownSamplingRate - 1, texOrig->getWidth() - 1);
		int y1 = Xitils::min(py + DownSamplingRate - 1, texOrig->getHeight() - 1);

		float dhdu2 = (texLow->rgbDifferentialU(px + 1, py).r - texLow->rgbDifferentialU(px - 1, py).r) / 2.0f;
		float dhdv2 = -(texLow->rgbDifferentialV(px, py + 1).r - texLow->rgbDifferentialU(px, py - 1).r) / 2.0f; // y 方向と v 方向は逆なので符号反転

		Vector2f ave_slope(
			(texLow->r(x1, y1) + texLow->r(x1, y0) - texLow->r(x0, y1) - texLow->r(x0, y0)) / 2.0f,
			-(texLow->r(x1, y1) + texLow->r(x0, y1) - texLow->r(x1, y0) - texLow->r(x0, y0)) / 2.0f // y 方向と v 方向は逆なので符号反転
		);

		return (ave_slope - ave_slope_orig[py * texLow->getWidth() + px]).lengthSq() - w * powf(dhdu2 + dhdv2, 2.0f);
	};

	// TODO: ダウンサンプリングの正規化項入れる

	return texLow;
}

class MultiLobeSVBRDF : public Material {
public:
	std::shared_ptr<Material> baseMaterial;
	std::shared_ptr<Texture> displacementTexLow;
	float displacementScale;
	std::shared_ptr<SVNDF> svndf;

	MultiLobeSVBRDF(std::shared_ptr<Material> baseMaterial, std::shared_ptr<Texture> displacementTexLow, float displacementScale, std::shared_ptr<SVNDF> svndf):
		baseMaterial(baseMaterial),
		displacementTexLow(displacementTexLow),
		displacementScale(displacementScale),
		svndf(svndf)
	{}

	Vector3f bsdfCos(const SurfaceInteraction& isect, Sampler& sampler, const Vector3f& wi) const override {
		return baseMaterial->bsdfCos(perturbInteraction(isect, sampler), sampler, wi);
	}

	Vector3f evalAndSample(const SurfaceInteraction& isect, Sampler& sampler, Vector3f* wi, float* pdf) const override {
		return baseMaterial->evalAndSample(perturbInteraction(isect, sampler), sampler, wi, pdf);
	}

	float pdf(const SurfaceInteraction& isect, const Vector3f& wi) const override {

		// TODO

		return clampPositive(dot(isect.shading.n, wi)) / M_PI;
	}

private:

	SurfaceInteraction perturbInteraction(const SurfaceInteraction& isect, Sampler& sampler) const {

		SurfaceInteraction isectPerturbed = isect;
		Vector3f n = svndf->sampleNormal(isect.texCoord, sampler);

		isectPerturbed.shading.n =
			(n.z * isect.n
				+ n.x * isect.tangent
				- n.y * isect.bitangent
				).normalize();

		isectPerturbed.shading.n = faceForward(isectPerturbed.shading.n, isectPerturbed.wo);

		return isectPerturbed;
	}

};

class PrefilteredDisplaceMapping : public Material {
public:

	PrefilteredDisplaceMapping(std::shared_ptr<Material> baseMaterial, std::shared_ptr<Texture> displacementTexOrig, float displacementScale, float displacementScaleForPrecalc):
		baseMaterial(baseMaterial),
		displacementTexOrig(displacementTexOrig),
		displacementScale(displacementScale),
		svndf(std::make_shared<SVNDF>())
	{
		std::shared_ptr<Sampler> sampler = std::make_shared<Sampler>(0);

		displacementTexLow = downsampleDisplacementTexture(displacementTexOrig, displacementScale, svndf);
		multiLobeSVBRDF = std::make_shared<MultiLobeSVBRDF>(baseMaterial, displacementTexLow, displacementScale, svndf);

		auto planeLow = std::make_shared<PlaneDisplaceMapped>(displacementTexLow, displacementScaleForPrecalc);
		auto sceneLow = std::make_shared<Scene>();
		sceneLow->addObject(std::make_shared<Object>(planeLow, multiLobeSVBRDF, Xitils::Transform()));
		sceneLow->buildAccelerationStructure();

		auto planeOrig = std::make_shared<PlaneDisplaceMapped>(displacementTexOrig, displacementScaleForPrecalc);
		auto sceneOrig = std::make_shared<Scene>();
		sceneOrig->addObject(std::make_shared<Object>(planeOrig, baseMaterial, Xitils::Transform()));
		sceneOrig->buildAccelerationStructure();

		int resU = displacementTexLow->getWidth();
		int resV = displacementTexLow->getHeight();
		scalingFunctions.resize(resU * resV);
		for (int y = 0; y < resV; ++y) {
			printf("%d / %d\n", y+1, resV);
			for (int x = 0; x < resU; ++x) {
				int index = y * displacementTexLow->getWidth() + x;
				Bounds2f b;
				b.min = Vector2f((float)x / resU, (float)(resV - y - 1) / resV);
				b.max = Vector2f((float)(x + 1) / resU, (float)(resV - y) / resV);
				scalingFunctions[index] = std::make_shared<ScalingFunction>(b);
			}
		}

		Vector3f C = estimateC(*sceneLow, *sceneOrig, *sampler);

#pragma omp parallel for schedule(dynamic, 1)
		for (int y = 0; y < resV; ++y) {
#pragma omp parallel for schedule(dynamic, 1)
			for (int x = 0; x < resU; ++x) {
				printf("%d %d / %d\n", y + 1, x+1, resV);
				int index = y * displacementTexLow->getWidth() + x;
				scalingFunctions[index]->precalc(C, *sceneOrig, *sceneLow, *sampler);
			}
		}

	}


	Vector3f bsdfCos(const SurfaceInteraction& isect, Sampler& sampler, const Vector3f& wi) const override {
		return multiLobeSVBRDF->bsdfCos(isect, sampler, wi) * evalRir(isect.texCoord, wi, isect.wo, sampler);
	}

	Vector3f evalAndSample(const SurfaceInteraction& isect, Sampler& sampler, Vector3f* wi, float* pdf) const override {
		return multiLobeSVBRDF->evalAndSample(isect, sampler, wi, pdf) * evalRir(isect.texCoord, *wi, isect.wo, sampler);
	}

	float pdf(const SurfaceInteraction& isect, const Vector3f& wi) const override {

		// TODO

		return clampPositive(dot(isect.shading.n, wi)) / M_PI;
	}

	Vector3f estimateC(const Scene& sceneLow, const Scene& sceneOrig, Sampler& sampler) {
		Vector3f C(0.0f);
		int SampleNum = 2500;
		//int SampleNum = 25;

		for (int i = 0; i < SampleNum; ++i) {
			Vector2f p( sampler.randf(), sampler.randf() );
			float prob_p = 1.0f;

			Vector3f n(0, 1, 0);
			Vector3f wi = sampleVectorFromCosinedHemiSphere(n, sampler);
			Vector3f wo = sampleVectorFromCosinedHemiSphere(n, sampler);
			float prob_wi = clampPositive(dot(wi, n)) / M_PI;
			float prob_wo = clampPositive(dot(wo, n)) / M_PI;

			C += estimateRir(p, wi, wo, sceneLow, sceneOrig, sampler) / (prob_p * prob_wi * prob_wo);
		}
		C /= SampleNum;
		return C;
	}

	std::shared_ptr<const Texture> getDisplacementTextureLow() { return displacementTexLow; }

private:
	std::shared_ptr<Material> baseMaterial;
	std::shared_ptr<Texture> displacementTexOrig;
	std::shared_ptr<Texture> displacementTexLow;
	std::shared_ptr<MultiLobeSVBRDF> multiLobeSVBRDF;
	float displacementScale;
	std::shared_ptr<SVNDF> svndf;
	Vector3f estimatedC;
	std::vector<std::shared_ptr<ScalingFunction>> scalingFunctions; // ダウンサンプリング後の各テクセルに対応するスケーリング関数

	Vector3f estimateRir(const Vector2f& texCoord, const Vector3f& wi, const Vector3f& wo, const Scene& sceneLow, const Scene& sceneOrig, Sampler& sampler) const {
		//int resolutionU = displacementTexLow->getWidth();
		//int resolutionV = displacementTexLow->getHeight();

		//int x0 = texCoord.u * resolutionU + 0.5f;
		//int y0 = (1 - texCoord.v) * resolutionV + 0.5f;
		//int x1 = x0 + 1;
		//int y1 = y0 + 1;
		//float wx1 = (texCoord.u * resolutionU + 0.5f) - x0;
		//float wy1 = ((1 - texCoord.v) * resolutionV + 0.5f) - y0;

		//x0 = Xitils::clamp(x0, 0, resolutionU - 1);
		//x1 = Xitils::clamp(x1, 0, resolutionU - 1);
		//y0 = Xitils::clamp(y0, 0, resolutionV - 1);
		//y1 = Xitils::clamp(y1, 0, resolutionV - 1);

		//int x = (sampler.randf() <= wx1) ? x1 : x0;
		//int y = (sampler.randf() <= wy1) ? y1 : y0;

		//return scalingFunctions[y * resolutionU + x]->estimateRir(texCoord, wi, wo, sceneLow, sceneOrig, sampler);

		int resolutionU = displacementTexLow->getWidth();
		int resolutionV = displacementTexLow->getHeight();
		int x = texCoord.u * resolutionU;
		int y = (1 - texCoord.v) * resolutionV;
		x = Xitils::clamp(x, 0, resolutionU - 1);
		y = Xitils::clamp(y, 0, resolutionV - 1);
		return scalingFunctions[y * resolutionU + x]->estimateRir(texCoord, wi, wo, sceneLow, sceneOrig, sampler);
	}


	Vector3f evalRir(const Vector2f& texCoord, const Vector3f& wi, const Vector3f& wo, Sampler& sampler) const {
		//int resolutionU = displacementTexLow->getWidth();
		//int resolutionV = displacementTexLow->getHeight();

		//int x0 = texCoord.u * resolutionU + 0.5f;
		//int y0 = (1 - texCoord.v) * resolutionV + 0.5f;
		//int x1 = x0 + 1;
		//int y1 = y0 + 1;
		//float wx1 = (texCoord.u * resolutionU + 0.5f) - x0;
		//float wy1 = ((1 - texCoord.v) * resolutionV + 0.5f) - y0;

		//x0 = Xitils::clamp(x0, 0, resolutionU - 1);
		//x1 = Xitils::clamp(x1, 0, resolutionU - 1);
		//y0 = Xitils::clamp(y0, 0, resolutionV - 1);
		//y1 = Xitils::clamp(y1, 0, resolutionV - 1);

		//int x = (sampler.randf() <= wx1) ? x1 : x0;
		//int y = (sampler.randf() <= wy1) ? y1 : y0;

		//return scalingFunctions[y * resolutionU + x]->evalRir(texCoord, wi, wo, sampler);

		int resolutionU = displacementTexLow->getWidth();
		int resolutionV = displacementTexLow->getHeight();
		int x = texCoord.u * resolutionU;
		int y = (1 - texCoord.v) * resolutionV;
		x = Xitils::clamp(x, 0, resolutionU - 1);
		y = Xitils::clamp(y, 0, resolutionV - 1);
		return scalingFunctions[y * resolutionU + x]->evalRir(texCoord, wi, wo, sampler);
	}

};


struct MyFrameData {
	float initElapsed;
	float frameElapsed;
	Surface surface;
	int triNum;
	int sampleNum = 0;
};

struct MyUIFrameData {
	Vector3f rot;
};

class MyApp : public Xitils::App::XApp<MyFrameData, MyUIFrameData> {
public:
	void onSetup(MyFrameData* frameData, MyUIFrameData* uiFrameData) override;
	void onCleanup(MyFrameData* frameData, MyUIFrameData* uiFrameData) override;
	void onUpdate(MyFrameData& frameData, const MyUIFrameData& uiFrameData) override;
	void onDraw(const MyFrameData& frameData, MyUIFrameData& uiFrameData) override;

private:
	
	gl::TextureRef texture;

	std::shared_ptr<Scene> scene;
	inline static const glm::ivec2 ImageSize = glm::ivec2(800, 800);

	std::shared_ptr<RenderTarget> renderTarget;
	std::shared_ptr<PathTracer> pathTracer;

	decltype(std::chrono::system_clock::now()) time_start;
};

void MyApp::onSetup(MyFrameData* frameData, MyUIFrameData* uiFrameData) {
	time_start = std::chrono::system_clock::now();
	auto init_time_start = std::chrono::system_clock::now();
	
	frameData->surface = Surface(ImageSize.x, ImageSize.y, false);
	frameData->frameElapsed = 0.0f;
	frameData->sampleNum = 0;
	frameData->triNum = 0;

	getWindow()->setTitle("Xitils");
	setWindowSize(ImageSize);
	setFrameRate(60);

	ui::initialize();

	scene = std::make_shared<Scene>();

	scene->camera = std::make_shared<PinholeCamera>(
		transformTRS(Vector3f(0,1.5f,-4), Vector3f(10, 0, 0), Vector3f(1)), 
		40 * ToRad, 
		(float)ImageSize.y / ImageSize.x
		);
	//scene->camera = std::make_shared<OrthographicCamera>(
	//	translate(0, 0.5f, -3), 4, 3);

	const int subdivision = 10;
	auto teapot = std::make_shared<Teapot>();
	teapot->subdivisions(subdivision);
	auto teapotMeshData = std::make_shared<TriMesh>(*teapot);


	//auto teapot_material = std::make_shared<MultiLobeSVBRDF>();
	//teapot_material->baseMaterial = std::make_shared<Diffuse>(Vector3f(0.8f));
	//teapot_material->displacementScale = 0.01f;
	//teapot_material->dispTexLow = std::make_shared<Texture>("dispcloth.jpg");
	//teapot_material->dispTexLow = downsampleTexture(teapot_material->dispTexLow, teapot_material->displacementScale, &teapot_material->vmfs);
	//auto& dispmap = teapot_material->dispTexLow;

	auto teapot_material = std::make_shared<Diffuse>(Vector3f(0.8f));

	float dispScale = 0.01f;
	float dispScaleForPreCalc = 0.02f;
	auto dispTexOrig = std::make_shared<Texture>("dispcloth.jpg");
	dispTexOrig->warpClamp = false;


	auto prefilteredDispMaterial = std::make_shared<PrefilteredDisplaceMapping>(teapot_material, dispTexOrig, dispScale, dispScaleForPreCalc);

	auto teapotMesh = std::make_shared<TriangleMesh>();
	//teapotMesh->setGeometry(teapotMeshData);
	teapotMesh->setGeometryWithShellMapping(*teapotMeshData, prefilteredDispMaterial->getDisplacementTextureLow(), dispScale, ShellMappingLayerNum);
	//auto material = std::make_shared<SpecularFresnel>();
	//material->index = 1.2f;

	auto diffuse_white = std::make_shared<Diffuse>(Vector3f(0.8f));
	
	auto diffuse_red = std::make_shared<Diffuse>(Vector3f(0.8f, 0.1f, 0.1f));
	auto diffuse_green = std::make_shared<Diffuse>(Vector3f(0.1f, 0.8f, 0.1f));
	auto emission = std::make_shared<Emission>(Vector3f(1.0f, 1.0f, 0.95f) * 8);
	auto cube = std::make_shared<Xitils::Cube>();
	auto plane = std::make_shared<Xitils::Plane>();
	scene->addObject(
		std::make_shared<Object>( cube, diffuse_white, transformTRS(Vector3f(0,0,0), Vector3f(), Vector3f(4, 0.01f, 4)))
	);
	scene->addObject(
		std::make_shared<Object>(cube, diffuse_green, transformTRS(Vector3f(2, 2, 0), Vector3f(), Vector3f(0.01f, 4, 4)))
	);
	scene->addObject(
		std::make_shared<Object>(cube, diffuse_red, transformTRS(Vector3f(-2, 2, 0), Vector3f(), Vector3f(0.01f, 4, 4)))
	);
	scene->addObject(
		std::make_shared<Object>(cube, diffuse_white, transformTRS(Vector3f(0, 2, 2), Vector3f(), Vector3f(4, 4, 0.01f)))
	);
	scene->addObject(
		std::make_shared<Object>(cube, diffuse_white, transformTRS(Vector3f(0, 4, 0), Vector3f(), Vector3f(4, 0.01f, 4)))
	);
	scene->addObject(
		std::make_shared<Object>(plane, emission, transformTRS(Vector3f(0, 4.0f -0.01f, 0), Vector3f(-90,0,0), Vector3f(2.0f)))
	);


	//auto planeDisp = std::make_shared<PlaneDisplaceMapped>(dispmap, 0.02f);
	//scene->addObject(
	//	std::make_shared<Object>(planeDisp, diffuse_white, transformTRS(Vector3f(0, 1, -1), Vector3f(-45, 180, 0), Vector3f(1.0f)))
	//);

	//scene->addObject(std::make_shared<Object>(teapotMesh, diffuse_white,
	//	transformTRS(Vector3f(0.8f, 0, 0.0f), Vector3f(0, 0, 0), Vector3f(1, 1, 1)
	//	)));
	//scene->addObject(
	//	std::make_shared<Object>(cube, diffuse_white, transformTRS(Vector3f(-0.8f, 0.5f, 0.5f), Vector3f(0,30,0), Vector3f(1,1,1)))
	//);

	scene->addObject(std::make_shared<Object>(teapotMesh, teapot_material,
		transformTRS(Vector3f(0.0f, 0, 0.0f), Vector3f(0, 0, 0), Vector3f(1.5f))
		));

	scene->buildAccelerationStructure();

	//scene->skySphere = std::make_shared<SkySphereFromImage>("rnl_probe.hdr");

	renderTarget = std::make_shared<RenderTarget>(ImageSize.x, ImageSize.y);

	pathTracer = std::make_shared<StandardPathTracer>();
	//pathTracer = std::make_shared<DebugRayCaster>([](const SurfaceInteraction& isect) { 
	//	return (isect.shading.n) *0.5f + Vector3f(0.5f);
	//	} );

	auto time_end = std::chrono::system_clock::now();

	frameData->initElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - init_time_start).count();
	frameData->triNum = teapotMeshData->getNumTriangles();
}

void MyApp::onCleanup(MyFrameData* frameData, MyUIFrameData* uiFrameData) {

}

void MyApp::onUpdate(MyFrameData& frameData, const MyUIFrameData& uiFrameData) {
	auto time_start = std::chrono::system_clock::now();

	//scene->objects[0]->objectToWorld = rotateYXZ(uiFrameData.rot);
	//scene->buildAccelerationStructure();

	//renderTarget->clear();

	int sample = 1;

	frameData.sampleNum += sample;

	renderTarget->render(*scene, sample, [&](const Vector2f& pFilm, Sampler& sampler, Vector3f& color) {
		auto ray = scene->camera->GenerateRay(pFilm, sampler);

		color += pathTracer->eval(*scene, sampler, ray);

		//SurfaceInteraction isect;
		//
		//if (scene->intersect(ray, &isect)) {
		//	//Vector3f dLight = normalize(Vector3f(1, 1, -1));
		//	//color = Vector3f(1.0f, 1.0f, 1.0f) * clamp01(dot(isect.shading.n, dLight));

		//	Vector3f wi;
		//	Vector3f f;
		//	if (isect.object->material->emissive) {
		//		color += isect.object->material->emission(isect);
		//	}
		//	if (isect.object->material->specular) {
		//		f = isect.object->material->evalAndSampleSpecular(isect, sampler, &wi);
		//	} else {
		//		float pdf;
		//		f = isect.object->material->evalAndSample(isect, sampler, &wi, &pdf);
		//	}
		//	if (!f.isZero()) {

		//		Xitils::Ray ray2;
		//		SurfaceInteraction isect2;
		//		ray2.d = wi;
		//		ray2.o = isect.p + ray2.d * 0.001f;
		//		if (scene->intersect(ray2,&isect2)) {
		//			if (isect2.object->material->emissive) {
		//				color += f * isect2.object->material->emission(isect2) * 10;
		//			}
		//		}

		//	}

		//	//color += isect.shading.n * 0.5f + Vector3f(0.5f);

		//}
	});

	renderTarget->toneMap(&frameData.surface, frameData.sampleNum);

	auto time_end = std::chrono::system_clock::now();
	frameData.frameElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start).count();
}

void MyApp::onDraw(const MyFrameData& frameData, MyUIFrameData& uiFrameData) {

	texture = gl::Texture::create(frameData.surface);

	gl::clear(Color::gray(0.5f));
	
	auto windowSize = ci::app::getWindowSize();
	gl::draw(texture, (windowSize - ImageSize) / 2);

	ImGui::Begin("ImGui Window");
	ImGui::Text(("Image Resolution: " + std::to_string(ImageSize.x) + " x " + std::to_string(ImageSize.y)).c_str());
	ImGui::Text(("Elapsed in Initialization: " + std::_Floating_to_string("%.1f", frameData.initElapsed) + " ms").c_str());
	ImGui::Text(("Elapsed : " + std::to_string(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - time_start).count()) + " s").c_str());
	//ImGui::Text(("Triangles: " + std::to_string(frameData.triNum)).c_str());
	ImGui::Text(("Samples: " + std::to_string(frameData.sampleNum)).c_str());

	//float rot[3];
	//rot[0] = uiFrameData.rot.x;
	//rot[1] = uiFrameData.rot.y;
	//rot[2] = uiFrameData.rot.z;
	//ImGui::SliderFloat3("Rotation", rot, -180, 180 );
	//uiFrameData.rot.x = rot[0];
	//uiFrameData.rot.y = rot[1];
	//uiFrameData.rot.z = rot[2];

	ImGui::End();
}


XITILS_APP(MyApp)
