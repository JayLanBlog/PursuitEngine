#include "vulkan_driver.h"
#include <Module/Util/p_timer.h>
#include <Module/Math/pf_math.h>
#include <Core/pf_version.h>
#include <Utility/h264.h>

PFN_vkCmdBeginVideoCodingKHR pfnCmdBeginVideoCodingKHR = nullptr;
PFN_vkCmdEndVideoCodingKHR pfnCmdEndVideoCodingKHR = nullptr;
PFN_vkCmdControlVideoCodingKHR pfnCmdControlVideoCodingKHR = nullptr;

namespace pf::graphics {
	
	namespace vulkan_internal {
		static constexpr uint64_t timeout_value = 2000000000ull; // 2 seconds

		// These shifts are made so that Vulkan resource bindings slots don't interfere with each other across shader stages:
		//	These are also defined in wi::shadercompiler.cpp as hard coded compiler arguments for SPIRV, so they need to be the same
		enum
		{
			VULKAN_BINDING_SHIFT_B = 0,
			VULKAN_BINDING_SHIFT_T = 1000,
			VULKAN_BINDING_SHIFT_U = 2000,
			VULKAN_BINDING_SHIFT_S = 3000,
		};

		// Converters:
		constexpr VkFormat _ConvertFormat(Format value) {
			switch (value)
			{
			case Format::UNKNOWN:
				return VK_FORMAT_UNDEFINED;
			case Format::R32G32B32A32_FLOAT:
				return VK_FORMAT_R32G32B32A32_SFLOAT;
			case Format::R32G32B32A32_UINT:
				return VK_FORMAT_R32G32B32A32_UINT;
			case Format::R32G32B32A32_SINT:
				return VK_FORMAT_R32G32B32A32_SINT;
			case Format::R32G32B32_FLOAT:
				return VK_FORMAT_R32G32B32_SFLOAT;
			case Format::R32G32B32_UINT:
				return VK_FORMAT_R32G32B32_UINT;
			case Format::R32G32B32_SINT:
				return VK_FORMAT_R32G32B32_SINT;
			case Format::R16G16B16A16_FLOAT:
				return VK_FORMAT_R16G16B16A16_SFLOAT;
			case Format::R16G16B16A16_UNORM:
				return VK_FORMAT_R16G16B16A16_UNORM;
			case Format::R16G16B16A16_UINT:
				return VK_FORMAT_R16G16B16A16_UINT;
			case Format::R16G16B16A16_SNORM:
				return VK_FORMAT_R16G16B16A16_SNORM;
			case Format::R16G16B16A16_SINT:
				return VK_FORMAT_R16G16B16A16_SINT;
			case Format::R32G32_FLOAT:
				return VK_FORMAT_R32G32_SFLOAT;
			case Format::R32G32_UINT:
				return VK_FORMAT_R32G32_UINT;
			case Format::R32G32_SINT:
				return VK_FORMAT_R32G32_SINT;
			case Format::D32_FLOAT_S8X24_UINT:
				return VK_FORMAT_D32_SFLOAT_S8_UINT;
			case Format::R10G10B10A2_UNORM:
				return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
			case Format::R10G10B10A2_UINT:
				return VK_FORMAT_A2B10G10R10_UINT_PACK32;
			case Format::R11G11B10_FLOAT:
				return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
			case Format::R8G8B8A8_UNORM:
				return VK_FORMAT_R8G8B8A8_UNORM;
			case Format::R8G8B8A8_UNORM_SRGB:
				return VK_FORMAT_R8G8B8A8_SRGB;
			case Format::R8G8B8A8_UINT:
				return VK_FORMAT_R8G8B8A8_UINT;
			case Format::R8G8B8A8_SNORM:
				return VK_FORMAT_R8G8B8A8_SNORM;
			case Format::R8G8B8A8_SINT:
				return VK_FORMAT_R8G8B8A8_SINT;
			case Format::R16G16_FLOAT:
				return VK_FORMAT_R16G16_SFLOAT;
			case Format::R16G16_UNORM:
				return VK_FORMAT_R16G16_UNORM;
			case Format::R16G16_UINT:
				return VK_FORMAT_R16G16_UINT;
			case Format::R16G16_SNORM:
				return VK_FORMAT_R16G16_SNORM;
			case Format::R16G16_SINT:
				return VK_FORMAT_R16G16_SINT;
			case Format::D32_FLOAT:
				return VK_FORMAT_D32_SFLOAT;
			case Format::R32_FLOAT:
				return VK_FORMAT_R32_SFLOAT;
			case Format::R32_UINT:
				return VK_FORMAT_R32_UINT;
			case Format::R32_SINT:
				return VK_FORMAT_R32_SINT;
			case Format::D24_UNORM_S8_UINT:
				return VK_FORMAT_D24_UNORM_S8_UINT;
			case Format::R9G9B9E5_SHAREDEXP:
				return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;
			case Format::R8G8_UNORM:
				return VK_FORMAT_R8G8_UNORM;
			case Format::R8G8_UINT:
				return VK_FORMAT_R8G8_UINT;
			case Format::R8G8_SNORM:
				return VK_FORMAT_R8G8_SNORM;
			case Format::R8G8_SINT:
				return VK_FORMAT_R8G8_SINT;
			case Format::R16_FLOAT:
				return VK_FORMAT_R16_SFLOAT;
			case Format::D16_UNORM:
				return VK_FORMAT_D16_UNORM;
			case Format::R16_UNORM:
				return VK_FORMAT_R16_UNORM;
			case Format::R16_UINT:
				return VK_FORMAT_R16_UINT;
			case Format::R16_SNORM:
				return VK_FORMAT_R16_SNORM;
			case Format::R16_SINT:
				return VK_FORMAT_R16_SINT;
			case Format::R8_UNORM:
				return VK_FORMAT_R8_UNORM;
			case Format::R8_UINT:
				return VK_FORMAT_R8_UINT;
			case Format::R8_SNORM:
				return VK_FORMAT_R8_SNORM;
			case Format::R8_SINT:
				return VK_FORMAT_R8_SINT;
			case Format::BC1_UNORM:
				return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
			case Format::BC1_UNORM_SRGB:
				return VK_FORMAT_BC1_RGBA_SRGB_BLOCK;
			case Format::BC2_UNORM:
				return VK_FORMAT_BC2_UNORM_BLOCK;
			case Format::BC2_UNORM_SRGB:
				return VK_FORMAT_BC2_SRGB_BLOCK;
			case Format::BC3_UNORM:
				return VK_FORMAT_BC3_UNORM_BLOCK;
			case Format::BC3_UNORM_SRGB:
				return VK_FORMAT_BC3_SRGB_BLOCK;
			case Format::BC4_UNORM:
				return VK_FORMAT_BC4_UNORM_BLOCK;
			case Format::BC4_SNORM:
				return VK_FORMAT_BC4_SNORM_BLOCK;
			case Format::BC5_UNORM:
				return VK_FORMAT_BC5_UNORM_BLOCK;
			case Format::BC5_SNORM:
				return VK_FORMAT_BC5_SNORM_BLOCK;
			case Format::B8G8R8A8_UNORM:
				return VK_FORMAT_B8G8R8A8_UNORM;
			case Format::B8G8R8A8_UNORM_SRGB:
				return VK_FORMAT_B8G8R8A8_SRGB;
			case Format::BC6H_UF16:
				return VK_FORMAT_BC6H_UFLOAT_BLOCK;
			case Format::BC6H_SF16:
				return VK_FORMAT_BC6H_SFLOAT_BLOCK;
			case Format::BC7_UNORM:
				return VK_FORMAT_BC7_UNORM_BLOCK;
			case Format::BC7_UNORM_SRGB:
				return VK_FORMAT_BC7_SRGB_BLOCK;
			case Format::NV12:
				return VK_FORMAT_G8_B8R8_2PLANE_420_UNORM;
			}
			return VK_FORMAT_UNDEFINED;
		}

		constexpr VkCompareOp _ConvertComparisonFunc(ComparisonFunc value) {
			switch (value)
			{
			case ComparisonFunc::NEVER:
				return VK_COMPARE_OP_NEVER;
			case ComparisonFunc::LESS:
				return VK_COMPARE_OP_LESS;
			case ComparisonFunc::EQUAL:
				return VK_COMPARE_OP_EQUAL;
			case ComparisonFunc::LESS_EQUAL:
				return VK_COMPARE_OP_LESS_OR_EQUAL;
			case ComparisonFunc::GREATER:
				return VK_COMPARE_OP_GREATER;
			case ComparisonFunc::NOT_EQUAL:
				return VK_COMPARE_OP_NOT_EQUAL;
			case ComparisonFunc::GREATER_EQUAL:
				return VK_COMPARE_OP_GREATER_OR_EQUAL;
			case ComparisonFunc::ALWAYS:
				return VK_COMPARE_OP_ALWAYS;
			default:
				return VK_COMPARE_OP_NEVER;
			}
		}

		constexpr VkBlendFactor _ConvertBlend(Blend value)
		{
			switch (value)
			{
			case Blend::ZERO:
				return VK_BLEND_FACTOR_ZERO;
			case Blend::ONE:
				return VK_BLEND_FACTOR_ONE;
			case Blend::SRC_COLOR:
				return VK_BLEND_FACTOR_SRC_COLOR;
			case Blend::INV_SRC_COLOR:
				return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
			case Blend::SRC_ALPHA:
				return VK_BLEND_FACTOR_SRC_ALPHA;
			case Blend::INV_SRC_ALPHA:
				return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			case Blend::DEST_ALPHA:
				return VK_BLEND_FACTOR_DST_ALPHA;
			case Blend::INV_DEST_ALPHA:
				return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
			case Blend::DEST_COLOR:
				return VK_BLEND_FACTOR_DST_COLOR;
			case Blend::INV_DEST_COLOR:
				return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
			case Blend::SRC_ALPHA_SAT:
				return VK_BLEND_FACTOR_SRC_ALPHA_SATURATE;
			case Blend::BLEND_FACTOR:
				return VK_BLEND_FACTOR_CONSTANT_COLOR;
			case Blend::INV_BLEND_FACTOR:
				return VK_BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR;
				break;
			case Blend::SRC1_COLOR:
				return VK_BLEND_FACTOR_SRC1_COLOR;
			case Blend::INV_SRC1_COLOR:
				return VK_BLEND_FACTOR_ONE_MINUS_SRC1_COLOR;
			case Blend::SRC1_ALPHA:
				return VK_BLEND_FACTOR_SRC1_ALPHA;
			case Blend::INV_SRC1_ALPHA:
				return VK_BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA;
			default:
				return VK_BLEND_FACTOR_ZERO;
			}
		}

		constexpr VkBlendOp _ConvertBlendOp(BlendOp value)
		{
			switch (value)
			{
			case BlendOp::ADD:
				return VK_BLEND_OP_ADD;
			case BlendOp::SUBTRACT:
				return VK_BLEND_OP_SUBTRACT;
			case BlendOp::REV_SUBTRACT:
				return VK_BLEND_OP_REVERSE_SUBTRACT;
			case BlendOp::MIN:
				return VK_BLEND_OP_MIN;
			case BlendOp::MAX:
				return VK_BLEND_OP_MAX;
			default:
				return VK_BLEND_OP_ADD;
			}
		}

		constexpr VkSamplerAddressMode _ConvertTextureAddressMode(TextureAddressMode value, const VkPhysicalDeviceVulkan12Features& features_1_2)
		{
			switch (value)
			{
			case TextureAddressMode::WRAP:
				return VK_SAMPLER_ADDRESS_MODE_REPEAT;
			case TextureAddressMode::MIRROR:
				return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			case TextureAddressMode::CLAMP:
				return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			case TextureAddressMode::BORDER:
				return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
			case TextureAddressMode::MIRROR_ONCE:
				if (features_1_2.samplerMirrorClampToEdge == VK_TRUE)
				{
					return VK_SAMPLER_ADDRESS_MODE_MIRROR_CLAMP_TO_EDGE;
				}
				return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
			default:
				return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
			}
		}
		constexpr VkBorderColor _ConvertSamplerBorderColor(SamplerBorderColor value)
		{
			switch (value)
			{
			case SamplerBorderColor::TRANSPARENT_BLACK:
				return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
			case SamplerBorderColor::OPAQUE_BLACK:
				return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
			case SamplerBorderColor::OPAQUE_WHITE:
				return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
			default:
				return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
			}
		}

		constexpr VkStencilOp _ConvertStencilOp(StencilOp value)
		{
			switch (value)
			{
			case graphics::StencilOp::KEEP:
				return VK_STENCIL_OP_KEEP;
			case graphics::StencilOp::ZERO:
				return VK_STENCIL_OP_ZERO;
			case graphics::StencilOp::REPLACE:
				return VK_STENCIL_OP_REPLACE;
			case graphics::StencilOp::INCR_SAT:
				return VK_STENCIL_OP_INCREMENT_AND_CLAMP;
			case graphics::StencilOp::DECR_SAT:
				return VK_STENCIL_OP_DECREMENT_AND_CLAMP;
			case graphics::StencilOp::INVERT:
				return VK_STENCIL_OP_INVERT;
			case graphics::StencilOp::INCR:
				return VK_STENCIL_OP_INCREMENT_AND_WRAP;
			case graphics::StencilOp::DECR:
				return VK_STENCIL_OP_DECREMENT_AND_WRAP;
			default:
				return VK_STENCIL_OP_KEEP;
			}
		}

		constexpr VkImageLayout _ConvertImageLayout(ResourceState value)
		{
			switch (value)
			{
			case ResourceState::UNDEFINED:
				return VK_IMAGE_LAYOUT_UNDEFINED;
			case ResourceState::RENDERTARGET:
				return VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			case ResourceState::DEPTHSTENCIL:
				return VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			case ResourceState::DEPTHSTENCIL_READONLY:
				return VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
			case ResourceState::SHADER_RESOURCE:
			case ResourceState::SHADER_RESOURCE_COMPUTE:
				return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			case ResourceState::UNORDERED_ACCESS:
				return VK_IMAGE_LAYOUT_GENERAL;
			case ResourceState::COPY_SRC:
			case ResourceState::COPY_DST:
				// we can't assume transfer layout because it's allowed for resource to be used by multiple queues like DX12 (decay to common state), so this is a workaround
				//	the problem is that image copy commands will require specifying the current layout, but different queues can often use textures in different layouts
				return VK_IMAGE_LAYOUT_GENERAL;
			case ResourceState::SHADING_RATE_SOURCE:
				return VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
			case ResourceState::VIDEO_DECODE_DPB:
				return VK_IMAGE_LAYOUT_VIDEO_DECODE_DPB_KHR;
			case ResourceState::VIDEO_DECODE_SRC:
				return VK_IMAGE_LAYOUT_VIDEO_DECODE_SRC_KHR;
			case ResourceState::VIDEO_DECODE_DST:
				return VK_IMAGE_LAYOUT_VIDEO_DECODE_DST_KHR;
			case ResourceState::SWAPCHAIN:
				return VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
			default:
				// combination of state flags will default to general
				//	whether the combination of states is valid needs to be validated by the user
				//	combining read-only states should be fine
				return VK_IMAGE_LAYOUT_GENERAL;
			}
		}
		constexpr VkShaderStageFlags _ConvertStageFlags(ShaderStage value)
		{
			switch (value)
			{
			case ShaderStage::MS:
				return VK_SHADER_STAGE_MESH_BIT_EXT;
			case ShaderStage::AS:
				return VK_SHADER_STAGE_TASK_BIT_EXT;
			case ShaderStage::VS:
				return VK_SHADER_STAGE_VERTEX_BIT;
			case ShaderStage::HS:
				return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;
			case ShaderStage::DS:
				return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;
			case ShaderStage::GS:
				return VK_SHADER_STAGE_GEOMETRY_BIT;
			case ShaderStage::PS:
				return VK_SHADER_STAGE_FRAGMENT_BIT;
			case ShaderStage::CS:
				return VK_SHADER_STAGE_COMPUTE_BIT;
			default:
				return VK_SHADER_STAGE_ALL;
			}
		}
		constexpr VkImageAspectFlags _ConvertImageAspect(ImageAspect value)
		{
			switch (value)
			{
			default:
			case graphics::ImageAspect::COLOR:
				return VK_IMAGE_ASPECT_COLOR_BIT;
			case graphics::ImageAspect::DEPTH:
				return VK_IMAGE_ASPECT_DEPTH_BIT;
			case graphics::ImageAspect::STENCIL:
				return VK_IMAGE_ASPECT_STENCIL_BIT;
			case graphics::ImageAspect::LUMINANCE:
				return VK_IMAGE_ASPECT_PLANE_0_BIT;
			case graphics::ImageAspect::CHROMINANCE:
				return VK_IMAGE_ASPECT_PLANE_1_BIT;
			}
		}
		constexpr VkPipelineStageFlags2 _ConvertPipelineStage(ResourceState value)
		{
			VkPipelineStageFlags2 flags = VK_PIPELINE_STAGE_2_NONE;

			if (has_flag(value, ResourceState::SHADER_RESOURCE) ||
				has_flag(value, ResourceState::SHADER_RESOURCE_COMPUTE) ||
				has_flag(value, ResourceState::UNORDERED_ACCESS) ||
				has_flag(value, ResourceState::CONSTANT_BUFFER))
			{
				flags |= VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
			}
			if (has_flag(value, ResourceState::COPY_SRC) ||
				has_flag(value, ResourceState::COPY_DST))
			{
				flags |= VK_PIPELINE_STAGE_2_TRANSFER_BIT;
			}
			if (has_flag(value, ResourceState::RENDERTARGET))
			{
				flags |= VK_PIPELINE_STAGE_2_COLOR_ATTACHMENT_OUTPUT_BIT;
			}
			if (has_flag(value, ResourceState::DEPTHSTENCIL) ||
				has_flag(value, ResourceState::DEPTHSTENCIL_READONLY))
			{
				flags |= VK_PIPELINE_STAGE_2_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_2_LATE_FRAGMENT_TESTS_BIT;
			}
			if (has_flag(value, ResourceState::SHADING_RATE_SOURCE))
			{
				flags |= VK_PIPELINE_STAGE_2_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR;
			}
			if (has_flag(value, ResourceState::VERTEX_BUFFER))
			{
				flags |= VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT;
			}
			if (has_flag(value, ResourceState::INDEX_BUFFER))
			{
				flags |= VK_PIPELINE_STAGE_2_INDEX_INPUT_BIT;
			}
			if (has_flag(value, ResourceState::INDIRECT_ARGUMENT))
			{
				flags |= VK_PIPELINE_STAGE_2_DRAW_INDIRECT_BIT;
			}
			if (has_flag(value, ResourceState::RAYTRACING_ACCELERATION_STRUCTURE))
			{
				flags |= VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR | VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;
			}
			if (has_flag(value, ResourceState::PREDICATION))
			{
				flags |= VK_PIPELINE_STAGE_2_CONDITIONAL_RENDERING_BIT_EXT;
			}
			if (has_flag(value, ResourceState::VIDEO_DECODE_DST) ||
				has_flag(value, ResourceState::VIDEO_DECODE_SRC) ||
				has_flag(value, ResourceState::VIDEO_DECODE_DPB))
			{
				flags |= VK_PIPELINE_STAGE_2_VIDEO_DECODE_BIT_KHR;
			}

			return flags;
		}
		constexpr VkAccessFlags2 _ParseResourceState(ResourceState value)
		{
			VkAccessFlags2 flags = 0;

			if (has_flag(value, ResourceState::SHADER_RESOURCE))
			{
				flags |= VK_ACCESS_2_SHADER_READ_BIT;
			}
			if (has_flag(value, ResourceState::SHADER_RESOURCE_COMPUTE))
			{
				flags |= VK_ACCESS_2_SHADER_READ_BIT;
			}
			if (has_flag(value, ResourceState::UNORDERED_ACCESS))
			{
				flags |= VK_ACCESS_2_SHADER_READ_BIT;
				flags |= VK_ACCESS_2_SHADER_WRITE_BIT;
			}
			if (has_flag(value, ResourceState::COPY_SRC))
			{
				flags |= VK_ACCESS_2_TRANSFER_READ_BIT;
			}
			if (has_flag(value, ResourceState::COPY_DST))
			{
				flags |= VK_ACCESS_2_TRANSFER_WRITE_BIT;
			}
			if (has_flag(value, ResourceState::RENDERTARGET))
			{
				flags |= VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT;
				flags |= VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
			}
			if (has_flag(value, ResourceState::DEPTHSTENCIL))
			{
				flags |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
				flags |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			}
			if (has_flag(value, ResourceState::DEPTHSTENCIL_READONLY))
			{
				flags |= VK_ACCESS_2_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
			}
			if (has_flag(value, ResourceState::VERTEX_BUFFER))
			{
				flags |= VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT;
			}
			if (has_flag(value, ResourceState::INDEX_BUFFER))
			{
				flags |= VK_ACCESS_2_INDEX_READ_BIT;
			}
			if (has_flag(value, ResourceState::CONSTANT_BUFFER))
			{
				flags |= VK_ACCESS_2_UNIFORM_READ_BIT;
			}
			if (has_flag(value, ResourceState::INDIRECT_ARGUMENT))
			{
				flags |= VK_ACCESS_2_INDIRECT_COMMAND_READ_BIT;
			}
			if (has_flag(value, ResourceState::PREDICATION))
			{
				flags |= VK_ACCESS_2_CONDITIONAL_RENDERING_READ_BIT_EXT;
			}
			if (has_flag(value, ResourceState::SHADING_RATE_SOURCE))
			{
				flags |= VK_ACCESS_2_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR;
			}
			if (has_flag(value, ResourceState::VIDEO_DECODE_DST))
			{
				flags |= VK_ACCESS_2_VIDEO_DECODE_WRITE_BIT_KHR;
			}
			if (has_flag(value, ResourceState::VIDEO_DECODE_SRC))
			{
				flags |= VK_ACCESS_2_VIDEO_DECODE_READ_BIT_KHR;
			}
			if (has_flag(value, ResourceState::VIDEO_DECODE_DPB))
			{
				flags |= VK_ACCESS_2_VIDEO_DECODE_WRITE_BIT_KHR;
			}

			return flags;
		}
		constexpr VkComponentSwizzle _ConvertComponentSwizzle(ComponentSwizzle value)
		{
			switch (value)
			{
			default:
				return VK_COMPONENT_SWIZZLE_IDENTITY;
			case graphics::ComponentSwizzle::R:
				return VK_COMPONENT_SWIZZLE_R;
			case graphics::ComponentSwizzle::G:
				return VK_COMPONENT_SWIZZLE_G;
			case graphics::ComponentSwizzle::B:
				return VK_COMPONENT_SWIZZLE_B;
			case graphics::ComponentSwizzle::A:
				return VK_COMPONENT_SWIZZLE_A;
			case graphics::ComponentSwizzle::ZERO:
				return VK_COMPONENT_SWIZZLE_ZERO;
			case graphics::ComponentSwizzle::ONE:
				return VK_COMPONENT_SWIZZLE_ONE;
			}
		}
		constexpr VkComponentMapping _ConvertSwizzle(Swizzle value)
		{
			VkComponentMapping mapping = {};
			mapping.r = _ConvertComponentSwizzle(value.r);
			mapping.g = _ConvertComponentSwizzle(value.g);
			mapping.b = _ConvertComponentSwizzle(value.b);
			mapping.a = _ConvertComponentSwizzle(value.a);
			return mapping;
		}


		bool checkExtensionSupport(const char* checkExtension, const vector<VkExtensionProperties>& available_extensions)
		{
			for (const auto& x : available_extensions)
			{
				if (strcmp(x.extensionName, checkExtension) == 0)
				{
					return true;
				}
			}
			return false;
		}

		bool ValidateLayers(const vector<const char*>& required,
			const vector<VkLayerProperties>& available)
		{
			for (auto layer : required)
			{
				bool found = false;
				for (auto& available_layer : available)
				{
					if (strcmp(available_layer.layerName, layer) == 0)
					{
						found = true;
						break;
					}
				}

				if (!found)
				{
					return false;
				}
			}

			return true;
		}

		VKAPI_ATTR VkBool32 VKAPI_CALL debugUtilsMessengerCallback(
			VkDebugUtilsMessageSeverityFlagBitsEXT message_severity,
			VkDebugUtilsMessageTypeFlagsEXT message_type,
			const VkDebugUtilsMessengerCallbackDataEXT* callback_data,
			void* user_data)
		{
			// Log debug message
			std::string ss;

			if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
			{
				ss += "[Vulkan Warning]: ";
				ss += callback_data->pMessage;
				ss += "\n";
				helper::DebugOut(ss, helper::DebugLevel::Warning);
			}
			else if (message_severity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
			{
				ss += "[Vulkan Error]: ";
				ss += callback_data->pMessage;
				ss += "\n";
				helper::DebugOut(ss, helper::DebugLevel::Error);
			}

			return VK_FALSE;
		}

		inline std::string get_shader_cache_path()
		{
			return helper::GetCurrentPath() + "/pso_cache_vulkan";
		}

		struct BindingUsage
		{
			bool used = false;
			VkDescriptorSetLayoutBinding binding = {};
		};
		struct Buffer_Vulkan
		{
			std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler;
			VmaAllocation allocation = nullptr;
			VkBuffer resource = VK_NULL_HANDLE;
			struct BufferSubresource
			{
				bool is_typed = false;
				VkBufferView buffer_view = VK_NULL_HANDLE;
				VkDescriptorBufferInfo buffer_info = {};
				int index = -1; // bindless

				constexpr bool IsValid() const
				{
					return index >= 0;
				}
			};
			BufferSubresource srv;
			BufferSubresource uav;
			vector<BufferSubresource> subresources_srv;
			vector<BufferSubresource> subresources_uav;
			VkDeviceAddress address = 0;

			void destroy_subresources()
			{
				uint64_t framecount = allocationhandler->framecount;
				if (srv.IsValid())
				{
					if (srv.is_typed)
					{
						allocationhandler->destroyer_bufferviews.push_back(std::make_pair(srv.buffer_view, framecount));
						allocationhandler->destroyer_bindlessUniformTexelBuffers.push_back(std::make_pair(srv.index, framecount));
					}
					else
					{
						allocationhandler->destroyer_bindlessStorageBuffers.push_back(std::make_pair(srv.index, framecount));
					}
					srv = {};
				}
				if (uav.IsValid())
				{
					if (uav.is_typed)
					{
						allocationhandler->destroyer_bufferviews.push_back(std::make_pair(uav.buffer_view, framecount));
						allocationhandler->destroyer_bindlessStorageTexelBuffers.push_back(std::make_pair(uav.index, framecount));
					}
					else
					{
						allocationhandler->destroyer_bindlessStorageBuffers.push_back(std::make_pair(uav.index, framecount));
					}
					uav = {};
				}
				for (auto& x : subresources_srv)
				{
					if (x.is_typed)
					{
						allocationhandler->destroyer_bufferviews.push_back(std::make_pair(x.buffer_view, framecount));
						allocationhandler->destroyer_bindlessUniformTexelBuffers.push_back(std::make_pair(x.index, framecount));
					}
					else
					{
						allocationhandler->destroyer_bindlessStorageBuffers.push_back(std::make_pair(x.index, framecount));
					}
				}
				subresources_srv.clear();
				for (auto& x : subresources_uav)
				{
					if (x.is_typed)
					{
						allocationhandler->destroyer_bufferviews.push_back(std::make_pair(x.buffer_view, framecount));
						allocationhandler->destroyer_bindlessStorageTexelBuffers.push_back(std::make_pair(x.index, framecount));
					}
					else
					{
						allocationhandler->destroyer_bindlessStorageBuffers.push_back(std::make_pair(x.index, framecount));
					}
				}
				subresources_uav.clear();
			}

			~Buffer_Vulkan()
			{
				if (allocationhandler == nullptr)
					return;
				allocationhandler->destroylocker.lock();
				uint64_t framecount = allocationhandler->framecount;
				if (resource)
				{
					allocationhandler->destroyer_buffers.push_back(std::make_pair(std::make_pair(resource, allocation), framecount));
				}
				else if (allocation)
				{
					allocationhandler->destroyer_allocations.push_back(std::make_pair(allocation, framecount));
				}
				destroy_subresources();
				allocationhandler->destroylocker.unlock();
			}
		};
		struct Texture_Vulkan
		{
			std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler;
			VmaAllocation allocation = nullptr;
			VkImage resource = VK_NULL_HANDLE;
			VkImageLayout defaultLayout = VK_IMAGE_LAYOUT_GENERAL;
			VkBuffer staging_resource = VK_NULL_HANDLE;
			struct TextureSubresource
			{
				VkImageView image_view = VK_NULL_HANDLE;
				int index = -1; // bindless
				uint32_t firstMip = 0;
				uint32_t mipCount = 0;
				uint32_t firstSlice = 0;
				uint32_t sliceCount = 0;

				constexpr bool IsValid() const
				{
					return image_view != VK_NULL_HANDLE;
				}
			};
			TextureSubresource srv;
			TextureSubresource uav;
			TextureSubresource rtv;
			TextureSubresource dsv;
			uint32_t framebuffer_layercount = 0;
			vector<TextureSubresource> subresources_srv;
			vector<TextureSubresource> subresources_uav;
			vector<TextureSubresource> subresources_rtv;
			vector<TextureSubresource> subresources_dsv;

			vector<SubresourceData> mapped_subresources;
			SparseTextureProperties sparse_texture_properties;

			VkImageView video_decode_view = VK_NULL_HANDLE;

			void destroy_subresources()
			{
				uint64_t framecount = allocationhandler->framecount;
				if (srv.IsValid())
				{
					allocationhandler->destroyer_imageviews.push_back(std::make_pair(srv.image_view, framecount));
					allocationhandler->destroyer_bindlessSampledImages.push_back(std::make_pair(srv.index, framecount));
					srv = {};
				}
				if (uav.IsValid())
				{
					allocationhandler->destroyer_imageviews.push_back(std::make_pair(uav.image_view, framecount));
					allocationhandler->destroyer_bindlessStorageImages.push_back(std::make_pair(uav.index, framecount));
					uav = {};
				}
				if (rtv.IsValid())
				{
					allocationhandler->destroyer_imageviews.push_back(std::make_pair(rtv.image_view, framecount));
					rtv = {};
				}
				if (dsv.IsValid())
				{
					allocationhandler->destroyer_imageviews.push_back(std::make_pair(dsv.image_view, framecount));
					dsv = {};
				}
				for (auto x : subresources_srv)
				{
					allocationhandler->destroyer_imageviews.push_back(std::make_pair(x.image_view, framecount));
					allocationhandler->destroyer_bindlessSampledImages.push_back(std::make_pair(x.index, framecount));
				}
				subresources_srv.clear();
				for (auto x : subresources_uav)
				{
					allocationhandler->destroyer_imageviews.push_back(std::make_pair(x.image_view, framecount));
					allocationhandler->destroyer_bindlessStorageImages.push_back(std::make_pair(x.index, framecount));
				}
				subresources_uav.clear();
				for (auto x : subresources_rtv)
				{
					allocationhandler->destroyer_imageviews.push_back(std::make_pair(x.image_view, framecount));
				}
				subresources_rtv.clear();
				for (auto x : subresources_dsv)
				{
					allocationhandler->destroyer_imageviews.push_back(std::make_pair(x.image_view, framecount));
				}
				subresources_dsv.clear();
			}

			~Texture_Vulkan()
			{
				if (allocationhandler == nullptr)
					return;
				allocationhandler->destroylocker.lock();
				uint64_t framecount = allocationhandler->framecount;
				if (resource)
				{
					allocationhandler->destroyer_images.push_back(std::make_pair(std::make_pair(resource, allocation), framecount));
				}
				else if (staging_resource)
				{
					allocationhandler->destroyer_buffers.push_back(std::make_pair(std::make_pair(staging_resource, allocation), framecount));
				}
				else if (allocation)
				{
					allocationhandler->destroyer_allocations.push_back(std::make_pair(allocation, framecount));
				}
				if (video_decode_view != VK_NULL_HANDLE)
				{
					allocationhandler->destroyer_imageviews.push_back(std::make_pair(video_decode_view, framecount));
				}
				destroy_subresources();
				allocationhandler->destroylocker.unlock();
			}
		};
		struct Sampler_Vulkan
		{
			std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler;
			VkSampler resource = VK_NULL_HANDLE;
			int index = -1;

			~Sampler_Vulkan()
			{
				if (allocationhandler == nullptr)
					return;
				allocationhandler->destroylocker.lock();
				uint64_t framecount = allocationhandler->framecount;
				if (resource) allocationhandler->destroyer_samplers.push_back(std::make_pair(resource, framecount));
				if (index >= 0) allocationhandler->destroyer_bindlessSamplers.push_back(std::make_pair(index, framecount));
				allocationhandler->destroylocker.unlock();
			}
		};
		struct QueryHeap_Vulkan
		{
			std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler;
			VkQueryPool pool = VK_NULL_HANDLE;

			~QueryHeap_Vulkan()
			{
				if (allocationhandler == nullptr)
					return;
				allocationhandler->destroylocker.lock();
				uint64_t framecount = allocationhandler->framecount;
				if (pool) allocationhandler->destroyer_querypools.push_back(std::make_pair(pool, framecount));
				allocationhandler->destroylocker.unlock();
			}
		};

		struct Shader_Vulkan
		{
			std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler;
			VkShaderModule shaderModule = VK_NULL_HANDLE;
			VkPipeline pipeline_cs = VK_NULL_HANDLE;
			VkPipelineShaderStageCreateInfo stageInfo = {};
			std::shared_ptr<GraphicsDevice_Vulkan::PSOLayout> layout_lifetime; // lifetime management only
			VkPipelineLayout pipelineLayout_cs = VK_NULL_HANDLE; // no lifetime management here
			VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE; // no lifetime management here
			vector<VkDescriptorSetLayoutBinding> layoutBindings;
			vector<VkImageViewType> imageViewTypes;

			vector<BindingUsage> bindlessBindings;
			vector<VkDescriptorSet> bindlessSets;
			uint32_t bindlessFirstSet = 0;

			VkPushConstantRange pushconstants = {};

			VkDeviceSize uniform_buffer_sizes[DESCRIPTORBINDER_CBV_COUNT] = {};
			vector<uint32_t> uniform_buffer_dynamic_slots;

			~Shader_Vulkan()
			{
				if (allocationhandler == nullptr)
					return;
				allocationhandler->destroylocker.lock();
				uint64_t framecount = allocationhandler->framecount;
				if (shaderModule) allocationhandler->destroyer_shadermodules.push_back(std::make_pair(shaderModule, framecount));
				if (pipeline_cs) allocationhandler->destroyer_pipelines.push_back(std::make_pair(pipeline_cs, framecount));
				allocationhandler->destroylocker.unlock();
			}
		};

		struct PipelineState_Vulkan
		{
			std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler;
			VkPipeline pipeline = VK_NULL_HANDLE;
			std::shared_ptr<GraphicsDevice_Vulkan::PSOLayout> layout_lifetime; // lifetime management only
			VkPipelineLayout pipelineLayout = VK_NULL_HANDLE; // no lifetime management here
			VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE; // no lifetime management here
			vector<VkDescriptorSetLayoutBinding> layoutBindings;
			vector<VkImageViewType> imageViewTypes;

			vector<BindingUsage> bindlessBindings;
			vector<VkDescriptorSet> bindlessSets;
			uint32_t bindlessFirstSet = 0;

			VkPushConstantRange pushconstants = {};

			VkDeviceSize uniform_buffer_sizes[DESCRIPTORBINDER_CBV_COUNT] = {};
			vector<uint32_t> uniform_buffer_dynamic_slots;

			VkGraphicsPipelineCreateInfo pipelineInfo = {};
			VkPipelineShaderStageCreateInfo shaderStages[static_cast<size_t>(ShaderStage::Count)] = {};
			VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
			VkPipelineRasterizationStateCreateInfo rasterizer = {};
			VkPipelineRasterizationDepthClipStateCreateInfoEXT depthClipStateInfo = {};
			VkPipelineRasterizationConservativeStateCreateInfoEXT rasterizationConservativeState = {};
			VkPipelineViewportStateCreateInfo viewportState = {};
			VkPipelineDepthStencilStateCreateInfo depthstencil = {};
			VkSampleMask samplemask = {};
			VkPipelineTessellationStateCreateInfo tessellationInfo = {};

			~PipelineState_Vulkan()
			{
				if (allocationhandler == nullptr)
					return;
				allocationhandler->destroylocker.lock();
				uint64_t framecount = allocationhandler->framecount;
				if (pipeline) allocationhandler->destroyer_pipelines.push_back(std::make_pair(pipeline, framecount));
				allocationhandler->destroylocker.unlock();
			}
		};

		struct BVH_Vulkan
		{
			std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler;
			VmaAllocation allocation = nullptr;
			VkBuffer buffer = VK_NULL_HANDLE;
			VkAccelerationStructureKHR resource = VK_NULL_HANDLE;
			int index = -1;

			VkAccelerationStructureBuildGeometryInfoKHR buildInfo = {};
			VkAccelerationStructureBuildSizesInfoKHR sizeInfo = {};
			VkAccelerationStructureCreateInfoKHR createInfo = {};
			vector<VkAccelerationStructureGeometryKHR> geometries;
			vector<uint32_t> primitiveCounts;
			VkDeviceAddress scratch_address = 0;
			VkDeviceAddress as_address = 0;

			~BVH_Vulkan()
			{
				if (allocationhandler == nullptr)
					return;
				allocationhandler->destroylocker.lock();
				uint64_t framecount = allocationhandler->framecount;
				if (buffer) allocationhandler->destroyer_buffers.push_back(std::make_pair(std::make_pair(buffer, allocation), framecount));
				if (resource) allocationhandler->destroyer_bvhs.push_back(std::make_pair(resource, framecount));
				if (index >= 0) allocationhandler->destroyer_bindlessAccelerationStructures.push_back(std::make_pair(index, framecount));
				allocationhandler->destroylocker.unlock();
			}
		};

		struct RTPipelineState_Vulkan
		{
			std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler;
			VkPipeline pipeline;

			~RTPipelineState_Vulkan()
			{
				if (allocationhandler == nullptr)
					return;
				allocationhandler->destroylocker.lock();
				uint64_t framecount = allocationhandler->framecount;
				if (pipeline) allocationhandler->destroyer_pipelines.push_back(std::make_pair(pipeline, framecount));
				allocationhandler->destroylocker.unlock();
			}
		};

		struct SwapChain_Vulkan
		{
			std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler;
			VkSwapchainKHR swapChain = VK_NULL_HANDLE;
			VkFormat swapChainImageFormat;
			VkExtent2D swapChainExtent;
			vector<VkImage> swapChainImages;
			vector<VkImageView> swapChainImageViews;

			Texture dummyTexture;

			VkSurfaceKHR surface = VK_NULL_HANDLE;

			uint32_t swapChainImageIndex = 0;
			uint32_t swapChainAcquireSemaphoreIndex = 0;
			vector<VkSemaphore> swapchainAcquireSemaphores;
			vector<VkSemaphore> swapchainReleaseSemaphores;

			ColorSpace colorSpace = ColorSpace::SRGB;
			SwapChainDesc desc;
			std::mutex locker;

			~SwapChain_Vulkan()
			{
				if (allocationhandler == nullptr)
					return;
				allocationhandler->destroylocker.lock();
				uint64_t framecount = allocationhandler->framecount;

				for (size_t i = 0; i < swapChainImages.size(); ++i)
				{
					allocationhandler->destroyer_imageviews.push_back(std::make_pair(swapChainImageViews[i], framecount));
					allocationhandler->destroyer_semaphores.push_back(std::make_pair(swapchainAcquireSemaphores[i], framecount));
					allocationhandler->destroyer_semaphores.push_back(std::make_pair(swapchainReleaseSemaphores[i], framecount));
				}

#ifdef SDL2
				// Checks if the SDL VIDEO System was already destroyed.
				// If so we would delete the swapchain twice, causing a crash on wayland.
				if (SDL_WasInit(SDL_INIT_VIDEO))
#endif
				{
					allocationhandler->destroyer_swapchains.push_back(std::make_pair(swapChain, framecount));
					allocationhandler->destroyer_surfaces.push_back(std::make_pair(surface, framecount));
				}

				allocationhandler->destroylocker.unlock();

			}
		};

		struct VideoDecoder_Vulkan
		{
			std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler;
			VkVideoSessionKHR video_session = VK_NULL_HANDLE;
			VkVideoSessionParametersKHR session_parameters = VK_NULL_HANDLE;
			vector<VmaAllocation> allocations;

			~VideoDecoder_Vulkan()
			{
				if (allocationhandler == nullptr)
					return;
				allocationhandler->destroylocker.lock();
				uint64_t framecount = allocationhandler->framecount;
				allocationhandler->destroyer_video_sessions.push_back(std::make_pair(video_session, framecount));
				allocationhandler->destroyer_video_session_parameters.push_back(std::make_pair(session_parameters, framecount));
				for (auto& x : allocations)
				{
					allocationhandler->destroyer_allocations.push_back(std::make_pair(x, framecount));
				}
				allocationhandler->destroylocker.unlock();
			}
		};

		Buffer_Vulkan* to_internal(const GPUBuffer* param)
		{
			return static_cast<Buffer_Vulkan*>(param->internal_state.get());
		}
		Texture_Vulkan* to_internal(const Texture* param)
		{
			return static_cast<Texture_Vulkan*>(param->internal_state.get());
		}
		Sampler_Vulkan* to_internal(const Sampler* param)
		{
			return static_cast<Sampler_Vulkan*>(param->internal_state.get());
		}
		QueryHeap_Vulkan* to_internal(const GPUQueryHeap* param)
		{
			return static_cast<QueryHeap_Vulkan*>(param->internal_state.get());
		}
		Shader_Vulkan* to_internal(const Shader* param)
		{
			return static_cast<Shader_Vulkan*>(param->internal_state.get());
		}
		PipelineState_Vulkan* to_internal(const PipelineState* param)
		{
			return static_cast<PipelineState_Vulkan*>(param->internal_state.get());
		}
		BVH_Vulkan* to_internal(const RaytracingAccelerationStructure* param)
		{
			return static_cast<BVH_Vulkan*>(param->internal_state.get());
		}
		RTPipelineState_Vulkan* to_internal(const RaytracingPipelineState* param)
		{
			return static_cast<RTPipelineState_Vulkan*>(param->internal_state.get());
		}
		SwapChain_Vulkan* to_internal(const SwapChain* param)
		{
			return static_cast<SwapChain_Vulkan*>(param->internal_state.get());
		}
		VideoDecoder_Vulkan* to_internal(const VideoDecoder* param)
		{
			return static_cast<VideoDecoder_Vulkan*>(param->internal_state.get());
		}


		bool CreateSwapChainInternal(
			SwapChain_Vulkan* internal_state,
			VkPhysicalDevice physicalDevice,
			VkDevice device,
			std::shared_ptr<GraphicsDevice_Vulkan::AllocationHandler> allocationhandler
		) {
			// In vulkan, the swapchain recreate can happen whenever it gets outdated, it's not in application's control
			//	so we have to be extra careful
			std::scoped_lock lock(internal_state->locker);
			VkSurfaceCapabilitiesKHR swapchain_capabilities;
			vulkan_check(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, internal_state->surface, &swapchain_capabilities));

			uint32_t formatCount;
			vulkan_check(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, internal_state->surface, &formatCount, nullptr));

			vector<VkSurfaceFormatKHR> swapchain_formats(formatCount);
			vulkan_check(vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, internal_state->surface, &formatCount, swapchain_formats.data()));

			uint32_t presentModeCount;
			vulkan_check(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, internal_state->surface, &presentModeCount, nullptr));
			
			vector<VkPresentModeKHR> swapchain_presentModes(presentModeCount);
			swapchain_presentModes.resize(presentModeCount);
			vulkan_check(vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, internal_state->surface, &presentModeCount, swapchain_presentModes.data()));

			VkSurfaceFormatKHR surfaceFormat = {};
			surfaceFormat.format = _ConvertFormat(internal_state->desc.format);
			surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
			bool valid = false;

			for (const auto& format : swapchain_formats)
			{
				if (!internal_state->desc.allow_hdr && format.colorSpace != VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
					continue;
				if (format.format == surfaceFormat.format)
				{
					surfaceFormat = format;
					valid = true;
					break;
				}
			}

			if (!valid)
			{
				internal_state->desc.format = Format::B8G8R8A8_UNORM;
				surfaceFormat.format = VK_FORMAT_B8G8R8A8_UNORM;
				surfaceFormat.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
			}
			
			// For now, we only include the color spaces that were tested successfully:
			ColorSpace prev_colorspace = internal_state->colorSpace;
			switch (surfaceFormat.colorSpace)
			{
			default:
			case VK_COLOR_SPACE_SRGB_NONLINEAR_KHR:
				internal_state->colorSpace = ColorSpace::SRGB;
				break;
			case VK_COLOR_SPACE_EXTENDED_SRGB_LINEAR_EXT:
				internal_state->colorSpace = ColorSpace::HDR_LINEAR;
				break;
			case VK_COLOR_SPACE_HDR10_ST2084_EXT:
				internal_state->colorSpace = ColorSpace::HDR10_ST2084;
				break;
			}
			

			if (prev_colorspace != internal_state->colorSpace)
			{
				if (internal_state->swapChain != VK_NULL_HANDLE)
				{
					// For some reason, if the swapchain gets recreated (via oldSwapChain) with different color space but same image format,
					//	the color space change will not be applied
					vulkan_check(vkDeviceWaitIdle(device));
					vkDestroySwapchainKHR(device, internal_state->swapChain, nullptr);
					internal_state->swapChain = nullptr;
				}
			}


			if (swapchain_capabilities.currentExtent.width != 0xFFFFFFFF && swapchain_capabilities.currentExtent.height != 0xFFFFFFFF)
			{
				internal_state->swapChainExtent = swapchain_capabilities.currentExtent;
			}
			else
			{
				internal_state->swapChainExtent = { internal_state->desc.width, internal_state->desc.height };
				internal_state->swapChainExtent.width = std::max(swapchain_capabilities.minImageExtent.width, std::min(swapchain_capabilities.maxImageExtent.width, internal_state->swapChainExtent.width));
				internal_state->swapChainExtent.height = std::max(swapchain_capabilities.minImageExtent.height, std::min(swapchain_capabilities.maxImageExtent.height, internal_state->swapChainExtent.height));
			}

			uint32_t imageCount = std::max(internal_state->desc.buffer_count, swapchain_capabilities.minImageCount);
			if ((swapchain_capabilities.maxImageCount > 0) && (imageCount > swapchain_capabilities.maxImageCount))
			{
				imageCount = swapchain_capabilities.maxImageCount;
			}

			VkSwapchainCreateInfoKHR createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			createInfo.surface = internal_state->surface;
			createInfo.minImageCount = imageCount;
			createInfo.imageFormat = surfaceFormat.format;
			createInfo.imageColorSpace = surfaceFormat.colorSpace;
			createInfo.imageExtent = internal_state->swapChainExtent;
			createInfo.imageArrayLayers = 1;
			createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			createInfo.preTransform = swapchain_capabilities.currentTransform;

			createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR; // The only one that is always supported


			if (!internal_state->desc.vsync)
			{
				// The mailbox/immediate present mode is not necessarily supported:
				for (auto& presentMode : swapchain_presentModes)
				{
					if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
					{
						createInfo.presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
						break;
					}
					if (presentMode == VK_PRESENT_MODE_IMMEDIATE_KHR)
					{
						createInfo.presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
					}
				}
			}

			createInfo.clipped = VK_TRUE;
			createInfo.oldSwapchain = internal_state->swapChain;

			vulkan_check(vkCreateSwapchainKHR(device, &createInfo, nullptr, &internal_state->swapChain));

			if (createInfo.oldSwapchain != VK_NULL_HANDLE)
			{
				std::scoped_lock lock(allocationhandler->destroylocker);
				allocationhandler->destroyer_swapchains.emplace_back(createInfo.oldSwapchain, allocationhandler->framecount);
			}

			vulkan_check(vkGetSwapchainImagesKHR(device, internal_state->swapChain, &imageCount, nullptr));
			internal_state->swapChainImages.resize(imageCount);
			vulkan_check(vkGetSwapchainImagesKHR(device, internal_state->swapChain, &imageCount, internal_state->swapChainImages.data()));
			internal_state->swapChainImageFormat = surfaceFormat.format;

			// Create swap chain render targets:
			internal_state->swapChainImageViews.resize(internal_state->swapChainImages.size());
			for (size_t i = 0; i < internal_state->swapChainImages.size(); ++i)
			{
				VkImageViewCreateInfo createInfo = {};
				createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
				createInfo.image = internal_state->swapChainImages[i];
				createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
				createInfo.format = internal_state->swapChainImageFormat;
				createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
				createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
				createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
				createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
				createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				createInfo.subresourceRange.baseMipLevel = 0;
				createInfo.subresourceRange.levelCount = 1;
				createInfo.subresourceRange.baseArrayLayer = 0;
				createInfo.subresourceRange.layerCount = 1;

				if (internal_state->swapChainImageViews[i] != VK_NULL_HANDLE)
				{
					allocationhandler->destroylocker.lock();
					allocationhandler->destroyer_imageviews.push_back(std::make_pair(internal_state->swapChainImageViews[i], allocationhandler->framecount));
					allocationhandler->destroylocker.unlock();
				}
				vulkan_check(vkCreateImageView(device, &createInfo, nullptr, &internal_state->swapChainImageViews[i]));
			}


			VkSemaphoreCreateInfo semaphoreInfo = {};
			semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

			// safety release of current swapchain semaphores that might still be working, since this could have been called mid-frame:
			allocationhandler->destroylocker.lock();
			for (auto& x : internal_state->swapchainAcquireSemaphores)
			{
				allocationhandler->destroyer_semaphores.push_back(std::make_pair(x, allocationhandler->framecount));
			}
			internal_state->swapchainAcquireSemaphores.clear();
			for (auto& x : internal_state->swapchainReleaseSemaphores)
			{
				allocationhandler->destroyer_semaphores.push_back(std::make_pair(x, allocationhandler->framecount));
			}
			internal_state->swapchainReleaseSemaphores.clear();
			allocationhandler->destroylocker.unlock();

			internal_state->swapChainAcquireSemaphoreIndex = 0;
			internal_state->swapChainImageIndex = 0;

			if (internal_state->swapchainAcquireSemaphores.empty())
			{
				for (size_t i = 0; i < internal_state->swapChainImages.size(); ++i)
				{
					vulkan_check(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &internal_state->swapchainAcquireSemaphores.emplace_back()));
				}
			}

			if (internal_state->swapchainReleaseSemaphores.empty())
			{
				for (size_t i = 0; i < internal_state->swapChainImages.size(); ++i)
				{
					vulkan_check(vkCreateSemaphore(device, &semaphoreInfo, nullptr, &internal_state->swapchainReleaseSemaphores.emplace_back()));
				}
			}

			return true;
		}
	}

	PFN_vkSetDebugUtilsObjectNameEXT pfnSetDebugUtilsObjectNameEXT = nullptr;

	using namespace vulkan_internal;

	void GraphicsDevice_Vulkan::set_fence_name(VkFence fence, const char* name) {
		if (!debugUtils)
			return;
		if (fence == VK_NULL_HANDLE)
			return;
		VkDebugUtilsObjectNameInfoEXT info{ VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
		info.pObjectName = name;
		info.objectType = VK_OBJECT_TYPE_FENCE;
		info.objectHandle = (uint64_t)fence;
		//vkSetDebugUtilsObjectNameEXT();
		if (pfnSetDebugUtilsObjectNameEXT == nullptr) {
			pfnSetDebugUtilsObjectNameEXT = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(instance, "vkSetDebugUtilsObjectNameEXT");
		}
		vulkan_check(pfnSetDebugUtilsObjectNameEXT(device, &info));
	}

	void GraphicsDevice_Vulkan::set_semaphore_name(VkSemaphore semaphore, const char* name)
	{
		if (!debugUtils)
			return;
		if (semaphore == VK_NULL_HANDLE)
			return;

		VkDebugUtilsObjectNameInfoEXT info{ VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT };
		info.pObjectName = name;
		info.objectType = VK_OBJECT_TYPE_SEMAPHORE;
		info.objectHandle = (uint64_t)semaphore;

		vulkan_check(pfnSetDebugUtilsObjectNameEXT(device, &info));
	}

	void GraphicsDevice_Vulkan::CommandQueue::clear()
	{
		swapchain_updates.clear();
		submit_waitSemaphoreInfos.clear();
		submit_signalSemaphoreInfos.clear();
		submit_cmds.clear();

		swapchainWaitSemaphores.clear();
		swapchains.clear();
		swapchainImageIndices.clear();
	}

	void GraphicsDevice_Vulkan::CommandQueue::signal(VkSemaphore semaphore)
	{
		if (queue == VK_NULL_HANDLE)
			return;
		VkSemaphoreSubmitInfo& signalSemaphore = submit_signalSemaphoreInfos.emplace_back();
		signalSemaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		signalSemaphore.semaphore = semaphore;
		signalSemaphore.value = 0; // not a timeline semaphore
		signalSemaphore.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	}

	void GraphicsDevice_Vulkan::CommandQueue::wait(VkSemaphore semaphore)
	{
		if (queue == VK_NULL_HANDLE)
			return;
		VkSemaphoreSubmitInfo& waitSemaphore = submit_waitSemaphoreInfos.emplace_back();
		waitSemaphore.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
		waitSemaphore.semaphore = semaphore;
		waitSemaphore.value = 0; // not a timeline semaphore
		waitSemaphore.stageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
	}

	void GraphicsDevice_Vulkan::CommandQueue::submit(GraphicsDevice_Vulkan* device, VkFence fence)
	{
		if (queue == VK_NULL_HANDLE)
			return;
		std::scoped_lock lock(*locker);

		// Main submit with command lists and semaphores:
		{
			if (fence != VK_NULL_HANDLE)
			{
				// end of frame mark:
				for (int q = 0; q < QUEUE_COUNT; ++q)
				{
					if (frame_semaphores[device->GetBufferIndex()][q] == VK_NULL_HANDLE)
						continue;
					signal(frame_semaphores[device->GetBufferIndex()][q]);
				}
			}

			VkSubmitInfo2 submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
			submitInfo.commandBufferInfoCount = (uint32_t)submit_cmds.size();
			submitInfo.pCommandBufferInfos = submit_cmds.data();

			submitInfo.waitSemaphoreInfoCount = (uint32_t)submit_waitSemaphoreInfos.size();
			submitInfo.pWaitSemaphoreInfos = submit_waitSemaphoreInfos.data();

			submitInfo.signalSemaphoreInfoCount = (uint32_t)submit_signalSemaphoreInfos.size();
			submitInfo.pSignalSemaphoreInfos = submit_signalSemaphoreInfos.data();

			vulkan_check(vkQueueSubmit2(queue, 1, &submitInfo, fence));

			submit_waitSemaphoreInfos.clear();
			submit_signalSemaphoreInfos.clear();
			submit_cmds.clear();
		}

		// Swapchain presents:
		if (!swapchains.empty())
		{
			VkPresentInfoKHR presentInfo = {};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			presentInfo.waitSemaphoreCount = (uint32_t)swapchainWaitSemaphores.size();
			presentInfo.pWaitSemaphores = swapchainWaitSemaphores.data();
			presentInfo.swapchainCount = (uint32_t)swapchains.size();
			presentInfo.pSwapchains = swapchains.data();
			presentInfo.pImageIndices = swapchainImageIndices.data();
			VkResult res = vkQueuePresentKHR(queue, &presentInfo);
			if (res != VK_SUCCESS)
			{
				// Handle outdated error in present:
				if (res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR)
				{
					for (auto& swapchain : swapchain_updates)
					{
						auto internal_state = to_internal(&swapchain);
						bool success = CreateSwapChainInternal(internal_state, device->physicalDevice, device->device, device->allocationhandler);
						assert(success);
					}
				}
				else
				{
					vulkan_assert(false, "vkQueuePresentKHR");
				}
			}

			swapchain_updates.clear();
			swapchains.clear();
			swapchainImageIndices.clear();
			swapchainWaitSemaphores.clear();
		}
	}

	void GraphicsDevice_Vulkan::CopyAllocator::init(GraphicsDevice_Vulkan* device)
	{
		this->device = device;
	}

	void GraphicsDevice_Vulkan::CopyAllocator::destroy()
	{
		vkQueueWaitIdle(device->queue_init.queue);
		for (auto& x : freelist)
		{
			vkDestroyCommandPool(device->device, x.transferCommandPool, nullptr);
			vkDestroyFence(device->device, x.fence, nullptr);
		}
	}
	GraphicsDevice_Vulkan::CopyAllocator::CopyCMD GraphicsDevice_Vulkan::CopyAllocator::allocate(uint64_t staging_size)
	{
		CopyCMD cmd;

		locker.lock();
		// Try to search for a staging buffer that can fit the request:
		for (size_t i = 0; i < freelist.size(); ++i)
		{
			if (freelist[i].uploadbuffer.desc.size >= staging_size)
			{
				cmd = std::move(freelist[i]);
				std::swap(freelist[i], freelist.back());
				freelist.pop_back();
				break;
			}
		}
		locker.unlock();

		// If no buffer was found that fits the data, create one:
		if (!cmd.IsValid())
		{
			VkCommandPoolCreateInfo poolInfo = {};
			poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			poolInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
			poolInfo.queueFamilyIndex = device->initFamily;
			vulkan_check(vkCreateCommandPool(device->device, &poolInfo, nullptr, &cmd.transferCommandPool));

			VkCommandBufferAllocateInfo commandBufferInfo = {};
			commandBufferInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			commandBufferInfo.commandBufferCount = 1;
			commandBufferInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
			commandBufferInfo.commandPool = cmd.transferCommandPool;
			vulkan_check(vkAllocateCommandBuffers(device->device, &commandBufferInfo, &cmd.transferCommandBuffer));

			VkFenceCreateInfo fenceInfo = {};
			fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			vulkan_check(vkCreateFence(device->device, &fenceInfo, nullptr, &cmd.fence));
			device->set_fence_name(cmd.fence, "CopyAllocator::fence");

			GPUBufferDesc uploaddesc;
			uploaddesc.size = pf::math::GetNextPowerOfTwo(staging_size);
			uploaddesc.size = std::max(uploaddesc.size, uint64_t(65536));
			uploaddesc.usage = Usage::UPLOAD;
			bool upload_success = device->CreateBuffer(&uploaddesc, nullptr, &cmd.uploadbuffer);
			assert(upload_success);
			device->SetName(&cmd.uploadbuffer, "CopyAllocator::uploadBuffer");
		}

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		beginInfo.pInheritanceInfo = nullptr;

		vulkan_check(vkResetCommandPool(device->device, cmd.transferCommandPool, 0));
		vulkan_check(vkBeginCommandBuffer(cmd.transferCommandBuffer, &beginInfo));

		vulkan_check(vkResetFences(device->device, 1, &cmd.fence));

		return cmd;
	}

	void GraphicsDevice_Vulkan::CopyAllocator::submit(CopyCMD cmd)
	{
		VkSubmitInfo2 submitInfo = {};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;

		VkCommandBufferSubmitInfo cbSubmitInfo = {};
		cbSubmitInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;

		{
			vulkan_check(vkEndCommandBuffer(cmd.transferCommandBuffer));
			cbSubmitInfo.commandBuffer = cmd.transferCommandBuffer;
			submitInfo.commandBufferInfoCount = 1;
			submitInfo.pCommandBufferInfos = &cbSubmitInfo;

			std::scoped_lock lock(*device->queue_init.locker);
			vulkan_check(vkQueueSubmit2(device->queue_init.queue, 1, &submitInfo, cmd.fence));
		}

		while (vulkan_check(vkWaitForFences(device->device, 1, &cmd.fence, VK_TRUE, timeout_value)) == VK_TIMEOUT)
		{
			log_error("[CopyAllocator::submit] vkWaitForFences resulted in VK_TIMEOUT");
			std::this_thread::yield();
		}

		std::scoped_lock lock(locker);
		freelist.push_back(cmd);
	}

	void GraphicsDevice_Vulkan::DescriptorBinderPool::init(GraphicsDevice_Vulkan* device) {
		this->device = device;

		// Create descriptor pool:
		VkDescriptorPoolSize poolSizes[10] = {};
		uint32_t count = 0;

		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = DESCRIPTORBINDER_CBV_COUNT * poolSize;
		count++;

		poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
		poolSizes[1].descriptorCount = DESCRIPTORBINDER_CBV_COUNT * poolSize;
		count++;
		poolSizes[2].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		poolSizes[2].descriptorCount = DESCRIPTORBINDER_SRV_COUNT * poolSize;
		count++;

		poolSizes[3].type = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
		poolSizes[3].descriptorCount = DESCRIPTORBINDER_SRV_COUNT * poolSize;
		count++;

		poolSizes[4].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		poolSizes[4].descriptorCount = DESCRIPTORBINDER_SRV_COUNT * poolSize;
		count++;

		poolSizes[5].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
		poolSizes[5].descriptorCount = DESCRIPTORBINDER_UAV_COUNT * poolSize;
		count++;

		poolSizes[6].type = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
		poolSizes[6].descriptorCount = DESCRIPTORBINDER_UAV_COUNT * poolSize;
		count++;

		poolSizes[7].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
		poolSizes[7].descriptorCount = DESCRIPTORBINDER_UAV_COUNT * poolSize;
		count++;

		poolSizes[8].type = VK_DESCRIPTOR_TYPE_SAMPLER;
		poolSizes[8].descriptorCount = DESCRIPTORBINDER_SAMPLER_COUNT * poolSize;
		count++;

		if (device->CheckCapability(GraphicsDeviceCapability::RAYTRACING))
		{
			poolSizes[9].type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
			poolSizes[9].descriptorCount = DESCRIPTORBINDER_SRV_COUNT * poolSize;
			count++;
		}
		VkDescriptorPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.poolSizeCount = count;
		poolInfo.pPoolSizes = poolSizes;
		poolInfo.maxSets = poolSize;

		destroy(); // issues destroy if already exists, nop otherwise
		vulkan_check(vkCreateDescriptorPool(device->device, &poolInfo, nullptr, &descriptorPool));

	}
	
	void GraphicsDevice_Vulkan::DescriptorBinderPool::destroy()
	{
		if (descriptorPool != VK_NULL_HANDLE)
		{
			device->allocationhandler->destroylocker.lock();
			device->allocationhandler->destroyer_descriptorPools.push_back(std::make_pair(descriptorPool, device->FRAMECOUNT));
			descriptorPool = VK_NULL_HANDLE;
			device->allocationhandler->destroylocker.unlock();
		}
	}

	void GraphicsDevice_Vulkan::DescriptorBinderPool::reset()
	{
		if (descriptorPool != VK_NULL_HANDLE)
		{
			vulkan_check(vkResetDescriptorPool(device->device, descriptorPool, 0));
		}
	}

	void GraphicsDevice_Vulkan::DescriptorBinder::init(GraphicsDevice_Vulkan* device)
	{
		this->device = device;

		// Important that these don't reallocate themselves during writing descriptors!
		descriptorWrites.reserve(128);
		bufferInfos.reserve(128);
		imageInfos.reserve(128);
		texelBufferViews.reserve(128);
		accelerationStructureViews.reserve(128);
	}

	void GraphicsDevice_Vulkan::DescriptorBinder::reset()
	{
		table = {};
		dirty = true;
	}

	void GraphicsDevice_Vulkan::DescriptorBinder::flush(bool graphics, CommandList cmd)
	{
		if (dirty == DIRTY_NONE)
			return;

		CommandList_Vulkan& commandlist = device->GetCommandList(cmd);
		auto pso_internal = graphics ? to_internal(commandlist.active_pso) : nullptr;
		auto cs_internal = graphics ? nullptr : to_internal(commandlist.active_cs);
		VkCommandBuffer commandBuffer = commandlist.GetCommandBuffer();
		

		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
		VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
		uint32_t uniform_buffer_dynamic_count = 0;
		
		if (graphics)
		{
			pipelineLayout = pso_internal->pipelineLayout;
			descriptorSetLayout = pso_internal->descriptorSetLayout;
			descriptorSet = descriptorSet_graphics;
			uniform_buffer_dynamic_count = (uint32_t)pso_internal->uniform_buffer_dynamic_slots.size();
			for (size_t i = 0; i < pso_internal->uniform_buffer_dynamic_slots.size(); ++i)
			{
				uniform_buffer_dynamic_offsets[i] = (uint32_t)table.CBV_offset[pso_internal->uniform_buffer_dynamic_slots[i]];
			}
		}
		else
		{
			pipelineLayout = cs_internal->pipelineLayout_cs;
			descriptorSetLayout = cs_internal->descriptorSetLayout;
			descriptorSet = descriptorSet_compute;
			uniform_buffer_dynamic_count = (uint32_t)cs_internal->uniform_buffer_dynamic_slots.size();
			for (size_t i = 0; i < cs_internal->uniform_buffer_dynamic_slots.size(); ++i)
			{
				uniform_buffer_dynamic_offsets[i] = (uint32_t)table.CBV_offset[cs_internal->uniform_buffer_dynamic_slots[i]];
			}
		}

		if (dirty & DIRTY_DESCRIPTOR) {
			auto& binder_pool = commandlist.binder_pools[device->GetBufferIndex()];

			VkDescriptorSetAllocateInfo allocInfo = {};
			allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocInfo.descriptorPool = binder_pool.descriptorPool;
			allocInfo.descriptorSetCount = 1;
			allocInfo.pSetLayouts = &descriptorSetLayout;
			
			VkResult res = vkAllocateDescriptorSets(device->device, &allocInfo, &descriptorSet);
			while (res == VK_ERROR_OUT_OF_POOL_MEMORY)
			{
				binder_pool.poolSize *= 2;
				binder_pool.init(device);
				allocInfo.descriptorPool = binder_pool.descriptorPool;
				res = vkAllocateDescriptorSets(device->device, &allocInfo, &descriptorSet);
			}
			vulkan_assert(res >= VK_SUCCESS, "vkAllocateDescriptorSets");

			descriptorWrites.clear();
			bufferInfos.clear();
			imageInfos.clear();
			texelBufferViews.clear();
			accelerationStructureViews.clear();

			const auto& layoutBindings = graphics ? pso_internal->layoutBindings : cs_internal->layoutBindings;
			const auto& imageViewTypes = graphics ? pso_internal->imageViewTypes : cs_internal->imageViewTypes;


			int i = 0;
			for (auto& x : layoutBindings) {
				if (x.pImmutableSamplers != nullptr)
				{
					i++;
					continue;
				}
				VkImageViewType viewtype = imageViewTypes[i++];
				for (uint32_t descriptor_index = 0; descriptor_index < x.descriptorCount; ++descriptor_index)
				{
					uint32_t unrolled_binding = x.binding + descriptor_index;

					descriptorWrites.emplace_back();
					auto& write = descriptorWrites.back();
					write = {};
					write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					write.dstSet = descriptorSet;
					write.dstArrayElement = descriptor_index;
					write.descriptorType = x.descriptorType;
					write.dstBinding = x.binding;
					write.descriptorCount = 1;

					switch (x.descriptorType) {
					case VK_DESCRIPTOR_TYPE_SAMPLER:
					{
						imageInfos.emplace_back();
						write.pImageInfo = &imageInfos.back();
						imageInfos.back() = {};

						const uint32_t original_binding = unrolled_binding - VULKAN_BINDING_SHIFT_S;
						const Sampler& sampler = table.SAM[original_binding];
						if (!sampler.IsValid())
						{
							imageInfos.back().sampler = device->nullSampler;
						}
						else
						{
							imageInfos.back().sampler = to_internal(&sampler)->resource;
						}
					}
					break;

					case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
					{
						imageInfos.emplace_back();
						write.pImageInfo = &imageInfos.back();
						imageInfos.back() = {};

						const uint32_t original_binding = unrolled_binding - VULKAN_BINDING_SHIFT_T;
						const GPUResource& resource = table.SRV[original_binding];
						if (!resource.IsValid() || !resource.IsTexture())
						{
							switch (viewtype)
							{
							case VK_IMAGE_VIEW_TYPE_1D:
								imageInfos.back().imageView = device->nullImageView1D;
								break;
							case VK_IMAGE_VIEW_TYPE_2D:
								imageInfos.back().imageView = device->nullImageView2D;
								break;
							case VK_IMAGE_VIEW_TYPE_3D:
								imageInfos.back().imageView = device->nullImageView3D;
								break;
							case VK_IMAGE_VIEW_TYPE_CUBE:
								imageInfos.back().imageView = device->nullImageViewCube;
								break;
							case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
								imageInfos.back().imageView = device->nullImageView1DArray;
								break;
							case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
								imageInfos.back().imageView = device->nullImageView2DArray;
								break;
							case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
								imageInfos.back().imageView = device->nullImageViewCubeArray;
								break;
							case VK_IMAGE_VIEW_TYPE_MAX_ENUM:
								break;
							default:
								break;
							}
							imageInfos.back().imageLayout = VK_IMAGE_LAYOUT_GENERAL;
						}
						else
						{
							int subresource = table.SRV_index[original_binding];
							auto texture_internal = to_internal((const Texture*)&resource);
							auto& subresource_descriptor = subresource >= 0 ? texture_internal->subresources_srv[subresource] : texture_internal->srv;
							imageInfos.back().imageView = subresource_descriptor.image_view;
							imageInfos.back().imageLayout = texture_internal->defaultLayout;
						}
					}
					break;


					case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
					{
						imageInfos.emplace_back();
						write.pImageInfo = &imageInfos.back();
						imageInfos.back() = {};
						imageInfos.back().imageLayout = VK_IMAGE_LAYOUT_GENERAL;

						const uint32_t original_binding = unrolled_binding - VULKAN_BINDING_SHIFT_U;
						const GPUResource& resource = table.UAV[original_binding];
						if (!resource.IsValid() || !resource.IsTexture())
						{
							switch (viewtype)
							{
							case VK_IMAGE_VIEW_TYPE_1D:
								imageInfos.back().imageView = device->nullImageView1D;
								break;
							case VK_IMAGE_VIEW_TYPE_2D:
								imageInfos.back().imageView = device->nullImageView2D;
								break;
							case VK_IMAGE_VIEW_TYPE_3D:
								imageInfos.back().imageView = device->nullImageView3D;
								break;
							case VK_IMAGE_VIEW_TYPE_CUBE:
								imageInfos.back().imageView = device->nullImageViewCube;
								break;
							case VK_IMAGE_VIEW_TYPE_1D_ARRAY:
								imageInfos.back().imageView = device->nullImageView1DArray;
								break;
							case VK_IMAGE_VIEW_TYPE_2D_ARRAY:
								imageInfos.back().imageView = device->nullImageView2DArray;
								break;
							case VK_IMAGE_VIEW_TYPE_CUBE_ARRAY:
								imageInfos.back().imageView = device->nullImageViewCubeArray;
								break;
							case VK_IMAGE_VIEW_TYPE_MAX_ENUM:
								break;
							default:
								break;
							}
						}
						else
						{
							int subresource = table.UAV_index[original_binding];
							auto texture_internal = to_internal((const Texture*)&resource);
							auto& subresource_descriptor = subresource >= 0 ? texture_internal->subresources_uav[subresource] : texture_internal->uav;
							imageInfos.back().imageView = subresource_descriptor.image_view;
						}
					}
					break;

					case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
					{
						bufferInfos.emplace_back();
						write.pBufferInfo = &bufferInfos.back();
						bufferInfos.back() = {};

						const uint32_t original_binding = unrolled_binding - VULKAN_BINDING_SHIFT_B;
						const GPUBuffer& buffer = table.CBV[original_binding];
						uint64_t offset = table.CBV_offset[original_binding];

						if (!buffer.IsValid())
						{
							bufferInfos.back().buffer = device->nullBuffer;
							bufferInfos.back().range = VK_WHOLE_SIZE;
						}
						else
						{
							auto internal_state = to_internal(&buffer);
							bufferInfos.back().buffer = internal_state->resource;
							bufferInfos.back().offset = offset;
							if (graphics)
							{
								bufferInfos.back().range = pso_internal->uniform_buffer_sizes[original_binding];
							}
							else
							{
								bufferInfos.back().range = cs_internal->uniform_buffer_sizes[original_binding];
							}
							if (bufferInfos.back().range == 0ull)
							{
								bufferInfos.back().range = VK_WHOLE_SIZE;
							}
						}
					}
					break;

					case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
					{
						bufferInfos.emplace_back();
						write.pBufferInfo = &bufferInfos.back();
						bufferInfos.back() = {};

						const uint32_t original_binding = unrolled_binding - VULKAN_BINDING_SHIFT_B;
						const GPUBuffer& buffer = table.CBV[original_binding];

						if (!buffer.IsValid())
						{
							bufferInfos.back().buffer = device->nullBuffer;
							bufferInfos.back().range = VK_WHOLE_SIZE;
						}
						else
						{
							auto internal_state = to_internal(&buffer);
							bufferInfos.back().buffer = internal_state->resource;
							if (graphics)
							{
								bufferInfos.back().range = pso_internal->uniform_buffer_sizes[original_binding];
							}
							else
							{
								bufferInfos.back().range = cs_internal->uniform_buffer_sizes[original_binding];
							}
							if (bufferInfos.back().range == 0ull)
							{
								bufferInfos.back().range = VK_WHOLE_SIZE;
							}
						}
					}
					break;

					case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
					{
						texelBufferViews.emplace_back();
						write.pTexelBufferView = &texelBufferViews.back();
						texelBufferViews.back() = {};

						const uint32_t original_binding = unrolled_binding - VULKAN_BINDING_SHIFT_T;
						const GPUResource& resource = table.SRV[original_binding];
						if (!resource.IsValid() || !resource.IsBuffer())
						{
							texelBufferViews.back() = device->nullBufferView;
						}
						else
						{
							int subresource = table.SRV_index[original_binding];
							auto buffer_internal = to_internal((const GPUBuffer*)&resource);
							auto& subresource_descriptor = subresource >= 0 ? buffer_internal->subresources_srv[subresource] : buffer_internal->srv;
							texelBufferViews.back() = subresource_descriptor.buffer_view;
						}
					}
					break;

					case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
					{
						texelBufferViews.emplace_back();
						write.pTexelBufferView = &texelBufferViews.back();
						texelBufferViews.back() = {};

						const uint32_t original_binding = unrolled_binding - VULKAN_BINDING_SHIFT_U;
						const GPUResource& resource = table.UAV[original_binding];
						if (!resource.IsValid() || !resource.IsBuffer())
						{
							texelBufferViews.back() = device->nullBufferView;
						}
						else
						{
							int subresource = table.UAV_index[original_binding];
							auto buffer_internal = to_internal((const GPUBuffer*)&resource);
							auto& subresource_descriptor = subresource >= 0 ? buffer_internal->subresources_uav[subresource] : buffer_internal->uav;
							texelBufferViews.back() = subresource_descriptor.buffer_view;
						}
					}
					break;
		

					case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
					{
						bufferInfos.emplace_back();
						write.pBufferInfo = &bufferInfos.back();
						bufferInfos.back() = {};

						if (x.binding < VULKAN_BINDING_SHIFT_U)
						{
							// SRV
							const uint32_t original_binding = unrolled_binding - VULKAN_BINDING_SHIFT_T;
							const GPUResource& resource = table.SRV[original_binding];
							if (!resource.IsValid() || !resource.IsBuffer())
							{
								bufferInfos.back().buffer = device->nullBuffer;
								bufferInfos.back().range = VK_WHOLE_SIZE;
							}
							else
							{
								int subresource = table.SRV_index[original_binding];
								auto buffer_internal = to_internal((const GPUBuffer*)&resource);
								auto& subresource_descriptor = subresource >= 0 ? buffer_internal->subresources_srv[subresource] : buffer_internal->srv;
								bufferInfos.back() = subresource_descriptor.buffer_info;
							}
						}
						else
						{
							// UAV
							const uint32_t original_binding = unrolled_binding - VULKAN_BINDING_SHIFT_U;
							const GPUResource& resource = table.UAV[original_binding];
							if (!resource.IsValid() || !resource.IsBuffer())
							{
								bufferInfos.back().buffer = device->nullBuffer;
								bufferInfos.back().range = VK_WHOLE_SIZE;
							}
							else
							{
								int subresource = table.UAV_index[original_binding];
								auto buffer_internal = to_internal((const GPUBuffer*)&resource);
								auto& subresource_descriptor = subresource >= 0 ? buffer_internal->subresources_uav[subresource] : buffer_internal->uav;
								bufferInfos.back() = subresource_descriptor.buffer_info;
							}
						}
					}
					break;

					case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR:
					{
						accelerationStructureViews.emplace_back();
						write.pNext = &accelerationStructureViews.back();
						accelerationStructureViews.back() = {};
						accelerationStructureViews.back().sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
						accelerationStructureViews.back().accelerationStructureCount = 1;

						const uint32_t original_binding = unrolled_binding - VULKAN_BINDING_SHIFT_T;
						const GPUResource& resource = table.SRV[original_binding];
						if (!resource.IsValid() || !resource.IsAccelerationStructure())
						{
							assert(0); // invalid acceleration structure!
						}
						else
						{
							auto as_internal = to_internal((const RaytracingAccelerationStructure*)&resource);
							accelerationStructureViews.back().pAccelerationStructures = &as_internal->resource;
						}
					}
					break;

					default: break;
					}

				}
			}

			vkUpdateDescriptorSets(
				device->device,
				(uint32_t)descriptorWrites.size(),
				descriptorWrites.data(),
				0,
				nullptr
			);
		}

		VkPipelineBindPoint bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		if (!graphics)
		{
			bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE;

			if (commandlist.active_cs->stage == ShaderStage::LIB)
			{
				bindPoint = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;
			}
		}

		vkCmdBindDescriptorSets(
			commandBuffer,
			bindPoint,
			pipelineLayout,
			0,
			1,
			&descriptorSet,
			uniform_buffer_dynamic_count,
			uniform_buffer_dynamic_offsets
		);

		// Save last used descriptor set handles:
		//	This is needed to handle the case when descriptorSet is not allocated, but only dynamic offsets are updated
		if (graphics)
		{
			descriptorSet_graphics = descriptorSet;
		}
		else
		{
			descriptorSet_compute = descriptorSet;
		}

		dirty = DIRTY_NONE;
	}
	void GraphicsDevice_Vulkan::pso_validate(CommandList cmd) {
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		if (!commandlist.dirty_pso)
			return;

		const PipelineState* pso = commandlist.active_pso;
		const PipelineHash& pipeline_hash = commandlist.prev_pipeline_hash;
		auto internal_state = to_internal(pso);

		VkPipeline pipeline = VK_NULL_HANDLE;
		auto it = pipelines_global.find(pipeline_hash);
		if (it == pipelines_global.end()) {
			for (auto& x : commandlist.pipelines_worker)
			{
				if (pipeline_hash == x.first)
				{
					pipeline = x.second;
					break;
				}
			}
			if (pipeline == VK_NULL_HANDLE) {
				VkGraphicsPipelineCreateInfo pipelineInfo = internal_state->pipelineInfo; // make a copy here

				// MSAA:
				VkPipelineMultisampleStateCreateInfo multisampling = {};
				multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
				multisampling.sampleShadingEnable = VK_FALSE;
				multisampling.rasterizationSamples = (VkSampleCountFlagBits)commandlist.renderpass_info.sample_count;
				if (pso->desc.rs != nullptr)
				{
					const RasterizerState& desc = *pso->desc.rs;
					if (desc.forced_sample_count > 1)
					{
						multisampling.rasterizationSamples = (VkSampleCountFlagBits)desc.forced_sample_count;
					}
				}

				multisampling.minSampleShading = 1.0f;
				VkSampleMask samplemask = internal_state->samplemask;
				samplemask = pso->desc.sample_mask;
				multisampling.pSampleMask = &samplemask;
				if (pso->desc.bs != nullptr)
				{
					multisampling.alphaToCoverageEnable = pso->desc.bs->alpha_to_coverage_enable ? VK_TRUE : VK_FALSE;
				}
				else
				{
					multisampling.alphaToCoverageEnable = VK_FALSE;
				}
				multisampling.alphaToOneEnable = VK_FALSE;

				pipelineInfo.pMultisampleState = &multisampling;

				// Blending:
				uint32_t numBlendAttachments = 0;
				VkPipelineColorBlendAttachmentState colorBlendAttachments[8] = {};
				for (size_t i = 0; i < commandlist.renderpass_info.rt_count; ++i)
				{
					size_t attachmentIndex = 0;
					if (pso->desc.bs->independent_blend_enable)
						attachmentIndex = i;

					const auto& desc = pso->desc.bs->render_target[attachmentIndex];
					VkPipelineColorBlendAttachmentState& attachment = colorBlendAttachments[numBlendAttachments];
					numBlendAttachments++;

					attachment.blendEnable = desc.blend_enable ? VK_TRUE : VK_FALSE;

					attachment.colorWriteMask = 0;
					if (has_flag(desc.render_target_write_mask, ColorWrite::ENABLE_RED))
					{
						attachment.colorWriteMask |= VK_COLOR_COMPONENT_R_BIT;
					}
					if (has_flag(desc.render_target_write_mask, ColorWrite::ENABLE_GREEN))
					{
						attachment.colorWriteMask |= VK_COLOR_COMPONENT_G_BIT;
					}
					if (has_flag(desc.render_target_write_mask, ColorWrite::ENABLE_BLUE))
					{
						attachment.colorWriteMask |= VK_COLOR_COMPONENT_B_BIT;
					}
					if (has_flag(desc.render_target_write_mask, ColorWrite::ENABLE_ALPHA))
					{
						attachment.colorWriteMask |= VK_COLOR_COMPONENT_A_BIT;
					}

					attachment.srcColorBlendFactor = _ConvertBlend(desc.src_blend);
					attachment.dstColorBlendFactor = _ConvertBlend(desc.dest_blend);
					attachment.colorBlendOp = _ConvertBlendOp(desc.blend_op);
					attachment.srcAlphaBlendFactor = _ConvertBlend(desc.src_blend_alpha);
					attachment.dstAlphaBlendFactor = _ConvertBlend(desc.dest_blend_alpha);
					attachment.alphaBlendOp = _ConvertBlendOp(desc.blend_op_alpha);
				}


				VkPipelineColorBlendStateCreateInfo colorBlending = {};
				colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
				colorBlending.logicOpEnable = VK_FALSE;
				colorBlending.logicOp = VK_LOGIC_OP_COPY;
				colorBlending.attachmentCount = numBlendAttachments;
				colorBlending.pAttachments = colorBlendAttachments;
				colorBlending.blendConstants[0] = 1.0f;
				colorBlending.blendConstants[1] = 1.0f;
				colorBlending.blendConstants[2] = 1.0f;
				colorBlending.blendConstants[3] = 1.0f;

				pipelineInfo.pColorBlendState = &colorBlending;

				// Input layout:
				VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
				vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
				vector<VkVertexInputBindingDescription> bindings;
				vector<VkVertexInputAttributeDescription> attributes;

				if (pso->desc.il != nullptr) {
					uint32_t lastBinding = 0xFFFFFFFF;
					for (auto& x : pso->desc.il->elements)
					{
						if (x.input_slot == lastBinding)
							continue;
						lastBinding = x.input_slot;
						VkVertexInputBindingDescription& bind = bindings.emplace_back();
						bind.binding = x.input_slot;
						bind.inputRate = x.input_slot_class == InputClassification::PER_VERTEX_DATA ? VK_VERTEX_INPUT_RATE_VERTEX : VK_VERTEX_INPUT_RATE_INSTANCE;
						bind.stride = GetFormatStride(x.format);
					}
					uint32_t offset = 0;
					uint32_t i = 0;
					lastBinding = 0xFFFFFFFF;

					for (auto& x : pso->desc.il->elements)
					{
						VkVertexInputAttributeDescription attr = {};
						attr.binding = x.input_slot;
						if (attr.binding != lastBinding)
						{
							lastBinding = attr.binding;
							offset = 0;
						}
						attr.format = _ConvertFormat(x.format);
						attr.location = i;
						attr.offset = x.aligned_byte_offset;
						if (attr.offset == InputLayout::APPEND_ALIGNED_ELEMENT)
						{
							// need to manually resolve this from the format spec.
							attr.offset = offset;
							offset += GetFormatStride(x.format);
						}

						attributes.push_back(attr);

						i++;
					}

					vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
					vertexInputInfo.pVertexBindingDescriptions = bindings.data();
					vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
					vertexInputInfo.pVertexAttributeDescriptions = attributes.data();

				}


				pipelineInfo.renderPass = VK_NULL_HANDLE; // instead we use VkPipelineRenderingCreateInfo

				VkPipelineRenderingCreateInfo renderingInfo = {};
				renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
				renderingInfo.viewMask = 0;
				renderingInfo.colorAttachmentCount = commandlist.renderpass_info.rt_count;
				VkFormat formats[8] = {};
				for (uint32_t i = 0; i < commandlist.renderpass_info.rt_count; ++i)
				{
					formats[i] = _ConvertFormat(commandlist.renderpass_info.rt_formats[i]);
				}
				renderingInfo.pColorAttachmentFormats = formats;
				renderingInfo.depthAttachmentFormat = _ConvertFormat(commandlist.renderpass_info.ds_format);
				if (IsFormatStencilSupport(commandlist.renderpass_info.ds_format))
				{
					renderingInfo.stencilAttachmentFormat = renderingInfo.depthAttachmentFormat;
				}
				pipelineInfo.pNext = &renderingInfo;

				vulkan_check(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineInfo, nullptr, &pipeline));

				commandlist.pipelines_worker.push_back(std::make_pair(pipeline_hash, pipeline));

			}
		}
		else
		{
		pipeline = it->second;
		}
		assert(pipeline != VK_NULL_HANDLE);

		vkCmdBindPipeline(commandlist.GetCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
		commandlist.dirty_pso = false;
	}

	void GraphicsDevice_Vulkan::predraw(CommandList cmd)
	{
		pso_validate(cmd);

		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		commandlist.binder.flush(true, cmd);
	}

	void GraphicsDevice_Vulkan::predispatch(CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		commandlist.binder.flush(false, cmd);
	}


	// Engine functions
	GraphicsDevice_Vulkan::GraphicsDevice_Vulkan(platform::window_type window, ValidationMode validationMode_, GPUPreference preference)
	{
	//	Timer timer;
	//	capabilities |= GraphicsDeviceCapability::ALIASING_GENERIC;
	//	// This functionalty is missing from Vulkan but might be added in the future:
	//	//	Issue: https://github.com/KhronosGroup/Vulkan-Docs/issues/2079
	//	capabilities |= GraphicsDeviceCapability::COPY_BETWEEN_DIFFERENT_IMAGE_ASPECTS_NOT_SUPPORTED;
	//	unordered_map<uint32_t, std::shared_ptr<std::mutex>> queue_lockers;

	//	TOPLEVEL_ACCELERATION_STRUCTURE_INSTANCE_SIZE = sizeof(VkAccelerationStructureInstanceKHR);
	//	validationMode = validationMode_;

	//	VkResult res;

	//	// Fill out application info:
	//	VkApplicationInfo appInfo = {};
	//	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	//	appInfo.pApplicationName = "Wicked Engine Application";
	//	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	//	appInfo.pEngineName = "Wicked Engine";
	//	appInfo.engineVersion = VK_MAKE_VERSION(version::GetMajor(), version::GetMinor(), version::GetRevision());
	//	appInfo.apiVersion = VK_API_VERSION_1_3;
	}

	bool GraphicsDevice_Vulkan::CreateSwapChain(const SwapChainDesc* desc,platform::window_type window, SwapChain* swapchain) const
	{
		return true;
	}
	bool GraphicsDevice_Vulkan::CreateBuffer2(const GPUBufferDesc* desc, const std::function<void(void*)>& init_callback, GPUBuffer* buffer, const GPUResource* alias, uint64_t alias_offset) const
	{
		return true;
	}
	bool GraphicsDevice_Vulkan::CreateTexture(const TextureDesc* desc, const SubresourceData* initial_data, Texture* texture, const GPUResource* alias, uint64_t alias_offset) const
	{
		return true;
	}
	bool GraphicsDevice_Vulkan::CreateShader(ShaderStage stage, const void* shadercode, size_t shadercode_size, Shader* shader) const
	{
		return true;
	}
	
	bool GraphicsDevice_Vulkan::CreateSampler(const SamplerDesc* desc, Sampler* sampler) const
	{
		return true;
	}

	bool GraphicsDevice_Vulkan::CreateRaytracingPipelineState(const RaytracingPipelineStateDesc* desc, RaytracingPipelineState* rtpso) const
	{
		return true;
	}
	bool GraphicsDevice_Vulkan::CreateVideoDecoder(const VideoDesc* desc, VideoDecoder* video_decoder) const
	{
		return true;
	}

	int GraphicsDevice_Vulkan::CreateSubresource(Texture* texture, SubresourceType type, uint32_t firstSlice, uint32_t sliceCount, uint32_t firstMip, uint32_t mipCount, const Format* format_change, const ImageAspect* aspect, const Swizzle* swizzle, float min_lod_clamp) const
	{
		return 0;
	}
	int GraphicsDevice_Vulkan::CreateSubresource(GPUBuffer* buffer, SubresourceType type, uint64_t offset, uint64_t size, const Format* format_change, const uint32_t* structuredbuffer_stride_change) const
	{
		return 0;
	}
	void GraphicsDevice_Vulkan::DeleteSubresources(GPUResource* resource)
	{
	
	}
	int GraphicsDevice_Vulkan::GetDescriptorIndex(const GPUResource* resource, SubresourceType type, int subresource) const
	{
		return 0;
	}
	int GraphicsDevice_Vulkan::GetDescriptorIndex(const Sampler* sampler) const
	{
		return 0;
	}
	void GraphicsDevice_Vulkan::WriteShadingRateValue(ShadingRate rate, void* dest) const
	{
	
	}
	void GraphicsDevice_Vulkan::WriteTopLevelAccelerationStructureInstance(const RaytracingAccelerationStructureDesc::TopLevel::Instance* instance, void* dest) const
	{
	
	}
	void GraphicsDevice_Vulkan::WriteShaderIdentifier(const RaytracingPipelineState* rtpso, uint32_t group_index, void* dest) const
	{
	
	}




	void GraphicsDevice_Vulkan::SetName(Shader* shader, const char* name) const
	{
	
	}

	CommandList GraphicsDevice_Vulkan::BeginCommandList(QUEUE_TYPE queue)
	{
		return CommandList();
	}

	void GraphicsDevice_Vulkan::SubmitCommandLists()
	{
	
	}
	GraphicsDevice_Vulkan::~GraphicsDevice_Vulkan() {
		
	}
	void GraphicsDevice_Vulkan::WaitForGPU() const
	{
		vulkan_check(vkDeviceWaitIdle(device));
	}

	void GraphicsDevice_Vulkan::ClearPipelineStateCache()
	{
	
	}

	Texture GraphicsDevice_Vulkan::GetBackBuffer(const SwapChain* swapchain) const
	{
		return Texture();
	}

	ColorSpace GraphicsDevice_Vulkan::GetSwapChainColorSpace(const SwapChain* swapchain) const
	{
		auto internal_state = to_internal(swapchain);
		return internal_state->colorSpace;
	}

	bool GraphicsDevice_Vulkan::IsSwapChainSupportsHDR(const SwapChain* swapchain) const
	{
		auto internal_state = to_internal(swapchain);

		uint32_t formatCount;
		VkResult res = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, internal_state->surface, &formatCount, nullptr);
		if (res == VK_SUCCESS)
		{
			 vector<VkSurfaceFormatKHR> swapchain_formats(formatCount);
			res = vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, internal_state->surface, &formatCount, swapchain_formats.data());
			if (res == VK_SUCCESS)
			{
				for (const auto& format : swapchain_formats)
				{
					if (format.colorSpace != VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
					{
						return true;
					}
				}
			}
		}
		return false;
	}

	void GraphicsDevice_Vulkan::SparseUpdate(QUEUE_TYPE queue, const SparseUpdateCommand* commands, uint32_t command_count)
	{
		thread_local vector<VkBindSparseInfo> sparse_infos;
		struct DataPerBind
		{
			VkSparseBufferMemoryBindInfo buffer_bind_info;
			VkSparseImageOpaqueMemoryBindInfo image_opaque_bind_info;
			VkSparseImageMemoryBindInfo image_bind_info;
			vector<VkSparseMemoryBind> memory_binds;
			vector<VkSparseImageMemoryBind> image_memory_binds;
		};
		thread_local vector<DataPerBind> sparse_binds;

		sparse_infos.resize(command_count);
		sparse_binds.resize(command_count);

		for (uint32_t i = 0; i < command_count; ++i)
		{
			const SparseUpdateCommand& in_command = commands[i];
			VkBindSparseInfo& out_info = sparse_infos[i];
			out_info = {};
			out_info.sType = VK_STRUCTURE_TYPE_BIND_SPARSE_INFO;

			DataPerBind& out_bind = sparse_binds[i];

			VkDeviceMemory tile_pool_memory = VK_NULL_HANDLE;
			VkDeviceSize tile_pool_offset = 0;
			if (in_command.tile_pool != nullptr)
			{
				auto internal_tile_pool = to_internal(in_command.tile_pool);
				//tile_pool_memory = internal_tile_pool->allocation->GetMemory();
				//tile_pool_offset = internal_tile_pool->allocation->GetOffset();
			}

			out_bind.memory_binds.clear();
			out_bind.image_memory_binds.clear();

			out_bind.memory_binds.reserve(in_command.num_resource_regions);
			out_bind.image_memory_binds.reserve(in_command.num_resource_regions);

			const VkSparseMemoryBind* memory_bind_ptr = out_bind.memory_binds.data();
			const VkSparseImageMemoryBind* image_memory_bind_ptr = out_bind.image_memory_binds.data();

			if (in_command.sparse_resource->IsBuffer())
			{
				auto internal_sparse = to_internal((const GPUBuffer*)in_command.sparse_resource);

				VkSparseBufferMemoryBindInfo& info = out_bind.buffer_bind_info;
				info = {};
				info.buffer = internal_sparse->resource;
				info.pBinds = memory_bind_ptr;
				info.bindCount = in_command.num_resource_regions;
				memory_bind_ptr += in_command.num_resource_regions;

				for (uint32_t j = 0; j < in_command.num_resource_regions; ++j)
				{
					const SparseResourceCoordinate& in_coordinate = in_command.coordinates[j];
					const SparseRegionSize& in_size = in_command.sizes[j];

					const TileRangeFlags& in_flags = in_command.range_flags[j];
					uint32_t in_offset = in_command.range_start_offsets[j];
					uint32_t in_tile_count = in_command.range_tile_counts[j];
					VkSparseMemoryBind& out_memory_bind = out_bind.memory_binds.emplace_back();
					out_memory_bind = {};
					out_memory_bind.resourceOffset = in_coordinate.x * in_command.sparse_resource->sparse_page_size;
					out_memory_bind.size = in_tile_count * in_command.sparse_resource->sparse_page_size;
					if (in_flags == TileRangeFlags::Null)
					{
						out_memory_bind.memory = VK_NULL_HANDLE;
					}
					else
					{
						out_memory_bind.memory = tile_pool_memory;
						out_memory_bind.memoryOffset = tile_pool_offset + in_offset * in_command.sparse_resource->sparse_page_size;
					}
				}

				if (info.bindCount > 0)
				{
					out_info.pBufferBinds = &out_bind.buffer_bind_info;
					out_info.bufferBindCount = 1;
				}
			}
			else if (in_command.sparse_resource->IsTexture())
			{
				const Texture* sparse_texture = (const Texture*)in_command.sparse_resource;
				const TextureDesc& texture_desc = sparse_texture->GetDesc();
				auto internal_sparse = to_internal(sparse_texture);

				VkImageAspectFlags aspectMask = {};
				if (IsFormatDepthSupport(texture_desc.format))
				{
					aspectMask |= VK_IMAGE_ASPECT_DEPTH_BIT;
					if (IsFormatStencilSupport(texture_desc.format))
					{
						aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
					}
				}
				if (has_flag(texture_desc.bind_flags, BindFlag::RENDER_TARGET) ||
					has_flag(texture_desc.bind_flags, BindFlag::SHADER_RESOURCE) ||
					has_flag(texture_desc.bind_flags, BindFlag::UNORDERED_ACCESS))
				{
					aspectMask |= VK_IMAGE_ASPECT_COLOR_BIT;
				}

				VkSparseImageOpaqueMemoryBindInfo& opaque_info = out_bind.image_opaque_bind_info;
				opaque_info = {};
				opaque_info.image = internal_sparse->resource;
				opaque_info.pBinds = memory_bind_ptr;
				opaque_info.bindCount = 0;

				VkSparseImageMemoryBindInfo& info = out_bind.image_bind_info;
				info = {};
				info.image = internal_sparse->resource;
				info.pBinds = image_memory_bind_ptr;
				info.bindCount = 0;

				for (uint32_t j = 0; j < in_command.num_resource_regions; ++j)
				{
					const SparseResourceCoordinate& in_coordinate = in_command.coordinates[j];
					const SparseRegionSize& in_size = in_command.sizes[j];
					const bool is_miptail = in_coordinate.mip >= internal_sparse->sparse_texture_properties.packed_mip_start;

					if (is_miptail)
					{
						opaque_info.bindCount++;
						memory_bind_ptr++;

						const TileRangeFlags& in_flags = in_command.range_flags[j];
						uint32_t in_offset = in_command.range_start_offsets[j];
						uint32_t in_tile_count = in_command.range_tile_counts[j];
						VkSparseMemoryBind& out_memory_bind = out_bind.memory_binds.emplace_back();
						out_memory_bind = {};
						out_memory_bind.resourceOffset = internal_sparse->sparse_texture_properties.packed_mip_tile_offset * sparse_texture->sparse_page_size;
						out_memory_bind.size = in_tile_count * in_command.sparse_resource->sparse_page_size;
						if (in_flags == TileRangeFlags::Null)
						{
							out_memory_bind.memory = VK_NULL_HANDLE;
						}
						else
						{
							out_memory_bind.memory = tile_pool_memory;
							out_memory_bind.memoryOffset = tile_pool_offset + in_offset * in_command.sparse_resource->sparse_page_size;
						}
					}
					else
					{
						info.bindCount++;
						image_memory_bind_ptr++;

						const TileRangeFlags& in_flags = in_command.range_flags[j];
						uint32_t in_offset = in_command.range_start_offsets[j];
						uint32_t in_tile_count = in_command.range_tile_counts[j];
						VkSparseImageMemoryBind& out_image_memory_bind = out_bind.image_memory_binds.emplace_back();
						out_image_memory_bind = {};
						if (in_flags == TileRangeFlags::Null)
						{
							out_image_memory_bind.memory = VK_NULL_HANDLE;
						}
						else
						{
							out_image_memory_bind.memory = tile_pool_memory;
							out_image_memory_bind.memoryOffset = tile_pool_offset + in_offset * in_command.sparse_resource->sparse_page_size;
						}
						out_image_memory_bind.subresource.mipLevel = in_coordinate.mip;
						out_image_memory_bind.subresource.arrayLayer = in_coordinate.slice;
						out_image_memory_bind.subresource.aspectMask = aspectMask;
						out_image_memory_bind.offset.x = in_coordinate.x * internal_sparse->sparse_texture_properties.tile_width;
						out_image_memory_bind.offset.y = in_coordinate.y * internal_sparse->sparse_texture_properties.tile_height;
						out_image_memory_bind.offset.z = in_coordinate.z * internal_sparse->sparse_texture_properties.tile_depth;
						out_image_memory_bind.extent.width = std::min(texture_desc.width, in_size.width * internal_sparse->sparse_texture_properties.tile_width);
						out_image_memory_bind.extent.height = std::min(texture_desc.height, in_size.height * internal_sparse->sparse_texture_properties.tile_height);
						out_image_memory_bind.extent.depth = std::min(texture_desc.depth, in_size.depth * internal_sparse->sparse_texture_properties.tile_depth);
					}

				}

				if (opaque_info.bindCount > 0)
				{
					out_info.pImageOpaqueBinds = &out_bind.image_opaque_bind_info;
					out_info.imageOpaqueBindCount = 1;
				}
				if (info.bindCount > 0)
				{
					out_info.pImageBinds = &out_bind.image_bind_info;
					out_info.imageBindCount = 1;
				}

			}

		}

		// Queue command:
		{
			CommandQueue* q = &queues[queue];
			if (!q->sparse_binding_supported)
			{
				// 1.) fall back to any sparse supporting queue
				q = &queue_sparse;
			}
			std::scoped_lock lock(*q->locker);
			log_assert(q->sparse_binding_supported, "Vulkan sparse mapping was used while the feature is not available! This can result in broken rendering or crash. Try to update the graphics driver if this happens.");

			vulkan_check(vkQueueBindSparse(q->queue, (uint32_t)sparse_infos.size(), sparse_infos.data(), VK_NULL_HANDLE));
		}
	}

	void GraphicsDevice_Vulkan::WaitCommandList(CommandList cmd, CommandList wait_for)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		CommandList_Vulkan& commandlist_wait_for = GetCommandList(wait_for);
		assert(commandlist_wait_for.id < commandlist.id); // can't wait for future command list!
		VkSemaphore semaphore = new_semaphore();
		commandlist.waits.push_back(semaphore);
		commandlist_wait_for.signals.push_back(semaphore);
	}
	void GraphicsDevice_Vulkan::RenderPassBegin(const SwapChain* swapchain, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		commandlist.renderpass_barriers_begin.clear();
		commandlist.renderpass_barriers_end.clear();
		auto internal_state = to_internal(swapchain);

		internal_state->locker.lock();
		internal_state->swapChainAcquireSemaphoreIndex = (internal_state->swapChainAcquireSemaphoreIndex + 1) % internal_state->swapchainAcquireSemaphores.size();
		VkResult res;
		do {
			res = vkAcquireNextImageKHR(
				device,
				internal_state->swapChain,
				timeout_value,
				internal_state->swapchainAcquireSemaphores[internal_state->swapChainAcquireSemaphoreIndex],
				VK_NULL_HANDLE,
				&internal_state->swapChainImageIndex
			);
			if (res == VK_TIMEOUT)
			{
				log_error("vkAcquireNextImageKHR resulted in VK_TIMEOUT, retrying");
				std::this_thread::yield();
			}
		} while (res == VK_TIMEOUT);
		internal_state->locker.unlock();

		if (res != VK_SUCCESS)
		{
			// Handle outdated error in acquire:
			if (res == VK_SUBOPTIMAL_KHR || res == VK_ERROR_OUT_OF_DATE_KHR)
			{
				// we need to create a new semaphore or jump through a few hoops to
				// wait for the current one to be unsignalled before we can use it again
				// creating a new one is easiest. See also:
				// https://github.com/KhronosGroup/Vulkan-Docs/issues/152
				// https://www.khronos.org/blog/resolving-longstanding-issues-with-wsi
				{
					std::scoped_lock lock(allocationhandler->destroylocker);
					for (auto& x : internal_state->swapchainAcquireSemaphores)
					{
						allocationhandler->destroyer_semaphores.emplace_back(x, allocationhandler->framecount);
					}
				}
				internal_state->swapchainAcquireSemaphores.clear();
				if (CreateSwapChainInternal(internal_state, physicalDevice, device, allocationhandler))
				{
					RenderPassBegin(swapchain, cmd);
					return;
				}
			}
			assert(0);
		}
		commandlist.prev_swapchains.push_back(*swapchain);

		VkRenderingInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		info.renderArea.offset.x = 0;
		info.renderArea.offset.y = 0;
		info.renderArea.extent.width = std::min(swapchain->desc.width, internal_state->swapChainExtent.width);
		info.renderArea.extent.height = std::min(swapchain->desc.height, internal_state->swapChainExtent.height);
		info.layerCount = 1;

		VkRenderingAttachmentInfo color_attachment = {};
		color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		color_attachment.imageView = internal_state->swapChainImageViews[internal_state->swapChainImageIndex];
		color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		color_attachment.clearValue.color.float32[0] = swapchain->desc.clear_color[0];
		color_attachment.clearValue.color.float32[1] = swapchain->desc.clear_color[1];
		color_attachment.clearValue.color.float32[2] = swapchain->desc.clear_color[2];
		color_attachment.clearValue.color.float32[3] = swapchain->desc.clear_color[3];

		info.colorAttachmentCount = 1;
		info.pColorAttachments = &color_attachment;

		VkImageMemoryBarrier2 barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
		barrier.image = internal_state->swapChainImages[internal_state->swapChainImageIndex];
		barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		barrier.srcStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
		barrier.srcAccessMask = VK_ACCESS_2_NONE;
		barrier.dstStageMask = VK_PIPELINE_STAGE_2_ALL_GRAPHICS_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		VkDependencyInfo dependencyInfo = {};
		dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
		dependencyInfo.imageMemoryBarrierCount = 1;
		dependencyInfo.pImageMemoryBarriers = &barrier;
		vkCmdPipelineBarrier2(commandlist.GetCommandBuffer(), &dependencyInfo);

		barrier.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
		barrier.srcAccessMask = VK_ACCESS_2_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_2_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_2_NONE;
		commandlist.renderpass_barriers_end.push_back(barrier);

		vkCmdBeginRendering(commandlist.GetCommandBuffer(), &info);

		commandlist.renderpass_info = RenderPassInfo::from(swapchain->desc);
	}

	
	void GraphicsDevice_Vulkan::RenderPassBegin(const RenderPassImage* images, uint32_t image_count, CommandList cmd, RenderPassFlags flags)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		commandlist.renderpass_barriers_begin.clear();
		commandlist.renderpass_barriers_end.clear();

		VkRenderingInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
		if (has_flag(flags, RenderPassFlags::SUSPENDING))
		{
			info.flags |= VK_RENDERING_SUSPENDING_BIT;
		}
		if (has_flag(flags, RenderPassFlags::RESUMING))
		{
			info.flags |= VK_RENDERING_RESUMING_BIT;
		}
		info.layerCount = 1;
		info.renderArea.offset.x = 0;
		info.renderArea.offset.y = 0;
		info.renderArea.extent.width = properties2.properties.limits.maxFramebufferWidth;
		info.renderArea.extent.height = properties2.properties.limits.maxFramebufferHeight;
		VkRenderingAttachmentInfo color_attachments[8] = {};
		VkRenderingAttachmentInfo depth_attachment = {};
		VkRenderingAttachmentInfo stencil_attachment = {};
		VkRenderingFragmentShadingRateAttachmentInfoKHR shading_rate_attachment = {};
		bool color = false;
		bool depth = false;
		bool stencil = false;
		uint32_t color_resolve_count = 0;
		for (uint32_t i = 0; i < image_count; ++i)
		{
			const RenderPassImage& image = images[i];
			const Texture* texture = image.texture;
			const TextureDesc& desc = texture->GetDesc();
			int subresource = image.subresource;
			auto internal_state = to_internal(texture);

			info.renderArea.extent.width = std::min(info.renderArea.extent.width, desc.width);
			info.renderArea.extent.height = std::min(info.renderArea.extent.height, desc.height);

			VkAttachmentLoadOp loadOp;
			switch (image.loadop)
			{
			default:
			case RenderPassImage::LoadOp::LOAD:
				loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
				break;
			case RenderPassImage::LoadOp::CLEAR:
				loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
				break;
			case RenderPassImage::LoadOp::DONTCARE:
				loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				break;
			}

			VkAttachmentStoreOp storeOp;
			switch (image.storeop)
			{
			default:
			case RenderPassImage::StoreOp::STORE:
				storeOp = VK_ATTACHMENT_STORE_OP_STORE;
				break;
			case RenderPassImage::StoreOp::DONTCARE:
#ifdef PLATFORM_LINUX
				storeOp = VK_ATTACHMENT_STORE_OP_STORE;
#else
				storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
#endif // PLATFORM_LINUX
				break;
			}

			Texture_Vulkan::TextureSubresource descriptor;

			switch (image.type)
			{
			case RenderPassImage::Type::RENDERTARGET:
			{
				descriptor = subresource < 0 ? internal_state->rtv : internal_state->subresources_rtv[subresource];
				VkRenderingAttachmentInfo& color_attachment = color_attachments[info.colorAttachmentCount++];
				color_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
				color_attachment.imageView = descriptor.image_view;
				color_attachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				color_attachment.loadOp = loadOp;
				color_attachment.storeOp = storeOp;
				color_attachment.clearValue.color.float32[0] = desc.clear.color[0];
				color_attachment.clearValue.color.float32[1] = desc.clear.color[1];
				color_attachment.clearValue.color.float32[2] = desc.clear.color[2];
				color_attachment.clearValue.color.float32[3] = desc.clear.color[3];
				color = true;
			}
			break;

			case RenderPassImage::Type::RESOLVE:
			{
				descriptor = subresource < 0 ? internal_state->srv : internal_state->subresources_srv[subresource];
				VkRenderingAttachmentInfo& color_attachment = color_attachments[color_resolve_count++];
				color_attachment.resolveImageView = descriptor.image_view;
				color_attachment.resolveImageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
				color_attachment.resolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
			}
			break;

			case RenderPassImage::Type::DEPTH_STENCIL:
			{
				descriptor = subresource < 0 ? internal_state->dsv : internal_state->subresources_dsv[subresource];
				depth_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
				depth_attachment.imageView = descriptor.image_view;
				if (image.layout == ResourceState::DEPTHSTENCIL_READONLY)
				{
					depth_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL;
				}
				else
				{
					depth_attachment.imageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
				}
				depth_attachment.loadOp = loadOp;
				depth_attachment.storeOp = storeOp;
				depth_attachment.clearValue.depthStencil.depth = desc.clear.depth_stencil.depth;
				depth = true;
				if (IsFormatStencilSupport(desc.format))
				{
					stencil_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
					stencil_attachment.imageView = subresource < 0 ? internal_state->dsv.image_view : internal_state->subresources_dsv[subresource].image_view;
					if (image.layout == ResourceState::DEPTHSTENCIL_READONLY)
					{
						stencil_attachment.imageLayout = VK_IMAGE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL;
					}
					else
					{
						stencil_attachment.imageLayout = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
					}
					stencil_attachment.loadOp = loadOp;
					stencil_attachment.storeOp = storeOp;
					stencil_attachment.clearValue.depthStencil.stencil = desc.clear.depth_stencil.stencil;
					stencil = true;
				}
			}
			break;

			case RenderPassImage::Type::RESOLVE_DEPTH:
			{
				descriptor = subresource < 0 ? internal_state->dsv : internal_state->subresources_dsv[subresource];
				depth_attachment.resolveImageView = descriptor.image_view;
				stencil_attachment.resolveImageView = descriptor.image_view;
				depth_attachment.resolveImageLayout = VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL;
				stencil_attachment.resolveImageLayout = VK_IMAGE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL;
				switch (image.depth_resolve_mode)
				{
				default:
				case RenderPassImage::DepthResolveMode::Min:
					depth_attachment.resolveMode = VK_RESOLVE_MODE_MIN_BIT;
					stencil_attachment.resolveMode = VK_RESOLVE_MODE_MIN_BIT;
					break;
				case RenderPassImage::DepthResolveMode::Max:
					depth_attachment.resolveMode = VK_RESOLVE_MODE_MAX_BIT;
					stencil_attachment.resolveMode = VK_RESOLVE_MODE_MAX_BIT;
					break;
				}
			}
			break;

			case RenderPassImage::Type::SHADING_RATE_SOURCE:
				descriptor = subresource < 0 ? internal_state->uav : internal_state->subresources_uav[subresource];
				shading_rate_attachment.sType = VK_STRUCTURE_TYPE_RENDERING_FRAGMENT_SHADING_RATE_ATTACHMENT_INFO_KHR;
				shading_rate_attachment.imageView = descriptor.image_view;
				shading_rate_attachment.imageLayout = VK_IMAGE_LAYOUT_FRAGMENT_SHADING_RATE_ATTACHMENT_OPTIMAL_KHR;
				shading_rate_attachment.shadingRateAttachmentTexelSize.width = VARIABLE_RATE_SHADING_TILE_SIZE;
				shading_rate_attachment.shadingRateAttachmentTexelSize.height = VARIABLE_RATE_SHADING_TILE_SIZE;
				info.pNext = &shading_rate_attachment;
				break;
			default:
				break;
			}

			if (image.layout_before != image.layout)
			{
				VkImageMemoryBarrier2& barrier = commandlist.renderpass_barriers_begin.emplace_back();
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
				barrier.image = internal_state->resource;
				barrier.oldLayout = _ConvertImageLayout(image.layout_before);
				barrier.newLayout = _ConvertImageLayout(image.layout);

				assert(barrier.newLayout != VK_IMAGE_LAYOUT_UNDEFINED);

				barrier.srcStageMask = _ConvertPipelineStage(image.layout_before);
				barrier.dstStageMask = _ConvertPipelineStage(image.layout);
				barrier.srcAccessMask = _ParseResourceState(image.layout_before);
				barrier.dstAccessMask = _ParseResourceState(image.layout);

				if (IsFormatDepthSupport(desc.format))
				{
					barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
					if (IsFormatStencilSupport(desc.format))
					{
						barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
					}
				}
				else
				{
					barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				}
				barrier.subresourceRange.baseMipLevel = descriptor.firstMip;
				barrier.subresourceRange.levelCount = descriptor.mipCount;
				barrier.subresourceRange.baseArrayLayer = descriptor.firstSlice;
				barrier.subresourceRange.layerCount = descriptor.sliceCount;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			}

			if (image.layout != image.layout_after)
			{
				VkImageMemoryBarrier2& barrier = commandlist.renderpass_barriers_end.emplace_back();
				barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
				barrier.image = internal_state->resource;
				barrier.oldLayout = _ConvertImageLayout(image.layout);
				barrier.newLayout = _ConvertImageLayout(image.layout_after);

				assert(barrier.newLayout != VK_IMAGE_LAYOUT_UNDEFINED);

				barrier.srcStageMask = _ConvertPipelineStage(image.layout);
				barrier.dstStageMask = _ConvertPipelineStage(image.layout_after);
				barrier.srcAccessMask = _ParseResourceState(image.layout);
				barrier.dstAccessMask = _ParseResourceState(image.layout_after);

				if (IsFormatDepthSupport(desc.format))
				{
					barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
					if (IsFormatStencilSupport(desc.format))
					{
						barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
					}
				}
				else
				{
					barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				}
				barrier.subresourceRange.baseMipLevel = descriptor.firstMip;
				barrier.subresourceRange.levelCount = descriptor.mipCount;
				barrier.subresourceRange.baseArrayLayer = descriptor.firstSlice;
				barrier.subresourceRange.layerCount = descriptor.sliceCount;
				barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			}

			info.layerCount = std::min(desc.array_size, std::max(info.layerCount, descriptor.sliceCount));
		}
		info.pColorAttachments = color ? color_attachments : nullptr;
		info.pDepthAttachment = depth ? &depth_attachment : nullptr;
		info.pStencilAttachment = stencil ? &stencil_attachment : nullptr;

		if (!commandlist.renderpass_barriers_begin.empty())
		{
			VkDependencyInfo dependencyInfo = {};
			dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			dependencyInfo.imageMemoryBarrierCount = static_cast<uint32_t>(commandlist.renderpass_barriers_begin.size());
			dependencyInfo.pImageMemoryBarriers = commandlist.renderpass_barriers_begin.data();

			vkCmdPipelineBarrier2(commandlist.GetCommandBuffer(), &dependencyInfo);
		}

		vkCmdBeginRendering(commandlist.GetCommandBuffer(), &info);

		commandlist.renderpass_info = RenderPassInfo::from(images, image_count);
	}


	void GraphicsDevice_Vulkan::RenderPassEnd(CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdEndRendering(commandlist.GetCommandBuffer());

		if (!commandlist.renderpass_barriers_end.empty())
		{
			VkDependencyInfo dependencyInfo = {};
			dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			dependencyInfo.imageMemoryBarrierCount = static_cast<uint32_t>(commandlist.renderpass_barriers_end.size());
			dependencyInfo.pImageMemoryBarriers = commandlist.renderpass_barriers_end.data();

			vkCmdPipelineBarrier2(commandlist.GetCommandBuffer(), &dependencyInfo);
			commandlist.renderpass_barriers_end.clear();
		}

		commandlist.renderpass_info = {};
	}

	void GraphicsDevice_Vulkan::BindScissorRects(uint32_t numRects, const Rect* rects, CommandList cmd)
	{
		assert(rects != nullptr);
		VkRect2D scissors[16];
		assert(numRects <= arraysize(scissors));
		assert(numRects <= properties2.properties.limits.maxViewports);
		for (uint32_t i = 0; i < numRects; ++i)
		{
			scissors[i].extent.width = abs(rects[i].right - rects[i].left);
			scissors[i].extent.height = abs(rects[i].top - rects[i].bottom);
			scissors[i].offset.x = std::max(0, rects[i].left);
			scissors[i].offset.y = std::max(0, rects[i].top);
		}
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdSetScissorWithCount(commandlist.GetCommandBuffer(), numRects, scissors);
	}

	void GraphicsDevice_Vulkan::BindViewports(uint32_t NumViewports, const Viewport* pViewports, CommandList cmd)
	{
		assert(pViewports != nullptr);
		VkViewport vp[16];
		assert(NumViewports < arraysize(vp));
		assert(NumViewports < properties2.properties.limits.maxViewports);
		for (uint32_t i = 0; i < NumViewports; ++i)
		{
			vp[i].x = pViewports[i].top_left_x;
			vp[i].y = pViewports[i].top_left_y + pViewports[i].height;
			vp[i].width = std::max(1.0f, pViewports[i].width); // must be > 0 according to validation layer
			vp[i].height = -pViewports[i].height;
			vp[i].minDepth = pViewports[i].min_depth;
			vp[i].maxDepth = pViewports[i].max_depth;
		}
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdSetViewportWithCount(commandlist.GetCommandBuffer(), NumViewports, vp);
	}

	void GraphicsDevice_Vulkan::BindResource(const GPUResource* resource, uint32_t slot, CommandList cmd, int subresource)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		assert(slot < DESCRIPTORBINDER_SRV_COUNT);
		auto& binder = commandlist.binder;
		if (binder.table.SRV[slot].internal_state != resource->internal_state || binder.table.SRV_index[slot] != subresource)
		{
			binder.table.SRV[slot] = *resource;
			binder.table.SRV_index[slot] = subresource;
			binder.dirty |= DescriptorBinder::DIRTY_DESCRIPTOR;
		}
	}
	void GraphicsDevice_Vulkan::BindResources(const GPUResource* const* resources, uint32_t slot, uint32_t count, CommandList cmd)
	{
		if (resources != nullptr)
		{
			for (uint32_t i = 0; i < count; ++i)
			{
				BindResource(resources[i], slot + i, cmd, -1);
			}
		}
	}
	void GraphicsDevice_Vulkan::BindUAV(const GPUResource* resource, uint32_t slot, CommandList cmd, int subresource)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		assert(slot < DESCRIPTORBINDER_UAV_COUNT);
		auto& binder = commandlist.binder;
		if (binder.table.UAV[slot].internal_state != resource->internal_state || binder.table.UAV_index[slot] != subresource)
		{
			binder.table.UAV[slot] = *resource;
			binder.table.UAV_index[slot] = subresource;
			binder.dirty |= DescriptorBinder::DIRTY_DESCRIPTOR;
		}
	}
	void GraphicsDevice_Vulkan::BindUAVs(const GPUResource* const* resources, uint32_t slot, uint32_t count, CommandList cmd)
	{
		if (resources != nullptr)
		{
			for (uint32_t i = 0; i < count; ++i)
			{
				BindUAV(resources[i], slot + i, cmd, -1);
			}
		}
	}

	void GraphicsDevice_Vulkan::BindSampler(const Sampler* sampler, uint32_t slot, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		assert(slot < DESCRIPTORBINDER_SAMPLER_COUNT);
		auto& binder = commandlist.binder;
		if (binder.table.SAM[slot].internal_state != sampler->internal_state)
		{
			binder.table.SAM[slot] = *sampler;
			binder.dirty |= DescriptorBinder::DIRTY_DESCRIPTOR;
		}
	}
	void GraphicsDevice_Vulkan::BindConstantBuffer(const GPUBuffer* buffer, uint32_t slot, CommandList cmd, uint64_t offset)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		assert(slot < DESCRIPTORBINDER_CBV_COUNT);
		auto& binder = commandlist.binder;

		if (binder.table.CBV[slot].internal_state != buffer->internal_state)
		{
			binder.table.CBV[slot] = *buffer;
			binder.dirty |= DescriptorBinder::DIRTY_DESCRIPTOR;
		}

		if (binder.table.CBV_offset[slot] != offset)
		{
			binder.table.CBV_offset[slot] = offset;
			binder.dirty |= DescriptorBinder::DIRTY_OFFSET;
		}
	}
	void GraphicsDevice_Vulkan::BindVertexBuffers(const GPUBuffer* const* vertexBuffers, uint32_t slot, uint32_t count, const uint32_t* strides, const uint64_t* offsets, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);

		VkDeviceSize voffsets[8] = {};
		VkDeviceSize vstrides[8] = {};
		VkBuffer vbuffers[8] = {};
		assert(count <= 8);
		for (uint32_t i = 0; i < count; ++i)
		{
			if (vertexBuffers[i] == nullptr || !vertexBuffers[i]->IsValid())
			{
				vbuffers[i] = nullBuffer;
			}
			else
			{
				auto internal_state = to_internal(vertexBuffers[i]);
				vbuffers[i] = internal_state->resource;
				if (offsets != nullptr)
				{
					voffsets[i] = offsets[i];
				}
				if (strides != nullptr)
				{
					vstrides[i] = strides[i];
				}
			}
		}

		vkCmdBindVertexBuffers2(commandlist.GetCommandBuffer(), slot, count, vbuffers, voffsets, nullptr, vstrides);
	}
	void GraphicsDevice_Vulkan::BindIndexBuffer(const GPUBuffer* indexBuffer, const IndexBufferFormat format, uint64_t offset, CommandList cmd)
	{
		if (indexBuffer != nullptr)
		{
			auto internal_state = to_internal(indexBuffer);
			CommandList_Vulkan& commandlist = GetCommandList(cmd);
			vkCmdBindIndexBuffer(commandlist.GetCommandBuffer(), internal_state->resource, offset, format == IndexBufferFormat::UINT16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32);
		}
	}
	void GraphicsDevice_Vulkan::BindStencilRef(uint32_t value, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		if (commandlist.prev_stencilref != value)
		{
			commandlist.prev_stencilref = value;
			vkCmdSetStencilReference(commandlist.GetCommandBuffer(), VK_STENCIL_FRONT_AND_BACK, value);
		}
	}
	void GraphicsDevice_Vulkan::BindBlendFactor(float r, float g, float b, float a, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		float blendConstants[] = { r, g, b, a };
		vkCmdSetBlendConstants(commandlist.GetCommandBuffer(), blendConstants);
	}


	void GraphicsDevice_Vulkan::BindShadingRate(ShadingRate rate, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		if (CheckCapability(GraphicsDeviceCapability::VARIABLE_RATE_SHADING) && commandlist.prev_shadingrate != rate)
		{
			commandlist.prev_shadingrate = rate;

			VkExtent2D fragmentSize;
			switch (rate)
			{
			case ShadingRate::RATE_1X1:
				fragmentSize.width = 1;
				fragmentSize.height = 1;
				break;
			case ShadingRate::RATE_1X2:
				fragmentSize.width = 1;
				fragmentSize.height = 2;
				break;
			case ShadingRate::RATE_2X1:
				fragmentSize.width = 2;
				fragmentSize.height = 1;
				break;
			case ShadingRate::RATE_2X2:
				fragmentSize.width = 2;
				fragmentSize.height = 2;
				break;
			case ShadingRate::RATE_2X4:
				fragmentSize.width = 2;
				fragmentSize.height = 4;
				break;
			case ShadingRate::RATE_4X2:
				fragmentSize.width = 4;
				fragmentSize.height = 2;
				break;
			case ShadingRate::RATE_4X4:
				fragmentSize.width = 4;
				fragmentSize.height = 4;
				break;
			default:
				break;
			}

			VkFragmentShadingRateCombinerOpKHR combiner[] = {
				VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR,
				VK_FRAGMENT_SHADING_RATE_COMBINER_OP_KEEP_KHR
			};

			if (fragment_shading_rate_properties.fragmentShadingRateNonTrivialCombinerOps == VK_TRUE)
			{
				if (fragment_shading_rate_features.primitiveFragmentShadingRate == VK_TRUE)
				{
					combiner[0] = VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MAX_KHR;
				}
				if (fragment_shading_rate_features.attachmentFragmentShadingRate == VK_TRUE)
				{
					combiner[1] = VK_FRAGMENT_SHADING_RATE_COMBINER_OP_MAX_KHR;
				}
			}
			else
			{
				if (fragment_shading_rate_features.primitiveFragmentShadingRate == VK_TRUE)
				{
					combiner[0] = VK_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE_KHR;
				}
				if (fragment_shading_rate_features.attachmentFragmentShadingRate == VK_TRUE)
				{
					combiner[1] = VK_FRAGMENT_SHADING_RATE_COMBINER_OP_REPLACE_KHR;
				}
			}

			vkCmdSetFragmentShadingRateKHR(
				commandlist.GetCommandBuffer(),
				&fragmentSize,
				combiner
			);
		}
	}
	void GraphicsDevice_Vulkan::BindPipelineState(const PipelineState* pso, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		commandlist.active_cs = nullptr;
		commandlist.active_rt = nullptr;

		auto internal_state = to_internal(pso);

		if (internal_state->pipeline != VK_NULL_HANDLE)
		{
			if (commandlist.active_pso != pso)
			{
				vkCmdBindPipeline(commandlist.GetCommandBuffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, internal_state->pipeline);
			}
			else
				return; // early exit for static pso

			commandlist.prev_pipeline_hash = {};
			commandlist.dirty_pso = false;
		}
		else
		{
			PipelineHash pipeline_hash;
			pipeline_hash.pso = pso;
			pipeline_hash.renderpass_hash = commandlist.renderpass_info.get_hash();
			if (commandlist.prev_pipeline_hash == pipeline_hash)
			{
				commandlist.active_pso = pso;
				return; // early exit for dynamic pso|renderpass
			}
			commandlist.prev_pipeline_hash = pipeline_hash;
			commandlist.dirty_pso = true;
		}

		if (commandlist.active_pso == nullptr)
		{
			commandlist.binder.dirty |= DescriptorBinder::DIRTY_ALL;
		}
		else
		{
			auto active_internal = to_internal(commandlist.active_pso);
			if (internal_state->pipelineLayout != active_internal->pipelineLayout)
			{
				commandlist.binder.dirty |= DescriptorBinder::DIRTY_ALL;
			}
		}

		if (!internal_state->bindlessSets.empty())
		{
			vkCmdBindDescriptorSets(
				commandlist.GetCommandBuffer(),
				VK_PIPELINE_BIND_POINT_GRAPHICS,
				internal_state->pipelineLayout,
				internal_state->bindlessFirstSet,
				(uint32_t)internal_state->bindlessSets.size(),
				internal_state->bindlessSets.data(),
				0,
				nullptr
			);
		}

		commandlist.active_pso = pso;
	}


	void GraphicsDevice_Vulkan::BindComputeShader(const Shader* cs, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		if (commandlist.active_cs == cs)
			return;
		commandlist.active_pso = nullptr;
		commandlist.active_rt = nullptr;

		assert(cs->stage == ShaderStage::CS || cs->stage == ShaderStage::LIB);

		if (commandlist.active_cs == nullptr)
		{
			commandlist.binder.dirty |= DescriptorBinder::DIRTY_ALL;
		}
		else
		{
			auto internal_state = to_internal(cs);
			auto active_internal = to_internal(commandlist.active_cs);
			if (internal_state->pipelineLayout_cs != active_internal->pipelineLayout_cs)
			{
				commandlist.binder.dirty |= DescriptorBinder::DIRTY_ALL;
			}
		}

		commandlist.active_cs = cs;
		auto internal_state = to_internal(cs);

		if (cs->stage == ShaderStage::CS)
		{
			vkCmdBindPipeline(commandlist.GetCommandBuffer(), VK_PIPELINE_BIND_POINT_COMPUTE, internal_state->pipeline_cs);

			if (!internal_state->bindlessSets.empty())
			{
				vkCmdBindDescriptorSets(
					commandlist.GetCommandBuffer(),
					VK_PIPELINE_BIND_POINT_COMPUTE,
					internal_state->pipelineLayout_cs,
					internal_state->bindlessFirstSet,
					(uint32_t)internal_state->bindlessSets.size(),
					internal_state->bindlessSets.data(),
					0,
					nullptr
				);
			}
		}
		else if (cs->stage == ShaderStage::LIB)
		{
			if (!internal_state->bindlessSets.empty())
			{
				vkCmdBindDescriptorSets(
					commandlist.GetCommandBuffer(),
					VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
					internal_state->pipelineLayout_cs,
					internal_state->bindlessFirstSet,
					(uint32_t)internal_state->bindlessSets.size(),
					internal_state->bindlessSets.data(),
					0,
					nullptr
				);
			}
		}
	}
	void GraphicsDevice_Vulkan::BindDepthBounds(float min_bounds, float max_bounds, CommandList cmd)
	{
		if (features2.features.depthBounds == VK_TRUE)
		{
			CommandList_Vulkan& commandlist = GetCommandList(cmd);
			vkCmdSetDepthBounds(commandlist.GetCommandBuffer(), min_bounds, max_bounds);
		}
	}
	void GraphicsDevice_Vulkan::Draw(uint32_t vertexCount, uint32_t startVertexLocation, CommandList cmd)
	{
		predraw(cmd);
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdDraw(commandlist.GetCommandBuffer(), vertexCount, 1, startVertexLocation, 0);
	}
	void GraphicsDevice_Vulkan::DrawIndexed(uint32_t indexCount, uint32_t startIndexLocation, int32_t baseVertexLocation, CommandList cmd)
	{
		predraw(cmd);
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdDrawIndexed(commandlist.GetCommandBuffer(), indexCount, 1, startIndexLocation, baseVertexLocation, 0);
	}
	void GraphicsDevice_Vulkan::DrawInstanced(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertexLocation, uint32_t startInstanceLocation, CommandList cmd)
	{
		predraw(cmd);
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdDraw(commandlist.GetCommandBuffer(), vertexCount, instanceCount, startVertexLocation, startInstanceLocation);
	}
	void GraphicsDevice_Vulkan::DrawIndexedInstanced(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndexLocation, int32_t baseVertexLocation, uint32_t startInstanceLocation, CommandList cmd)
	{
		predraw(cmd);
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdDrawIndexed(commandlist.GetCommandBuffer(), indexCount, instanceCount, startIndexLocation, baseVertexLocation, startInstanceLocation);
	}
	void GraphicsDevice_Vulkan::DrawInstancedIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd)
	{
		predraw(cmd);
		auto internal_state = to_internal(args);
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdDrawIndirect(commandlist.GetCommandBuffer(), internal_state->resource, args_offset, 1, (uint32_t)sizeof(VkDrawIndirectCommand));
	}


	void GraphicsDevice_Vulkan::DrawIndexedInstancedIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd)
	{
		predraw(cmd);
		auto internal_state = to_internal(args);
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdDrawIndexedIndirect(commandlist.GetCommandBuffer(), internal_state->resource, args_offset, 1, sizeof(VkDrawIndexedIndirectCommand));
	}

	void GraphicsDevice_Vulkan::DrawInstancedIndirectCount(const GPUBuffer* args, uint64_t args_offset, const GPUBuffer* count, uint64_t count_offset, uint32_t max_count, CommandList cmd)
	{
		predraw(cmd);
		auto args_internal = to_internal(args);
		auto count_internal = to_internal(count);
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdDrawIndirectCount(commandlist.GetCommandBuffer(), args_internal->resource, args_offset, count_internal->resource, count_offset, max_count, sizeof(VkDrawIndirectCommand));
	}
	void GraphicsDevice_Vulkan::DrawIndexedInstancedIndirectCount(const GPUBuffer* args, uint64_t args_offset, const GPUBuffer* count, uint64_t count_offset, uint32_t max_count, CommandList cmd)
	{
		predraw(cmd);
		auto args_internal = to_internal(args);
		auto count_internal = to_internal(count);
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdDrawIndexedIndirectCount(commandlist.GetCommandBuffer(), args_internal->resource, args_offset, count_internal->resource, count_offset, max_count, sizeof(VkDrawIndexedIndirectCommand));
	}
	void GraphicsDevice_Vulkan::Dispatch(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, CommandList cmd)
	{
		predispatch(cmd);
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdDispatch(commandlist.GetCommandBuffer(), threadGroupCountX, threadGroupCountY, threadGroupCountZ);
	}
	void GraphicsDevice_Vulkan::DispatchIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd)
	{
		predispatch(cmd);
		auto internal_state = to_internal(args);
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdDispatchIndirect(commandlist.GetCommandBuffer(), internal_state->resource, args_offset);
	}
	void GraphicsDevice_Vulkan::DispatchMesh(uint32_t threadGroupCountX, uint32_t threadGroupCountY, uint32_t threadGroupCountZ, CommandList cmd)
	{
		predraw(cmd);
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdDrawMeshTasksEXT(commandlist.GetCommandBuffer(), threadGroupCountX, threadGroupCountY, threadGroupCountZ);
	}
	void GraphicsDevice_Vulkan::DispatchMeshIndirect(const GPUBuffer* args, uint64_t args_offset, CommandList cmd)
	{
		predraw(cmd);
		auto internal_state = to_internal(args);
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdDrawMeshTasksIndirectEXT(commandlist.GetCommandBuffer(), internal_state->resource, args_offset, 1, sizeof(VkDispatchIndirectCommand));
	}
	void GraphicsDevice_Vulkan::DispatchMeshIndirectCount(const GPUBuffer* args, uint64_t args_offset, const GPUBuffer* count, uint64_t count_offset, uint32_t max_count, CommandList cmd)
	{
		predraw(cmd);
		auto args_internal = to_internal(args);
		auto count_internal = to_internal(count);
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		vkCmdDrawMeshTasksIndirectCountEXT(commandlist.GetCommandBuffer(), args_internal->resource, args_offset, count_internal->resource, count_offset, max_count, sizeof(VkDispatchIndirectCommand));
	}
	void GraphicsDevice_Vulkan::CopyResource(const GPUResource* pDst, const GPUResource* pSrc, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		if (pDst->type == GPUResource::Type::TEXTURE && pSrc->type == GPUResource::Type::TEXTURE)
		{
			auto internal_state_src = to_internal((const Texture*)pSrc);
			auto internal_state_dst = to_internal((const Texture*)pDst);

			const TextureDesc& src_desc = ((const Texture*)pSrc)->GetDesc();
			const TextureDesc& dst_desc = ((const Texture*)pDst)->GetDesc();

			if (src_desc.usage == Usage::UPLOAD)
			{
				VkBufferImageCopy copy = {};
				copy.imageSubresource.baseArrayLayer = 0;
				copy.imageSubresource.layerCount = dst_desc.array_size;
				copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				const uint32_t data_stride = GetFormatStride(dst_desc.format);
				uint32_t mip_width = dst_desc.width;
				uint32_t mip_height = dst_desc.height;
				uint32_t mip_depth = dst_desc.depth;
				for (uint32_t mip = 0; mip < dst_desc.mip_levels; ++mip)
				{
					copy.imageExtent.width = mip_width;
					copy.imageExtent.height = mip_height;
					copy.imageExtent.depth = mip_depth;
					copy.imageSubresource.mipLevel = mip;
					vkCmdCopyBufferToImage(
						commandlist.GetCommandBuffer(),
						internal_state_src->staging_resource,
						internal_state_dst->resource,
						_ConvertImageLayout(ResourceState::COPY_DST),
						1,
						&copy
					);

					copy.bufferOffset += mip_width * mip_height * mip_depth * data_stride;
					mip_width = std::max(1u, mip_width / 2);
					mip_height = std::max(1u, mip_height / 2);
					mip_depth = std::max(1u, mip_depth / 2);
				}

			}
			else if (dst_desc.usage == Usage::READBACK)
			{
				VkBufferImageCopy copy = {};
				copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				const uint32_t data_stride = GetFormatStride(dst_desc.format);
				const uint32_t block_size = GetFormatBlockSize(dst_desc.format);
				const uint32_t num_blocks_x = dst_desc.width / block_size;
				const uint32_t num_blocks_y = dst_desc.height / block_size;
				for (uint32_t slice = 0; slice < dst_desc.array_size; ++slice)
				{
					copy.imageSubresource.baseArrayLayer = slice;
					copy.imageSubresource.layerCount = 1;
					uint32_t mip_blocks_x = num_blocks_x;
					uint32_t mip_blocks_y = num_blocks_y;
					uint32_t mip_width = dst_desc.width;
					uint32_t mip_height = dst_desc.height;
					uint32_t mip_depth = dst_desc.depth;
					for (uint32_t mip = 0; mip < dst_desc.mip_levels; ++mip)
					{
						copy.imageExtent.width = mip_width;
						copy.imageExtent.height = mip_height;
						copy.imageExtent.depth = mip_depth;
						copy.imageSubresource.mipLevel = mip;
						vkCmdCopyImageToBuffer(
							commandlist.GetCommandBuffer(),
							internal_state_src->resource,
							_ConvertImageLayout(ResourceState::COPY_SRC),
							internal_state_dst->staging_resource,
							1,
							&copy
						);

						copy.bufferOffset += mip_blocks_x * mip_blocks_y * mip_depth * data_stride;
						mip_blocks_x = std::max(1u, mip_blocks_x / 2);
						mip_blocks_y = std::max(1u, mip_blocks_y / 2);
						mip_width = std::max(1u, mip_width / 2);
						mip_height = std::max(1u, mip_height / 2);
						mip_depth = std::max(1u, mip_depth / 2);
					}
				}
			}
			else
			{
				VkImageCopy copy = {};
				copy.extent.width = dst_desc.width;
				copy.extent.height = dst_desc.height;
				copy.extent.depth = std::max(1u, dst_desc.depth);

				copy.srcOffset.x = 0;
				copy.srcOffset.y = 0;
				copy.srcOffset.z = 0;

				copy.dstOffset.x = 0;
				copy.dstOffset.y = 0;
				copy.dstOffset.z = 0;

				if (IsFormatDepthSupport(src_desc.format))
				{
					copy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
					if (IsFormatStencilSupport(src_desc.format))
					{
						copy.srcSubresource.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
					}
				}
				else
				{
					copy.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				}
				copy.srcSubresource.baseArrayLayer = 0;
				copy.srcSubresource.layerCount = src_desc.array_size;
				copy.srcSubresource.mipLevel = 0;

				if (has_flag(dst_desc.bind_flags, BindFlag::DEPTH_STENCIL))
				{
					copy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
					if (IsFormatStencilSupport(dst_desc.format))
					{
						copy.dstSubresource.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
					}
				}
				else
				{
					copy.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				}
				copy.dstSubresource.baseArrayLayer = 0;
				copy.dstSubresource.layerCount = dst_desc.array_size;
				copy.dstSubresource.mipLevel = 0;

				vkCmdCopyImage(commandlist.GetCommandBuffer(),
					internal_state_src->resource, _ConvertImageLayout(ResourceState::COPY_SRC),
					internal_state_dst->resource, _ConvertImageLayout(ResourceState::COPY_DST),
					1, &copy
				);
			}
		}
		else if (pDst->type == GPUResource::Type::BUFFER && pSrc->type == GPUResource::Type::BUFFER)
		{
			auto internal_state_src = to_internal((const GPUBuffer*)pSrc);
			auto internal_state_dst = to_internal((const GPUBuffer*)pDst);

			const GPUBufferDesc& src_desc = ((const GPUBuffer*)pSrc)->GetDesc();
			const GPUBufferDesc& dst_desc = ((const GPUBuffer*)pDst)->GetDesc();

			VkBufferCopy copy = {};
			copy.srcOffset = 0;
			copy.dstOffset = 0;
			copy.size = std::min(src_desc.size, dst_desc.size);

			vkCmdCopyBuffer(commandlist.GetCommandBuffer(),
				internal_state_src->resource,
				internal_state_dst->resource,
				1, &copy
			);
		}
	}
	void GraphicsDevice_Vulkan::CopyBuffer(const GPUBuffer* pDst, uint64_t dst_offset, const GPUBuffer* pSrc, uint64_t src_offset, uint64_t size, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		auto internal_state_src = to_internal(pSrc);
		auto internal_state_dst = to_internal(pDst);

		VkBufferCopy copy = {};
		copy.srcOffset = src_offset;
		copy.dstOffset = dst_offset;
		copy.size = size;

		vkCmdCopyBuffer(commandlist.GetCommandBuffer(),
			internal_state_src->resource,
			internal_state_dst->resource,
			1, &copy
		);
	}
	void GraphicsDevice_Vulkan::CopyTexture(const Texture* dst, uint32_t dstX, uint32_t dstY, uint32_t dstZ, uint32_t dstMip, uint32_t dstSlice, const Texture* src, uint32_t srcMip, uint32_t srcSlice, CommandList cmd, const Box* srcbox, ImageAspect dst_aspect, ImageAspect src_aspect)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		auto src_internal = to_internal(src);
		auto dst_internal = to_internal(dst);

		VkImageCopy copy = {};
		copy.dstSubresource.aspectMask = _ConvertImageAspect(dst_aspect);
		copy.dstSubresource.baseArrayLayer = dstSlice;
		copy.dstSubresource.layerCount = 1;
		copy.dstSubresource.mipLevel = dstMip;
		copy.dstOffset.x = dstX;
		copy.dstOffset.y = dstY;
		copy.dstOffset.z = dstZ;

		copy.srcSubresource.aspectMask = _ConvertImageAspect(src_aspect);
		copy.srcSubresource.baseArrayLayer = srcSlice;
		copy.srcSubresource.layerCount = 1;
		copy.srcSubresource.mipLevel = srcMip;

		if (srcbox == nullptr)
		{
			copy.srcOffset.x = 0;
			copy.srcOffset.y = 0;
			copy.srcOffset.z = 0;
			if (src->desc.format == Format::NV12 && src_aspect == ImageAspect::CHROMINANCE)
			{
				copy.extent.width = std::min(dst->desc.width, src->desc.width / 2);
				copy.extent.height = std::min(dst->desc.height, src->desc.height / 2);
			}
			else
			{
				copy.extent.width = std::min(dst->desc.width, src->desc.width);
				copy.extent.height = std::min(dst->desc.height, src->desc.height);
			}
			copy.extent.depth = std::min(dst->desc.depth, src->desc.depth);

			copy.extent.width = std::max(1u, copy.extent.width >> srcMip);
			copy.extent.height = std::max(1u, copy.extent.height >> srcMip);
			copy.extent.depth = std::max(1u, copy.extent.depth >> srcMip);
		}
		else
		{
			copy.srcOffset.x = srcbox->left;
			copy.srcOffset.y = srcbox->top;
			copy.srcOffset.z = srcbox->front;
			copy.extent.width = srcbox->right - srcbox->left;
			copy.extent.height = srcbox->bottom - srcbox->top;
			copy.extent.depth = srcbox->back - srcbox->front;
		}

		vkCmdCopyImage(
			commandlist.GetCommandBuffer(),
			src_internal->resource,
			_ConvertImageLayout(ResourceState::COPY_SRC),
			dst_internal->resource,
			_ConvertImageLayout(ResourceState::COPY_DST),
			1,
			&copy
		);
	}
	void GraphicsDevice_Vulkan::QueryBegin(const GPUQueryHeap* heap, uint32_t index, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		auto internal_state = to_internal(heap);

		switch (heap->desc.type)
		{
		case GpuQueryType::OCCLUSION_BINARY:
			vkCmdBeginQuery(commandlist.GetCommandBuffer(), internal_state->pool, index, 0);
			break;
		case GpuQueryType::OCCLUSION:
			vkCmdBeginQuery(commandlist.GetCommandBuffer(), internal_state->pool, index, VK_QUERY_CONTROL_PRECISE_BIT);
			break;
		case GpuQueryType::TIMESTAMP:
			break;
		}
	}
	void GraphicsDevice_Vulkan::QueryEnd(const GPUQueryHeap* heap, uint32_t index, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		auto internal_state = to_internal(heap);

		switch (heap->desc.type)
		{
		case GpuQueryType::TIMESTAMP:
			vkCmdWriteTimestamp2(commandlist.GetCommandBuffer(), VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT, internal_state->pool, index);
			break;
		case GpuQueryType::OCCLUSION_BINARY:
		case GpuQueryType::OCCLUSION:
			vkCmdEndQuery(commandlist.GetCommandBuffer(), internal_state->pool, index);
			break;
		}
	}
	void GraphicsDevice_Vulkan::QueryResolve(const GPUQueryHeap* heap, uint32_t index, uint32_t count, const GPUBuffer* dest, uint64_t dest_offset, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);

		auto internal_state = to_internal(heap);
		auto dst_internal = to_internal(dest);

		VkQueryResultFlags flags = VK_QUERY_RESULT_64_BIT;
		flags |= VK_QUERY_RESULT_WAIT_BIT;

		switch (heap->desc.type)
		{
		case GpuQueryType::OCCLUSION_BINARY:
			flags |= VK_QUERY_RESULT_PARTIAL_BIT;
			break;
		default:
			break;
		}

		vkCmdCopyQueryPoolResults(
			commandlist.GetCommandBuffer(),
			internal_state->pool,
			index,
			count,
			dst_internal->resource,
			dest_offset,
			sizeof(uint64_t),
			flags
		);

	}


	void GraphicsDevice_Vulkan::QueryReset(const GPUQueryHeap* heap, uint32_t index, uint32_t count, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);

		auto internal_state = to_internal(heap);

		vkCmdResetQueryPool(
			commandlist.GetCommandBuffer(),
			internal_state->pool,
			index,
			count
		);

	}

	void GraphicsDevice_Vulkan::Barrier(const GPUBarrier* barriers, uint32_t numBarriers, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);

		auto& memoryBarriers = commandlist.frame_memoryBarriers;
		auto& imageBarriers = commandlist.frame_imageBarriers;
		auto& bufferBarriers = commandlist.frame_bufferBarriers;

		for (uint32_t i = 0; i < numBarriers; ++i)
		{
			const GPUBarrier& barrier = barriers[i];

			if (barrier.type == GPUBarrier::Type::IMAGE && (barrier.image.texture == nullptr || !barrier.image.texture->IsValid()))
				continue;
			if (barrier.type == GPUBarrier::Type::BUFFER && (barrier.buffer.buffer == nullptr || !barrier.buffer.buffer->IsValid()))
				continue;

			switch (barrier.type)
			{
			default:
			case GPUBarrier::Type::MEMORY:
			case GPUBarrier::Type::ALIASING:
			{
				VkMemoryBarrier2 barrierdesc = {};
				barrierdesc.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
				barrierdesc.pNext = nullptr;
				barrierdesc.srcStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
				barrierdesc.dstStageMask = VK_PIPELINE_STAGE_2_ALL_COMMANDS_BIT;
				barrierdesc.srcAccessMask = VK_ACCESS_2_MEMORY_WRITE_BIT;
				barrierdesc.dstAccessMask = VK_ACCESS_2_MEMORY_READ_BIT;

				if (CheckCapability(GraphicsDeviceCapability::RAYTRACING))
				{
					barrierdesc.srcStageMask |= VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR | VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;
					barrierdesc.dstStageMask |= VK_PIPELINE_STAGE_2_ACCELERATION_STRUCTURE_BUILD_BIT_KHR | VK_PIPELINE_STAGE_2_RAY_TRACING_SHADER_BIT_KHR;
					barrierdesc.srcAccessMask |= VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
					barrierdesc.dstAccessMask |= VK_ACCESS_2_ACCELERATION_STRUCTURE_READ_BIT_KHR | VK_ACCESS_2_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
				}

				if (CheckCapability(GraphicsDeviceCapability::PREDICATION))
				{
					barrierdesc.srcStageMask |= VK_PIPELINE_STAGE_2_CONDITIONAL_RENDERING_BIT_EXT;
					barrierdesc.dstStageMask |= VK_PIPELINE_STAGE_2_CONDITIONAL_RENDERING_BIT_EXT;
					barrierdesc.srcAccessMask |= VK_ACCESS_2_CONDITIONAL_RENDERING_READ_BIT_EXT;
					barrierdesc.dstAccessMask |= VK_ACCESS_2_CONDITIONAL_RENDERING_READ_BIT_EXT;
				}

				memoryBarriers.push_back(barrierdesc);
			}
			break;
			case GPUBarrier::Type::IMAGE:
			{
				const TextureDesc& desc = barrier.image.texture->desc;
				auto internal_state = to_internal(barrier.image.texture);

				VkImageMemoryBarrier2 barrierdesc = {};
				barrierdesc.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
				barrierdesc.pNext = nullptr;
				barrierdesc.image = internal_state->resource;
				barrierdesc.oldLayout = _ConvertImageLayout(barrier.image.layout_before);
				barrierdesc.newLayout = _ConvertImageLayout(barrier.image.layout_after);
				barrierdesc.srcStageMask = _ConvertPipelineStage(barrier.image.layout_before);
				barrierdesc.dstStageMask = _ConvertPipelineStage(barrier.image.layout_after);
				barrierdesc.srcAccessMask = _ParseResourceState(barrier.image.layout_before);
				barrierdesc.dstAccessMask = _ParseResourceState(barrier.image.layout_after);
				if (IsFormatDepthSupport(desc.format))
				{
					barrierdesc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
					if (IsFormatStencilSupport(desc.format))
					{
						barrierdesc.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
					}
				}
				else
				{
					barrierdesc.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				}
				if (barrier.image.aspect != nullptr)
				{
					barrierdesc.subresourceRange.aspectMask = _ConvertImageAspect(*barrier.image.aspect);
				}
				if (barrier.image.mip >= 0 || barrier.image.slice >= 0)
				{
					barrierdesc.subresourceRange.baseMipLevel = (uint32_t)std::max(0, barrier.image.mip);
					barrierdesc.subresourceRange.levelCount = 1;
					barrierdesc.subresourceRange.baseArrayLayer = (uint32_t)std::max(0, barrier.image.slice);
					barrierdesc.subresourceRange.layerCount = 1;
				}
				else
				{
					barrierdesc.subresourceRange.baseMipLevel = 0;
					barrierdesc.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
					barrierdesc.subresourceRange.baseArrayLayer = 0;
					barrierdesc.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
				}
				barrierdesc.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrierdesc.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

				imageBarriers.push_back(barrierdesc);
			}
			break;
			case GPUBarrier::Type::BUFFER:
			{
				const GPUBufferDesc& desc = barrier.buffer.buffer->desc;
				auto internal_state = to_internal(barrier.buffer.buffer);

				VkBufferMemoryBarrier2 barrierdesc = {};
				barrierdesc.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2;
				barrierdesc.pNext = nullptr;
				barrierdesc.buffer = internal_state->resource;
				barrierdesc.size = desc.size;
				barrierdesc.offset = 0;
				barrierdesc.srcStageMask = _ConvertPipelineStage(barrier.buffer.state_before);
				barrierdesc.dstStageMask = _ConvertPipelineStage(barrier.buffer.state_after);
				barrierdesc.srcAccessMask = _ParseResourceState(barrier.buffer.state_before);
				barrierdesc.dstAccessMask = _ParseResourceState(barrier.buffer.state_after);
				barrierdesc.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
				barrierdesc.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

				if (has_flag(desc.misc_flags, ResourceMiscFlag::RAY_TRACING))
				{
					assert(CheckCapability(GraphicsDeviceCapability::RAYTRACING));
					barrierdesc.srcStageMask |= _ConvertPipelineStage(ResourceState::RAYTRACING_ACCELERATION_STRUCTURE);
					barrierdesc.dstStageMask |= _ConvertPipelineStage(ResourceState::RAYTRACING_ACCELERATION_STRUCTURE);
				}

				if (has_flag(desc.misc_flags, ResourceMiscFlag::PREDICATION))
				{
					assert(CheckCapability(GraphicsDeviceCapability::PREDICATION));
					barrierdesc.srcStageMask |= _ConvertPipelineStage(ResourceState::PREDICATION);
					barrierdesc.dstStageMask |= _ConvertPipelineStage(ResourceState::PREDICATION);
				}

				bufferBarriers.push_back(barrierdesc);
			}
			break;
			}
		}

		if (!memoryBarriers.empty() ||
			!bufferBarriers.empty() ||
			!imageBarriers.empty()
			)
		{
			VkDependencyInfo dependencyInfo = {};
			dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
			dependencyInfo.memoryBarrierCount = static_cast<uint32_t>(memoryBarriers.size());
			dependencyInfo.pMemoryBarriers = memoryBarriers.data();
			dependencyInfo.bufferMemoryBarrierCount = static_cast<uint32_t>(bufferBarriers.size());
			dependencyInfo.pBufferMemoryBarriers = bufferBarriers.data();
			dependencyInfo.imageMemoryBarrierCount = static_cast<uint32_t>(imageBarriers.size());
			dependencyInfo.pImageMemoryBarriers = imageBarriers.data();

			vkCmdPipelineBarrier2(commandlist.GetCommandBuffer(), &dependencyInfo);

			memoryBarriers.clear();
			imageBarriers.clear();
			bufferBarriers.clear();
		}
	}


	void GraphicsDevice_Vulkan::BuildRaytracingAccelerationStructure(const RaytracingAccelerationStructure* dst, CommandList cmd, const RaytracingAccelerationStructure* src)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		auto dst_internal = to_internal(dst);

		VkAccelerationStructureBuildGeometryInfoKHR info = dst_internal->buildInfo;
		info.dstAccelerationStructure = dst_internal->resource;
		info.srcAccelerationStructure = VK_NULL_HANDLE;
		info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;

		info.scratchData.deviceAddress = dst_internal->scratch_address;

		if (src != nullptr && (dst->desc.flags & RaytracingAccelerationStructureDesc::FLAG_ALLOW_UPDATE))
		{
			info.mode = VK_BUILD_ACCELERATION_STRUCTURE_MODE_UPDATE_KHR;

			auto src_internal = to_internal(src);
			info.srcAccelerationStructure = src_internal->resource;
		}

		commandlist.accelerationstructure_build_geometries = dst_internal->geometries; // copy!
		commandlist.accelerationstructure_build_ranges.clear();

		info.type = dst_internal->createInfo.type;
		info.geometryCount = (uint32_t)commandlist.accelerationstructure_build_geometries.size();
		commandlist.accelerationstructure_build_ranges.reserve(info.geometryCount);

		switch (dst->desc.type)
		{
		case RaytracingAccelerationStructureDesc::Type::BOTTOMLEVEL:
		{
			size_t i = 0;
			for (auto& x : dst->desc.bottom_level.geometries)
			{
				auto& geometry = commandlist.accelerationstructure_build_geometries[i];

				auto& range = commandlist.accelerationstructure_build_ranges.emplace_back();
				range = {};

				if (x.flags & RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_OPAQUE)
				{
					geometry.flags |= VK_GEOMETRY_OPAQUE_BIT_KHR;
				}
				if (x.flags & RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_NO_DUPLICATE_ANYHIT_INVOCATION)
				{
					geometry.flags |= VK_GEOMETRY_NO_DUPLICATE_ANY_HIT_INVOCATION_BIT_KHR;
				}

				if (x.type == RaytracingAccelerationStructureDesc::BottomLevel::Geometry::Type::TRIANGLES)
				{
					geometry.geometry.triangles.vertexData.deviceAddress = to_internal(&x.triangles.vertex_buffer)->address +
						x.triangles.vertex_byte_offset;

					geometry.geometry.triangles.indexData.deviceAddress = to_internal(&x.triangles.index_buffer)->address +
						x.triangles.index_offset * (x.triangles.index_format == IndexBufferFormat::UINT16 ? sizeof(uint16_t) : sizeof(uint32_t));

					if (x.flags & RaytracingAccelerationStructureDesc::BottomLevel::Geometry::FLAG_USE_TRANSFORM)
					{
						geometry.geometry.triangles.transformData.deviceAddress = to_internal(&x.triangles.transform_3x4_buffer)->address;
						range.transformOffset = x.triangles.transform_3x4_buffer_offset;
					}

					range.primitiveCount = x.triangles.index_count / 3;
					range.primitiveOffset = 0;
				}
				else if (x.type == RaytracingAccelerationStructureDesc::BottomLevel::Geometry::Type::PROCEDURAL_AABBS)
				{
					geometry.geometry.aabbs.data.deviceAddress = to_internal(&x.aabbs.aabb_buffer)->address;

					range.primitiveCount = x.aabbs.count;
					range.primitiveOffset = x.aabbs.offset;
				}

				i++;
			}
		}
		break;
		case RaytracingAccelerationStructureDesc::Type::TOPLEVEL:
		{
			auto& geometry = commandlist.accelerationstructure_build_geometries.back();
			geometry.geometry.instances.data.deviceAddress = to_internal(&dst->desc.top_level.instance_buffer)->address;

			auto& range = commandlist.accelerationstructure_build_ranges.emplace_back();
			range = {};
			range.primitiveCount = dst->desc.top_level.count;
			range.primitiveOffset = dst->desc.top_level.offset;
		}
		break;
		}

		info.pGeometries = commandlist.accelerationstructure_build_geometries.data();

		VkAccelerationStructureBuildRangeInfoKHR* pRangeInfo = commandlist.accelerationstructure_build_ranges.data();

		vkCmdBuildAccelerationStructuresKHR(
			commandlist.GetCommandBuffer(),
			1,
			&info,
			&pRangeInfo
		);
	}
	void GraphicsDevice_Vulkan::BindRaytracingPipelineState(const RaytracingPipelineState* rtpso, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		commandlist.prev_pipeline_hash = {};
		commandlist.active_rt = rtpso;

		BindComputeShader(rtpso->desc.shader_libraries.front().shader, cmd);

		vkCmdBindPipeline(commandlist.GetCommandBuffer(), VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, to_internal(rtpso)->pipeline);
	}


	void GraphicsDevice_Vulkan::DispatchRays(const DispatchRaysDesc* desc, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		predispatch(cmd);

		VkStridedDeviceAddressRegionKHR raygen = {};
		raygen.deviceAddress = desc->ray_generation.buffer ? to_internal(desc->ray_generation.buffer)->address : 0;
		raygen.deviceAddress += desc->ray_generation.offset;
		raygen.size = desc->ray_generation.size;
		raygen.stride = raygen.size; // raygen specifically must be size == stride

		VkStridedDeviceAddressRegionKHR miss = {};
		miss.deviceAddress = desc->miss.buffer ? to_internal(desc->miss.buffer)->address : 0;
		miss.deviceAddress += desc->miss.offset;
		miss.size = desc->miss.size;
		miss.stride = desc->miss.stride;

		VkStridedDeviceAddressRegionKHR hitgroup = {};
		hitgroup.deviceAddress = desc->hit_group.buffer ? to_internal(desc->hit_group.buffer)->address : 0;
		hitgroup.deviceAddress += desc->hit_group.offset;
		hitgroup.size = desc->hit_group.size;
		hitgroup.stride = desc->hit_group.stride;

		VkStridedDeviceAddressRegionKHR callable = {};
		callable.deviceAddress = desc->callable.buffer ? to_internal(desc->callable.buffer)->address : 0;
		callable.deviceAddress += desc->callable.offset;
		callable.size = desc->callable.size;
		callable.stride = desc->callable.stride;

		vkCmdTraceRaysKHR(
			commandlist.GetCommandBuffer(),
			&raygen,
			&miss,
			&hitgroup,
			&callable,
			desc->width,
			desc->height,
			desc->depth
		);
	}
	void GraphicsDevice_Vulkan::PushConstants(const void* data, uint32_t size, CommandList cmd, uint32_t offset)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);

		if (commandlist.active_pso != nullptr)
		{
			auto pso_internal = to_internal(commandlist.active_pso);
			if (pso_internal->pushconstants.size > 0)
			{
				vkCmdPushConstants(
					commandlist.GetCommandBuffer(),
					pso_internal->pipelineLayout,
					pso_internal->pushconstants.stageFlags,
					offset,
					size,
					data
				);
				return;
			}
			assert(0); // there was no push constant block!
		}
		if (commandlist.active_cs != nullptr)
		{
			auto cs_internal = to_internal(commandlist.active_cs);
			if (cs_internal->pushconstants.size > 0)
			{
				vkCmdPushConstants(
					commandlist.GetCommandBuffer(),
					cs_internal->pipelineLayout_cs,
					cs_internal->pushconstants.stageFlags,
					offset,
					size,
					data
				);
				return;
			}
			assert(0); // there was no push constant block!
		}
		assert(0); // there was no active pipeline!
	}
	void GraphicsDevice_Vulkan::PredicationBegin(const GPUBuffer* buffer, uint64_t offset, PredicationOp op, CommandList cmd)
	{
		if (CheckCapability(GraphicsDeviceCapability::PREDICATION))
		{
			CommandList_Vulkan& commandlist = GetCommandList(cmd);
			auto internal_state = to_internal(buffer);

			VkConditionalRenderingBeginInfoEXT info = {};
			info.sType = VK_STRUCTURE_TYPE_CONDITIONAL_RENDERING_BEGIN_INFO_EXT;
			if (op == PredicationOp::NOT_EQUAL_ZERO)
			{
				info.flags = VK_CONDITIONAL_RENDERING_INVERTED_BIT_EXT;
			}
			info.offset = offset;
			info.buffer = internal_state->resource;
			vkCmdBeginConditionalRenderingEXT(commandlist.GetCommandBuffer(), &info);
		}
	}
	void GraphicsDevice_Vulkan::PredicationEnd(CommandList cmd)
	{
		if (CheckCapability(GraphicsDeviceCapability::PREDICATION))
		{
			CommandList_Vulkan& commandlist = GetCommandList(cmd);
			vkCmdEndConditionalRenderingEXT(commandlist.GetCommandBuffer());
		}
	}
	void GraphicsDevice_Vulkan::ClearUAV(const GPUResource* resource, uint32_t value, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);

		if (resource->IsBuffer())
		{
			auto internal_state = to_internal((const GPUBuffer*)resource);
			vkCmdFillBuffer(
				commandlist.GetCommandBuffer(),
				internal_state->resource,
				0,
				VK_WHOLE_SIZE,
				value
			);
		}
		else if (resource->IsTexture())
		{
			VkClearColorValue color = {};
			color.uint32[0] = value;
			color.uint32[1] = value;
			color.uint32[2] = value;
			color.uint32[3] = value;

			VkImageSubresourceRange range = {};
			range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			range.baseArrayLayer = 0;
			range.baseMipLevel = 0;
			range.layerCount = VK_REMAINING_ARRAY_LAYERS;
			range.levelCount = VK_REMAINING_MIP_LEVELS;

			auto internal_state = to_internal((const Texture*)resource);
			vkCmdClearColorImage(
				commandlist.GetCommandBuffer(),
				internal_state->resource,
				VK_IMAGE_LAYOUT_GENERAL, // "ClearUAV" so must be in UNORDERED_ACCESS state, that's a given
				&color,
				1,
				&range
			);
		}

	}

	void GraphicsDevice_Vulkan::VideoDecode(const VideoDecoder* video_decoder, const VideoDecodeOperation* op, CommandList cmd)
	{
		CommandList_Vulkan& commandlist = GetCommandList(cmd);
		auto decoder_internal = to_internal(video_decoder);
		auto stream_internal = to_internal(op->stream);
		auto dpb_internal = to_internal(op->DPB);

		if (video_decoder->desc.profile == VideoProfile::H264)
		{
			const h264::SliceHeader* slice_header = (const h264::SliceHeader*)op->slice_header;
			const h264::PPS* pps = (const h264::PPS*)op->pps;
			const h264::SPS* sps = (const h264::SPS*)op->sps;

			StdVideoDecodeH264PictureInfo std_picture_info_h264 = {};
			std_picture_info_h264.pic_parameter_set_id = slice_header->pic_parameter_set_id;
			std_picture_info_h264.seq_parameter_set_id = pps->seq_parameter_set_id;
			std_picture_info_h264.frame_num = slice_header->frame_num;
			std_picture_info_h264.PicOrderCnt[0] = op->poc[0];
			std_picture_info_h264.PicOrderCnt[1] = op->poc[1];
			std_picture_info_h264.idr_pic_id = slice_header->idr_pic_id;
			std_picture_info_h264.flags.is_intra = op->frame_type == VideoFrameType::Intra ? 1 : 0;
			std_picture_info_h264.flags.is_reference = op->reference_priority > 0 ? 1 : 0;
			std_picture_info_h264.flags.IdrPicFlag = (std_picture_info_h264.flags.is_intra && std_picture_info_h264.flags.is_reference) ? 1 : 0;
			std_picture_info_h264.flags.field_pic_flag = slice_header->field_pic_flag;
			std_picture_info_h264.flags.bottom_field_flag = slice_header->bottom_field_flag;
			std_picture_info_h264.flags.complementary_field_pair = 0;

			VkVideoReferenceSlotInfoKHR reference_slot_infos[17] = {};
			VkVideoPictureResourceInfoKHR reference_slot_pictures[17] = {};
			VkVideoDecodeH264DpbSlotInfoKHR dpb_slots_h264[17] = {};
			StdVideoDecodeH264ReferenceInfo reference_infos_h264[17] = {};
			for (uint32_t i = 0; i < op->DPB->desc.array_size; ++i)
			{
				VkVideoReferenceSlotInfoKHR& slot = reference_slot_infos[i];
				VkVideoPictureResourceInfoKHR& pic = reference_slot_pictures[i];
				VkVideoDecodeH264DpbSlotInfoKHR& dpb = dpb_slots_h264[i];
				StdVideoDecodeH264ReferenceInfo& ref = reference_infos_h264[i];

				slot.sType = VK_STRUCTURE_TYPE_VIDEO_REFERENCE_SLOT_INFO_KHR;
				slot.pPictureResource = &pic;
				slot.slotIndex = i;
				slot.pNext = &dpb;

				pic.sType = VK_STRUCTURE_TYPE_VIDEO_PICTURE_RESOURCE_INFO_KHR;
				pic.codedOffset.x = 0;
				pic.codedOffset.y = 0;
				pic.codedExtent.width = op->DPB->desc.width;
				pic.codedExtent.height = op->DPB->desc.height;
				pic.baseArrayLayer = i;
				pic.imageViewBinding = dpb_internal->video_decode_view;

				dpb.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_DPB_SLOT_INFO_KHR;
				dpb.pStdReferenceInfo = &ref;

				ref.flags.bottom_field_flag = 0;
				ref.flags.top_field_flag = 0;
				ref.flags.is_non_existing = 0;
				ref.flags.used_for_long_term_reference = 0;
				ref.FrameNum = op->dpb_framenum[i];
				ref.PicOrderCnt[0] = op->dpb_poc[i];
				ref.PicOrderCnt[1] = op->dpb_poc[i];
			}

			VkVideoReferenceSlotInfoKHR reference_slots[17] = {};
			for (size_t i = 0; i < op->dpb_reference_count; ++i)
			{
				uint32_t ref_slot = op->dpb_reference_slots[i];
				assert(ref_slot != op->current_dpb);
				reference_slots[i] = reference_slot_infos[ref_slot];
			}
			reference_slots[op->dpb_reference_count] = reference_slot_infos[op->current_dpb];
			reference_slots[op->dpb_reference_count].slotIndex = -1;

			VkVideoBeginCodingInfoKHR begin_info = {};
			begin_info.sType = VK_STRUCTURE_TYPE_VIDEO_BEGIN_CODING_INFO_KHR;
			begin_info.videoSession = decoder_internal->video_session;
			begin_info.videoSessionParameters = decoder_internal->session_parameters;
			begin_info.referenceSlotCount = op->dpb_reference_count + 1; // add in the current reconstructed DPB image
			begin_info.pReferenceSlots = begin_info.referenceSlotCount == 0 ? nullptr : reference_slots;
			if (pfnCmdBeginVideoCodingKHR == nullptr) {
				pfnCmdBeginVideoCodingKHR = (PFN_vkCmdBeginVideoCodingKHR)vkGetDeviceProcAddr(device, "vkCmdBeginVideoCodingKHR");
			}
			pfnCmdBeginVideoCodingKHR(commandlist.GetCommandBuffer(), &begin_info);

			if (op->flags & VideoDecodeOperation::FLAG_SESSION_RESET)
			{
				VkVideoCodingControlInfoKHR control_info = {};
				control_info.sType = VK_STRUCTURE_TYPE_VIDEO_CODING_CONTROL_INFO_KHR;
				control_info.flags = VK_VIDEO_CODING_CONTROL_RESET_BIT_KHR;
				if (pfnCmdControlVideoCodingKHR ==nullptr) {
					pfnCmdControlVideoCodingKHR = (PFN_vkCmdControlVideoCodingKHR)vkGetDeviceProcAddr(device, "vkCmdControlVideoCodingKHR");
				}
				vkCmdControlVideoCodingKHR(commandlist.GetCommandBuffer(), &control_info);
			}

			VkVideoDecodeInfoKHR decode_info = {};
			decode_info.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_INFO_KHR;
			decode_info.srcBuffer = stream_internal->resource;
			decode_info.srcBufferOffset = (VkDeviceSize)op->stream_offset;
			decode_info.srcBufferRange = (VkDeviceSize)AlignTo(op->stream_size, VIDEO_DECODE_BITSTREAM_ALIGNMENT);
			if (op->output == nullptr)
			{
				decode_info.dstPictureResource = *reference_slot_infos[op->current_dpb].pPictureResource;
			}
			else
			{
				auto output_internal = to_internal(op->output);
				decode_info.dstPictureResource.sType = VK_STRUCTURE_TYPE_VIDEO_PICTURE_RESOURCE_INFO_KHR;
				decode_info.dstPictureResource.codedOffset.x = 0;
				decode_info.dstPictureResource.codedOffset.y = 0;
				decode_info.dstPictureResource.codedExtent.width = op->DPB->desc.width;
				decode_info.dstPictureResource.codedExtent.height = op->DPB->desc.height;
				decode_info.dstPictureResource.baseArrayLayer = 0;
				decode_info.dstPictureResource.imageViewBinding = output_internal->video_decode_view;
			}
			decode_info.referenceSlotCount = op->dpb_reference_count;
			decode_info.pReferenceSlots = decode_info.referenceSlotCount == 0 ? nullptr : reference_slots;
			decode_info.pSetupReferenceSlot = &reference_slot_infos[op->current_dpb];

			uint32_t slice_offset = 0;

			// https://vulkan.lunarg.com/doc/view/1.3.239.0/windows/1.3-extensions/vkspec.html#_h_264_decoding_parameters
			VkVideoDecodeH264PictureInfoKHR picture_info_h264 = {};
			picture_info_h264.sType = VK_STRUCTURE_TYPE_VIDEO_DECODE_H264_PICTURE_INFO_KHR;
			picture_info_h264.pStdPictureInfo = &std_picture_info_h264;
			picture_info_h264.sliceCount = 1;
			picture_info_h264.pSliceOffsets = &slice_offset;
			decode_info.pNext = &picture_info_h264;

			vkCmdDecodeVideoKHR(commandlist.GetCommandBuffer(), &decode_info);

			VkVideoEndCodingInfoKHR end_info = {};
			end_info.sType = VK_STRUCTURE_TYPE_VIDEO_END_CODING_INFO_KHR;
			if (pfnCmdEndVideoCodingKHR == nullptr) {
				pfnCmdEndVideoCodingKHR = (PFN_vkCmdEndVideoCodingKHR)vkGetDeviceProcAddr(device, "vkCmdEndVideoCodingKHR");
			}
			pfnCmdEndVideoCodingKHR(commandlist.GetCommandBuffer(), &end_info);
		}
		else if (video_decoder->desc.profile == VideoProfile::H265)
		{
			assert(0); // TODO
		}
	}

	void GraphicsDevice_Vulkan::EventBegin(const char* name, CommandList cmd)
	{
		if (!debugUtils)
			return;
		CommandList_Vulkan& commandlist = GetCommandList(cmd);

		VkDebugUtilsLabelEXT label = { VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
		label.pLabelName = name;
		label.color[0] = 0.0f;
		label.color[1] = 0.0f;
		label.color[2] = 0.0f;
		label.color[3] = 1.0f;
		vkCmdBeginDebugUtilsLabelEXT(commandlist.GetCommandBuffer(), &label);
	}
	void GraphicsDevice_Vulkan::EventEnd(CommandList cmd)
	{
		if (!debugUtils)
			return;
		CommandList_Vulkan& commandlist = GetCommandList(cmd);

		vkCmdEndDebugUtilsLabelEXT(commandlist.GetCommandBuffer());
	}

	void GraphicsDevice_Vulkan::SetMarker(const char* name, CommandList cmd)
	{
		if (!debugUtils)
			return;
		CommandList_Vulkan& commandlist = GetCommandList(cmd);

		VkDebugUtilsLabelEXT label{ VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT };
		label.pLabelName = name;
		label.color[0] = 0.0f;
		label.color[1] = 0.0f;
		label.color[2] = 0.0f;
		label.color[3] = 1.0f;
		vkCmdInsertDebugUtilsLabelEXT(commandlist.GetCommandBuffer(), &label);
	}
	VkDevice GraphicsDevice_Vulkan::GetDevice()
	{
		return device;
	}

	VkImage GraphicsDevice_Vulkan::GetTextureInternalResource(const Texture* texture)
	{
		return to_internal(texture)->resource;
	}
	VkPhysicalDevice GraphicsDevice_Vulkan::GetPhysicalDevice()
	{
		return physicalDevice;
	}
	VkInstance GraphicsDevice_Vulkan::GetInstance()
	{
		return instance;
	}
	VkQueue GraphicsDevice_Vulkan::GetGraphicsCommandQueue()
	{
		return queues[QUEUE_GRAPHICS].queue;
	}
	uint32_t GraphicsDevice_Vulkan::GetGraphicsFamilyIndex()
	{
		return graphicsFamily;
	}

}