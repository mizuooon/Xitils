
#include <Xitils/App.h>
#include <Xitils/Geometry.h>
#include <Xitils/AccelerationStructure.h>
#include <CinderImGui.h>

using namespace Xitils;
using namespace Xitils::Geometry;
using namespace ci;
using namespace ci::app;
using namespace ci::geom;


struct MyFrameData {
	float initElapsed;
	float frameElapsed;
	Surface surface;
	int triNum;
};

class MyApp : public Xitils::App::XApp<MyFrameData> {
public:
	void onSetup(MyFrameData* frameData) override;
	void onUpdate(MyFrameData* frameData) override;
	void onCleanup(MyFrameData* frameData) override;
	void onDraw(const MyFrameData& frameData) override;

private:
	int frameCount = 0;

	gl::TextureRef texture;

	std::shared_ptr<TriMesh> mesh;
	std::vector<Shape*> tris;
	std::shared_ptr<BVH> bvh;
	inline static const glm::ivec2 ImageSize = glm::ivec2(800, 600);
};

void MyApp::onSetup(MyFrameData* frameData) {
	auto time_start = std::chrono::system_clock::now();

	frameData->surface = Surface(ImageSize.x, ImageSize.y, false);
	frameData->frameElapsed = 0.0f;
	frameData->triNum = 0;

	getWindow()->setTitle("Xitils");
	setWindowSize(ImageSize);
	setFrameRate(60);

	ui::initialize();

	const int subdivision = 100;
	auto teapot = std::make_shared<Teapot>();
	teapot->subdivisions(subdivision);
	mesh = std::make_shared<TriMesh>(*teapot);

	tris.reserve(mesh->getNumTriangles());
	for (int i = 0; i < mesh->getNumTriangles(); ++i) {
		Triangle* tri = new Triangle(Matrix4x4());
		glm::vec3 p0, p1, p2;
		mesh->getTriangleVertices(i, &p0, &p2, &p1); // ŽžŒv‰ñ‚è‚É’¼‚·
		tri->p[0] = Vector3f(p0);
		tri->p[1] = Vector3f(p1);
		tri->p[2] = Vector3f(p2);

		tris.push_back(tri);
	}

	bvh = std::make_shared<BVH>(tris);

	auto time_end = std::chrono::system_clock::now();
	frameData->initElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start).count();
}

void MyApp::onCleanup(MyFrameData* frameData) {
	for(auto tri : tris) {
		delete tri;
	}
}

void MyApp::onUpdate(MyFrameData* frameData) {
	auto time_start = std::chrono::system_clock::now();

#pragma omp parallel for schedule(dynamic, 1)
	for (int y = 0; y < frameData->surface.getHeight(); ++y) {
#pragma omp parallel for schedule(dynamic, 1)
		for (int x = 0; x < frameData->surface.getWidth(); ++x) {

			Vector3f color(0, 0, 0);

			Vector2f cameraRange;
			cameraRange.x = 4.0f;
			cameraRange.y = cameraRange.x * 3.0f / 4.0f;

			Vector2f cameraOffset(0,0.5f);

			float nx = (float) x / frameData->surface.getWidth();
			float ny = (float) y / frameData->surface.getHeight();

			Xitils::Ray ray;
			ray.o = Vector3f( (nx-0.5f)*cameraRange.x + cameraOffset.x, -(ny-0.5f)*cameraRange.y + cameraOffset.y, 100 );
			ray.d = normalize(Vector3f(0,0,-1));
			
			SurfaceInteraction isect;

			if (bvh->intersect(ray, &isect)) {
				Vector3f dLight = normalize(Vector3f(1, 1, -1));
				color = Vector3f(1.0f, 1.0f, 1.0f) * clamp01(dot(isect.n, dLight));
				color = clamp01(isect.n * 0.5f + Vector3f(0.5f));
				color = clamp01(isect.n);
			}

			ColorA8u colA8u;
			colA8u.r = Xitils::clamp((int)(color.x * 255), 0, 255);
			colA8u.g = Xitils::clamp((int)(color.y * 255), 0, 255);
			colA8u.b = Xitils::clamp((int)(color.z * 255), 0, 255);
			colA8u.a = 255;

			frameData->surface.setPixel(ivec2(x, y), colA8u);
			frameData->triNum = mesh->getNumTriangles();
		}
	}

	++frameCount;

	auto time_end = std::chrono::system_clock::now();
	frameData->frameElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start).count();
}

void MyApp::onDraw(const MyFrameData& frameData) {
	texture = gl::Texture::create(frameData.surface);

	gl::clear(Color::gray(0.5f));
	
	auto windowSize = ci::app::getWindowSize();
	gl::draw(texture, (windowSize - ImageSize) / 2);

	ImGui::Begin("ImGui Window");
	ImGui::Text(("Image Resolution: " + std::to_string(ImageSize.x) + " x " + std::to_string(ImageSize.y)).c_str());
	ImGui::Text(("Elapsed in Initialization: " + std::_Floating_to_string("%.1f", frameData.initElapsed) + " ms").c_str());
	ImGui::Text(("Elapsed per frame: " + std::_Floating_to_string("%.1f", frameData.frameElapsed) + " ms").c_str());
	ImGui::Text(("Triangles: " + std::to_string(frameData.triNum)).c_str());
	ImGui::End();
}


XITILS_APP(MyApp)
