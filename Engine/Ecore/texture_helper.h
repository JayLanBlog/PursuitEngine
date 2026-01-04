#pragma once

#include "color.h"
#include "Core/core_include.h"
#include "Engine/Device/graph_driver.h"

namespace pf::texturehelper {
	bool CreateTexture(
		graphics::Texture& texture,
		const void* data,
		uint32_t width,
		uint32_t height,
		graphics::Format format = graphics::Format::R8G8B8A8_UNORM,
		graphics::Swizzle swizzle = {}
	);

}