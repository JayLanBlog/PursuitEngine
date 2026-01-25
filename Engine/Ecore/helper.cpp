#include "helper.h"
namespace pf::helper {
	bool saveTextureToMemory(const pf::graphics::Texture& texture, pf::vector<uint8_t>& texturedata) {
		using namespace pf::graphics;

		GraphicsDevice* device = pf::graphics::GetDevice();

		TextureDesc desc = texture.GetDesc();

		Texture stagingTex;
		TextureDesc staging_desc = desc;
		staging_desc.usage = Usage::READBACK;
		staging_desc.layout = ResourceState::COPY_DST;
		staging_desc.bind_flags = BindFlag::NONE;
		staging_desc.misc_flags = ResourceMiscFlag::NONE;
		bool success = device->CreateTexture(&staging_desc, nullptr, &stagingTex);
		assert(success);

		CommandList cmd = device->BeginCommandList();

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&texture,texture.desc.layout,ResourceState::COPY_SRC),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->CopyResource(&stagingTex, &texture, cmd);

		{
			GPUBarrier barriers[] = {
				GPUBarrier::Image(&texture,ResourceState::COPY_SRC,texture.desc.layout),
			};
			device->Barrier(barriers, arraysize(barriers), cmd);
		}

		device->SubmitCommandLists();
		device->WaitForGPU();

		texturedata.clear();

		if (stagingTex.mapped_data != nullptr)
		{
			texturedata.resize(ComputeTextureMemorySizeInBytes(desc));

			const uint32_t data_stride = GetFormatStride(desc.format);
			const uint32_t block_size = GetFormatBlockSize(desc.format);
			size_t cpy_offset = 0;
			size_t subresourceIndex = 0;
			for (uint32_t layer = 0; layer < desc.array_size; ++layer)
			{
				for (uint32_t mip = 0; mip < desc.mip_levels; ++mip)
				{
					const uint32_t mip_width = std::max(1u, desc.width >> mip);
					const uint32_t mip_height = std::max(1u, desc.height >> mip);
					const uint32_t mip_depth = std::max(1u, desc.depth >> mip);
					const uint32_t num_blocks_x = (mip_width + block_size - 1) / block_size;
					const uint32_t num_blocks_y = (mip_height + block_size - 1) / block_size;

					assert(subresourceIndex < stagingTex.mapped_subresource_count);
					const SubresourceData& subresourcedata = stagingTex.mapped_subresources[subresourceIndex++];
					const size_t dst_rowpitch = num_blocks_x * data_stride;
					for (uint32_t z = 0; z < mip_depth; ++z)
					{
						uint8_t* dst_slice = texturedata.data() + cpy_offset;
						uint8_t* src_slice = (uint8_t*)subresourcedata.data_ptr + subresourcedata.slice_pitch * z;
						for (uint32_t i = 0; i < num_blocks_y; ++i)
						{
							std::memcpy(
								dst_slice + i * dst_rowpitch,
								src_slice + i * subresourcedata.row_pitch,
								dst_rowpitch
							);
						}
						cpy_offset += num_blocks_y * dst_rowpitch;
					}
				}
			}
		}
		else
		{
			assert(0);
		}

		return stagingTex.mapped_data != nullptr;
	}
}