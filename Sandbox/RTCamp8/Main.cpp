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

#include <OpenImageDenoise/oidn.hpp>

#include <queue>


using namespace xitils;
using namespace ci;
using namespace ci::app;
using namespace ci::geom;

#define SCENE_DURATION 5.0f
#define FRAME_PER_SECOND 10
#define TOTAL_FRAME_COUNT (SCENE_DURATION * FRAME_PER_SECOND)

#define ENABLE_DENOISE true
#define SHOW_WINDOW true
#define ENABLE_SAVE_IMAGE true

#define IMAGE_WIDTH 1920/2
#define IMAGE_HEIGHT 1920/2


class MetalWithMultipleScattering : public Material
{
public:
	float alpha;

	Vector3f f0;

	MetalWithMultipleScattering(float roughness, const Vector3f& f0) :
		alpha(roughness* roughness),
		f0(f0)
	{
	}

	Vector3f F_Reflection(const Vector3f& wo, const Vector3f& h) const
	{
		return f0 + (Vector3f(1.0f) - f0) * pow(1 - dot(wo, h), 5.0f);
	}

	float D_GGX(const Vector3f& n, const Vector3f& h) const
	{
		float alpha2 = alpha * alpha;
		float cosThetaH2 = pow(dot(n, h), 2);
		float cosThetaH4 = cosThetaH2 * cosThetaH2;
		float tanThetaH2 = 1 / cosThetaH2 - 1;

		return alpha2 * heavisideStep(dot(n, h)) / (M_PI * cosThetaH4 * pow(alpha2 + tanThetaH2, 2));
	}

	float G1_FullSphere(const Vector3f& v, const Vector3f& n, const Vector3f& h) const
	{
		float alpha2 = alpha * alpha;
		auto lambda = [&](const Vector3f& v)
		{
			float c2 = pow(dot(v, n), 2);
			float t2 = 1/c2 - 1;
			float a2 = 1/(t2 * alpha2);
			return (-1 + safeSqrt(1 + 1 / a2)) / 2;
		};

		float G1_local = heavisideStep(dot(v, h));
		float G1_dist = abs(1 / (1 + lambda(v)));
		return G1_local * G1_dist;
	}

	float G1_Smith(const Vector3f& v, const Vector3f& n, const Vector3f& h) const
	{
		float alpha2 = alpha * alpha;
		float cosThetaV2 = pow(dot(n, v), 2);
		float tanThetaV2 = 1 / cosThetaV2 - 1;
		return heavisideStep(dot(v, h) / dot(v, n)) * 2 / (1 + safeSqrt(1 + alpha2 * tanThetaV2));
	}

	float G_Smith(const Vector3f& wi, const Vector3f& wo, const Vector3f& n, const Vector3f& h) const
	{
		return G1_Smith(wi, n, h) * G1_Smith(wo, n, h);
	}

	Vector3f bsdfCos(const SurfaceIntersection& isect, Sampler& sampler, const Vector3f& wi) const override {
		const auto& wo = isect.wo;
		const auto& n = isect.shading.n;

		// 透過は端折ってる
		// BDPT も実装してない
		// heig-uncorrelated のみ

		const int sampleNum = 16;
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
				const float CONTINUE_PROBABILITY = 0.8f;
				if(i > 0)
				{
					if (sampler.randf() > CONTINUE_PROBABILITY)
					{
						break;
					}
					weight /= CONTINUE_PROBABILITY;
				}
				//if(i >= 3)
				//{
				//	break;
				//}

				{
					// NEE
					Vector3f d_iplus1 = wi;
					Vector3f h_i = (-d_i + d_iplus1).normalize();
					float s_i = s(i, d_i, h_iminus1, h_i, false);
					float s_iplus1 = s(i + 1, d_iplus1, h_i, Vector3f(0), true);

					Vector3f Fr = F_Reflection(-d_i, h_i);
					float D = D_GGX(n, h_i);
					if (abs(dot(-d_i, n)) < 0.00001f) { continue; }
					Vector3f v_i = Fr * D / abs(4 * dot(-d_i, n));

					bsdfCos += weight * (s_i * v_i * s_iplus1) / pdf;
					//printf("%f %f %f %f %f\n", weight.x, s_i, v_i.x, s_iplus1, pdf);
				}

				// GGX からマイクロファセットをサンプル
				float r0 = sampler.randf();
				float r1 = sampler.randf();
				float theta = atanf(alpha * safeSqrt(r0) / safeSqrt(1 - r0));
				float phi = 2 * M_PI * r1;
				const auto h_i = BasisVectors(n).toGlobal(anglesToVector(Vector2f(theta, phi)));

				Vector3f d_iplus1 = 2 * dot(-d_i, h_i) * h_i - (-d_i);

				float D = D_GGX(n, h_i);
				float G = G_Smith(wi, wo, n, h_i);
				pdf *= fabs(dot(wo, h_i) * G  / (dot(wo, n) * dot(h_i, n)));
				if (pdf == 0) { break; }

				float s_i = s(i, d_i, h_iminus1, h_i, false);

				if (abs(dot(-d_i, n)) < 0.00001f) { break; }
				Vector3f Fr = F_Reflection(-d_i, h_i);
				Vector3f v_i = Fr * D / abs(4 * dot(-d_i, n));

				weight *= s_i * v_i;
				if (weight == Vector3f(0)) { break; }

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

		// GGX からマイクロファセットをサンプル
		float r0 = sampler.randf();
		float r1 = sampler.randf();
		float theta = atanf(alpha * safeSqrt(r0) / safeSqrt(1 - r0));
		float phi = 2 * M_PI * r1;
		const auto h = BasisVectors(n).toGlobal(anglesToVector(Vector2f(theta, phi)));

		*wi = 2 * dot(wo, h) * h - wo;

		float G = G_Smith(*wi, wo, n, h);
		*pdf = dot(wo, h) * G / (dot(wo, n) * dot(h, n));

		return bsdfCos(isect, sampler, *wi);
	}

	float getPDF(const SurfaceIntersection& isect, const Vector3f& wi) const override {
		const auto& wo = isect.wo;
		const auto& n = isect.shading.n;
		const auto h = (wi + wo).normalize();

		float G = G_Smith(wi, wo, n, h);
		return fabs(dot(wo, h) * G / (dot(wo, n) * dot(h, n)));
	}

	Vector3f getAlbedo() const override {
		return f0;
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
				std::string seq = std::to_string(sequentialNum);
				while (seq.length() < 3) { seq = "0" + seq; }
				writeImage(seq + ".png", *image);
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
	float frameElapsed;
	int triNum;
	int frameCount = 0;
	std::shared_ptr<Surface> surface;
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
	
	frameData->frameElapsed = 0.0f;
	frameData->triNum = 0;

	getWindow()->setTitle("Xitils");
	setWindowSize(ImageSize);
	setFrameRate(60);

	imageSaveThread.start();

	ui::initialize();

	scene = std::make_shared<Scene>();

	auto camera = std::make_shared<PinholeCamera>(
		translate(0, 2.0f, -5), 60 * ToRad, (float)ImageSize.y / ImageSize.x
		);
	scene->camera = camera;
	camera->addKeyFrame(
		5.0f,
		translate(0, 2.0f, -10),
		30 * ToRad
		);


	// cornell box
	auto diffuse_white = std::make_shared<Diffuse>(Vector3f(0.8f));
	auto diffuse_red = std::make_shared<Diffuse>(Vector3f(0.8f, 0.1f, 0.1f));
	auto diffuse_green = std::make_shared<Diffuse>(Vector3f(0.1f, 0.8f, 0.1f));
	auto emission = std::make_shared<Emission>(Vector3f(1.0f, 1.0f, 0.95f) * 4);
	auto cube = std::make_shared<xitils::Cube>();
	auto plane = std::make_shared<xitils::Plane>();
	scene->addObject(
		std::make_shared<Object>( cube, diffuse_white, transformTRS(Vector3f(0,0,0), Vector3f(), Vector3f(4, 0.01f, 4)))
	);
	//scene->addObject(
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
	scene->skySphere = std::make_shared<SkySphereFromImage>("rnl_probe.hdr");

	// teapot 作成
	const int subdivision = 10;
	auto teapot = std::make_shared<Teapot>();
	teapot->subdivisions(subdivision);
	auto teapotMeshData = std::make_shared<TriMesh>(*teapot);
	auto teapotMesh = std::make_shared<TriangleMesh>();
	teapotMesh->setGeometry(*teapotMeshData);
	auto teapotMaterial = std::make_shared<Metal>(1.0f, Vector3f(0.955f, 0.638f, 0.652f));
	scene->addObject(std::make_shared<Object>(teapotMesh, teapotMaterial,
		transformTRS(Vector3f(0.0f, 0, 0.0f), Vector3f(0, 0, 0), Vector3f(1.5f))
		));

	scene->buildAccelerationStructure();

	pathTracer = std::make_shared<StandardPathTracer>();
	//pathTracer = std::make_shared<NaivePathTracer>();

	auto time_end = std::chrono::system_clock::now();

	frameData->initElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - init_time_start).count();
	frameData->triNum = teapotMeshData->getNumTriangles();


	device = oidn::newDevice();
	device.commit();

#if !SHOW_WINDOW
	getWindow()->hide();
#endif

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
	auto time_start = std::chrono::system_clock::now();

	// update 1 フレームごとに計算するサンプル数
	int sample = 1;

	auto renderTarget = std::make_shared<DenoisableRenderTarget>(ImageSize.x, ImageSize.y);
	scene->camera->setCurrentTime((float)frameData.frameCount / FRAME_PER_SECOND);
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
			v = powf(v, 1.0f / 2.2f);
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

	if(frameData.frameCount >= TOTAL_FRAME_COUNT)
	{
		close();
	}

}

void MyApp::onDraw(const MyFrameData& frameData, MyUIFrameData& uiFrameData) {
#if SHOW_WINDOW
	if (!frameData.surface) { return; }

	texture = gl::Texture::create(*frameData.surface);

	gl::clear(Color::gray(0.5f));


	auto windowSize = getWindowSize();

	gl::draw(texture, (windowSize - ImageSize) / 2);
#endif
}

XITILS_APP(MyApp)
