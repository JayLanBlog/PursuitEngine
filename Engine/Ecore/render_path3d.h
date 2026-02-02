#pragma once
#include "render_path2d.h"
#include "renderer.h"
#include "Engine/Device/graph_driver.h"
#include "resource_manager.h"
#include "scene.h"
#include "Module/Container/unorderedmap.h"


namespace pf
{

	class RenderPath3D :
		public RenderPath2D
	{
	public:
		enum AO
		{
			AO_DISABLED,	// no ambient occlusion
			AO_SSAO,		// simple brute force screen space ambient occlusion
			AO_HBAO,		// horizon based screen space ambient occlusion
			AO_MSAO,		// multi scale screen space ambient occlusion
			AO_RTAO,		// ray traced ambient occlusion
			// Don't alter order! (bound to lua manually)
		};
		enum class FSR2_Preset
		{
			// Guidelines: https://github.com/GPUOpen-Effects/FidelityFX-FSR2#scaling-modes
			Quality,
			Balanced,
			Performance,
			Ultra_Performance,
		};
	private:
		float exposure = 1.0f;
		float brightness = 0.0f;
		float contrast = 1.0f;
		float saturation = 1.0f;
		float bloomThreshold = 1.0f;
		float motionBlurStrength = 100.0f;
		float dofStrength = 10.0f;
		float sharpenFilterAmount = 0.28f;
		float outlineThreshold = 0.2f;
		float outlineThickness = 1.0f;
		XMFLOAT4 outlineColor = XMFLOAT4(0, 0, 0, 1);
		float aoRange = 1.0f;
		uint32_t aoSampleCount = 16;
		float aoPower = 1.0f;
		float chromaticAberrationAmount = 2.0f;
		uint32_t screenSpaceShadowSampleCount = 16;
		float screenSpaceShadowRange = 1;
		float eyeadaptionKey = 0.115f;
		float eyeadaptionRate = 1;
		float fsrSharpness = 1.0f;
		float fsr2Sharpness = 0.5f;
		float lightShaftsStrength = 0.5f;
		float lightShaftsFadeSpeed = 3.0f;
		float raytracedDiffuseRange = 10;
		float raytracedReflectionsRange = 10000.0f;
		float reflectionRoughnessCutoff = 0.6f;
		float ssgiDepthRejection = 8;
		pf::renderer::Tonemap tonemap = pf::renderer::Tonemap::ACES;
		float hdr_calibration = 1;

		AO ao = AO_DISABLED;
		bool fxaaEnabled = false;
		bool ssrEnabled = false;
		bool ssgiEnabled = false;
		bool raytracedReflectionsEnabled = false;
		bool raytracedDiffuseEnabled = false;
		bool reflectionsEnabled = true;
		bool shadowsEnabled = true;
		bool bloomEnabled = true;
		bool colorGradingEnabled = true;
		bool volumeLightsEnabled = true;
		bool lightShaftsEnabled = false;
		bool lensFlareEnabled = true;
		bool motionBlurEnabled = false;
		bool depthOfFieldEnabled = true;
		bool eyeAdaptionEnabled = false;
		bool sharpenFilterEnabled = false;
		bool outlineEnabled = false;
		bool chromaticAberrationEnabled = false;
		bool ditherEnabled = true;
		bool occlusionCullingEnabled = true;
		bool sceneUpdateEnabled = true;
		bool fsrEnabled = false;
		bool fsr2Enabled = false;
		bool mesh_blend = true;
		bool crtFilterEnabled = false;

		mutable bool first_frame = true;
		mutable bool prerender_happened = false;

		void RenderCameraComponents(pf::jobsystem::context& ctx) const;

	public:
		pf::graphics::Texture rtMain;
		pf::graphics::Texture rtMain_render; // can be MSAA
		pf::graphics::Texture rtPrimitiveID;
		pf::graphics::Texture rtPrimitiveID_render; // can be MSAA
		pf::graphics::Texture rtVelocity; // optional R16G16_FLOAT
		pf::graphics::Texture rtReflection; // contains the scene rendered for planar reflections, MSAA
		pf::graphics::Texture rtReflection_resolved; // contains the scene rendered for planar reflections, single sample
		pf::graphics::Texture rtRaytracedDiffuse; // raytraced diffuse screen space texture
		pf::graphics::Texture rtSSR; // standard screen-space reflection results
		pf::graphics::Texture rtSSGI; // standard screen-space GI results
		pf::graphics::Texture rtSceneCopy; // contains the rendered scene that can be fed into transparent pass for distortion effect
		pf::graphics::Texture rtSceneCopy_tmp; // temporary for gaussian mipchain
		pf::graphics::Texture rtWaterRipple; // water ripple sprite normal maps are rendered into this
		pf::graphics::Texture rtParticleDistortion_render; // contains distortive particles (can be MSAA)
		pf::graphics::Texture rtParticleDistortion; // contains distortive particles
		pf::graphics::Texture rtVolumetricLights; // contains the volumetric light results
		pf::graphics::Texture rtBloom; // contains the bright parts of the image + mipchain
		pf::graphics::Texture rtBloom_tmp; // temporary for bloom downsampling
		pf::graphics::Texture rtAO; // full res AO
		pf::graphics::Texture rtShadow; // raytraced shadows mask
		pf::graphics::Texture rtSun[3]; // 0: sun render target used for lightshafts (can be MSAA), 1: radial blurred lightshafts
		pf::graphics::Texture rtSun_resolved; // sun render target, but the resolved version if MSAA is enabled
		pf::graphics::Texture rtGUIBlurredBackground[3];	// downsampled, gaussian blurred scene for GUI
		pf::graphics::Texture rtShadingRate; // UINT8 shading rate per tile
		pf::graphics::Texture rtFSR[2]; // FSR upscaling result (full resolution LDR)
		pf::graphics::Texture rtOutlineSource; // linear depth but only the regions which have outline stencil

		pf::graphics::Texture rtPostprocess; // ping-pong with main scene RT in post-process chain

		pf::graphics::Texture depthBuffer_Main; // used for depth-testing, can be MSAA
		pf::graphics::Texture depthBuffer_Copy; // used for shader resource, single sample
		pf::graphics::Texture depthBuffer_Copy1; // used for disocclusion check
		pf::graphics::Texture depthBuffer_Reflection; // used for reflection, MSAA
		pf::graphics::Texture depthBuffer_Reflection_resolved; // used for reflection, single sample
		pf::graphics::Texture rtLinearDepth; // linear depth result + mipchain (max filter)
		pf::graphics::Texture reprojectedDepth; // prev frame depth reprojected into current, and downsampled for meshlet occlusion culling

		pf::graphics::Texture debugUAV; // debug UAV can be used by some shaders...
		pf::renderer::TiledLightResources tiledLightResources;
		pf::renderer::TiledLightResources tiledLightResources_planarReflection;
		pf::renderer::LuminanceResources luminanceResources;
		pf::renderer::SSAOResources ssaoResources;
		pf::renderer::MSAOResources msaoResources;
		pf::renderer::RTAOResources rtaoResources;
		pf::renderer::RTDiffuseResources rtdiffuseResources;
		pf::renderer::RTReflectionResources rtreflectionResources;
		pf::renderer::SSRResources ssrResources;
		pf::renderer::SSGIResources ssgiResources;
		pf::renderer::RTShadowResources rtshadowResources;
		pf::renderer::ScreenSpaceShadowResources screenspaceshadowResources;
		pf::renderer::DepthOfFieldResources depthoffieldResources;
		pf::renderer::MotionBlurResources motionblurResources;
		pf::renderer::AerialPerspectiveResources aerialperspectiveResources;
		pf::renderer::AerialPerspectiveResources aerialperspectiveResources_reflection;
		pf::renderer::VolumetricCloudResources volumetriccloudResources;
		pf::renderer::VolumetricCloudResources volumetriccloudResources_reflection;
		pf::renderer::BloomResources bloomResources;
		pf::renderer::SurfelGIResources surfelGIResources;
		pf::renderer::TemporalAAResources temporalAAResources;
		pf::renderer::VisibilityResources visibilityResources;
		pf::renderer::FSR2Resources fsr2Resources;
		pf::renderer::VXGIResources vxgiResources;
		pf::renderer::MeshBlendResources meshblendResources;

		pf::graphics::CommandList video_cmd;

		pf::graphics::Texture distortion_overlay; // optional full screen distortion from an asset

		mutable const pf::graphics::Texture* lastPostprocessRT = &rtPostprocess;
		// Post-processes are ping-ponged, this function helps to obtain the last postprocess render target that was written
		const pf::graphics::Texture* GetLastPostprocessRT() const
		{
			return lastPostprocessRT;
		}

		mutable float lightShaftsFadeFactor = 0.0f;

		virtual void RenderAO(pf::graphics::CommandList cmd) const;
		virtual void RenderSSR(pf::graphics::CommandList cmd) const;
		virtual void RenderSSGI(pf::graphics::CommandList cmd) const;
		virtual void RenderOutline(pf::graphics::CommandList cmd) const;
		virtual void RenderLightShafts(pf::graphics::CommandList cmd) const;
		virtual void RenderVolumetrics(pf::graphics::CommandList cmd) const;
		virtual void RenderSceneMIPChain(pf::graphics::CommandList cmd) const;
		virtual void RenderTransparents(pf::graphics::CommandList cmd) const;
		virtual void RenderPostprocessChain(pf::graphics::CommandList cmd) const;

		void DeleteGPUResources() override;
		void ResizeBuffers() override;

		pf::scene::CameraComponent* camera = &pf::scene::GetCamera();
		pf::scene::CameraComponent camera_previous;
		pf::scene::CameraComponent camera_reflection;
		pf::scene::CameraComponent camera_reflection_previous;

		pf::scene::Scene* scene = &pf::scene::GetScene();
		pf::renderer::Visibility visibility_main;
		pf::renderer::Visibility visibility_reflection;

		FrameCB frameCB = {};

		bool visibility_shading_in_compute = false;

		// Crop parameters in logical coordinates:
		float crop_left = 0;
		float crop_top = 0;
		float crop_right = 0;
		float crop_bottom = 0;
		pf::graphics::Rect GetScissorNativeResolution() const
		{
			pf::graphics::Rect scissor;
			scissor.left = int(LogicalToPhysical(crop_left));
			scissor.top = int(LogicalToPhysical(crop_top));
			scissor.right = int(GetPhysicalWidth() - LogicalToPhysical(crop_right));
			scissor.bottom = int(GetPhysicalHeight() - LogicalToPhysical(crop_bottom));
			return scissor;
		}
		pf::graphics::Rect GetScissorInternalResolution() const
		{
			pf::graphics::Rect scissor;
			scissor.left = int(LogicalToPhysical(crop_left) * resolutionScale);
			scissor.top = int(LogicalToPhysical(crop_top) * resolutionScale);
			scissor.right = int(GetInternalResolution().x - LogicalToPhysical(crop_right) * resolutionScale);
			scissor.bottom = int(GetInternalResolution().y - LogicalToPhysical(crop_bottom) * resolutionScale);
			return scissor;
		}

		const pf::graphics::Texture* GetDepthStencil() const override { return &depthBuffer_Main; }
		const pf::graphics::Texture* GetGUIBlurredBackground() const override { return &rtGUIBlurredBackground[2]; }

		constexpr float getExposure() const { return exposure; }
		constexpr float getHDRCalibration() const { return hdr_calibration; }
		constexpr float getBrightness() const { return brightness; }
		constexpr float getContrast() const { return contrast; }
		constexpr float getSaturation() const { return saturation; }
		constexpr float getBloomThreshold() const { return bloomThreshold; }
		constexpr float getMotionBlurStrength() const { return motionBlurStrength; }
		constexpr float getDepthOfFieldStrength() const { return dofStrength; }
		constexpr float getSharpenFilterAmount() const { return sharpenFilterAmount; }
		constexpr float getOutlineThreshold() const { return outlineThreshold; }
		constexpr float getOutlineThickness() const { return outlineThickness; }
		constexpr XMFLOAT4 getOutlineColor() const { return outlineColor; }
		constexpr float getAORange() const { return aoRange; }
		constexpr uint32_t getAOSampleCount() const { return aoSampleCount; }
		constexpr float getAOPower() const { return aoPower; }
		constexpr float getChromaticAberrationAmount() const { return chromaticAberrationAmount; }
		constexpr uint32_t getScreenSpaceShadowSampleCount() const { return screenSpaceShadowSampleCount; }
		constexpr float getScreenSpaceShadowRange() const { return screenSpaceShadowRange; }
		constexpr float getEyeAdaptionKey() const { return eyeadaptionKey; }
		constexpr float getEyeAdaptionRate() const { return eyeadaptionRate; }
		constexpr float getFSRSharpness() const { return fsrSharpness; }
		constexpr float getFSR2Sharpness() const { return fsr2Sharpness; }
		constexpr float getLightShaftsStrength() const { return lightShaftsStrength; }
		constexpr float getLightShaftsFadeSpeed() const { return lightShaftsFadeSpeed; }
		constexpr float getRaytracedDiffuseRange() const { return raytracedDiffuseRange; }
		constexpr float getRaytracedReflectionsRange() const { return raytracedReflectionsRange; }
		constexpr float getReflectionRoughnessCutoff() const { return reflectionRoughnessCutoff; }
		constexpr float getSSGIDepthRejection() const { return ssgiDepthRejection; }
		constexpr pf::renderer::Tonemap getTonemap() const { return tonemap; }

		constexpr bool getAOEnabled() const { return ao != AO_DISABLED; }
		constexpr AO getAO() const { return ao; }
		constexpr bool getSSREnabled() const { return ssrEnabled; }
		constexpr bool getSSGIEnabled() const { return ssgiEnabled; }
		constexpr bool getRaytracedDiffuseEnabled() const { return raytracedDiffuseEnabled; }
		constexpr bool getRaytracedReflectionEnabled() const { return raytracedReflectionsEnabled; }
		constexpr bool getShadowsEnabled() const { return shadowsEnabled; }
		constexpr bool getReflectionsEnabled() const { return reflectionsEnabled; }
		constexpr bool getFXAAEnabled() const { return fxaaEnabled; }
		constexpr bool getBloomEnabled() const { return bloomEnabled; }
		constexpr bool getColorGradingEnabled() const { return colorGradingEnabled; }
		constexpr bool getVolumeLightsEnabled() const { return volumeLightsEnabled; }
		constexpr bool getLightShaftsEnabled() const { return lightShaftsEnabled; }
		constexpr bool getLensFlareEnabled() const { return lensFlareEnabled; }
		constexpr bool getMotionBlurEnabled() const { return motionBlurEnabled; }
		constexpr bool getDepthOfFieldEnabled() const { return depthOfFieldEnabled; }
		constexpr bool getEyeAdaptionEnabled() const { return eyeAdaptionEnabled; }
		constexpr bool getSharpenFilterEnabled() const { return sharpenFilterEnabled && getSharpenFilterAmount() > 0; }
		constexpr bool getCRTFilterEnabled() const { return crtFilterEnabled && getSharpenFilterAmount() > 0; }
		constexpr bool getOutlineEnabled() const { return outlineEnabled; }
		constexpr bool getChromaticAberrationEnabled() const { return chromaticAberrationEnabled; }
		constexpr bool getDitherEnabled() const { return ditherEnabled; }
		constexpr bool getOcclusionCullingEnabled() const { return occlusionCullingEnabled; }
		constexpr bool getSceneUpdateEnabled() const { return sceneUpdateEnabled; }
		constexpr bool getFSREnabled() const { return fsrEnabled; }
		constexpr bool getFSR2Enabled() const { return fsr2Enabled; }
		constexpr bool getVisibilityComputeShadingEnabled() const { return visibility_shading_in_compute; }
		constexpr bool getMeshBlendEnabled() const { return mesh_blend; }

		constexpr void setExposure(float value) { exposure = value; }
		constexpr void setHDRCalibration(float value) { hdr_calibration = value; }
		constexpr void setBrightness(float value) { brightness = value; }
		constexpr void setContrast(float value) { contrast = value; }
		constexpr void setSaturation(float value) { saturation = value; }
		constexpr void setBloomThreshold(float value) { bloomThreshold = value; }
		constexpr void setMotionBlurStrength(float value) { motionBlurStrength = value; }
		constexpr void setDepthOfFieldStrength(float value) { dofStrength = value; }
		constexpr void setSharpenFilterAmount(float value) { sharpenFilterAmount = value; }
		constexpr void setOutlineThreshold(float value) { outlineThreshold = value; }
		constexpr void setOutlineThickness(float value) { outlineThickness = value; }
		constexpr void setOutlineColor(const XMFLOAT4& value) { outlineColor = value; }
		constexpr void setAORange(float value) { aoRange = value; }
		constexpr void setAOSampleCount(uint32_t value) { aoSampleCount = value; }
		constexpr void setAOPower(float value) { aoPower = value; }
		constexpr void setChromaticAberrationAmount(float value) { chromaticAberrationAmount = value; }
		constexpr void setScreenSpaceShadowSampleCount(uint32_t value) { screenSpaceShadowSampleCount = value; }
		constexpr void setScreenSpaceShadowRange(float value) { screenSpaceShadowRange = value; }
		constexpr void setEyeAdaptionKey(float value) { eyeadaptionKey = value; }
		constexpr void setEyeAdaptionRate(float value) { eyeadaptionRate = value; }
		constexpr void setFSRSharpness(float value) { fsrSharpness = value; }
		constexpr void setFSR2Sharpness(float value) { fsr2Sharpness = value; }
		constexpr void setLightShaftsStrength(float value) { lightShaftsStrength = value; }
		constexpr void setLightShaftsFadeSpeed(float value) { lightShaftsFadeSpeed = value; }
		constexpr void setRaytracedDiffuseRange(float value) { raytracedDiffuseRange = value; }
		constexpr void setRaytracedReflectionsRange(float value) { raytracedReflectionsRange = value; }
		constexpr void setReflectionRoughnessCutoff(float value) { reflectionRoughnessCutoff = value; }
		constexpr void setSSGIDepthRejection(float value) { ssgiDepthRejection = value; }
		constexpr void setTonemap(pf::renderer::Tonemap value) { tonemap = value; }
		constexpr void setVisibilityComputeShadingEnabled(bool value) { visibility_shading_in_compute = value; }

		void setAO(AO value);
		void setSSREnabled(bool value);
		void setSSGIEnabled(bool value);
		void setRaytracedReflectionsEnabled(bool value);
		void setRaytracedDiffuseEnabled(bool value);
		void setMotionBlurEnabled(bool value);
		void setDepthOfFieldEnabled(bool value);
		void setEyeAdaptionEnabled(bool value);
		void setReflectionsEnabled(bool value);
		void setBloomEnabled(bool value);
		void setVolumeLightsEnabled(bool value);
		void setLightShaftsEnabled(bool value);
		void setOutlineEnabled(bool value);
		constexpr void setShadowsEnabled(bool value) { shadowsEnabled = value; }
		constexpr void setFXAAEnabled(bool value) { fxaaEnabled = value; }
		constexpr void setColorGradingEnabled(bool value) { colorGradingEnabled = value; }
		constexpr void setLensFlareEnabled(bool value) { lensFlareEnabled = value; }
		constexpr void setSharpenFilterEnabled(bool value) { sharpenFilterEnabled = value; }
		constexpr void setCRTFilterEnabled(bool value) { crtFilterEnabled = value; }
		constexpr void setChromaticAberrationEnabled(bool value) { chromaticAberrationEnabled = value; }
		constexpr void setDitherEnabled(bool value) { ditherEnabled = value; }
		constexpr void setOcclusionCullingEnabled(bool value) { occlusionCullingEnabled = value; }
		constexpr void setSceneUpdateEnabled(bool value) { sceneUpdateEnabled = value; }
		constexpr void setMeshBlendEnabled(bool value) { mesh_blend = value; }
		void setFSREnabled(bool value);
		void setFSR2Enabled(bool value);
		void setFSR2Preset(FSR2_Preset preset); // this will modify resolution scaling and sampler lod bias

		struct CustomPostprocess
		{
			std::string name = "CustomPostprocess";
			pf::graphics::Shader computeshader;
			XMFLOAT4 params0;
			XMFLOAT4 params1;
			enum class Stage
			{
				BeforeTonemap, // Before tonemap and bloom in HDR color space
				AfterTonemap // After tonemap, in display color space
			} stage = Stage::AfterTonemap;
		};
		pf::vector<CustomPostprocess> custom_post_processes;

		void PreUpdate() override;
		void Update(float dt) override;
		void PreRender() override;
		void Render() const override;
		void Compose(pf::graphics::CommandList cmd) const override;

		void Stop() override;
		void Start() override;

		// Creates screenshot of the render result and replaces background (sky) pixels with transparency
		pf::graphics::Texture CreateScreenshotWithAlphaBackground();
	};

}
