#include "test_render_path2d_main.h"
#include "test_render_path.h"

#include "SDL3/SDL.h"
#include <Engine/Shaders/ShaderCompiler.h>
#include <Module/Util/sdl_input.h>
Tests tests;

int sdl_loop() {
	bool quit = false;
	while (!quit) {
		tests.Run();
		SDL_Event event;
		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_EVENT_QUIT:
				quit = true;
				break;
			case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
				quit = true;
				break;
			case SDL_EVENT_WINDOW_RESIZED:
				tests.SetWindow(tests.window);
				break;
			case SDL_EVENT_WINDOW_FOCUS_LOST:
				tests.is_window_active = false;
				break;
			case SDL_EVENT_WINDOW_FOCUS_GAINED:
				tests.is_window_active = true;
				if (pf::shadercompiler::GetRegisteredShaderCount() > 0)
				{
					std::thread([] {
						pf::backlogger::postin("[Shader check] Started checking " + std::to_string(pf::shadercompiler::GetRegisteredShaderCount()) + " registered shaders for changes...");
						if (pf::shadercompiler::CheckRegisteredShadersOutdated())
						{
							pf::backlogger::postin("[Shader check] Changes detected, initiating reload...");
							pf::eventhandler::Subscribe_Once(pf::eventhandler::EVENT_THREAD_SAFE_POINT, [](uint64_t userdata) {
								pf::renderer::ReloadShaders();
								});
						}
						else
						{
							pf::backlogger::postin("[Shader check] All up to date");
						}
						}).detach();
				}
				break;
			}
			pf::input::sdlinput::ProcessEvent(event);
		}
	}
	return 0;
}

int test_render_path_main(){
	if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS)) {
		SDL_Log("Failed : %s", SDL_GetError());
	}
	SDL_WindowFlags window_flags = (SDL_WindowFlags)(SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE  | SDL_WINDOW_HIDDEN);
	SDL_Window*  window_c = SDL_CreateWindow("Test Pursuit example", 1280, 800, window_flags );
	//graphicsDevice_c = std::make_unique<GraphicsDevice_Vulkan>(window_c, ValidationMode::Enabled, GPUPreference::Discrete);
	//pf::graphics::GetDevice() = graphicsDevice_c.get();
	SDL_ShowWindow(window_c);
	//if (!window_c) {
	//	throw sdl3::SDLError("Error creating window");
	//}
	tests.SetWindow(window_c);
	int ret = sdl_loop();
	SDL_Quit();
	return ret;
}