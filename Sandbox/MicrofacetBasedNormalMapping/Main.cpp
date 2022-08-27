

#include <Xitils/AccelerationStructure.h>
#include <Xitils/App.h>
#include <Xitils/Camera.h>
#include <Xitils/Geometry.h>
#include <Xitils/PathTracer.h>
#include <Xitils/Scene.h>
#include <Xitils/TriangleMesh.h>
#include <Xitils/RenderTarget.h>
#include <CinderImGui.h>

enum _MethodMode{
	MethodModeNaive,
	MethodModeProposed
};

enum _TangentFacetMode {
	SameMaterialExplicit,
	SpecularExplicit
};

const _MethodMode MethodMode = MethodModeProposed;
const _TangentFacetMode TangentFacetMode = SameMaterialExplicit;


using namespace xitils;
using namespace ci;
using namespace ci::app;
using namespace ci::geom;

// wi ���W�I���g������̕����A�܂��� wo ���V�F�[�f�B���O�@�������`����锼���̊O�ł���ꍇ��
// �l�� 0 �ƂȂ� Glossy
struct GlossyClamped : public Material {
public:

	Vector3f albedo;
	float sharpness;

	GlossyClamped(const Vector3f& albedo, float sharpness) :
		albedo(albedo),
		sharpness(sharpness)
	{}

	Vector3f bsdfCos(const SurfaceIntersection& isect, Sampler& sampler, const Vector3f& wi) const override {
		const auto& n = isect.shading.n;
		float N = 2 * M_PI / (sharpness + 2);

		if (dot(isect.wo, n) > 0.0f && dot(wi, n) > 0.0f) {
			return albedo * powf(dot((isect.wo + wi).normalize(), n), sharpness) / N;
		} else {
			return Vector3f(0.0f);
		}
	}

	Vector3f evalAndSample(const SurfaceIntersection& isect, Sampler& sampler, Vector3f* wi, float* pdf) const override {
		float r1 = sampler.randf();
		float r2 = sampler.randf();

		const Vector3f& n = isect.shading.n;

		BasisVectors basis(n);
		float sqrt = safeSqrt(1 - powf(r2, 2 / (sharpness + 1.0f)));
		Vector3f h = basis.toGlobal(powf(r2, 1 / (sharpness + 1.0f)), cosf(2 * M_PI * r1) * sqrt, sinf(2 * M_PI * r1) * sqrt).normalize();
		*wi = -(isect.wo - 2.0f * dot(isect.wo, h) * h).normalize();

		*pdf = this->getPDF(isect, *wi);

		if (*pdf == 0.0f) { return Vector3f(0.0f); }

		return bsdfCos(isect, sampler, *wi) / *pdf;
	}


	float getPDF(const SurfaceIntersection& isect, const Vector3f& wi) const override {
		const auto& n = isect.shading.n;
		if (dot(isect.wo, n) > 0.0f && dot(wi, n) > 0.0f) {
			return (sharpness + 1) / (2 * M_PI) * powf(clampPositive(dot((isect.wo + wi).normalize(), n)), sharpness);
		} else {
			return 0.0f;
		}
	}

};

class MicrofacetNormalMapping : public Material {
public:

	// material_wp �̓x�[�X�ƂȂ�}�e���A��
	MicrofacetNormalMapping(const std::shared_ptr<Material>& material_wp):
		material_wp(material_wp)
	{}

	Vector3f bsdfCos(const SurfaceIntersection& isect, Sampler& sampler, const Vector3f& wi) const override {
		const auto& wo = isect.wo;

		// ---- �ڐ�����������@�� wt ����߂�
		Vector3f wg = isect.n;
		Vector3f wp = isect.shading.n;
		if (dot(wo, wg) < 0.0f) { wg *= -1; }
		if (dot(wg, wp) < 0.0f) { wp *= -1; }

		if (wg == wp) {
			return material_wp->bsdfCos(isect, sampler, wi);
		}

		Vector3f wt = cross(cross(wg, wp).normalize(), wg).normalize();

		if (wt.hasNan()) {
			// ���l�I�ȃG���[
			return Vector3f(0.0f);
		}
		if (dot(wp, wt) > 0) {
			wt *= -1;
		}

		// ---- �Ǐ��U����l������ bsdf �̒l��A�����I�ȃ��[�v��p���ċ��߂�
		Vector3f result(0.0f);
		Vector3f weight(1.0f);
		bool d = sampler.randf() < lambda_p(wo, wp, wt, wg); // true:wp, false:wt
		Vector3f wr = -wo;
		SurfaceIntersection isectfacet;
		isectfacet = isect;
		int i = 0;
		float pdf;

		while (true) {
			const Vector3f& wm = d ? wp : wt;
			isectfacet.n = wm;
			isectfacet.shading.n = wm;
			isectfacet.wo = -wr;

			if (d) {

				if (TangentFacetMode == SpecularExplicit) {
					Vector3f wi_dash = -(wi - 2 * dot(wt, wi) * wt).normalize();
					wi_dash *= -1;
					result += weight * (1.0f - G1(wi_dash, wp, wp, wt, wg)) * G1(wi, wt, wp, wt, wg) * material_wp->bsdfCos(isectfacet, sampler, wi_dash);
				}

				result += weight * G1(wi, wm, wp, wt, wg) * material_wp->bsdfCos(isectfacet, sampler, wi);
				weight *= material_wp->evalAndSample(isectfacet, sampler, &wr, &pdf);

			} else {
				if (TangentFacetMode == SameMaterialExplicit) {
					result += weight * G1(wi, wm, wp, wt, wg) * material_wp->bsdfCos(isectfacet, sampler, wi);
					weight *= material_wp->evalAndSample(isectfacet, sampler, &wr, &pdf);
				} else if (TangentFacetMode == SpecularExplicit) {
					wr = -(isectfacet.wo - 2 * dot(isectfacet.shading.n, isectfacet.wo) * isectfacet.shading.n).normalize();
				}
			}

			float g = G1(wr, wm, wp, wt, wg);

			if (sampler.randf() < g) {
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

	Vector3f evalAndSample(const SurfaceIntersection& isect, Sampler& sampler, Vector3f* wi, float* pdf) const override {
		const auto& wo = isect.wo;

		// ---- �ڐ�����������@�� wt ����߂�
		Vector3f wg = isect.n;
		Vector3f wp = isect.shading.n;
		if (dot(wo, wg) < 0.0f) { wg *= -1; }
		if (dot(wg, wp) < 0.0f) { wp *= -1; }

		if (wg == wp) {
			return material_wp->evalAndSample(isect, sampler, wi, pdf);
		}

		Vector3f wt = cross(cross(wg, wp).normalize(), wg).normalize();
		
		if ( wt.hasNan() ) {
			// ���l�I�ȃG���[
			return Vector3f(0.0f);
		}
		if (dot(wp,wt) > 0) {
			wt *= -1;
		}

		// ---- wi �̕����� bsdf �̒l��A�����I�ȃ��[�v��p���ċ��߂�
		Vector3f result(0.0f);
		Vector3f weight(1.0f);
		bool d = sampler.randf() < lambda_p(wo, wp, wt, wg); // true:wp, false:wt
		Vector3f wr = -wo;
		SurfaceIntersection isectfacet;
		isectfacet = isect;
		int i = 0;
		float tmppdf;
		bool specular = true;
		while (true) {
			++i;

			const Vector3f& wm = d ? wp : wt;

			isectfacet.n = wm;
			isectfacet.shading.n = wm;
			isectfacet.wo = -wr;
			if (d) {
				weight *= material_wp->evalAndSample(isectfacet, sampler, &wr, &tmppdf);
				if (tmppdf >= 0.0f) { specular = false; }
			} else {
				if(TangentFacetMode == SameMaterialExplicit) {
					weight *= material_wp->evalAndSample(isectfacet, sampler, &wr, &tmppdf);
					if (tmppdf >= 0.0f) { specular = false; }
				}else if (TangentFacetMode == SpecularExplicit) {
					wr = -(isectfacet.wo - 2 * dot(isectfacet.shading.n, isectfacet.wo) * isectfacet.shading.n).normalize();
				}
			}

			float g = G1(wr, wm, wp, wt, wg);

			if (sampler.randf() < g ) {
				break;
			}

			d = !d;

			if (weight.isZero()) { break; }

			if (i > 100) {
				weight *= Vector3f(0.0f);
				break;
			}

		}
		result += weight;
		*wi = wr;

		if (!specular) {
			*pdf = this->getPDF(isect, *wi);
		} else {
			*pdf = -1.0f;
		}

		return result;
	}

	float getPDF(const SurfaceIntersection& isect, const Vector3f& wi) const override {

		// �����̔��˂� diffuse �ŋߎ����āApdf �̋ߎ��l����߂�

		const auto& wo = isect.wo;

		Vector3f wg = isect.n;
		Vector3f wp = isect.shading.n;
		if (dot(wo, wg) < 0.0f) { wg *= -1; }
		if (dot(wg, wp) < 0.0f) { wp *= -1; }

		if (wg == wp) {
			return material_wp->getPDF(isect, wi);
		}

		Vector3f wt = cross(cross(wg, wp).normalize(), wg).normalize();

		if (wt.hasNan()) {
			// ���l�I�ȃG���[
			return 1.0f;
		}
		if (dot(wp, wt) > 0) {
			wt *= -1;
		}

		Vector3f result(0.0f);
		Vector3f weight(1.0f);
		float probability_wp = lambda_p(wo, wp, wt, wg);
		SurfaceIntersection isectfacet;
		isectfacet = isect;
		
		float pdf = 0;
		float diff_pdf = 0.0f;

		float g_wi_wp = G1(wi, wp, wp, wt, wg);
		if (probability_wp > 0.0f) {
			isectfacet.n = wp;
			isectfacet.shading.n = wp;
			pdf += probability_wp * g_wi_wp * material_wp->getPDF(isectfacet, wi);
			diff_pdf += probability_wp * (1.0f - g_wi_wp);
		}

		if (probability_wp < 1.0f) {
			isectfacet.n = wt;
			isectfacet.shading.n = wt;
			float g_wi_wt = G1(wi, wt, wp, wt, wg);
			if (TangentFacetMode == SameMaterialExplicit) {
				pdf += probability_wp * g_wi_wt * material_wp->getPDF(isectfacet, wi);
				diff_pdf += probability_wp * g_wi_wt;
			} else if (TangentFacetMode == SpecularExplicit) {
				Vector3f wr = -(isectfacet.wo - 2 * dot(isectfacet.shading.n, isectfacet.wo) * isectfacet.shading.n).normalize();
				float g_wr_wt = G1(wr, wt, wp, wt, wg);
				
				isectfacet.n = wp;
				isectfacet.shading.n = wp;
				isectfacet.wo = -wr;

				pdf += probability_wp * (1.0f - g_wr_wt) * g_wi_wp * material_wp->getPDF(isectfacet, wi);
				diff_pdf += probability_wp * (1.0f - g_wr_wt) * (1.0f - g_wi_wp);
			}
		}
		
		if (TangentFacetMode == SameMaterialExplicit) {
			return pdf + diff_pdf * clampPositive(dot(wi, wg)) / M_PI;
		} else if (TangentFacetMode == SpecularExplicit) {
			return pdf + diff_pdf * clampPositive(dot(wi, wg)) / M_PI * 0.5f;
		}
	}

private:
	std::shared_ptr<Material> material_wp;

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
		return dot(w, wm) > 0.0f ? min(1.0f, clampPositive(dot(w, wg)) / (a_p(w, wp, wg) + a_t(w, wp, wt, wg))) : 0.0f;
	};
};


struct MyFrameData {
	float frameElapsed;
	Surface surface;
	int triNum;
	int sampleNum = 0;
};

struct MyUIFrameData {
	Vector3f rot;
};

class MyApp : public xitils::app::XApp<MyFrameData, MyUIFrameData> {
public:
	void onSetup(MyFrameData* frameData, MyUIFrameData* uiFrameData) override;
	void onCleanup(MyFrameData* frameData, MyUIFrameData* uiFrameData) override;
	void onUpdate(MyFrameData& frameData, const MyUIFrameData& uiFrameData) override;
	void onDraw(const MyFrameData& frameData, MyUIFrameData& uiFrameData) override;

private:
	
	gl::TextureRef texture;

	std::shared_ptr<Scene> scene;
	inline static const glm::ivec2 ImageSize = glm::ivec2(800, 800);

	std::shared_ptr<SimpleRenderTarget> renderTarget;
	std::shared_ptr<PathTracer> pathTracer;

	decltype(std::chrono::system_clock::now()) time_start;
};

void MyApp::onSetup(MyFrameData* frameData, MyUIFrameData* uiFrameData) {
	time_start = std::chrono::system_clock::now();
	
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
		transformTRS(Vector3f(0, 2.0f, -5), Vector3f(10, 0, 0), Vector3f(1)), 
		40 * ToRad, (float)ImageSize.y / ImageSize.x
		);

	auto diffuse_white = std::make_shared <Diffuse>(Vector3f(0.8f));
	auto cube = std::make_shared<xitils::Cube>();
	scene->addObject(
		std::make_shared<Object>( cube, diffuse_white, transformTRS(Vector3f(0,0,0), Vector3f(), Vector3f(4, 0.01f, 4)))
	);

	scene->skySphere = std::make_shared<SkySphereFromImage>("rnl_probe.hdr");

	auto baseMaterial = std::make_shared<GlossyClamped>(Vector3f(0.8f), 30.0f);

	std::shared_ptr<Material> sphereMaterial;
	
	if (MethodMode == MethodModeNaive) {
		sphereMaterial = baseMaterial;
	} else if (MethodMode == MethodModeProposed) {
		sphereMaterial = 
			std::make_shared<MicrofacetNormalMapping>(
			baseMaterial
			);
	}

	sphereMaterial->normalmap = std::make_shared<Texture>("normal_scale.jpg");

	scene->addObject(std::make_shared<Object>(std::make_shared<xitils::Sphere>(Vector2f(2, 1)), sphereMaterial,
		transformTRS(Vector3f(0.0f, 1, 0.0f), Vector3f(0, -30, 30), Vector3f(1.0f))
		));

	scene->buildAccelerationStructure();

	renderTarget = std::make_shared<SimpleRenderTarget>(ImageSize.x, ImageSize.y);

	pathTracer = std::make_shared<StandardPathTracer>();
}

void MyApp::onCleanup(MyFrameData* frameData, MyUIFrameData* uiFrameData) {
}

void MyApp::onUpdate(MyFrameData& frameData, const MyUIFrameData& uiFrameData) {
	auto time_start = std::chrono::system_clock::now();

	int sample = 1;

	frameData.sampleNum += sample;

	renderTarget->render(*scene, sample, [&](const Vector2f& pFilm, Sampler& sampler, Vector3f& color) {
		auto ray = scene->camera->GenerateRay(pFilm, sampler);

		color += pathTracer->eval(*scene, sampler, ray).color * 0.5f;

	});

	renderTarget->map(&frameData.surface, [&frameData](const Vector3f& pixel)
		{
			auto color = pixel/ frameData.sampleNum;
			ci::ColorA8u colA8u;
			colA8u.r = xitils::clamp((int)(color.x * 255), 0, 255);
			colA8u.g = xitils::clamp((int)(color.y * 255), 0, 255);
			colA8u.b = xitils::clamp((int)(color.z * 255), 0, 255);
			colA8u.a = 255;
			return colA8u;
		});

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
	ImGui::Text(("Elapsed : " + std::to_string(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - time_start).count()) + " s").c_str());
	ImGui::Text(("Samples: " + std::to_string(frameData.sampleNum)).c_str());

	ImGui::End();
}


XITILS_APP(MyApp)
