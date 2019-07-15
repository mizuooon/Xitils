
#include <Xitils/App.h>
#include <CinderImGui.h>

using namespace ci;
using namespace ci::app;


struct MyFrameData {
	float elapsed;
	Surface surface;
};

class MyApp : public Xitils::App::XApp<MyFrameData> {
public:
	void onSetup(MyFrameData* frameData) override;
	void onUpdate(MyFrameData* frameData) override;
	void onDraw(const MyFrameData& frameData) override;

	int frameCount = 0;
	gl::TextureRef texture;

	inline static const glm::ivec2 ImageSize = glm::ivec2(800, 600);
};

void MyApp::onSetup(MyFrameData* frameData) {
	frameData->surface = Surface(ImageSize.x, ImageSize.y, false);
	frameData->elapsed = 0.0f;

	getWindow()->setTitle("Xitils");
	setWindowSize(ImageSize);
	setFrameRate(60);

	ui::initialize();
}


void MyApp::onUpdate(MyFrameData* frameData) {
	auto time_start = std::chrono::system_clock::now();

#pragma omp parallel for schedule(dynamic, 1)
	for (int y = 0; y < frameData->surface.getHeight(); ++y) {
#pragma omp parallel for schedule(dynamic, 1)
		for (int x = 0; x < frameData->surface.getWidth(); ++x) {

			ColorA8u color(0, 0, 0, 255);

			vec2 p0(x, y);

			float theta1 = frameCount / M_PI * 0.05f;
			float theta2 = theta1 + M_PI * 2.0f / 3.0f;
			float theta3 = theta2 + M_PI * 2.0f / 3.0f;
			const float r = 30.0f;
			const float L = 100.0f;

			vec2 p1 = vec2(cos(theta1), sin(theta1)) * L + vec2(ImageSize / 2);
			vec2 p2 = vec2(cos(theta2), sin(theta2)) * L + vec2(ImageSize / 2);
			vec2 p3 = vec2(cos(theta3), sin(theta3)) * L + vec2(ImageSize / 2);


			if (length(p1 - p0) <= r) { color.r = 255; }
			if (length(p2 - p0) <= r) { color.g = 255; }
			if (length(p3 - p0) <= r) { color.b = 255; }

			frameData->surface.setPixel(ivec2(x, y), color);
		}
	}

	++frameCount;

	auto time_end = std::chrono::system_clock::now();
	frameData->elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start).count();
}

void MyApp::onDraw(const MyFrameData& frameData) {
	texture = gl::Texture::create(frameData.surface);

	gl::clear(Color::gray(0.5f));
	
	auto windowSize = ci::app::getWindowSize();
	gl::draw(texture, (windowSize - ImageSize) / 2);

	ImGui::Begin("ImGui Window");
	ImGui::Text("Hello World!");
	ImGui::Text(("Image Resolution: " + std::to_string(ImageSize.x) + " x " + std::to_string(ImageSize.y)).c_str());
	ImGui::Text(("Elapsed: " + std::_Floating_to_string("%.1f", frameData.elapsed) + " ms / frame").c_str());
	ImGui::End();
}


XITILS_APP(MyApp)
