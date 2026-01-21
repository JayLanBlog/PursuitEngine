#pragma once
#include "Core/core_include.h"
#include "enums.h"
#include "Engine/Device/graph_driver.h"


namespace pf::renderer {



	constexpr uint32_t CombineStencilrefs(enums::STENCILREF engineStencilRef, uint8_t userStencilRef)
	{
		return (userStencilRef << 4) | static_cast<uint8_t>(engineStencilRef);
	}

	void Initialize();

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


	const pf::graphics::Shader* GetShader(pf::enums::SHADERTYPE id);
	const pf::graphics::InputLayout* GetInputLayout(pf::enums::ILTYPES id);
	const pf::graphics::RasterizerState* GetRasterizerState(pf::enums::RSTYPES id);
	const pf::graphics::DepthStencilState* GetDepthStencilState(pf::enums::DSSTYPES id);
	const pf::graphics::BlendState* GetBlendState(pf::enums::BSTYPES id);




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
}