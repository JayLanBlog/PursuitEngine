#include "app_run.h"
#include "Module/Util/profiler.h"


namespace pf::arun {
	
	void TLauncher::intialize() {


		// SDL3 建议只初始化真正需要的子系统
		if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
			// SDL3 中返回 false 表示失败
			SDL_Log("失败: %s", SDL_GetError());
		}
		pf::graphics::GetDevice() = graphicsDevice_c.get();
		//	canvas.init(window);
	 
		canvas_c.init(imageW_c, imageH_c);

		font::SetCanvas(canvas_c);

		SwapChainDesc desc = swapChain_c.desc;
		if (!swapChain_c.IsValid())
		{
			// initialize for the first time
			desc.buffer_count = 3;
			if (graphicsDevice_c->CheckCapability(GraphicsDeviceCapability::R9G9B9E5_SHAREDEXP_RENDERABLE))
			{
				desc.format = Format::R9G9B9E5_SHAREDEXP;
			}
			else
			{
				desc.format = Format::R10G10B10A2_UNORM;
			}
		}

		desc.width = canvas_c.GetLogicalWidth();
		desc.height = canvas_c.GetPhysicalHeight();
		desc.allow_hdr = allow_hdr_c;
		bool success = graphicsDevice_c->CreateSwapChain(&desc, window_c, &swapChain_c);
		assert(success);
		pf::image::Initialize();
		pf::font::Initialize();
		pf::profiler::SetEnabled(true);
		infodisplay_str_c = "Hello Test Engine";

	}

	void TLauncher::render() {
		if (!isInitialize_c) {
			intialize();
			isInitialize_c = true;
		}
		font::UpdateAtlas(canvas_c.GetDPIScaling());

		rendertargetPreHDR10_c = {};
		ColorSpace colorspace = graphicsDevice_c->GetSwapChainColorSpace(&swapChain_c);
		if (!rendertargetPreHDR10_c.IsValid()) {
			//	Logger("rendertargetPreHDR10 is not valid");
			TextureDesc desc;
			desc.width = swapChain_c.desc.width;
			desc.height = swapChain_c.desc.height;
			desc.format = Format::R11G11B10_FLOAT;
			desc.bind_flags = BindFlag::RENDER_TARGET | BindFlag::SHADER_RESOURCE;
			// try to set background color for swapchain color as if it's using hdr scaling:
			desc.clear.color[0] = swapChain_c.desc.clear_color[0] * 9;
			desc.clear.color[1] = swapChain_c.desc.clear_color[1] * 9;
			desc.clear.color[2] = swapChain_c.desc.clear_color[2] * 9;
			desc.clear.color[3] = swapChain_c.desc.clear_color[3];
			bool success = graphicsDevice_c->CreateTexture(&desc, nullptr, &rendertargetPreHDR10_c);
			assert(success);
			graphicsDevice_c->SetName(&rendertargetPreHDR10_c, "Application::rendertargetPreHDR10");
		}

		CommandList cmd = graphicsDevice_c->BeginCommandList();

//		auto range = pf::profiler::BeginRangeGPU("TLauncher render", cmd);
		auto range = pf::profiler::BeginRangeCPU("Compose");

		splash_screen_c = {}; // splash screen no longer needed after initialization, it is deleted

		//static bool startup_script = false;


		Viewport viewport;
		viewport.width = (float)swapChain_c.desc.width;
		viewport.height = (float)swapChain_c.desc.height;

		graphicsDevice_c->BindViewports(1, &viewport, cmd);
		//rect.left = 10;
		//rect.top = 10;
		graphicsDevice_c->RenderPassBegin(&rendertargetPreHDR10_c, cmd, true);
		font::Params params = font::Params(
			4 + canvas_c.PhysicalToLogical((uint32_t)rect.left),
			4 + canvas_c.PhysicalToLogical((uint32_t)rect.top),
			size,
			font::WIFALIGN_LEFT,
			font::WIFALIGN_TOP,
			Color::White(),
			Color::Shadow()
		);
		params.shadow_softness = 0.4f;
		params.cursor = pf::font::Draw(infodisplay_str_c, params, cmd);

		//Draw Font
		graphicsDevice_c->RenderPassEnd(cmd);

		profiler::DrawData(canvas_c, 4, 10, cmd, colorspace);

		pf::profiler::EndRange(range); // BVH rebuild
		// In HDR10, we perform a final mapping from linear to HDR10, into the swapchain
		graphicsDevice_c->RenderPassBegin(&swapChain_c, cmd);
		pf::image::Params fx;
		fx.enableFullScreen();
		fx.enableHDR10OutputMapping();
		pf::image::Draw(&rendertargetPreHDR10_c, fx, cmd);
		graphicsDevice_c->RenderPassEnd(cmd);
		
		
		graphicsDevice_c->SubmitCommandLists();

	
	}

	void TLauncher::run() {
		SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY | SDL_WINDOW_HIDDEN);
		window_c = SDL_CreateWindow("Test Pursuit example", screenW_c, screenH_c, window_flags);
		graphicsDevice_c = std::make_unique<GraphicsDevice_Vulkan>(window_c, ValidationMode::Enabled, GPUPreference::Discrete);
		pf::graphics::GetDevice() = graphicsDevice_c.get();
		SDL_ShowWindow(window_c);

		bool quit = false;
		while (!quit) {
			render();
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
		SDL_Quit();
	}
}