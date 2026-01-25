#include "renderer.h"
#include "Engine/Shaders/ShaderCompiler.h"
#include "module.h"
#include "engine_core.h"
#include <algorithm>
#include <atomic>
#include <mutex>
#include "sheen_lut.h"
#include <Module/Util/spin_lock.h>

using namespace pf::graphics;
using namespace pf::enums;

namespace pf::renderer {

	GraphicsDevice*& device = GetDevice();
	Shader				shaders[SHADERTYPE_COUNT];
	Texture				textures[TEXTYPE_COUNT];
	InputLayout			inputLayouts[ILTYPE_COUNT];
	RasterizerState		rasterizers[RSTYPE_COUNT];
	DepthStencilState	depthStencils[DSSTYPE_COUNT];
	BlendState			blendStates[BSTYPE_COUNT];
	GPUBuffer			buffers[BUFFERTYPE_COUNT];
	Sampler				samplers[SAMPLER_COUNT];

#ifdef SHADERDUMP_ENABLED
	// Note: when using Shader Dump, use relative directory, because the dump will contain relative names too
	std::string SHADERPATH = "shaders/";
	std::string SHADERSOURCEPATH = "../WickedEngine/shaders/";
#else
	// Note: when NOT using Shader Dump, use absolute directory, to avoid the case when something (eg. file dialog) overrides working directory
	std::string SHADERPATH = pf::helper::GetCurrentPath() + "/shaders/";
	std::string SHADERSOURCEPATH = SHADER_INTEROP_PATH;
#endif // SHADERDUMP_ENABLED




	WIREFRAME_MODE wireframeMode = WIREFRAME_DISABLED;
	bool wireRender = false;
	bool debugBoneLines = false;
	bool debugPartitionTree = false;
	bool debugEmitters = false;
	bool freezeCullingCamera = false;
	bool debugEnvProbes = false;
	bool debugForceFields = false;
	bool debugCameras = false;
	bool debugColliders = false;
	bool debugSprings = false;
	bool gridHelper = false;
	bool advancedLightCulling = true;
	bool variableRateShadingClassification = false;
	bool variableRateShadingClassificationDebug = false;
	float GameSpeed = 1;
	bool debugLightCulling = false;
	bool occlusionCulling = true;
	bool temporalAA = false;
	bool temporalAADEBUG = false;
	uint32_t raytraceBounceCount = 8;
	bool raytraceDebugVisualizer = false;
	bool raytracedShadows = false;
	bool tessellationEnabled = true;
	bool disableAlbedoMaps = false;
	bool forceDiffuseLighting = false;
	bool SHADOWS_ENABLED = true;
	bool SCREENSPACESHADOWS = false;
	bool SURFELGI = false;
	//TO DO : SURFEL_DEBUG SURFELGI_DEBUG = SURFEL_DEBUG_NONE;
	bool DDGI_ENABLED = false;
	bool DDGI_DEBUG_ENABLED = false;
	uint32_t DDGI_RAYCOUNT = 256u;
	float DDGI_BLEND_SPEED = 0.1f;
	float GI_BOOST = 1.0f;
	bool MESH_SHADER_ALLOWED = false;
	bool MESHLET_OCCLUSION_CULLING = false;
	std::atomic<size_t> SHADER_ERRORS{ 0 };
	std::atomic<size_t> SHADER_MISSING{ 0 };
	bool VXGI_ENABLED = false;
	bool VXGI_REFLECTIONS_ENABLED = true;
	bool VXGI_DEBUG = false;
	int VXGI_DEBUG_CLIPMAP = 0;
	bool CAPSULE_SHADOW_ENABLED = false;
	float CAPSULE_SHADOW_ANGLE = XM_PIDIV4;
	float CAPSULE_SHADOW_FADE = 0.2f;
	bool SHADOW_LOD_OVERRIDE = true;

	Texture shadowMapAtlas;
	Texture shadowMapAtlas_Transparent;
	int max_shadow_resolution_2D = 1024;
	int max_shadow_resolution_cube = 256;


	GPUBuffer indirectDebugStatsReadback[GraphicsDevice::GetBufferCount()];



	enum SKYRENDERING
	{
		SKYRENDERING_STATIC,
		SKYRENDERING_DYNAMIC,
		SKYRENDERING_SUN,
		SKYRENDERING_ENVMAPCAPTURE_STATIC,
		SKYRENDERING_ENVMAPCAPTURE_DYNAMIC,
		SKYRENDERING_COUNT
	};

	PipelineState PSO_sky[SKYRENDERING_COUNT];

	enum DEBUGRENDERING
	{
		DEBUGRENDERING_ENVPROBE,
		DEBUGRENDERING_DDGI,
		DEBUGRENDERING_GRID,
		DEBUGRENDERING_CUBE,
		DEBUGRENDERING_CUBE_DEPTH,
		DEBUGRENDERING_LINES,
		DEBUGRENDERING_LINES_DEPTH,
		DEBUGRENDERING_TRIANGLE_SOLID,
		DEBUGRENDERING_TRIANGLE_WIREFRAME,
		DEBUGRENDERING_TRIANGLE_SOLID_DEPTH,
		DEBUGRENDERING_TRIANGLE_WIREFRAME_DEPTH,
		DEBUGRENDERING_EMITTER,
		DEBUGRENDERING_PAINTRADIUS,
		DEBUGRENDERING_VOXEL,
		DEBUGRENDERING_FORCEFIELD_POINT,
		DEBUGRENDERING_FORCEFIELD_PLANE,
		DEBUGRENDERING_RAYTRACE_BVH,
		DEBUGRENDERING_COUNT
	};
	PipelineState PSO_debug[DEBUGRENDERING_COUNT];



	SpinLock deferredMIPGenLock;
	vector<std::pair<Texture, bool>> deferredMIPGens;
	vector<std::pair<Texture, Texture>> deferredBCQueue;


	jobsystem::context mesh_shader_ctx;
	jobsystem::context object_pso_job_ctx;

	PipelineState PSO_object_wire;
	PipelineState PSO_object_wire_tessellation;
	PipelineState PSO_object_wire_mesh_shader;

	PipelineState PSO_occlusionquery;
	PipelineState PSO_impostor[RENDERPASS_COUNT];
	PipelineState PSO_impostor_wire;
	PipelineState PSO_captureimpostor;

	/*PipelineState PSO_lightvisualizer[LightComponent::LIGHTTYPE_COUNT];
	PipelineState PSO_volumetriclight[LightComponent::LIGHTTYPE_COUNT];*/

	PipelineState PSO_renderlightmap;
	PipelineState PSO_paintdecal;
	PipelineState PSO_lensflare;
	PipelineState PSO_downsampledepthbuffer;
	PipelineState PSO_upsample_bilateral;
	PipelineState PSO_volumetricclouds_upsample;
	PipelineState PSO_outline;
	PipelineState PSO_copyDepth;
	PipelineState PSO_copyStencilBit[8];
	PipelineState PSO_copyStencilBit_MSAA[8];
	PipelineState PSO_extractStencilBit[8];
	PipelineState PSO_waveeffect;
	PipelineState PSO_mesh_blend_resolve;

	jobsystem::context raytracing_ctx;
	jobsystem::context objectps_ctx;

	vector<CustomShader> customShaders;
	int RegisterCustomShader(const CustomShader& customShader)
	{
		static std::mutex locker;
		std::scoped_lock lck(locker);
		int result = (int)customShaders.size();
		customShaders.push_back(customShader);
		return result;
	}
	const vector<CustomShader>& GetCustomShaders()
	{
		return customShaders;
	}

	SHADERTYPE GetASTYPE(RENDERPASS renderPass, bool tessellation, bool alphatest, bool transparent, bool mesh_shader)
	{
		if (!mesh_shader)
			return SHADERTYPE_COUNT;

		return ASTYPE_OBJECT;
	}
	SHADERTYPE GetMSTYPE(RENDERPASS renderPass, bool tessellation, bool alphatest, bool transparent, bool mesh_shader)
	{
		if (!mesh_shader)
			return SHADERTYPE_COUNT;

		SHADERTYPE realMS = SHADERTYPE_COUNT;

		switch (renderPass)
		{
		case RENDERPASS_MAIN:
			realMS = MSTYPE_OBJECT;
			break;
		case RENDERPASS_PREPASS:
		case RENDERPASS_PREPASS_DEPTHONLY:
			if (alphatest)
			{
				realMS = MSTYPE_OBJECT_PREPASS_ALPHATEST;
			}
			else
			{
				realMS = MSTYPE_OBJECT_PREPASS;
			}
			break;
		case RENDERPASS_SHADOW:
			if (transparent)
			{
				realMS = MSTYPE_SHADOW_TRANSPARENT;
			}
			else
			{
				if (alphatest)
				{
					realMS = MSTYPE_SHADOW_ALPHATEST;
				}
				else
				{
					realMS = MSTYPE_SHADOW;
				}
			}
			break;
		case RENDERPASS_RAINBLOCKER:
			realMS = MSTYPE_SHADOW;
			break;
		default:
			break;
		}

		return realMS;
	}
	SHADERTYPE GetVSTYPE(RENDERPASS renderPass, bool tessellation, bool alphatest, bool transparent)
	{
		SHADERTYPE realVS = VSTYPE_OBJECT_SIMPLE;

		switch (renderPass)
		{
		case RENDERPASS_MAIN:
			if (tessellation)
			{
				realVS = VSTYPE_OBJECT_COMMON_TESSELLATION;
			}
			else
			{
				realVS = VSTYPE_OBJECT_COMMON;
			}
			break;
		case RENDERPASS_PREPASS:
		case RENDERPASS_PREPASS_DEPTHONLY:
			if (tessellation)
			{
				if (alphatest)
				{
					realVS = VSTYPE_OBJECT_PREPASS_ALPHATEST_TESSELLATION;
				}
				else
				{
					realVS = VSTYPE_OBJECT_PREPASS_TESSELLATION;
				}
			}
			else
			{
				if (alphatest)
				{
					realVS = VSTYPE_OBJECT_PREPASS_ALPHATEST;
				}
				else
				{
					realVS = VSTYPE_OBJECT_PREPASS;
				}
			}
			break;
		case RENDERPASS_ENVMAPCAPTURE:
			realVS = VSTYPE_ENVMAP;
			break;
		case RENDERPASS_SHADOW:
			if (transparent)
			{
				realVS = VSTYPE_SHADOW_TRANSPARENT;
			}
			else
			{
				if (alphatest)
				{
					realVS = VSTYPE_SHADOW_ALPHATEST;
				}
				else
				{
					realVS = VSTYPE_SHADOW;
				}
			}
			break;
		case RENDERPASS_VOXELIZE:
			realVS = VSTYPE_VOXELIZER;
			break;
		case RENDERPASS_RAINBLOCKER:
			realVS = VSTYPE_SHADOW;
			break;
		default:
			break;
		}

		return realVS;
	}
	SHADERTYPE GetGSTYPE(RENDERPASS renderPass, bool alphatest, bool transparent)
	{
		SHADERTYPE realGS = SHADERTYPE_COUNT;

		switch (renderPass)
		{
#ifdef VOXELIZATION_GEOMETRY_SHADER_ENABLED
		case RENDERPASS_VOXELIZE:
			realGS = GSTYPE_VOXELIZER;
			break;
#endif // VOXELIZATION_GEOMETRY_SHADER_ENABLED

		default:
			break;
		}

		return realGS;
	}
	SHADERTYPE GetHSTYPE(RENDERPASS renderPass, bool tessellation, bool alphatest)
	{
		if (tessellation)
		{
			switch (renderPass)
			{
			case RENDERPASS_PREPASS:
			case RENDERPASS_PREPASS_DEPTHONLY:
				if (alphatest)
				{
					return HSTYPE_OBJECT_PREPASS_ALPHATEST;
				}
				else
				{
					return HSTYPE_OBJECT_PREPASS;
				}
				break;
			case RENDERPASS_MAIN:
				return HSTYPE_OBJECT;
				break;
			default:
				break;
			}
		}

		return SHADERTYPE_COUNT;
	}
	SHADERTYPE GetDSTYPE(RENDERPASS renderPass, bool tessellation, bool alphatest)
	{
		if (tessellation)
		{
			switch (renderPass)
			{
			case RENDERPASS_PREPASS:
			case RENDERPASS_PREPASS_DEPTHONLY:
				if (alphatest)
				{
					return DSTYPE_OBJECT_PREPASS_ALPHATEST;
				}
				else
				{
					return DSTYPE_OBJECT_PREPASS;
				}
			case RENDERPASS_MAIN:
				return DSTYPE_OBJECT;
			default:
				break;
			}
		}

		return SHADERTYPE_COUNT;
	}
	/*SHADERTYPE GetPSTYPE(RENDERPASS renderPass, bool alphatest, bool transparent, MaterialComponent::SHADERTYPE shaderType)
	{
		SHADERTYPE realPS = SHADERTYPE_COUNT;

		switch (renderPass)
		{
		case RENDERPASS_MAIN:
			realPS = SHADERTYPE((transparent ? PSTYPE_OBJECT_TRANSPARENT_PERMUTATION_BEGIN : PSTYPE_OBJECT_PERMUTATION_BEGIN) + shaderType);
			break;
		case RENDERPASS_PREPASS:
			if (alphatest)
			{
				realPS = PSTYPE_OBJECT_PREPASS_ALPHATEST;
			}
			else
			{
				realPS = PSTYPE_OBJECT_PREPASS;
			}
			break;
		case RENDERPASS_PREPASS_DEPTHONLY:
			if (alphatest)
			{
				realPS = PSTYPE_OBJECT_PREPASS_DEPTHONLY_ALPHATEST;
			}
			break;
		case RENDERPASS_ENVMAPCAPTURE:
			realPS = PSTYPE_ENVMAP;
			break;
		case RENDERPASS_SHADOW:
			if (transparent)
			{
				realPS = shaderType == MaterialComponent::SHADERTYPE_WATER ? PSTYPE_SHADOW_WATER : PSTYPE_SHADOW_TRANSPARENT;
			}
			else
			{
				if (alphatest)
				{
					realPS = PSTYPE_SHADOW_ALPHATEST;
				}
				else
				{
					realPS = SHADERTYPE_COUNT;
				}
			}
			break;
		case RENDERPASS_VOXELIZE:
			realPS = PSTYPE_VOXELIZER;
			break;
		default:
			break;
		}

		return realPS;
	}*/



	void LoadShaders() {
		pf::jobsystem::Wait(raytracing_ctx);
		raytracing_ctx.priority = pf::jobsystem::Priority::Low;

		pf::jobsystem::Wait(objectps_ctx);
		objectps_ctx.priority = pf::jobsystem::Priority::Low;

		pf::jobsystem::context ctx;

		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) {
			LoadShader(ShaderStage::VS, shaders[VSTYPE_OBJECT_DEBUG], "objectVS_debug.cso");
			});

		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) {
			LoadShader(ShaderStage::VS, shaders[VSTYPE_OBJECT_COMMON], "objectVS_common.cso");
			});

		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) {
			LoadShader(ShaderStage::VS, shaders[VSTYPE_OBJECT_PREPASS], "objectVS_prepass.cso");
			});

		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) {
			LoadShader(ShaderStage::VS, shaders[VSTYPE_OBJECT_PREPASS_ALPHATEST], "objectVS_prepass_alphatest.cso");
			});

		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) {
			LoadShader(ShaderStage::VS, shaders[VSTYPE_OBJECT_SIMPLE], "objectVS_simple.cso");
			});

		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) {
			inputLayouts[ILTYPE_VERTEXCOLOR].elements =
			{
				{ "POSITION", 0, Format::R32G32B32A32_FLOAT, 0, InputLayout::APPEND_ALIGNED_ELEMENT, InputClassification::PER_VERTEX_DATA },
				{ "TEXCOORD", 0, Format::R32G32B32A32_FLOAT, 0, InputLayout::APPEND_ALIGNED_ELEMENT, InputClassification::PER_VERTEX_DATA },
			};
			LoadShader(ShaderStage::VS, shaders[VSTYPE_VERTEXCOLOR], "vertexcolorVS.cso");
			});

		inputLayouts[ILTYPE_POSITION].elements =
		{
			{ "POSITION", 0, Format::R32G32B32A32_FLOAT, 0, InputLayout::APPEND_ALIGNED_ELEMENT, InputClassification::PER_VERTEX_DATA },
		};

		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::VS, shaders[VSTYPE_OBJECT_COMMON_TESSELLATION], "objectVS_common_tessellation.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::VS, shaders[VSTYPE_OBJECT_PREPASS_TESSELLATION], "objectVS_prepass_tessellation.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::VS, shaders[VSTYPE_OBJECT_PREPASS_ALPHATEST_TESSELLATION], "objectVS_prepass_alphatest_tessellation.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::VS, shaders[VSTYPE_OBJECT_SIMPLE_TESSELLATION], "objectVS_simple_tessellation.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::VS, shaders[VSTYPE_IMPOSTOR], "impostorVS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::VS, shaders[VSTYPE_VOLUMETRICLIGHT_DIRECTIONAL], "volumetriclight_directionalVS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::VS, shaders[VSTYPE_VOLUMETRICLIGHT_POINT], "volumetriclight_pointVS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::VS, shaders[VSTYPE_VOLUMETRICLIGHT_SPOT], "volumetriclight_spotVS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::VS, shaders[VSTYPE_VOLUMETRICLIGHT_RECTANGLE], "volumetriclight_rectangleVS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::VS, shaders[VSTYPE_LIGHTVISUALIZER_SPOTLIGHT], "vSpotLightVS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::VS, shaders[VSTYPE_LIGHTVISUALIZER_POINTLIGHT], "vPointLightVS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::VS, shaders[VSTYPE_LIGHTVISUALIZER_RECTLIGHT], "vRectLightVS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::VS, shaders[VSTYPE_SPHERE], "sphereVS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::VS, shaders[VSTYPE_OCCLUDEE], "occludeeVS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::VS, shaders[VSTYPE_SKY], "skyVS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::VS, shaders[VSTYPE_VOXELIZER], "objectVS_voxelizer.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::VS, shaders[VSTYPE_VOXEL], "voxelVS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::VS, shaders[VSTYPE_FORCEFIELDVISUALIZER_POINT], "forceFieldPointVisualizerVS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::VS, shaders[VSTYPE_FORCEFIELDVISUALIZER_PLANE], "forceFieldPlaneVisualizerVS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::VS, shaders[VSTYPE_RAYTRACE_SCREEN], "raytrace_screenVS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::VS, shaders[VSTYPE_POSTPROCESS], "postprocessVS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::VS, shaders[VSTYPE_LENSFLARE], "lensFlareVS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::VS, shaders[VSTYPE_DDGI_DEBUG], "ddgi_debugVS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::VS, shaders[VSTYPE_SCREEN], "screenVS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::VS, shaders[VSTYPE_PAINTDECAL], "paintdecalVS.cso"); });

		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::VS, shaders[VSTYPE_ENVMAP], "envMapVS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::VS, shaders[VSTYPE_ENVMAP_SKY], "envMap_skyVS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::VS, shaders[VSTYPE_SHADOW], "shadowVS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::VS, shaders[VSTYPE_SHADOW_ALPHATEST], "shadowVS_alphatest.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::VS, shaders[VSTYPE_SHADOW_TRANSPARENT], "shadowVS_transparent.cso"); });

		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_IMPOSTOR], "impostorPS.cso"); });

		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_OBJECT_HOLOGRAM], "objectPS_hologram.cso"); });

		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_OBJECT_DEBUG], "objectPS_debug.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_OBJECT_PAINTRADIUS], "objectPS_paintradius.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_OBJECT_SIMPLE], "objectPS_simple.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_OBJECT_PREPASS], "objectPS_prepass.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_OBJECT_PREPASS_ALPHATEST], "objectPS_prepass_alphatest.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_OBJECT_PREPASS_DEPTHONLY_ALPHATEST], "objectPS_prepass_depthonly_alphatest.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_IMPOSTOR_PREPASS], "impostorPS_prepass.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_IMPOSTOR_PREPASS_DEPTHONLY], "impostorPS_prepass_depthonly.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_IMPOSTOR_SIMPLE], "impostorPS_simple.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_LIGHTVISUALIZER], "lightVisualizerPS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_LIGHTVISUALIZER_RECTLIGHT], "vRectLightPS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_VOLUMETRICLIGHT_DIRECTIONAL], "volumetricLight_DirectionalPS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_VOLUMETRICLIGHT_POINT], "volumetricLight_PointPS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_VOLUMETRICLIGHT_SPOT], "volumetricLight_SpotPS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_VOLUMETRICLIGHT_RECTANGLE], "volumetriclight_rectanglePS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_ENVMAP], "envMapPS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_ENVMAP_SKY_STATIC], "envMap_skyPS_static.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_ENVMAP_SKY_DYNAMIC], "envMap_skyPS_dynamic.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_CAPTUREIMPOSTOR], "captureImpostorPS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_CUBEMAP], "cubeMapPS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_VERTEXCOLOR], "vertexcolorPS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_SKY_STATIC], "skyPS_static.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_SKY_DYNAMIC], "skyPS_dynamic.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_SUN], "sunPS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_SHADOW_ALPHATEST], "shadowPS_alphatest.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_SHADOW_TRANSPARENT], "shadowPS_transparent.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_SHADOW_WATER], "shadowPS_water.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_VOXELIZER], "objectPS_voxelizer.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_VOXEL], "voxelPS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_FORCEFIELDVISUALIZER], "forceFieldVisualizerPS.cso"); });

		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_RAYTRACE_DEBUGBVH], "raytrace_debugbvhPS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_DOWNSAMPLEDEPTHBUFFER], "downsampleDepthBuffer4xPS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_POSTPROCESS_UPSAMPLE_BILATERAL], "upsample_bilateralPS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_POSTPROCESS_OUTLINE], "outlinePS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_LENSFLARE], "lensFlarePS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_DDGI_DEBUG], "ddgi_debugPS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_POSTPROCESS_VOLUMETRICCLOUDS_UPSAMPLE], "volumetricCloud_upsamplePS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_COPY_DEPTH], "copyDepthPS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_COPY_STENCIL_BIT], "copyStencilBitPS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_COPY_STENCIL_BIT_MSAA], "copyStencilBitPS.cso", ShaderModel::SM_6_0, { "MSAA" }); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_EXTRACT_STENCIL_BIT], "extractStencilBitPS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_PAINTDECAL], "paintdecalPS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_WAVE_EFFECT], "waveeffectPS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::PS, shaders[PSTYPE_POSTPROCESS_MESH_BLEND], "mesh_blendPS.cso"); });

		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::GS, shaders[GSTYPE_VOXELIZER], "objectGS_voxelizer.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::GS, shaders[GSTYPE_VOXEL], "voxelGS.cso"); });

		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_LUMINANCE_PASS1], "luminancePass1CS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_LUMINANCE_PASS2], "luminancePass2CS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_SHADINGRATECLASSIFICATION], "shadingRateClassificationCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_SHADINGRATECLASSIFICATION_DEBUG], "shadingRateClassificationCS_DEBUG.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_LIGHTCULLING], "lightCullingCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_LIGHTCULLING_DEBUG], "lightCullingCS_DEBUG.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_LIGHTCULLING_ADVANCED], "lightCullingCS_ADVANCED.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_LIGHTCULLING_ADVANCED_DEBUG], "lightCullingCS_ADVANCED_DEBUG.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_RESOLVEMSAADEPTHSTENCIL], "resolveMSAADepthStencilCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_VXGI_OFFSETPREV], "vxgi_offsetprevCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_VXGI_TEMPORAL], "vxgi_temporalCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_VXGI_SDF_JUMPFLOOD], "vxgi_sdf_jumpfloodCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_VXGI_RESOLVE_DIFFUSE], "vxgi_resolve_diffuseCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_VXGI_RESOLVE_SPECULAR], "vxgi_resolve_specularCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_SKYATMOSPHERE_TRANSMITTANCELUT], "skyAtmosphere_transmittanceLutCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_SKYATMOSPHERE_MULTISCATTEREDLUMINANCELUT], "skyAtmosphere_multiScatteredLuminanceLutCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_SKYATMOSPHERE_SKYVIEWLUT], "skyAtmosphere_skyViewLutCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_SKYATMOSPHERE_SKYLUMINANCELUT], "skyAtmosphere_skyLuminanceLutCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_SKYATMOSPHERE_CAMERAVOLUMELUT], "skyAtmosphere_cameraVolumeLutCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_GENERATEMIPCHAIN2D_FLOAT4], "generateMIPChain2DCS_float4.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_GENERATEMIPCHAIN3D_FLOAT4], "generateMIPChain3DCS_float4.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_GENERATEMIPCHAINCUBE_FLOAT4], "generateMIPChainCubeCS_float4.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_GENERATEMIPCHAINCUBEARRAY_FLOAT4], "generateMIPChainCubeArrayCS_float4.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_BLOCKCOMPRESS_BC1], "blockcompressCS_BC1.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_BLOCKCOMPRESS_BC3], "blockcompressCS_BC3.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_BLOCKCOMPRESS_BC4], "blockcompressCS_BC4.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_BLOCKCOMPRESS_BC5], "blockcompressCS_BC5.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_BLOCKCOMPRESS_BC6H], "blockcompressCS_BC6H.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_BLOCKCOMPRESS_BC6H_CUBEMAP], "blockcompressCS_BC6H_cubemap.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_FILTERENVMAP], "filterEnvMapCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_COPYTEXTURE2D_FLOAT4], "copytexture2D_float4CS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_COPYTEXTURE2D_FLOAT4_BORDEREXPAND], "copytexture2D_float4_borderexpandCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_SKINNING], "skinningCS.cso"); });

		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_PAINT_TEXTURE], "paint_textureCS.cso"); });

		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_BLUR_GAUSSIAN_FLOAT1], "blur_gaussian_float1CS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_BLUR_GAUSSIAN_FLOAT4], "blur_gaussian_float4CS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_BLUR_GAUSSIAN_WIDE_FLOAT1], "blur_gaussian_wide_float1CS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_BLUR_GAUSSIAN_WIDE_FLOAT4], "blur_gaussian_wide_float4CS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_BLUR_BILATERAL_FLOAT1], "blur_bilateral_float1CS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_BLUR_BILATERAL_FLOAT4], "blur_bilateral_float4CS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_BLUR_BILATERAL_WIDE_FLOAT1], "blur_bilateral_wide_float1CS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_BLUR_BILATERAL_WIDE_FLOAT4], "blur_bilateral_wide_float4CS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_SSAO], "ssaoCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_HBAO], "hbaoCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_MSAO_PREPAREDEPTHBUFFERS1], "msao_preparedepthbuffers1CS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_MSAO_PREPAREDEPTHBUFFERS2], "msao_preparedepthbuffers2CS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_MSAO_INTERLEAVE], "msao_interleaveCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_MSAO], "msaoCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_MSAO_BLURUPSAMPLE], "msao_blurupsampleCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_MSAO_BLURUPSAMPLE_BLENDOUT], "msao_blurupsampleCS_blendout.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_MSAO_BLURUPSAMPLE_PREMIN], "msao_blurupsampleCS_premin.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_MSAO_BLURUPSAMPLE_PREMIN_BLENDOUT], "msao_blurupsampleCS_premin_blendout.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_SSR_TILEMAXROUGHNESS_HORIZONTAL], "ssr_tileMaxRoughness_horizontalCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_SSR_TILEMAXROUGHNESS_VERTICAL], "ssr_tileMaxRoughness_verticalCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_SSR_DEPTHHIERARCHY], "ssr_depthHierarchyCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_SSR_RAYTRACE], "ssr_raytraceCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_SSR_RAYTRACE_EARLYEXIT], "ssr_raytraceCS_earlyexit.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_SSR_RAYTRACE_CHEAP], "ssr_raytraceCS_cheap.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_SSR_RESOLVE], "ssr_resolveCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_SSR_TEMPORAL], "ssr_temporalCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_SSR_UPSAMPLE], "ssr_upsampleCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_LIGHTSHAFTS], "lightShaftsCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_TILEMAXCOC_HORIZONTAL], "depthoffield_tileMaxCOC_horizontalCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_TILEMAXCOC_VERTICAL], "depthoffield_tileMaxCOC_verticalCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_NEIGHBORHOODMAXCOC], "depthoffield_neighborhoodMaxCOCCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_PREPASS], "depthoffield_prepassCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_PREPASS_EARLYEXIT], "depthoffield_prepassCS_earlyexit.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_MAIN], "depthoffield_mainCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_MAIN_EARLYEXIT], "depthoffield_mainCS_earlyexit.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_MAIN_CHEAP], "depthoffield_mainCS_cheap.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_POSTFILTER], "depthoffield_postfilterCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_DEPTHOFFIELD_UPSAMPLE], "depthoffield_upsampleCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_MOTIONBLUR_TILEMAXVELOCITY_HORIZONTAL], "motionblur_tileMaxVelocity_horizontalCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_MOTIONBLUR_TILEMAXVELOCITY_VERTICAL], "motionblur_tileMaxVelocity_verticalCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_MOTIONBLUR_NEIGHBORHOODMAXVELOCITY], "motionblur_neighborhoodMaxVelocityCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_MOTIONBLUR], "motionblurCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_MOTIONBLUR_EARLYEXIT], "motionblurCS_earlyexit.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_MOTIONBLUR_CHEAP], "motionblurCS_cheap.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_BLOOMSEPARATE], "bloomseparateCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_AERIALPERSPECTIVE], "aerialPerspectiveCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_AERIALPERSPECTIVE_CAPTURE], "aerialPerspectiveCS_capture.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_AERIALPERSPECTIVE_CAPTURE_MSAA], "aerialPerspectiveCS_capture_MSAA.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_VOLUMETRICCLOUDS_SHAPENOISE], "volumetricCloud_shapenoiseCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_VOLUMETRICCLOUDS_DETAILNOISE], "volumetricCloud_detailnoiseCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_VOLUMETRICCLOUDS_CURLNOISE], "volumetricCloud_curlnoiseCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_VOLUMETRICCLOUDS_WEATHERMAP], "volumetricCloud_weathermapCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_VOLUMETRICCLOUDS_RENDER], "volumetricCloud_renderCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_VOLUMETRICCLOUDS_RENDER_CAPTURE], "volumetricCloud_renderCS_capture.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_VOLUMETRICCLOUDS_RENDER_CAPTURE_MSAA], "volumetricCloud_renderCS_capture_MSAA.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_VOLUMETRICCLOUDS_REPROJECT], "volumetricCloud_reprojectCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_VOLUMETRICCLOUDS_SHADOW_RENDER], "volumetricCloud_shadow_renderCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_FXAA], "fxaaCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_TEMPORALAA], "temporalaaCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_SHARPEN], "sharpenCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_CRT], "crt_screenCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_TONEMAP], "tonemapCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_UNDERWATER], "underwaterCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_MESH_BLEND_PREPARE], "mesh_blend_prepareCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_MESH_BLEND_EXPAND], "mesh_blend_expandCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_FSR_UPSCALING], "fsr_upscalingCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_FSR_SHARPEN], "fsr_sharpenCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_FSR2_AUTOGEN_REACTIVE_PASS], "ffx-fsr2/ffx_fsr2_autogen_reactive_pass.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_FSR2_COMPUTE_LUMINANCE_PYRAMID_PASS], "ffx-fsr2/ffx_fsr2_compute_luminance_pyramid_pass.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_FSR2_PREPARE_INPUT_COLOR_PASS], "ffx-fsr2/ffx_fsr2_prepare_input_color_pass.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_FSR2_RECONSTRUCT_PREVIOUS_DEPTH_PASS], "ffx-fsr2/ffx_fsr2_reconstruct_previous_depth_pass.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_FSR2_DEPTH_CLIP_PASS], "ffx-fsr2/ffx_fsr2_depth_clip_pass.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_FSR2_LOCK_PASS], "ffx-fsr2/ffx_fsr2_lock_pass.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_FSR2_ACCUMULATE_PASS], "ffx-fsr2/ffx_fsr2_accumulate_pass.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_FSR2_RCAS_PASS], "ffx-fsr2/ffx_fsr2_rcas_pass.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_CHROMATIC_ABERRATION], "chromatic_aberrationCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_UPSAMPLE_BILATERAL_FLOAT1], "upsample_bilateral_float1CS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_UPSAMPLE_BILATERAL_FLOAT4], "upsample_bilateral_float4CS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_DOWNSAMPLE4X], "downsample4xCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_LINEARDEPTH], "lineardepthCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_NORMALSFROMDEPTH], "normalsfromdepthCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_SCREENSPACESHADOW], "screenspaceshadowCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_SSGI_DEINTERLEAVE], "ssgi_deinterleaveCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_SSGI], "ssgiCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_SSGI_WIDE], "ssgiCS.cso", pf::graphics::ShaderModel::SM_5_0, { "WIDE" }); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_SSGI_UPSAMPLE], "ssgi_upsampleCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_SSGI_UPSAMPLE_WIDE], "ssgi_upsampleCS.cso", pf::graphics::ShaderModel::SM_5_0, { "WIDE" }); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_RTDIFFUSE_SPATIAL], "rtdiffuse_spatialCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_RTDIFFUSE_TEMPORAL], "rtdiffuse_temporalCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_RTDIFFUSE_UPSAMPLE], "rtdiffuse_upsampleCS.cso"); });

		if (device->CheckCapability(GraphicsDeviceCapability::RAYTRACING))
		{
			pf::jobsystem::Execute(raytracing_ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_RTDIFFUSE], "rtdiffuseCS.cso", ShaderModel::SM_6_5); });

			pf::jobsystem::Execute(raytracing_ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_RTREFLECTION], "rtreflectionCS.cso", ShaderModel::SM_6_5); });

			pf::jobsystem::Execute(raytracing_ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_RTSHADOW], "rtshadowCS.cso", ShaderModel::SM_6_5); });
			pf::jobsystem::Execute(raytracing_ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_RTSHADOW_DENOISE_TILECLASSIFICATION], "rtshadow_denoise_tileclassificationCS.cso"); });
			pf::jobsystem::Execute(raytracing_ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_RTSHADOW_DENOISE_FILTER], "rtshadow_denoise_filterCS.cso"); });
			pf::jobsystem::Execute(raytracing_ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_RTSHADOW_DENOISE_TEMPORAL], "rtshadow_denoise_temporalCS.cso"); });

			pf::jobsystem::Execute(raytracing_ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_RTAO], "rtaoCS.cso", ShaderModel::SM_6_5); });
			pf::jobsystem::Execute(raytracing_ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_RTAO_DENOISE_TILECLASSIFICATION], "rtao_denoise_tileclassificationCS.cso"); });
			pf::jobsystem::Execute(raytracing_ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_RTAO_DENOISE_FILTER], "rtao_denoise_filterCS.cso"); });

		}

		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_POSTPROCESS_RTSHADOW_UPSAMPLE], "rtshadow_upsampleCS.cso"); });

		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_SURFEL_COVERAGE], "surfel_coverageCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_SURFEL_INDIRECTPREPARE], "surfel_indirectprepareCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_SURFEL_UPDATE], "surfel_updateCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_SURFEL_GRIDOFFSETS], "surfel_gridoffsetsCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_SURFEL_BINNING], "surfel_binningCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_SURFEL_INTEGRATE], "surfel_integrateCS.cso"); });
		if (device->CheckCapability(GraphicsDeviceCapability::RAYTRACING))
		{
			pf::jobsystem::Execute(raytracing_ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_SURFEL_RAYTRACE], "surfel_raytraceCS_rtapi.cso", ShaderModel::SM_6_5); });
		}
		else
		{
			pf::jobsystem::Execute(raytracing_ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_SURFEL_RAYTRACE], "surfel_raytraceCS.cso"); });
		}

		if (device->CheckCapability(GraphicsDeviceCapability::RAYTRACING))
		{
			pf::jobsystem::Execute(raytracing_ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_RAYTRACE], "raytraceCS_rtapi.cso", ShaderModel::SM_6_5); });
		}
		else
		{
			pf::jobsystem::Execute(raytracing_ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_RAYTRACE], "raytraceCS.cso"); });
		}

		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_VISIBILITY_RESOLVE], "visibility_resolveCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_VISIBILITY_RESOLVE_MSAA], "visibility_resolveCS_MSAA.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_VISIBILITY_SKY], "visibility_skyCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_VISIBILITY_VELOCITY], "visibility_velocityCS.cso"); });

		if (device->CheckCapability(GraphicsDeviceCapability::RAYTRACING))
		{
			pf::jobsystem::Execute(raytracing_ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_DDGI_RAYTRACE], "ddgi_raytraceCS_rtapi.cso", ShaderModel::SM_6_5); });
		}
		else
		{
			pf::jobsystem::Execute(raytracing_ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_DDGI_RAYTRACE], "ddgi_raytraceCS.cso"); });
		}
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_DDGI_RAYALLOCATION], "ddgi_rayallocationCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_DDGI_INDIRECTPREPARE], "ddgi_indirectprepareCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_DDGI_UPDATE], "ddgi_updateCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_DDGI_UPDATE_DEPTH], "ddgi_updateCS_depth.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_TERRAIN_VIRTUALTEXTURE_UPDATE_BASECOLORMAP], "terrainVirtualTextureUpdateCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_TERRAIN_VIRTUALTEXTURE_UPDATE_NORMALMAP], "terrainVirtualTextureUpdateCS_normalmap.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_TERRAIN_VIRTUALTEXTURE_UPDATE_SURFACEMAP], "terrainVirtualTextureUpdateCS_surfacemap.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_TERRAIN_VIRTUALTEXTURE_UPDATE_EMISSIVEMAP], "terrainVirtualTextureUpdateCS_emissivemap.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_MESHLET_PREPARE], "meshlet_prepareCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_IMPOSTOR_PREPARE], "impostor_prepareCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_VIRTUALTEXTURE_TILEREQUESTS], "virtualTextureTileRequestsCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_VIRTUALTEXTURE_TILEALLOCATE], "virtualTextureTileAllocateCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_VIRTUALTEXTURE_RESIDENCYUPDATE], "virtualTextureResidencyUpdateCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_WIND], "windCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_YUV_TO_RGB], "yuv_to_rgbCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_YUV_TO_RGB_ARRAY], "yuv_to_rgbCS.cso", ShaderModel::SM_6_0, { "ARRAY" }); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_WETMAP_UPDATE], "wetmap_updateCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_CAUSTICS], "causticsCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_DEPTH_REPROJECT], "depth_reprojectCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_DEPTH_PYRAMID], "depth_pyramidCS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::CS, shaders[CSTYPE_LIGHTMAP_EXPAND], "lightmap_expandCS.cso"); });

		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::HS, shaders[HSTYPE_OBJECT], "objectHS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::HS, shaders[HSTYPE_OBJECT_PREPASS], "objectHS_prepass.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::HS, shaders[HSTYPE_OBJECT_PREPASS_ALPHATEST], "objectHS_prepass_alphatest.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::HS, shaders[HSTYPE_OBJECT_SIMPLE], "objectHS_simple.cso"); });

		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::DS, shaders[DSTYPE_OBJECT], "objectDS.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::DS, shaders[DSTYPE_OBJECT_PREPASS], "objectDS_prepass.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::DS, shaders[DSTYPE_OBJECT_PREPASS_ALPHATEST], "objectDS_prepass_alphatest.cso"); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::DS, shaders[DSTYPE_OBJECT_SIMPLE], "objectDS_simple.cso"); });

		//pf::jobsystem::Dispatch(objectps_ctx, MaterialComponent::SHADERTYPE_COUNT, 1, [](pf::jobsystem::JobArgs args) {

		//	LoadShader(
		//		ShaderStage::PS,
		//		shaders[PSTYPE_OBJECT_PERMUTATION_BEGIN + args.jobIndex],
		//		"objectPS.cso",
		//		ShaderModel::SM_6_0,
		//		MaterialComponent::shaderTypeDefines[args.jobIndex] // permutation defines
		//	);

		//	});

		//pf::jobsystem::Dispatch(objectps_ctx, MaterialComponent::SHADERTYPE_COUNT, 1, [](pf::jobsystem::JobArgs args) {

		//	auto defines = MaterialComponent::shaderTypeDefines[args.jobIndex];
		//	defines.push_back("TRANSPARENT");
		//	LoadShader(
		//		ShaderStage::PS,
		//		shaders[PSTYPE_OBJECT_TRANSPARENT_PERMUTATION_BEGIN + args.jobIndex],
		//		"objectPS.cso",
		//		ShaderModel::SM_6_0,
		//		defines // permutation defines
		//	);

		//	});

		pf::jobsystem::Wait(ctx);

		if (device->CheckCapability(GraphicsDeviceCapability::MESH_SHADER))
		{
			// Note: Mesh shader loading is very slow in Vulkan, so all mesh shader loading will be executed on a separate context
			//	and only waited by mesh shader PSO jobs, not holding back the rest of initialization
			pf::jobsystem::Wait(mesh_shader_ctx);
			mesh_shader_ctx.priority = pf::jobsystem::Priority::Low;

			pf::jobsystem::Execute(mesh_shader_ctx, [](pf::jobsystem::JobArgs args) {
				LoadShader(ShaderStage::AS, shaders[ASTYPE_OBJECT], "objectAS.cso");
				LoadShader(ShaderStage::MS, shaders[MSTYPE_OBJECT_SIMPLE], "objectMS_simple.cso");

				PipelineStateDesc desc;
				desc.as = &shaders[ASTYPE_OBJECT];
				desc.ms = &shaders[MSTYPE_OBJECT_SIMPLE];
				desc.ps = &shaders[PSTYPE_OBJECT_SIMPLE]; // this is created in a different thread, so wait for the ctx before getting here
				desc.rs = &rasterizers[RSTYPE_WIRE];
				desc.bs = &blendStates[BSTYPE_OPAQUE];
				desc.dss = &depthStencils[DSSTYPE_DEFAULT];
				desc.pt = PrimitiveTopology::TRIANGLELIST;
				device->CreatePipelineState(&desc, &PSO_object_wire_mesh_shader);
				});

			pf::jobsystem::Execute(mesh_shader_ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::MS, shaders[MSTYPE_OBJECT], "objectMS.cso"); });
			pf::jobsystem::Execute(mesh_shader_ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::MS, shaders[MSTYPE_OBJECT_PREPASS], "objectMS_prepass.cso"); });
			pf::jobsystem::Execute(mesh_shader_ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::MS, shaders[MSTYPE_OBJECT_PREPASS_ALPHATEST], "objectMS_prepass_alphatest.cso"); });

			pf::jobsystem::Execute(mesh_shader_ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::MS, shaders[MSTYPE_SHADOW], "shadowMS.cso"); });
			pf::jobsystem::Execute(mesh_shader_ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::MS, shaders[MSTYPE_SHADOW_ALPHATEST], "shadowMS_alphatest.cso"); });
			pf::jobsystem::Execute(mesh_shader_ctx, [](pf::jobsystem::JobArgs args) { LoadShader(ShaderStage::MS, shaders[MSTYPE_SHADOW_TRANSPARENT], "shadowMS_transparent.cso"); });
		}


//		pf::jobsystem::Dispatch(ctx, MaterialComponent::SHADERTYPE_COUNT, 1, [](pf::jobsystem::JobArgs args) {
//
//			LoadShader(
//				ShaderStage::CS,
//				shaders[CSTYPE_VISIBILITY_SURFACE_PERMUTATION_BEGIN + args.jobIndex],
//				"visibility_surfaceCS.cso",
//				ShaderModel::SM_6_0,
//				MaterialComponent::shaderTypeDefines[args.jobIndex] // permutation defines
//			);
//
//			});
//
//		pf::jobsystem::Dispatch(ctx, MaterialComponent::SHADERTYPE_COUNT, 1, [](pf::jobsystem::JobArgs args) {
//
//			auto defines = MaterialComponent::shaderTypeDefines[args.jobIndex];
//			defines.push_back("REDUCED");
//			LoadShader(
//				ShaderStage::CS,
//				shaders[CSTYPE_VISIBILITY_SURFACE_REDUCED_PERMUTATION_BEGIN + args.jobIndex],
//				"visibility_surfaceCS.cso",
//				ShaderModel::SM_6_0,
//				defines // permutation defines
//			);
//
//			});
//
//		pf::jobsystem::Dispatch(ctx, MaterialComponent::SHADERTYPE_COUNT, 1, [](pf::jobsystem::JobArgs args) {
//
//			LoadShader(
//				ShaderStage::CS,
//				shaders[CSTYPE_VISIBILITY_SHADE_PERMUTATION_BEGIN + args.jobIndex],
//				"visibility_shadeCS.cso",
//				ShaderModel::SM_6_0,
//				MaterialComponent::shaderTypeDefines[args.jobIndex] // permutation defines
//			);
//
//			});

		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) {
			PipelineStateDesc desc;
			desc.vs = &shaders[VSTYPE_OBJECT_SIMPLE];
			desc.ps = &shaders[PSTYPE_OBJECT_SIMPLE];
			desc.rs = &rasterizers[RSTYPE_WIRE];
			desc.bs = &blendStates[BSTYPE_OPAQUE];
			desc.dss = &depthStencils[DSSTYPE_DEFAULT];

			device->CreatePipelineState(&desc, &PSO_object_wire);

			desc.pt = PrimitiveTopology::PATCHLIST;
			desc.vs = &shaders[VSTYPE_OBJECT_SIMPLE_TESSELLATION];
			desc.hs = &shaders[HSTYPE_OBJECT_SIMPLE];
			desc.ds = &shaders[DSTYPE_OBJECT_SIMPLE];
			device->CreatePipelineState(&desc, &PSO_object_wire_tessellation);
			});
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) {
			PipelineStateDesc desc;
			desc.vs = &shaders[VSTYPE_OCCLUDEE];
			desc.rs = &rasterizers[RSTYPE_OCCLUDEE];
			desc.bs = &blendStates[BSTYPE_COLORWRITEDISABLE];
			desc.dss = &depthStencils[DSSTYPE_DEPTHREAD];
			desc.pt = PrimitiveTopology::TRIANGLESTRIP;

			device->CreatePipelineState(&desc, &PSO_occlusionquery);
			});
		pf::jobsystem::Dispatch(ctx, RENDERPASS_COUNT, 1, [](pf::jobsystem::JobArgs args) {
			const bool impostorRequest =
				args.jobIndex != RENDERPASS_VOXELIZE &&
				args.jobIndex != RENDERPASS_SHADOW &&
				args.jobIndex != RENDERPASS_ENVMAPCAPTURE;
			if (!impostorRequest)
			{
				return;
			}

			PipelineStateDesc desc;
			desc.rs = &rasterizers[RSTYPE_DOUBLESIDED];
			desc.bs = &blendStates[BSTYPE_OPAQUE];
			desc.dss = &depthStencils[DSSTYPE_DEFAULT];
			desc.il = nullptr;

			switch (args.jobIndex)
			{
			case RENDERPASS_MAIN:
				desc.dss = &depthStencils[DSSTYPE_DEPTHREADEQUAL];
				desc.vs = &shaders[VSTYPE_IMPOSTOR];
				desc.ps = &shaders[PSTYPE_IMPOSTOR];
				break;
			case RENDERPASS_PREPASS:
				desc.vs = &shaders[VSTYPE_IMPOSTOR];
				desc.ps = &shaders[PSTYPE_IMPOSTOR_PREPASS];
				break;
			case RENDERPASS_PREPASS_DEPTHONLY:
				desc.vs = &shaders[VSTYPE_IMPOSTOR];
				desc.ps = &shaders[PSTYPE_IMPOSTOR_PREPASS_DEPTHONLY];
				break;
			default:
				desc.vs = &shaders[VSTYPE_IMPOSTOR];
				desc.ps = &shaders[PSTYPE_IMPOSTOR_PREPASS];
				break;
			}

			device->CreatePipelineState(&desc, &PSO_impostor[args.jobIndex]);
			});

		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) {
			PipelineStateDesc desc;
			desc.vs = &shaders[VSTYPE_IMPOSTOR];
			desc.ps = &shaders[PSTYPE_IMPOSTOR_SIMPLE];
			desc.rs = &rasterizers[RSTYPE_WIRE_DOUBLESIDED];
			desc.bs = &blendStates[BSTYPE_OPAQUE];
			desc.dss = &depthStencils[DSSTYPE_DEFAULT];
			desc.il = nullptr;

			device->CreatePipelineState(&desc, &PSO_impostor_wire);
			});
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) {
			PipelineStateDesc desc;
			desc.vs = &shaders[VSTYPE_OBJECT_COMMON];
			desc.rs = &rasterizers[RSTYPE_DOUBLESIDED];
			desc.bs = &blendStates[BSTYPE_OPAQUE];
			desc.dss = &depthStencils[DSSTYPE_CAPTUREIMPOSTOR];

			desc.ps = &shaders[PSTYPE_CAPTUREIMPOSTOR];
			device->CreatePipelineState(&desc, &PSO_captureimpostor);
			});

		//pf::jobsystem::Dispatch(ctx, LightComponent::LIGHTTYPE_COUNT, 1, [](pf::jobsystem::JobArgs args) {
		//	PipelineStateDesc desc;

		//	// deferred lights:

		//	desc.pt = PrimitiveTopology::TRIANGLELIST;


		//	// light visualizers:
		//	if (args.jobIndex != LightComponent::DIRECTIONAL)
		//	{

		//		desc.dss = &depthStencils[DSSTYPE_DEPTHREAD];
		//		desc.ps = &shaders[PSTYPE_LIGHTVISUALIZER];
		//		desc.pt = PrimitiveTopology::TRIANGLELIST;
		//		desc.il = nullptr;

		//		switch (args.jobIndex)
		//		{
		//		case LightComponent::POINT:
		//			desc.bs = &blendStates[BSTYPE_ADDITIVE];
		//			desc.vs = &shaders[VSTYPE_LIGHTVISUALIZER_POINTLIGHT];
		//			desc.rs = &rasterizers[RSTYPE_FRONT];
		//			desc.il = &inputLayouts[ILTYPE_POSITION];
		//			break;
		//		case LightComponent::SPOT:
		//			desc.bs = &blendStates[BSTYPE_ADDITIVE];
		//			desc.vs = &shaders[VSTYPE_LIGHTVISUALIZER_SPOTLIGHT];
		//			desc.rs = &rasterizers[RSTYPE_DOUBLESIDED];
		//			break;
		//		case LightComponent::RECTANGLE:
		//			desc.bs = &blendStates[BSTYPE_TRANSPARENT];
		//			desc.vs = &shaders[VSTYPE_LIGHTVISUALIZER_RECTLIGHT];
		//			desc.ps = &shaders[PSTYPE_LIGHTVISUALIZER_RECTLIGHT];
		//			desc.rs = &rasterizers[RSTYPE_FRONT];
		//			desc.pt = PrimitiveTopology::TRIANGLESTRIP;
		//			desc.dss = &depthStencils[DSSTYPE_DEFAULT];
		//			break;
		//		}

		//		device->CreatePipelineState(&desc, &PSO_lightvisualizer[args.jobIndex]);
		//	}


		//	// volumetric lights:
		//	if (args.jobIndex <= LightComponent::RECTANGLE)
		//	{
		//		desc.dss = &depthStencils[DSSTYPE_DEPTHDISABLED];
		//		desc.bs = &blendStates[BSTYPE_ADDITIVE];
		//		desc.rs = &rasterizers[RSTYPE_BACK];
		//		desc.pt = PrimitiveTopology::TRIANGLELIST;

		//		switch (args.jobIndex)
		//		{
		//		case LightComponent::DIRECTIONAL:
		//			desc.vs = &shaders[VSTYPE_VOLUMETRICLIGHT_DIRECTIONAL];
		//			desc.ps = &shaders[PSTYPE_VOLUMETRICLIGHT_DIRECTIONAL];
		//			break;
		//		case LightComponent::POINT:
		//			desc.vs = &shaders[VSTYPE_VOLUMETRICLIGHT_POINT];
		//			desc.ps = &shaders[PSTYPE_VOLUMETRICLIGHT_POINT];
		//			break;
		//		case LightComponent::SPOT:
		//			desc.vs = &shaders[VSTYPE_VOLUMETRICLIGHT_SPOT];
		//			desc.ps = &shaders[PSTYPE_VOLUMETRICLIGHT_SPOT];
		//			break;
		//		case LightComponent::RECTANGLE:
		//			desc.vs = &shaders[VSTYPE_VOLUMETRICLIGHT_RECTANGLE];
		//			desc.ps = &shaders[PSTYPE_VOLUMETRICLIGHT_RECTANGLE];
		//			desc.rs = &rasterizers[RSTYPE_FRONT];
		//			desc.pt = PrimitiveTopology::TRIANGLESTRIP;
		//			break;
		//		}

		//		device->CreatePipelineState(&desc, &PSO_volumetriclight[args.jobIndex]);
		//	}


		//	});
		pf::jobsystem::Execute(raytracing_ctx, [](pf::jobsystem::JobArgs args) {
			LoadShader(ShaderStage::VS, shaders[VSTYPE_RENDERLIGHTMAP], "renderlightmapVS.cso");
			if (device->CheckCapability(GraphicsDeviceCapability::RAYTRACING))
			{
				LoadShader(ShaderStage::PS, shaders[PSTYPE_RENDERLIGHTMAP], "renderlightmapPS_rtapi.cso", ShaderModel::SM_6_5);
			}
			else
			{
				LoadShader(ShaderStage::PS, shaders[PSTYPE_RENDERLIGHTMAP], "renderlightmapPS.cso");
			}
			PipelineStateDesc desc;
			desc.vs = &shaders[VSTYPE_RENDERLIGHTMAP];
			desc.ps = &shaders[PSTYPE_RENDERLIGHTMAP];
			desc.rs = &rasterizers[RSTYPE_LIGHTMAP];
			desc.bs = &blendStates[BSTYPE_TRANSPARENT];
			desc.dss = &depthStencils[DSSTYPE_DEFAULT]; // Note: depth is used to disallow overlapped pixel/primitive writes with conservative rasterization!

			device->CreatePipelineState(&desc, &PSO_renderlightmap);
			});
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) {
			PipelineStateDesc desc;
			desc.vs = &shaders[VSTYPE_PAINTDECAL];
			desc.ps = &shaders[PSTYPE_PAINTDECAL];
			desc.rs = &rasterizers[RSTYPE_DOUBLESIDED];
			desc.bs = &blendStates[BSTYPE_TRANSPARENT];
			desc.dss = &depthStencils[DSSTYPE_DEPTHDISABLED];

			RenderPassInfo renderpass_info;
			renderpass_info.rt_count = 1;
			renderpass_info.rt_formats[0] = Format::R8G8B8A8_UNORM;

			device->CreatePipelineState(&desc, &PSO_paintdecal, &renderpass_info);
			});
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) {
			PipelineStateDesc desc;
			desc.vs = &shaders[VSTYPE_POSTPROCESS];
			desc.ps = &shaders[PSTYPE_DOWNSAMPLEDEPTHBUFFER];
			desc.rs = &rasterizers[RSTYPE_DOUBLESIDED];
			desc.bs = &blendStates[BSTYPE_OPAQUE];
			desc.dss = &depthStencils[DSSTYPE_WRITEONLY];

			device->CreatePipelineState(&desc, &PSO_downsampledepthbuffer);
			});
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) {
			PipelineStateDesc desc;
			desc.vs = &shaders[VSTYPE_POSTPROCESS];
			desc.ps = &shaders[PSTYPE_POSTPROCESS_UPSAMPLE_BILATERAL];
			desc.rs = &rasterizers[RSTYPE_DOUBLESIDED];
			desc.bs = &blendStates[BSTYPE_PREMULTIPLIED];
			desc.dss = &depthStencils[DSSTYPE_DEPTHDISABLED];

			device->CreatePipelineState(&desc, &PSO_upsample_bilateral);
			});
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) {
			PipelineStateDesc desc;
			desc.vs = &shaders[VSTYPE_POSTPROCESS];
			desc.ps = &shaders[PSTYPE_POSTPROCESS_VOLUMETRICCLOUDS_UPSAMPLE];
			desc.rs = &rasterizers[RSTYPE_DOUBLESIDED];
			desc.bs = &blendStates[BSTYPE_PREMULTIPLIED];
			desc.dss = &depthStencils[DSSTYPE_DEPTHDISABLED];

			device->CreatePipelineState(&desc, &PSO_volumetricclouds_upsample);
			});
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) {
			PipelineStateDesc desc;
			desc.vs = &shaders[VSTYPE_POSTPROCESS];
			desc.ps = &shaders[PSTYPE_POSTPROCESS_OUTLINE];
			desc.rs = &rasterizers[RSTYPE_DOUBLESIDED];
			desc.bs = &blendStates[BSTYPE_TRANSPARENT];
			desc.dss = &depthStencils[DSSTYPE_DEPTHDISABLED];

			device->CreatePipelineState(&desc, &PSO_outline);
			});
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) {
			PipelineStateDesc desc;

			desc.vs = &shaders[VSTYPE_SCREEN];
			desc.ps = &shaders[PSTYPE_COPY_DEPTH];
			desc.bs = &blendStates[BSTYPE_OPAQUE];
			desc.rs = &rasterizers[RSTYPE_DOUBLESIDED];
			desc.dss = &depthStencils[DSSTYPE_WRITEONLY];
			device->CreatePipelineState(&desc, &PSO_copyDepth);

			desc.ps = &shaders[PSTYPE_COPY_STENCIL_BIT];
			for (int i = 0; i < 8; ++i)
			{
				desc.dss = &depthStencils[DSSTYPE_COPY_STENCIL_BIT_0 + i];
				device->CreatePipelineState(&desc, &PSO_copyStencilBit[i]);
			}

			desc.ps = &shaders[PSTYPE_COPY_STENCIL_BIT_MSAA];
			for (int i = 0; i < 8; ++i)
			{
				desc.dss = &depthStencils[DSSTYPE_COPY_STENCIL_BIT_0 + i];
				device->CreatePipelineState(&desc, &PSO_copyStencilBit_MSAA[i]);
			}

			desc.ps = &shaders[PSTYPE_EXTRACT_STENCIL_BIT];
			for (int i = 0; i < 8; ++i)
			{
				desc.dss = &depthStencils[DSSTYPE_EXTRACT_STENCIL_BIT_0 + i];
				device->CreatePipelineState(&desc, &PSO_extractStencilBit[i]);
			}
			});
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) {
			PipelineStateDesc desc;

			desc.vs = &shaders[VSTYPE_SCREEN];
			desc.ps = &shaders[PSTYPE_WAVE_EFFECT];
			desc.bs = &blendStates[BSTYPE_PREMULTIPLIED];
			desc.rs = &rasterizers[RSTYPE_DOUBLESIDED];
			desc.dss = &depthStencils[DSSTYPE_DEPTHDISABLED];
			device->CreatePipelineState(&desc, &PSO_waveeffect);
			});
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) {
			PipelineStateDesc desc;

			desc.vs = &shaders[VSTYPE_SCREEN];
			desc.ps = &shaders[PSTYPE_POSTPROCESS_MESH_BLEND];
			desc.bs = &blendStates[BSTYPE_TRANSPARENT];
			desc.rs = &rasterizers[RSTYPE_DOUBLESIDED];
			desc.dss = &depthStencils[DSSTYPE_DEPTHDISABLED];
			device->CreatePipelineState(&desc, &PSO_mesh_blend_resolve);
			});
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) {
			PipelineStateDesc desc;
			desc.vs = &shaders[VSTYPE_LENSFLARE];
			desc.ps = &shaders[PSTYPE_LENSFLARE];
			desc.bs = &blendStates[BSTYPE_ADDITIVE];
			desc.rs = &rasterizers[RSTYPE_DOUBLESIDED];
			desc.dss = &depthStencils[DSSTYPE_DEPTHDISABLED];
			desc.pt = PrimitiveTopology::TRIANGLESTRIP;

			device->CreatePipelineState(&desc, &PSO_lensflare);
			});
		pf::jobsystem::Dispatch(ctx, SKYRENDERING_COUNT, 1, [](pf::jobsystem::JobArgs args) {
			PipelineStateDesc desc;
			desc.rs = &rasterizers[RSTYPE_SKY];
			desc.dss = &depthStencils[DSSTYPE_DEPTHREAD];

			switch (args.jobIndex)
			{
			case SKYRENDERING_STATIC:
				desc.bs = &blendStates[BSTYPE_OPAQUE];
				desc.vs = &shaders[VSTYPE_SKY];
				desc.ps = &shaders[PSTYPE_SKY_STATIC];
				break;
			case SKYRENDERING_DYNAMIC:
				desc.bs = &blendStates[BSTYPE_OPAQUE];
				desc.vs = &shaders[VSTYPE_SKY];
				desc.ps = &shaders[PSTYPE_SKY_DYNAMIC];
				break;
			case SKYRENDERING_SUN:
				desc.bs = &blendStates[BSTYPE_ADDITIVE];
				desc.vs = &shaders[VSTYPE_SKY];
				desc.ps = &shaders[PSTYPE_SUN];
				break;
			case SKYRENDERING_ENVMAPCAPTURE_STATIC:
				desc.bs = &blendStates[BSTYPE_OPAQUE];
				desc.vs = &shaders[VSTYPE_ENVMAP_SKY];
				desc.ps = &shaders[PSTYPE_ENVMAP_SKY_STATIC];
				break;
			case SKYRENDERING_ENVMAPCAPTURE_DYNAMIC:
				desc.bs = &blendStates[BSTYPE_OPAQUE];
				desc.vs = &shaders[VSTYPE_ENVMAP_SKY];
				desc.ps = &shaders[PSTYPE_ENVMAP_SKY_DYNAMIC];
				break;
			}

			device->CreatePipelineState(&desc, &PSO_sky[args.jobIndex]);
			});
		pf::jobsystem::Dispatch(ctx, DEBUGRENDERING_COUNT, 1, [](pf::jobsystem::JobArgs args) {
			PipelineStateDesc desc;

			switch (args.jobIndex)
			{
			case DEBUGRENDERING_ENVPROBE:
				desc.vs = &shaders[VSTYPE_SPHERE];
				desc.ps = &shaders[PSTYPE_CUBEMAP];
				desc.dss = &depthStencils[DSSTYPE_DEFAULT];
				desc.rs = &rasterizers[RSTYPE_FRONT];
				desc.bs = &blendStates[BSTYPE_OPAQUE];
				desc.pt = PrimitiveTopology::TRIANGLELIST;
				break;
			case DEBUGRENDERING_DDGI:
				desc.vs = &shaders[VSTYPE_DDGI_DEBUG];
				desc.ps = &shaders[PSTYPE_DDGI_DEBUG];
				desc.dss = &depthStencils[DSSTYPE_DEFAULT];
				desc.rs = &rasterizers[RSTYPE_FRONT];
				desc.bs = &blendStates[BSTYPE_OPAQUE];
				desc.pt = PrimitiveTopology::TRIANGLELIST;
				break;
			case DEBUGRENDERING_GRID:
				desc.vs = &shaders[VSTYPE_VERTEXCOLOR];
				desc.ps = &shaders[PSTYPE_VERTEXCOLOR];
				desc.il = &inputLayouts[ILTYPE_VERTEXCOLOR];
				desc.dss = &depthStencils[DSSTYPE_DEPTHREAD];
				desc.rs = &rasterizers[RSTYPE_WIRE_DOUBLESIDED_SMOOTH];
				desc.bs = &blendStates[BSTYPE_TRANSPARENT];
				desc.pt = PrimitiveTopology::LINELIST;
				break;
			case DEBUGRENDERING_CUBE:
				desc.vs = &shaders[VSTYPE_VERTEXCOLOR];
				desc.ps = &shaders[PSTYPE_VERTEXCOLOR];
				desc.il = &inputLayouts[ILTYPE_VERTEXCOLOR];
				desc.dss = &depthStencils[DSSTYPE_DEPTHDISABLED];
				desc.rs = &rasterizers[RSTYPE_WIRE_DOUBLESIDED_SMOOTH];
				desc.bs = &blendStates[BSTYPE_TRANSPARENT];
				desc.pt = PrimitiveTopology::LINELIST;
				break;
			case DEBUGRENDERING_CUBE_DEPTH:
				desc.vs = &shaders[VSTYPE_VERTEXCOLOR];
				desc.ps = &shaders[PSTYPE_VERTEXCOLOR];
				desc.il = &inputLayouts[ILTYPE_VERTEXCOLOR];
				desc.dss = &depthStencils[DSSTYPE_DEPTHREAD];
				desc.rs = &rasterizers[RSTYPE_WIRE_DOUBLESIDED_SMOOTH];
				desc.bs = &blendStates[BSTYPE_TRANSPARENT];
				desc.pt = PrimitiveTopology::LINELIST;
				break;
			case DEBUGRENDERING_LINES:
				desc.vs = &shaders[VSTYPE_VERTEXCOLOR];
				desc.ps = &shaders[PSTYPE_VERTEXCOLOR];
				desc.il = &inputLayouts[ILTYPE_VERTEXCOLOR];
				desc.dss = &depthStencils[DSSTYPE_DEPTHDISABLED];
				desc.rs = &rasterizers[RSTYPE_WIRE_DOUBLESIDED_SMOOTH];
				desc.bs = &blendStates[BSTYPE_TRANSPARENT];
				desc.pt = PrimitiveTopology::LINELIST;
				break;
			case DEBUGRENDERING_LINES_DEPTH:
				desc.vs = &shaders[VSTYPE_VERTEXCOLOR];
				desc.ps = &shaders[PSTYPE_VERTEXCOLOR];
				desc.il = &inputLayouts[ILTYPE_VERTEXCOLOR];
				desc.dss = &depthStencils[DSSTYPE_DEPTHREAD];
				desc.rs = &rasterizers[RSTYPE_WIRE_DOUBLESIDED_SMOOTH];
				desc.bs = &blendStates[BSTYPE_TRANSPARENT];
				desc.pt = PrimitiveTopology::LINELIST;
				break;
			case DEBUGRENDERING_TRIANGLE_SOLID:
				desc.vs = &shaders[VSTYPE_VERTEXCOLOR];
				desc.ps = &shaders[PSTYPE_VERTEXCOLOR];
				desc.il = &inputLayouts[ILTYPE_VERTEXCOLOR];
				desc.dss = &depthStencils[DSSTYPE_DEPTHDISABLED];
				desc.rs = &rasterizers[RSTYPE_DOUBLESIDED];
				desc.bs = &blendStates[BSTYPE_TRANSPARENT];
				desc.pt = PrimitiveTopology::TRIANGLELIST;
				break;
			case DEBUGRENDERING_TRIANGLE_WIREFRAME:
				desc.vs = &shaders[VSTYPE_VERTEXCOLOR];
				desc.ps = &shaders[PSTYPE_VERTEXCOLOR];
				desc.il = &inputLayouts[ILTYPE_VERTEXCOLOR];
				desc.dss = &depthStencils[DSSTYPE_DEPTHDISABLED];
				desc.rs = &rasterizers[RSTYPE_WIRE_DOUBLESIDED_SMOOTH];
				desc.bs = &blendStates[BSTYPE_TRANSPARENT];
				desc.pt = PrimitiveTopology::TRIANGLELIST;
				break;
			case DEBUGRENDERING_TRIANGLE_SOLID_DEPTH:
				desc.vs = &shaders[VSTYPE_VERTEXCOLOR];
				desc.ps = &shaders[PSTYPE_VERTEXCOLOR];
				desc.il = &inputLayouts[ILTYPE_VERTEXCOLOR];
				desc.dss = &depthStencils[DSSTYPE_DEPTHREAD];
				desc.rs = &rasterizers[RSTYPE_DOUBLESIDED];
				desc.bs = &blendStates[BSTYPE_TRANSPARENT];
				desc.pt = PrimitiveTopology::TRIANGLELIST;
				break;
			case DEBUGRENDERING_TRIANGLE_WIREFRAME_DEPTH:
				desc.vs = &shaders[VSTYPE_VERTEXCOLOR];
				desc.ps = &shaders[PSTYPE_VERTEXCOLOR];
				desc.il = &inputLayouts[ILTYPE_VERTEXCOLOR];
				desc.dss = &depthStencils[DSSTYPE_DEPTHREAD];
				desc.rs = &rasterizers[RSTYPE_WIRE_DOUBLESIDED_SMOOTH];
				desc.bs = &blendStates[BSTYPE_TRANSPARENT];
				desc.pt = PrimitiveTopology::TRIANGLELIST;
				break;
			case DEBUGRENDERING_EMITTER:
				desc.vs = &shaders[VSTYPE_OBJECT_DEBUG];
				desc.ps = &shaders[PSTYPE_OBJECT_DEBUG];
				desc.dss = &depthStencils[DSSTYPE_DEPTHREAD];
				desc.rs = &rasterizers[RSTYPE_WIRE_DOUBLESIDED_SMOOTH];
				desc.bs = &blendStates[BSTYPE_OPAQUE];
				desc.pt = PrimitiveTopology::TRIANGLELIST;
				break;
			case DEBUGRENDERING_PAINTRADIUS:
				desc.vs = &shaders[VSTYPE_OBJECT_SIMPLE];
				desc.ps = &shaders[PSTYPE_OBJECT_PAINTRADIUS];
				desc.dss = &depthStencils[DSSTYPE_DEPTHREAD];
				desc.rs = &rasterizers[RSTYPE_FRONT];
				desc.bs = &blendStates[BSTYPE_TRANSPARENT];
				desc.pt = PrimitiveTopology::TRIANGLELIST;
				break;
			case DEBUGRENDERING_VOXEL:
				desc.vs = &shaders[VSTYPE_VOXEL];
				desc.ps = &shaders[PSTYPE_VOXEL];
				desc.gs = &shaders[GSTYPE_VOXEL];
				desc.dss = &depthStencils[DSSTYPE_DEFAULT];
				desc.rs = &rasterizers[RSTYPE_BACK];
				desc.bs = &blendStates[BSTYPE_OPAQUE];
				desc.pt = PrimitiveTopology::POINTLIST;
				break;
			case DEBUGRENDERING_FORCEFIELD_POINT:
				desc.vs = &shaders[VSTYPE_FORCEFIELDVISUALIZER_POINT];
				desc.ps = &shaders[PSTYPE_FORCEFIELDVISUALIZER];
				desc.dss = &depthStencils[DSSTYPE_DEPTHDISABLED];
				desc.rs = &rasterizers[RSTYPE_BACK];
				desc.bs = &blendStates[BSTYPE_ADDITIVE];
				desc.pt = PrimitiveTopology::TRIANGLELIST;
				break;
			case DEBUGRENDERING_FORCEFIELD_PLANE:
				desc.vs = &shaders[VSTYPE_FORCEFIELDVISUALIZER_PLANE];
				desc.ps = &shaders[PSTYPE_FORCEFIELDVISUALIZER];
				desc.dss = &depthStencils[DSSTYPE_DEPTHDISABLED];
				desc.rs = &rasterizers[RSTYPE_FRONT];
				desc.bs = &blendStates[BSTYPE_ADDITIVE];
				desc.pt = PrimitiveTopology::TRIANGLESTRIP;
				break;
			case DEBUGRENDERING_RAYTRACE_BVH:
				desc.vs = &shaders[VSTYPE_RAYTRACE_SCREEN];
				desc.ps = &shaders[PSTYPE_RAYTRACE_DEBUGBVH];
				desc.dss = &depthStencils[DSSTYPE_DEPTHDISABLED];
				desc.rs = &rasterizers[RSTYPE_DOUBLESIDED];
				desc.bs = &blendStates[BSTYPE_TRANSPARENT];
				desc.pt = PrimitiveTopology::TRIANGLELIST;
				break;
			}

			device->CreatePipelineState(&desc, &PSO_debug[args.jobIndex]);
			});
//
#ifdef RTREFLECTION_WITH_RAYTRACING_PIPELINE
		if (device->CheckCapability(GraphicsDeviceCapability::RAYTRACING))
		{
			pf::jobsystem::Execute(raytracing_ctx, [](pf::jobsystem::JobArgs args) {

				bool success = LoadShader(ShaderStage::LIB, shaders[RTTYPE_RTREFLECTION], "rtreflectionLIB.cso");
				assert(success);

				RaytracingPipelineStateDesc rtdesc;
				rtdesc.shader_libraries.emplace_back();
				rtdesc.shader_libraries.back().shader = &shaders[RTTYPE_RTREFLECTION];
				rtdesc.shader_libraries.back().function_name = "RTReflection_Raygen";
				rtdesc.shader_libraries.back().type = ShaderLibrary::Type::RAYGENERATION;

				rtdesc.shader_libraries.emplace_back();
				rtdesc.shader_libraries.back().shader = &shaders[RTTYPE_RTREFLECTION];
				rtdesc.shader_libraries.back().function_name = "RTReflection_ClosestHit";
				rtdesc.shader_libraries.back().type = ShaderLibrary::Type::CLOSESTHIT;

				rtdesc.shader_libraries.emplace_back();
				rtdesc.shader_libraries.back().shader = &shaders[RTTYPE_RTREFLECTION];
				rtdesc.shader_libraries.back().function_name = "RTReflection_AnyHit";
				rtdesc.shader_libraries.back().type = ShaderLibrary::Type::ANYHIT;

				rtdesc.shader_libraries.emplace_back();
				rtdesc.shader_libraries.back().shader = &shaders[RTTYPE_RTREFLECTION];
				rtdesc.shader_libraries.back().function_name = "RTReflection_Miss";
				rtdesc.shader_libraries.back().type = ShaderLibrary::Type::MISS;

				rtdesc.hit_groups.emplace_back();
				rtdesc.hit_groups.back().type = ShaderHitGroup::Type::GENERAL;
				rtdesc.hit_groups.back().name = "RTReflection_Raygen";
				rtdesc.hit_groups.back().general_shader = 0;

				rtdesc.hit_groups.emplace_back();
				rtdesc.hit_groups.back().type = ShaderHitGroup::Type::GENERAL;
				rtdesc.hit_groups.back().name = "RTReflection_Miss";
				rtdesc.hit_groups.back().general_shader = 3;

				rtdesc.hit_groups.emplace_back();
				rtdesc.hit_groups.back().type = ShaderHitGroup::Type::TRIANGLES;
				rtdesc.hit_groups.back().name = "RTReflection_Hitgroup";
				rtdesc.hit_groups.back().closest_hit_shader = 1;
				rtdesc.hit_groups.back().any_hit_shader = 2;

				rtdesc.max_trace_recursion_depth = 1;
				rtdesc.max_payload_size_in_bytes = sizeof(float4);
				rtdesc.max_attribute_size_in_bytes = sizeof(float2); // bary
				success = device->CreateRaytracingPipelineState(&rtdesc, &RTPSO_reflection);


				});
		};
#endif // RTREFLECTION_WITH_RAYTRACING_PIPELINE
//
		// Clear custom shaders (Custom shaders coming from user will need to be handled by the user in case of shader reload):
	//  	customShaders.clear();

		// Hologram sample shader will be registered as custom shader:
		//	It's best to register all custom shaders from the same thread, so under here
		//	or after engine has been completely initialized
		//	This is because RegisterCustomShader() will give out IDs in increasing order
		//	and you can keep the order stable by ensuring they are registered in order.
		{
			SHADERTYPE realVS = GetVSTYPE(RENDERPASS_MAIN, false, false, true);

			PipelineStateDesc desc;
			desc.vs = &shaders[realVS];
			desc.ps = &shaders[PSTYPE_OBJECT_HOLOGRAM];

			desc.bs = &blendStates[BSTYPE_ADDITIVE];
			desc.rs = &rasterizers[RSTYPE_FRONT];
			desc.dss = &depthStencils[DSSTYPE_HOLOGRAM];
			desc.pt = PrimitiveTopology::TRIANGLELIST;

			PipelineState pso;
			device->CreatePipelineState(&desc, &pso);

			CustomShader customShader;
			customShader.name = "Hologram";
			customShader.filterMask = FILTER_TRANSPARENT;
			customShader.pso[RENDERPASS_MAIN] = pso;
			RegisterCustomShader(customShader);
		}

		pf::jobsystem::Wait(ctx);

		// default objectshaders:
		//	We don't wait for these here, because then it can slow down the init time a lot
		//	We compile PipelineStates on backround threads, but we will add them to be useable by GetObjectPSO on the main thread (pf::eventhandler::EVENT_THREAD_SAFE_POINT)
		//	The RenderMeshes that uses these pipeline states will be checking the PipelineState.IsValid() and skip draws if the pipeline is not yet valid
		pf::jobsystem::Wait(object_pso_job_ctx);
		object_pso_job_ctx.priority = pf::jobsystem::Priority::Low;

		//for (uint32_t renderPass = 0; renderPass < RENDERPASS_COUNT; ++renderPass)
		//{
		//	for (uint32_t shaderType = 0; shaderType < MaterialComponent::SHADERTYPE_COUNT; ++shaderType)
		//	{
		//		for (uint32_t mesh_shader = 0; mesh_shader <= (device->CheckCapability(GraphicsDeviceCapability::MESH_SHADER) ? 1u : 0u); ++mesh_shader)
		//		{
		//			pf::jobsystem::Execute(object_pso_job_ctx, [=](pf::jobsystem::JobArgs args) {
		//				for (uint32_t blendMode = 0; blendMode < BLENDMODE_COUNT; ++blendMode)
		//				{
		//					for (uint32_t cullMode = 0; cullMode <= 3; ++cullMode)
		//					{
		//						for (uint32_t tessellation = 0; tessellation <= 1; ++tessellation)
		//						{
		//							if (tessellation && renderPass > RENDERPASS_PREPASS_DEPTHONLY)
		//								continue;
		//							for (uint32_t alphatest = 0; alphatest <= 1; ++alphatest)
		//							{
		//								const bool transparency = blendMode != BLENDMODE_OPAQUE;
		//								if ((renderPass == RENDERPASS_PREPASS || renderPass == RENDERPASS_PREPASS_DEPTHONLY) && transparency)
		//									continue;

		//								PipelineStateDesc desc;

		//								if (mesh_shader)
		//								{
		//									if (tessellation)
		//										continue;
		//									SHADERTYPE realAS = GetASTYPE((RENDERPASS)renderPass, tessellation, alphatest, transparency, mesh_shader);
		//									SHADERTYPE realMS = GetMSTYPE((RENDERPASS)renderPass, tessellation, alphatest, transparency, mesh_shader);
		//									if (realMS == SHADERTYPE_COUNT)
		//										continue;
		//									desc.as = realAS < SHADERTYPE_COUNT ? &shaders[realAS] : nullptr;
		//									desc.ms = realMS < SHADERTYPE_COUNT ? &shaders[realMS] : nullptr;
		//								}
		//								else
		//								{
		//									SHADERTYPE realVS = GetVSTYPE((RENDERPASS)renderPass, tessellation, alphatest, transparency);
		//									SHADERTYPE realHS = GetHSTYPE((RENDERPASS)renderPass, tessellation, alphatest);
		//									SHADERTYPE realDS = GetDSTYPE((RENDERPASS)renderPass, tessellation, alphatest);
		//									SHADERTYPE realGS = GetGSTYPE((RENDERPASS)renderPass, alphatest, transparency);

		//									if (tessellation && (realHS == SHADERTYPE_COUNT || realDS == SHADERTYPE_COUNT))
		//										continue;

		//									desc.vs = realVS < SHADERTYPE_COUNT ? &shaders[realVS] : nullptr;
		//									desc.hs = realHS < SHADERTYPE_COUNT ? &shaders[realHS] : nullptr;
		//									desc.ds = realDS < SHADERTYPE_COUNT ? &shaders[realDS] : nullptr;
		//									desc.gs = realGS < SHADERTYPE_COUNT ? &shaders[realGS] : nullptr;
		//								}

		//								SHADERTYPE realPS = GetPSTYPE((RENDERPASS)renderPass, alphatest, transparency, (MaterialComponent::SHADERTYPE)shaderType);
		//								desc.ps = realPS < SHADERTYPE_COUNT ? &shaders[realPS] : nullptr;

		//								switch (blendMode)
		//								{
		//								case BLENDMODE_OPAQUE:
		//									desc.bs = &blendStates[BSTYPE_OPAQUE];
		//									break;
		//								case BLENDMODE_ALPHA:
		//									desc.bs = &blendStates[BSTYPE_TRANSPARENT];
		//									break;
		//								case BLENDMODE_ADDITIVE:
		//									desc.bs = &blendStates[BSTYPE_ADDITIVE];
		//									break;
		//								case BLENDMODE_PREMULTIPLIED:
		//									desc.bs = &blendStates[BSTYPE_PREMULTIPLIED];
		//									break;
		//								case BLENDMODE_MULTIPLY:
		//									desc.bs = &blendStates[BSTYPE_MULTIPLY];
		//									break;
		//								case BLENDMODE_INVERSE:
		//									desc.bs = &blendStates[BSTYPE_INVERSE];
		//									break;
		//								default:
		//									assert(0);
		//									break;
		//								}

		//								switch (renderPass)
		//								{
		//								case RENDERPASS_SHADOW:
		//									desc.bs = &blendStates[transparency ? BSTYPE_TRANSPARENTSHADOW : BSTYPE_COLORWRITEDISABLE];
		//									break;
		//								case RENDERPASS_RAINBLOCKER:
		//									desc.bs = &blendStates[BSTYPE_COLORWRITEDISABLE];
		//									break;
		//								default:
		//									break;
		//								}

		//								switch (renderPass)
		//								{
		//								case RENDERPASS_SHADOW:
		//									desc.dss = &depthStencils[transparency ? DSSTYPE_DEPTHREAD : DSSTYPE_SHADOW];
		//									break;
		//								case RENDERPASS_MAIN:
		//									if (blendMode == BLENDMODE_ADDITIVE)
		//									{
		//										desc.dss = &depthStencils[DSSTYPE_DEPTHREAD];
		//									}
		//									else
		//									{
		//										desc.dss = &depthStencils[transparency ? DSSTYPE_TRANSPARENT : DSSTYPE_DEPTHREADEQUAL];
		//									}
		//									break;
		//								case RENDERPASS_ENVMAPCAPTURE:
		//									desc.dss = &depthStencils[DSSTYPE_ENVMAP];
		//									break;
		//								case RENDERPASS_VOXELIZE:
		//									desc.dss = &depthStencils[DSSTYPE_DEPTHDISABLED];
		//									break;
		//								case RENDERPASS_RAINBLOCKER:
		//									desc.dss = &depthStencils[DSSTYPE_DEFAULT];
		//									break;
		//								default:
		//									if (blendMode == BLENDMODE_ADDITIVE)
		//									{
		//										desc.dss = &depthStencils[DSSTYPE_DEPTHREAD];
		//									}
		//									else
		//									{
		//										desc.dss = &depthStencils[DSSTYPE_DEFAULT];
		//									}
		//									break;
		//								}

		//								switch (renderPass)
		//								{
		//								case RENDERPASS_SHADOW:
		//									desc.rs = &rasterizers[cullMode == (int)CullMode::NONE ? RSTYPE_SHADOW_DOUBLESIDED : RSTYPE_SHADOW];
		//									break;
		//								case RENDERPASS_VOXELIZE:
		//									desc.rs = &rasterizers[RSTYPE_VOXELIZE];
		//									break;
		//								default:
		//									switch ((CullMode)cullMode)
		//									{
		//									default:
		//									case CullMode::BACK:
		//										desc.rs = &rasterizers[RSTYPE_FRONT];
		//										break;
		//									case CullMode::NONE:
		//										desc.rs = &rasterizers[RSTYPE_DOUBLESIDED];
		//										break;
		//									case CullMode::FRONT:
		//										desc.rs = &rasterizers[RSTYPE_BACK];
		//										break;
		//									}
		//									break;
		//								}

		//								if (tessellation)
		//								{
		//									desc.pt = PrimitiveTopology::PATCHLIST;
		//								}
		//								else
		//								{
		//									desc.pt = PrimitiveTopology::TRIANGLELIST;
		//								}

		//								pf::jobsystem::Wait(objectps_ctx);
		//								if (mesh_shader)
		//								{
		//									pf::jobsystem::Wait(mesh_shader_ctx);
		//								}

		//								if (pf::jobsystem::IsShuttingDown())
		//									return;

		//								ObjectRenderingVariant variant = {};
		//								variant.bits.renderpass = renderPass;
		//								variant.bits.shadertype = shaderType;
		//								variant.bits.blendmode = blendMode;
		//								variant.bits.cullmode = cullMode;
		//								variant.bits.tessellation = tessellation;
		//								variant.bits.alphatest = alphatest;
		//								variant.bits.sample_count = 1;
		//								variant.bits.mesh_shader = mesh_shader;

		//								switch (renderPass)
		//								{
		//								case RENDERPASS_MAIN:
		//								case RENDERPASS_PREPASS:
		//								case RENDERPASS_PREPASS_DEPTHONLY:
		//								{
		//									RenderPassInfo renderpass_info;
		//									if (renderPass == RENDERPASS_PREPASS_DEPTHONLY)
		//									{
		//										renderpass_info.rt_count = 0;
		//										renderpass_info.rt_formats[0] = Format::UNKNOWN;
		//									}
		//									else
		//									{
		//										renderpass_info.rt_count = 1;
		//										renderpass_info.rt_formats[0] = renderPass == RENDERPASS_MAIN ? format_rendertarget_main : format_idbuffer;
		//									}
		//									renderpass_info.ds_format = format_depthbuffer_main;
		//									const uint32_t msaa_support[] = { 1,2,4,8 };
		//									for (uint32_t msaa : msaa_support)
		//									{
		//										variant.bits.sample_count = msaa;
		//										renderpass_info.sample_count = msaa;
		//										PipelineState pso;
		//										device->CreatePipelineState(&desc, &pso, &renderpass_info);
		//										pf::eventhandler::Subscribe_Once(pf::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
		//											*GetObjectPSO(variant) = pso;
		//											});
		//									}
		//								}
		//								break;

		//								case RENDERPASS_ENVMAPCAPTURE:
		//								{
		//									RenderPassInfo renderpass_info;
		//									renderpass_info.rt_count = 1;
		//									renderpass_info.rt_formats[0] = format_rendertarget_envprobe;
		//									renderpass_info.ds_format = format_depthbuffer_envprobe;
		//									const uint32_t msaa_support[] = { 1,8 };
		//									for (uint32_t msaa : msaa_support)
		//									{
		//										variant.bits.sample_count = msaa;
		//										renderpass_info.sample_count = msaa;
		//										PipelineState pso;
		//										device->CreatePipelineState(&desc, &pso, &renderpass_info);
		//										pf::eventhandler::Subscribe_Once(pf::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
		//											*GetObjectPSO(variant) = pso;
		//											});
		//									}
		//								}
		//								break;

		//								case RENDERPASS_SHADOW:
		//								{
		//									RenderPassInfo renderpass_info;
		//									renderpass_info.rt_count = 1;
		//									renderpass_info.rt_formats[0] = format_rendertarget_shadowmap;
		//									renderpass_info.ds_format = format_depthbuffer_shadowmap;
		//									PipelineState pso;
		//									device->CreatePipelineState(&desc, &pso, &renderpass_info);
		//									pf::eventhandler::Subscribe_Once(pf::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
		//										*GetObjectPSO(variant) = pso;
		//										});
		//								}
		//								break;

		//								case RENDERPASS_RAINBLOCKER:
		//								{
		//									RenderPassInfo renderpass_info;
		//									renderpass_info.rt_count = 0;
		//									renderpass_info.ds_format = format_depthbuffer_shadowmap;
		//									PipelineState pso;
		//									device->CreatePipelineState(&desc, &pso, &renderpass_info);
		//									pf::eventhandler::Subscribe_Once(pf::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
		//										*GetObjectPSO(variant) = pso;
		//										});
		//								}
		//								break;

		//								default:
		//									PipelineState pso;
		//									device->CreatePipelineState(&desc, &pso);
		//									pf::eventhandler::Subscribe_Once(pf::eventhandler::EVENT_THREAD_SAFE_POINT, [=](uint64_t userdata) {
		//										*GetObjectPSO(variant) = pso;
		//										});
		//									break;
		//								}
		//							}
		//						}
		//					}
		//				}
		//				});
		//		}
		//	}
		//}

	}

	void LoadBuffers() {
		GPUBufferDesc bd;
		bd.usage = Usage::DEFAULT;
		bd.size = sizeof(FrameCB);
		bd.bind_flags = BindFlag::CONSTANT_BUFFER;
		device->CreateBuffer(&bd, nullptr, &buffers[BUFFERTYPE_FRAMECB]);
		device->SetName(&buffers[BUFFERTYPE_FRAMECB], "buffers[BUFFERTYPE_FRAMECB]");

		bd.size = sizeof(IndirectDrawArgsInstanced) + (sizeof(XMFLOAT4) + sizeof(XMFLOAT4)) * 1000;
		bd.bind_flags = BindFlag::VERTEX_BUFFER | BindFlag::UNORDERED_ACCESS;
		bd.misc_flags = ResourceMiscFlag::BUFFER_RAW | ResourceMiscFlag::INDIRECT_ARGS;
		device->CreateBufferZeroed(&bd, &buffers[BUFFERTYPE_INDIRECT_DEBUG_0]);
		device->SetName(&buffers[BUFFERTYPE_INDIRECT_DEBUG_0], "buffers[BUFFERTYPE_INDIRECT_DEBUG_0]");
		device->CreateBufferZeroed(&bd, &buffers[BUFFERTYPE_INDIRECT_DEBUG_1]);
		device->SetName(&buffers[BUFFERTYPE_INDIRECT_DEBUG_1], "buffers[BUFFERTYPE_INDIRECT_DEBUG_1]");

		bd.size = sizeof(IndirectDrawArgsInstanced);
		bd.usage = Usage::READBACK;
		bd.bind_flags = {};
		bd.misc_flags = {};
		for (auto& buf : indirectDebugStatsReadback)
		{
			device->CreateBufferZeroed(&bd, &buf);
			device->SetName(&buf, "indirectDebugStatsReadback");
		}

		{
			TextureDesc desc;
			desc.bind_flags = BindFlag::SHADER_RESOURCE;
			desc.format = Format::R8_UNORM;
			desc.height = 16;
			desc.width = 16;
			SubresourceData InitData;
			InitData.data_ptr = sheenLUTdata;
			InitData.row_pitch = desc.width;
			device->CreateTexture(&desc, &InitData, &textures[TEXTYPE_2D_SHEENLUT]);
			device->SetName(&textures[TEXTYPE_2D_SHEENLUT], "textures[TEXTYPE_2D_SHEENLUT]");
		}

		{
			TextureDesc desc;
			desc.type = TextureDesc::Type::TEXTURE_3D;
			desc.format = Format::R16_FLOAT;
			desc.width = 32;
			desc.height = 32;
			desc.depth = 32;
			desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
			device->CreateTexture(&desc, nullptr, &textures[TEXTYPE_3D_WIND]);
			device->SetName(&textures[TEXTYPE_3D_WIND], "textures[TEXTYPE_3D_WIND]");
			device->CreateTexture(&desc, nullptr, &textures[TEXTYPE_3D_WIND_PREV]);
			device->SetName(&textures[TEXTYPE_3D_WIND_PREV], "textures[TEXTYPE_3D_WIND_PREV]");
		}
		{
			TextureDesc desc;
			desc.type = TextureDesc::Type::TEXTURE_2D;
			desc.format = Format::R8G8B8A8_UNORM;
			desc.width = 256;
			desc.height = 256;
			desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::UNORDERED_ACCESS;
			device->CreateTexture(&desc, nullptr, &textures[TEXTYPE_2D_CAUSTICS]);
			device->SetName(&textures[TEXTYPE_2D_CAUSTICS], "textures[TEXTYPE_2D_CAUSTICS]");
		}
	}


	void SetUpStates() {
		RasterizerState rs;
		rs.fill_mode = FillMode::SOLID;
		rs.cull_mode = CullMode::BACK;
		rs.front_counter_clockwise = true;
		rs.depth_bias = 0;
		rs.depth_bias_clamp = 0;
		rs.slope_scaled_depth_bias = 0;
		rs.depth_clip_enable = true;
		rs.multisample_enable = false;
		rs.antialiased_line_enable = false;
		rs.conservative_rasterization_enable = false;
		rasterizers[RSTYPE_FRONT] = rs;


		rs.fill_mode = FillMode::SOLID;
		rs.cull_mode = CullMode::BACK;
		rs.front_counter_clockwise = true;
		// Note: biases work slightly differently with unorm and float formats
		//	depth_bias				: needs to be tested when light is facing surface head-on (for example: directional light pointing down perpendicular to plane)
		//	slope_scaled_depth_bias	: needs to be tested when light gets more parallel to surface. This can cause holes in shadow maps at mismatching triangle orientations
		if (IsFormatUnorm(format_depthbuffer_shadowmap))
		{
			rs.depth_bias = -1;
			rs.slope_scaled_depth_bias = -4.0f;
		}
		else
		{
			rs.depth_bias = -10;
			rs.slope_scaled_depth_bias = -3.4f;
		}
		rs.depth_bias_clamp = 0;
		rs.depth_clip_enable = false;
		rs.multisample_enable = false;
		rs.antialiased_line_enable = false;
		rs.conservative_rasterization_enable = false;
		rasterizers[RSTYPE_SHADOW] = rs;
		rs.cull_mode = CullMode::NONE;
		rasterizers[RSTYPE_SHADOW_DOUBLESIDED] = rs;

		rs.fill_mode = FillMode::WIREFRAME;
		rs.cull_mode = CullMode::BACK;
		rs.front_counter_clockwise = true;
		rs.depth_bias = 0;
		rs.depth_bias_clamp = 0;
		rs.slope_scaled_depth_bias = 0;
		rs.depth_clip_enable = true;
		rs.multisample_enable = false;
		rs.antialiased_line_enable = false;
		rs.conservative_rasterization_enable = false;
		rasterizers[RSTYPE_WIRE] = rs;
		rs.antialiased_line_enable = true;
		rasterizers[RSTYPE_WIRE_SMOOTH] = rs;

		rs.fill_mode = FillMode::SOLID;
		rs.cull_mode = CullMode::NONE;
		rs.front_counter_clockwise = true;
		rs.depth_bias = 0;
		rs.depth_bias_clamp = 0;
		rs.slope_scaled_depth_bias = 0;
		rs.depth_clip_enable = true;
		rs.multisample_enable = false;
		rs.antialiased_line_enable = false;
		rs.conservative_rasterization_enable = false;
		rasterizers[RSTYPE_DOUBLESIDED] = rs;

		rs.fill_mode = FillMode::WIREFRAME;
		rs.cull_mode = CullMode::NONE;
		rs.front_counter_clockwise = true;
		rs.depth_bias = 0;
		rs.depth_bias_clamp = 0;
		rs.slope_scaled_depth_bias = 0;
		rs.depth_clip_enable = true;
		rs.multisample_enable = false;
		rs.antialiased_line_enable = false;
		rs.conservative_rasterization_enable = false;
		rasterizers[RSTYPE_WIRE_DOUBLESIDED] = rs;
		rs.antialiased_line_enable = true;
		rasterizers[RSTYPE_WIRE_DOUBLESIDED_SMOOTH] = rs;

		rs.fill_mode = FillMode::SOLID;
		rs.cull_mode = CullMode::FRONT;
		rs.front_counter_clockwise = true;
		rs.depth_bias = 0;
		rs.depth_bias_clamp = 0;
		rs.slope_scaled_depth_bias = 0;
		rs.depth_clip_enable = true;
		rs.multisample_enable = false;
		rs.antialiased_line_enable = false;
		rs.conservative_rasterization_enable = false;
		rasterizers[RSTYPE_BACK] = rs;

		rs.fill_mode = FillMode::SOLID;
		rs.cull_mode = CullMode::NONE;
		rs.front_counter_clockwise = true;
		rs.depth_bias = 0;
		rs.depth_bias_clamp = 0;
		rs.slope_scaled_depth_bias = 0;
		rs.depth_clip_enable = false;
		rs.multisample_enable = false;
		rs.antialiased_line_enable = false;
		rs.conservative_rasterization_enable = false;
		rasterizers[RSTYPE_OCCLUDEE] = rs;

		rs.fill_mode = FillMode::SOLID;
		rs.cull_mode = CullMode::FRONT;
		rs.front_counter_clockwise = true;
		rs.depth_bias = 0;
		rs.depth_bias_clamp = 0;
		rs.slope_scaled_depth_bias = 0;
		rs.depth_clip_enable = false;
		rs.multisample_enable = false;
		rs.antialiased_line_enable = false;
		rs.conservative_rasterization_enable = false;
		rasterizers[RSTYPE_SKY] = rs;

		rs.fill_mode = FillMode::SOLID;
		rs.cull_mode = CullMode::NONE;
		rs.front_counter_clockwise = true;
		rs.depth_bias = 0;
		rs.depth_bias_clamp = 0;
		rs.slope_scaled_depth_bias = 0;
		rs.depth_clip_enable = true;
		rs.multisample_enable = false;
		rs.antialiased_line_enable = false;
#ifdef VOXELIZATION_CONSERVATIVE_RASTERIZATION_ENABLED
		if (device->CheckCapability(GraphicsDeviceCapability::CONSERVATIVE_RASTERIZATION))
		{
			rs.conservative_rasterization_enable = true;
		}
		else
#endif // VOXELIZATION_CONSERVATIVE_RASTERIZATION_ENABLED
		{
			rs.forced_sample_count = 8;
		}
		rasterizers[RSTYPE_VOXELIZE] = rs;


		rs = rasterizers[RSTYPE_DOUBLESIDED];
		if (device->CheckCapability(GraphicsDeviceCapability::CONSERVATIVE_RASTERIZATION))
		{
			rs.conservative_rasterization_enable = true;
		}
		rasterizers[RSTYPE_LIGHTMAP] = rs;



		DepthStencilState dsd;
		dsd.depth_enable = true;
		dsd.depth_write_mask = DepthWriteMask::ALL;
		dsd.depth_func = ComparisonFunc::GREATER;

		dsd.stencil_enable = true;
		dsd.stencil_read_mask = 0;
		dsd.stencil_write_mask = 0xFF;
		dsd.front_face.stencil_func = ComparisonFunc::ALWAYS;
		dsd.front_face.stencil_pass_op = StencilOp::REPLACE;
		dsd.front_face.stencil_fail_op = StencilOp::KEEP;
		dsd.front_face.stencil_depth_fail_op = StencilOp::KEEP;
		dsd.back_face.stencil_func = ComparisonFunc::ALWAYS;
		dsd.back_face.stencil_pass_op = StencilOp::REPLACE;
		dsd.back_face.stencil_fail_op = StencilOp::KEEP;
		dsd.back_face.stencil_depth_fail_op = StencilOp::KEEP;
		depthStencils[DSSTYPE_DEFAULT] = dsd;

		dsd.depth_func = ComparisonFunc::GREATER_EQUAL;
		depthStencils[DSSTYPE_TRANSPARENT] = dsd;
		dsd.depth_func = ComparisonFunc::GREATER;

		dsd.depth_write_mask = DepthWriteMask::ZERO;
		depthStencils[DSSTYPE_HOLOGRAM] = dsd;

		dsd.depth_enable = true;
		dsd.depth_write_mask = DepthWriteMask::ALL;
		dsd.depth_func = ComparisonFunc::GREATER;
		dsd.stencil_enable = false;
		depthStencils[DSSTYPE_SHADOW] = dsd;

		dsd.depth_enable = true;
		dsd.depth_write_mask = DepthWriteMask::ALL;
		dsd.depth_func = ComparisonFunc::GREATER;
		dsd.stencil_enable = false;
		depthStencils[DSSTYPE_CAPTUREIMPOSTOR] = dsd;


		dsd.depth_enable = true;
		dsd.stencil_enable = false;
		dsd.depth_write_mask = DepthWriteMask::ZERO;
		dsd.depth_func = ComparisonFunc::GREATER_EQUAL;
		depthStencils[DSSTYPE_DEPTHREAD] = dsd;

		dsd.depth_enable = false;
		dsd.stencil_enable = false;
		depthStencils[DSSTYPE_DEPTHDISABLED] = dsd;


		dsd.depth_enable = true;
		dsd.depth_write_mask = DepthWriteMask::ZERO;
		dsd.depth_func = ComparisonFunc::EQUAL;
		depthStencils[DSSTYPE_DEPTHREADEQUAL] = dsd;


		dsd.depth_enable = true;
		dsd.depth_write_mask = DepthWriteMask::ALL;
		dsd.depth_func = ComparisonFunc::GREATER;
		depthStencils[DSSTYPE_ENVMAP] = dsd;

		dsd.depth_enable = true;
		dsd.depth_write_mask = DepthWriteMask::ALL;
		dsd.depth_func = ComparisonFunc::ALWAYS;
		dsd.stencil_enable = false;
		depthStencils[DSSTYPE_WRITEONLY] = dsd;

		dsd.depth_enable = false;
		dsd.depth_write_mask = DepthWriteMask::ZERO;
		dsd.stencil_enable = true;
		dsd.stencil_read_mask = 0;
		dsd.front_face.stencil_func = ComparisonFunc::ALWAYS;
		dsd.front_face.stencil_pass_op = StencilOp::REPLACE;
		dsd.back_face = dsd.front_face;
		for (int i = 0; i < 8; ++i)
		{
			dsd.stencil_write_mask = uint8_t(1 << i);
			depthStencils[DSSTYPE_COPY_STENCIL_BIT_0 + i] = dsd;
		}

		dsd.stencil_write_mask = 0;
		dsd.front_face.stencil_func = ComparisonFunc::EQUAL;
		dsd.front_face.stencil_pass_op = StencilOp::KEEP;
		dsd.back_face = dsd.front_face;
		for (int i = 0; i < 8; ++i)
		{
			dsd.stencil_read_mask = uint8_t(1 << i);
			depthStencils[DSSTYPE_EXTRACT_STENCIL_BIT_0 + i] = dsd;
		}


		BlendState bd;
		bd.render_target[0].blend_enable = false;
		bd.render_target[0].render_target_write_mask = ColorWrite::ENABLE_ALL;
		bd.alpha_to_coverage_enable = false;
		bd.independent_blend_enable = false;
		blendStates[BSTYPE_OPAQUE] = bd;

		bd.render_target[0].src_blend = Blend::SRC_ALPHA;
		bd.render_target[0].dest_blend = Blend::INV_SRC_ALPHA;
		bd.render_target[0].blend_op = BlendOp::ADD;
		bd.render_target[0].src_blend_alpha = Blend::ONE;
		bd.render_target[0].dest_blend_alpha = Blend::INV_SRC_ALPHA;
		bd.render_target[0].blend_op_alpha = BlendOp::ADD;
		bd.render_target[0].blend_enable = true;
		bd.render_target[0].render_target_write_mask = ColorWrite::ENABLE_ALL;
		bd.alpha_to_coverage_enable = false;
		bd.independent_blend_enable = false;
		blendStates[BSTYPE_TRANSPARENT] = bd;

		bd.render_target[0].blend_enable = true;
		bd.render_target[0].src_blend = Blend::ONE;
		bd.render_target[0].dest_blend = Blend::INV_SRC_ALPHA;
		bd.render_target[0].blend_op = BlendOp::ADD;
		bd.render_target[0].src_blend_alpha = Blend::ONE;
		bd.render_target[0].dest_blend_alpha = Blend::INV_SRC_ALPHA;
		bd.render_target[0].blend_op_alpha = BlendOp::ADD;
		bd.render_target[0].render_target_write_mask = ColorWrite::ENABLE_ALL;
		bd.independent_blend_enable = false;
		bd.alpha_to_coverage_enable = false;
		blendStates[BSTYPE_PREMULTIPLIED] = bd;


		bd.render_target[0].blend_enable = true;
		bd.render_target[0].src_blend = Blend::SRC_ALPHA;
		bd.render_target[0].dest_blend = Blend::ONE;
		bd.render_target[0].blend_op = BlendOp::ADD;
		bd.render_target[0].src_blend_alpha = Blend::ZERO;
		bd.render_target[0].dest_blend_alpha = Blend::ONE;
		bd.render_target[0].blend_op_alpha = BlendOp::ADD;
		bd.render_target[0].render_target_write_mask = ColorWrite::ENABLE_ALL;
		bd.independent_blend_enable = false,
			bd.alpha_to_coverage_enable = false;
		blendStates[BSTYPE_ADDITIVE] = bd;


		bd.render_target[0].blend_enable = false;
		bd.render_target[0].render_target_write_mask = ColorWrite::DISABLE;
		bd.independent_blend_enable = false,
			bd.alpha_to_coverage_enable = false;
		blendStates[BSTYPE_COLORWRITEDISABLE] = bd;

		bd.render_target[0].src_blend = Blend::DEST_COLOR;
		bd.render_target[0].dest_blend = Blend::ZERO;
		bd.render_target[0].blend_op = BlendOp::ADD;
		bd.render_target[0].src_blend_alpha = Blend::DEST_ALPHA;
		bd.render_target[0].dest_blend_alpha = Blend::ZERO;
		bd.render_target[0].blend_op_alpha = BlendOp::ADD;
		bd.render_target[0].blend_enable = true;
		bd.render_target[0].render_target_write_mask = ColorWrite::ENABLE_ALL;
		bd.alpha_to_coverage_enable = false;
		bd.independent_blend_enable = false;
		blendStates[BSTYPE_MULTIPLY] = bd;

		bd.render_target[0].src_blend = Blend::INV_DEST_COLOR;
		bd.render_target[0].dest_blend = Blend::ZERO;
		bd.render_target[0].blend_op = BlendOp::ADD;
		bd.render_target[0].src_blend_alpha = Blend::DEST_ALPHA;
		bd.render_target[0].dest_blend_alpha = Blend::ZERO;
		bd.render_target[0].blend_op_alpha = BlendOp::ADD;
		bd.render_target[0].blend_enable = true;
		bd.render_target[0].render_target_write_mask = ColorWrite::ENABLE_ALL;
		bd.alpha_to_coverage_enable = false;
		bd.independent_blend_enable = false;
		blendStates[BSTYPE_INVERSE] = bd;

		bd.render_target[0].src_blend = Blend::ZERO;
		bd.render_target[0].dest_blend = Blend::SRC_COLOR;
		bd.render_target[0].blend_op = BlendOp::ADD;
		bd.render_target[0].src_blend_alpha = Blend::ONE;
		bd.render_target[0].dest_blend_alpha = Blend::ONE;
		bd.render_target[0].blend_op_alpha = BlendOp::MAX;
		bd.render_target[0].blend_enable = true;
		bd.render_target[0].render_target_write_mask = ColorWrite::ENABLE_ALL;
		bd.alpha_to_coverage_enable = false;
		bd.independent_blend_enable = false;
		blendStates[BSTYPE_TRANSPARENTSHADOW] = bd;





		SamplerDesc samplerDesc;
		samplerDesc.filter = Filter::MIN_MAG_MIP_LINEAR;
		samplerDesc.address_u = TextureAddressMode::MIRROR;
		samplerDesc.address_v = TextureAddressMode::MIRROR;
		samplerDesc.address_w = TextureAddressMode::MIRROR;
		samplerDesc.mip_lod_bias = 0.0f;
		samplerDesc.max_anisotropy = 0;
		samplerDesc.comparison_func = ComparisonFunc::NEVER;
		samplerDesc.border_color = SamplerBorderColor::TRANSPARENT_BLACK;
		samplerDesc.min_lod = 0;
		samplerDesc.max_lod = std::numeric_limits<float>::max();
		device->CreateSampler(&samplerDesc, &samplers[SAMPLER_LINEAR_MIRROR]);

		samplerDesc.filter = Filter::MIN_MAG_MIP_LINEAR;
		samplerDesc.address_u = TextureAddressMode::CLAMP;
		samplerDesc.address_v = TextureAddressMode::CLAMP;
		samplerDesc.address_w = TextureAddressMode::CLAMP;
		device->CreateSampler(&samplerDesc, &samplers[SAMPLER_LINEAR_CLAMP]);

		samplerDesc.filter = Filter::MIN_MAG_MIP_LINEAR;
		samplerDesc.address_u = TextureAddressMode::WRAP;
		samplerDesc.address_v = TextureAddressMode::WRAP;
		samplerDesc.address_w = TextureAddressMode::WRAP;
		device->CreateSampler(&samplerDesc, &samplers[SAMPLER_LINEAR_WRAP]);

		samplerDesc.filter = Filter::MIN_MAG_MIP_POINT;
		samplerDesc.address_u = TextureAddressMode::MIRROR;
		samplerDesc.address_v = TextureAddressMode::MIRROR;
		samplerDesc.address_w = TextureAddressMode::MIRROR;
		device->CreateSampler(&samplerDesc, &samplers[SAMPLER_POINT_MIRROR]);

		samplerDesc.filter = Filter::MIN_MAG_MIP_POINT;
		samplerDesc.address_u = TextureAddressMode::WRAP;
		samplerDesc.address_v = TextureAddressMode::WRAP;
		samplerDesc.address_w = TextureAddressMode::WRAP;
		device->CreateSampler(&samplerDesc, &samplers[SAMPLER_POINT_WRAP]);


		samplerDesc.filter = Filter::MIN_MAG_MIP_POINT;
		samplerDesc.address_u = TextureAddressMode::CLAMP;
		samplerDesc.address_v = TextureAddressMode::CLAMP;
		samplerDesc.address_w = TextureAddressMode::CLAMP;
		device->CreateSampler(&samplerDesc, &samplers[SAMPLER_POINT_CLAMP]);

		samplerDesc.filter = Filter::ANISOTROPIC;
		samplerDesc.address_u = TextureAddressMode::CLAMP;
		samplerDesc.address_v = TextureAddressMode::CLAMP;
		samplerDesc.address_w = TextureAddressMode::CLAMP;
		samplerDesc.max_anisotropy = 16;
		device->CreateSampler(&samplerDesc, &samplers[SAMPLER_ANISO_CLAMP]);

		samplerDesc.filter = Filter::ANISOTROPIC;
		samplerDesc.address_u = TextureAddressMode::WRAP;
		samplerDesc.address_v = TextureAddressMode::WRAP;
		samplerDesc.address_w = TextureAddressMode::WRAP;
		samplerDesc.max_anisotropy = 16;
		device->CreateSampler(&samplerDesc, &samplers[SAMPLER_ANISO_WRAP]);

		samplerDesc.filter = Filter::ANISOTROPIC;
		samplerDesc.address_u = TextureAddressMode::MIRROR;
		samplerDesc.address_v = TextureAddressMode::MIRROR;
		samplerDesc.address_w = TextureAddressMode::MIRROR;
		samplerDesc.max_anisotropy = 16;
		device->CreateSampler(&samplerDesc, &samplers[SAMPLER_ANISO_MIRROR]);

		samplerDesc.filter = Filter::ANISOTROPIC;
		samplerDesc.address_u = TextureAddressMode::WRAP;
		samplerDesc.address_v = TextureAddressMode::WRAP;
		samplerDesc.address_w = TextureAddressMode::WRAP;
		samplerDesc.max_anisotropy = 16;
		device->CreateSampler(&samplerDesc, &samplers[SAMPLER_OBJECTSHADER]);

		samplerDesc.filter = Filter::ANISOTROPIC;
		samplerDesc.address_u = TextureAddressMode::CLAMP;
		samplerDesc.address_v = TextureAddressMode::CLAMP;
		samplerDesc.address_w = TextureAddressMode::CLAMP;
		samplerDesc.max_anisotropy = 16;
		device->CreateSampler(&samplerDesc, &samplers[SAMPLER_OBJECTSHADER_CLAMP]);

		samplerDesc.filter = Filter::COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
		samplerDesc.address_u = TextureAddressMode::CLAMP;
		samplerDesc.address_v = TextureAddressMode::CLAMP;
		samplerDesc.address_w = TextureAddressMode::CLAMP;
		samplerDesc.mip_lod_bias = 0.0f;
		samplerDesc.max_anisotropy = 0;
		samplerDesc.comparison_func = ComparisonFunc::GREATER_EQUAL;
		device->CreateSampler(&samplerDesc, &samplers[SAMPLER_CMP_DEPTH]);
	}


	void Initialize() {
		Timer timer;
		SetUpStates();
		LoadBuffers();
		LoadShaders();
		log("pf::renderer Initialized (%d ms)", (int)std::round(timer.elapsed()));
	}

	bool LoadShader(
		pf::graphics::ShaderStage stage,
		pf::graphics::Shader& shader,
		const std::string& filename,
		pf::graphics::ShaderModel minshadermodel,
		const vector<std::string>& permutation_defines 
	) {
		std::string shaderbinaryfilename = SHADERPATH + filename;

		if (!permutation_defines.empty())
		{
			std::string ext = helper::GetExtensionFromFileName(shaderbinaryfilename);
			shaderbinaryfilename = helper::RemoveExtension(shaderbinaryfilename);
			for (auto& def : permutation_defines)
			{
				shaderbinaryfilename += "_" + def;
			}
			shaderbinaryfilename += "." + ext;
		}
		if (device != nullptr)
		{
#ifdef SHADERDUMP_ENABLED
			// Loading shader from precompiled dump:
			auto it = wiShaderDump::shaderdump.find(shaderbinaryfilename);
			if (it != wiShaderDump::shaderdump.end())
			{
				pf::vector<uint8_t> decompressed;
				bool success = pf::helper::Decompress(it->second.data, it->second.size, decompressed);
				if (success)
				{
					return device->CreateShader(stage, decompressed.data(), decompressed.size(), &shader);
				}
				pf::backlog::post("shader dump decompression failure: " + shaderbinaryfilename, pf::backlog::LogLevel::Error);
			}
			else
			{
				pf::backlog::post("shader dump doesn't contain shader: " + shaderbinaryfilename, pf::backlog::LogLevel::Error);
			}
#endif // SHADERDUMP_ENABLED
		}
		
		shadercompiler::RegisterShader(shaderbinaryfilename);

		if (shadercompiler::IsShaderOutdated(shaderbinaryfilename)) {
			shadercompiler::CompilerInput input;
			input.format = device->GetShaderFormat();
			input.stage = stage;
			input.minshadermodel = minshadermodel;
			input.defines = permutation_defines;

			std::string sourcedir = SHADERSOURCEPATH;
			helper::MakePathAbsolute(sourcedir);
			input.include_directories.push_back(sourcedir);
			input.include_directories.push_back(sourcedir + helper::GetDirectoryFromPath(filename));
			input.shadersourcefilename = helper::ReplaceExtension(sourcedir + filename, "hlsl");
			shadercompiler::CompilerOutput output;
			shadercompiler::Compile(input, output);

			if (output.IsValid())
			{
				shadercompiler::SaveShaderAndMetadata(shaderbinaryfilename, output);

				if (!output.error_message.empty())
				{
					//backlog::post(output.error_message, pf::backlog::LogLevel::Warning);
				}
				//backlog::post("shader compiled: " + shaderbinaryfilename);
				return device->CreateShader(stage, output.shaderdata, output.shadersize, &shader);
			}
			else
			{
				//pf::backlog::post("shader compile FAILED: " + shaderbinaryfilename + "\n" + output.error_message, pf::backlog::LogLevel::Error);
				SHADER_ERRORS.fetch_add(1);
			}
		}

		if (device != nullptr)
		{
			vector<uint8_t> buffer;
			if (helper::FileRead(shaderbinaryfilename, buffer))
			{
				bool success = device->CreateShader(stage, buffer.data(), buffer.size(), &shader);
				if (success)
				{
					device->SetName(&shader, shaderbinaryfilename.c_str());
				}
				return success;
			}
			else
			{
				SHADER_MISSING.fetch_add(1);
			}
		}
		return false;
	}

	void AddDeferredMIPGen(const graphics::Texture& texture, bool preserve_coverage ) {
		deferredMIPGenLock.lock();
		deferredMIPGens.push_back(std::make_pair(texture, preserve_coverage));
		deferredMIPGenLock.unlock();
	}

	void AddDeferredBlockCompression(const graphics::Texture& texture_src, const graphics::Texture& texture_bc) {
		deferredMIPGenLock.lock();
		deferredBCQueue.push_back(std::make_pair(texture_src, texture_bc));
		deferredMIPGenLock.unlock();
	}

	const pf::graphics::Shader* GetShader(pf::enums::SHADERTYPE id) {
		return &shaders[id];
	}


	const Sampler* GetSampler(SAMPLERTYPES id)
	{
		return &samplers[id];
	}

	const InputLayout* GetInputLayout(ILTYPES id)
	{
		return &inputLayouts[id];
	}
	const RasterizerState* GetRasterizerState(RSTYPES id)
	{
		return &rasterizers[id];
	}
	const DepthStencilState* GetDepthStencilState(DSSTYPES id)
	{
		return &depthStencils[id];
	}
	const BlendState* GetBlendState(BSTYPES id)
	{
		return &blendStates[id];
	}
	const GPUBuffer* GetBuffer(BUFFERTYPES id)
	{
		return &buffers[id];
	}
	const Texture* GetTexture(TEXTYPES id)
	{
		return &textures[id];
	}







	void SetWireframeMode(WIREFRAME_MODE mode) {
		wireframeMode = mode;
		wireRender = (mode != WIREFRAME_DISABLED);
	}
	void SetWireRender(bool value) {wireRender = value;}
	bool IsWireRender() { return wireRender; }
	WIREFRAME_MODE GetWireframeMode() { return wireframeMode; }
	void SetToDrawDebugBoneLines(bool param) { debugBoneLines = param; }
	bool GetToDrawDebugBoneLines() { return debugBoneLines; }
	void SetToDrawDebugPartitionTree(bool param) { debugPartitionTree = param; }
	bool GetToDrawDebugPartitionTree() { return debugPartitionTree; }
	bool GetToDrawDebugEnvProbes() { return debugEnvProbes; }
	void SetToDrawDebugEnvProbes(bool value) { debugEnvProbes = value; }
	void SetToDrawDebugEmitters(bool param) { debugEmitters = param; }
	bool GetToDrawDebugEmitters() { return debugEmitters; }
	void SetToDrawDebugForceFields(bool param) { debugForceFields = param; }
	bool GetToDrawDebugForceFields() { return debugForceFields; }
	void SetToDrawDebugCameras(bool param) { debugCameras = param; }
	bool GetToDrawDebugCameras() { return debugCameras; }
	void SetToDrawDebugColliders(bool param) { debugColliders = param; }
	bool GetToDrawDebugColliders() { return debugColliders; }
	void SetToDrawDebugSprings(bool param) { debugSprings = param; }
	bool GetToDrawDebugSprings() { return debugSprings; }
	bool GetToDrawGridHelper() { return gridHelper; }
	void SetToDrawGridHelper(bool value) { gridHelper = value; }
	bool GetToDrawVoxelHelper() { return VXGI_DEBUG; }
	void SetToDrawVoxelHelper(bool value, int clipmap_level) { VXGI_DEBUG = value; VXGI_DEBUG_CLIPMAP = clipmap_level; }
	void SetDebugLightCulling(bool enabled) { debugLightCulling = enabled; }
	bool GetDebugLightCulling() { return debugLightCulling; }
	void SetAdvancedLightCulling(bool enabled) { advancedLightCulling = enabled; }
	bool GetAdvancedLightCulling() { return advancedLightCulling; }
	void SetVariableRateShadingClassification(bool enabled) { variableRateShadingClassification = enabled; }
	bool GetVariableRateShadingClassification() { return variableRateShadingClassification; }
	void SetVariableRateShadingClassificationDebug(bool enabled) { variableRateShadingClassificationDebug = enabled; }
	bool GetVariableRateShadingClassificationDebug() { return variableRateShadingClassificationDebug; }
	void SetOcclusionCullingEnabled(bool value)
	{
		occlusionCulling = value;
	}
	bool GetOcclusionCullingEnabled() { return occlusionCulling; }
	void SetTemporalAAEnabled(bool enabled) { temporalAA = enabled; }
	bool GetTemporalAAEnabled() { return temporalAA; }
	void SetTemporalAADebugEnabled(bool enabled) { temporalAADEBUG = enabled; }
	bool GetTemporalAADebugEnabled() { return temporalAADEBUG; }
	void SetFreezeCullingCameraEnabled(bool enabled) { freezeCullingCamera = enabled; }
	bool GetFreezeCullingCameraEnabled() { return freezeCullingCamera; }
	void SetVXGIEnabled(bool enabled)
	{
		VXGI_ENABLED = enabled;
	}
	bool GetVXGIEnabled() { return VXGI_ENABLED; }
	void SetVXGIReflectionsEnabled(bool enabled) { VXGI_REFLECTIONS_ENABLED = enabled; }
	bool GetVXGIReflectionsEnabled() { return VXGI_REFLECTIONS_ENABLED; }
	void SetGameSpeed(float value) { GameSpeed = std::max(0.0f, value); }
	float GetGameSpeed() { return GameSpeed; }
	void SetShadowsEnabled(bool value)
	{
		SHADOWS_ENABLED = value;
	}
	bool IsShadowsEnabled()
	{
		return SHADOWS_ENABLED;
	}
	void SetRaytraceBounceCount(uint32_t bounces)
	{
		raytraceBounceCount = bounces;
	}
	uint32_t GetRaytraceBounceCount()
	{
		return raytraceBounceCount;
	}
	void SetRaytraceDebugBVHVisualizerEnabled(bool value)
	{
		raytraceDebugVisualizer = value;
	}
	bool GetRaytraceDebugBVHVisualizerEnabled()
	{
		return raytraceDebugVisualizer;
	}
	void SetRaytracedShadowsEnabled(bool value)
	{
		raytracedShadows = value;
	}
	bool GetRaytracedShadowsEnabled()
	{
		return raytracedShadows;
	}
	void SetTessellationEnabled(bool value)
	{
		tessellationEnabled = value;
	}
	bool GetTessellationEnabled()
	{
		return tessellationEnabled;
	}
	void SetDisableAlbedoMaps(bool value)
	{
		disableAlbedoMaps = value;
	}
	bool IsDisableAlbedoMaps()
	{
		return disableAlbedoMaps;
	}
	void SetForceDiffuseLighting(bool value)
	{
		forceDiffuseLighting = value;
	}
	bool IsForceDiffuseLighting()
	{
		return forceDiffuseLighting;
	}
	void SetScreenSpaceShadowsEnabled(bool value)
	{
		SCREENSPACESHADOWS = value;
	}
	bool GetScreenSpaceShadowsEnabled()
	{
		return SCREENSPACESHADOWS;
	}
	void SetSurfelGIEnabled(bool value)
	{
		SURFELGI = value;
	}
	bool GetSurfelGIEnabled()
	{
		return SURFELGI;
	}
	//TO DO:
	//void SetSurfelGIDebugEnabled(SURFEL_DEBUG value)
	//{
	//	SURFELGI_DEBUG = value;
	//}
	//SURFEL_DEBUG GetSurfelGIDebugEnabled()
	//{
	//	return SURFELGI_DEBUG;
	//}
	void SetDDGIEnabled(bool value)
	{
		DDGI_ENABLED = value;
	}
	bool GetDDGIEnabled()
	{
		return DDGI_ENABLED;
	}
	void SetDDGIDebugEnabled(bool value)
	{
		DDGI_DEBUG_ENABLED = value;
	}
	bool GetDDGIDebugEnabled()
	{
		return DDGI_DEBUG_ENABLED;
	}
	void SetDDGIRayCount(uint32_t value)
	{
		DDGI_RAYCOUNT = value;
	}
	uint32_t GetDDGIRayCount()
	{
		return DDGI_RAYCOUNT;
	}
	void SetDDGIBlendSpeed(float value)
	{
		DDGI_BLEND_SPEED = value;
	}
	float GetDDGIBlendSpeed()
	{
		return DDGI_BLEND_SPEED;
	}
	void SetGIBoost(float value)
	{
		GI_BOOST = value;
	}
	float GetGIBoost()
	{
		return GI_BOOST;
	}
	void SetMeshShaderAllowed(bool value)
	{
		MESH_SHADER_ALLOWED = value;
	}
	bool IsMeshShaderAllowed()
	{
		return MESH_SHADER_ALLOWED && device->CheckCapability(GraphicsDeviceCapability::MESH_SHADER);
	}
	void SetMeshletOcclusionCullingEnabled(bool value)
	{
		MESHLET_OCCLUSION_CULLING = value;
	}
	bool IsMeshletOcclusionCullingEnabled()
	{
		return MESHLET_OCCLUSION_CULLING;
	}
	void SetCapsuleShadowEnabled(bool value)
	{
		CAPSULE_SHADOW_ENABLED = value;
	}
	bool IsCapsuleShadowEnabled()
	{
		return CAPSULE_SHADOW_ENABLED;
	}
	void SetCapsuleShadowAngle(float value)
	{
		CAPSULE_SHADOW_ANGLE = value;
	}
	float GetCapsuleShadowAngle()
	{
		return CAPSULE_SHADOW_ANGLE;
	}
	void SetCapsuleShadowFade(float value)
	{
		CAPSULE_SHADOW_FADE = value;
	}
	float GetCapsuleShadowFade()
	{
		return CAPSULE_SHADOW_FADE;
	}
	void SetShadowLODOverrideEnabled(bool value)
	{
		SHADOW_LOD_OVERRIDE = value;
	}
	bool IsShadowLODOverrideEnabled()
	{
		return SHADOW_LOD_OVERRIDE;
	}

	// This is responsible to manage big chunks of GPUBuffer, each of which will be used for suballocations:
	struct GPUSubAllocator
	{
		static constexpr uint64_t blocksize = 256ull * 1024ull * 1024ull; // 256 MB
		struct Block
		{
			allocator::PageAllocator allocator;
			GPUBuffer buffer;
		};
		vector<Block> blocks;
		std::mutex locker;
	} static suballocator;



	BufferSuballocation SuballocateGPUBuffer(uint64_t size)
	{
		if (size > GPUSubAllocator::blocksize / 2)
			return {}; // invalid, larger allocations than half block size will not be suballocated

		// scoped for locker
		{
			std::scoped_lock lock(suballocator.locker);

			// See if any of the large blocks can fulfill the allocation request:
			BufferSuballocation allocation;
			for (auto& block : suballocator.blocks)
			{
				allocation.allocation = block.allocator.allocate(size);
				if (allocation.allocation.IsValid())
				{
					allocation.alias = block.buffer;
					//wilog("SuballocateGPUBuffer allocated size: %s, pages: %d, free space remaining: %s", pf::helper::GetMemorySizeText(size).c_str(), block.allocator.page_count_from_bytes(size), pf::helper::GetMemorySizeText(allocation.allocation.allocator->allocator.storageReport().totalFreeSpace * block.allocator.page_size).c_str());
					return allocation;
				}
			}

			// Allocation couldn't be fulfilled, create new block:
			GPUBufferDesc desc;
			desc.size = GPUSubAllocator::blocksize;
			if (device->CheckCapability(GraphicsDeviceCapability::CACHE_COHERENT_UMA))
			{
				// In UMA mode, it is better to create UPLOAD buffer, this avoids one copy from UPLOAD to DEFAULT
				desc.usage = Usage::UPLOAD;
			}
			else
			{
				desc.usage = Usage::DEFAULT;
			}
			desc.bind_flags = BindFlag::SHADER_RESOURCE | BindFlag::VERTEX_BUFFER | BindFlag::INDEX_BUFFER;
			desc.misc_flags = ResourceMiscFlag::ALIASING_BUFFER | ResourceMiscFlag::NO_DEFAULT_DESCRIPTORS;
			desc.alignment = device->GetMinOffsetAlignment(&desc);
			auto& block = suballocator.blocks.emplace_back();
			bool success = device->CreateBuffer(&desc, nullptr, &block.buffer);
			assert(success);
			device->SetName(&block.buffer, "GPUSubAllocator");
			block.allocator.init(desc.size, (uint32_t)desc.alignment, true);
			//Logger("SuballocateGPUBuffer created buffer block with size: %s, with page size: %s, page count: %d", pf::helper::GetMemorySizeText(block.allocator.total_size_in_bytes()).c_str(), pf::helper::GetMemorySizeText(block.allocator.page_size).c_str(), (int)block.allocator.page_count);
		}
		return SuballocateGPUBuffer(size); // retry
	}
	void UpdateGPUSuballocator()
	{
		std::scoped_lock lock(suballocator.locker);
		for (auto& block : suballocator.blocks)
		{
			block.allocator.update_deferred_release(device->GetFrameCount(), device->GetBufferCount());
		}
		for (size_t i = 0; i < suballocator.blocks.size(); ++i)
		{
			if (suballocator.blocks[i].allocator.is_empty())
			{
				suballocator.blocks.erase(suballocator.blocks.begin() + i);
				break;
			}
		}
	}


	void BlockCompress(const Texture& texture_src, const Texture& texture_bc, CommandList cmd, uint32_t dst_slice_offset)
	{
		const uint32_t block_size = GetFormatBlockSize(texture_bc.desc.format);
		TextureDesc desc;
		desc.width = std::max(1u, (texture_bc.desc.width + block_size - 1) / block_size);
		desc.height = std::max(1u, (texture_bc.desc.height + block_size - 1) / block_size);
		desc.bind_flags = BindFlag::UNORDERED_ACCESS;
		desc.layout = ResourceState::UNORDERED_ACCESS;

		Texture bc_raw_dest;
		{
			// Find a raw block texture that will fit the request:
			static std::mutex locker;
			std::scoped_lock lock(locker);
			static Texture bc_raw_uint2;
			static Texture bc_raw_uint4;
			static Texture bc_raw_uint4_cubemap;
			Texture* bc_raw = nullptr;
			switch (texture_bc.desc.format)
			{
			case Format::BC1_UNORM:
			case Format::BC1_UNORM_SRGB:
				desc.format = Format::R32G32_UINT;
				bc_raw = &bc_raw_uint2;
				device->BindComputeShader(&shaders[CSTYPE_BLOCKCOMPRESS_BC1], cmd);
				device->EventBegin("BlockCompress - BC1", cmd);
				break;
			case Format::BC3_UNORM:
			case Format::BC3_UNORM_SRGB:
				desc.format = Format::R32G32B32A32_UINT;
				bc_raw = &bc_raw_uint4;
				device->BindComputeShader(&shaders[CSTYPE_BLOCKCOMPRESS_BC3], cmd);
				device->EventBegin("BlockCompress - BC3", cmd);
				break;
			case Format::BC4_UNORM:
				desc.format = Format::R32G32_UINT;
				bc_raw = &bc_raw_uint2;
				device->BindComputeShader(&shaders[CSTYPE_BLOCKCOMPRESS_BC4], cmd);
				device->EventBegin("BlockCompress - BC4", cmd);
				break;
			case Format::BC5_UNORM:
				desc.format = Format::R32G32B32A32_UINT;
				bc_raw = &bc_raw_uint4;
				device->BindComputeShader(&shaders[CSTYPE_BLOCKCOMPRESS_BC5], cmd);
				device->EventBegin("BlockCompress - BC5", cmd);
				break;
			case Format::BC6H_UF16:
				desc.format = Format::R32G32B32A32_UINT;
				if (has_flag(texture_src.desc.misc_flags, ResourceMiscFlag::TEXTURECUBE))
				{
					bc_raw = &bc_raw_uint4_cubemap;
					device->BindComputeShader(&shaders[CSTYPE_BLOCKCOMPRESS_BC6H_CUBEMAP], cmd);
					device->EventBegin("BlockCompress - BC6H - Cubemap", cmd);
					desc.array_size = texture_src.desc.array_size; // src array size not dst!!
				}
				else
				{
					bc_raw = &bc_raw_uint4;
					device->BindComputeShader(&shaders[CSTYPE_BLOCKCOMPRESS_BC6H], cmd);
					device->EventBegin("BlockCompress - BC6H", cmd);
				}
				break;
			default:
				assert(0); // not supported
				return;
			}

			if (!bc_raw->IsValid() || bc_raw->desc.width < desc.width || bc_raw->desc.height < desc.height || bc_raw->desc.array_size < desc.array_size)
			{
				TextureDesc bc_raw_desc = desc;
				bc_raw_desc.width = std::max(64u, bc_raw_desc.width);
				bc_raw_desc.height = std::max(64u, bc_raw_desc.height);
				bc_raw_desc.width = std::max(bc_raw->desc.width, bc_raw_desc.width);
				bc_raw_desc.height = std::max(bc_raw->desc.height, bc_raw_desc.height);
				bc_raw_desc.width = pf::math::GetNextPowerOfTwo(bc_raw_desc.width);
				bc_raw_desc.height = pf::math::GetNextPowerOfTwo(bc_raw_desc.height);
				device->CreateTexture(&bc_raw_desc, nullptr, bc_raw);
				device->SetName(bc_raw, "bc_raw");

				device->ClearUAV(bc_raw, 0, cmd);
				device->Barrier(GPUBarrier::Memory(bc_raw), cmd);

				std::string info;
				info += "BlockCompress created a new raw block texture to fit request: " + std::string(GetFormatString(texture_bc.desc.format)) + " (" + std::to_string(texture_bc.desc.width) + ", " + std::to_string(texture_bc.desc.height) + ")";
				info += "\n\tFormat = ";
				info += GetFormatString(bc_raw_desc.format);
				info += "\n\tResolution = " + std::to_string(bc_raw_desc.width) + " * " + std::to_string(bc_raw_desc.height);
				info += "\n\tArray Size = " + std::to_string(bc_raw_desc.array_size);
				size_t total_size = 0;
				total_size += ComputeTextureMemorySizeInBytes(bc_raw_desc);
				info += "\n\tMemory = " + pf::helper::GetMemorySizeText(total_size) + "\n";
				pf::backlogger::postin(info);
			}

			bc_raw_dest = *bc_raw;
		}

		for (uint32_t mip = 0; mip < texture_bc.desc.mip_levels; ++mip)
		{
			const uint32_t src_width = std::max(1u, texture_bc.desc.width >> mip);
			const uint32_t src_height = std::max(1u, texture_bc.desc.height >> mip);
			const uint32_t dst_width = (src_width + block_size - 1) / block_size;
			const uint32_t dst_height = (src_height + block_size - 1) / block_size;
			device->BindResource(&texture_src, 0, cmd, texture_src.desc.mip_levels == 1 ? -1 : mip);
			device->BindUAV(&bc_raw_dest, 0, cmd);
			device->Dispatch((dst_width + 7u) / 8u, (dst_height + 7u) / 8u, desc.array_size, cmd);

			GPUBarrier barriers[] = {
				GPUBarrier::Image(&bc_raw_dest, ResourceState::UNORDERED_ACCESS, ResourceState::COPY_SRC),
				GPUBarrier::Image(&texture_bc, texture_bc.desc.layout, ResourceState::COPY_DST),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);

			for (uint32_t slice = 0; slice < desc.array_size; ++slice)
			{
				Box box;
				box.left = 0;
				box.right = dst_width;
				box.top = 0;
				box.bottom = dst_height;
				box.front = 0;
				box.back = 1;

				device->CopyTexture(
					&texture_bc, 0, 0, 0, mip, dst_slice_offset + slice,
					&bc_raw_dest, 0, slice,
					cmd,
					&box
				);
			}

			for (int i = 0; i < arraysize(barriers); ++i)
			{
				std::swap(barriers[i].image.layout_before, barriers[i].image.layout_after);
			}
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->EventEnd(cmd);
	}

}