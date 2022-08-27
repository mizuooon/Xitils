

#include <Xitils/App.h>
#include <Xitils/Texture.h>
#include <CinderImGui.h>

#pragma warning(push)
#pragma warning(disable:4067)
#pragma warning(disable:4715)
#include <tinycolormap.hpp>
#pragma warning(pop)

using namespace xitils;
using namespace ci;
using namespace ci::app;
using namespace ci::geom;

struct MyFrameData {
	Surface surface;
};

struct MyUIFrameData {
	float maxValue = 0.05f;
};

class MyApp : public xitils::app::XApp<MyFrameData, MyUIFrameData> {
public:
	void onSetup(MyFrameData* frameData, MyUIFrameData* uiFrameData) override;
	void onCleanup(MyFrameData* frameData, MyUIFrameData* uiFrameData) override;
	void onUpdate(MyFrameData& frameData, const MyUIFrameData& uiFrameData) override;
	void onDraw(const MyFrameData& frameData, MyUIFrameData& uiFrameData) override;

private:

	std::shared_ptr<xitils::Texture> image1;
	std::shared_ptr<xitils::Texture> image2;

	gl::TextureRef texture;

};

void MyApp::onSetup(MyFrameData* frameData, MyUIFrameData* uiFrameData) {
	auto image1Path = this->getOpenFilePath().string();
	auto image2Path = this->getOpenFilePath(image1Path).string();

	printf("%s\n", image1Path.c_str());
	printf("%s\n", image2Path.c_str());

	image1 = std::make_shared<Texture>(image1Path);
	image2 = std::make_shared<Texture>(image2Path);

	frameData->surface = Surface(image1->getWidth(), image2->getHeight(), false);

	setWindowSize(image1->getWidth(), image1->getHeight());

	ui::initialize();
}

void MyApp::onCleanup(MyFrameData* frameData, MyUIFrameData* uiFrameData) {
}

void MyApp::onUpdate(MyFrameData& frameData, const MyUIFrameData& uiFrameData) {

#pragma omp parallel for schedule(dynamic, 1)
	for (int y = 0; y < image1->getHeight(); ++y) {
#pragma omp parallel for schedule(dynamic, 1)
		for (int x = 0; x < image1->getWidth(); ++x) {
			float val1 = rgbToLuminance(image1->rgb(x, y));
			float val2 = rgbToLuminance(image2->rgb(x, y));
			float d = fabsf(val1 - val2);
			float d2 = powf(d, 2.0f);

			float t = d2 / uiFrameData.maxValue;
			tinycolormap::Color color = tinycolormap::GetColor(t, tinycolormap::ColormapType::Jet);
			
			Color8u col8u;
			col8u.r = xitils::clamp<int>(color.r() * 255, 0, 255);
			col8u.g = xitils::clamp<int>(color.g() * 255, 0, 255);
			col8u.b = xitils::clamp<int>(color.b() * 255, 0, 255);
			frameData.surface.setPixel(ivec2(x, y), col8u);
		}
	}
	
}

void MyApp::onDraw(const MyFrameData& frameData, MyUIFrameData& uiFrameData) {
	gl::clear(Color::gray(0.5f));

	texture = gl::Texture::create(frameData.surface);
	auto windowSize = ci::app::getWindowSize();
	gl::draw(texture, (windowSize - frameData.surface.getSize()) / 2);

	ImGui::Begin("ImGui Window");
	ImGui::SliderFloat("ErrorMax", &uiFrameData.maxValue, 0.01f, 10.0f );
	ImGui::End();
}


XITILS_APP(MyApp)
