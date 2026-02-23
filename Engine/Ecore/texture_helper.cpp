#include "texture_helper.h"
#include "module_util.h"
#include "logo.h"
#include "waterripple.h"
#include <Utility/dds.h>
#include <Utility/lodepng.h>
#include <Utility/win32ico.h>
#include <Utility/stb_image_write.h>
#include "Module/Filer/file_helper.h"
using namespace pf::graphics;

// from Utility/samplerBlueNoiseErrorDistribution_128x128_OptimizedFor_2d2d2d2d_1spp.cpp
extern float samplerBlueNoiseErrorDistribution_128x128_OptimizedFor_2d2d2d2d_1spp(int pixel_i, int pixel_j, int sampleIndex, int sampleDimension);
namespace pf::texturehelper {

	enum HELPERTEXTURES
	{
		HELPERTEXTURE_LOGO,
		HELPERTEXTURE_RANDOM64X64,
		HELPERTEXTURE_COLORGRADEDEFAULT,
		HELPERTEXTURE_BLACKCUBEMAP,
		HELPERTEXTURE_UINT4,
		HELPERTEXTURE_BLUENOISE,
		HELPERTEXTURE_WATERRIPPLE,
		HELPERTEXTURE_BLACK,
		HELPERTEXTURE_WHITE,
		HELPERTEXTURE_TRANSPARENT,
		HELPERTEXTURE_NORMALMAPDEFAULT,
		HELPERTEXTURE_CHECKERBOARD,
		HELPERTEXTURE_COUNT
	};

	graphics::Texture helperTextures[HELPERTEXTURE_COUNT];
	unordered_map<unsigned long, graphics::Texture> colorTextures;
	SpinLock colorlock;

	void Initialize() {

		pf::Timer timer;

		GraphicsDevice* device = pf::graphics::GetDevice();

		// Logo
		{
			CreateTexture(helperTextures[HELPERTEXTURE_LOGO],engine_logo, 256, 256);
			device->SetName(&helperTextures[HELPERTEXTURE_LOGO], "HELPERTEXTURE_LOGO");
		}

		// Random64x64
		{
			uint8_t data[64 * 64 * 4];
			for (int i = 0; i < arraysize(data); i += 4)
			{
				data[i] = pf::random::GetRandom(0, 255);
				data[i + 1] = pf::random::GetRandom(0, 255);
				data[i + 2] = pf::random::GetRandom(0, 255);
				data[i + 3] = pf::random::GetRandom(0, 255);
			}

			CreateTexture(helperTextures[HELPERTEXTURE_RANDOM64X64], data, 64, 64);
			device->SetName(&helperTextures[HELPERTEXTURE_RANDOM64X64], "HELPERTEXTURE_RANDOM64X64");
		}

		// ColorGradeDefault
		{
			uint8_t data[256 * 16 * 4];
			for (uint8_t slice = 0; slice < 16; ++slice)
			{
				for (int x = 0; x < 16; ++x)
				{
					for (int y = 0; y < 16; ++y)
					{
						uint8_t r = x * 16 + x;
						uint8_t g = y * 16 + y;
						uint8_t b = slice * 16 + slice;

						int gridPos = (slice * 16 + y * 256 + x) * 4;
						data[gridPos] = r;
						data[gridPos + 1] = g;
						data[gridPos + 2] = b;
						data[gridPos + 3] = 255;
					}
				}
			}

			CreateTexture(helperTextures[HELPERTEXTURE_COLORGRADEDEFAULT], data, 256, 16);
			device->SetName(&helperTextures[HELPERTEXTURE_COLORGRADEDEFAULT], "HELPERTEXTURE_COLORGRADEDEFAULT");
		}

		// BlackCubemap
		{
			const int width = 1;
			const int height = 1;

			TextureDesc texDesc;
			texDesc.width = width;
			texDesc.height = height;
			texDesc.mip_levels = 1;
			texDesc.array_size = 6;
			texDesc.format = Format::R8G8B8A8_UNORM;
			texDesc.sample_count = 1;
			texDesc.usage = Usage::DEFAULT;
			texDesc.bind_flags = BindFlag::SHADER_RESOURCE;
			texDesc.misc_flags = ResourceMiscFlag::TEXTURECUBE;

			SubresourceData pData[6];
			pf::Color d[6][width * height] = {}; // 6 images initialized to 0 (transparent black)

			for (int cubeMapFaceIndex = 0; cubeMapFaceIndex < 6; cubeMapFaceIndex++)
			{
				pData[cubeMapFaceIndex].data_ptr = &d[cubeMapFaceIndex][0];// description.data;
				pData[cubeMapFaceIndex].row_pitch = width * 4;
				pData[cubeMapFaceIndex].slice_pitch = 0;
			}

			device->CreateTexture(&texDesc, &pData[0], &helperTextures[HELPERTEXTURE_BLACKCUBEMAP]);
			device->SetName(&helperTextures[HELPERTEXTURE_BLACKCUBEMAP], "HELPERTEXTURE_BLACKCUBEMAP");
		}

		// UINT4:
		{
			uint8_t data[16] = {};
			CreateTexture(helperTextures[HELPERTEXTURE_UINT4], data, 1, 1, Format::R32G32B32A32_UINT);
			device->SetName(&helperTextures[HELPERTEXTURE_UINT4], "HELPERTEXTURE_UINT4");
		}

		// Blue Noise:
		{
			pf::vector<pf::Color> bluenoise(128 * 128); // heap alloc intended (PS5)

			for (int y = 0; y < 128; ++y)
			{
				for (int x = 0; x < 128; ++x)
				{
					const float f0 = samplerBlueNoiseErrorDistribution_128x128_OptimizedFor_2d2d2d2d_1spp(x, y, 0, 0);
					const float f1 = samplerBlueNoiseErrorDistribution_128x128_OptimizedFor_2d2d2d2d_1spp(x, y, 0, 1);
					const float f2 = samplerBlueNoiseErrorDistribution_128x128_OptimizedFor_2d2d2d2d_1spp(x, y, 0, 2);
					const float f3 = samplerBlueNoiseErrorDistribution_128x128_OptimizedFor_2d2d2d2d_1spp(x, y, 0, 3);

					bluenoise[x + y * 128] = pf::Color::fromFloat4(XMFLOAT4(f0, f1, f2, f3));
				}
			}

			CreateTexture(helperTextures[HELPERTEXTURE_BLUENOISE], bluenoise.data(), 128, 128, Format::R8G8B8A8_UNORM);
			device->SetName(&helperTextures[HELPERTEXTURE_BLUENOISE], "HELPERTEXTURE_BLUENOISE");
		}

		// Water ripple:
		{
			TextureDesc desc;
			desc.width = 64;
			desc.height = 64;
			desc.mip_levels = 7;
			desc.format = Format::BC5_UNORM;
			desc.swizzle = { ComponentSwizzle::R,ComponentSwizzle::G,ComponentSwizzle::ONE,ComponentSwizzle::ONE };
			desc.bind_flags = BindFlag::SHADER_RESOURCE;

			const uint32_t data_stride = GetFormatStride(desc.format);
			const uint32_t block_size = GetFormatBlockSize(desc.format);
			const uint8_t* src = waterriple;
			SubresourceData initdata[7] = {};
			for (uint32_t mip = 0; mip < desc.mip_levels; ++mip)
			{
				const uint32_t num_blocks_x = std::max(1u, desc.width >> mip) / block_size;
				const uint32_t num_blocks_y = std::max(1u, desc.height >> mip) / block_size;
				initdata[mip].data_ptr = src;
				initdata[mip].row_pitch = num_blocks_x * data_stride;
				src += num_blocks_x * num_blocks_y * data_stride;
			}
			device->CreateTexture(&desc, initdata, &helperTextures[HELPERTEXTURE_WATERRIPPLE]);
			device->SetName(&helperTextures[HELPERTEXTURE_WATERRIPPLE], "HELPERTEXTURE_WATERRIPPLE");
		}

		// Checkerboard:
		{
			pf::Color checker[] = {
				pf::Color(255,255,255,255), pf::Color(127,127,127,255),
				pf::Color(127,127,127,255), pf::Color(255,255,255,255),
			};

			CreateTexture(helperTextures[HELPERTEXTURE_CHECKERBOARD], checker, 2, 2, Format::R8G8B8A8_UNORM);
			device->SetName(&helperTextures[HELPERTEXTURE_CHECKERBOARD], "HELPERTEXTURE_CHECKERBOARD");
		}

		// Single colors:
		{
			pf::Color color = pf::Color::Black();
			CreateTexture(helperTextures[HELPERTEXTURE_BLACK], (const uint8_t*)&color, 1, 1);
			device->SetName(&helperTextures[HELPERTEXTURE_BLACK], "HELPERTEXTURE_BLACK");

			color = pf::Color::White();
			CreateTexture(helperTextures[HELPERTEXTURE_WHITE], (const uint8_t*)&color, 1, 1);
			device->SetName(&helperTextures[HELPERTEXTURE_WHITE], "HELPERTEXTURE_WHITE");

			color = pf::Color::Transparent();
			CreateTexture(helperTextures[HELPERTEXTURE_TRANSPARENT], (const uint8_t*)&color, 1, 1);
			device->SetName(&helperTextures[HELPERTEXTURE_TRANSPARENT], "HELPERTEXTURE_TRANSPARENT");

			color = pf::Color(127, 127, 255, 255);
			CreateTexture(helperTextures[HELPERTEXTURE_NORMALMAPDEFAULT], (const uint8_t*)&color, 1, 1);
			device->SetName(&helperTextures[HELPERTEXTURE_NORMALMAPDEFAULT], "HELPERTEXTURE_NORMALMAPDEFAULT");
		}

		p_log("pf::texturehelper Initialized (%d ms)", (int)std::round(timer.elapsed()));

	}


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


	bool saveTextureToMemory(const pf::graphics::Texture& texture, pf::vector<uint8_t>& texturedata)
	{
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



	bool saveTextureToFile(const pf::graphics::Texture& texture, const std::string& fileName) {
		using namespace pf::graphics;
		TextureDesc desc = texture.GetDesc();
		pf::vector<uint8_t> data;
		if (saveTextureToMemory(texture, data))
		{
			return saveTextureToFile(data, desc, fileName);
		}
		return false;
	}

	std::string GetExtensionFromFileName(const std::string& filename)
	{
		size_t idx = filename.rfind('.');

		if (idx != std::string::npos)
		{
			std::string extension = filename.substr(idx + 1);
			return extension;
		}

		// No extension found
		return "";
	}

	bool saveTextureToMemoryFile(const pf::vector<uint8_t>& texturedata, const pf::graphics::TextureDesc& desc, const std::string& fileExtension, pf::vector<uint8_t>& filedata)
	{
		using namespace pf::graphics;
		const uint32_t data_stride = GetFormatStride(desc.format);

		std::string extension = pf::helper::toUpper(fileExtension);

		if (extension.compare("DDS") == 0)
		{
			filedata.resize(sizeof(dds::Header) + texturedata.size());
			dds::DXGI_FORMAT dds_format = dds::DXGI_FORMAT_UNKNOWN;
			switch (desc.format)
			{
			case pf::graphics::Format::R32G32B32A32_FLOAT:
				dds_format = dds::DXGI_FORMAT_R32G32B32A32_FLOAT;
				break;
			case pf::graphics::Format::R32G32B32A32_UINT:
				dds_format = dds::DXGI_FORMAT_R32G32B32A32_UINT;
				break;
			case pf::graphics::Format::R32G32B32A32_SINT:
				dds_format = dds::DXGI_FORMAT_R32G32B32A32_SINT;
				break;
			case pf::graphics::Format::R32G32B32_FLOAT:
				dds_format = dds::DXGI_FORMAT_R32G32B32_FLOAT;
				break;
			case pf::graphics::Format::R32G32B32_UINT:
				dds_format = dds::DXGI_FORMAT_R32G32B32_UINT;
				break;
			case pf::graphics::Format::R32G32B32_SINT:
				dds_format = dds::DXGI_FORMAT_R32G32B32_SINT;
				break;
			case pf::graphics::Format::R16G16B16A16_FLOAT:
				dds_format = dds::DXGI_FORMAT_R16G16B16A16_FLOAT;
				break;
			case pf::graphics::Format::R16G16B16A16_UNORM:
				dds_format = dds::DXGI_FORMAT_R16G16B16A16_UNORM;
				break;
			case pf::graphics::Format::R16G16B16A16_UINT:
				dds_format = dds::DXGI_FORMAT_R16G16B16A16_UINT;
				break;
			case pf::graphics::Format::R16G16B16A16_SNORM:
				dds_format = dds::DXGI_FORMAT_R16G16B16A16_SNORM;
				break;
			case pf::graphics::Format::R16G16B16A16_SINT:
				dds_format = dds::DXGI_FORMAT_R16G16B16A16_SINT;
				break;
			case pf::graphics::Format::R32G32_FLOAT:
				dds_format = dds::DXGI_FORMAT_R32G32_FLOAT;
				break;
			case pf::graphics::Format::R32G32_UINT:
				dds_format = dds::DXGI_FORMAT_R32G32_UINT;
				break;
			case pf::graphics::Format::R32G32_SINT:
				dds_format = dds::DXGI_FORMAT_R32G32_SINT;
				break;
			case pf::graphics::Format::R10G10B10A2_UNORM:
				dds_format = dds::DXGI_FORMAT_R10G10B10A2_UNORM;
				break;
			case pf::graphics::Format::R10G10B10A2_UINT:
				dds_format = dds::DXGI_FORMAT_R10G10B10A2_UINT;
				break;
			case pf::graphics::Format::R11G11B10_FLOAT:
				dds_format = dds::DXGI_FORMAT_R11G11B10_FLOAT;
				break;
			case pf::graphics::Format::R8G8B8A8_UNORM:
				dds_format = dds::DXGI_FORMAT_R8G8B8A8_UNORM;
				break;
			case pf::graphics::Format::R8G8B8A8_UNORM_SRGB:
				dds_format = dds::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
				break;
			case pf::graphics::Format::R8G8B8A8_UINT:
				dds_format = dds::DXGI_FORMAT_R8G8B8A8_UINT;
				break;
			case pf::graphics::Format::R8G8B8A8_SNORM:
				dds_format = dds::DXGI_FORMAT_R8G8B8A8_SNORM;
				break;
			case pf::graphics::Format::R8G8B8A8_SINT:
				dds_format = dds::DXGI_FORMAT_R8G8B8A8_SINT;
				break;
			case pf::graphics::Format::B8G8R8A8_UNORM:
				dds_format = dds::DXGI_FORMAT_B8G8R8A8_UNORM;
				break;
			case pf::graphics::Format::B8G8R8A8_UNORM_SRGB:
				dds_format = dds::DXGI_FORMAT_R16G16_SINT;
				break;
			case pf::graphics::Format::R16G16_FLOAT:
				dds_format = dds::DXGI_FORMAT_R16G16_FLOAT;
				break;
			case pf::graphics::Format::R16G16_UNORM:
				dds_format = dds::DXGI_FORMAT_R16G16_UNORM;
				break;
			case pf::graphics::Format::R16G16_UINT:
				dds_format = dds::DXGI_FORMAT_R16G16_UINT;
				break;
			case pf::graphics::Format::R16G16_SNORM:
				dds_format = dds::DXGI_FORMAT_R16G16_SNORM;
				break;
			case pf::graphics::Format::R16G16_SINT:
				dds_format = dds::DXGI_FORMAT_R16G16_SINT;
				break;
			case pf::graphics::Format::D32_FLOAT:
			case pf::graphics::Format::R32_FLOAT:
				dds_format = dds::DXGI_FORMAT_R32_FLOAT;
				break;
			case pf::graphics::Format::R32_UINT:
				dds_format = dds::DXGI_FORMAT_R32_UINT;
				break;
			case pf::graphics::Format::R32_SINT:
				dds_format = dds::DXGI_FORMAT_R32_SINT;
				break;
			case pf::graphics::Format::R9G9B9E5_SHAREDEXP:
				dds_format = dds::DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
				break;
			case pf::graphics::Format::R8G8_UNORM:
				dds_format = dds::DXGI_FORMAT_R8G8_UNORM;
				break;
			case pf::graphics::Format::R8G8_UINT:
				dds_format = dds::DXGI_FORMAT_R8G8_UINT;
				break;
			case pf::graphics::Format::R8G8_SNORM:
				dds_format = dds::DXGI_FORMAT_R8G8_SNORM;
				break;
			case pf::graphics::Format::R8G8_SINT:
				dds_format = dds::DXGI_FORMAT_R8G8_SINT;
				break;
			case pf::graphics::Format::R16_FLOAT:
				dds_format = dds::DXGI_FORMAT_R16_FLOAT;
				break;
			case pf::graphics::Format::D16_UNORM:
			case pf::graphics::Format::R16_UNORM:
				dds_format = dds::DXGI_FORMAT_R16_UNORM;
				break;
			case pf::graphics::Format::R16_UINT:
				dds_format = dds::DXGI_FORMAT_R16_UINT;
				break;
			case pf::graphics::Format::R16_SNORM:
				dds_format = dds::DXGI_FORMAT_R16_SNORM;
				break;
			case pf::graphics::Format::R16_SINT:
				dds_format = dds::DXGI_FORMAT_R16_SINT;
				break;
			case pf::graphics::Format::R8_UNORM:
				dds_format = dds::DXGI_FORMAT_R8_UNORM;
				break;
			case pf::graphics::Format::R8_UINT:
				dds_format = dds::DXGI_FORMAT_R8_UINT;
				break;
			case pf::graphics::Format::R8_SNORM:
				dds_format = dds::DXGI_FORMAT_R8_SNORM;
				break;
			case pf::graphics::Format::R8_SINT:
				dds_format = dds::DXGI_FORMAT_R8_SINT;
				break;
			case pf::graphics::Format::BC1_UNORM:
				dds_format = dds::DXGI_FORMAT_BC1_UNORM;
				break;
			case pf::graphics::Format::BC1_UNORM_SRGB:
				dds_format = dds::DXGI_FORMAT_BC1_UNORM_SRGB;
				break;
			case pf::graphics::Format::BC2_UNORM:
				dds_format = dds::DXGI_FORMAT_BC2_UNORM;
				break;
			case pf::graphics::Format::BC2_UNORM_SRGB:
				dds_format = dds::DXGI_FORMAT_BC2_UNORM_SRGB;
				break;
			case pf::graphics::Format::BC3_UNORM:
				dds_format = dds::DXGI_FORMAT_BC3_UNORM;
				break;
			case pf::graphics::Format::BC3_UNORM_SRGB:
				dds_format = dds::DXGI_FORMAT_BC3_UNORM_SRGB;
				break;
			case pf::graphics::Format::BC4_UNORM:
				dds_format = dds::DXGI_FORMAT_BC4_UNORM;
				break;
			case pf::graphics::Format::BC4_SNORM:
				dds_format = dds::DXGI_FORMAT_BC4_SNORM;
				break;
			case pf::graphics::Format::BC5_UNORM:
				dds_format = dds::DXGI_FORMAT_BC5_UNORM;
				break;
			case pf::graphics::Format::BC5_SNORM:
				dds_format = dds::DXGI_FORMAT_BC5_SNORM;
				break;
			case pf::graphics::Format::BC6H_UF16:
				dds_format = dds::DXGI_FORMAT_BC6H_UF16;
				break;
			case pf::graphics::Format::BC6H_SF16:
				dds_format = dds::DXGI_FORMAT_BC6H_SF16;
				break;
			case pf::graphics::Format::BC7_UNORM:
				dds_format = dds::DXGI_FORMAT_BC7_UNORM;
				break;
			case pf::graphics::Format::BC7_UNORM_SRGB:
				dds_format = dds::DXGI_FORMAT_BC7_UNORM_SRGB;
				break;
			default:
				assert(0);
				return false;
			}
			dds::write_header(
				filedata.data(),
				dds_format,
				desc.width,
				desc.type == TextureDesc::Type::TEXTURE_1D ? 0 : desc.height,
				desc.mip_levels,
				desc.array_size,
				has_flag(desc.misc_flags, ResourceMiscFlag::TEXTURECUBE),
				desc.type == TextureDesc::Type::TEXTURE_3D ? desc.depth : 0
			);
			std::memcpy(filedata.data() + sizeof(dds::Header), texturedata.data(), texturedata.size());
			return true;
		}

		const bool is_png = extension.compare("PNG") == 0;

		if (is_png)
		{
			if (desc.format == Format::R16_UNORM || desc.format == Format::R16_UINT)
			{
				// Specialized handling for 16-bit single channel PNG:
				pf::vector<uint8_t> src_bigendian = texturedata;
				uint16_t* dest = (uint16_t*)src_bigendian.data();
				for (uint32_t i = 0; i < desc.width * desc.height; ++i)
				{
					uint16_t r = dest[i];
					r = (r >> 8) | ((r & 0xFF) << 8); // little endian to big endian
					dest[i] = r;
				}
				unsigned error = lodepng::encode(filedata, src_bigendian, desc.width, desc.height, LCT_GREY, 16);
				return error == 0;
			}
			if (desc.format == Format::R16G16_UNORM || desc.format == Format::R16G16_UINT)
			{
				// Specialized handling for 16-bit PNG:
				//	Two channel RG data is expanded to RGBA (2-channel PNG is not good because that is interpreted as red and alpha)
				pf::vector<uint8_t> src_bigendian = texturedata;
				const uint32_t* src_rg = (const uint32_t*)src_bigendian.data();
				pf::vector<pf::Color16> dest_rgba(desc.width * desc.height);
				for (uint32_t i = 0; i < desc.width * desc.height; ++i)
				{
					uint32_t rg = src_rg[i];
					pf::Color16& rgba = dest_rgba[i];
					uint16_t r = rg & 0xFFFF;
					r = (r >> 8) | ((r & 0xFF) << 8); // little endian to big endian
					uint16_t g = (rg >> 16u) & 0xFFFF;
					g = (g >> 8) | ((g & 0xFF) << 8); // little endian to big endian
					rgba = pf::Color16(r, g, 0xFFFF, 0xFFFF);
				}
				unsigned error = lodepng::encode(filedata, (const unsigned char*)dest_rgba.data(), desc.width, desc.height, LCT_RGBA, 16);
				return error == 0;
			}
			if (desc.format == Format::R16G16B16A16_UNORM || desc.format == Format::R16G16B16A16_UINT)
			{
				// Specialized handling for 16-bit PNG:
				pf::vector<uint8_t> src_bigendian = texturedata;
				pf::Color16* dest = (pf::Color16*)src_bigendian.data();
				for (uint32_t i = 0; i < desc.width * desc.height; ++i)
				{
					pf::Color16 rgba = dest[i];
					uint16_t r = rgba.getR();
					r = (r >> 8) | ((r & 0xFF) << 8); // little endian to big endian
					uint16_t g = rgba.getG();
					g = (g >> 8) | ((g & 0xFF) << 8); // little endian to big endian
					uint16_t b = rgba.getB();
					b = (b >> 8) | ((b & 0xFF) << 8); // little endian to big endian
					uint16_t a = rgba.getA();
					a = (a >> 8) | ((a & 0xFF) << 8); // little endian to big endian
					rgba = pf::Color16(r, g, b, a);
					dest[i] = rgba;
				}
				unsigned error = lodepng::encode(filedata, src_bigendian, desc.width, desc.height, LCT_RGBA, 16);
				return error == 0;
			}
		}

		struct MipDesc
		{
			const uint8_t* address = nullptr;
			uint32_t width = 0;
			uint32_t height = 0;
			uint32_t depth = 0;
		};
		pf::vector<MipDesc> mips;
		mips.reserve(desc.mip_levels);

		uint32_t data_count = 0;
		uint32_t mip_width = desc.width;
		uint32_t mip_height = desc.height;
		uint32_t mip_depth = desc.depth;
		for (uint32_t mip = 0; mip < desc.mip_levels; ++mip)
		{
			MipDesc& mipdesc = mips.emplace_back();
			mipdesc.address = texturedata.data() + data_count * data_stride;
			data_count += mip_width * mip_height * mip_depth;
			mipdesc.width = mip_width;
			mipdesc.height = mip_height;
			mipdesc.depth = mip_depth;
			mip_width = std::max(1u, mip_width / 2);
			mip_height = std::max(1u, mip_height / 2);
			mip_depth = std::max(1u, mip_depth / 2);
		}

		int dst_channel_count = 4;
		if (desc.format == Format::R10G10B10A2_UNORM)
		{
			// This will be converted first to rgba8 before saving to common format:
			uint32_t* data32 = (uint32_t*)texturedata.data();

			for (uint32_t i = 0; i < data_count; ++i)
			{
				uint32_t pixel = data32[i];
				float r = ((pixel >> 0) & 1023) / 1023.0f;
				float g = ((pixel >> 10) & 1023) / 1023.0f;
				float b = ((pixel >> 20) & 1023) / 1023.0f;
				float a = ((pixel >> 30) & 3) / 3.0f;

				uint32_t rgba8 = 0;
				rgba8 |= (uint32_t)(r * 255.0f) << 0;
				rgba8 |= (uint32_t)(g * 255.0f) << 8;
				rgba8 |= (uint32_t)(b * 255.0f) << 16;
				rgba8 |= (uint32_t)(a * 255.0f) << 24;

				data32[i] = rgba8;
			}
		}
		else if (desc.format == Format::R32G32B32A32_FLOAT)
		{
			// This will be converted first to rgba8 before saving to common format:
			XMFLOAT4* dataSrc = (XMFLOAT4*)texturedata.data();
			uint32_t* data32 = (uint32_t*)texturedata.data();

			for (uint32_t i = 0; i < data_count; ++i)
			{
				XMFLOAT4 pixel = dataSrc[i];
				float r = std::max(0.0f, std::min(pixel.x, 1.0f));
				float g = std::max(0.0f, std::min(pixel.y, 1.0f));
				float b = std::max(0.0f, std::min(pixel.z, 1.0f));
				float a = std::max(0.0f, std::min(pixel.w, 1.0f));

				uint32_t rgba8 = 0;
				rgba8 |= (uint32_t)(r * 255.0f) << 0;
				rgba8 |= (uint32_t)(g * 255.0f) << 8;
				rgba8 |= (uint32_t)(b * 255.0f) << 16;
				rgba8 |= (uint32_t)(a * 255.0f) << 24;

				data32[i] = rgba8;
			}
		}
		else if (desc.format == Format::R16G16B16A16_FLOAT)
		{
			// This will be converted first to rgba8 before saving to common format:
			XMHALF4* dataSrc = (XMHALF4*)texturedata.data();
			uint32_t* data32 = (uint32_t*)texturedata.data();

			for (uint32_t i = 0; i < data_count; ++i)
			{
				XMHALF4 pixel = dataSrc[i];
				float r = std::max(0.0f, std::min(XMConvertHalfToFloat(pixel.x), 1.0f));
				float g = std::max(0.0f, std::min(XMConvertHalfToFloat(pixel.y), 1.0f));
				float b = std::max(0.0f, std::min(XMConvertHalfToFloat(pixel.z), 1.0f));
				float a = std::max(0.0f, std::min(XMConvertHalfToFloat(pixel.w), 1.0f));

				uint32_t rgba8 = 0;
				rgba8 |= (uint32_t)(r * 255.0f) << 0;
				rgba8 |= (uint32_t)(g * 255.0f) << 8;
				rgba8 |= (uint32_t)(b * 255.0f) << 16;
				rgba8 |= (uint32_t)(a * 255.0f) << 24;

				data32[i] = rgba8;
			}
		}
		else if (desc.format == Format::R16G16B16A16_UNORM || desc.format == Format::R16G16B16A16_UINT)
		{
			// This will be converted first to rgba8 before saving to common format:
			pf::Color16* dataSrc = (pf::Color16*)texturedata.data();
			pf::Color* data32 = (pf::Color*)texturedata.data();

			for (uint32_t i = 0; i < data_count; ++i)
			{
				pf::Color16 pixel16 = dataSrc[i];
				data32[i] = pf::Color::fromFloat4(pixel16.toFloat4());
			}
		}
		else if (desc.format == Format::R11G11B10_FLOAT)
		{
			// This will be converted first to rgba8 before saving to common format:
			XMFLOAT3PK* dataSrc = (XMFLOAT3PK*)texturedata.data();
			uint32_t* data32 = (uint32_t*)texturedata.data();

			for (uint32_t i = 0; i < data_count; ++i)
			{
				XMFLOAT3PK pixel = dataSrc[i];
				XMVECTOR V = XMLoadFloat3PK(&pixel);
				XMFLOAT3 pixel3;
				XMStoreFloat3(&pixel3, V);
				float r = std::max(0.0f, std::min(pixel3.x, 1.0f));
				float g = std::max(0.0f, std::min(pixel3.y, 1.0f));
				float b = std::max(0.0f, std::min(pixel3.z, 1.0f));
				float a = 1;

				uint32_t rgba8 = 0;
				rgba8 |= (uint32_t)(r * 255.0f) << 0;
				rgba8 |= (uint32_t)(g * 255.0f) << 8;
				rgba8 |= (uint32_t)(b * 255.0f) << 16;
				rgba8 |= (uint32_t)(a * 255.0f) << 24;

				data32[i] = rgba8;
			}
		}
		else if (desc.format == Format::R9G9B9E5_SHAREDEXP)
		{
			// This will be converted first to rgba8 before saving to common format:
			XMFLOAT3SE* dataSrc = (XMFLOAT3SE*)texturedata.data();
			uint32_t* data32 = (uint32_t*)texturedata.data();

			for (uint32_t i = 0; i < data_count; ++i)
			{
				XMFLOAT3SE pixel = dataSrc[i];
				XMVECTOR V = XMLoadFloat3SE(&pixel);
				XMFLOAT3 pixel3;
				XMStoreFloat3(&pixel3, V);
				float r = std::max(0.0f, std::min(pixel3.x, 1.0f));
				float g = std::max(0.0f, std::min(pixel3.y, 1.0f));
				float b = std::max(0.0f, std::min(pixel3.z, 1.0f));
				float a = 1;

				uint32_t rgba8 = 0;
				rgba8 |= (uint32_t)(r * 255.0f) << 0;
				rgba8 |= (uint32_t)(g * 255.0f) << 8;
				rgba8 |= (uint32_t)(b * 255.0f) << 16;
				rgba8 |= (uint32_t)(a * 255.0f) << 24;

				data32[i] = rgba8;
			}
		}
		else if (desc.format == Format::B8G8R8A8_UNORM || desc.format == Format::B8G8R8A8_UNORM_SRGB)
		{
			// This will be converted first to rgba8 before saving to common format:
			uint32_t* data32 = (uint32_t*)texturedata.data();

			for (uint32_t i = 0; i < data_count; ++i)
			{
				uint32_t pixel = data32[i];
				uint8_t b = (pixel >> 0u) & 0xFF;
				uint8_t g = (pixel >> 8u) & 0xFF;
				uint8_t r = (pixel >> 16u) & 0xFF;
				uint8_t a = (pixel >> 24u) & 0xFF;
				data32[i] = r | (g << 8u) | (b << 16u) | (a << 24u);
			}
		}
		else if (desc.format == Format::R8_UNORM)
		{
			// This can be saved by reducing target channel count, no conversion needed
			dst_channel_count = 1;
		}
		else if (desc.format == Format::R8G8_UNORM)
		{
			// This can be saved by reducing target channel count, no conversion needed
			dst_channel_count = 2;
		}
		else
		{
			assert(desc.format == Format::R8G8B8A8_UNORM || desc.format == Format::R8G8B8A8_UNORM_SRGB); // If you need to save other texture format, implement data conversion for it
		}

		if (!extension.compare("ICO"))
		{
			const uint32_t minsize = 32;

			size_t filesize = sizeof(ico::ICONDIR);

			ico::ICONDIR icondir = { 0,1,0 };

			for (auto& mip : mips)
			{
				if (mip.width > 256 || mip.height > 256)
					continue;
				if (mip.width < minsize || mip.height < minsize)
					break;
				icondir.idCount++;
				filesize += sizeof(ico::ICONDIRENTRY);
			}

			if (icondir.idCount < 1)
			{
				//plog_assert(0, "No valid images were found that can be added to ICO file format!");
				return false;
			}

			uint32_t imageDataOffset = (uint32_t)filesize;

			for (auto& mip : mips)
			{
				if (mip.width > 256 || mip.height > 256)
					continue;
				if (mip.width < minsize || mip.height < minsize)
					break;
				const uint32_t pixelCount = mip.width * mip.height;
				const uint32_t rgbDataSize = pixelCount * 4; // 32-bit RGBA
				const uint32_t maskSize = ((mip.width + 7) / 8) * mip.height; // 1-bit mask, padded to byte
				const uint32_t imageDataSize = sizeof(ico::BITMAPINFOHEADER) + rgbDataSize + maskSize;
				filesize += imageDataSize;
			}

			filedata.resize(filesize);
			uint8_t* ptr = filedata.data();

			std::memcpy(ptr, &icondir, sizeof(ico::ICONDIR));
			ptr += sizeof(ico::ICONDIR);

			for (auto& mip : mips)
			{
				if (mip.width > 256 || mip.height > 256)
					continue;
				if (mip.width < minsize || mip.height < minsize)
					break;

				const uint32_t pixelCount = mip.width * mip.height;
				const uint32_t rgbDataSize = pixelCount * 4; // 32-bit RGBA
				const uint32_t maskSize = ((mip.width + 7) / 8) * mip.height; // 1-bit mask, padded to byte
				const uint32_t imageDataSize = sizeof(ico::BITMAPINFOHEADER) + rgbDataSize + maskSize;

				ico::ICONDIRENTRY iconEntry = {
					static_cast<uint8_t>(mip.width > 255 ? 0 : mip.width), // Width (0 for 256+)
					static_cast<uint8_t>(mip.height > 255 ? 0 : mip.height), // Height (0 for 256+)
					0, // Color count (0 for 32-bit)
					0, // Reserved
					1, // Color planes
					32, // Bits per pixel
					imageDataSize, // Size of image data
					imageDataOffset // Offset to image data
				};
				std::memcpy(ptr, &iconEntry, sizeof(ico::ICONDIRENTRY));
				ptr += sizeof(ico::ICONDIRENTRY);
				imageDataOffset += imageDataSize;
			}

			for (auto& mip : mips)
			{
				if (mip.width > 256 || mip.height > 256)
					continue;
				if (mip.width < minsize || mip.height < minsize)
					break;

				const uint32_t pixelCount = mip.width * mip.height;
				const uint32_t rgbDataSize = pixelCount * 4; // 32-bit RGBA
				const uint32_t maskSize = ((mip.width + 7) / 8) * mip.height; // 1-bit mask, padded to byte

				ico::BITMAPINFOHEADER bmpHeader = {
					sizeof(ico::BITMAPINFOHEADER), // Size of header
					int32_t(mip.width), // Width
					int32_t(mip.height * 2), // Height (doubled for XOR + AND mask)
					1, // Planes
					32, // Bits per pixel
					0, // No compression
					rgbDataSize + maskSize, // Image size
					0, // X pixels per meter
					0, // Y pixels per meter
					0, // Colors used
					0  // Important colors
				};
				std::memcpy(ptr, &bmpHeader, sizeof(ico::BITMAPINFOHEADER));
				ptr += sizeof(ico::BITMAPINFOHEADER);

				// Convert RGBA to BGRA and write XOR mask (flipped vertically)
				for (uint32_t y = mip.height; y > 0; --y)
				{
					const uint8_t* src = mip.address + (y - 1) * mip.width * 4;
					for (uint32_t x = 0; x < mip.width; ++x)
					{
						// Convert RGBA to BGRA
						ptr[0] = src[2]; // B
						ptr[1] = src[1]; // G
						ptr[2] = src[0]; // R
						ptr[3] = src[3]; // A
						ptr += 4;
						src += 4;
					}
				}

				// Write AND mask (1-bit transparency mask)
				for (uint32_t y = mip.height; y > 0; --y)
				{
					const uint8_t* src = mip.address + (y - 1) * mip.width * 4;
					for (uint32_t x = 0; x < mip.width; x += 8)
					{
						uint8_t maskByte = 0;
						for (uint32_t bit = 0; bit < 8 && (x + bit) < mip.width; ++bit)
						{
							// Set bit to 0 if pixel is opaque (alpha > 0), 1 if transparent
							if (src[(x + bit) * 4 + 3] == 0)
							{
								maskByte |= (1 << (7 - bit));
							}
						}
						*ptr++ = maskByte;
					}
				}
			}

			return true;
		}

		int write_result = 0;

		filedata.clear();
		stbi_write_func* func = [](void* context, void* data, int size) {
			pf::vector<uint8_t>& filedata = *(pf::vector<uint8_t>*)context;
			for (int i = 0; i < size; ++i)
			{
				filedata.push_back(*((uint8_t*)data + i));
			}
		};

		static int mip_request = 0; // you can use this while debugging to write specific mip level to file (todo: option param?)
		const MipDesc& mip = mips[mip_request];

		if (is_png)
		{
			write_result = stbi_write_png_to_func(func, &filedata, (int)mip.width, (int)mip.height, dst_channel_count, mip.address, 0);
		}
		else if (!extension.compare("JPG") || !extension.compare("JPEG"))
		{
			write_result = stbi_write_jpg_to_func(func, &filedata, (int)mip.width, (int)mip.height, dst_channel_count, mip.address, 100);
		}
		else if (!extension.compare("TGA"))
		{
			write_result = stbi_write_tga_to_func(func, &filedata, (int)mip.width, (int)mip.height, dst_channel_count, mip.address);
		}
		else if (!extension.compare("BMP"))
		{
			write_result = stbi_write_bmp_to_func(func, &filedata, (int)mip.width, (int)mip.height, dst_channel_count, mip.address);
		}
		else if (!extension.compare("H"))
		{
			const size_t datasize = mip.width * mip.height * sizeof(pf::Color);
			std::string ss;
			ss += "struct EmbeddedImage {\n";
			ss += "const unsigned int width = " + std::to_string(mip.width) + ";\n";
			ss += "const unsigned int height = " + std::to_string(mip.height) + ";\n";
			ss += "const unsigned int bytes_per_pixel = 4;\n";
			ss += "const char comment[256] = \"RGBA Image Header Generated By Wicked Engine\";\n";
			ss += "const unsigned char pixel_data[" + std::to_string(mip.width) + " * " + std::to_string(mip.height) + " * 4] = {";
			for (size_t i = 0; i < datasize; ++i)
			{
				if (i % 32 == 0)
				{
					ss += "\n";
				}
				ss += std::to_string((uint32_t)mip.address[i]) + ",";
			}
			ss += "\n};\n};\n";
			filedata.resize(ss.size());
			std::memcpy(filedata.data(), ss.c_str(), ss.length());
			return true;
		}
		else if (!extension.compare("RAW"))
		{
			filedata.resize(mip.width * mip.height * sizeof(pf::Color));
			std::memcpy(filedata.data(), mip.address, filedata.size());
			return true;
		}
		else
		{
			assert(0 && "Unsupported extension");
		}

		return write_result != 0;
	}

	bool saveTextureToFile(const pf::vector<uint8_t>& texturedata, const pf::graphics::TextureDesc& desc, const std::string& fileName)
	{
		using namespace pf::graphics;

		std::string ext = GetExtensionFromFileName(fileName);
		pf::vector<uint8_t> filedata;
		if (saveTextureToMemoryFile(texturedata, desc, ext, filedata))
		{
			return pf::helper::FileWrite(fileName, filedata.data(), filedata.size());
		}

		return false;
	}

	Texture CreateGradientTexture(
		GradientType type,
		uint32_t width,
		uint32_t height,
		const XMFLOAT2& uv_start,
		const XMFLOAT2& uv_end,
		GradientFlags flags,
		Swizzle swizzle,
		float perlin_scale,
		uint32_t perlin_seed,
		int perlin_octaves,
		float perlin_persistence
	)
	{
		pf::vector<uint8_t> data;
		pf::vector<uint16_t> data16;
		if (has_flag(flags, GradientFlags::R16Unorm))
		{
			data16.resize(width * height);
		}
		else
		{
			data.resize(width * height);
		}
		pf::noise::Perlin perlin;
		if (has_flag(flags, GradientFlags::PerlinNoise))
		{
			perlin.init(perlin_seed);
		}
		float aspect = float(height) / float(width);
		XMFLOAT2 perlin_scale2 = XMFLOAT2(perlin_scale, perlin_scale * aspect);

		switch (type)
		{
		default:
		case GradientType::Linear:
		{
			const XMVECTOR a = XMLoadFloat2(&uv_start);
			const XMVECTOR b = XMLoadFloat2(&uv_end);
			const float distance = XMVectorGetX(XMVector3Length(b - a));
			for (uint32_t y = 0; y < height; ++y)
			{
				for (uint32_t x = 0; x < width; ++x)
				{
					const XMFLOAT2 uv = XMFLOAT2((float(x) + 0.5f) / float(width), (float(y) + 0.5f) / float(height));
					const XMVECTOR point_on_line = pf::math::ClosestPointOnLineSegment(a, b, XMLoadFloat2(&uv));
					const float uv_distance = XMVectorGetX(XMVector3Length(point_on_line - a));
					float gradient = saturate(pf::math::InverseLerp(0, distance, uv_distance));
					if (has_flag(flags, GradientFlags::Inverse))
					{
						gradient = 1 - gradient;
					}
					if (has_flag(flags, GradientFlags::Smoothstep))
					{
						gradient = pf::math::SmoothStep(0, 1, gradient);
					}
					if (has_flag(flags, GradientFlags::PerlinNoise))
					{
						gradient *= perlin.compute(uv.x * perlin_scale2.x, uv.y * perlin_scale2.y, 0, perlin_octaves, perlin_persistence) * 0.5f + 0.5f;
					}
					gradient = saturate(gradient);
					if (has_flag(flags, GradientFlags::R16Unorm))
					{
						data16[x + y * width] = uint16_t(gradient * 65535);
					}
					else
					{
						data[x + y * width] = uint8_t(gradient * 255);
					}
				}
			}
		}
		break;

		case GradientType::Circular:
		{
			const XMVECTOR a = XMLoadFloat2(&uv_start);
			const XMVECTOR b = XMLoadFloat2(&uv_end);
			const float distance = XMVectorGetX(XMVector3Length(b - a));
			for (uint32_t y = 0; y < height; ++y)
			{
				for (uint32_t x = 0; x < width; ++x)
				{
					const XMFLOAT2 uv = XMFLOAT2((float(x) + 0.5f) / float(width), (float(y) + 0.5f) / float(height));
					const float uv_distance = pf::math::Clamp(XMVectorGetX(XMVector3Length(XMLoadFloat2(&uv) - a)), 0, distance);
					float gradient = saturate(pf::math::InverseLerp(0, distance, uv_distance));
					if (has_flag(flags, GradientFlags::Inverse))
					{
						gradient = 1 - gradient;
					}
					if (has_flag(flags, GradientFlags::Smoothstep))
					{
						gradient = pf::math::SmoothStep(0, 1, gradient);
					}
					if (has_flag(flags, GradientFlags::PerlinNoise))
					{
						gradient *= perlin.compute(uv.x * perlin_scale2.x, uv.y * perlin_scale2.y, 0, perlin_octaves, perlin_persistence) * 0.5f + 0.5f;
					}
					gradient = saturate(gradient);
					if (has_flag(flags, GradientFlags::R16Unorm))
					{
						data16[x + y * width] = uint16_t(gradient * 65535);
					}
					else
					{
						data[x + y * width] = uint8_t(gradient * 255);
					}
				}
			}
		}
		break;

		case GradientType::Angular:
		{
			XMFLOAT2 direction;
			XMStoreFloat2(&direction, XMVector2Normalize(XMLoadFloat2(&uv_end) - XMLoadFloat2(&uv_start)));
			for (uint32_t y = 0; y < height; ++y)
			{
				for (uint32_t x = 0; x < width; ++x)
				{
					const XMFLOAT2 uv = XMFLOAT2((float(x) + 0.5f) / float(width), (float(y) + 0.5f) / float(height));
					const XMFLOAT2 coord = XMFLOAT2(uv.x - uv_start.x, uv.y - uv_start.y);
					float gradient = pf::math::GetAngle(direction, coord) / XM_2PI;
					if (has_flag(flags, GradientFlags::Inverse))
					{
						gradient = 1 - gradient;
					}
					if (has_flag(flags, GradientFlags::Smoothstep))
					{
						gradient = pf::math::SmoothStep(0, 1, gradient);
					}
					if (has_flag(flags, GradientFlags::PerlinNoise))
					{
						gradient *= perlin.compute(uv.x * perlin_scale2.x, uv.y * perlin_scale2.y, 0, perlin_octaves, perlin_persistence) * 0.5f + 0.5f;
					}
					gradient = saturate(gradient);
					if (has_flag(flags, GradientFlags::R16Unorm))
					{
						data16[x + y * width] = uint16_t(gradient * 65535);
					}
					else
					{
						data[x + y * width] = uint8_t(gradient * 255);
					}
				}
			}
		}
		break;

		}

		Texture texture;
		if (has_flag(flags, GradientFlags::R16Unorm))
		{
			CreateTexture(texture, (const uint8_t*)data16.data(), width, height, Format::R16_UNORM, swizzle);
		}
		else
		{
			CreateTexture(texture, data.data(), width, height, Format::R8_UNORM, swizzle);
		}
		return texture;
	}


	pf::graphics::Texture CreateLensDistortionNormalMap(
		uint32_t width,
		uint32_t height,
		const XMFLOAT2& uv_start,
		float radius,
		float squish,
		float blend,
		float edge_smoothness
	)
	{
		XMFLOAT2 offset = XMFLOAT2(uv_start.x * 2 - 1, uv_start.y * 2 - 1);
		float scale = 1.0f / (radius * 2);
		float edge = 1.0f - edge_smoothness;

		pf::vector<uint32_t> data(width * height);
		for (uint32_t y = 0; y < height; ++y)
		{
			for (uint32_t x = 0; x < width; ++x)
			{
				XMFLOAT2 uv = XMFLOAT2(float(x) / float(width - 1) * 2 - 1, float(y) / float(height - 1) * 2 - 1);

				uv.x -= offset.x;
				uv.y -= offset.y;
				uv.x *= scale;
				uv.y *= scale;

				if (width > height)
					uv.x *= float(width) / float(height);
				else
					uv.y *= float(height) / float(width);

				const float d = pf::math::Length(uv);
				const float dp = std::pow(saturate(d), squish);
				uv.x = uv.x * dp;
				uv.y = uv.y * dp;

				XMFLOAT2 color = XMFLOAT2(uv.x * 0.5f + 0.5f, uv.y * 0.5f + 0.5f);
				float s = smoothstep(1.0f, edge, d) * blend;
				color.x = lerp(0.5f, color.x, s);
				color.y = lerp(0.5f, color.y, s);
				data[x + y * width] = (uint32_t(color.x * 65535) & 0xFFFF) | (uint32_t(color.y * 65535) & 0xFFFF) << 16u;
			}
		}

		Texture texture;
		CreateTexture(texture, data.data(), width, height, Format::R16G16_UNORM);
		return texture;
	}

	const Texture* getLogo()
	{
		return &helperTextures[HELPERTEXTURE_LOGO];
	}
	const Texture* getRandom64x64()
	{
		return &helperTextures[HELPERTEXTURE_RANDOM64X64];
	}
	const Texture* getColorGradeDefault()
	{
		return &helperTextures[HELPERTEXTURE_COLORGRADEDEFAULT];
	}
	const Texture* getNormalMapDefault()
	{
		return &helperTextures[HELPERTEXTURE_NORMALMAPDEFAULT];
	}
	const Texture* getBlackCubeMap()
	{
		return &helperTextures[HELPERTEXTURE_BLACKCUBEMAP];
	}
	const Texture* getUINT4()
	{
		return &helperTextures[HELPERTEXTURE_UINT4];
	}
	const Texture* getBlueNoise()
	{
		return &helperTextures[HELPERTEXTURE_BLUENOISE];
	}
	const Texture* getWaterRipple()
	{
		return &helperTextures[HELPERTEXTURE_WATERRIPPLE];
	}
	const Texture* getCheckerBoard()
	{
		return &helperTextures[HELPERTEXTURE_CHECKERBOARD];
	}
	const Texture* getWhite()
	{
		return &helperTextures[HELPERTEXTURE_WHITE];
	}
	const Texture* getBlack()
	{
		return &helperTextures[HELPERTEXTURE_BLACK];
	}
	const Texture* getTransparent()
	{
		return &helperTextures[HELPERTEXTURE_TRANSPARENT];
	}
}