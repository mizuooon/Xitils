

#include <Xitils/AccelerationStructure.h>
#include <Xitils/App.h>
#include <Xitils/Camera.h>
#include <Xitils/Geometry.h>
#include <Xitils/PathTracer.h>
#include <Xitils/Scene.h>
#include <Xitils/TriangleMesh.h>
#include <Xitils/RenderTarget.h>
#include <Xitils/VonMisesFisherDistribution.h>
#include <CinderImGui.h>


using namespace Xitils;
using namespace ci;
using namespace ci::app;
using namespace ci::geom;

const int DownSamplingRate = 16;
const int VMFLobeNum = 6;
const int ShellMappingLayerNum = 16;

class MultiLobeSVBRDF : public Material {
public:
	std::shared_ptr<Material> baseMaterial;
	std::vector<VonMisesFisherDistribution<VMFLobeNum>> vmfs;
	std::shared_ptr<Texture> dispTexLow;
	float displacementScale;

	Vector3f bsdfCos(const SurfaceInteraction& isect, Sampler& sampler, const Vector3f& wi) const override {
		return baseMaterial->bsdfCos(perturbInteraction(isect, sampler), sampler, wi);
	}

	Vector3f evalAndSample(const SurfaceInteraction& isect, Sampler& sampler, Vector3f* wi, float* pdf) const override {
		return baseMaterial->evalAndSample(perturbInteraction(isect, sampler), sampler, wi, pdf);
	}

	float pdf(const SurfaceInteraction& isect, const Vector3f& wi) const override {
		
		// TODO
		
		return clampPositive(dot(isect.shading.n, wi)) / M_PI;
	}

private:

	SurfaceInteraction perturbInteraction(const SurfaceInteraction& isect, Sampler& sampler) const {

		SurfaceInteraction isectPerturbed = isect;

		//int x, y;
		//if (dispTexLow->filter) {
		//	int x0 = isect.texCoord.u * dispTexLow->getWidth() + 0.5f;
		//	int y0 = (1 - isect.texCoord.v) * dispTexLow->getHeight() + 0.5f;
		//	int x1 = x0 + 1;
		//	int y1 = y0 + 1;
		//	float wx1 = (isect.texCoord.u * dispTexLow->getWidth() + 0.5f) - x0;
		//	float wy1 = ((1 - isect.texCoord.v) * dispTexLow->getHeight() + 0.5f) - y0;

		//	dispTexLow->warp(x0, y0);
		//	dispTexLow->warp(x1, y1);

		//	x = (sampler.randf() <= wx1) ? x1 : x0;
		//	y = (sampler.randf() <= wy1) ? y1 : y0;
		//} else {
		//	x = isect.texCoord.u * dispTexLow->getWidth();
		//	y = (1 - isect.texCoord.v) * dispTexLow->getHeight();
		//	dispTexLow->warp(x, y);
		//}

		//Vector3f n = vmfs[y * dispTexLow->getWidth() + x].sample(sampler);

		//isectPerturbed.shading.n =
		//	(n.z * isect.n
		//		+ n.x * isect.tangent
		//		- n.y * isect.bitangent
		//		).normalize();

		//isectPerturbed.shading.n = faceForward(isectPerturbed.shading.n, isectPerturbed.wo);

		return isectPerturbed;
	}

};

std::shared_ptr<Texture> downsampleTexture(std::shared_ptr<Texture> texOrig, float displacementScale, std::vector<VonMisesFisherDistribution<VMFLobeNum>>* vmfs) {
	
	Sampler sampler(0);

	auto tex = std::make_shared<Texture>( 
		(texOrig->getWidth()  + DownSamplingRate - 1) / DownSamplingRate, 
		(texOrig->getHeight() + DownSamplingRate - 1) / DownSamplingRate);

	vmfs->clear();
	vmfs->resize(tex->getWidth() * tex->getHeight());

	// tex の初期値は普通に texOrig をダウンサンプリングした値

	std::vector<Vector2f> ave_slope_orig ( tex->getWidth() * tex->getHeight() );
	for (int py = 0; py < tex->getHeight(); ++py) {
		for (int px = 0; px < tex->getWidth(); ++px) {

			int count = 0;
			std::vector<Vector3f> normals;
			normals.reserve(DownSamplingRate * DownSamplingRate);

			for (int ly = 0; ly < DownSamplingRate; ++ly) {
				int y = py * DownSamplingRate + ly;
				if (y >= texOrig->getHeight()) { continue; }
				for (int lx = 0; lx < DownSamplingRate; ++lx) {
					int x = px * DownSamplingRate + lx;
					if (x >= texOrig->getWidth()) { continue; }
					
					Vector2f slope = Vector2f(texOrig->rgbDifferentialU(x, y).x, texOrig->rgbDifferentialV(x, y).x);
					Vector3f n = Vector3f(-slope.u * displacementScale, -slope.v * displacementScale, 1).normalize();

					ave_slope_orig[py * tex->getWidth() + px] += slope;
					tex->r(px, py) += texOrig->r(x, y);

					normals.push_back(n);

					++count;
				}
			}
			ave_slope_orig[py * tex->getWidth() + px] /= count;
			tex->r(px, py) /= count;

			(*vmfs)[py * tex->getWidth() + px] = VonMisesFisherDistribution<VMFLobeNum>::approximateBySEM(normals, sampler);
		}
	}

	auto L = [&](int px, int py, float dh) {
		const float w = 0.01f;
		
		int x0 = px * DownSamplingRate;
		int y0 = py * DownSamplingRate;
		int x1 = Xitils::min(px + DownSamplingRate - 1, texOrig->getWidth()  - 1);
		int y1 = Xitils::min(py + DownSamplingRate - 1, texOrig->getHeight() - 1);

		float dhdu2 =  (tex->rgbDifferentialU(px + 1, py).r - tex->rgbDifferentialU(px - 1, py).r) / 2.0f;
		float dhdv2 = -(tex->rgbDifferentialV(px, py + 1).r - tex->rgbDifferentialU(px, py - 1).r) / 2.0f; // y 方向と v 方向は逆なので符号反転

		Vector2f ave_slope(
			(tex->r(x1, y1) + tex->r(x1, y0) - tex->r(x0, y1) - tex->r(x0, y0)) / 2.0f,
			-(tex->r(x1, y1) + tex->r(x0, y1) - tex->r(x1, y0) - tex->r(x0, y0)) / 2.0f // y 方向と v 方向は逆なので符号反転
		);

		return (ave_slope - ave_slope_orig[py * tex->getWidth() + px]).lengthSq() - w * powf(dhdu2 + dhdv2, 2.0f);
	};

	// TODO: ダウンサンプリングの正規化項入れる

	return tex;
}

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
	std::shared_ptr<PathTracer> pathTracer;

	decltype(std::chrono::system_clock::now()) time_start;
};

void MyApp::onSetup(MyFrameData* frameData, MyUIFrameData* uiFrameData) {
	time_start = std::chrono::system_clock::now();
	auto init_time_start = std::chrono::system_clock::now();
	
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
		transformTRS(Vector3f(0,1.5f,-4), Vector3f(10, 0, 0), Vector3f(1)), 
		40 * ToRad, 
		(float)ImageSize.y / ImageSize.x
		);
	//scene->camera = std::make_shared<OrthographicCamera>(
	//	translate(0, 0.5f, -3), 4, 3);

	const int subdivision = 10;
	auto teapot = std::make_shared<Teapot>();
	teapot->subdivisions(subdivision);
	auto teapotMeshData = std::make_shared<TriMesh>(*teapot);


	auto teapot_material = std::make_shared<MultiLobeSVBRDF>();
	teapot_material->baseMaterial = std::make_shared<Diffuse>(Vector3f(0.8f));
	teapot_material->displacementScale = 0.01f;
	teapot_material->dispTexLow = std::make_shared<Texture>("dispcloth.jpg");
	teapot_material->dispTexLow = downsampleTexture(teapot_material->dispTexLow, teapot_material->displacementScale, &teapot_material->vmfs);
	auto& dispmap = teapot_material->dispTexLow;

	//auto teapot_material = std::make_shared<Diffuse>(Vector3f(0.8f));
	//auto dispmap = std::make_shared<Texture>("dispcloth.jpg");

	auto teapotMesh = std::make_shared<TriangleMesh>();
	//teapotMesh->setGeometry(teapotMeshData);
	teapotMesh->setGeometryWithShellMapping(teapotMeshData, dispmap, teapot_material->displacementScale, ShellMappingLayerNum);
	auto material = std::make_shared<SpecularFresnel>();
	material->index = 1.2f;

	auto diffuse_white = std::make_shared<Diffuse>(Vector3f(0.8f));
	
	auto diffuse_red = std::make_shared<Diffuse>(Vector3f(0.8f, 0.1f, 0.1f));
	auto diffuse_green = std::make_shared<Diffuse>(Vector3f(0.1f, 0.8f, 0.1f));
	auto emission = std::make_shared<Emission>(Vector3f(1.0f, 1.0f, 0.95f) * 8);
	auto cube = std::make_shared<Xitils::Cube>();
	auto plane = std::make_shared<Xitils::Plane>();
	scene->addObject(
		std::make_shared<Object>( cube, diffuse_white, transformTRS(Vector3f(0,0,0), Vector3f(), Vector3f(4, 0.01f, 4)))
	);
	scene->addObject(
		std::make_shared<Object>(cube, diffuse_green, transformTRS(Vector3f(2, 2, 0), Vector3f(), Vector3f(0.01f, 4, 4)))
	);
	scene->addObject(
		std::make_shared<Object>(cube, diffuse_red, transformTRS(Vector3f(-2, 2, 0), Vector3f(), Vector3f(0.01f, 4, 4)))
	);
	scene->addObject(
		std::make_shared<Object>(cube, diffuse_white, transformTRS(Vector3f(0, 2, 2), Vector3f(), Vector3f(4, 4, 0.01f)))
	);
	scene->addObject(
		std::make_shared<Object>(cube, diffuse_white, transformTRS(Vector3f(0, 4, 0), Vector3f(), Vector3f(4, 0.01f, 4)))
	);
	scene->addObject(
		std::make_shared<Object>(plane, emission, transformTRS(Vector3f(0, 4.0f -0.01f, 0), Vector3f(-90,0,0), Vector3f(2.0f)))
	);

	//scene->addObject(std::make_shared<Object>(teapotMesh, diffuse_white,
	//	transformTRS(Vector3f(0.8f, 0, 0.0f), Vector3f(0, 0, 0), Vector3f(1, 1, 1)
	//	)));
	//scene->addObject(
	//	std::make_shared<Object>(cube, diffuse_white, transformTRS(Vector3f(-0.8f, 0.5f, 0.5f), Vector3f(0,30,0), Vector3f(1,1,1)))
	//);

	scene->addObject(std::make_shared<Object>(teapotMesh, teapot_material,
		transformTRS(Vector3f(0.0f, 0, 0.0f), Vector3f(0, 0, 0), Vector3f(1.5f))
		));

	scene->buildAccelerationStructure();

	//scene->skySphere = std::make_shared<SkySphereFromImage>("rnl_probe.hdr");

	renderTarget = std::make_shared<RenderTarget>(ImageSize.x, ImageSize.y);

	pathTracer = std::make_shared<StandardPathTracer>();
	//pathTracer = std::make_shared<DebugRayCaster>([](const SurfaceInteraction& isect) { 
	//	return (isect.shading.n) *0.5f + Vector3f(0.5f);
	//	} );

	auto time_end = std::chrono::system_clock::now();

	frameData->initElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - init_time_start).count();
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

	renderTarget->render(scene, sample, [&](const Vector2f& pFilm, Sampler& sampler, Vector3f& color) {
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
	ImGui::Text(("Elapsed : " + std::to_string(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - time_start).count()) + " s").c_str());
	//ImGui::Text(("Triangles: " + std::to_string(frameData.triNum)).c_str());
	ImGui::Text(("Samples: " + std::to_string(frameData.sampleNum)).c_str());

	//float rot[3];
	//rot[0] = uiFrameData.rot.x;
	//rot[1] = uiFrameData.rot.y;
	//rot[2] = uiFrameData.rot.z;
	//ImGui::SliderFloat3("Rotation", rot, -180, 180 );
	//uiFrameData.rot.x = rot[0];
	//uiFrameData.rot.y = rot[1];
	//uiFrameData.rot.z = rot[2];

	ImGui::End();
}


XITILS_APP(MyApp)
