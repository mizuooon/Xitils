
#define EMBREE_API_NAMESPACE
#include <embree3/rtcore.h>
#include <embree3/rtcore_ray.h>

#include "Xitils/include/App.h"
#include "Xitils/include/Geometry.h"
#include "Xitils/include/ObjectStructure.h"
#include "CinderImGui.h"

#include "cinder/gl/Batch.h"

#pragma comment(lib, "cinder")
#pragma comment(lib, "cinder_imgui")

#pragma comment(lib, "embree3")
#pragma comment(lib, "tbb")
#pragma comment(lib, "tbbmalloc")


using namespace Xitils;
using namespace Xitils::Geometry;
using namespace ci;
using namespace ci::app;
using namespace ci::geom;


struct XVertex { float x, y, z, r; }; // FIXME: rename to Vertex4f
struct XTriangle { int v0, v1, v2; };

struct MyFrameData {
	float elapsed;
	Surface surface;
	int triNum;
};

class MyApp : public Xitils::App::XApp<MyFrameData> {
public:
	void onSetup(MyFrameData* frameData) override;
	void onCleanup(MyFrameData* frameData) override;
	void onUpdate(MyFrameData* frameData) override;
	void onDraw(const MyFrameData& frameData) override;

	int frameCount = 0;

	gl::TextureRef texture;

	std::shared_ptr<TriMesh> mesh;
	inline static const glm::ivec2 ImageSize = glm::ivec2(800, 600);

	RTCDevice rtcDevice;
	RTCScene rtcScene;
	int rtcGeometryID;
};

void MyApp::onSetup(MyFrameData* frameData) {
	frameData->surface = Surface(ImageSize.x, ImageSize.y, false);
	frameData->elapsed = 0.0f;
	frameData->triNum = 0;

	getWindow()->setTitle("Xitils");
	setWindowSize(ImageSize.x, ImageSize.y);
	setFrameRate(60);

	const int subdivision = 200;
	auto teapot = std::make_shared<Teapot>();
	teapot->subdivisions(subdivision);
	mesh = std::make_shared<TriMesh>(*teapot);

	ui::initialize();


	rtcDevice = rtcNewDevice(nullptr);
	rtcScene = rtcNewScene(rtcDevice);

	RTCGeometry rtcGeometry = rtcNewGeometry(rtcDevice, RTC_GEOMETRY_TYPE_TRIANGLE);
	
	rtcSetSharedGeometryBuffer(rtcGeometry, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, mesh->getPositions<3>(), 0, sizeof(vec3), mesh->getNumVertices());
	rtcSetSharedGeometryBuffer(rtcGeometry, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, mesh->getIndices().data(), 0, sizeof(ivec3), mesh->getNumTriangles());

	rtcCommitGeometry(rtcGeometry);
	rtcGeometryID = rtcAttachGeometry(rtcScene, rtcGeometry);
	rtcReleaseGeometry(rtcGeometry);

	rtcCommitScene(rtcScene);

}

void MyApp::onCleanup(MyFrameData* frameData) {
	rtcReleaseScene(rtcScene);
	rtcReleaseDevice(rtcDevice);
}

void MyApp::onUpdate(MyFrameData* frameData) {
	auto time_start = std::chrono::system_clock::now();

#pragma omp parallel for schedule(dynamic, 1)
	for (int y = 0; y < frameData->surface.getHeight(); ++y) {
#pragma omp parallel for schedule(dynamic, 1)
		for (int x = 0; x < frameData->surface.getWidth(); ++x) {

			Vector3 color(0, 0, 0);

			Vector2 cameraRange;
			cameraRange.x = 4.0f;
			cameraRange.y = cameraRange.x * 3.0f / 4.0f;

			Vector2 cameraOffset(0, 0.5f);

			float nx = (float)x / frameData->surface.getWidth();
			float ny = (float)y / frameData->surface.getHeight();

			Xitils::Geometry::Ray ray;
			ray.o = Vector3((nx - 0.5f) * cameraRange.x + cameraOffset.x, -(ny - 0.5f) * cameraRange.y + cameraOffset.y, -100);
			ray.d = normalize(Vector3(0, 0, 1));


			RTCIntersectContext context;
			rtcInitIntersectContext(&context);
			
			RTCRayHit rtcRayHit;
			RTCRay& rtcRay = rtcRayHit.ray;
			rtcRay.org_x = ray.o.x;
			rtcRay.org_y = ray.o.y;
			rtcRay.org_z = ray.o.z;
			rtcRay.dir_x = ray.d.x;
			rtcRay.dir_y = ray.d.y;
			rtcRay.dir_z = ray.d.z;
			rtcRay.tnear = 0.0f;
			rtcRay.tfar = Infinity;
			rtcRay.time = 0.0f;
			rtcRayHit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
			rtcRayHit.hit.primID = RTC_INVALID_GEOMETRY_ID;
			
			rtcIntersect1(rtcScene, &context, &rtcRayHit);

			if (rtcRayHit.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
				Vector3 dLight = normalize(Vector3(1, 1, -1));
				//color = Vector3(1.0f, 1.0f, 1.0f) * clamp01(dot(rtcRayHit.hit, dLight));
				color = Vector3(1.0f, 1.0f, 1.0f);
				
				Vector3 n = normalize(Vector3(rtcRayHit.hit.Ng_x, rtcRayHit.hit.Ng_y, rtcRayHit.hit.Ng_z));
				color = Vector3(1.0f, 1.0f, 1.0f) * clamp01(dot(n, dLight));
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

	gl::draw(texture, (windowSize - ImageSize)/2);

	ImGui::Begin("ImGui Window");
	ImGui::Text(("Image Resolution: " + std::to_string(ImageSize.x) + " x " + std::to_string(ImageSize.y)).c_str());
	ImGui::Text(("Elapsed: " + std::_Floating_to_string("%.1f", frameData.elapsed) + " ms / frame").c_str());
	ImGui::Text(("Triangles: " + std::to_string(frameData.triNum)).c_str());
	ImGui::End();

}


XITILS_APP(MyApp)
