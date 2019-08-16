
#include <Xitils/App.h>
#include <Xitils/Geometry.h>
#include <Xitils/AccelerationStructure.h>
#include <Xitils/TriangleMesh.h>
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

	TriangleMesh* mesh;
	std::shared_ptr<AccelerationStructure> bvh;
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
	auto teapotMesh = std::make_shared<TriMesh>(*teapot);

	std::vector<Vector3f> positions(teapotMesh->getNumVertices());
	for (int i = 0; i < positions.size(); ++i) {
		positions[i] = Vector3f(teapotMesh->getPositions<3>()[i]);
	}
	std::vector<int> indices(teapotMesh->getNumTriangles() * 3);
	for (int i = 0; i < indices.size(); i += 3) {
		indices[i + 0] = teapotMesh->getIndices()[i + 0];
		indices[i + 1] = teapotMesh->getIndices()[i + 2]; // ŽžŒv‰ñ‚è‚É’¼‚·
		indices[i + 2] = teapotMesh->getIndices()[i + 1];
	}

	mesh = new TriangleMesh( Matrix4x4() );
	mesh->setGeometry(
		positions.data(), positions.size(),
		indices.data(), indices.size()
	);

	std::vector<Shape*> shapes;
	shapes.push_back(mesh);

	bvh = std::make_shared<BVH>(shapes);

	auto time_end = std::chrono::system_clock::now();
	frameData->initElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start).count();
}

void MyApp::onCleanup(MyFrameData* frameData) {

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
				Vector3f dLight = normalize(Vector3f(1, 1, 1));
				color = Vector3f(1.0f, 1.0f, 1.0f) * clamp01(dot(isect.n, dLight));
			}

			ColorA8u colA8u;
			colA8u.r = Xitils::clamp((int)(color.x * 255), 0, 255);
			colA8u.g = Xitils::clamp((int)(color.y * 255), 0, 255);
			colA8u.b = Xitils::clamp((int)(color.z * 255), 0, 255);
			colA8u.a = 255;

			frameData->surface.setPixel(ivec2(x, y), colA8u);
			frameData->triNum = mesh->triangleNum();
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
