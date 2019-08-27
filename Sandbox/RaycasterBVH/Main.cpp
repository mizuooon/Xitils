

#include <Xitils/AccelerationStructure.h>
#include <Xitils/App.h>
#include <Xitils/Camera.h>
#include <Xitils/Geometry.h>
#include <Xitils/PathTracer.h>
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
	int sampleNum = 0;
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
	inline static const glm::ivec2 ImageSize = glm::ivec2(800, 800);

	std::shared_ptr<RenderTarget> renderTarget;
	std::shared_ptr<NaivePathTracer> pathTracer;

};

void MyApp::onSetup(MyFrameData* frameData, MyUIFrameData* uiFrameData) {
	auto time_start = std::chrono::system_clock::now();
	
	frameData->surface = Surface(ImageSize.x, ImageSize.y, false);
	frameData->frameElapsed = 0.0f;
	frameData->sampleNum = 0;
	frameData->triNum = 0;

	getWindow()->setTitle("Xitils");
	setWindowSize(ImageSize);
	setFrameRate(60);

	ui::initialize();

	scene = std::make_shared<Scene>();

	scene->camera = std::make_shared<PinholeCamera>(
		translate(0,2.0f,-5), 60 * ToRad, (float)ImageSize.y / ImageSize.x
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
	material->index = 1.2f;

	auto diffuse_white = std::make_shared<Diffuse>(Vector3f(0.95f));
	

	auto diffuse_red = std::make_shared<Diffuse>(Vector3f(0.9f, 0.1f, 0.1f));
	auto diffuse_green = std::make_shared<Diffuse>(Vector3f(0.1f, 0.9f, 0.1f));
	auto emission = std::make_shared<Emission>(Vector3f(1.0f, 1.0f, 0.95f) * 40);
	auto cube = std::make_shared<Xitils::Cube>();
	auto plane = std::make_shared<Xitils::Plane>();
	scene->objects.push_back(
		std::make_shared<Object>( cube, diffuse_white, transformTRS(Vector3f(0,0,0), Vector3f(), Vector3f(4, 0.01f, 4)))
	);
	scene->objects.push_back(
		std::make_shared<Object>(cube, diffuse_green, transformTRS(Vector3f(2, 2, 0), Vector3f(), Vector3f(0.01f, 4, 4)))
	);
	scene->objects.push_back(
		std::make_shared<Object>(cube, diffuse_red, transformTRS(Vector3f(-2, 2, 0), Vector3f(), Vector3f(0.01f, 4, 4)))
	);
	scene->objects.push_back(
		std::make_shared<Object>(cube, diffuse_white, transformTRS(Vector3f(0, 2, 2), Vector3f(), Vector3f(4, 4, 0.01f)))
	);
	scene->objects.push_back(
		std::make_shared<Object>(cube, diffuse_white, transformTRS(Vector3f(0, 4, 0), Vector3f(), Vector3f(4, 0.01f, 4)))
	);
	scene->objects.push_back(
		std::make_shared<Object>(plane, emission, transformTRS(Vector3f(0, 4-0.01f, 0), Vector3f(-90,0,0), Vector3f(1, 1, 1)))
	);

	scene->objects.push_back(std::make_shared<Object>(teapotMesh, material, 
		transformTRS(Vector3f(0.8f, 0, 0.0f), Vector3f(0, 0, 0), Vector3f(1, 1, 1)
		)));
	scene->objects.push_back(
		std::make_shared<Object>(cube, diffuse_white, transformTRS(Vector3f(-0.8f, 0.5f, 0.5f), Vector3f(0,30,0), Vector3f(1,1,1)))
	);

	scene->buildAccelerationStructure();

	renderTarget = std::make_shared<RenderTarget>(ImageSize.x, ImageSize.y);

	pathTracer = std::make_shared<NaivePathTracer>();

	auto time_end = std::chrono::system_clock::now();

	frameData->initElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start).count();
	frameData->triNum = teapotMeshData->getNumTriangles();
}

void MyApp::onCleanup(MyFrameData* frameData, MyUIFrameData* uiFrameData) {

}

void MyApp::onUpdate(MyFrameData& frameData, const MyUIFrameData& uiFrameData) {
	auto time_start = std::chrono::system_clock::now();

	//scene->objects[0]->objectToWorld = rotateYXZ(uiFrameData.rot);
	//scene->buildAccelerationStructure();

	//renderTarget->clear();

	int sample = 1;

	frameData.sampleNum += sample;

	renderTarget->render(scene, sample, [&](const Vector2f& pFilm, const std::shared_ptr<Sampler>& sampler, Vector3f& color) {
		auto ray = scene->camera->GenerateRay(pFilm, sampler);

		color += pathTracer->eval(scene, sampler, ray);

		//SurfaceInteraction isect;
		//
		//if (scene->intersect(ray, &isect)) {
		//	//Vector3f dLight = normalize(Vector3f(1, 1, -1));
		//	//color = Vector3f(1.0f, 1.0f, 1.0f) * clamp01(dot(isect.shading.n, dLight));

		//	Vector3f wi;
		//	Vector3f f;
		//	if (isect.object->material->emissive) {
		//		color += isect.object->material->emission(isect);
		//	}
		//	if (isect.object->material->specular) {
		//		f = isect.object->material->evalAndSampleSpecular(isect, sampler, &wi);
		//	} else {
		//		float pdf;
		//		f = isect.object->material->evalAndSample(isect, sampler, &wi, &pdf);
		//	}
		//	if (!f.isZero()) {

		//		Xitils::Ray ray2;
		//		SurfaceInteraction isect2;
		//		ray2.d = wi;
		//		ray2.o = isect.p + ray2.d * 0.001f;
		//		if (scene->intersect(ray2,&isect2)) {
		//			if (isect2.object->material->emissive) {
		//				color += f * isect2.object->material->emission(isect2) * 10;
		//			}
		//		}

		//	}

		//	//color += isect.shading.n * 0.5f + Vector3f(0.5f);

		//}
	});

	renderTarget->toneMap(&frameData.surface, frameData.sampleNum);

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
	ImGui::Text(("Samples: " + std::to_string(frameData.sampleNum)).c_str());

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
