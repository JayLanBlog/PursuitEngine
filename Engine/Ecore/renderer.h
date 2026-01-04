#pragma once
#include "Core/core_include.h"
#include "enums.h"
#include "Engine/Device/graph_driver.h"


namespace pf::renderer {



	constexpr uint32_t CombineStencilrefs(enums::STENCILREF engineStencilRef, uint8_t userStencilRef)
	{
		return (userStencilRef << 4) | static_cast<uint8_t>(engineStencilRef);
	}

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
}