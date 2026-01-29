#pragma once
#include "Core/core_include.h"
#include "Engine/Device/graph_driver.h"


namespace pf::fftgenerator
{
	void fft_512x512_c2c(
		const pf::graphics::GPUResource& pUAV_Dst,
		const pf::graphics::GPUResource& pSRV_Dst,
		const pf::graphics::GPUResource& pSRV_Src,
		pf::graphics::CommandList cmd);

	void LoadShaders();
}
