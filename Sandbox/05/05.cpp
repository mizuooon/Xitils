
#include <cuda_runtime.h>

#include <optixu/optixu_math_namespace.h>
#include <optixu/optixpp_namespace.h>


#include <Xitils/App.h>
#include <Xitils/Geometry.h>
#include <Xitils/AccelerationStructure.h>
#include <CinderImGui.h>

#include <cinder/gl/Batch.h>

#include "Common.h"
#include "optixRaycastingKernels.h"

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

class MyApp : public Xitils::App::XApp<MyFrameData> {
public:
	void onSetup(MyFrameData* frameData) override;
	void onCleanup(MyFrameData* frameData) override;
	void onUpdate(MyFrameData* frameData) override;
	void onDraw(const MyFrameData& frameData) override;

private:
	bool readSourceFile(std::string& str, const std::string& filename);
	void createContext();
	void setMesh();
	void execute();

	int frameCount = 0;
	optix::Context context;
	int cuda_device_ordinal;
	int optix_device_ordinal;
	optix::Buffer positionBuffer;
	optix::Buffer indexBuffer;
	optix::Buffer texcoordBuffer;
	optix::Geometry geometry;
	optix::Material material;
	optix::Buffer rays;
	optix::Buffer hits;
	::Ray* rays_d = nullptr;
	Hit* hits_d = nullptr;
	optix::float3* image_d = nullptr;
	std::vector<optix::float3> image_h;

	gl::TextureRef texture;

	std::shared_ptr<TriMesh> mesh;
	inline static const glm::ivec2 ImageSize = glm::ivec2(800, 600);

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

	createContext();

	setMesh();

	Vector2 cameraRange;
	cameraRange.x = 4.0f;
	cameraRange.y = cameraRange.x * 3.0f / 4.0f;

	optix::float3 bbox_min, bbox_max;
	bbox_min.x = -cameraRange.x / 2.0f;
	bbox_min.y = -cameraRange.y / 2.0f;
	bbox_min.z = -100;
	bbox_max.x = cameraRange.x / 2.0f;
	bbox_max.y = cameraRange.y / 2.0f;
	bbox_max.z = 100;

	bbox_min.y += 0.5f;
	bbox_max.y += 0.5f;

	const int n = ImageSize.x* ImageSize.y;

	cudaMalloc(&rays_d, sizeof(::Ray) * n);
	createRaysOrthoOnDevice( rays_d, ImageSize.x, ImageSize.y, bbox_min, bbox_max, 0.0f);
	rays = context->createBuffer(RT_BUFFER_INPUT_OUTPUT | RT_BUFFER_GPU_LOCAL, RT_FORMAT_USER, n);
	rays->setElementSize(sizeof(::Ray));
	rays->setDevicePointer(optix_device_ordinal, rays_d);
	context["rays"]->set(rays);

	cudaMalloc(&hits_d, sizeof(Hit) * n);
	hits = context->createBuffer(RT_BUFFER_INPUT_OUTPUT | RT_BUFFER_GPU_LOCAL, RT_FORMAT_USER, n);
	hits->setElementSize(sizeof(Hit));
	hits->setDevicePointer(optix_device_ordinal, hits_d);
	context["hits"]->set(hits);

	execute();

	cudaMalloc(&image_d, n * sizeof(optix::float3));
	image_h.resize(n);
	shadeHitsOnDevice(image_d, n, hits_d);
	cudaMemcpy(&image_h[0], image_d, (size_t)n * sizeof(optix::float3), cudaMemcpyDeviceToHost);
}

void MyApp::execute() {
	RTsize n;
	rays->getSize(n);
	context->launch( /*entry point*/ 0, n);
}

void MyApp::onCleanup(MyFrameData* frameData) {
	cudaFree(rays_d);
	cudaFree(hits_d);
	cudaFree(image_d);

	//indexBuffer->destroy();
	//positionBuffer->destroy();
	context->destroy();
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

			color.x = image_h[(ImageSize.y-1 - y) * ImageSize.x + x].x;
			color.y = image_h[(ImageSize.y-1 - y) * ImageSize.x + x].y;
			color.z = image_h[(ImageSize.y-1 - y) * ImageSize.x + x].z;


			//RTCIntersectContext context;
			//rtcInitIntersectContext(&context);
			//
			//RTCRayHit rtcRayHit;
			//RTCRay& rtcRay = rtcRayHit.ray;
			//rtcRay.org_x = ray.o.x;
			//rtcRay.org_y = ray.o.y;
			//rtcRay.org_z = ray.o.z;
			//rtcRay.dir_x = ray.d.x;
			//rtcRay.dir_y = ray.d.y;
			//rtcRay.dir_z = ray.d.z;
			//rtcRay.tnear = 0.0f;
			//rtcRay.tfar = Infinity;
			//rtcRay.time = 0.0f;
			//rtcRayHit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
			//rtcRayHit.hit.primID = RTC_INVALID_GEOMETRY_ID;
			//
			//rtcIntersect1(rtcScene, &context, &rtcRayHit);

			//if (rtcRayHit.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
			//	Vector3 dLight = normalize(Vector3(1, 1, -1));
			//	//color = Vector3(1.0f, 1.0f, 1.0f) * clamp01(dot(rtcRayHit.hit, dLight));
			//	color = Vector3(1.0f, 1.0f, 1.0f);
			//	
			//	Vector3 n = normalize(Vector3(rtcRayHit.hit.Ng_x, rtcRayHit.hit.Ng_y, rtcRayHit.hit.Ng_z));
			//	color = Vector3(1.0f, 1.0f, 1.0f) * clamp01(dot(n, dLight));
			//}

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

	//ImGui::Begin("ImGui Window");
	//ImGui::Text(("Image Resolution: " + std::to_string(ImageSize.x) + " x " + std::to_string(ImageSize.y)).c_str());
	//ImGui::Text(("Elapsed: " + std::_Floating_to_string("%.1f", frameData.elapsed) + " ms / frame").c_str());
	//ImGui::Text(("Triangles: " + std::to_string(frameData.triNum)).c_str());
	//ImGui::End();

}

bool MyApp::readSourceFile(std::string& str, const std::string& filename){
	// Try to open file
	std::ifstream file(filename.c_str());
	if (file.good())
	{
		// Found usable source file
		std::stringstream source_buffer;
		source_buffer << file.rdbuf();
		str = source_buffer.str();
		return true;
	}
	return false;
}


void MyApp::createContext() {
	context = optix::Context::create();
	context->setRayTypeCount(1);
	context->setEntryPointCount(1);

	context->setStackSize(200);

	const std::vector<int> enabled_devices = context->getEnabledDevices();
	context->setDevices(enabled_devices.begin(), enabled_devices.begin() + 1);
	optix_device_ordinal = enabled_devices[0];
	{
		cuda_device_ordinal = -1;
		context->getDeviceAttribute(optix_device_ordinal, RT_DEVICE_ATTRIBUTE_CUDA_DEVICE_ORDINAL, sizeof(int), &cuda_device_ordinal);
	}
	
	cudaSetDevice(cuda_device_ordinal);

	std::string ptx;
	readSourceFile(ptx, "05_generated_05.cu.ptx");

	geometry = context->createGeometry();

	geometry->setIntersectionProgram(context->createProgramFromPTXString(ptx, "intersect"));
	geometry->setBoundingBoxProgram(context->createProgramFromPTXString(ptx, "bounds"));

	material = context->createMaterial();
	optix::GeometryInstance geometry_instance = context->createGeometryInstance(geometry, &material, &material + 1);
	optix::GeometryGroup geometry_group = context->createGeometryGroup(&geometry_instance, &geometry_instance + 1);
	geometry_group->setAcceleration(context->createAcceleration("Trbvh"));
	context["top_object"]->set(geometry_group);

	// Closest hit program for returning geometry attributes.  No shading.
	optix::Program closest_hit = context->createProgramFromPTXString(ptx, "closest_hit");
	material->setClosestHitProgram( /*ray type*/ 0, closest_hit);

	// Raygen program that reads rays directly from an input buffer.
	optix::Program ray_gen = context->createProgramFromPTXString(ptx, "ray_gen");
	context->setRayGenerationProgram( /*entry point*/ 0, ray_gen);
}

void MyApp::setMesh() {
	int vertexNum = mesh->getNumVertices();
	positionBuffer = context->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT3, vertexNum);
	memcpy(positionBuffer->map(), mesh->getPositions<3>(), vertexNum * 3 * sizeof(float));
	positionBuffer->unmap();

	int trianglesNum = mesh->getNumTriangles();
	indexBuffer = context->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_INT3, trianglesNum);
	memcpy(indexBuffer->map(), mesh->getIndices().data(), trianglesNum * 3 * sizeof(int32_t));
	indexBuffer->unmap();

	geometry->setPrimitiveCount(trianglesNum);
	geometry["vertex_buffer"]->set(positionBuffer);
	geometry["index_buffer"]->set(indexBuffer);

	// Connect texcoord buffer, used for masking
	const int num_texcoords = 0;
	texcoordBuffer = context->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT2, num_texcoords);
	geometry["texcoord_buffer"]->set(texcoordBuffer);
}

XITILS_APP(MyApp)
