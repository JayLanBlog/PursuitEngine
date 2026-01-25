#pragma once

#include "color.h"
#include "Core/core_include.h"
#include "Engine/Device/graph_driver.h"

namespace pf::texturehelper {

	void Initialize();

	const graphics::Texture* getLogo();
	const graphics::Texture* getRandom64x64();
	const graphics::Texture* getColorGradeDefault();
	const graphics::Texture* getNormalMapDefault();
	const graphics::Texture* getBlackCubeMap();
	const graphics::Texture* getUINT4();
	const graphics::Texture* getBlueNoise();
	const graphics::Texture* getWaterRipple();
	const graphics::Texture* getCheckerBoard();

	const graphics::Texture* getWhite();
	const graphics::Texture* getBlack();
	const graphics::Texture* getTransparent();

	bool CreateTexture(
		graphics::Texture& texture,
		const void* data,
		uint32_t width,
		uint32_t height,
		graphics::Format format = graphics::Format::R8G8B8A8_UNORM,
		graphics::Swizzle swizzle = {}
	);

	enum class GradientType
	{
		Linear,
		Circular,
		Angular,
	};
	enum class GradientFlags
	{
		None = 0,
		Inverse = 1 << 0,		// inverts resulting gradient
		Smoothstep = 1 << 1,	// applies smoothstep function to resulting gradient
		PerlinNoise = 1 << 2,	// applies perlin noise to gradient
		R16Unorm = 1 << 3,		// the texture will be created in R16_UNORM format instead of R8_UNORM
	};
	pf::graphics::Texture CreateGradientTexture(
		GradientType type,
		uint32_t width,
		uint32_t height,
		const XMFLOAT2& uv_start = XMFLOAT2(0, 0),
		const XMFLOAT2& uv_end = XMFLOAT2(1, 0),
		GradientFlags gradient_flags = GradientFlags::None,
		pf::graphics::Swizzle swizzle = { pf::graphics::ComponentSwizzle::R, pf::graphics::ComponentSwizzle::R, pf::graphics::ComponentSwizzle::R, pf::graphics::ComponentSwizzle::R },
		float perlin_scale = 1,
		uint32_t perlin_seed = 1234u,
		int perlin_octaves = 8,
		float perlin_persistence = 0.5f
	);
	
	inline constexpr GradientFlags operator&(GradientFlags lhs, GradientFlags rhs)
	{
		using T = std::underlying_type_t<GradientFlags>;
		return static_cast<GradientFlags>(static_cast<T>(lhs) & static_cast<T>(rhs));
	}


	// Create a lens distortion normal map (16-bit precision)
	//	width		: texture width in pixels
	//	height		: texture height in pixels
	//	uv_start	: center of lens in uv-space [0,1]
	//	radius		: radius of lens in uv_space [0,1]
	//	squish		: squish the lens (higher value is more squished down)
	//	blend		: blend out the distortion by a constant amount
	//	edge_smoothness : smoothen the edge of the circle
	graphics::Texture CreateLensDistortionNormalMap(
		uint32_t width,
		uint32_t height,
		const XMFLOAT2& uv_start = XMFLOAT2(0.5f, 0.5f),
		float radius = 0.5f,
		float squish = 1,
		float blend = 1,
		float edge_smoothness = 0.04f
	);

}