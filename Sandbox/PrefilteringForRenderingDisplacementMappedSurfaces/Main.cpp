

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
#include <cinder/ObjLoader.h>

#include <omp.h>


#define SAMPLE_P_WITH_EXPLICIT_RAYCAST

enum _MethodMode {
	MethodModeReference,
	MethodModeNaive,
	MethodModeMultiLobeSVBRDF,
	MethodModeProposed
};

const _MethodMode MethodMode = MethodModeProposed;

using namespace xitils;
using namespace ci;
using namespace ci::app;
using namespace ci::geom;

const int DownSamplingRate = 8;
const int VMFLobeNum = 4;
const int ShellMappingLayerNum = 16;


//----------------------------------------------------------------------------------------------------------

// スケーリング関数の事前計算に使用するための、ディスプレースメントマッピングを適用した平面メッシュ
class PlaneDisplaceMapped : public TriangleMesh {
public:

	PlaneDisplaceMapped(std::shared_ptr<const Texture> displacement, float displacementScale) {

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

//----------------------------------------------------------------------------------------------------------

// 2 次元の方向を 1 次元のインデックスに対応付けるためのクラス
class AngularParam {
public:

	// theta は極角、phi は方位角
	AngularParam(int resolutionTheta, int resolutionPhi):
		resolutionTheta(resolutionTheta),
		resolutionPhi(resolutionPhi)
	{}

	int vectorToIndex(const Vector3f& v) const {
		return angleToIndex(vectorToAngle(v));
	}

	void vectorToIndexWeighted(const Vector3f& v, int* indices, float* weights) const {
		angleToIndexWeighted(vectorToAngle(v), indices, weights);
	}

	Vector3f indexToVector(int index, Sampler& sampler) const {
		return angleToVector(indexToAngle(index, sampler));
	}
	
	int size() const { return resolutionTheta * resolutionPhi; }

private:
	int resolutionTheta;
	int resolutionPhi;

	Vector2f vectorToAngle(const Vector3f& v) const {

		Vector3f n = normalize(v);
		if (n.z < 0) { n.z *= -1; }

		if (n.x == 0.0f && n.y == 0.0f) { return Vector2f(M_PI / 2, 0.0f); }
		//if (n.x < 1e-6 && n.y < 1e-6) { return Vector2f(M_PI / 2, 0.0f); }

		float theta = asinf(n.z); // [0, PI/2]
		float phi = atan2(n.y, n.x); // [-PI, PI];
		if (phi < 0.0f) { phi += 2 * M_PI; } // [0, 2PI] -- x 軸方向が phi=0

		return Vector2f(theta, phi);
	}

	Vector3f angleToVector(const Vector2f& a) const {
		float theta = a.x;
		float phi = a.y;
		return Vector3f( cosf(theta) * cosf(phi), cosf(theta) * sinf(phi), sinf(theta));
	}

	Vector2f indexToAngle(int index, Sampler& sampler) const {
		int thetaIndex = index / resolutionPhi;
		int phiIndex = index % resolutionPhi;
		float thetaNormalized = (float)(thetaIndex + sampler.randf()) / resolutionTheta;
		float phiNormalized = (float)(phiIndex + sampler.randf()) / resolutionPhi;
		float theta = thetaNormalized * (M_PI / 2);
		float phi = phiNormalized * (2 * M_PI);
		return Vector2f(theta, phi);
	}

	int angleToIndex(const Vector2f& a) const {
		float theta = a.x;
		float phi = a.y;
		float thetaNormalized = (float)theta / (M_PI / 2);
		float phiNormalized = (float)phi / (2 * M_PI);
		int thetaIndex = (int)(thetaNormalized * resolutionTheta);
		int phiIndex = (int)(phiNormalized * resolutionPhi);
		thetaIndex = xitils::clamp(thetaIndex, 0, resolutionTheta - 1);
		phiIndex = xitils::clamp(phiIndex, 0, resolutionPhi - 1);
		return thetaIndex * resolutionPhi + phiIndex;
	}

	void angleToIndexWeighted(const Vector2f& a, int* indices, float* weights) const {
		float theta = a.x;
		float phi = a.y;

		ASSERT(inRange(theta, 0, M_PI / 2));
		ASSERT(inRange(phi, 0, 2 * M_PI));

		float thetaNormalized = (float)theta / (M_PI / 2);
		float phiNormalized = (float)phi / (2 * M_PI);

		int thetaIndex0 = (int)floorf(thetaNormalized * resolutionTheta - 0.5f);
		int phiIndex0 = (int)floorf(phiNormalized * resolutionPhi - 0.5f);
		int thetaIndex1 = thetaIndex0 + 1;
		int phiIndex1 = phiIndex0 + 1;

		float wTheta1 = (thetaNormalized * resolutionTheta - 0.5f) - thetaIndex0;
		float wPhi1 = (phiNormalized * resolutionPhi - 0.5f) - phiIndex0;
		float wTheta0 = 1.0f - wTheta1;
		float wPhi0 = 1.0f - wPhi1;

		ASSERT(inRange01(wTheta1));
		ASSERT(inRange01(wPhi1));
		
		thetaIndex0 = xitils::clamp(thetaIndex0, 0, resolutionTheta - 1);
		thetaIndex1 = xitils::clamp(thetaIndex1, 0, resolutionTheta - 1);
		if (phiIndex0 < 0) { phiIndex0 = resolutionPhi - 1; }
		if (phiIndex1 >= resolutionPhi) { phiIndex1 = 0; }

		indices[0] = thetaIndex0 * resolutionPhi + phiIndex0;
		weights[0] = wTheta0 * wPhi0;
		indices[1] = thetaIndex0 * resolutionPhi + phiIndex1;
		weights[1] = wTheta0 * wPhi1;
		indices[2] = thetaIndex1 * resolutionPhi + phiIndex0;
		weights[2] = wTheta1 * wPhi0;
		indices[3] = thetaIndex1 * resolutionPhi + phiIndex1;
		weights[3] = wTheta1 * wPhi1;
	}
};

// 2 次元のテクスチャ座標系空間を 1 次元のインデックスに対応付けるためのクラス
class SpatialParam {
public:

	SpatialParam(int resolutionU, int resolutionV):
		resolutionU(resolutionU),
		resolutionV(resolutionV)
	{}

	int positionToIndex(const Vector2f& p) const {
		int uIndex = xitils::clamp((int)(p.u * resolutionU), 0, resolutionU - 1);
		int vIndex = xitils::clamp((int)(p.v * resolutionV), 0, resolutionV - 1);
		return uIndex * resolutionV + vIndex;
	}

	void positionToIndexWeighted(const Vector2f& p, int* indices, float* weights) const {
		int uIndex0 = (int)floorf(p.u * resolutionU - 0.5f);
		int vIndex0 = (int)floorf(p.v * resolutionV - 0.5f);
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

	Vector2f indexToPosition(int index, Sampler& sampler) const {
		int uIndex = index / resolutionV;
		int vIndex = index % resolutionV;
		return Vector2f( (uIndex + sampler.randf()) / resolutionU, (vIndex + sampler.randf()) / resolutionV );
	}

	int size() const { return resolutionU * resolutionV; }

private:
	int resolutionU;
	int resolutionV;
};

// 2 個の 2 次元方向 wi, wo を引数とする 4 次元関数をテーブル化するクラス
class AngularTable {
public:

	// theta は極角、phi は方位角
	AngularTable(int resolutionTheta, int resolutionPhi):
		paramWi(resolutionTheta, resolutionPhi),
		paramWo(resolutionTheta, resolutionPhi)
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
		paramWo.vectorToIndexWeighted(wo, indicesWo.data(), weightsWo.data());
		int indexWo = sampler.selectAlongWeights(indicesWo, weightsWo);

		return data[indexWi * paramWo.size() + indexWo];
	}

	int vectorToIndex(const Vector3f& wi, const Vector3f& wo) const {
		return paramWi.vectorToIndex(wi) * paramWo.size() + paramWo.vectorToIndex(wo);
	}

	void indexToVector(int index, Vector3f* wi, Vector3f* wo, Sampler& sampler) const {
		int indexWi = index / paramWo.size();
		int indexWo = index % paramWo.size();
		*wi = paramWi.indexToVector(indexWi, sampler);
		*wo = paramWo.indexToVector(indexWo, sampler);
	}

	 int size() const {
		 return paramWi.size() * paramWo.size();
	 }

private:
	std::vector<Vector3f> data;
	AngularParam paramWi, paramWo;
};

// 1 個の 2 次元テクスチャ座標系座標 p を引数とする 2 次元関数をテーブル化するクラス
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

	Vector2f indexToPosition(int index, Sampler& sampler) const {
		return paramPos.indexToPosition(index, sampler);
	}

	int size() const {
		return paramPos.size();
	}

private:
	std::vector<Vector3f> data;
	SpatialParam paramPos;
};

//----------------------------------------------------------------------------------------------------------

// 2 次元のテクスチャ座標系空間で変化する NDF を表現するクラス
class SVNDF {
public:
	int resolutionU, resolutionV;
	std::vector<VonMisesFisherDistribution<VMFLobeNum>> vmfs;
	
	Vector3f sampleNormal(const Vector2f& texCoord, Sampler& sampler) const {
		Vector2f normalizedUV;
		normalizedUV.u = texCoord.u - floorf(texCoord.u);
		normalizedUV.v = texCoord.v - floorf(texCoord.v);

		int x0 = normalizedUV.u * resolutionU - 0.5f;
		int y0 = normalizedUV.v * resolutionV - 0.5f;
		int x1 = x0 + 1;
		int y1 = y0 + 1;
		float wx1 = (normalizedUV.u * resolutionU - 0.5f) - x0;
		float wy1 = (normalizedUV.v * resolutionV - 0.5f) - y0;

		if (x0 < 0) { x0 = resolutionU - 1; }
		if (y0 < 0) { y0 = resolutionV - 1; }
		if (x1 >= resolutionU) { x1 = 0; }
		if (y1 >= resolutionU) { y1 = 0; }

		int x = (sampler.randf() <= wx1) ? x1 : x0;
		int y = (sampler.randf() <= wy1) ? y1 : y0;

		return vmfs[y * resolutionU + x].sample(sampler);
	}
};

// ディスプレースメントマッピングをダウンサンプリングし、低解像度テクスチャと SVNDF を生成する
std::shared_ptr<Texture> downsampleDisplacementTexture(std::shared_ptr<Texture> texOrig, float displacementScale, std::shared_ptr<SVNDF> svndf) {

	std::vector<std::shared_ptr<Sampler>> samplers(omp_get_max_threads());
	for (int i = 0; i < samplers.size(); ++i) {
		samplers[i] = std::make_shared<Sampler>(i);
	}

	auto texLow = std::make_shared<Texture>(
		(texOrig->getWidth() + DownSamplingRate - 1) / DownSamplingRate,
		(texOrig->getHeight() + DownSamplingRate - 1) / DownSamplingRate);
	texLow->warpClamp = texOrig->warpClamp;

	svndf->resolutionU = texLow->getWidth();
	svndf->resolutionV = texLow->getHeight();
	svndf->vmfs.clear();
	svndf->vmfs.resize(texLow->getWidth() * texLow->getHeight());

	// texLow の初期値は普通に texOrig をダウンサンプリングした値

	std::vector<Vector2f> ave_slope_orig(texLow->getWidth() * texLow->getHeight());
#pragma omp parallel for schedule(dynamic, 1)
	for (int py = 0; py < texLow->getHeight(); ++py) {
#pragma omp parallel for schedule(dynamic, 1)
		for (int px = 0; px < texLow->getWidth(); ++px) {

			auto& sampler = samplers[omp_get_thread_num()];

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

			svndf->vmfs[py * texLow->getWidth() + px] = VonMisesFisherDistribution<VMFLobeNum>::approximateBySEM(normals, *sampler);
		}
	}

	auto L = [&](int px, int py, float dh) {
		const float w = 0.01f;

		int x0 = px * DownSamplingRate;
		int y0 = py * DownSamplingRate;
		int x1 = xitils::min(px + DownSamplingRate - 1, texOrig->getWidth() - 1);
		int y1 = xitils::min(py + DownSamplingRate - 1, texOrig->getHeight() - 1);

		float dhdu2 = (texLow->rgbDifferentialU(px + 1, py).r - texLow->rgbDifferentialU(px - 1, py).r) / 2.0f;
		float dhdv2 = (texLow->rgbDifferentialV(px, py + 1).r - texLow->rgbDifferentialU(px, py - 1).r) / 2.0f;

		Vector2f ave_slope(
			(texLow->r(x1, y1) + texLow->r(x1, y0) - texLow->r(x0, y1) - texLow->r(x0, y0)) / 2.0f,
			(texLow->r(x1, y1) + texLow->r(x0, y1) - texLow->r(x1, y0) - texLow->r(x0, y0)) / 2.0f
		);

		return (ave_slope - ave_slope_orig[py * texLow->getWidth() + px]).lengthSq() - w * powf(dhdu2 + dhdv2, 2.0f);
	};

	// TODO: ダウンサンプリングの正規化項入れる

	return texLow;
}

// SVNDF とベース BRDF の畳み込みを行ったマテリアル。シャドーイング・マスキング効果を考慮しない。
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

	Vector3f bsdfCos(const SurfaceIntersection& isect, Sampler& sampler, const Vector3f& wi) const override {
		return baseMaterial->bsdfCos(perturbInteraction(isect, sampler), sampler, wi);
	}

	Vector3f evalAndSample(const SurfaceIntersection& isect, Sampler& sampler, Vector3f* wi, float* pdf) const override {
		return baseMaterial->evalAndSample(perturbInteraction(isect, sampler), sampler, wi, pdf);
	}

	float pdf(const SurfaceIntersection& isect, const Vector3f& wi) const override {
		return clampPositive(dot(isect.shading.n, wi)) / M_PI;
	}

	SurfaceIntersection perturbInteraction(const SurfaceIntersection& isect, Sampler& sampler) const {

		SurfaceIntersection isectPerturbed = isect;
		Vector3f n = svndf->sampleNormal(isect.texCoord, sampler);

		isectPerturbed.shading.n =
			( n.z * isect.n
			+ n.x * isect.tangent
			+ n.y * isect.bitangent
			).normalize();

		//isectPerturbed.shading.n = faceForward(isectPerturbed.shading.n, isectPerturbed.wo);

		return isectPerturbed;
	}

};

//----------------------------------------------------------------------------------------------------------

// スケーリング関数でシャドーイング・マスキング効果を補償するマテリアル
class PrefilteredDisplaceMapping : public Material {
public:

	const int SpatialResolution = 2;
	const int PhiResolution = 20;
	const int ThetaResolution = PhiResolution / 4;

	PrefilteredDisplaceMapping(std::shared_ptr<Material> baseMaterial, std::shared_ptr<Texture> displacementTexOrig, float displacementScale):
		T(SpatialResolution, SpatialResolution),
		S(ThetaResolution, PhiResolution),
		baseMaterial(baseMaterial),
		displacementTexOrig(displacementTexOrig),
		displacementScale(displacementScale),
		svndf(std::make_shared<SVNDF>())
	{

		displacementTexLow = downsampleDisplacementTexture(displacementTexOrig, displacementScale, svndf);
		multiLobeSVBRDF = std::make_shared<MultiLobeSVBRDF>(baseMaterial, displacementTexLow, displacementScale, svndf);

		auto planeLow = std::make_shared<PlaneDisplaceMapped>(displacementTexLow, displacementScale);
		auto sceneLow = std::make_shared<Scene>();
		sceneLow->addObject(std::make_shared<Object>(planeLow, multiLobeSVBRDF, xitils::Transform()));
		sceneLow->buildAccelerationStructure();

		auto planeOrig = std::make_shared<PlaneDisplaceMapped>(displacementTexOrig, displacementScale);
		auto sceneOrig = std::make_shared<Scene>();
		sceneOrig->addObject(std::make_shared<Object>(planeOrig, baseMaterial, xitils::Transform()));
		sceneOrig->buildAccelerationStructure();

		estimateT(*sceneLow, *sceneOrig);
		estimateS(*sceneLow, *sceneOrig);
	}

	Vector3f bsdfCos(const SurfaceIntersection& isect, Sampler& sampler, const Vector3f& wi) const override {
		Vector3f wiLocal = Vector3f(dot(isect.tangent, wi), dot(isect.bitangent, wi), dot(isect.n, wi));
		Vector3f woLocal = Vector3f(dot(isect.tangent, isect.wo), dot(isect.bitangent, isect.wo), dot(isect.n, isect.wo));

		return multiLobeSVBRDF->bsdfCos(isect, sampler, wi) * evalRir(isect.texCoord, wiLocal, woLocal, sampler);
	}

	Vector3f evalAndSample(const SurfaceIntersection& isect, Sampler& sampler, Vector3f* wi, float* pdf) const override {
		Vector3f wiLocal = Vector3f(dot(isect.tangent, *wi), dot(isect.bitangent, *wi), dot(isect.n, *wi));
		Vector3f woLocal = Vector3f(dot(isect.tangent, isect.wo), dot(isect.bitangent, isect.wo), dot(isect.n, isect.wo));

		return multiLobeSVBRDF->evalAndSample(isect, sampler, wi, pdf) * evalRir(isect.texCoord, wiLocal, woLocal, sampler);
	}

	float pdf(const SurfaceIntersection& isect, const Vector3f& wi) const override {
		return 0.9f * multiLobeSVBRDF->pdf(isect, wi) + 0.1f * clampPositive(dot(isect.shading.n, wi)) / M_PI;
	}

	std::shared_ptr<const Texture> getDisplacementTextureLow() { return displacementTexLow; }
	std::shared_ptr<Material> getMutliLobeSVBRDF() { return multiLobeSVBRDF; }

private:
	SpatialTable T;
	AngularTable S;

	std::shared_ptr<Material> baseMaterial;
	std::shared_ptr<Texture> displacementTexOrig;
	std::shared_ptr<Texture> displacementTexLow;
	std::shared_ptr<Material> multiLobeSVBRDF;
	float displacementScale;
	std::shared_ptr<SVNDF> svndf;

	Vector3f evalRir(Vector2f texCoord, const Vector3f& wi, const Vector3f& wo, Sampler& sampler) const {
		normalizeUV(texCoord);
		return T.eval(texCoord, sampler) * S.eval(wi, wo, sampler);
	}

	Vector3f estimateRir(Vector2f p, const Vector3f& wi, const Vector3f& wo, const Scene& sceneLow, const Scene& sceneOrig, Sampler& sampler) const {
		normalizeUV(p);
		Vector3f f_eff_low = estimateEffectiveBRDFLow(p, wi, wo, sceneLow, sampler);
		Vector3f f_eff_ir_orig = estimateEffectiveBRDFIROrig(p, wi, wo, sceneOrig, sampler);
		return Vector3f(
			f_eff_low.x > 1e-6 ? f_eff_ir_orig.x / f_eff_low.x : 0.0f,
			f_eff_low.y > 1e-6 ? f_eff_ir_orig.y / f_eff_low.y : 0.0f,
			f_eff_low.z > 1e-6 ? f_eff_ir_orig.z / f_eff_low.z : 0.0f
		);
	}

	void normalizeUV(Vector2f& uv) const {
		uv.u = uv.u - floorf(uv.u);
		uv.v = uv.v - floorf(uv.v);
	}

	void estimateT(const Scene& sceneLow, const Scene& sceneOrig) {

		if (SpatialResolution == 1) {
			T[0] = Vector3f(1.0f);
			return;
		}

		std::vector<std::shared_ptr<Sampler>> samplers(omp_get_max_threads());
		for (int i = 0; i < samplers.size(); ++i) {
			samplers[i] = std::make_shared<Sampler>(i);
		}

		//int SampleNum = 5000;
		int SampleNum = 2500;
		Vector3f n(0, 0, 1);
#pragma omp parallel for schedule(dynamic, 1)
		for (int i = 0; i < T.size(); ++i) {
			T[i] = Vector3f(0.0f);
			for (int sample = 0; sample < SampleNum; ++sample) {
				Vector2f p = T.indexToPosition(i, *samplers[omp_get_thread_num()]);

				Vector3f wi = sampleVectorFromCosinedHemiSphere(n, *samplers[omp_get_thread_num()]);
				Vector3f wo = sampleVectorFromCosinedHemiSphere(n, *samplers[omp_get_thread_num()]);
				float prob_wi = clampPositive(dot(wi, n)) / M_PI;
				float prob_wo = clampPositive(dot(wo, n)) / M_PI;

				T[i] += estimateRir(p, wi, wo, sceneLow, sceneOrig, *samplers[omp_get_thread_num()]) / (prob_wi * prob_wo);
			}
			T[i] /= SampleNum;
			printf("unnormalized T[%d] = (%f, %f, %f)\n", i, T[i].r, T[i].g, T[i].b);
		}

		Vector3f C(0.0f);
		for (int i = 0; i < T.size(); ++i) {
			C += T[i];
		}
		C /= T.size();
		for (int i = 0; i < T.size(); ++i) {
			T[i] /= C;
			printf("%d / %d | T[%d] = (%f, %f, %f)\n", i + 1, T.size(), i, T[i].r, T[i].g, T[i].b);
		}

	}

	void estimateS(const Scene& sceneLow, const Scene& sceneOrig) {
		//printf("estimate S\n");

		//int SampleNum = 2500;
		int SampleNum = 25;

		std::vector<std::shared_ptr<Sampler>> samplers(omp_get_max_threads());
		for (int i = 0; i < samplers.size(); ++i) {
			samplers[i] = std::make_shared<Sampler>(i);
		}

#pragma omp parallel for schedule(dynamic, 1)
		for (int i = 0; i < S.size(); ++i) {
			S[i] = Vector3f(0.0f);
			for (int sample = 0; sample < SampleNum; ++sample) {
				Vector3f wi, wo;
				S.indexToVector(i, &wi, &wo, *samplers[omp_get_thread_num()]);

				auto& sampler = samplers[omp_get_thread_num()];
				Vector2f t = Vector2f(((i / SpatialResolution) % SpatialResolution + sampler->randf()) / SpatialResolution, ((i % SpatialResolution) + sampler->randf()) / SpatialResolution);
				Vector2f p = Vector2f(sampler->randf(), sampler->randf() );
				float prob_p = 1.0f;
				float kernel_p = 1.0f;

				S[i] += estimateRir(p, wi, wo, sceneLow, sceneOrig, *sampler) * kernel_p / prob_p;
			}
			S[i] /= SampleNum;

			printf("%d / %d | S[%d] = (%f, %f, %f)\n", i + 1, S.size(), i, S[i].r, S[i].g, S[i].b);
		}
	}

	Vector3f getMesoscaleNormal(const Bounds2f& patch) const {
		float h00 = displacementTexLow->rgb(patch.lerp(Vector2f(0, 0))).r;
		float h01 = displacementTexLow->rgb(patch.lerp(Vector2f(0, 1))).r;
		float h10 = displacementTexLow->rgb(patch.lerp(Vector2f(1, 0))).r;
		float h11 = displacementTexLow->rgb(patch.lerp(Vector2f(1, 1))).r;
		Vector2f s( (h11 + h10 - h01 - h00) / 2.0f, (h11 + h01 - h10 - h00) / 2.0f );
		return Vector3f(-s.u, -s.v, 1).normalize();
	}

	Bounds2f getPatch(const Vector2f& p) const {
		Bounds2f patch;
		Vector2f patchSize = Vector2f(1.0f) / SpatialResolution;
		int patchIndexU = (int)(p.u / patchSize.u * SpatialResolution);
		int patchIndexV = (int)(p.v / patchSize.v * SpatialResolution);
		patchIndexU = xitils::clamp(patchIndexU, 0, SpatialResolution - 1);
		patchIndexV = xitils::clamp(patchIndexV, 0, SpatialResolution - 1);
		patch.min.u = (float)patchIndexU / SpatialResolution;
		patch.min.v = (float)patchIndexV / SpatialResolution;
		patch.max = patch.min + patchSize;
		return patch;
	}

	Vector3f estimateEffectiveBRDFLow(const Vector2f& texelPos, const Vector3f& wi, const Vector3f& wo, const Scene& sceneLow, Sampler& sampler) const {
		// IR は考慮しない

		Vector3f res;
		const Vector3f wg(0, 0, 1);
		Bounds2f patch = getPatch(texelPos);

		//const int Sample = 100; // 何回評価する？
		const int Sample = 100;
		const float RayOffset = 1e-6;
		for (int s = 0; s < Sample; ++s) {
			xitils::Ray ray;
			SurfaceIntersection isect;

			Vector3f contrib(1.0f);

			// patch 内の位置をサンプル
#ifndef SAMPLE_P_WITH_EXPLICIT_RAYCAST
			Vector2f p(sampler.randf(patch.min.u, patch.max.u), sampler.randf(patch.min.v, patch.max.v));
			float prob_p = 1.0f / patch.area();
			float kernel_p = 1.0f / patch.area();

			ray.o = Vector3f(p.u, p.v, 99);
			ray.d = Vector3f(0, 0, -1);
			bool hit = sceneLow.intersect(ray, &isect);
			if (!hit) { printf("%f %f - %f %f | %f %f\n", patch.min.u, patch.max.u, patch.min.v, patch.max.v, p.u, p.v); }
			ASSERT(hit);

			xitils::Ray rayWo;
			rayWo.d = wo;
			rayWo.o = isect.p + rayWo.d * RayOffset;
			if (dot(wo, isect.shading.n) <= 0.0f || sceneLow.intersectAny(rayWo)) { continue; }

			Vector3f wm = getNormalLow(p);
			float A_G_p_wo = clampPositive(dot(wo, wm)) / clampPositive(dot(wg, wm)); // 係数 V は除いた値
			contrib *= kernel_p / prob_p * A_G_p_wo;
#else
			ray.d = -wo;
			Vector3f center(patch.center(), 0.0f);
			BasisVectors basis(ray.d);

			Vector3f e1 = ray.d;
			Vector3f v2 = Vector3f(patch.lerp(Vector2f(1, 1)) - patch.lerp(Vector2f(0, 0)), 0);
			Vector3f v3 = Vector3f(patch.lerp(Vector2f(1, 0)) - patch.lerp(Vector2f(0, 1)), 0);

			Vector3f e3 = cross(e1, v2).normalize();
			e3 *= dot(e3, v3) / 2.0f;

			Vector3f e2 = cross(e3, e1).normalize();
			e2 *= dot(e2, v2) / 2.0f;

			bool hit = false;
			do {
				float r2, r3;
				do {
					r2 = sampler.randf(-1.0f, 1.0f);
					r3 = sampler.randf(-1.0f, 1.0f);
				} while (!inRange(r2 + r3, -1.0f, 1.0f));

				ray.o = center + basis.e1 * -99 + basis.e2 * r2 + basis.e3 * r3;
				ray.tMax = Infinity;
				hit = sceneLow.intersect(ray, &isect);
				//if (hit)printf("%f %f | %f %f - %f %f\n",isect.p.x, isect.p.y, patch.min.u, patch.min.v, patch.max.u, patch.max.v);
			} while ( !hit || !inRange(isect.p.x, patch.min.u, patch.max.u) || !inRange(isect.p.y, patch.min.v, patch.max.v));
#endif

			xitils::Ray rayWi;
			rayWi.d = wi;
			rayWi.o = isect.p + rayWi.d * RayOffset;
			if (dot(wi, isect.shading.n) <= 0.0f || sceneLow.intersectAny(rayWi)) { continue; }
			
			contrib *= isect.object->material->bsdfCos(isect, sampler, wi);

			res += clampPositive(contrib);

		}
		res /= Sample;

#ifndef SAMPLE_P_WITH_EXPLICIT_RAYCAST
		Vector3f wn = getMesoscaleNormal(patch);
		float A_G_wo = clampPositive(dot(wo, wn)) / clampPositive(dot(wg, wn));
		if (A_G_wo > 1e-6) {
			res /= A_G_wo;
		} else {
			res = Vector3f(0.0f);
		}
#endif
		return res;
	}

	Vector3f getNormalLow(const Vector2f& texelPos) const {
		Vector3f a;
		a.x = -displacementTexLow->rgbDifferentialU(texelPos).x * displacementScale;
		a.y = -displacementTexLow->rgbDifferentialV(texelPos).x * displacementScale;
		a.z = 1;
		a.normalize();

		return a;
	}

	Vector3f estimateEffectiveBRDFIROrig(const Vector2f& texelPos, const Vector3f& wi, const Vector3f& wo, const Scene& sceneOrig, Sampler& sampler) const {
		// IR を考慮する

		Vector3f res;
		const Vector3f wg(0, 0, 1);
		Bounds2f patch = getPatch(texelPos);

		//const int Sample = 2500;
		const int Sample = 500;
		const float RayOffset = 1e-6;
		for (int s = 0; s < Sample; ++s) {
			xitils::Ray ray;
			SurfaceIntersection isect;

			Vector3f weight(1.0f);
			int pathLength = 1;

			// patch 内の位置をサンプル
#ifndef SAMPLE_P_WITH_EXPLICIT_RAYCAST
			// texelPos を含むパッチ内で始点となる位置をサンプル
			Vector2f p(sampler.randf(patch.min.u, patch.max.u), sampler.randf(patch.min.v, patch.max.v));
			float prob_p = 1.0f / patch.area();
			float kernel_p = 1.0f / patch.area();

			ray.o = Vector3f(p.u, p.v, 99);
			ray.d = Vector3f(0, 0, -1);
			bool hit = sceneOrig.intersect(ray, &isect);
			ASSERT(hit);

			xitils::Ray rayWo;
			rayWo.d = wo;
			rayWo.o = isect.p + rayWo.d * RayOffset;
			if (dot(wo, isect.shading.n) <= 0.0f || sceneOrig.intersectAny(rayWo)) { continue; }

			const Vector3f& wm = isect.shading.n;
			float A_G_p_wo = clampPositive(dot(wo, wm)) / clampPositive(dot(wg, wm)); // 係数 V は除いた値

			weight *= A_G_p_wo * kernel_p / prob_p;
#else
			ray.d = -wo;
			Vector3f center(patch.center(), 0.0f);
			BasisVectors basis(ray.d);

			Vector3f e1 = ray.d;
			Vector3f v2 = Vector3f(patch.lerp(Vector2f(1, 1)) - patch.lerp(Vector2f(0, 0)), 0);
			Vector3f v3 = Vector3f(patch.lerp(Vector2f(1, 0)) - patch.lerp(Vector2f(0, 1)), 0);

			Vector3f e3 = cross(e1, v2).normalize();
			e3 *= dot(e3, v3) / 2.0f;

			Vector3f e2 = cross(e3, e1).normalize();
			e2 *= dot(e2, v2) / 2.0f;

			bool hit = false;
			do {
				float r2, r3;
				do {
					r2 = sampler.randf(-1.0f, 1.0f);
					r3 = sampler.randf(-1.0f, 1.0f);
				} while (!inRange(r2 + r3, -1.0f, 1.0f));

				ray.o = center + basis.e1 * -99 + basis.e2 * r2 + basis.e3 * r3;
				ray.tMax = Infinity;
				hit = sceneOrig.intersect(ray, &isect);
				//if (hit)printf("%f %f | %f %f - %f %f\n",isect.p.x, isect.p.y, patch.min.u, patch.min.v, patch.max.u, patch.max.v);
			} while (!hit || !inRange(isect.p.x, patch.min.u, patch.max.u) || !!inRange(isect.p.y, patch.min.v, patch.max.v));
#endif

			while (true) {
				xitils::Ray rayWi;
				rayWi.d = wi;
				rayWi.o = isect.p + rayWi.d * RayOffset;
				if (dot(wi, isect.shading.n) > 0.0f && !sceneOrig.intersectAny(rayWi)) {
					res += clampPositive(weight * isect.object->material->bsdfCos(isect, sampler, wi));
				}

				xitils::Ray rayNext;
				float pdf;
				weight *= isect.object->material->evalAndSample(isect, sampler, &rayNext.d, &pdf);
				rayNext.o = isect.p + rayNext.d * RayOffset;
				if (!sceneOrig.intersect(rayNext, &isect)) { break; }

				++pathLength;
				if (pathLength > 20 || weight.isZero()) { break; }

			}

		}
		res /= Sample;
		
#ifndef SAMPLE_P_WITH_EXPLICIT_RAYCAST
		Vector3f wn = getMesoscaleNormal(patch);
		float A_G_wo = clampPositive(dot(wo, wn)) / clampPositive(dot(wg, wn));
		if (A_G_wo > 1e-6) {
			res /= A_G_wo;
		} else {
			res = Vector3f(0.0f);
		}
#endif
		return res;
	}
};


struct MyFrameData {
	float initElapsed;
	float frameElapsed;
	Surface surface;
	int sampleNum = 0;
};

struct MyUIFrameData {
	Vector3f rot;
};

class MyApp : public xitils::app::XApp<MyFrameData, MyUIFrameData> {
public:
	void onSetup(MyFrameData* frameData, MyUIFrameData* uiFrameData) override;
	void onCleanup(MyFrameData* frameData, MyUIFrameData* uiFrameData) override;
	void onUpdate(MyFrameData& frameData, const MyUIFrameData& uiFrameData) override;
	void onDraw(const MyFrameData& frameData, MyUIFrameData& uiFrameData) override;

private:
	
	gl::TextureRef texture;

	std::shared_ptr<Scene> scene;
	inline static const glm::ivec2 ImageSize = glm::ivec2(800, 600);

	std::shared_ptr<RenderTarget> renderTarget;
	std::shared_ptr<PathTracer> pathTracer;

	decltype(std::chrono::system_clock::now()) time_start;
};

void MyApp::onSetup(MyFrameData* frameData, MyUIFrameData* uiFrameData) {
	auto init_time_start = std::chrono::system_clock::now();
	
	frameData->surface = Surface(ImageSize.x, ImageSize.y, false);
	frameData->frameElapsed = 0.0f;
	frameData->sampleNum = 0;

	getWindow()->setTitle("Xitils");
	setWindowSize(ImageSize);
	setFrameRate(60);

	ui::initialize();

	scene = std::make_shared<Scene>();

	scene->camera = std::make_shared<PinholeCamera>(
		transformTRS(Vector3f(0,1.2f,-4), Vector3f(10, 0, 0), Vector3f(1)), 
		40 * ToRad, 
		(float)ImageSize.y / ImageSize.x
		);

	// cornell box
	auto diffuse_white = std::make_shared<Diffuse>(Vector3f(0.6f));
	auto diffuse_red = std::make_shared<Diffuse>(Vector3f(0.8f, 0.1f, 0.1f));
	auto diffuse_green = std::make_shared<Diffuse>(Vector3f(0.1f, 0.8f, 0.1f));
	auto emission = std::make_shared<Emission>(Vector3f(1.0f, 1.0f, 0.95f) * 8);
	auto cube = std::make_shared<xitils::Cube>();
	auto plane = std::make_shared<xitils::Plane>();
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


	auto baseMaterial = std::make_shared<GlossyWithHighLight>(Vector3f(0.8f, 0.2f, 0.2f), Vector3f(1.0f), 2.0f, 10.0f, 0.05f);

	const float DispScale = 0.01f;

	auto dispTexOrig = std::make_shared<Texture>("disp_fabric.jpg");
	dispTexOrig->warpClamp = false;

	if (MethodMode == MethodModeReference) {
		auto cloth = std::make_shared<TriangleMesh>();
		cloth->setGeometryWithShellMapping(*std::make_shared<TriMesh>(ObjLoader(loadFile("cloth.obj"))), dispTexOrig, DispScale, ShellMappingLayerNum);
		scene->addObject(
			std::make_shared<Object>(cloth, baseMaterial, transformTRS(Vector3f(0, -0.025f, 0), Vector3f(0, 0, 0), Vector3f(1.0f)))
		);
	} else if (MethodMode == MethodModeNaive) {
		auto svndf = std::make_shared<SVNDF>();
		auto dispTexLow = downsampleDisplacementTexture(dispTexOrig, DispScale, svndf);
		auto cloth = std::make_shared<TriangleMesh>();
		cloth->setGeometryWithShellMapping(*std::make_shared<TriMesh>(ObjLoader(loadFile("cloth.obj"))), dispTexLow, DispScale, ShellMappingLayerNum);
		scene->addObject(
			std::make_shared<Object>(cloth, baseMaterial, transformTRS(Vector3f(0, -0.03f, 0), Vector3f(0, 0, 0), Vector3f(1.0f)))
		);
	} else if (MethodMode == MethodModeMultiLobeSVBRDF) {
		auto svndf = std::make_shared<SVNDF>();
		auto dispTexLow = downsampleDisplacementTexture(dispTexOrig, DispScale, svndf);
		auto multiLobeSVBRDF = std::make_shared<MultiLobeSVBRDF>(baseMaterial, dispTexLow, DispScale, svndf);
		auto cloth = std::make_shared<TriangleMesh>();
		cloth->setGeometryWithShellMapping(*std::make_shared<TriMesh>(ObjLoader(loadFile("cloth.obj"))), dispTexLow, DispScale, ShellMappingLayerNum);
		scene->addObject(
			std::make_shared<Object>(cloth, multiLobeSVBRDF, transformTRS(Vector3f(0, -0.03f, 0), Vector3f(0, 0, 0), Vector3f(1.0f)))
		);
	} else if (MethodMode == MethodModeProposed) {
		auto prefilteredDispMaterial = std::make_shared<PrefilteredDisplaceMapping>(baseMaterial, dispTexOrig, DispScale);
		auto cloth = std::make_shared<TriangleMesh>();
		cloth->setGeometryWithShellMapping(*std::make_shared<TriMesh>(ObjLoader(loadFile("cloth.obj"))), prefilteredDispMaterial->getDisplacementTextureLow(), DispScale, ShellMappingLayerNum);
		scene->addObject(
			std::make_shared<Object>(cloth, prefilteredDispMaterial, transformTRS(Vector3f(0, -0.03f, 0), Vector3f(0, 0, 0), Vector3f(1.0f)))
		);
	}

	scene->buildAccelerationStructure();

	renderTarget = std::make_shared<RenderTarget>(ImageSize.x, ImageSize.y);

	pathTracer = std::make_shared<StandardPathTracer>();

	auto time_end = std::chrono::system_clock::now();

	frameData->initElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - init_time_start).count();

	time_start = std::chrono::system_clock::now();
}

void MyApp::onCleanup(MyFrameData* frameData, MyUIFrameData* uiFrameData) {

}

void MyApp::onUpdate(MyFrameData& frameData, const MyUIFrameData& uiFrameData) {
	auto time_start = std::chrono::system_clock::now();

	int sample = 1;

	frameData.sampleNum += sample;

	renderTarget->render(*scene, sample, [&](const Vector2f& pFilm, Sampler& sampler, Vector3f& color) {
		auto ray = scene->camera->GenerateRay(pFilm, sampler);

		color += pathTracer->eval(*scene, sampler, ray);
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
	ImGui::Text(("Elapsed : " + std::to_string(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - time_start).count()) + " s").c_str());
	ImGui::Text(("Samples: " + std::to_string(frameData.sampleNum)).c_str());

	ImGui::End();
}


XITILS_APP(MyApp)
