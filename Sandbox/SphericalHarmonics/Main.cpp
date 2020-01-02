

#include <Xitils/AccelerationStructure.h>
#include <Xitils/App.h>
#include <Xitils/Camera.h>
#include <Xitils/Geometry.h>
#include <Xitils/PathTracer.h>
#include <Xitils/Scene.h>
#include <Xitils/TriangleMesh.h>
#include <Xitils/RenderTarget.h>
#include <Xitils/SphericalHarmonics.h>
#include <CinderImGui.h>


using namespace xitils;
using namespace ci;
using namespace ci::app;
using namespace ci::geom;


const int L = 10;
const int Samples = 10000;

struct MyFrameData {
	float initElapsed;
	Surface surface;
};

struct MyUIFrameData {
};

class MyApp : public xitils::app::XApp<MyFrameData, MyUIFrameData> {
public:
	void onSetup(MyFrameData* frameData, MyUIFrameData* uiFrameData) override;
	void onCleanup(MyFrameData* frameData, MyUIFrameData* uiFrameData) override;
	void onUpdate(MyFrameData& frameData, const MyUIFrameData& uiFrameData) override;
	void onDraw(const MyFrameData& frameData, MyUIFrameData& uiFrameData) override;

private:
	
	gl::TextureRef texture;

	std::shared_ptr<SphericalHarmonics<L>> sh;

	std::shared_ptr<Scene> scene;
	inline static const glm::ivec2 ImageSize = glm::ivec2(800, 400);

	std::shared_ptr<Sampler> sampler;

	std::shared_ptr<cinder::Font> font;

	std::function<float(Vector3f)> func;

	decltype(std::chrono::system_clock::now()) time_start;
};

void MyApp::onSetup(MyFrameData* frameData, MyUIFrameData* uiFrameData) {
	time_start = std::chrono::system_clock::now();
	auto init_time_start = std::chrono::system_clock::now();
	
	frameData->surface = Surface(ImageSize.x, ImageSize.y, false);
#pragma omp parallel for schedule(dynamic, 1)
	for (int y = 0; y < frameData->surface.getHeight(); ++y) {
#pragma omp parallel for schedule(dynamic, 1)
		for (int x = 0; x < frameData->surface.getWidth(); ++x) {
			ColorA8u color(0, 0, 0, 255);
			frameData->surface.setPixel(ivec2(x, y), color);
		}
	}

	getWindow()->setTitle("Xitils");
	setWindowSize(ImageSize);
	setFrameRate(60);

	ui::initialize();

	sampler = std::make_shared<Sampler>(0);

	font = std::make_shared<cinder::Font>("arial", 20.0f);

	auto sky = std::make_shared<SkySphereFromImage>("rnl_probe.hdr");

	func = [sky](const Vector3f& v) {
		Vector3f a = v;
		a.y *= -1;

		//return v.y > 0.0 ? 1.0f : 0.0f;

		return clamp01(rgbToLuminance(sky->getRadiance(a)));
	};

	sh = std::make_shared<SphericalHarmonics<L>>(Samples, func, *sampler);

	auto time_end = std::chrono::system_clock::now();

	frameData->initElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - init_time_start).count();


	auto windowSize = ci::app::getWindowSize();
	glm::vec2 windowCenter;
	windowCenter.x = ImageSize.x / 2;
	windowCenter.y = ImageSize.y / 2;

	int r = 100;
#pragma omp parallel for schedule(dynamic, 1)
	for (int y = -r; y <= r; ++y) {
#pragma omp parallel for schedule(dynamic, 1)
		for (int x = -r; x <= r; ++x) {
			if (x * x + y * y > r * r) { continue; }

			Vector3f v;
			v.x = (float)x / r;
			v.y = (float)y / r;
			v.z = safeSqrt(1.0f - powf(v.x, 2.0f) - powf(v.y, 2.0f));

			ColorA8u color(0, 0, 0, 255);
			float value = func(v);
			int ivalue = xitils::clamp((int)(value * 255), 0, 255);
			color.r = ivalue;
			color.g = ivalue;
			color.b = ivalue;
			frameData->surface.setPixel(windowCenter + vec2(-200, 0) + vec2(x, y), color);

			value = sh->eval(v);
			ivalue = xitils::clamp((int)(value * 255), 0, 255);
			color.r = ivalue;
			color.g = ivalue;
			color.b = ivalue;
			frameData->surface.setPixel(windowCenter + vec2(200, 0) + vec2(x, y), color);
		}
	}
}

void MyApp::onCleanup(MyFrameData* frameData, MyUIFrameData* uiFrameData) {
}

void MyApp::onUpdate(MyFrameData& frameData, const MyUIFrameData& uiFrameData) {
}

void MyApp::onDraw(const MyFrameData& frameData, MyUIFrameData& uiFrameData) {
	texture = gl::Texture::create(frameData.surface);

	gl::clear(Color::gray(0.5f));
	
	auto windowSize = ci::app::getWindowSize();
	gl::draw(texture, (windowSize - ImageSize) / 2);

	glm::vec2 windowCenter;
	windowCenter.x = ImageSize.x / 2;
	windowCenter.y = ImageSize.y / 2;

	gl::drawString("Original Spherical Distribution", windowCenter - glm::vec2(200, 0) + glm::vec2(0, 120), cinder::ColorA(1, 1, 1, 1), *font);
	gl::drawString("Approximated by SH", windowCenter + glm::vec2(200, 0) + glm::vec2(0, 120), cinder::ColorA(1,1,1,1), *font);
}


XITILS_APP(MyApp)
