#pragma once
#include "Core/core_include.h"
#include "enums.h"
#include "Engine/Device/graph_driver.h"
#include "module.h"

namespace pf::renderer {

	constexpr pf::graphics::Format format_depthbuffer_main = pf::graphics::Format::D32_FLOAT_S8X24_UINT;
	constexpr pf::graphics::Format format_rendertarget_main = pf::graphics::Format::R11G11B10_FLOAT;
	constexpr pf::graphics::Format format_idbuffer = pf::graphics::Format::R32_UINT;
	constexpr pf::graphics::Format format_rendertarget_shadowmap = pf::graphics::Format::R16G16B16A16_FLOAT;
	constexpr pf::graphics::Format format_depthbuffer_shadowmap = pf::graphics::Format::D16_UNORM;
	//constexpr pf::graphics::Format format_depthbuffer_shadowmap = pf::graphics::Format::D32_FLOAT;
	constexpr pf::graphics::Format format_rendertarget_envprobe = pf::graphics::Format::R11G11B10_FLOAT;
	constexpr pf::graphics::Format format_depthbuffer_envprobe = pf::graphics::Format::D16_UNORM;


	constexpr uint32_t CombineStencilrefs(enums::STENCILREF engineStencilRef, uint8_t userStencilRef)
	{
		return (userStencilRef << 4) | static_cast<uint8_t>(engineStencilRef);
	}

	void Initialize();
	struct BufferSuballocation
	{
		pf::graphics::GPUBuffer alias;
		pf::allocator::PageAllocator::Allocation allocation;
	};

	// Sub-allocate (thread-safe) from a global GPU buffer for memory aliasing purpose:
//	The buffer will be DEFAULT usage, useable as vertex buffer, index buffer and shader resource
//	The purpose is to suballocate smaller GPUBuffers inside a larger GPUBuffer and bind the large GPUBuffer once as index buffer,
//	while the small buffers can be allocated/deallocated from it with memory aliasing and also used regularly by themselves
	BufferSuballocation SuballocateGPUBuffer(uint64_t size);
	void UpdateGPUSuballocator(); // called every frame for deferred release of GPU suballocations



	bool LoadShader(
		pf::graphics::ShaderStage stage,
		pf::graphics::Shader& shader,
		const std::string& filename,
		pf::graphics::ShaderModel minshadermodel = pf::graphics::ShaderModel::SM_6_0,
		const vector<std::string>& permutation_defines = {}
	);

	// Add a texture that should be mipmapped whenever it is feasible to do so
	void AddDeferredMIPGen(const graphics::Texture& texture, bool preserve_coverage = false);

	void AddDeferredBlockCompression(const graphics::Texture& texture_src, const graphics::Texture& texture_bc);



	const pf::graphics::Sampler* GetSampler(pf::enums::SAMPLERTYPES id);
	const pf::graphics::Shader* GetShader(pf::enums::SHADERTYPE id);
	const pf::graphics::InputLayout* GetInputLayout(pf::enums::ILTYPES id);
	const pf::graphics::RasterizerState* GetRasterizerState(pf::enums::RSTYPES id);
	const pf::graphics::DepthStencilState* GetDepthStencilState(pf::enums::DSSTYPES id);
	const pf::graphics::BlendState* GetBlendState(pf::enums::BSTYPES id);
	const pf::graphics::GPUBuffer* GetBuffer(pf::enums::BUFFERTYPES id);
	const pf::graphics::Texture* GetTexture(pf::enums::TEXTYPES id);

	enum WIREFRAME_MODE
	{
		WIREFRAME_DISABLED = 0,
		WIREFRAME_ONLY = 1,
		WIREFRAME_OVERLAY = 2,
	};

	//const param config

	void SetWireRender(bool value);
	bool IsWireRender();
	void SetWireframeMode(WIREFRAME_MODE mode);
	WIREFRAME_MODE GetWireframeMode();
	void SetToDrawDebugBoneLines(bool param);
	bool GetToDrawDebugBoneLines();
	void SetToDrawDebugPartitionTree(bool param);
	bool GetToDrawDebugPartitionTree();
	bool GetToDrawDebugEnvProbes();
	void SetToDrawDebugEnvProbes(bool value);
	void SetToDrawDebugEmitters(bool param);
	bool GetToDrawDebugEmitters();
	void SetToDrawDebugForceFields(bool param);
	bool GetToDrawDebugForceFields();
	void SetToDrawDebugCameras(bool param);
	bool GetToDrawDebugCameras();
	void SetToDrawDebugColliders(bool param);
	bool GetToDrawDebugColliders();
	void SetToDrawDebugSprings(bool param);
	bool GetToDrawDebugSprings();
	bool GetToDrawGridHelper();
	void SetToDrawGridHelper(bool value);
	bool GetToDrawVoxelHelper();
	void SetToDrawVoxelHelper(bool value, int clipmap_level);
	void SetDebugLightCulling(bool enabled);
	bool GetDebugLightCulling();
	void SetAdvancedLightCulling(bool enabled);
	bool GetAdvancedLightCulling();
	void SetVariableRateShadingClassification(bool enabled);
	bool GetVariableRateShadingClassification();
	void SetVariableRateShadingClassificationDebug(bool enabled);
	bool GetVariableRateShadingClassificationDebug();
	void SetOcclusionCullingEnabled(bool enabled);
	bool GetOcclusionCullingEnabled();
	void SetTemporalAAEnabled(bool enabled);
	bool GetTemporalAAEnabled();
	void SetTemporalAADebugEnabled(bool enabled);
	bool GetTemporalAADebugEnabled();
	void SetFreezeCullingCameraEnabled(bool enabled);
	bool GetFreezeCullingCameraEnabled();
	void SetVXGIEnabled(bool enabled);
	bool GetVXGIEnabled();
	void SetVXGIReflectionsEnabled(bool enabled);
	bool GetVXGIReflectionsEnabled();
	void SetGameSpeed(float value);
	float GetGameSpeed();
	void SetShadowsEnabled(bool value);
	bool IsShadowsEnabled();
	void SetRaytraceBounceCount(uint32_t bounces);
	uint32_t GetRaytraceBounceCount();
	void SetRaytraceDebugBVHVisualizerEnabled(bool value);
	bool GetRaytraceDebugBVHVisualizerEnabled();
	void SetRaytracedShadowsEnabled(bool value);
	bool GetRaytracedShadowsEnabled();
	void SetTessellationEnabled(bool value);
	bool GetTessellationEnabled();
	void SetDisableAlbedoMaps(bool value);
	bool IsDisableAlbedoMaps();
	void SetForceDiffuseLighting(bool value);
	bool IsForceDiffuseLighting();
	void SetScreenSpaceShadowsEnabled(bool value);
	bool GetScreenSpaceShadowsEnabled();
	void SetSurfelGIEnabled(bool value);
	bool GetSurfelGIEnabled();
	//void SetSurfelGIDebugEnabled(SURFEL_DEBUG value);
	//SURFEL_DEBUG GetSurfelGIDebugEnabled();
	void SetDDGIEnabled(bool value);
	bool GetDDGIEnabled();
	void SetDDGIDebugEnabled(bool value);
	bool GetDDGIDebugEnabled();
	void SetDDGIRayCount(uint32_t value);
	uint32_t GetDDGIRayCount();
	void SetDDGIBlendSpeed(float value);
	float GetDDGIBlendSpeed();
	void SetGIBoost(float value);
	float GetGIBoost();
	void SetMeshShaderAllowed(bool value);
	bool IsMeshShaderAllowed();
	void SetMeshletOcclusionCullingEnabled(bool value);
	bool IsMeshletOcclusionCullingEnabled();
	void Workaround(const int bug, pf::graphics::CommandList cmd);
	void SetCapsuleShadowEnabled(bool value);
	bool IsCapsuleShadowEnabled();
	void SetCapsuleShadowAngle(float value); // cone angle in radians
	float GetCapsuleShadowAngle();
	void SetCapsuleShadowFade(float value);
	float GetCapsuleShadowFade();
	void SetShadowLODOverrideEnabled(bool value); // Allow shadowmap rendering to request custom LOD for objects (can result in shadow mismatch, but increased GPU performance)
	bool IsShadowLODOverrideEnabled();



	struct CustomShader
	{
		std::string name;
		uint32_t filterMask = pf::enums::FILTER_OPAQUE;
		pf::graphics::PipelineState pso[pf::enums::RENDERPASS_COUNT] = {};
	};
	// Registers a custom shader that can be set to materials. 
	//	Returns the ID of the custom shader that can be used with MaterialComponent::SetCustomShaderID()
	int RegisterCustomShader(const CustomShader& customShader);
	const pf::vector<CustomShader>& GetCustomShaders();



	// Compress a texture into Block Compressed format
//	texture_src	: source uncompressed texture
//	texture_bc	: destination comporessed texture, must be a supported BC format (BC1/BC3/BC4/BC5/BC6H_UFLOAT)
//	Currently this will handle simple Texture2D with mip levels, and additionally BC6H cubemap
	void BlockCompress(const pf::graphics::Texture& texture_src, const pf::graphics::Texture& texture_bc, pf::graphics::CommandList cmd, uint32_t dst_slice_offset = 0);



	constexpr uint8_t raytracing_inclusion_mask_shadow = 1 << 0;
	constexpr uint8_t raytracing_inclusion_mask_reflection = 1 << 1;
}