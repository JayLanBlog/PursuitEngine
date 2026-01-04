#include "shader_compare_test.h"

#include "Engine/Device/graphic.h"
#include "Module/Container/pvector.h"
#include "Module/Container/unorderedmap.h"
#include "Module/Util/arguments.h"
#include "Module/Filer/file_helper.h"
#include "Module/Logger/backlogger.h"
#include "Module/Util/job_system.h"

#include <objbase.h>
#include <mutex>

#include <iostream>
#include <iomanip>
#include <mutex>
#include <string>
#include <cstdlib>
#include <Engine/Shaders/ShaderCompiler.h>
#include <Module/Util/p_timer.h>

using namespace pf;
using namespace pf::graphics;
std::mutex locker;
struct ShaderEntry
{
	std::string name;
	ShaderStage stage = ShaderStage::Count;
	pf::graphics::ShaderModel minshadermodel = ShaderModel::SM_5_0;
	struct Permutation
	{
		vector<std::string> defines;
	};
	vector<Permutation> permutations;
};


vector<ShaderEntry> shaders = {
	{"hairparticle_simulateCS",pf::graphics::ShaderStage::CS},
	{"emittedparticle_simulateCS",pf::graphics::ShaderStage::CS},
	{"generateMIPChainCubeCS_float4",pf::graphics::ShaderStage::CS},
	{"generateMIPChainCubeArrayCS_float4",pf::graphics::ShaderStage::CS},
	{"generateMIPChain3DCS_float4",pf::graphics::ShaderStage::CS},
	{"generateMIPChain2DCS_float4",pf::graphics::ShaderStage::CS},
	{"blockcompressCS_BC1",pf::graphics::ShaderStage::CS},
	{"blockcompressCS_BC3",pf::graphics::ShaderStage::CS},
	{"blockcompressCS_BC4",pf::graphics::ShaderStage::CS},
	{"blockcompressCS_BC5",pf::graphics::ShaderStage::CS},
	{"blockcompressCS_BC6H",pf::graphics::ShaderStage::CS},
	{"blockcompressCS_BC6H_cubemap",pf::graphics::ShaderStage::CS},
	{"blur_gaussian_float4CS",pf::graphics::ShaderStage::CS},
	{"bloomseparateCS",pf::graphics::ShaderStage::CS},
	{"depthoffield_mainCS",pf::graphics::ShaderStage::CS},
	{"depthoffield_neighborhoodMaxCOCCS",pf::graphics::ShaderStage::CS},
	{"depthoffield_prepassCS",pf::graphics::ShaderStage::CS},
	{"depthoffield_upsampleCS",pf::graphics::ShaderStage::CS},
	{"depthoffield_tileMaxCOC_verticalCS",pf::graphics::ShaderStage::CS},
	{"depthoffield_tileMaxCOC_horizontalCS",pf::graphics::ShaderStage::CS},
	{"vxgi_offsetprevCS",pf::graphics::ShaderStage::CS},
	{"vxgi_temporalCS",pf::graphics::ShaderStage::CS},
	{"vxgi_sdf_jumpfloodCS",pf::graphics::ShaderStage::CS},
	{"vxgi_resolve_diffuseCS",pf::graphics::ShaderStage::CS},
	{"vxgi_resolve_specularCS",pf::graphics::ShaderStage::CS},
	{"upsample_bilateral_float1CS",pf::graphics::ShaderStage::CS},
	{"upsample_bilateral_float4CS",pf::graphics::ShaderStage::CS},
	{"temporalaaCS",pf::graphics::ShaderStage::CS},
	{"tonemapCS",pf::graphics::ShaderStage::CS},
	{"underwaterCS",pf::graphics::ShaderStage::CS},
	{"mesh_blend_prepareCS",pf::graphics::ShaderStage::CS},
	{"mesh_blend_expandCS",pf::graphics::ShaderStage::CS},
	{"mesh_blendPS",pf::graphics::ShaderStage::PS},
	{"fsr_upscalingCS",pf::graphics::ShaderStage::CS},
	{"fsr_sharpenCS",pf::graphics::ShaderStage::CS},
	{"ffx-fsr2/ffx_fsr2_autogen_reactive_pass",pf::graphics::ShaderStage::CS},
	{"ffx-fsr2/ffx_fsr2_compute_luminance_pyramid_pass",pf::graphics::ShaderStage::CS},
	{"ffx-fsr2/ffx_fsr2_prepare_input_color_pass",pf::graphics::ShaderStage::CS},
	{"ffx-fsr2/ffx_fsr2_reconstruct_previous_depth_pass",pf::graphics::ShaderStage::CS},
	{"ffx-fsr2/ffx_fsr2_depth_clip_pass",pf::graphics::ShaderStage::CS},
	{"ffx-fsr2/ffx_fsr2_lock_pass",pf::graphics::ShaderStage::CS},
	{"ffx-fsr2/ffx_fsr2_accumulate_pass",pf::graphics::ShaderStage::CS},
	{"ffx-fsr2/ffx_fsr2_rcas_pass",pf::graphics::ShaderStage::CS},
	{"ssaoCS",pf::graphics::ShaderStage::CS},
	{"ssgi_deinterleaveCS",pf::graphics::ShaderStage::CS},
	{"ssgiCS",pf::graphics::ShaderStage::CS},
	{"ssgi_upsampleCS",pf::graphics::ShaderStage::CS},
	{"rtdiffuseCS",pf::graphics::ShaderStage::CS,pf::graphics::ShaderModel::SM_6_5},
	{"rtdiffuse_spatialCS",pf::graphics::ShaderStage::CS},
	{"rtdiffuse_temporalCS",pf::graphics::ShaderStage::CS},
	{"rtdiffuse_upsampleCS",pf::graphics::ShaderStage::CS},
	{"rtreflectionCS",pf::graphics::ShaderStage::CS,pf::graphics::ShaderModel::SM_6_5},
	{"ssr_tileMaxRoughness_horizontalCS",pf::graphics::ShaderStage::CS},
	{"ssr_tileMaxRoughness_verticalCS",pf::graphics::ShaderStage::CS},
	{"ssr_depthHierarchyCS",pf::graphics::ShaderStage::CS},
	{"ssr_resolveCS",pf::graphics::ShaderStage::CS},
	{"ssr_temporalCS",pf::graphics::ShaderStage::CS},
	{"ssr_upsampleCS",pf::graphics::ShaderStage::CS},
	{"ssr_raytraceCS",pf::graphics::ShaderStage::CS},
	{"ssr_raytraceCS_cheap",pf::graphics::ShaderStage::CS},
	{"ssr_raytraceCS_earlyexit",pf::graphics::ShaderStage::CS},
	{"sharpenCS",pf::graphics::ShaderStage::CS},
	{"crt_screenCS",pf::graphics::ShaderStage::CS},
	{"skinningCS",pf::graphics::ShaderStage::CS},
	{"resolveMSAADepthStencilCS",pf::graphics::ShaderStage::CS},
	{"raytraceCS",pf::graphics::ShaderStage::CS},
	{"raytraceCS_rtapi",pf::graphics::ShaderStage::CS,pf::graphics::ShaderModel::SM_6_5},
	{"paint_textureCS",pf::graphics::ShaderStage::CS},
	{"oceanUpdateDisplacementMapCS",pf::graphics::ShaderStage::CS},
	{"oceanUpdateGradientFoldingCS",pf::graphics::ShaderStage::CS},
	{"oceanSimulatorCS",pf::graphics::ShaderStage::CS},
	{"msao_interleaveCS",pf::graphics::ShaderStage::CS},
	{"msao_preparedepthbuffers1CS",pf::graphics::ShaderStage::CS},
	{"msao_preparedepthbuffers2CS",pf::graphics::ShaderStage::CS},
	{"msao_blurupsampleCS",pf::graphics::ShaderStage::CS},
	{"msao_blurupsampleCS_blendout",pf::graphics::ShaderStage::CS},
	{"msao_blurupsampleCS_premin",pf::graphics::ShaderStage::CS},
	{"msao_blurupsampleCS_premin_blendout",pf::graphics::ShaderStage::CS},
	{"msaoCS",pf::graphics::ShaderStage::CS},
	{"motionblur_neighborhoodMaxVelocityCS",pf::graphics::ShaderStage::CS},
	{"motionblur_tileMaxVelocity_horizontalCS",pf::graphics::ShaderStage::CS},
	{"motionblur_tileMaxVelocity_verticalCS",pf::graphics::ShaderStage::CS},
	{"luminancePass2CS",pf::graphics::ShaderStage::CS},
	{"motionblurCS",pf::graphics::ShaderStage::CS},
	{"motionblurCS_cheap",pf::graphics::ShaderStage::CS},
	{"motionblurCS_earlyexit",pf::graphics::ShaderStage::CS},
	{"luminancePass1CS",pf::graphics::ShaderStage::CS},
	{"lightShaftsCS",pf::graphics::ShaderStage::CS},
	{"lightCullingCS_ADVANCED_DEBUG",pf::graphics::ShaderStage::CS},
	{"lightCullingCS_DEBUG",pf::graphics::ShaderStage::CS},
	{"lightCullingCS",pf::graphics::ShaderStage::CS},
	{"lightCullingCS_ADVANCED",pf::graphics::ShaderStage::CS},
	{"hbaoCS",pf::graphics::ShaderStage::CS},
	{"gpusortlib_sortInnerCS",pf::graphics::ShaderStage::CS},
	{"gpusortlib_sortStepCS",pf::graphics::ShaderStage::CS},
	{"gpusortlib_kickoffSortCS",pf::graphics::ShaderStage::CS},
	{"gpusortlib_sortCS",pf::graphics::ShaderStage::CS},
	{"fxaaCS",pf::graphics::ShaderStage::CS},
	{"filterEnvMapCS",pf::graphics::ShaderStage::CS},
	{"fft_512x512_c2c_CS",pf::graphics::ShaderStage::CS},
	{"fft_512x512_c2c_v2_CS",pf::graphics::ShaderStage::CS},
	{"emittedparticle_sphpartitionCS",pf::graphics::ShaderStage::CS},
	{"emittedparticle_sphcellallocationCS",pf::graphics::ShaderStage::CS},
	{"emittedparticle_sphbinningCS",pf::graphics::ShaderStage::CS},
	{"emittedparticle_simulateCS_SORTING",pf::graphics::ShaderStage::CS},
	{"emittedparticle_simulateCS_SORTING_DEPTHCOLLISIONS",pf::graphics::ShaderStage::CS},
	{"emittedparticle_sphdensityCS",pf::graphics::ShaderStage::CS},
	{"emittedparticle_sphforceCS",pf::graphics::ShaderStage::CS},
	{"emittedparticle_kickoffUpdateCS",pf::graphics::ShaderStage::CS},
	{"emittedparticle_simulateCS_DEPTHCOLLISIONS",pf::graphics::ShaderStage::CS},
	{"emittedparticle_emitCS",pf::graphics::ShaderStage::CS},
	{"emittedparticle_emitCS_FROMMESH",pf::graphics::ShaderStage::CS},
	{"emittedparticle_emitCS_volume",pf::graphics::ShaderStage::CS},
	{"emittedparticle_finishUpdateCS",pf::graphics::ShaderStage::CS},
	{"downsample4xCS",pf::graphics::ShaderStage::CS},
	{"lineardepthCS",pf::graphics::ShaderStage::CS},
	{"depthoffield_prepassCS_earlyexit",pf::graphics::ShaderStage::CS},
	{"depthoffield_mainCS_cheap",pf::graphics::ShaderStage::CS},
	{"depthoffield_mainCS_earlyexit",pf::graphics::ShaderStage::CS },
	{"depthoffield_postfilterCS",pf::graphics::ShaderStage::CS },
	{"copytexture2D_float4_borderexpandCS",pf::graphics::ShaderStage::CS },
	{"copytexture2D_float4CS",pf::graphics::ShaderStage::CS },
	{"chromatic_aberrationCS",pf::graphics::ShaderStage::CS },
	{"bvh_hierarchyCS",pf::graphics::ShaderStage::CS },
	{"bvh_primitivesCS",pf::graphics::ShaderStage::CS },
	{"bvh_propagateaabbCS",pf::graphics::ShaderStage::CS },
	{"blur_gaussian_wide_float1CS",pf::graphics::ShaderStage::CS },
	{"blur_gaussian_wide_float4CS",pf::graphics::ShaderStage::CS },
	{"blur_gaussian_float1CS",pf::graphics::ShaderStage::CS },
	{"blur_bilateral_wide_float1CS",pf::graphics::ShaderStage::CS },
	{"blur_bilateral_wide_float4CS",pf::graphics::ShaderStage::CS },
	{"blur_bilateral_float1CS",pf::graphics::ShaderStage::CS },
	{"blur_bilateral_float4CS",pf::graphics::ShaderStage::CS },
	{"normalsfromdepthCS",pf::graphics::ShaderStage::CS },
	{"volumetricCloud_curlnoiseCS",pf::graphics::ShaderStage::CS },
	{"volumetricCloud_detailnoiseCS",pf::graphics::ShaderStage::CS },
	{"volumetricCloud_renderCS",pf::graphics::ShaderStage::CS },
	{"volumetricCloud_renderCS_capture",pf::graphics::ShaderStage::CS },
	{"volumetricCloud_renderCS_capture_MSAA",pf::graphics::ShaderStage::CS },
	{"volumetricCloud_reprojectCS",pf::graphics::ShaderStage::CS },
	{"volumetricCloud_shadow_renderCS",pf::graphics::ShaderStage::CS },
	{"volumetricCloud_shapenoiseCS",pf::graphics::ShaderStage::CS },
	{"volumetricCloud_upsamplePS",pf::graphics::ShaderStage::PS },
	{"volumetricCloud_weathermapCS",pf::graphics::ShaderStage::CS },
	{"shadingRateClassificationCS",pf::graphics::ShaderStage::CS },
	{"shadingRateClassificationCS_DEBUG",pf::graphics::ShaderStage::CS },
	{"aerialPerspectiveCS",pf::graphics::ShaderStage::CS },
	{"aerialPerspectiveCS_capture",pf::graphics::ShaderStage::CS },
	{"aerialPerspectiveCS_capture_MSAA",pf::graphics::ShaderStage::CS },
	{"skyAtmosphere_cameraVolumeLutCS",pf::graphics::ShaderStage::CS },
	{"skyAtmosphere_transmittanceLutCS",pf::graphics::ShaderStage::CS },
	{"skyAtmosphere_skyViewLutCS",pf::graphics::ShaderStage::CS },
	{"skyAtmosphere_multiScatteredLuminanceLutCS",pf::graphics::ShaderStage::CS },
	{"skyAtmosphere_skyLuminanceLutCS",pf::graphics::ShaderStage::CS },
	{"screenspaceshadowCS",pf::graphics::ShaderStage::CS },
	{"rtshadowCS",pf::graphics::ShaderStage::CS,pf::graphics::ShaderModel::SM_6_5 },
	{"rtshadow_denoise_tileclassificationCS",pf::graphics::ShaderStage::CS },
	{"rtshadow_denoise_filterCS",pf::graphics::ShaderStage::CS },
	{"rtshadow_denoise_temporalCS",pf::graphics::ShaderStage::CS },
	{"rtshadow_upsampleCS",pf::graphics::ShaderStage::CS },
	{"rtaoCS",pf::graphics::ShaderStage::CS,pf::graphics::ShaderModel::SM_6_5 },
	{"rtao_denoise_tileclassificationCS",pf::graphics::ShaderStage::CS },
	{"rtao_denoise_filterCS",pf::graphics::ShaderStage::CS },
	{"visibility_resolveCS",pf::graphics::ShaderStage::CS },
	{"visibility_resolveCS_MSAA",pf::graphics::ShaderStage::CS },
	{"visibility_velocityCS",pf::graphics::ShaderStage::CS },
	{"visibility_skyCS",pf::graphics::ShaderStage::CS },
	{"surfel_coverageCS",pf::graphics::ShaderStage::CS },
	{"surfel_indirectprepareCS",pf::graphics::ShaderStage::CS },
	{"surfel_updateCS",pf::graphics::ShaderStage::CS },
	{"surfel_gridoffsetsCS",pf::graphics::ShaderStage::CS },
	{"surfel_binningCS",pf::graphics::ShaderStage::CS },
	{"surfel_raytraceCS_rtapi",pf::graphics::ShaderStage::CS,pf::graphics::ShaderModel::SM_6_5 },
	{"surfel_raytraceCS",pf::graphics::ShaderStage::CS },
	{"surfel_integrateCS",pf::graphics::ShaderStage::CS },
	{"ddgi_rayallocationCS",pf::graphics::ShaderStage::CS },
	{"ddgi_indirectprepareCS",pf::graphics::ShaderStage::CS },
	{"ddgi_raytraceCS",pf::graphics::ShaderStage::CS },
	{"ddgi_raytraceCS_rtapi",pf::graphics::ShaderStage::CS,pf::graphics::ShaderModel::SM_6_5 },
	{"ddgi_updateCS",pf::graphics::ShaderStage::CS },
	{"ddgi_updateCS_depth",pf::graphics::ShaderStage::CS },
	{"terrainVirtualTextureUpdateCS",pf::graphics::ShaderStage::CS },
	{"terrainVirtualTextureUpdateCS_normalmap",pf::graphics::ShaderStage::CS },
	{"terrainVirtualTextureUpdateCS_surfacemap",pf::graphics::ShaderStage::CS },
	{"terrainVirtualTextureUpdateCS_emissivemap",pf::graphics::ShaderStage::CS },
	{"meshlet_prepareCS",pf::graphics::ShaderStage::CS },
	{"impostor_prepareCS",pf::graphics::ShaderStage::CS },
	{"virtualTextureTileRequestsCS",pf::graphics::ShaderStage::CS },
	{"virtualTextureTileAllocateCS",pf::graphics::ShaderStage::CS },
	{"virtualTextureResidencyUpdateCS",pf::graphics::ShaderStage::CS },
	{"windCS",pf::graphics::ShaderStage::CS },
	{"yuv_to_rgbCS",pf::graphics::ShaderStage::CS },
	{"wetmap_updateCS",pf::graphics::ShaderStage::CS },
	{"causticsCS",pf::graphics::ShaderStage::CS },
	{"depth_reprojectCS",pf::graphics::ShaderStage::CS },
	{"depth_pyramidCS",pf::graphics::ShaderStage::CS },
	{"lightmap_expandCS",pf::graphics::ShaderStage::CS },


	{"emittedparticlePS_soft",pf::graphics::ShaderStage::PS },
	{"imagePS",pf::graphics::ShaderStage::PS },
	{"emittedparticlePS_soft_lighting",pf::graphics::ShaderStage::PS },
	{"oceanSurfacePS",pf::graphics::ShaderStage::PS },
	{"hairparticlePS",pf::graphics::ShaderStage::PS },
	{"hairparticlePS_simple",pf::graphics::ShaderStage::PS },
	{"hairparticlePS_prepass",pf::graphics::ShaderStage::PS },
	{"hairparticlePS_prepass_depthonly",pf::graphics::ShaderStage::PS },
	{"hairparticlePS_shadow",pf::graphics::ShaderStage::PS },
	{"volumetricLight_SpotPS",pf::graphics::ShaderStage::PS },
	{"volumetricLight_PointPS",pf::graphics::ShaderStage::PS },
	{"volumetricLight_DirectionalPS",pf::graphics::ShaderStage::PS },
	{"volumetriclight_rectanglePS",pf::graphics::ShaderStage::PS },
	{"voxelPS",pf::graphics::ShaderStage::PS },
	{"vertexcolorPS",pf::graphics::ShaderStage::PS },
	{"upsample_bilateralPS",pf::graphics::ShaderStage::PS },
	{"sunPS",pf::graphics::ShaderStage::PS },
	{"skyPS_dynamic",pf::graphics::ShaderStage::PS },
	{"skyPS_static",pf::graphics::ShaderStage::PS },
	{"shadowPS_transparent",pf::graphics::ShaderStage::PS },
	{"shadowPS_water",pf::graphics::ShaderStage::PS },
	{"shadowPS_alphatest",pf::graphics::ShaderStage::PS },
	{"paintdecalPS",pf::graphics::ShaderStage::PS },
	{"renderlightmapPS",pf::graphics::ShaderStage::PS },
	{"renderlightmapPS_rtapi",pf::graphics::ShaderStage::PS,pf::graphics::ShaderModel::SM_6_5 },
	{"raytrace_debugbvhPS",pf::graphics::ShaderStage::PS },
	{"outlinePS",pf::graphics::ShaderStage::PS },
	{"oceanSurfaceSimplePS",pf::graphics::ShaderStage::PS },
	{"objectPS_voxelizer",pf::graphics::ShaderStage::PS },
	{"objectPS_hologram",pf::graphics::ShaderStage::PS },
	{"objectPS_paintradius",pf::graphics::ShaderStage::PS },
	{"objectPS_simple",pf::graphics::ShaderStage::PS },
	{"objectPS_debug",pf::graphics::ShaderStage::PS },
	{"objectPS_prepass",pf::graphics::ShaderStage::PS },
	{"objectPS_prepass_alphatest",pf::graphics::ShaderStage::PS },
	{"objectPS_prepass_depthonly_alphatest",pf::graphics::ShaderStage::PS },
	{"lightVisualizerPS",pf::graphics::ShaderStage::PS },
	{"vRectLightPS",pf::graphics::ShaderStage::PS },
	{"lensFlarePS",pf::graphics::ShaderStage::PS },
	{"impostorPS",pf::graphics::ShaderStage::PS },
	{"impostorPS_simple",pf::graphics::ShaderStage::PS },
	{"impostorPS_prepass",pf::graphics::ShaderStage::PS },
	{"impostorPS_prepass_depthonly",pf::graphics::ShaderStage::PS },
	{"forceFieldVisualizerPS",pf::graphics::ShaderStage::PS },
	{"fontPS",pf::graphics::ShaderStage::PS },
	{"envMap_skyPS_static",pf::graphics::ShaderStage::PS },
	{"envMap_skyPS_dynamic",pf::graphics::ShaderStage::PS },
	{"envMapPS",pf::graphics::ShaderStage::PS },
	{"emittedparticlePS_soft_distortion",pf::graphics::ShaderStage::PS },
	{"downsampleDepthBuffer4xPS",pf::graphics::ShaderStage::PS },
	{"emittedparticlePS_simple",pf::graphics::ShaderStage::PS },
	{"cubeMapPS",pf::graphics::ShaderStage::PS },
	{"circlePS",pf::graphics::ShaderStage::PS },
	{"captureImpostorPS",pf::graphics::ShaderStage::PS },
	{"ddgi_debugPS",pf::graphics::ShaderStage::PS },
	{"copyDepthPS",pf::graphics::ShaderStage::PS },
	{"copyStencilBitPS",pf::graphics::ShaderStage::PS },
	{"extractStencilBitPS",pf::graphics::ShaderStage::PS },
	{"trailPS",pf::graphics::ShaderStage::PS },
	{"waveeffectPS",pf::graphics::ShaderStage::PS },


	{"hairparticleVS",pf::graphics::ShaderStage::VS },
	{"emittedparticleVS",pf::graphics::ShaderStage::VS },
	{"imageVS",pf::graphics::ShaderStage::VS },
	{"fontVS",pf::graphics::ShaderStage::VS },
	{"voxelVS",pf::graphics::ShaderStage::VS },
	{"vertexcolorVS",pf::graphics::ShaderStage::VS },
	{"volumetriclight_directionalVS",pf::graphics::ShaderStage::VS },
	{"volumetriclight_pointVS",pf::graphics::ShaderStage::VS },
	{"volumetriclight_spotVS",pf::graphics::ShaderStage::VS },
	{"volumetriclight_rectangleVS",pf::graphics::ShaderStage::VS },
	{"vSpotLightVS",pf::graphics::ShaderStage::VS },
	{"vPointLightVS",pf::graphics::ShaderStage::VS },
	{"vRectLightVS",pf::graphics::ShaderStage::VS },
	{"sphereVS",pf::graphics::ShaderStage::VS },
	{"skyVS",pf::graphics::ShaderStage::VS },
	{"postprocessVS",pf::graphics::ShaderStage::VS },
	{"paintdecalVS",pf::graphics::ShaderStage::VS },
	{"renderlightmapVS",pf::graphics::ShaderStage::VS },
	{"raytrace_screenVS",pf::graphics::ShaderStage::VS },
	{"oceanSurfaceVS",pf::graphics::ShaderStage::VS },
	{"objectVS_debug",pf::graphics::ShaderStage::VS },
	{"objectVS_voxelizer",pf::graphics::ShaderStage::VS },
	{"lensFlareVS",pf::graphics::ShaderStage::VS },
	{"impostorVS",pf::graphics::ShaderStage::VS },
	{"forceFieldPointVisualizerVS",pf::graphics::ShaderStage::VS },
	{"forceFieldPlaneVisualizerVS",pf::graphics::ShaderStage::VS },
	{"envMap_skyVS",pf::graphics::ShaderStage::VS },
	{"envMapVS",pf::graphics::ShaderStage::VS },
	{"occludeeVS",pf::graphics::ShaderStage::VS },
	{"ddgi_debugVS",pf::graphics::ShaderStage::VS },
	{"voxelGS",pf::graphics::ShaderStage::GS },
	{"objectGS_voxelizer",pf::graphics::ShaderStage::GS },
	{"objectVS_simple",pf::graphics::ShaderStage::VS },
	{"objectVS_common",pf::graphics::ShaderStage::VS },
	{"objectVS_common_tessellation",pf::graphics::ShaderStage::VS },
	{"objectVS_prepass",pf::graphics::ShaderStage::VS },
	{"objectVS_prepass_alphatest",pf::graphics::ShaderStage::VS },
	{"objectVS_prepass_tessellation",pf::graphics::ShaderStage::VS },
	{"objectVS_prepass_alphatest_tessellation",pf::graphics::ShaderStage::VS },
	{"objectVS_simple_tessellation",pf::graphics::ShaderStage::VS },
	{"shadowVS",pf::graphics::ShaderStage::VS },
	{"shadowVS_alphatest",pf::graphics::ShaderStage::VS },
	{"shadowVS_transparent",pf::graphics::ShaderStage::VS },
	{"screenVS",pf::graphics::ShaderStage::VS },
	{"trailVS",pf::graphics::ShaderStage::VS },



	{"objectDS",pf::graphics::ShaderStage::DS },
	{"objectDS_prepass",pf::graphics::ShaderStage::DS },
	{"objectDS_prepass_alphatest",pf::graphics::ShaderStage::DS },
	{"objectDS_simple",pf::graphics::ShaderStage::DS },


	{"objectHS",pf::graphics::ShaderStage::HS },
	{"objectHS_prepass",pf::graphics::ShaderStage::HS },
	{"objectHS_prepass_alphatest",pf::graphics::ShaderStage::HS },
	{"objectHS_simple",pf::graphics::ShaderStage::HS },

	{"emittedparticleMS",pf::graphics::ShaderStage::MS },

	{"objectMS",pf::graphics::ShaderStage::MS },
	{"objectMS_prepass",pf::graphics::ShaderStage::MS },
	{"objectMS_prepass_alphatest",pf::graphics::ShaderStage::MS },
	{"objectMS_simple",pf::graphics::ShaderStage::MS },
	{"shadowMS",pf::graphics::ShaderStage::MS },
	{"shadowMS_alphatest",pf::graphics::ShaderStage::MS },
	{"shadowMS_transparent",pf::graphics::ShaderStage::MS },

	{"objectAS",pf::graphics::ShaderStage::AS },


	//{"rtreflectionLIB",pf::graphics::ShaderStage::LIB },
};

struct Target
{
	pf::graphics::ShaderFormat format;
	std::string dir;
};
vector<Target> targets;
unordered_map<std::string, pf::shadercompiler::CompilerOutput> results;
bool rebuild = false;
bool shaderdump_enabled = false;

int test_shader_compare_test(int argc, char* argv[]) {
	pf::shadercompiler::Flags compile_flags = pf::shadercompiler::Flags::NONE;
	std::cout << "[Wicked Engine Offline Shader Compiler]\n";
	std::cout << "Available command arguments:\n";
	std::cout << "\thlsl5 : \t\tCompile shaders to hlsl5 (dx11) format (using d3dcompiler)\n";
	std::cout << "\thlsl6 : \t\tCompile shaders to hlsl6 (dx12) format (using dxcompiler)\n";
	std::cout << "\tspirv : \t\tCompile shaders to spirv (vulkan) format (using dxcompiler)\n";
	std::cout << "\thlsl6_xs : \t\tCompile shaders to hlsl6 Xbox Series native (dx12) format (requires Xbox SDK)\n";
	std::cout << "\tps5 : \t\t\tCompile shaders to PlayStation 5 native format (requires PlayStation 5 SDK)\n";
	std::cout << "\trebuild : \t\tAll shaders will be rebuilt, regardless if they are outdated or not\n";
	std::cout << "\tdisable_optimization : \tShaders will be compiled without optimizations\n";
	std::cout << "\tstrip_reflection : \tReflection will be stripped from shader binary to reduce file size\n";
	std::cout << "\tshaderdump : \t\tShaders will be saved to wiShaderDump.h C++ header file (can be combined with \"rebuild\")\n";
	std::cout << "Command arguments used: ";
	pf::arguments::Parse(argc, argv);


	if (pf::arguments::HasArgument("hlsl5"))
	{
		targets.push_back({ ShaderFormat::HLSL5, "shaders/hlsl5/" });
		std::cout << "hlsl5 ";
	}
	if (pf::arguments::HasArgument("hlsl6"))
	{
		targets.push_back({ ShaderFormat::HLSL6, "shaders/hlsl6/" });
		std::cout << "hlsl6 ";
	}
	if (pf::arguments::HasArgument("spirv"))
	{
		targets.push_back({ ShaderFormat::SPIRV, "shaders/spirv/" });
		std::cout << "spirv ";
	}
	if (pf::arguments::HasArgument("hlsl6_xs"))
	{
		targets.push_back({ ShaderFormat::HLSL6_XS, "shaders/hlsl6_xs/" });
		std::cout << "hlsl6_xs ";
	}
	if (pf::arguments::HasArgument("ps5"))
	{
		targets.push_back({ ShaderFormat::PS5, "shaders/ps5/" });
		std::cout << "ps5 ";
	}

	if (pf::arguments::HasArgument("shaderdump"))
	{
		shaderdump_enabled = true;
		std::cout << "shaderdump ";
	}

	if (pf::arguments::HasArgument("rebuild"))
	{
		rebuild = true;
		std::cout << "rebuild ";
	}

	if (pf::arguments::HasArgument("disable_optimization"))
	{
		compile_flags |= pf::shadercompiler::Flags::DISABLE_OPTIMIZATION;
		std::cout << "disable_optimization ";
	}

	if (pf::arguments::HasArgument("strip_reflection"))
	{
		compile_flags |= pf::shadercompiler::Flags::STRIP_REFLECTION;
		std::cout << "strip_reflection ";
	}

	std::cout << "\n";

	if (targets.empty())
	{
		targets = {
			//{ ShaderFormat::HLSL5, "shaders/hlsl5/" },
			{ ShaderFormat::HLSL6, "shaders/hlsl6/" },
			{ ShaderFormat::SPIRV, "shaders/spirv/" },
		};
		std::cout << "No shader formats were specified, assuming command arguments: spirv hlsl6\n";
	}


	// permutations for objectPS:
	shaders.push_back({ "objectPS", graphics::ShaderStage::PS });
	
	shaders.push_back({ "visibility_surfaceCS", graphics::ShaderStage::CS });

	shaders.push_back({ "visibility_surfaceCS", graphics::ShaderStage::CS });

	std::string SHADERSOURCEPATH  = helper::GetCurrentPath() + "/Res/";
	//Logger("current dir %s ", SHADERSOURCEPATH.c_str());
	std::string absPath = SHADERSOURCEPATH + "/shaders";

	//Logger("absPath dir %s ", SHADERSOURCEPATH.c_str());
	helper::MakePathAbsolute(absPath);
	Timer timer;
	static int errors = 0;

	jobsystem::Initialize();
	jobsystem::context ctx;

	for (auto& target : targets) {
		std::string SHADERPATH = SHADERSOURCEPATH + target.dir;
		helper::DirectoryCreate(SHADERPATH);
		for (auto& shader : shaders) {
			//Logger("shader ");

			if (target.format == ShaderFormat::HLSL5)
			{
				if (
					shader.stage == ShaderStage::MS ||
					shader.stage == ShaderStage::AS ||
					shader.stage == ShaderStage::LIB
					)
				{
					// shader stage not applicable to HLSL5
					continue;
				}
			}

			vector<ShaderEntry::Permutation> permutations = shader.permutations;
			if (permutations.empty())
			{
				permutations.emplace_back();
			}


			for (auto permutation : permutations) {
				
				jobsystem::Execute(ctx, [=](jobsystem::JobArgs args) {
					std::string shaderbinaryfilename = SHADERPATH + shader.name;
					//Logger("Executee shaderbinaryfilename : %s ", shaderbinaryfilename.c_str());
					for (auto& def : permutation.defines)
					{
						shaderbinaryfilename += "_" + def;
					}
					shaderbinaryfilename += ".cso";

					shadercompiler::CompilerOutput output;

					if (!shadercompiler::IsShaderOutdated(shaderbinaryfilename))
					{
					//	Logger("IsShaderOutdated : %s ", shaderbinaryfilename.c_str());
						if (!rebuild)
						{
							if (shaderdump_enabled)
							{
								auto vec = std::make_shared<std::vector<uint8_t>>();

								if (helper::FileRead(shaderbinaryfilename, *vec))
								{
									output.internal_state = vec;
									output.shaderdata = vec->data();
									output.shadersize = vec->size();
									locker.lock();
									results[shaderbinaryfilename] = output;
									std::cout << "up-to-date: " << shaderbinaryfilename << std::endl;
									Logger("FileRead error  up-to-date:: %s ", shaderbinaryfilename.c_str());
									locker.unlock();
								}
								else {
									locker.lock();
									Logger("ERROR reading binary shader:: %s ", shaderbinaryfilename.c_str());
									std::cerr << "ERROR reading binary shader: " << shaderbinaryfilename << std::endl;
									locker.unlock();
								}
							}
							return;
						}
					}

					shadercompiler::CompilerInput input;
					input.flags = compile_flags;
					input.format = target.format;
					input.stage = shader.stage;
					input.shadersourcefilename = SHADERSOURCEPATH+"shaders/" + shader.name + ".hlsl";
					input.include_directories.push_back(SHADERSOURCEPATH + "shaders/");
					input.include_directories.push_back(SHADERSOURCEPATH + "shaders/" + helper::GetDirectoryFromPath(shader.name));
					input.minshadermodel = shader.minshadermodel;
					input.defines = permutation.defines;
				//	Logger("input : %s ", input.shadersourcefilename.c_str());
					if (input.minshadermodel > ShaderModel::SM_5_0 && target.format == ShaderFormat::HLSL5)
					{
						// if shader format cannot support shader model, then we cancel the task without returning error
						return;
					}
					if (target.format == ShaderFormat::PS5 && (input.minshadermodel >= ShaderModel::SM_6_5 || input.stage == ShaderStage::MS || input.stage == ShaderStage::AS))
					{
						// TODO PS5 raytracing, mesh shader
						return;
					}
					if (target.format == ShaderFormat::HLSL6_XS && (input.stage == ShaderStage::MS || input.stage == ShaderStage::AS))
					{
						// TODO Xbox mesh shader
						return;
					}

					shadercompiler::Compile(input, output);
					//std::cout << "shadercompiler is valid " << output.IsValid() << std::endl;
					if (output.IsValid())
					{
						shadercompiler::SaveShaderAndMetadata(shaderbinaryfilename, output);

						locker.lock();
						if (!output.error_message.empty())
						{
							std::cerr << output.error_message << "\n";
						}
						Logger("shader compiled : %s ", shaderbinaryfilename.c_str());
						//std::cout << "shader compiled: " << shaderbinaryfilename << "\n";
						if (shaderdump_enabled)
						{
							results[shaderbinaryfilename] = output;
						}
						locker.unlock();
					}
					else
					{
						locker.lock();
						std::cerr << "shader compile FAILED: " << shaderbinaryfilename << "\n" << output.error_message;
						errors++;
						locker.unlock();
					}
					});
			}
		}

	}


	jobsystem::Wait(ctx);



	std::cout << "[Wicked Engine Offline Shader Compiler] Finished in " << std::setprecision(4) << timer.elapsed_seconds() << " seconds with " << errors << " errors\n";

	return 0;
}