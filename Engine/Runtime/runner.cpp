#include "runner.h"
#include "Engine/Ecore/render_path.h"
#include "Engine/Ecore/renderer.h"
#include "Engine/Ecore/helper.h"
#include  "module.h"
#include "Core/pf_version.h"
#include "Engine/Ecore/enums.h"
#include "Engine/Ecore/texture_helper.h"
#include "initializer.h"
#include "Module/Util/arguments.h"
#include "Engine/Component/font.h"
#include "Engine/Ecore/draw_image.h"
#include "Core/platform.h"

#include <string>
#include <algorithm>
#include <new>
#include <cstdlib>
#include <atomic>
#include "Engine/Device/vulkan_driver.h"
#include <Engine/Ecore/input.h>



using namespace pf::graphics;

namespace pf {

	void Application::Initialize() {
		if (initialized)
		{
			return;
		}
		initialized = true;

		initializer::InitializeComponentsAsync();

		alwaysactive = pf::arguments::HasArgument("alwaysactive");

		// Note: lua is always initialized immediately on main thread by pf::initializer, so this is safe to do:
		// assert(pf::initializer::IsInitializeFinished(pf::initializer::INITIALIZED_SYSTEM_LUA));

	}

	void Application::ActivatePath(RenderPath* component, float fadeSeconds, Color fadeColor, FadeManager::FadeType fadetype)
	{
		if (component != nullptr)
		{
			component->init(canvas);
		}

		// Fade manager will activate on fadeout
		fadeManager.Start(fadeSeconds, fadeColor, [this, component]() {

			if (activePath != nullptr)
			{
				activePath->Stop();
			}

			if (component != nullptr)
			{
				component->Start();
			}
			activePath = component;
			}, fadetype);

		fadeManager.Update(0); // If user calls ActivatePath without fadeout, it will be instant
	}

	void Application::Run() {

		if (!initialized)
		{
			// Initialize in a lazy way, so the user application doesn't have to call this explicitly
			Initialize();
			initialized = true;
		}

		font::UpdateAtlas(canvas.GetDPIScaling());

		ColorSpace colorspace = graphicsDevice->GetSwapChainColorSpace(&swapChain);

		if (colorspace == ColorSpace::HDR10_ST2084) {
			// In HDR10, we perform the compositing in a custom linear color space render target
			//	The reason is that blending doesn't look good in HDR10 color space
			//	In HDR10 the composition is done like:
			//	rendertargetPreHDR10:
			//		- RenderPath3D: linear space
			//		- RenderPath2D: SRGB space -> linear space with HDR scaling
			//	swapChain:
			//		- HDR10 composition: linear -> HDR10_ST2084
			if (!rendertargetPreHDR10.IsValid())
			{
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
		}
		else
		{
			// If swapchain is SRGB or Linear HDR, it can be used for blending and rendertargetPreHDR10 is not needed
			//	- If it is SRGB, the render path will ensure tonemapping to SDR
			//	- If it is Linear HDR, we can blend trivially in linear space
			rendertargetPreHDR10 = {};
		}

		if (!initializer::IsInitializeFinished()) {
			// Until engine is not loaded, present initialization screen...
			CommandList cmd = graphicsDevice->BeginCommandList();
			if (rendertargetPreHDR10.IsValid())
			{
				graphicsDevice->RenderPassBegin(&rendertargetPreHDR10, cmd, true);
			}
			else
			{
				graphicsDevice->RenderPassBegin(&swapChain, cmd);
			}

			Viewport viewport;
			viewport.width = (float)swapChain.desc.width;
			viewport.height = (float)swapChain.desc.height;
			graphicsDevice->BindViewports(1, &viewport, cmd);
			static bool splash_screen_check = false;

			if (!splash_screen.IsValid() && !splash_screen_check)
			{
				splash_screen_check = true;
				std::string splash_screen_path = helper::GetCurrentPath() + "/splash_screen.png";
				if (helper::FileExists(splash_screen_path))
				{
					Resource resource = resourcemanager::Load(splash_screen_path);
					if (resource.IsValid())
					{
						splash_screen = resource.GetTexture();
						splash_screen_subresource = graphicsDevice->CreateSubresource(&splash_screen, SubresourceType::SRV, 0, 1, 0, 1); // only first mip! mipgen is not initialized at this point...
					}
				}
			}

			if (splash_screen.IsValid())
			{
				if (pf::initializer::IsInitializeFinished(pf::initializer::INITIALIZED_SYSTEM_IMAGE))
				{
					// Draw the splash screen while engine is initializing and image renderer is ready
					pf::image::SetCanvas(canvas);
					pf::image::Params fx;
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
					pf::image::Draw(&splash_screen, fx, cmd);
				}
			}
			else if (pf::initializer::IsInitializeFinished(pf::initializer::INITIALIZED_SYSTEM_FONT))
			{
				// If there is no splash screen, the log is rendered while engine is initializing
				ColorSpace colorspace = graphicsDevice->GetSwapChainColorSpace(&swapChain);
				//TO DO :pf::backlogger::DrawOutputText(canvas, cmd, colorspace);
			}
			graphicsDevice->RenderPassEnd(cmd);

			if (rendertargetPreHDR10.IsValid() && pf::initializer::IsInitializeFinished(pf::initializer::INITIALIZED_SYSTEM_IMAGE))
			{
				// In HDR10, we perform a final mapping from linear to HDR10, into the swapchain
				graphicsDevice->RenderPassBegin(&swapChain, cmd);
				pf::image::Params fx;
				fx.enableFullScreen();
				fx.enableHDR10OutputMapping(); // this is doing the linear -> HDR10_ST2084 conversion
				pf::image::Draw(&rendertargetPreHDR10, fx, cmd);
				graphicsDevice->RenderPassEnd(cmd);
			}

			graphicsDevice->SubmitCommandLists();
			return;
		}

		splash_screen = {}; // splash screen no longer needed after initialization, it is deleted

		static bool startup_script = false;
		if (!startup_script){
			startup_script = true;
			const std::string workingdir = pf::helper::GetCurrentPath() + "/";
			const std::string rewriteable_script_filename = workingdir + rewriteable_startup_script_text;
			if (pf::helper::FileExists(rewriteable_script_filename))
			{
				/*
				if (pf::lua::RunFile(rewriteable_script_filename))
				{
					pf::backlog::post("Executed startup file: " + rewriteable_script_filename);
				}
				*/
			}
			else
			{
				const std::string startup_lua_filename = workingdir + "startup.lua";
				if (pf::helper::FileExists(startup_lua_filename))
				{
					/*if (pf::lua::RunFile(startup_lua_filename))
					{
						pf::backlog::post("Executed startup file: " + startup_lua_filename);
					}*/
				}
				const std::string startup_luab_filename = workingdir + "startup.luab";
				if (pf::helper::FileExists(startup_luab_filename))
				{
					/*
					if (pf::lua::RunBinaryFile(startup_luab_filename))
					{
						pf::backlog::post("Executed startup file: " + startup_luab_filename);
					}
					*/
				}
			}
		}

		if (!is_window_active && !alwaysactive)
		{
			// If the application is not active, disable Update loops:
			deltaTimeAccumulator = 0;
			pf::helper::Sleep(10);
			pf::input::Update(window, canvas); // update input while inactive, this solves a problem with past inputs processed immediately after activation
			timer.record_elapsed_seconds(); // after application becomes active, delta time shouldn't spike, could blow up gameplay or physics
			return;
		}

		profiler::BeginFrame();

		deltaTime = float(timer.record_elapsed_seconds());

		const float target_deltaTime = 1.0f / targetFrameRate;
		if (framerate_lock && deltaTime < target_deltaTime)
		{
			pf::helper::QuickSleep((target_deltaTime - deltaTime) * 1000);
			deltaTime += float(timer.record_elapsed_seconds());
		}


		// avoid instability caused by large delta time
		deltaTime = clamp(deltaTime, 0.0f, 0.5f);

		input::Update(window, canvas);

		eventhandler::FireEvent(eventhandler::EVENT_THREAD_SAFE_POINT, 0);

		fadeManager.Update(deltaTime);

		if (activePath != nullptr)
		{
			ColorSpace colorspace = graphicsDevice->GetSwapChainColorSpace(&swapChain);
			activePath->colorspace = colorspace;
			activePath->init(canvas);
			activePath->PreUpdate();
		}
	
		// Fixed time update:
		auto range = pf::profiler::BeginRangeCPU("Fixed Update");
		{
			if (frameskip)
			{
				deltaTimeAccumulator += deltaTime;
				if (deltaTimeAccumulator > 10)
				{
					// application probably lost control, fixed update would take too long
					deltaTimeAccumulator = 0;
				}

				const float targetFrameRateInv = 1.0f / targetFrameRate;
				while (deltaTimeAccumulator >= targetFrameRateInv)
				{
					FixedUpdate();
					deltaTimeAccumulator -= targetFrameRateInv;
				}
			}
			else
			{
				FixedUpdate();
			}
		}

		profiler::EndRange(range); // Fixed Update

		// Variable-timed update:
		Update(deltaTime);

		Render();



		// Begin final compositing:
		CommandList cmd = graphicsDevice->BeginCommandList();

		// CrossFade texture save:
		if (fadeManager.crossFadeTextureSaveRequired)
		{
			Texture backbuffer = rendertargetPreHDR10.IsValid() ? rendertargetPreHDR10 : graphicsDevice->GetBackBuffer(&swapChain);
			if (
				fadeManager.crossFadeTexture.desc.width != backbuffer.desc.width ||
				fadeManager.crossFadeTexture.desc.height != backbuffer.desc.height ||
				fadeManager.crossFadeTexture.desc.format != backbuffer.desc.format
				)
			{
				TextureDesc desc = backbuffer.desc;
				desc.bind_flags = BindFlag::SHADER_RESOURCE;
				bool success = graphicsDevice->CreateTexture(&desc, nullptr, &fadeManager.crossFadeTexture);
				assert(success);
				graphicsDevice->SetName(&fadeManager.crossFadeTexture, "wiFadeManager::crossFadeTexture");
			}
			pf::renderer::PushBarrier(GPUBarrier::Image(&backbuffer, backbuffer.desc.layout, ResourceState::COPY_SRC));
			pf::renderer::PushBarrier(GPUBarrier::Image(&fadeManager.crossFadeTexture, fadeManager.crossFadeTexture.desc.layout, ResourceState::COPY_DST));
			pf::renderer::FlushBarriers(cmd);
			graphicsDevice->CopyResource(&fadeManager.crossFadeTexture, &backbuffer, cmd);
			pf::renderer::PushBarrier(GPUBarrier::Image(&fadeManager.crossFadeTexture, ResourceState::COPY_DST, fadeManager.crossFadeTexture.desc.layout));
			pf::renderer::PushBarrier(GPUBarrier::Image(&backbuffer, ResourceState::COPY_SRC, backbuffer.desc.layout));
			pf::renderer::FlushBarriers(cmd);
			fadeManager.crossFadeTextureSaveRequired = false;
		}

		pf::image::SetCanvas(canvas);
		pf::font::SetCanvas(canvas);
		Viewport viewport;
		viewport.width = (float)swapChain.desc.width;
		viewport.height = (float)swapChain.desc.height;
		graphicsDevice->BindViewports(1, &viewport, cmd);

		if (rendertargetPreHDR10.IsValid())
		{
			graphicsDevice->RenderPassBegin(&rendertargetPreHDR10, cmd, true);
		}
		else
		{
			graphicsDevice->RenderPassBegin(&swapChain, cmd);
		}

		Compose(cmd);
		graphicsDevice->RenderPassEnd(cmd);


		if (rendertargetPreHDR10.IsValid())
		{
			// In HDR10, we perform a final mapping from linear to HDR10, into the swapchain
			graphicsDevice->RenderPassBegin(&swapChain, cmd);
			pf::image::Params fx;
			fx.enableFullScreen();
			fx.enableHDR10OutputMapping();
			pf::image::Draw(&rendertargetPreHDR10, fx, cmd);
			graphicsDevice->RenderPassEnd(cmd);
		}

		pf::input::ClearForNextFrame();
		pf::profiler::EndFrame(cmd);
		graphicsDevice->SubmitCommandLists();
		pf::renderer::UpdateGPUSuballocator();
	}

	void Application::Update(float dt)
	{
		auto range = pf::profiler::BeginRangeCPU("Update");

		infoDisplay.rect = {};

		//TO DO
		//pf::lua::SetDeltaTime(double(dt));
		//pf::lua::Update();

		//pf::backlogger::Update(canvas, dt);

		pf::resourcemanager::UpdateStreamingResources(dt);

		if (activePath != nullptr)
		{
			activePath->Update(dt);
			activePath->PostUpdate();
		}

		pf::profiler::EndRange(range); // Update
	}

	void Application::FixedUpdate()
	{
		//TO DO : pf::lua::FixedUpdate();

		if (activePath != nullptr)
		{
			activePath->FixedUpdate();
		}
	}

	void Application::Render()
	{
		auto range = pf::profiler::BeginRangeCPU("Render");

		//TO DO : pf::lua::Render();

		if (activePath != nullptr)
		{
			activePath->PreRender();
			activePath->Render();
			activePath->PostRender();
		}

		pf::profiler::EndRange(range); // Render
	}

	void Application::Compose(CommandList cmd)
	{
		auto range = pf::profiler::BeginRangeCPU("Compose");
		ColorSpace colorspace = graphicsDevice->GetSwapChainColorSpace(&swapChain);

		if (activePath != nullptr)
		{
			activePath->Compose(cmd);
		}

		if (fadeManager.IsActive())
		{
			// display fade rect
			pf::image::Params fx;
			fx.enableFullScreen();
			fx.opacity = fadeManager.opacity;
			if (fadeManager.type == FadeManager::FadeType::FadeToColor)
			{
				fx.color = fadeManager.color;
				pf::image::Draw(nullptr, fx, cmd);
			}
			else if (fadeManager.type == FadeManager::FadeType::CrossFade)
			{
				pf::image::Draw(&fadeManager.crossFadeTexture, fx, cmd);
			}
		}
		else
		{
			fadeManager.crossFadeTexture = {};
		}

		// Draw the information display
		if (infoDisplay.active)
		{
			if (infoDisplay.rect.right > 0)
			{
				graphicsDevice->BindScissorRects(1, &infoDisplay.rect, cmd);
			}

			infodisplay_str.clear();
			if (infoDisplay.watermark)
			{
				infodisplay_str += "Wicked Engine ";
				infodisplay_str += pf::version::GetVersionString();
				infodisplay_str += " ";

#if defined(PLATFORM_WINDOWS_DESKTOP)
				infodisplay_str += "[Windows]";
#elif defined(PLATFORM_LINUX)
				infodisplay_str += "[Linux]";
#elif defined(PLATFORM_PS5)
				infodisplay_str += "[PS5]";
#elif defined(PLATFORM_XBOX)
				infodisplay_str += "[Xbox]";
#endif // PLATFORM

#if defined(_ARM)
				infodisplay_str += "[ARM]";
#elif defined(_WIN64)
				infodisplay_str += "[64-bit]";
#elif defined(_WIN32)
				infodisplay_str += "[32-bit]";
#endif // _ARM

#ifdef WICKEDENGINE_BUILD_DX12
				if (dynamic_cast<GraphicsDevice_DX12*>(graphicsDevice.get()))
				{
					infodisplay_str += "[DX12]";
				}
#endif // WICKEDENGINE_BUILD_DX12
#ifdef WICKEDENGINE_BUILD_VULKAN
				if (dynamic_cast<GraphicsDevice_Vulkan*>(graphicsDevice.get()))
				{
					infodisplay_str += "[Vulkan]";
				}
#endif // WICKEDENGINE_BUILD_VULKAN

#ifdef _DEBUG
				infodisplay_str += "[DEBUG]";
#endif // _DEBUG
				if (graphicsDevice->IsDebugDevice())
				{
					infodisplay_str += "[debugdevice]";
				}
				infodisplay_str += "\n";
			}
			if (infoDisplay.device_name)
			{
				infodisplay_str += "Device: ";
				infodisplay_str += graphicsDevice->GetAdapterName();
				infodisplay_str += "\n";
			}
			if (infoDisplay.resolution)
			{
				infodisplay_str += "Resolution: ";
				infodisplay_str += std::to_string(canvas.GetPhysicalWidth());
				infodisplay_str += " x ";
				infodisplay_str += std::to_string(canvas.GetPhysicalHeight());
				infodisplay_str += " (";
				infodisplay_str += std::to_string(int(canvas.GetDPI()));
				infodisplay_str += " dpi)\n";
			}
			if (infoDisplay.logical_size)
			{
				infodisplay_str += "Logical Size: ";
				infodisplay_str += std::to_string(int(canvas.GetLogicalWidth()));
				infodisplay_str += " x ";
				infodisplay_str += std::to_string(int(canvas.GetLogicalHeight()));
				infodisplay_str += "\n";
			}
			if (infoDisplay.colorspace)
			{
				infodisplay_str += "Color Space: ";
				ColorSpace colorSpace = graphicsDevice->GetSwapChainColorSpace(&swapChain);
				switch (colorSpace)
				{
				default:
				case pf::graphics::ColorSpace::SRGB:
					infodisplay_str += "sRGB";
					break;
				case pf::graphics::ColorSpace::HDR10_ST2084:
					infodisplay_str += "ST.2084 (HDR10)";
					break;
				case pf::graphics::ColorSpace::HDR_LINEAR:
					infodisplay_str += "Linear (HDR)";
					break;
				}
				infodisplay_str += "\n";
			}
			if (infoDisplay.fpsinfo)
			{
				deltatimes[fps_avg_counter++ % arraysize(deltatimes)] = deltaTime;
				float displaydeltatime = deltaTime;
				if (fps_avg_counter > arraysize(deltatimes))
				{
					float avg_time = 0;
					for (int i = 0; i < arraysize(deltatimes); ++i)
					{
						avg_time += deltatimes[i];
					}
					displaydeltatime = avg_time / arraysize(deltatimes);
				}

				infodisplay_str += std::to_string(int(std::round(1.0f / displaydeltatime))) + " FPS\n";
			}
			if (infoDisplay.heap_allocation_counter)
			{
				infodisplay_str += "Heap allocations per frame: ";
#ifdef WICKED_ENGINE_HEAP_ALLOCATION_COUNTER
				infodisplay_str += std::to_string(number_of_heap_allocations.load());
				infodisplay_str += " (";
				infodisplay_str += std::to_string(size_of_heap_allocations.load());
				infodisplay_str += " bytes)\n";
				number_of_heap_allocations.store(0);
				size_of_heap_allocations.store(0);
#else
				infodisplay_str += "[disabled]\n";
#endif // WICKED_ENGINE_HEAP_ALLOCATION_COUNTER
			}
			if (infoDisplay.pipeline_count)
			{
				infodisplay_str += "Graphics pipelines active: ";
				infodisplay_str += std::to_string(graphicsDevice->GetActivePipelineCount());
				infodisplay_str += "\n";
			}

			if (infoDisplay.pipeline_creation)
			{
				int pipeline_creation = pf::renderer::IsPipelineCreationActive();
				if (pipeline_creation > 0)
				{
					infodisplay_str += "Pending pipeline creations by graphics driver: " + std::to_string(pipeline_creation) + ". Some rendering will be skipped.\n";
				}
			}

			pf::font::Params params = pf::font::Params(
				4 + canvas.PhysicalToLogical((uint32_t)infoDisplay.rect.left),
				4 + canvas.PhysicalToLogical((uint32_t)infoDisplay.rect.top),
				infoDisplay.size,
				pf::font::WIFALIGN_LEFT,
				pf::font::WIFALIGN_TOP,
				pf::Color::White(),
				pf::Color::Shadow()
			);

			params.shadow_softness = 0.4f;

			// Explanation: this compose pass is in LINEAR space if display output is linear or HDR10
			//	If HDR10, the HDR10 output mapping will be performed on whole image later when drawing to swapchain
			if (colorspace != ColorSpace::SRGB)
			{
				params.enableLinearOutputMapping(9);
			}

			params.cursor = pf::font::Draw(infodisplay_str, params, cmd);

			// VRAM:
			{
				GraphicsDevice::MemoryUsage vram = graphicsDevice->GetMemoryUsage();
				bool warn = false;
				if (vram.usage > vram.budget)
				{
					params.color = pf::Color::Error();
					warn = true;
				}
				else if (float(vram.usage) / float(vram.budget) > 0.9f)
				{
					params.color = pf::Color::Warning();
					warn = true;
				}
				if (infoDisplay.vram_usage || warn)
				{
					params.cursor = pf::font::Draw("VRAM usage: " + std::to_string(vram.usage / 1024 / 1024) + "MB / " + std::to_string(vram.budget / 1024 / 1024) + "MB\n", params, cmd);
					params.color = pf::Color::White();
				}
			}

			// Write warnings below:
			params.color = pf::Color::Warning();
#ifdef _DEBUG
			params.cursor = pf::font::Draw("Warning: This is a [DEBUG] build, performance will be slow!\n", params, cmd);
#endif
			if (graphicsDevice->IsDebugDevice())
			{
				params.cursor = pf::font::Draw("Warning: Graphics is in [debugdevice] mode, performance will be slow!\n", params, cmd);
			}

			// Write errors below:
			params.color = pf::Color::Error();
			if (pf::renderer::GetShaderMissingCount() > 0)
			{
				params.cursor = pf::font::Draw(std::to_string(pf::renderer::GetShaderMissingCount()) + " shaders missing! Check the backlog for more information!\n", params, cmd);
			}
			if (pf::renderer::GetShaderErrorCount() > 0)
			{
				params.cursor = pf::font::Draw(std::to_string(pf::renderer::GetShaderErrorCount()) + " shader compilation errors! Check the backlog for more information!\n", params, cmd);
			}
			/*if (pf::backlogger::GetUnseenLogLevelMax() >=LogLevel::Error)
			{
				params.cursor = pf::font::Draw("Errors found, check the backlog for more information!", params, cmd);
			}*/

			if (infoDisplay.colorgrading_helper)
			{
				pf::image::Draw(
					pf::texturehelper::getColorGradeDefault(),
					pf::image::Params(
						canvas.PhysicalToLogical((uint32_t)infoDisplay.rect.left),
						canvas.PhysicalToLogical((uint32_t)infoDisplay.rect.top),
						canvas.PhysicalToLogical(256),
						canvas.PhysicalToLogical(16)
					),
					cmd
				);
			}

			if (infoDisplay.rect.right > 0)
			{
				Rect rect;
				rect.right = canvas.width;
				rect.bottom = canvas.height;
				graphicsDevice->BindScissorRects(1, &rect, cmd);
			}
		}

		pf::profiler::DrawData(canvas, 4, 10, cmd, colorspace);

		//pf::backlog::Draw(canvas, cmd, colorspace);

		pf::profiler::EndRange(range); // Compose
	}
	void Application::Exit()
	{
		platform::Exit();
	}


	void Application::SetWindow(platform::window_type window)
	{
		this->window = window;

		// User can also create a graphics device if custom logic is desired, but they must do before this function!
		if (graphicsDevice == nullptr)
		{
			ValidationMode validationMode = ValidationMode::Disabled;
			if (pf::arguments::HasArgument("debugdevice"))
			{
				validationMode = ValidationMode::Enabled;
			}
			if (pf::arguments::HasArgument("gpuvalidation"))
			{
				validationMode = ValidationMode::GPU;
			}
			if (pf::arguments::HasArgument("gpu_verbose"))
			{
				validationMode = ValidationMode::Verbose;
			}

			GPUPreference preference = GPUPreference::Discrete;
			if (pf::arguments::HasArgument("igpu"))
			{
				preference = GPUPreference::Integrated;
			}
			else if (pf::arguments::HasArgument("nvidiagpu"))
			{
				preference = GPUPreference::Nvidia;
			}
			else if (pf::arguments::HasArgument("amdgpu"))
			{
				preference = GPUPreference::AMD;
			}
			else if (pf::arguments::HasArgument("intelgpu"))
			{
				preference = GPUPreference::Intel;
			}

#ifdef PLATFORM_PS5
			pf::renderer::SetShaderPath(pf::renderer::GetShaderPath() + "ps5/");
			graphicsDevice = std::make_unique<GraphicsDevice_PS5>(validationMode);

#else
			bool use_dx12 = pf::arguments::HasArgument("dx12");
			bool use_vulkan = pf::arguments::HasArgument("vulkan");

#ifndef WICKEDENGINE_BUILD_DX12
			if (use_dx12) {
				//pf::helper::messageBox("The engine was built without DX12 support!", "Error");
				use_dx12 = false;
			}
#endif // WICKEDENGINE_BUILD_DX12
#ifndef WICKEDENGINE_BUILD_VULKAN
			if (use_vulkan) {
				//pf::helper::messageBox("The engine was built without Vulkan support!", "Error");
				use_vulkan = false;
			}
#endif // WICKEDENGINE_BUILD_VULKAN

			if (!use_dx12 && !use_vulkan)
			{
#if defined(WICKEDENGINE_BUILD_DX12)
				use_dx12 = true;
#elif defined(WICKEDENGINE_BUILD_VULKAN)
				use_vulkan = true;
#else
				pf::backlogger::postin("No rendering backend is enabled! Please enable at least one so we can use it as default", LogLevel::Error);
				assert(false);
#endif
			}
			assert(use_dx12 || use_vulkan);

			if (use_vulkan)
			{
#ifdef WICKEDENGINE_BUILD_VULKAN
				pf::renderer::SetShaderPath(pf::renderer::GetShaderPath() + "spirv/");
				graphicsDevice = std::make_unique<GraphicsDevice_Vulkan>(window, validationMode, preference);
#endif
			}
			else if (use_dx12)
			{
#ifdef WICKEDENGINE_BUILD_DX12
#ifdef PLATFORM_XBOX
				pf::renderer::SetShaderPath(pf::renderer::GetShaderPath() + "hlsl6_xs/");
#else
				pf::renderer::SetShaderPath(pf::renderer::GetShaderPath() + "hlsl6/");
#endif // PLATFORM_XBOX
				graphicsDevice = std::make_unique<GraphicsDevice_DX12>(validationMode, preference);
#endif
			}
#endif // PLATFORM_PS5
		}
		pf::graphics::GetDevice() = graphicsDevice.get();

		canvas.init(window);

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
		desc.width = canvas.GetPhysicalWidth();
		desc.height = canvas.GetPhysicalHeight();
		desc.allow_hdr = allow_hdr;
		bool success = graphicsDevice->CreateSwapChain(&desc, window, &swapChain);
		assert(success);

		rendertargetPreHDR10 = {};

#ifdef PLATFORM_PS5
		// PS5 swapchain resolution was decided in CreateSwapchain(), so reinit canvas:
		canvas.init(swapChain.desc.width, swapChain.desc.height);
#endif // PLATFORM_PS5

		swapChainVsyncChangeEvent = pf::eventhandler::Subscribe(pf::eventhandler::EVENT_SET_VSYNC, [this](uint64_t userdata) {
			SwapChainDesc desc = swapChain.desc;
			desc.vsync = userdata != 0;
			bool success = graphicsDevice->CreateSwapChain(&desc, nullptr, &swapChain);
			assert(success);
			});

	}


	void Application::SetFullScreen(bool fullscreen)
	{
#if defined(PLATFORM_WINDOWS_DESKTOP)
#ifdef ENABLESDL3
		SDL_SetWindowFullscreen(window, fullscreen);

#else
		// Based on: https://devblogs.microsoft.com/oldnewthing/20100412-00/?p=14353
		static WINDOWPLACEMENT wp = {};
		DWORD dwStyle = GetWindowLong(window, GWL_STYLE);
		bool currently_windowed = dwStyle & WS_OVERLAPPEDWINDOW;
		if (currently_windowed && fullscreen) {
			MONITORINFO mi = { sizeof(mi) };
			if (GetWindowPlacement(window, &wp) &&
				GetMonitorInfo(MonitorFromWindow(window,
					MONITOR_DEFAULTTOPRIMARY), &mi)) {
				SetWindowLong(window, GWL_STYLE,
					dwStyle & ~WS_OVERLAPPEDWINDOW);
				SetWindowPos(window, HWND_TOP,
					mi.rcMonitor.left, mi.rcMonitor.top,
					mi.rcMonitor.right - mi.rcMonitor.left,
					mi.rcMonitor.bottom - mi.rcMonitor.top,
					SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
			}
		}
		else if (!currently_windowed && !fullscreen) {
			SetWindowLong(window, GWL_STYLE,
				dwStyle | WS_OVERLAPPEDWINDOW);
			SetWindowPlacement(window, &wp);
			SetWindowPos(window, NULL, 0, 0, 0, 0,
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
				SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
		}
#endif // ENABLESDL3
#elif defined(PLATFORM_LINUX)
		SDL_SetWindowFullscreen(window, fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
#endif // PLATFORM_WINDOWS_DESKTOP
	}

	bool Application::IsScriptReplacement() const
	{
		return pf::helper::FileExists(rewriteable_startup_script_text);
	}

}