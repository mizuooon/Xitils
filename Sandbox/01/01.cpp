
#include "Xitils/include/App.h"
#include "CinderImGui.h"

#pragma comment(lib, "cinder")
#pragma comment(lib, "cinder_imgui")


using namespace ci;
using namespace ci::app;

// OnUpdate と OnDraw 両方が使用する変数は
// スレッドセーフに処理する必要があるので
// FrameParams に含める
struct MyFrameParams {
	float elapsed;
	Surface surface;
};


class MyApp : public Xitils::App::XApp<MyFrameParams> {
public:

	// OnDraw と OnUpdate は異なるスレッドから呼ばれる

	void OnSetup(MyFrameParams* frameParams) override;
	void OnUpdate(MyFrameParams* frameParams) override;
	void OnDraw(const MyFrameParams& frameParams) override;

	static const glm::ivec2 ImageSize;

private:
	// スレッドセーフに処理するため、
	// FrameParmas に含まれない情報は
	// OnUpdate または OnDraw のどちらか一方からのみアクセスされるようにする。

	//---- OnUpdate のみが使用する変数
	int frameCount;

	//---- OnDraw のみが使用する変数
	gl::TextureRef texture;
};


const glm::ivec2 MyApp::ImageSize = glm::ivec2(800, 600);

void MyApp::OnSetup(MyFrameParams* frameParams) {
	// OnSetup ではスレッド関係なくどの変数にもアクセスしてよい
	frameParams->surface = Surface(ImageSize.x, ImageSize.y, false);
	frameParams->elapsed = 0.0f;
	texture = nullptr;
	frameCount = 0;

	setWindowSize(ImageSize);
	setFrameRate(60);

	ui::initialize();
}

void MyApp::OnUpdate(MyFrameParams* frameParams) {
	auto time_start = std::chrono::system_clock::now();

#pragma omp parallel for schedule(dynamic, 1)
	for (int y = 0; y < frameParams->surface.getHeight(); ++y) {
#pragma omp parallel for schedule(dynamic, 1)
		for (int x = 0; x < frameParams->surface.getWidth(); ++x) {

			ColorA8u color(0, 0, 0, 255);

			vec2 p0(x, y);

			float theta1 = frameCount / M_PI * 0.05f;
			float theta2 = theta1 + M_PI * 2.0f / 3.0f;
			float theta3 = theta2 + M_PI * 2.0f / 3.0f;
			const float r = 30.0f;
			const float L = 100.0f;

			vec2 p1 = vec2(cos(theta1), sin(theta1)) * L + vec2(MyApp::ImageSize / 2);
			vec2 p2 = vec2(cos(theta2), sin(theta2)) * L + vec2(MyApp::ImageSize / 2);
			vec2 p3 = vec2(cos(theta3), sin(theta3)) * L + vec2(MyApp::ImageSize / 2);


			if (length(p1 - p0) <= r) { color.r = 255; }
			if (length(p2 - p0) <= r) { color.g = 255; }
			if (length(p3 - p0) <= r) { color.b = 255; }

			frameParams->surface.setPixel(ivec2(x, y), color);
		}
	}

	++frameCount;

	auto time_end = std::chrono::system_clock::now();
	frameParams->elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start).count();
}

void MyApp::OnDraw(const MyFrameParams& frameParams) {
	texture = gl::Texture::create(frameParams.surface);

	gl::clear(Color::gray(0.5f));

	auto windowSize = getWindowSize();
	gl::draw(texture, (windowSize - ImageSize) / 2);


	ImGui::Begin("ImGui Window");
	ImGui::Text("Hello World!");
	ImGui::Text(("Image Resolution: " + std::to_string(ImageSize.x) + " x " + std::to_string(ImageSize.y)).c_str());
	ImGui::Text(("Elapsed: " + std::_Floating_to_string("%.1f", frameParams.elapsed) + " ms / frame").c_str());
	ImGui::End();
}


XITILS_APP(MyApp)
