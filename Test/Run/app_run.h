#pragma once
#include "Engine/Device/vulkan_driver.h"
#include "Engine/Ecore/resource_manager.h"
#include "Engine/Ecore/draw_image.h"
#include "Engine/Component/font.h"

namespace pf::arun {
	using namespace pf;
	using namespace pf::graphics;

	class TLauncher {
	public:
		std::unique_ptr<pf::graphics::GraphicsDevice> graphicsDevice_c;

		pf::graphics::SwapChain swapChain_c;
		pf::graphics::Texture rendertargetPreHDR10_c;
		Texture splash_screen_c;

		Canvas canvas_c;
		Canvas canvas_p;
		
		int screenW_c = 1920;
		int screenH_c = 1080;
		int imageW_c = 1000;
		int imageH_c = 656;
		bool allow_hdr_c = true;
		bool isInitialize_c = false;
		SDL_Window* window_c;

		std::string infodisplay_str_c;

		Rect rect;

		int size = 106;

		TLauncher() {}

		virtual void intialize();

		virtual void render();

		virtual void run();
	};
}