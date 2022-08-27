
//#include <windows.h>

//#include <cuda_gl_interop.h>
#include <cuda_runtime.h>

#include <optix.h>
#include <optix_function_table_definition.h>
#include <optix_stubs.h>

#include <xitils/App.h>
#include <xitils/Geometry.h>
#include <xitils/AccelerationStructure.h>
#include <CinderImGui.h>

//#include <cinder/gl/Batch.h>

using namespace xitils;
using namespace ci;
using namespace ci::app;
using namespace ci::geom;


#define CUDA_CHECK( call ) cudaCheck( call, #call, __FILE__, __LINE__ )

#define OPTIX_CHECK( call ) optixCheck( call, #call, __FILE__, __LINE__ )

#define OPTIX_CHECK_LOG( call ) optixCheckLog( call, log, sizeof( log ), sizeof_log, #call, __FILE__, __LINE__ )

inline void cudaCheck(cudaError_t error, const char* call, const char* file, unsigned int line)
{
	if (error != cudaSuccess)
	{
		std::stringstream ss;
		ss << "CUDA call (" << call << " ) failed with error: '"
			<< cudaGetErrorString(error) << "' (" << file << ":" << line << ")\n";
		throw Exception(ss.str().c_str());
	}
}

inline void optixCheck(OptixResult res, const char* call, const char* file, unsigned int line)
{
	if (res != OPTIX_SUCCESS)
	{
		std::stringstream ss;
		ss << "Optix call '" << call << "' failed: " << file << ':' << line << " " << res << ")\n";
		throw Exception(ss.str().c_str());
	}
}

inline void optixCheckLog(OptixResult  res,
	const char* log,
	size_t       sizeof_log,
	size_t       sizeof_log_returned,
	const char* call,
	const char* file,
	unsigned int line)
{
	if (res != OPTIX_SUCCESS)
	{
		std::stringstream ss;
		ss << "Optix call '" << call << "' failed: " << file << ':' << line << ")\nLog:\n"
			<< log << (sizeof_log_returned > sizeof_log ? "<TRUNCATED>" : "") << " " << res << '\n';
		throw Exception(ss.str().c_str());
	}
}

static void contextLogCallback(uint32_t level, const char* tag, const char* msg, void* /* callback_data */)
{
	std::cerr << "[" << std::setw(2) << level << "][" << std::setw(12) << tag << "]: " << msg << "\n";
}

static void getInputDataFromFile(std::string& ptx, const char* sample_name, const char* filename)
{
	//const std::string sourceFilePath = sampleInputFilePath(sample_name, filename);

	//// Try to open source PTX file
	//if (!readSourceFile(ptx, sourceFilePath))
	//{
	//	std::string err = "Couldn't open source file " + sourceFilePath;
	//	throw std::runtime_error(err.c_str());
	//}
}

struct PtxSourceCache
{
	std::map<std::string, std::string*> map;
	~PtxSourceCache()
	{
		for (std::map<std::string, std::string*>::const_iterator it = map.begin(); it != map.end(); ++it)
			delete it->second;
	}
};
static PtxSourceCache g_ptxSourceCache;
const char* getInputData(const char* sample,
	const char* sampleDir,
	const char* filename,
	size_t& dataSize,
	const char** log,
	const std::vector<const char*>& compilerOptions)
{
	if (log)
		*log = NULL;

	std::string* ptx, cu;
	std::string                                   key = std::string(filename) + ";" + (sample ? sample : "");
	std::map<std::string, std::string*>::iterator elem = g_ptxSourceCache.map.find(key);

	if (elem == g_ptxSourceCache.map.end())
	{
		ptx = new std::string();
#if CUDA_NVRTC_ENABLED
		SUTIL_ASSERT(fileExtensionForLoading() == ".ptx");
		std::string location;
		getCuStringFromFile(cu, location, sampleDir, filename);
		getPtxFromCuString(*ptx, sampleDir, cu.c_str(), location.c_str(), log, compilerOptions);
#else
		getInputDataFromFile(*ptx, sample, filename);
#endif
		g_ptxSourceCache.map[key] = ptx;
	} else
	{
		ptx = elem->second;
	}
	dataSize = ptx->size();
	return ptx->c_str();
}





struct MyFrameData {
	float elapsed;
	Surface surface;
	int triNum;
};

struct MyUIFrameData {
};

class MyApp : public xitils::app::XApp<MyFrameData, MyUIFrameData> {
public:
	void onSetup(MyFrameData* frameData, MyUIFrameData* uiFrameData) override;
	void onCleanup(MyFrameData* frameData, MyUIFrameData* uiFrameData) override;
	void onUpdate(MyFrameData& frameData, const MyUIFrameData& uiFrameData) override;
	void onDraw(const MyFrameData& frameData, MyUIFrameData& uiFrameData) override;

private:
	bool readSourceFile(std::string& str, const std::string& filename);
	void createContext();
	void setMesh();
	void execute();

	int frameCount = 0;

	struct OptixInfo
	{
		OptixDeviceContext context = 0;

		// シーン全体のInstance acceleration structure
		//InstanceAccelData ias = {};
		// GPU上におけるシーンの球体データ全てを格納している配列のポインタ
		void* d_sphere_data = nullptr;
		// GPU上におけるシーンの三角形データ全てを格納している配列のポインタ
		void* d_mesh_data = nullptr;
		
		OptixModule module = nullptr;
		OptixPipelineCompileOptions pipeline_compile_options = {};
		OptixPipeline               pipeline = nullptr;

		// Ray generation プログラム 
		OptixProgramGroup           raygen_prg = nullptr;
		// Miss プログラム
		OptixProgramGroup           miss_prg = nullptr;

		// 球体用のHitGroup プログラム
		OptixProgramGroup           sphere_hitgroup_prg = nullptr;
		// メッシュ用のHitGroupプログラム
		OptixProgramGroup           mesh_hitgroup_prg = nullptr;

		// マテリアル用のCallableプログラム
		// OptiXでは基底クラスのポインタを介した、派生クラスの関数呼び出し (ポリモーフィズム)が
		// 禁止されているため、Callable関数を使って疑似的なポリモーフィズムを実現する
		// ここでは、Lambertian, Dielectric, Metal の3種類を実装している
		//CallableProgram             lambertian_prg = {};
		//CallableProgram             dielectric_prg = {};
		//CallableProgram             metal_prg = {};

		// テクスチャ用のCallableプログラム
		// Constant ... 単色、Checker ... チェッカーボード
		//CallableProgram             constant_prg = {};
		//CallableProgram             checker_prg = {};

		// CUDA stream
		CUstream                    stream = 0;

		// Pipeline launch parameters
		// CUDA内で extern "C" __constant__ Params params
		// と宣言することで、全モジュールからアクセス可能である。
		//Params                      params;
		//Params* d_params;

		// Shader binding table
		OptixShaderBindingTable     sbt = {};
	} optixInfo;
	
	gl::TextureRef texture;

	std::shared_ptr<TriMesh> mesh;
	inline static const glm::ivec2 ImageSize = glm::ivec2(800, 600);

};

void MyApp::onSetup(MyFrameData* frameData, MyUIFrameData* uiFrameData) {
	frameData->surface = Surface(ImageSize.x, ImageSize.y, false);
	frameData->elapsed = 0.0f;
	frameData->triNum = 0;

	getWindow()->setTitle("xitils");
	setWindowSize(ImageSize.x, ImageSize.y);
	setFrameRate(60);

	const int subdivision = 200;
	auto teapot = std::make_shared<Teapot>();
	teapot->subdivisions(subdivision);
	mesh = std::make_shared<TriMesh>(*teapot);

	ui::initialize();

	createContext();

	setMesh();

	Vector2f cameraRange;
	cameraRange.x = 4.0f;
	cameraRange.y = cameraRange.x * 3.0f / 4.0f;

	// CUDAの初期化
	CUDA_CHECK(cudaFree(0));

	OptixDeviceContext context;
	CUcontext   cu_ctx = 0;
	OPTIX_CHECK(optixInit());
	OptixDeviceContextOptions options = {};
	options.logCallbackFunction = &contextLogCallback;
	// Callbackで取得するメッセージのレベル
	// 0 ... disable、メッセージを受け取らない
	// 1 ... fatal、修復不可能なエラー。コンテクストやOptiXが不能状態にある
	// 2 ... error、修復可能エラー。
	// 3 ... warning、意図せぬ挙動や低パフォーマンスを導くような場合に警告してくれる
	// 4 ... print、全メッセージを受け取る
	options.logCallbackLevel = 4;
	OPTIX_CHECK(optixDeviceContextCreate(cu_ctx, &options, &context));

	optixInfo.context = context;





	OptixModuleCompileOptions module_compile_options = {};
	module_compile_options.maxRegisterCount = OPTIX_COMPILE_DEFAULT_MAX_REGISTER_COUNT;
	module_compile_options.optLevel = OPTIX_COMPILE_OPTIMIZATION_DEFAULT;
	// ~7.3 系では OPTIX_COMPILE_DEBUG_LEVEL_LINEINFO
	module_compile_options.debugLevel = OPTIX_COMPILE_DEBUG_LEVEL_MINIMAL;

	optixInfo.pipeline_compile_options.usesMotionBlur = false;
	optixInfo.pipeline_compile_options.traversableGraphFlags = OPTIX_TRAVERSABLE_GRAPH_FLAG_ALLOW_ANY;
	optixInfo.pipeline_compile_options.numPayloadValues = 2;

	// Attributeの個数設定
	// Sphereの交差判定で法線とテクスチャ座標を intersection -> closesthitに渡すので
	// (x, y, z) ... 3次元、(s, t) ... 2次元 で計5つのAttributeが必要 
	// optixinOneWeekend.cu:347行目参照
	optixInfo.pipeline_compile_options.numAttributeValues = 5;

#ifdef DEBUG 
	optixInfo.pipeline_compile_options.exceptionFlags = OPTIX_EXCEPTION_FLAG_DEBUG | OPTIX_EXCEPTION_FLAG_TRACE_DEPTH | OPTIX_EXCEPTION_FLAG_STACK_OVERFLOW;
#else
	optixInfo.pipeline_compile_options.exceptionFlags = OPTIX_EXCEPTION_FLAG_NONE;
#endif
	// Pipeline launch parameterの変数名
	optixInfo.pipeline_compile_options.pipelineLaunchParamsVariableName = "params";

	size_t      inputSize = 0;
	auto file = cinder::loadFile("F:/workspace/Xitils/Sandbox/_Experimental/RaycasterOptix/optixPathTracer.cu");
	const char* input = (const char*)file->getBuffer()->getData();
	inputSize = file->getBuffer()->getSize();

	// PTXからModuleを作成
	char   log[2048];
	size_t sizeof_log = sizeof(log);
	OPTIX_CHECK_LOG(optixModuleCreateFromPTX(
		optixInfo.context,      // OptixDeviceContext
		&module_compile_options,
		&optixInfo.pipeline_compile_options,
		input,
		inputSize,
		log,
		&sizeof_log,
		&optixInfo.module       // OptixModule
	));

}

void MyApp::execute() {
	//RTsize n;
	//rays->getSize(n);
	//context->launch(0, n);
}

void MyApp::onCleanup(MyFrameData* frameData, MyUIFrameData* uiFrameData) {
	//cudaFree(rays_d);
	//cudaFree(hits_d);
	//cudaFree(image_d);

	//indexBuffer->destroy();
	//positionBuffer->destroy();
	//context->destroy();
}

void MyApp::onUpdate(MyFrameData& frameData, const MyUIFrameData& uiFrameData) {
	auto time_start = std::chrono::system_clock::now();

	const int n = ImageSize.x * ImageSize.y;

	execute();

	//cudaMalloc(&image_d, n * sizeof(optix::float3));
	//image_h.resize(n);
	//shadeHitsOnDevice(image_d, n, hits_d);
	//cudaMemcpy(&image_h[0], image_d, (size_t)n * sizeof(optix::float3), cudaMemcpyDeviceToHost);

#pragma omp parallel for schedule(dynamic, 1)
	for (int y = 0; y < frameData.surface.getHeight(); ++y) {
#pragma omp parallel for schedule(dynamic, 1)
		for (int x = 0; x < frameData.surface.getWidth(); ++x) {

			Vector3f color(0, 0, 0);

			Vector2f cameraRange;
			cameraRange.x = 4.0f;
			cameraRange.y = cameraRange.x * 3.0f / 4.0f;

			Vector2f cameraOffset(0, 0.5f);

			float nx = (float)x / frameData.surface.getWidth();
			float ny = (float)y / frameData.surface.getHeight();

			xitils::Ray ray;
			ray.o = Vector3f((nx - 0.5f) * cameraRange.x + cameraOffset.x, -(ny - 0.5f) * cameraRange.y + cameraOffset.y, -100);
			ray.d = normalize(Vector3f(0, 0, 1));

			//color.x = image_h[(ImageSize.y-1 - y) * ImageSize.x + x].x;
			//color.y = image_h[(ImageSize.y-1 - y) * ImageSize.x + x].y;
			//color.z = image_h[(ImageSize.y-1 - y) * ImageSize.x + x].z;

			ColorA8u colA8u;
			colA8u.r = xitils::clamp((int)(color.x * 255), 0, 255);
			colA8u.g = xitils::clamp((int)(color.y * 255), 0, 255);
			colA8u.b = xitils::clamp((int)(color.z * 255), 0, 255);
			colA8u.a = 255;

			frameData.surface.setPixel(ivec2(x, y), colA8u);

			frameData.triNum = mesh->getNumTriangles();
		}
	}

	++frameCount;

	auto time_end = std::chrono::system_clock::now();
	frameData.elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - time_start).count();
}

void MyApp::onDraw(const MyFrameData& frameData, MyUIFrameData& uiFrameData) {
	texture = gl::Texture::create(frameData.surface);

	gl::clear(Color::gray(0.5f));

	auto windowSize = getWindowSize();

	gl::draw(texture, (windowSize - ImageSize)/2);

	ImGui::Begin("ImGui Window");
	ImGui::Text(("Image Resolution: " + std::to_string(ImageSize.x) + " x " + std::to_string(ImageSize.y)).c_str());
	ImGui::Text(("Elapsed: " + std::to_string(frameData.elapsed) + " ms / frame").c_str());
	ImGui::Text(("Triangles: " + std::to_string(frameData.triNum)).c_str());
	ImGui::End();

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
	//context = optix::Context::create();
	//context->setRayTypeCount(1);
	//context->setEntryPointCount(1);

	//context->setStackSize(200);

	//const std::vector<int> enabled_devices = context->getEnabledDevices();
	//context->setDevices(enabled_devices.begin(), enabled_devices.begin() + 1);
	//optix_device_ordinal = enabled_devices[0];
	//{
	//	cuda_device_ordinal = -1;
	//	context->getDeviceAttribute(optix_device_ordinal, RT_DEVICE_ATTRIBUTE_CUDA_DEVICE_ORDINAL, sizeof(int), &cuda_device_ordinal);
	//}
	//
	//cudaSetDevice(cuda_device_ordinal);

	//std::string ptx;
	//readSourceFile(ptx, "RaycasterOptix_generated_RaycasterOptix.cu.ptx");

	//geometry = context->createGeometry();

	//geometry->setIntersectionProgram(context->createProgramFromPTXString(ptx, "intersect"));
	//geometry->setBoundingBoxProgram(context->createProgramFromPTXString(ptx, "bounds"));

	//material = context->createMaterial();
	//optix::GeometryInstance geometry_instance = context->createGeometryInstance(geometry, &material, &material + 1);
	//optix::GeometryGroup geometry_group = context->createGeometryGroup(&geometry_instance, &geometry_instance + 1);
	//geometry_group->setAcceleration(context->createAcceleration("Trbvh"));
	//context["top_object"]->set(geometry_group);

	//// Closest hit program for returning geometry attributes.  No shading.
	//optix::Program closest_hit = context->createProgramFromPTXString(ptx, "closest_hit");
	//material->setClosestHitProgram( /*ray type*/ 0, closest_hit);

	//// Raygen program that reads rays directly from an input buffer.
	//optix::Program ray_gen = context->createProgramFromPTXString(ptx, "ray_gen");
	//context->setRayGenerationProgram( /*entry point*/ 0, ray_gen);
}

void MyApp::setMesh() {
	//int vertexNum = mesh->getNumVertices();
	//positionBuffer = context->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT3, vertexNum);
	//memcpy(positionBuffer->map(), mesh->getPositions<3>(), vertexNum * 3 * sizeof(float));
	//positionBuffer->unmap();

	//int trianglesNum = mesh->getNumTriangles();
	//indexBuffer = context->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_INT3, trianglesNum);
	//memcpy(indexBuffer->map(), mesh->getIndices().data(), trianglesNum * 3 * sizeof(int32_t));
	//indexBuffer->unmap();

	//geometry->setPrimitiveCount(trianglesNum);
	//geometry["vertex_buffer"]->set(positionBuffer);
	//geometry["index_buffer"]->set(indexBuffer);

	//// Connect texcoord buffer, used for masking
	//const int num_texcoords = 0;
	//texcoordBuffer = context->createBuffer(RT_BUFFER_INPUT, RT_FORMAT_FLOAT2, num_texcoords);
	//geometry["texcoord_buffer"]->set(texcoordBuffer);
}

XITILS_APP(MyApp)
