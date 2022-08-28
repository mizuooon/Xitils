

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
#define FRAME_PER_SECOND 4
#define TOTAL_FRAME_COUNT (SCENE_DURATION * FRAME_PER_SECOND)

#define ENABLE_DENOISE true
#define SHOW_WINDOW true
#define ENABLE_SAVE_IMAGE true

#define IMAGE_WIDTH 1920/4
#define IMAGE_HEIGHT 1080/4

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

	// teapot 作成
	const int subdivision = 10;
	auto teapot = std::make_shared<Teapot>();
	teapot->subdivisions(subdivision);
	auto teapotMeshData = std::make_shared<TriMesh>(*teapot);
	auto teapotMesh = std::make_shared<TriangleMesh>();
	teapotMesh->setGeometry(*teapotMeshData);
	auto teapotMaterial = std::make_shared<Diffuse>(Vector3f(0.8f));
	scene->addObject(std::make_shared<Object>(teapotMesh, diffuse_white,
		transformTRS(Vector3f(0.0f, 0, 0.0f), Vector3f(0, 0, 0), Vector3f(1.5f))
		));

	scene->buildAccelerationStructure();

	pathTracer = std::make_shared<StandardPathTracer>();

	auto time_end = std::chrono::system_clock::now();

	frameData->initElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - init_time_start).count();
	frameData->triNum = teapotMeshData->getNumTriangles();


	device = oidn::newDevice();
	device.commit();

#if !SHOW_WINDOW
	getWindow()->hide();
#endif

	// fps の数値を fps.txt に書き込む
	{
		auto file = writeFile("fps.txt");
		writeString(file, std::to_string(FRAME_PER_SECOND));
	}
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
