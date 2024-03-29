﻿

#include <Xitils/AccelerationStructure.h>
#include <Xitils/App.h>
#include <Xitils/Camera.h>
#include <Xitils/Geometry.h>
#include <Xitils/PathTracer.h>
#include <Xitils/Scene.h>
#include <Xitils/TriangleMesh.h>
#include <Xitils/RenderTarget.h>
#include <CinderImGui.h>
#include <cinder/ObjLoader.h>

#include <OpenImageDenoise/oidn.hpp>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#pragma warning(push)
#pragma warning(disable : 4996)
// cinder のと干渉するので無名空間で囲ってる
namespace {
#include <stb/stb_image_write.h>
}
#pragma warning(pop)

#include <queue>

using namespace xitils;
using namespace ci;
using namespace ci::app;
using namespace ci::geom;

#define SCENE_DURATION 6.0f
#define FRAME_PER_SECOND 30
#define TOTAL_FRAME_COUNT (SCENE_DURATION * FRAME_PER_SECOND)

#define ENABLE_DENOISE true
#define SHOW_WINDOW false
#define ENABLE_SAVE_IMAGE true

#define IMAGE_WIDTH 1920
#define IMAGE_HEIGHT 1080

static float g_time = 0.0f;

// Position-free Multiple-bounce Computations for Smith Microfacet BSDFs [Wang et al. 2022]
class MetalWithMultipleScattering : public Material
{
public:
	float alpha() const
	{
		float roughness = lerp(0.1f, 1.0f, clamp01((g_time - 2.5f) / 2.0f));
		return roughness * roughness;
	}

	Vector3f f0;
	bool ssOnly = false;

	MetalWithMultipleScattering(float roughness, const Vector3f& f0, bool ssOnly = false) :
		//alpha(roughness* roughness),
		f0(f0),
		ssOnly(ssOnly)
	{
	}

	Vector3f F_Reflection(const Vector3f& eye, const Vector3f& h) const
	{
		//return Vector3f(1);
		return f0 + (Vector3f(1.0f) - f0) * pow(1 - dot(eye, h), 5.0f);
	}

	float D_GGX(const Vector3f& x, const Vector3f& n) const
	{
		float alpha2 = alpha() * alpha();
		float cosThetaX = dot(n, x);
		float cosThetaX2 = cosThetaX * cosThetaX;
		float cosThetaX4 = cosThetaX2 * cosThetaX2;
		float tanThetaX2 = 1 / cosThetaX2 - 1;

		//return alpha2 * heavisideStep(dot(n, x)) / (M_PI * cosThetaH4 * pow(alpha2 + tanThetaH2, 2));

		float sinThetaX2 = 1 - cosThetaX2;
		return alpha2 / (M_PI * pow(sinThetaX2 + alpha2 * cosThetaX2, 2));
	}

	float D_V_GGX(const Vector3f& x, const Vector3f& n, const Vector3f& eye) const
	{
		return G1_Smith(eye, n) * clampPositive(dot(eye, x)) * D_GGX(x, n) / dot(eye, n);
	}

	float lambda(const Vector3f& x, const Vector3f& n) const
	{
		float alpha2 = alpha() * alpha();
		float cosThetaX = dot(n, x);
		float cosThetaX2 = cosThetaX * cosThetaX;
		float tanThetaX2 = 1 / cosThetaX2 - 1;
		return (-1 + sign(cosThetaX) * safeSqrt(1 + alpha2 * tanThetaX2)) / 2;


		//if (cosThetaX > 0.9999999f) { return 0; }
		//if (cosThetaX < -0.9999999f) { return -1.0f; }

		//float theta = acosf(cosThetaX);
		//float a = 1.0f / tanf(theta) / alpha;

		//return 0.5f * (-1.0f + sign(a) * safeSqrt(1 + 1 / (a * a)));
	}

	float G1_FullSphere(const Vector3f& x, const Vector3f& n, const Vector3f& h) const
	{
		float G1_local = heavisideStep(dot(x, h));
		//if (G1_local == 0) { return 0; }

		//if (lambda(x, n) < -0.5f) { return 0; }
		float G1_dist = (1 / (1 + lambda(x, n))) * sign(dot(x,n));

		return G1_local * G1_dist;
	}

	float G1_Smith(const Vector3f& x, const Vector3f& n) const
	{
		return 1 / (1 + lambda(x, n)) * sign(dot(x, n));
	}

	Vector3f sampleGGXVNDF(const Vector3f& eye, const Vector3f& n, Sampler& sampler) const
	{
		// Sampling the GGX Distribution of Visible Normals [Heitz 2018]

		auto basis = BasisVectors(n);
		Vector3f Ve = basis.fromGlobal(eye);
		Vector3f Vh = Vector3f(Ve.x, alpha() * Ve.y, alpha() * Ve.z).normalize();
		float lensq = Vh.y * Vh.y + Vh.z * Vh.z;
		Vector3f T1 = lensq > 0 ? Vector3f(0, -Vh.z, Vh.y) / safeSqrt(lensq) : Vector3f(0, 0, 1);
		Vector3f T2 = cross(Vh, T1);
		float U1 = sampler.randf();
		float U2 = sampler.randf();
		float r = sqrt(U1);
		float phi = 2.0 * M_PI * U2;
		float t1 = r * cos(phi);
		float t2 = r * sin(phi);
		float s = 0.5 * (1.0 + Vh.x);
		t2 = (1.0 - s) * sqrt(1.0 - t1 * t1) + s * t2;
		Vector3f Nh = t1 * T1 + t2 * T2 + safeSqrt(max(0.0, 1.0 - t1 * t1 - t2 * t2)) * Vh;
		Vector3f Ne = normalize(Vector3f(std::max<float>(0.0, Nh.x), alpha() * Nh.y, alpha() * Nh.z));
		Vector3f h = basis.toGlobal(Ne);
		return h;
	}

	Vector3f bsdfCos(const SurfaceIntersection& isect, Sampler& sampler, const Vector3f& wi) const override {
		const auto& wo = isect.wo;
		const auto& n = isect.shading.n;

		// 透過は端折ってる
		// BDPT も実装してない
		// heig-uncorrelated のみ

		bool ommitMS = ssOnly || alpha() < 0.2 * 0.2f;

		const int sampleNum = ommitMS ? 1 : 4;
		Vector3f bsdfCos(0);

		auto s = [&](int i, const Vector3f& d_i, const Vector3f& h_iminus1, const Vector3f& h_i, bool exit) {
			float e_i, p_i;
			if(!exit)
			{
				if (i == 0)
				{
					e_i = 1;
				} else
				{
					e_i = dot(d_i, n) > 0 ? (1 - G1_FullSphere(d_i, n, h_iminus1)) : 1;
				}
				p_i = G1_FullSphere(-d_i, n, h_i);
			}
			else
			{
				e_i = dot(d_i, n) > 0 ? G1_FullSphere(d_i, n, h_iminus1) : 0;
				p_i = 1;
			}
			return e_i * p_i;
		};

		for(int sample = 0; sample < sampleNum; sample++)
		{
			float pdf = 1;
			Vector3f weight(1);

			Vector3f d_i = -wo;
			Vector3f h_iminus1(0);
			for (int i = 0; true; i++)
			{
				const float CONTINUE_PROBABILITY = 0.5f;
				if(i > 0)
				{
					if (sampler.randf() > CONTINUE_PROBABILITY)
					{
						break;
					}
					weight /= CONTINUE_PROBABILITY;
				}

				{
					// NEE
					Vector3f d_iplus1 = wi;
					Vector3f h_i = (-d_i + d_iplus1).normalize();
					float s_i = s(i, d_i, h_iminus1, h_i, false);
					float s_iplus1 = s(i + 1, d_iplus1, h_i, Vector3f(0), true);

					Vector3f Fr = F_Reflection(-d_i, h_i);
					float D = D_GGX(h_i, n);
					//if (abs(dot(-d_i, n)) < 0.00001f) { continue; }
					Vector3f v_i = Fr * D / abs(4 * dot(-d_i, n));

					bsdfCos += weight * (s_i * v_i * s_iplus1) / pdf;
					//printf("%f %f %f %f %f\n", weight.x, s_i, v_i.x, s_iplus1, pdf);
				}

				if(ommitMS)
				{
					break;
				}

				const auto h_i = sampleGGXVNDF(-d_i, n, sampler);
				Vector3f d_iplus1 = 2 * dot(-d_i, h_i) * h_i - (-d_i);
				pdf *= D_V_GGX(h_i, n, -d_i) / (4 * dot(-d_i, h_i));

				//const auto h_i = sampleGGXVNDF(n, n, sampler);
				//Vector3f d_iplus1 = 2 * dot(-d_i, h_i) * h_i - (-d_i);
				//pdf *= D_V_GGX(h_i, n, n) / (4 * dot(n, h_i));

				//auto h_i = sampleGGXVNDF(wo, n, sampler);
				//Vector3f d_iplus1 = 2 * dot(-d_i, h_i) * h_i - (-d_i);
				//pdf *= D_V_GGX(h_i, n, wo) / (4 * dot(wo, h_i));


				//Vector3f d_iplus1 = sampleVectorFromHemiSphere(n, sampler);
				//pdf *= 1 / (2 * M_PI);
				//Vector3f h_i = (-d_i + d_iplus1).normalize();

				//Vector3f d_iplus1 = sampleVectorFromCosinedHemiSphere(n, sampler);
				//pdf *= dot(d_iplus1, n) / M_PI;
				//Vector3f h_i = (-d_i + d_iplus1).normalize();

				//if (pdf == 0) { break; }

				float s_i = s(i, d_i, h_iminus1, h_i, false);

				//if (abs(dot(-d_i, n)) < 0.00001f) { break; }
				Vector3f Fr = F_Reflection(-d_i, h_i);
				float D = D_GGX(h_i, n);
				Vector3f v_i = Fr * D / abs(4 * dot(-d_i, n));
				
				weight *= s_i * v_i;
				if (weight.lengthSq() <= 0.0000001f) { break; }

				d_i = d_iplus1;
				h_iminus1 = h_i;
			}
		}
		bsdfCos /= sampleNum;

		return bsdfCos;
	}

	Vector3f evalAndSample(const SurfaceIntersection& isect, Sampler& sampler, Vector3f* wi, float* pdf) const override {
		const auto& wo = isect.wo;
		const auto& n = isect.shading.n;


		auto h = sampleGGXVNDF(wo, n, sampler);
		*wi = 2 * dot(wo, h) * h - wo;

		//*wi = sampleVectorFromCosinedHemiSphere(n, sampler);

		*pdf = getPDF(isect, *wi);

		return bsdfCos(isect, sampler, *wi) / *pdf;
	}

	float getPDF(const SurfaceIntersection& isect, const Vector3f& wi) const override {
		const auto& wo = isect.wo;
		const auto& n = isect.shading.n;
		const auto h = (wi + wo).normalize();

		//return dot(wi, n) / M_PI;
		return D_V_GGX(h, n, wo) / (4 * dot(wo, h));
	}

	Vector3f getAlbedo(const SurfaceIntersection& isect) const override {
		return f0;
	}

};

class ChekerDiffuse : public Material {
public:
	Vector3f albedo;

	Vector3f getChecker(const SurfaceIntersection& isect) const
	{
		const auto& p = isect.p;
		return (int)(floorf(p.x + 0.5f) + floorf(p.y + 0.5f) + floorf(p.z + 0.5f)) % 2 ==0 ? Vector3f(1.0f) : Vector3f(0.1f);
	}

	ChekerDiffuse(const Vector3f& albedo) :
		albedo(albedo)
	{}

	Vector3f bsdfCos(const SurfaceIntersection& isect, Sampler& sampler, const Vector3f& wi) const override {
		return albedo / M_PI * getChecker(isect) * clampPositive(dot(isect.shading.n, wi));
	}

	Vector3f evalAndSample(const SurfaceIntersection& isect, Sampler& sampler, Vector3f* wi, float* pdf) const override {
		const auto& n = isect.shading.n;
		*wi = sampleVectorFromCosinedHemiSphere(n, sampler);
		*pdf = dot(*wi, n) / M_PI;
		return albedo * getChecker(isect);
	}

	float getPDF(const SurfaceIntersection& isect, const Vector3f& wi) const override {
		const auto& n = isect.shading.n;
		return clampPositive(dot(wi, n)) / M_PI;
	}

	Vector3f getAlbedo(const SurfaceIntersection& isect) const override {
		return albedo * getChecker(isect);
	}
};



class ImageSaveThread
{
public:
	void start()
	{
		thread = std::make_shared<std::thread>([&] {
			this->mainLoop();
			});
	}

	void release()
	{
		if (thread) {
			{
				std::lock_guard lock(mtx);
				threadClosing = true;
			}
			thread->join();
		}
	}

	void push(const std::shared_ptr<Surface>& image)
	{
		std::lock_guard lock(mtx);
		images.push(image);
	}

private:
	std::mutex mtx;
	std::shared_ptr<std::thread> thread;
	bool threadClosing = false;
	std::queue<std::shared_ptr<Surface>> images;
	int sequentialNum = 0;

	std::shared_ptr<Surface> pop()
	{
		if (images.empty()) { return nullptr; }
		std::lock_guard lock(mtx);
		auto front = images.front();
		images.pop();
		return front;
	}

	void mainLoop()
	{
		while (true) {

			if(auto image = pop())
			{
				std::string seq = std::to_string(sequentialNum + 1); // ファイル名は 1 始まり
				while (seq.length() < 3) { seq = "0" + seq; }
				// HeadLess 起動だと何故か動かない。cinderのコンテキストに依存している？
				//writeImage(seq + ".png", *image);

				std::string filename = seq + ".png";
				::stbi_write_png(filename.c_str(),
					image->getWidth(),
					image->getHeight(),
					3,
					image->getData(),
					image->getWidth() * 3);

				++sequentialNum;
			}

			if(images.empty())
			{
				std::lock_guard lock(mtx);
				if (threadClosing) { break; }
			}

			std::this_thread::yield();
		}
	}
};



struct MyFrameData {
	float initElapsed;
	std::chrono::time_point<std::chrono::system_clock> timeAppStart;
	float frameElapsed;
	int triNum;
	int frameCount = 0;
	std::shared_ptr<Surface> surface;
};

struct MyUIFrameData {
	Vector3f rot;
};

class MyApp :
#if SHOW_WINDOW
public xitils::app::XApp<MyFrameData, MyUIFrameData>
#else
	public xitils::app::XAppHeadless<MyFrameData, MyUIFrameData>
#endif
{
public:
	void onSetup(MyFrameData* frameData, MyUIFrameData* uiFrameData) override;
	void onCleanup(MyFrameData* frameData, MyUIFrameData* uiFrameData) override;
	void onUpdate(MyFrameData& frameData, const MyUIFrameData& uiFrameData) override;
#if SHOW_WINDOOW
	void onDraw(const MyFrameData& frameData, MyUIFrameData& uiFrameData) override;
#endif

private:
	ImageSaveThread	imageSaveThread;
	
	gl::TextureRef texture;

	std::shared_ptr<Scene> scene;
	inline static const glm::ivec2 ImageSize = glm::ivec2(IMAGE_WIDTH, IMAGE_HEIGHT);

	std::shared_ptr<PathTracer> pathTracer;

	oidn::DeviceRef device;

	decltype(std::chrono::system_clock::now()) time_start;
};

void MyApp::onSetup(MyFrameData* frameData, MyUIFrameData* uiFrameData) {
	time_start = std::chrono::system_clock::now();
	auto init_time_start = std::chrono::system_clock::now();


	frameData->timeAppStart = std::chrono::system_clock::now();
	
	frameData->frameElapsed = 0.0f;
	frameData->triNum = 0;

#if SHOW_WINDOW
	getWindow()->setTitle("Xitils");
	setWindowSize(ImageSize);
	setFrameRate(60);
	ui::initialize();
#endif

	imageSaveThread.start();


	scene = std::make_shared<Scene>();

	auto camera = std::make_shared<PinholeCamera>(
		translate(-5, 1.0f, -5), 60 * ToRad, (float)ImageSize.y / ImageSize.x
		);
	scene->camera = camera;
	camera->addKeyFrame(
		2.0f,
		translate(5, 1.0f, -5), 60 * ToRad);
	//camera->addKeyFrame(
	//	2.001f,
	//	transformTRS(Vector3f(0, 4.0f, -6), Vector3f(30,0,0), Vector3f(1))
	//	, 60 * ToRad);
	camera->addKeyFrame(
		2.001f,
		transformTRS(Vector3f(-2, 4.5f, -6), Vector3f(35, 10, 0), Vector3f(1))
		, 70 * ToRad);
	camera->addKeyFrame(
		SCENE_DURATION,
		transformTRS(Vector3f(2, 4.5f, -5), Vector3f(35, -20, 0), Vector3f(1))
		, 70 * ToRad);


	// cornell box
	auto diffuse_white = std::make_shared<ChekerDiffuse>(Vector3f(0.8f));
	auto cube = std::make_shared<xitils::Cube>();
	scene->addObject(
		std::make_shared<Object>( cube, diffuse_white, transformTRS(Vector3f(0,-5.0f,1.5f), Vector3f(), Vector3f(8, 10.0f, 8)))
	);
	////scene->addObject(
	//	std::make_shared<Object>(cube, diffuse_green, transformTRS(Vector3f(2, 2, 0), Vector3f(), Vector3f(0.01f, 4, 4)))
	//);
	//scene->addObject(
	//	std::make_shared<Object>(cube, diffuse_red, transformTRS(Vector3f(-2, 2, 0), Vector3f(), Vector3f(0.01f, 4, 4)))
	//);
	//scene->addObject(
	//	std::make_shared<Object>(cube, diffuse_white, transformTRS(Vector3f(0, 2, 2), Vector3f(), Vector3f(4, 4, 0.01f)))
	//);
	//scene->addObject(
	//	std::make_shared<Object>(cube, diffuse_white, transformTRS(Vector3f(0, 4, 0), Vector3f(), Vector3f(4, 0.01f, 4)))
	//);
	//scene->addObject(
	//	std::make_shared<Object>(plane, emission, transformTRS(Vector3f(0, 4.0f -0.01f, 0), Vector3f(-90,0,0), Vector3f(2.0f)))
	//);
	scene->skySphere = std::make_shared<SkySphereFromImage>("data/rnl_probe.hdr");
	//scene->skySphere = std::make_shared<SkySphereUniform>(Vector3f(0.5f));

	// teapot 作成
	const int subdivision = 10;
	auto teapot = std::make_shared<Teapot>();
	teapot->subdivisions(subdivision);
	auto teapotMeshData = std::make_shared<TriMesh>(*teapot);
	auto teapotMesh = std::make_shared<TriangleMesh>();
	teapotMesh->setGeometry(*teapotMeshData);

	auto meshR = std::make_shared<TriangleMesh>();
	auto meshT = std::make_shared<TriangleMesh>();
	auto mesh8 = std::make_shared<TriangleMesh>();
	meshR->setGeometry(*std::make_shared<TriMesh>(TriMesh(ObjLoader(loadFile("data/R.obj")))));
	meshT->setGeometry(TriMesh(ObjLoader(loadFile("data/T.obj"))));
	mesh8->setGeometry(TriMesh(ObjLoader(loadFile("data/8.obj"))));


	auto goldSS = std::make_shared<MetalWithMultipleScattering>(0.25f, Vector3f(1.022f, 0.782f, 0.344f),true);
	auto silverSS = std::make_shared<MetalWithMultipleScattering>(0.25f, Vector3f(0.972f, 0.960f, 0.915f), true);
	auto copperSS = std::make_shared<MetalWithMultipleScattering>(0.25f, Vector3f(0.955f, 0.638f, 0.652f), true);
	auto ironSS = std::make_shared<MetalWithMultipleScattering>(0.25f, Vector3f(0.562f, 0.565f, 0.578f), true);
	auto goldMS = std::make_shared<MetalWithMultipleScattering>(0.25f, Vector3f(1.022f, 0.782f, 0.344f));
	auto silverMS = std::make_shared<MetalWithMultipleScattering>(0.25f, Vector3f(0.972f, 0.960f, 0.915f));
	auto copperMS = std::make_shared<MetalWithMultipleScattering>(0.25f, Vector3f(0.955f, 0.638f, 0.652f));
	auto ironMS = std::make_shared<MetalWithMultipleScattering>(0.25f, Vector3f(0.562f, 0.565f, 0.578f));

	scene->addObject(std::make_shared<Object>(meshR, goldSS,
		transformTRS(Vector3f(-2.0f, 0, 3.0f), Vector3f(0, 0, 0), Vector3f(1.5f))
		));
	scene->addObject(std::make_shared<Object>(meshT, silverSS,
		transformTRS(Vector3f(0.0f, 0, 3.0f), Vector3f(0, 0, 0), Vector3f(1.5f))
		));
	scene->addObject(std::make_shared<Object>(mesh8, copperSS,
		transformTRS(Vector3f(2.0f, 0, 3.0f), Vector3f(0, 0, 0), Vector3f(1.5f))
		));
	scene->addObject(std::make_shared<Object>(teapotMesh, ironSS,
		transformTRS(Vector3f(0.0f, 0, 4.5f), Vector3f(0, 0, 0), Vector3f(1.0f))
		));
	scene->addObject(std::make_shared<Object>(meshR, goldMS,
		transformTRS(Vector3f(-2.0f, 0, 0.0f), Vector3f(0, 0, 0), Vector3f(1.5f))
		));
	scene->addObject(std::make_shared<Object>(meshT, silverMS,
		transformTRS(Vector3f(0.0f, 0, 0.0f), Vector3f(0, 0, 0), Vector3f(1.5f))
		));
	scene->addObject(std::make_shared<Object>(mesh8, copperMS,
		transformTRS(Vector3f(2.0f, 0, 0.0f), Vector3f(0, 0, 0), Vector3f(1.5f))
		));
	scene->addObject(std::make_shared<Object>(teapotMesh, ironMS,
		transformTRS(Vector3f(0.0f, 0, -1.5f), Vector3f(0, 0, 0), Vector3f(1.0f))
		));
	//auto sphere = std::make_shared<cinder::geom::Sphere>();
	//sphere->subdivisions(100);
	//auto sphereMeshData = std::make_shared<TriMesh>(*sphere);
	//auto sphereMesh = std::make_shared<TriangleMesh>();
	//sphereMesh->setGeometry(*sphereMeshData);
	//scene->addObject(std::make_shared<Object>(sphereMesh, teapotMaterial1,
	//	transformTRS(Vector3f(-4.0f, 2, 0.0f), Vector3f(0, 0, 0), Vector3f(1.5f))
	//	));
	//scene->addObject(std::make_shared<Object>(sphereMesh, teapotMaterial2,
	//	transformTRS(Vector3f(0.0f, 2, 0.0f), Vector3f(0, 0, 0), Vector3f(1.5f))
	//	));
	//scene->addObject(std::make_shared<Object>(sphereMesh, teapotMaterial3,
	//	transformTRS(Vector3f(4.0f, 2, 0.0f), Vector3f(0, 0, 0), Vector3f(1.5f))
	//	));

	scene->buildAccelerationStructure();


	// ****************************************************************************************************************************************
	//pathTracer = std::make_shared<StandardPathTracer>();
	pathTracer = std::make_shared<NaivePathTracer>();

	auto time_end = std::chrono::system_clock::now();

	frameData->initElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - init_time_start).count();
	//frameData->triNum = teapotMeshData->getNumTriangles();


	device = oidn::newDevice();
	device.commit();

	// fps の数値を fps.txt に書き込む
	//{
	//	auto file = writeFile("fps.txt");
	//	writeString(file, std::to_string(FRAME_PER_SECOND));
	//}
}

void MyApp::onCleanup(MyFrameData* frameData, MyUIFrameData* uiFrameData) {
	imageSaveThread.release();
}

void MyApp::onUpdate(MyFrameData& frameData, const MyUIFrameData& uiFrameData) {

	if (frameData.frameCount >= TOTAL_FRAME_COUNT) { return; }

	auto time_start = std::chrono::system_clock::now();

	float time = (float)frameData.frameCount / FRAME_PER_SECOND;
	g_time = time;

	int sample = time < 2.0f ? 2 : 8;

	auto renderTarget = std::make_shared<DenoisableRenderTarget>(ImageSize.x, ImageSize.y);
	scene->camera->setCurrentTime(time);
	renderTarget->render(*scene, sample, [&](const Vector2f& pFilm, Sampler& sampler, DenoisableRenderTargetPixel& pixel) {
		auto ray = scene->camera->generateRay(pFilm, sampler);

		auto res = pathTracer->eval(*scene, sampler, ray);
		pixel.color += res.color;
		pixel.albedo += res.albedo;
		pixel.normal += res.normal;
		});

	renderTarget->map([&](DenoisableRenderTargetPixel& pixel)
	{
		pixel /= sample;
		pixel.normal = clamp01(pixel.normal * 0.5f + Vector3f(1.0f));
	});

	frameData.surface = Surface::create(ImageSize.x, ImageSize.y, false);

	// TODO : map が shared_ptr<Surface> を受け取るようにする
#if ENABLE_DENOISE

	auto denoisedRenderTarget = std::make_shared<SimpleRenderTarget>(ImageSize.x, ImageSize.y);

	int colorOffset = 0;
	int albedoOffset = colorOffset + sizeof(Vector3f);
	int normalOffset = albedoOffset + sizeof(Vector3f);

	int width = renderTarget->width;
	int height = renderTarget->height;
	oidn::FilterRef filter = device.newFilter("RT");
	filter.setImage("color", renderTarget->data.data(), oidn::Format::Float3, width, height, colorOffset, sizeof(DenoisableRenderTargetPixel));
	filter.setImage("albedo", renderTarget->data.data(), oidn::Format::Float3, width, height, albedoOffset, sizeof(DenoisableRenderTargetPixel));
	filter.setImage("normal", renderTarget->data.data(), oidn::Format::Float3, width, height, normalOffset, sizeof(DenoisableRenderTargetPixel));
	filter.setImage("output", denoisedRenderTarget->data.data(), oidn::Format::Float3, width, height, 0, sizeof(Vector3f));
	filter.set("hdr", true);
	filter.commit();
	filter.execute();

	denoisedRenderTarget ->map(frameData.surface.get(), [](const Vector3f& pixel)
	{
		auto color = pixel;
#else

	renderTarget->map(frameData.surface.get(), [](const DenoisableRenderTargetPixel& pixel)
		{
			auto color = pixel.color;
#endif
		ci::ColorA8u colA8u;

		auto f = [](float v)
		{
			v = powf(v * 0.3f, 1.0f / 2.2f);
			return xitils::clamp((int)(v * 255), 0, 255);
		};

		colA8u.r = f(color.x);
		colA8u.g = f(color.y);
		colA8u.b = f(color.z);
		colA8u.a = 255;
		return colA8u;
	});

	auto time_end = std::chrono::system_clock::now();
	frameData.frameElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start).count();

#if ENABLE_SAVE_IMAGE
	imageSaveThread.push(frameData.surface);
#endif

	++frameData.frameCount;

	printf("frame %d : %f\n", frameData.frameCount, frameData.frameElapsed);

	if(frameData.frameCount == TOTAL_FRAME_COUNT)
	{
		auto time_app_end = std::chrono::system_clock::now();
		float elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(time_app_end - frameData.timeAppStart).count();
		printf("totalTime : %f\n", elapsed);
			
	}

	if(frameData.frameCount >= TOTAL_FRAME_COUNT)
	{
		close();
	}

}

#if SHOW_WINDOW
void MyApp::onDraw(const MyFrameData& frameData, MyUIFrameData& uiFrameData) {
	if (!frameData.surface) { return; }

	texture = gl::Texture::create(*frameData.surface);

	gl::clear(Color::gray(0.5f));


	auto windowSize = getWindowSize();

	gl::draw(texture, (windowSize - ImageSize) / 2);
}
#endif

#if SHOW_WINDOW

XITILS_APP(MyApp)

#else

// TODO : きれいに
//int __stdcall WinMain(HINSTANCE /*hInstance*/, HINSTANCE /*hPrevInstance*/, LPSTR /*lpCmdLine*/, int /*nCmdShow*/)
int main()
{
	MyApp app;
	app.run();
	return 0;
}

#endif