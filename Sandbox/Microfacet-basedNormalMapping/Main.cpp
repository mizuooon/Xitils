

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

class DiffuseClamped : public Material {
public:
	Vector3f albedo;

	DiffuseClamped(const Vector3f& albedo) :
		albedo(albedo)
	{
		specular = false;
	}

	Vector3f bsdfCos(const SurfaceInteraction& isect, const std::shared_ptr<Sampler>& sampler, const Vector3f& wi) const override {
		return dot(isect.wo, isect.shading.n) > 0.0f && dot(wi, isect.n) > 0.0f ? albedo / M_PI * clampPositive(dot(wi, isect.shading.n)) : Vector3f(0.0f);
	}

	Vector3f evalAndSample(const SurfaceInteraction& isect, const std::shared_ptr<Sampler>& sampler, Vector3f* wi, float* pdf) const override {
		const auto& n = isect.shading.n;
		*wi = sampleVectorFromCosinedHemiSphere(n, sampler);
		*pdf = dot(*wi, n) / M_PI;
		return dot(isect.wo, isect.shading.n) > 0.0f && dot(*wi, isect.n) > 0.0f ? albedo : Vector3f(0.0f);
	}

	float pdf(const SurfaceInteraction& isect, const Vector3f& wi) const override {
		const auto& n = isect.shading.n;
		return dot(isect.wo, isect.shading.n) > 0.0f && dot(wi, isect.n) > 0.0f ? clampPositive(dot(wi, n)) / M_PI : 0.0f;
	}
};

class SpecularClamped : public Material {
public:

	SpecularClamped() {
		specular = true;
	}

	Vector3f evalAndSample(const SurfaceInteraction& isect, const std::shared_ptr<Sampler>& sampler, Vector3f* wi, float* pdf) const override {
		const auto& wo = isect.wo;

		const auto& n = isect.shading.n;
		*wi = -(wo - 2 * dot(n, wo) * n).normalize();
		return dot(*wi, n) > 0.0f ? Vector3f(1.0f) : Vector3f(0.0f);
	}

};

class SpecularMicrofacetNormalMapping : public Material {
public:
	
	enum TangentFacetMode {
		SameMaterialExplicit,
		SameMaterialAnalyticDiffuse,
		SpecularExplicit,
		SpecularAnalytic2nd
	};

	SpecularMicrofacetNormalMapping(const std::shared_ptr<Material>& material_wp, TangentFacetMode tangentFacetMode):
		material_wp(material_wp),
		tangentFacetMode(tangentFacetMode)
	{
		specular = material_wp->specular;
	}

	Vector3f bsdfCos(const SurfaceInteraction& isect, const std::shared_ptr<Sampler>& sampler, const Vector3f& wi) const override {
		const auto& wo = isect.wo;

		Vector3f wg = isect.n;
		Vector3f wp = isect.shading.n;
		if (dot(wo, wg) < 0.0f) { wg *= -1; }
		if (dot(wg, wp) < 0.0f) { wp *= -1; }

		if (wg == wp) {
			return material_wp->bsdfCos(isect, sampler, wi);
		}

		Vector3f wt = cross(cross(wg, wp).normalize(), wg).normalize();

		if (wt.hasNan()) {
			// 数値的なエラー
			return Vector3f(0.0f);
		}
		if (dot(wp, wt) > 0) {
			wt *= -1;
		}

		Vector3f result(0.0f);
		Vector3f weight(1.0f);
		bool d = sampler->randf() < lambda_p(wo, wp, wt, wg); // true:wp, false:wt
		Vector3f wr = -wo;
		SurfaceInteraction isectfacet;
		isectfacet = isect;
		int i = 0;
		float pdf;

		while (true) {
			const Vector3f& wm = d ? wp : wt;
			isectfacet.n = wm;
			isectfacet.shading.n = wm;
			isectfacet.wo = -wr;

			if (d) {
				result += weight * G1(wi, wm, wp, wt, wg) * material_wp->bsdfCos(isectfacet, sampler, wi);
				weight *= material_wp->evalAndSample(isectfacet, sampler, &wr, &pdf);

			} else {
				if (tangentFacetMode == SameMaterialExplicit) {
					result += weight * G1(wi, wm, wp, wt, wg) * material_wp->bsdfCos(isectfacet, sampler, wi);
					weight *= material_wp->evalAndSample(isectfacet, sampler, &wr, &pdf);
				} else if (tangentFacetMode == SpecularExplicit) {
					wr = -(isectfacet.wo - 2 * dot(isectfacet.shading.n, isectfacet.wo) * isectfacet.shading.n).normalize();
				}
			}

			float g = G1(wr, wm, wp, wt, wg);

			if (sampler->randf() < g) {
				break;
			}

			d = !d;

			if (weight.isZero()) { break; }

			++i;
			if (i > 100) {
				break;
			}

		}

		return result;
	}

	Vector3f evalAndSample(const SurfaceInteraction& isect, const std::shared_ptr<Sampler>& sampler, Vector3f* wi, float* pdf) const override {
		const auto& wo = isect.wo;

		Vector3f wg = isect.n;
		Vector3f wp = isect.shading.n;
		if (dot(wo, wg) < 0.0f) { wg *= -1; }
		if (dot(wg, wp) < 0.0f) { wp *= -1; }

		if (wg == wp) {
			return material_wp->evalAndSample(isect, sampler, wi, pdf);
		}

		Vector3f wt = cross(cross(wg, wp).normalize(), wg).normalize();
		
		if ( wt.hasNan() ) {
			// 数値的なエラー
			return Vector3f(0.0f);
		}
		if (dot(wp,wt) > 0) {
			wt *= -1;
		}

		Vector3f result(0.0f);
		Vector3f weight(1.0f);
		bool d = sampler->randf() < lambda_p(wo, wp, wt, wg); // true:wp, false:wt
		Vector3f wr = -wo;
		SurfaceInteraction isectfacet;
		isectfacet = isect;
		int i = 0;
		while (true) {
			const Vector3f& wm = d ? wp : wt;

			isectfacet.n = wm;
			isectfacet.shading.n = wm;
			isectfacet.wo = -wr;
			if (d) {
				weight *= material_wp->evalAndSample(isectfacet, sampler, &wr, pdf);
			} else {
				if(tangentFacetMode == SameMaterialExplicit) {
					weight *= material_wp->evalAndSample(isectfacet, sampler, &wr, pdf);
				}else if (tangentFacetMode == SpecularExplicit) {
					wr = -(isectfacet.wo - 2 * dot(isectfacet.shading.n, isectfacet.wo) * isectfacet.shading.n).normalize();
				}
			}

			float g = G1(wr, wm, wp, wt, wg);
			if (sampler->randf() < g ) {
				break;
			}

			d = !d;

			if (weight.isZero()) { break; }

			++i;
			if (i > 100) {
				//printf("%f %f %f %f\n", G1(wr, wm), a_p(wr), a_t(wt), dot(wp, wg));
				weight *= Vector3f(0.0f);
				break;
			}

		}
		result += weight;
		*wi = wr;

		*pdf = this->pdf(isect, *wi);

		return result;
	}

	// pdf の近似値を求めている
	float pdf(const SurfaceInteraction& isect, const Vector3f& wi) const override {
		const auto& wo = isect.wo;

		Vector3f wg = isect.n;
		Vector3f wp = isect.shading.n;
		if (dot(wo, wg) < 0.0f) { wg *= -1; }
		if (dot(wg, wp) < 0.0f) { wp *= -1; }

		if (wg == wp) {
			return material_wp->pdf(isect, wi);
		}

		Vector3f wt = cross(cross(wg, wp).normalize(), wg).normalize();

		if (wt.hasNan()) {
			// 数値的なエラー
			return 1.0f;
		}
		if (dot(wp, wt) > 0) {
			wt *= -1;
		}

		Vector3f result(0.0f);
		Vector3f weight(1.0f);
		float probability_wp = lambda_p(wo, wp, wt, wg);
		SurfaceInteraction isectfacet;
		isectfacet = isect;
		
		float pdf = 0;
		float diff_pdf = 0.0f;

		float g_wi_wp = G1(wi, wp, wp, wt, wg);
		float pdf_wi_wp = material_wp->pdf(isectfacet, wi);
		if (probability_wp > 0.0f) {
			isectfacet.n = wp;
			isectfacet.shading.n = wp;
			pdf += probability_wp * g_wi_wp * pdf_wi_wp;
			diff_pdf += probability_wp * (1.0f - g_wi_wp);
		}

		if (probability_wp < 1.0f) {
			isectfacet.n = wt;
			isectfacet.shading.n = wt;
			float g_wi_wt = G1(wi, wt, wp, wt, wg);
			if (tangentFacetMode == SameMaterialExplicit) {
				pdf += g_wi_wt * clampPositive(dot(wi,wt)) / M_PI;
				diff_pdf += probability_wp * g_wi_wt;
			} else if (tangentFacetMode == SpecularExplicit) {
				Vector3f wr = -(isectfacet.wo - 2 * dot(isectfacet.shading.n, isectfacet.wo) * isectfacet.shading.n).normalize();
				float g_wr_wt = G1(wr, wt, wp, wt, wg);
				pdf += probability_wp * (1.0f - g_wr_wt) * g_wi_wp * pdf_wi_wp;
				diff_pdf += probability_wp * (1.0f - g_wr_wt) * (1.0f - g_wi_wp);
			}
		}

		return pdf + diff_pdf * clampPositive(dot(wi, wg)) / M_PI;
	}

private:
	std::shared_ptr<Material> material_wp;
	TangentFacetMode tangentFacetMode;


	float a_p (const Vector3f& w, const Vector3f& wp, const Vector3f& wg) const {
		return clampPositive(dot(w, wp)) / fabsf(dot(wp, wg));
	};
	float a_t (const Vector3f& w, const Vector3f& wp, const Vector3f& wt, const Vector3f& wg) const {
		float d = dot(wp, wg);
		return clampPositive(dot(w, wt)) * safeSqrt(1 - powf(d, 2)) / fabsf(d);
	};

	float lambda_p (const Vector3f& w, const Vector3f& wp, const Vector3f& wt, const Vector3f& wg) const {
		return a_p(w, wp, wg) / (a_p(w, wp, wg) + a_t(w, wp, wt, wg));
	};
	float G1 (const Vector3f& w, const Vector3f& wm, const Vector3f& wp, const Vector3f& wt, const Vector3f& wg) const{
		return dot(w, wm) > 0.0f ? min(1.0f, fabsf(dot(w, wg)) / (a_p(w, wp, wg) + a_t(w, wp, wt, wg))) : 0.0f;
	};
};


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

	//scene->camera = std::make_shared<PinholeCamera>(
	//	translate(0, 1.0f, -5), 60 * ToRad, (float)ImageSize.y / ImageSize.x
	//	);
	scene->camera = std::make_shared<PinholeCamera>(
		translate(0, 2.0f, -5), 60 * ToRad, (float)ImageSize.y / ImageSize.x
		);

	auto diffuse_white = std::make_shared <Diffuse>(Vector3f(0.8f));
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

	//auto sphere_material = std::make_shared<SpecularMicrofacetNormalMapping>(
	//	std::make_shared<SpecularClamped>(),
	//	SpecularMicrofacetNormalMapping::TangentFacetMode::SameMaterialExplicit
	//	);
	//auto sphere_material = std::make_shared<DiffuseClamped>(Vector3f(0.8f));
	auto sphere_material = std::make_shared<SpecularMicrofacetNormalMapping>(
		std::make_shared<DiffuseClamped>(Vector3f(0.8f)),
		SpecularMicrofacetNormalMapping::TangentFacetMode::SpecularExplicit
		);

	//sphere_material->normalmap = std::make_shared<TextureNormalHump>(16, 8, 0.3f);
	sphere_material->normalmap = std::make_shared<TextureFromFile>("normals.png");
	scene->addObject(std::make_shared<Object>(std::make_shared<Xitils::Sphere>(), sphere_material,
		transformTRS(Vector3f(0.0f, 1, 0.0f), Vector3f(0, -30, 30), Vector3f(1.0f))
		));

	scene->buildAccelerationStructure();

	//scene->skySphere = std::make_shared<SkySphereFromImage>("rnl_probe.hdr");

	renderTarget = std::make_shared<RenderTarget>(ImageSize.x, ImageSize.y);

	pathTracer = std::make_shared<StandardPathTracer>();
	//pathTracer = std::make_shared<DebugRayCaster>([](const SurfaceInteraction& isect) { 
	//	return (isect.shading.bitangent) *0.5f + Vector3f(0.5f);
	//	} );

	auto time_end = std::chrono::system_clock::now();

	frameData->initElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(time_end - init_time_start).count();
}

void MyApp::onCleanup(MyFrameData* frameData, MyUIFrameData* uiFrameData) {

}

void MyApp::onUpdate(MyFrameData& frameData, const MyUIFrameData& uiFrameData) {
	auto time_start = std::chrono::system_clock::now();

	int sample = 1;

	frameData.sampleNum += sample;

	renderTarget->render(scene, sample, [&](const Vector2f& pFilm, const std::shared_ptr<Sampler>& sampler, Vector3f& color) {
		auto ray = scene->camera->GenerateRay(pFilm, sampler);

		color += pathTracer->eval(scene, sampler, ray);

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
	ImGui::Text(("Samples: " + std::to_string(frameData.sampleNum)).c_str());

	ImGui::End();
}


XITILS_APP(MyApp)
