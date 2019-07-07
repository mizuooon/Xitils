
#include "Xitils/include/App.h"
#include "Xitils/include/Geometry.h"
#include "CinderImGui.h"


#pragma comment(lib, "cinder")
#pragma comment(lib, "cinder_imgui")


using namespace Xitils;
using namespace ci;
using namespace ci::app;
using namespace ci::geom;


struct MyFrameData {
	float elapsed;
	Surface surface;
	int triNum;
};

class MyApp : public Xitils::App::XApp<MyFrameData> {
public:
	void onSetup() override;

	class UpdateThread : public Xitils::App::UpdateThread<MyFrameData> {
	public:
		void onSetup() override;
		void onUpdate(MyFrameData* frameData) override;
	private:
		int frameCount = 0;
		std::shared_ptr<TriMesh> mesh;
	};

	class DrawThread : public Xitils::App::DrawThread<MyFrameData> {
	public:
		void onDraw(const MyFrameData& frameData) override;
	private:
		gl::TextureRef texture;
	};

	inline static const glm::ivec2 ImageSize = glm::ivec2(800, 600);
};

void MyApp::onSetup() {
	frameData.surface = Surface(ImageSize.x, ImageSize.y, false);
	frameData.elapsed = 0.0f;
	frameData.triNum = 0;

	getWindow()->setTitle("Xitils");
	setWindowSize(ImageSize);
	setFrameRate(60);

	ui::initialize();
}

void MyApp::UpdateThread::onSetup() {
	const int subdivision = 10;

	auto teapot = std::make_shared<Teapot>();
	teapot->subdivisions(subdivision);

	mesh = std::make_shared<TriMesh>(*teapot);
}

void MyApp::UpdateThread::onUpdate(MyFrameData* frameData) {
	auto time_start = std::chrono::system_clock::now();

	mat4 rot = rotate(frameCount * 1.0f, Vector3(0,1,0));

#pragma omp parallel for schedule(dynamic, 1)
	for (int y = 0; y < frameData->surface.getHeight(); ++y) {
#pragma omp parallel for schedule(dynamic, 1)
		for (int x = 0; x < frameData->surface.getWidth(); ++x) {

			Vector3 color(0, 0, 0);

			Vector2 cameraRange;
			cameraRange.x = 4.0f;
			cameraRange.y = cameraRange.x * 3.0f / 4.0f;

			Vector2 cameraOffset(0,0.5f);

			float nx = (float) x / frameData->surface.getWidth();
			float ny = (float) y / frameData->surface.getHeight();

			Xitils::Geometry::Ray ray;
			ray.o = Vector3( (nx-0.5f)*cameraRange.x + cameraOffset.x, -(ny-0.5f)*cameraRange.y + cameraOffset.y, -100 );
			ray.d =  normalize(Vector3(0,0,1));
			
			int triNum = mesh->getNumTriangles();
			std::optional< Xitils::Geometry::Intersection::RayTriangle::Intersection> intersection;
			for (int i = 0; i < triNum; ++i) {
				Xitils::Geometry::Triangle tri;

				mesh->getTriangleVertices(i, &tri.p[0],&tri.p[1],&tri.p[2]);

				auto tmpIntsct = Xitils::Geometry::Intersection::RayTriangle::getIntersection(ray,tri);

				if (tmpIntsct && (!intersection || tmpIntsct->t < intersection->t)) {
					intersection = tmpIntsct;
				}
			}

			if (intersection) {
				Vector3 dLight = normalize(Vector3(1, 1, -1));
				color = Vector3(1.0f, 1.0f, 1.0f) * clamp01(dot(intersection->n, dLight));
			}

			ColorA8u colA8u;
			colA8u.r = Xitils::clamp((int)(color.x * 255), 0, 255);
			colA8u.g = Xitils::clamp((int)(color.y * 255), 0, 255);
			colA8u.b = Xitils::clamp((int)(color.z * 255), 0, 255);
			colA8u.a = 255;

			frameData->surface.setPixel(ivec2(x, y), colA8u);
			frameData->triNum = triNum;
		}
	}

	++frameCount;

	auto time_end = std::chrono::system_clock::now();
	frameData->elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start).count();
}

void MyApp::DrawThread::onDraw(const MyFrameData& frameData) {
	texture = gl::Texture::create(frameData.surface);

	gl::clear(Color::gray(0.5f));
	
	auto windowSize = ci::app::getWindowSize();
	gl::draw(texture, (windowSize - ImageSize) / 2);

	ImGui::Begin("ImGui Window");
	ImGui::Text(("Image Resolution: " + std::to_string(ImageSize.x) + " x " + std::to_string(ImageSize.y)).c_str());
	ImGui::Text(("Elapsed: " + std::_Floating_to_string("%.1f", frameData.elapsed) + " ms / frame").c_str());
	ImGui::Text(("Triangles: " + std::to_string(frameData.triNum)).c_str());
	ImGui::End();
}


XITILS_APP(MyApp)
