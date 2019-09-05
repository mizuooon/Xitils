

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

struct MyFrameData {
	float initElapsed;
	float frameElapsed;
	Surface surface;
};

struct MyUIFrameData {
};

class MyApp : public Xitils::App::XApp<MyFrameData, MyUIFrameData> {
public:
	void onSetup(MyFrameData* frameData, MyUIFrameData* uiFrameData) override;
	void onCleanup(MyFrameData* frameData, MyUIFrameData* uiFrameData) override;
	void onUpdate(MyFrameData& frameData, const MyUIFrameData& uiFrameData) override;
	void onDraw(const MyFrameData& frameData, MyUIFrameData& uiFrameData) override;

private:
	
	gl::TextureRef texture;

	VonMisesFisherDistribution<15> vmf;

	std::shared_ptr<Scene> scene;
	inline static const glm::ivec2 ImageSize = glm::ivec2(800, 400);

	std::shared_ptr<Sampler> sampler;
	std::vector<Vector3f> samples;

	std::shared_ptr<cinder::Font> font;

	decltype(std::chrono::system_clock::now()) time_start;
};

void MyApp::onSetup(MyFrameData* frameData, MyUIFrameData* uiFrameData) {
	time_start = std::chrono::system_clock::now();
	auto init_time_start = std::chrono::system_clock::now();
	
	frameData->surface = Surface(ImageSize.x, ImageSize.y, false);
	frameData->frameElapsed = 0.0f;

	getWindow()->setTitle("Xitils");
	setWindowSize(ImageSize);
	setFrameRate(60);

	ui::initialize();

	sampler = std::make_shared<Sampler>(0);

	font = std::make_shared<cinder::Font>("arial", 20);

	while (samples.size() < 10000) {
		auto v = sampleVectorFromHemiSphere(Vector3f(0,0,1), *sampler);

		float theta = asinf(v.z);
		float phi = atan2(v.y, v.x) + M_PI;

		float p = ((int)(theta / M_PI * 4) + (int)(phi / (2 * M_PI) * 8)) % 2 == 0 ? 1.0f : 0.0f;

		if (sampler->randf() < p ) {
			samples.push_back( v );
		}
	}
	vmf = VonMisesFisherDistribution<decltype(vmf)::LobeNum>::approximateBySEM(samples, *sampler);

	auto time_end = std::chrono::system_clock::now();

	frameData->initElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - init_time_start).count();
}

void MyApp::onCleanup(MyFrameData* frameData, MyUIFrameData* uiFrameData) {
}

void MyApp::onUpdate(MyFrameData& frameData, const MyUIFrameData& uiFrameData) {
	auto time_start = std::chrono::system_clock::now();

	auto time_end = std::chrono::system_clock::now();
	frameData.frameElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start).count();
}

void MyApp::onDraw(const MyFrameData& frameData, MyUIFrameData& uiFrameData) {

	texture = gl::Texture::create(frameData.surface);

	gl::clear(Color::gray(0.0f));
	
	auto windowSize = ci::app::getWindowSize();
	//gl::draw(texture, (windowSize - ImageSize) / 2);

	glm::vec2 windowCenter;
	windowCenter.x = ImageSize.x / 2;
	windowCenter.y = ImageSize.y / 2;

	gl::color(Color::gray(1.0f));
	for(auto& s : samples){
		glm::vec2 p = glm::vec2(s.x, s.y) * 100.0f + windowCenter - glm::vec2(200, 0);
		float r = 0.5f;
		gl::drawSolidRect(cinder::Rectf(p - glm::vec2(r,r), p + glm::vec2(r,r)));
	}
	gl::drawString("Original Spherical Distribution", windowCenter - glm::vec2(200, 0) + glm::vec2(0, 120), cinder::ColorA(1, 1, 1, 1), *font);

	for (int i = 0; i < samples.size(); ++i) {
		auto s = vmf.sample(*sampler);
		glm::vec2 p = glm::vec2(s.x, s.y) * 100.0f + windowCenter + glm::vec2(200, 0);
		float r = 0.5f;
		gl::drawSolidRect(cinder::Rectf(p - glm::vec2(r, r), p + glm::vec2(r, r)));
	}
	gl::drawString("Approximated by vMF", windowCenter + glm::vec2(200, 0) + glm::vec2(0, 120), cinder::ColorA(1,1,1,1), *font);

}


XITILS_APP(MyApp)
