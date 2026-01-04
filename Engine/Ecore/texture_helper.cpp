#include "texture_helper.h"

using namespace pf::graphics;

namespace pf::texturehelper {

	bool CreateTexture(
		graphics::Texture& texture,
		const void* data,
		uint32_t width,
		uint32_t height,
		graphics::Format format,
		graphics::Swizzle swizzle
	) {
		if (data == nullptr)
		{
			return false;
		}
		GraphicsDevice* device = GetDevice();
		TextureDesc desc;
		desc.width = width;
		desc.height = height;
		desc.mip_levels = 1;
		desc.array_size = 1;
		desc.format = format;
		desc.sample_count = 1;
		desc.bind_flags = BindFlag::SHADER_RESOURCE;
		desc.swizzle = swizzle;
		SubresourceData InitData;
		InitData.data_ptr = data;
		InitData.row_pitch = width * GetFormatStride(format) / GetFormatBlockSize(format);

		return device->CreateTexture(&desc, &InitData, &texture);
	}
	
}