

#include <Xitils/AccelerationStructure.h>
#include <Xitils/App.h>
#include <Xitils/Camera.h>
#include <Xitils/Geometry.h>
#include <Xitils/Scene.h>
#include <Xitils/TriangleMesh.h>
#include <Xitils/RenderTarget.h>
#include <CinderImGui.h>


using namespace Xitils;
using namespace ci;
using namespace ci::app;
using namespace ci::geom;

struct MyFrameData {
	float initElapsed;
	float frameElapsed;
	Surface surface;
	int triNum;
};

struct MyUIFrameData {
	Vector3f rot;
};

class MyApp : public Xitils::App::XApp<MyFrameData, MyUIFrameData> {
public:
	void onSetup(MyFrameData* frameData, MyUIFrameData* uiFrameData) override;
	void onCleanup(MyFrameData* frameData, MyUIFrameData* uiFrameData) override;
	void onUpdate(MyFrameData& frameData, const MyUIFrameData& uiFrameData) override;
	void onDraw(const MyFrameData& frameData, MyUIFrameData& uiFrameData) override;

private:
	
	gl::TextureRef texture;

	std::shared_ptr<Scene> scene;
	inline static const glm::ivec2 ImageSize = glm::ivec2(800, 600);

	std::shared_ptr<RenderTarget> renderTarget;

};

void MyApp::onSetup(MyFrameData* frameData, MyUIFrameData* uiFrameData) {
	auto time_start = std::chrono::system_clock::now();
	
	frameData->surface = Surface(ImageSize.x, ImageSize.y, false);
	frameData->frameElapsed = 0.0f;
	frameData->triNum = 0;

	getWindow()->setTitle("Xitils");
	setWindowSize(ImageSize);
	setFrameRate(60);

	ui::initialize();

	scene = std::make_shared<Scene>();

	scene->camera = std::make_shared<PinholeCamera>(
		translate(0,0.5f,-3), 60 * ToRad, (float)ImageSize.y / ImageSize.x
		);
	//scene->camera = std::make_shared<OrthographicCamera>(
	//	translate(0, 0.5f, -3), 4, 3);

	const int subdivision = 10;
	auto teapot = std::make_shared<Teapot>();
	teapot->subdivisions(subdivision);
	auto teapotMeshData = std::make_shared<TriMesh>(*teapot);

	std::vector<Vector3f> positions(teapotMeshData->getNumVertices());
	for (int i = 0; i < positions.size(); ++i) {
		positions[i] = Vector3f(teapotMeshData->getPositions<3>()[i]);
	}
	std::vector<Vector3f> normals(teapotMeshData->getNumVertices());
	for (int i = 0; i < normals.size(); ++i) {
		normals[i] = Vector3f(teapotMeshData->getNormals()[i]);
	}
	std::vector<int> indices(teapotMeshData->getNumTriangles() * 3);
	for (int i = 0; i < indices.size(); i += 3) {
		indices[i + 0] = teapotMeshData->getIndices()[i + 0];
		indices[i + 1] = teapotMeshData->getIndices()[i + 1];
		indices[i + 2] = teapotMeshData->getIndices()[i + 2];
	}

	auto teapotMesh = std::make_shared<TriangleMesh>();
	teapotMesh->setGeometry(
		positions.data(), positions.size(),
		normals.data(), normals.size(),
		indices.data(), indices.size()
	);
	auto material = std::make_shared<SpecularFresnel>();
	material->index = 1.02f;
	scene->objects.push_back(std::make_shared<Object>( teapotMesh, material, Matrix4x4()));

	scene->buildAccelerationStructure();

	renderTarget = std::make_shared<RenderTarget>(ImageSize.x, ImageSize.y);

	scene->skySphere = std::make_shared<SkySphereFromImage>("rnl_probe.hdr");

	auto time_end = std::chrono::system_clock::now();
	frameData->initElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start).count();
	frameData->triNum = teapotMeshData->getNumTriangles();
}

void MyApp::onCleanup(MyFrameData* frameData, MyUIFrameData* uiFrameData) {

}

void MyApp::onUpdate(MyFrameData& frameData, const MyUIFrameData& uiFrameData) {
	auto time_start = std::chrono::system_clock::now();

	for (auto& obj : scene->objects) {
		obj->objectToWorld = rotateYXZ(uiFrameData.rot);
	}
	scene->buildAccelerationStructure();

	renderTarget->clear();

	int SampleNum = 4;

	renderTarget->render(scene, SampleNum, [&](const Vector2f& pFilm, const std::shared_ptr<Sampler>& sampler, Vector3f& color) {
		auto ray = scene->camera->GenerateRay(pFilm, sampler);

		SurfaceInteraction isect;
		
		if (scene->intersect(ray, &isect)) {
			//Vector3f dLight = normalize(Vector3f(1, 1, -1));
			//color = Vector3f(1.0f, 1.0f, 1.0f) * clamp01(dot(isect.shading.n, dLight));

			ray.depth += 1;
			while (ray.depth < 10) {
				ray.tMax = Infinity;
				isect.object->material->sampleSpecular(isect, sampler, &ray.d);
				ray.o = isect.p + 0.0001f * ray.d;
				if (!scene->intersect(ray, &isect)) {
					color += scene->skySphere->getRadiance(ray.d);
					break;
				}
				ray.depth += 1;
			}

		} else {
			color += scene->skySphere->getRadiance(ray.d);
		}
	});

	renderTarget->toneMap(&frameData.surface, SampleNum);

	auto time_end = std::chrono::system_clock::now();
	frameData.frameElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start).count();
}

void MyApp::onDraw(const MyFrameData& frameData, MyUIFrameData& uiFrameData) {

	texture = gl::Texture::create(frameData.surface);

	gl::clear(Color::gray(0.5f));
	
	auto windowSize = ci::app::getWindowSize();
	gl::draw(texture, (windowSize - ImageSize) / 2);

	ImGui::Begin("ImGui Window");
	ImGui::Text(("Image Resolution: " + std::to_string(ImageSize.x) + " x " + std::to_string(ImageSize.y)).c_str());
	ImGui::Text(("Elapsed in Initialization: " + std::_Floating_to_string("%.1f", frameData.initElapsed) + " ms").c_str());
	ImGui::Text(("Elapsed per frame: " + std::_Floating_to_string("%.1f", frameData.frameElapsed) + " ms").c_str());
	ImGui::Text(("Triangles: " + std::to_string(frameData.triNum)).c_str());

	float rot[3];
	rot[0] = uiFrameData.rot.x;
	rot[1] = uiFrameData.rot.y;
	rot[2] = uiFrameData.rot.z;
	ImGui::SliderFloat3("Rotation", rot, -180, 180 );
	uiFrameData.rot.x = rot[0];
	uiFrameData.rot.y = rot[1];
	uiFrameData.rot.z = rot[2];

	ImGui::End();
}


XITILS_APP(MyApp)
