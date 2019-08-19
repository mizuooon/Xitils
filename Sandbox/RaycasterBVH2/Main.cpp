
#include <Xitils/App.h>
#include <Xitils/Geometry.h>
#include <Xitils/AccelerationStructure.h>
#include <CinderImGui.h>

#include <cinder/gl/Batch.h>


using namespace Xitils;
using namespace Xitils::Geometry;
using namespace ci;
using namespace ci::app;
using namespace ci::geom;

struct MyFrameData {
	float elapsed;
	Surface surface;
	int triNum;
};

struct MyUIFrameData {
	Vector3f rot;
};

class MyApp : public Xitils::App::XApp<MyFrameData, MyUIFrameData> {
public:
	void onSetup(MyFrameData* frameData, MyUIFrameData* uiFrameData) override;
	void onUpdate(MyFrameData& frameData, const MyUIFrameData& uiFrameData) override;
	void onDraw(const MyFrameData& frameData, MyUIFrameData& uiFrameData) override;

private:
	int frameCount = 0;

	gl::TextureRef texture;

	std::shared_ptr<TriMesh> mesh;
	std::shared_ptr<BVH> bvh;
	inline static const glm::ivec2 ImageSize = glm::ivec2(600, 600);
};

void MyApp::onSetup(MyFrameData* frameData) {
	frameData->surface = Surface(ImageSize.x, ImageSize.y, false);
	frameData->elapsed = 0.0f;
	frameData->triNum = 0;

	getWindow()->setTitle("Xitils");
	setWindowSize(ImageSize.x*2, ImageSize.y);
	setFrameRate(60);

	const int subdivision = 10;
	auto teapot = std::make_shared<Teapot>();
	teapot->subdivisions(subdivision);
	mesh = std::make_shared<TriMesh>(*teapot);

	std::vector<Triangle> tris(mesh->getNumTriangles());
	for (int i = 0; i < mesh->getNumTriangles(); ++i) {
		auto& tri = tris[i];
		mesh->getTriangleVertices(i, (glm::vec3*)&tri.p[0], (glm::vec3*) &tri.p[1], (glm::vec3*) &tri.p[2]);
	}
	
	bvh = std::make_shared<BVH<Triangle>>(tris, [](const Triangle& tri) { return getBoudingBox(tri); });

	ui::initialize();
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
			cameraRange.y = cameraRange.x * 1.0f;

			Vector2f cameraOffset(0,0.5f);

			float nx = (float) x / frameData->surface.getWidth();
			float ny = (float) y / frameData->surface.getHeight();

			Xitils::Geometry::Ray ray;
			ray.o = Vector3f( (nx-0.5f)*cameraRange.x + cameraOffset.x, -(ny-0.5f)*cameraRange.y + cameraOffset.y, -100 );
			ray.d =  normalize(Vector3f(0,0,1));
			
			std::optional<Intersection::RayTriangle::Intersection> intersection;
			
			for (auto it = bvh->traverse(ray); !it->end(); it->next()) {
				auto tmpIntsct = Intersection::RayTriangle::getIntersection(ray, **it);
				if (tmpIntsct && (!intersection || tmpIntsct->t < intersection->t)) {
					intersection = tmpIntsct;
				}
			}

			if (intersection) {
				Vector3f dLight = Vector3f(1, 1, -1).normalize();
				color = Vector3f(1.0f, 1.0f, 1.0f) * clamp01(dot(intersection->n, dLight));
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
	frameData->elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start).count();
}

void MyApp::onDraw(const MyFrameData& frameData) {
	texture = gl::Texture::create(frameData.surface);

	gl::clear(Color::gray(0.5f));
	
	auto windowSize = getWindowSize();

	auto lambert = gl::ShaderDef().lambert().color();
	gl::GlslProgRef shader = gl::getStockShader(lambert);
	auto batch = gl::Batch::create(*mesh, shader);
	auto camera = CameraPersp();
	camera.setAspectRatio(1.0f);
	camera.lookAt(vec3(3, 2, 4.5), vec3(0,0.5f,0), vec3(0, 1, 0));

	gl::enableDepthRead();
	gl::enableDepthWrite();
	gl::pushViewport();
	gl::pushMatrices();
	gl::pushModelMatrix();
	gl::viewport(vec2(windowSize.x / 2, 0), vec2(windowSize.x / 2, windowSize.y));
	gl::color(Color(1.0f, 1.0f, 1.0f));
	gl::setMatrices(camera);
	gl::setModelMatrix(rotate(frameCount * ToRad, vec3(0,1,0)));
	batch->draw();
	for (auto it = bvh->getNodeIterator(); !it->end(); it->next()) {
		auto node = **it;
		float t = clamp01(node->depth / 12.0f);
		gl::color(Color(1.0f, 1.0f-t,1.0f-t));
		gl::drawStrokedCube( ((node->aabb.min+node->aabb.max)/2.0f).glm(), (node->aabb.max-node->aabb.min).glm() );
	}
	gl::popModelMatrix();
	gl::popMatrices();
	gl::popViewport();


	gl::disableDepthRead();
	gl::disableDepthWrite();
	gl::pushViewport();
	gl::viewport(vec2(0, 0), vec2(windowSize.x / 2, windowSize.y));
	gl::setMatricesWindow(windowSize);
	gl::color(Color(1.0f, 1.0f, 1.0f));
	gl::draw(texture, Rectf(vec2(0,0), vec2(windowSize.x, windowSize.y)));
	gl::popViewport();


	ImGui::Begin("ImGui Window");
	ImGui::Text(("Image Resolution: " + std::to_string(ImageSize.x) + " x " + std::to_string(ImageSize.y)).c_str());
	ImGui::Text(("Elapsed: " + std::_Floating_to_string("%.1f", frameData.elapsed) + " ms / frame").c_str());
	ImGui::Text(("Triangles: " + std::to_string(frameData.triNum)).c_str());
	ImGui::End();

}


XITILS_APP(MyApp)
