#include "test_vulkan_engine.h"
#include "Engine/Device/vulkan_driver.h"
#include "Engine/Ecore/resource_manager.h"
#include "Engine/Ecore/draw_image.h"
#include <SDL3/SDL.h>
//
using namespace pf;
using namespace pf::graphics;
std::unique_ptr<pf::graphics::GraphicsDevice> graphicsDevice;
pf::graphics::SwapChain swapChain;
pf::graphics::Texture rendertargetPreHDR10;
Texture splash_screen;
int splash_screen_subresource = -1;

Canvas canvas;
int screenW = 1920;
int screenH = 1080;
int imageW = 256;
int imageH = 256;
bool allow_hdr = true;
bool isInitialize = false;
SDL_Window* window;

void intialize() {
	
	/*if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
		SDL_Log("Failed: %s", SDL_GetError());
	}
	*/
	// SDL3 建议只初始化真正需要的子系统
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
		// SDL3 中返回 false 表示失败
		SDL_Log("失败: %s", SDL_GetError());
	}
	pf::graphics::GetDevice() = graphicsDevice.get();
//	canvas.init(window);
	canvas.init(imageW,imageH);
	SwapChainDesc desc = swapChain.desc;
	if (!swapChain.IsValid())
	{
		// initialize for the first time
		desc.buffer_count = 3;
		if (graphicsDevice->CheckCapability(GraphicsDeviceCapability::R9G9B9E5_SHAREDEXP_RENDERABLE))
		{
			desc.format = Format::R9G9B9E5_SHAREDEXP;
		}
		else
		{
			desc.format = Format::R10G10B10A2_UNORM;
		}
	}

	desc.width = canvas.GetLogicalWidth();
	desc.height = canvas.GetPhysicalHeight();
	desc.allow_hdr = allow_hdr;
	bool success = graphicsDevice->CreateSwapChain(&desc, window, &swapChain);
	assert(success);
	pf::image::Initialize();
	
}
void show() {
	if (!isInitialize) {
		intialize();
		isInitialize = true;
	}

	rendertargetPreHDR10 = {};
	ColorSpace colorspace = graphicsDevice->GetSwapChainColorSpace(&swapChain);
	if (!rendertargetPreHDR10.IsValid()) {
	//	Logger("rendertargetPreHDR10 is not valid");
		TextureDesc desc;
		desc.width = swapChain.desc.width;
		desc.height = swapChain.desc.height;
		desc.format = Format::R11G11B10_FLOAT;
		desc.bind_flags = BindFlag::RENDER_TARGET | BindFlag::SHADER_RESOURCE;
		// try to set background color for swapchain color as if it's using hdr scaling:
		desc.clear.color[0] = swapChain.desc.clear_color[0] * 9;
		desc.clear.color[1] = swapChain.desc.clear_color[1] * 9;
		desc.clear.color[2] = swapChain.desc.clear_color[2] * 9;
		desc.clear.color[3] = swapChain.desc.clear_color[3];
		bool success = graphicsDevice->CreateTexture(&desc, nullptr, &rendertargetPreHDR10);
		assert(success);
		graphicsDevice->SetName(&rendertargetPreHDR10, "Application::rendertargetPreHDR10");
	}

	CommandList cmd = graphicsDevice->BeginCommandList();

	if (rendertargetPreHDR10.IsValid())
	{
		//Logger("rendertargetPreHDR10 is valid begin width rendertargetPreHDR10");
		graphicsDevice->RenderPassBegin(&rendertargetPreHDR10, cmd, true);
		Viewport viewport;
		viewport.width = (float)swapChain.desc.width;
		viewport.height = (float)swapChain.desc.height;
		graphicsDevice->BindViewports(1, &viewport, cmd);
		static bool splash_screen_check = false;
	//	Logger("swapChain : width : %d , height : %d", swapChain.desc.width, swapChain.desc.height);
		if (!splash_screen.IsValid() && !splash_screen_check) {
		//	Logger("splash_screen is invalid ");
			splash_screen_check = true;
		//	Logger("Current Path : %s ", pf::helper::GetCurrentPath().c_str());
			std::string splash_screen_path = pf::helper::GetCurrentPath() + "/Res/logo_small.png";
			if (pf::helper::FileExists(splash_screen_path))
			{
			//	Logger("splash_screen_path  image is in ");
				pf::Resource resource = pf::resourcemanager::Load(splash_screen_path);
				if (resource.IsValid())
				{
					splash_screen = resource.GetTexture();
					splash_screen_subresource = graphicsDevice->CreateSubresource(&splash_screen, SubresourceType::SRV, 0, 1, 0, 1); // only first mip! mipgen is not initialized at this point...
				}

			}
			//Logger("Canvas w : %u , h : %u", canvas.GetPhysicalWidth(), canvas.GetPhysicalHeight());
		}
		if (splash_screen.IsValid()) {
			//Logger("Canvas w : %u , h : %u", canvas.GetPhysicalWidth(), canvas.GetPhysicalHeight());
			// Draw the splash screen while engine is initializing and image renderer is ready
			image::SetCanvas(canvas);
			image::Params fx;

			const TextureDesc& desc = splash_screen.GetDesc();
			const float canvas_aspect = canvas.GetLogicalWidth() / canvas.GetLogicalHeight();
			const float image_aspect = float(desc.width) / float(desc.height);
			if (canvas_aspect > image_aspect)
			{
				// display aspect is wider than image:
				fx.siz.x = canvas.GetLogicalWidth();
				fx.siz.y = canvas.GetLogicalHeight() / image_aspect * canvas_aspect;
			}
			else
			{
				// image aspect is wider or equal to display
				fx.siz.x = canvas.GetLogicalWidth() / canvas_aspect * image_aspect;
				fx.siz.y = canvas.GetLogicalHeight();
			}

			fx.pos = XMFLOAT3(canvas.GetLogicalWidth() * 0.5f, canvas.GetLogicalHeight() * 0.5f, 0);
			fx.pivot = XMFLOAT2(0.5f, 0.5f);
			if (colorspace != ColorSpace::SRGB)
			{
				fx.enableLinearOutputMapping(9);
			}
			fx.image_subresource = splash_screen_subresource;
			image::Draw(&splash_screen, fx, cmd);
		}
		graphicsDevice->RenderPassEnd(cmd);
		graphicsDevice->RenderPassBegin(&swapChain, cmd);
		image::Params fx;
		fx.enableFullScreen();
		fx.enableHDR10OutputMapping(); // this is doing the linear -> HDR10_ST2084 conversion
		image::Draw(&rendertargetPreHDR10, fx, cmd);
		graphicsDevice->RenderPassEnd(cmd);	
	}
	else
	{
		//graphicsDevice->RenderPassBegin(&swapChain, cmd);
	}
	graphicsDevice->SubmitCommandLists();
	//graphicsDevice->WaitForGPU();
	//Logger("Color space : %d", colorspace);
}

void run() {
	bool quit = false;
	while (!quit) {
		show();
		SDL_Event event;
		
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
				quit = true;
				break;
			case SDL_EVENT_WINDOW_RESIZED:
				break;
			case SDL_EVENT_WINDOW_FOCUS_LOST:
			//	tests.is_window_active = false;
				break;
			case SDL_EVENT_WINDOW_FOCUS_GAINED:
				break;
			}
		}
	}
}

void startmain() {
	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_HIDDEN);
	window = SDL_CreateWindow("Test Pursuit example", screenW, screenH, window_flags);
	graphicsDevice = std::make_unique<GraphicsDevice_Vulkan>(window, ValidationMode::Enabled, GPUPreference::Discrete);
	pf::graphics::GetDevice() = graphicsDevice.get();
	SDL_ShowWindow(window);
	run();
	SDL_Quit();
}

int test_vulkan_egine() {
	startmain();
	return 0;
}