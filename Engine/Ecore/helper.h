#pragma once
#include "module_file.h"

namespace pf::helper {
	bool saveTextureToMemory(const pf::graphics::Texture& texture, pf::vector<uint8_t>& texturedata);
}

