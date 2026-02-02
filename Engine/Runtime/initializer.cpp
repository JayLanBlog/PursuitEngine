#include "initializer.h"
#include "Engine/pfengine.h"
#include <thread>
#include <atomic>

#if defined(PLATFORM_WINDOWS_DESKTOP) || defined(PLATFORM_LINUX)
#include "Utility/cpuinfo.hpp"
#endif // defined(PLATFORM_WINDOWS_DESKTOP) || defined(PLATFORM_LINUX)
#include <Module/Logger/backlogger.h>
#include <Engine/Ecore/renderer.h>
#include <Module/Util/input.h>

#include <Engine/Ecore/gpu_sortlib.h>
#include <Engine/Ecore/physics.h>
#include "Engine/Ecore/trail_renderer.h"
#include <Engine/Ecore/texture_helper.h>

namespace pf::initializer {
	static std::atomic_bool initializationStarted{ false };
	static pf::jobsystem::context ctx;
	static pf::Timer timer;
	static std::atomic_bool systems[INITIALIZED_SYSTEM_COUNT]{};


	// Initializes systems and blocks CPU until it is complete
	void InitializeComponentsImmediate() {
		if (IsInitializeFinished())
			return;
		if (!initializationStarted.load())
		{
			InitializeComponentsAsync();
		}
		WaitForInitializationsToFinish();
	}

	// Begins initializing systems, but doesn't block CPU. Check completion status with IsInitializeFinished()
	void InitializeComponentsAsync() {
		if (IsInitializeFinished())
			return;
		timer.record();

		initializationStarted.store(true);

#if defined(PLATFORM_WINDOWS_DESKTOP)
		static constexpr const char* platform_string = "Windows";
#elif defined(PLATFORM_LINUX)
		static constexpr const char* platform_string = "Linux";
#elif defined(PLATFORM_PS5)
		static constexpr const char* platform_string = "PS5";
#elif defined(PLATFORM_XBOX)
		static constexpr const char* platform_string = "Xbox";
#endif // PLATFORM
		p_log("\n[pf::initializer] Initializing Wicked Engine, please wait...\nVersion: %s\nPlatform: %s", pf::version::GetVersionString(), platform_string);

		StackString<1024> cpustring;
#if defined(PLATFORM_WINDOWS_DESKTOP) || defined(PLATFORM_LINUX)
		CPUInfo cpuinfo;
		cpustring.push_back("\nCPU: ");
		cpustring.push_back(cpuinfo.model().c_str());
		cpustring.push_back("\n\tFeatures available: ");
		if (cpuinfo.haveSSE())
		{
			cpustring.push_back("SSE; ");
		}
		if (cpuinfo.haveSSE2())
		{
			cpustring.push_back("SSE 2; ");
		}
		if (cpuinfo.haveSSE3())
		{
			cpustring.push_back("SSE 3; ");
		}
		if (cpuinfo.haveSSE41())
		{
			cpustring.push_back("SSE 4.1; ");
		}
		if (cpuinfo.haveSSE42())
		{
			cpustring.push_back("SSE 4.2; ");
		}
		if (cpuinfo.haveAVX())
		{
			cpustring.push_back("AVX; ");
		}
		if (cpuinfo.haveFMA3())
		{
			cpustring.push_back("FMA3; ");
		}
		if (cpuinfo.haveF16C())
		{
			cpustring.push_back("F16C; ");
		}
		if (cpuinfo.haveAVX2())
		{
			cpustring.push_back("AVX 2; ");
		}
		if (cpuinfo.haveAVX512F())
		{
			cpustring.push_back("AVX 512; ");
		}
#endif // defined(PLATFORM_WINDOWS_DESKTOP) || defined(PLATFORM_LINUX)
		cpustring.push_back("\n\tFeatures used: ");
#ifdef _XM_SSE_INTRINSICS_
		cpustring.push_back("SSE; ");
		cpustring.push_back("SSE 2; ");
#endif // _XM_SSE_INTRINSICS_
#ifdef _XM_SSE3_INTRINSICS_
		cpustring.push_back("SSE 3; ");
#endif // _XM_SSE3_INTRINSICS_
#ifdef _XM_SSE4_INTRINSICS_
		cpustring.push_back("SSE 4.1; ");
#endif // _XM_SSE4_INTRINSICS_
#ifdef _XM_AVX_INTRINSICS_
		cpustring.push_back("AVX; ");
#endif // _XM_AVX_INTRINSICS_
#ifdef _XM_FMA3_INTRINSICS_
		cpustring.push_back("FMA3; ");
#endif // _XM_FMA3_INTRINSICS_
#ifdef _XM_F16C_INTRINSICS_
		cpustring.push_back("F16C; ");
#endif // _XM_F16C_INTRINSICS_
#ifdef _XM_AVX2_INTRINSICS_
		cpustring.push_back("AVX 2; ");
#endif // _XM_AVX2_INTRINSICS_
#ifdef _XM_ARM_NEON_INTRINSICS_
		cpustring.push_back("NEON; ");
#endif // _XM_ARM_NEON_INTRINSICS_

		pf::backlogger::postin(cpustring.c_str());

		if (!XMVerifyCPUSupport())
		{
		//	wilog_messagebox("XMVerifyCPUSupport() failed! This means that your CPU doesn't support a required feature! %s", cpustring.c_str());
		}

		p_log("\nRAM: %s", pf::helper::GetMemorySizeText(pf::helper::GetMemoryUsage().total_physical).c_str());

		size_t shaderdump_count = pf::renderer::GetShaderDumpCount();
		if (shaderdump_count > 0)
		{
			p_log("\nEmbedded shaders found: %d", (int)shaderdump_count);
		}
		else
		{
			p_log("\nNo embedded shaders found, shaders will be compiled at runtime if needed.\n\tShader source path: %s\n\tShader binary path: %s", pf::renderer::GetShaderSourcePath().c_str(), pf::renderer::GetShaderPath().c_str());
		}

		pf::backlogger::postin("");
		pf::jobsystem::Initialize();

		pf::backlogger::postin("");
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { pf::image::Initialize(); systems[INITIALIZED_SYSTEM_IMAGE].store(true); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { pf::input::Initialize(); systems[INITIALIZED_SYSTEM_INPUT].store(true); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { pf::renderer::Initialize(); systems[INITIALIZED_SYSTEM_RENDERER].store(true); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { pf::texturehelper::Initialize(); systems[INITIALIZED_SYSTEM_TEXTUREHELPER].store(true); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { pf::HairParticleSystem::Initialize(); systems[INITIALIZED_SYSTEM_HAIRPARTICLESYSTEM].store(true); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { pf::EmittedParticleSystem::Initialize(); systems[INITIALIZED_SYSTEM_EMITTEDPARTICLESYSTEM].store(true); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { pf::Ocean::Initialize(); systems[INITIALIZED_SYSTEM_OCEAN].store(true); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { pf::gpusortlib::Initialize(); systems[INITIALIZED_SYSTEM_GPUSORTLIB].store(true); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { pf::GPUBVH::Initialize(); systems[INITIALIZED_SYSTEM_GPUBVH].store(true); });
		pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { pf::physics::Initialize(); systems[INITIALIZED_SYSTEM_PHYSICS].store(true); });
		//pf::jobsystem::Execute(ctx, [](pf::jobsystem::JobArgs args) { pf::TrailRenderer::Initialize(); systems[INITIALIZED_SYSTEM_TRAILRENDERER].store(true); });

		// Initialize these immediately:
		//pf::lua::Initialize(); systems[INITIALIZED_SYSTEM_LUA].store(true);
		pf::audio::Initialize(); systems[INITIALIZED_SYSTEM_AUDIO].store(true);
		pf::font::Initialize(); systems[INITIALIZED_SYSTEM_FONT].store(true);

		std::thread([] {
			pf::jobsystem::Wait(ctx);
			p_log("\n[pf::initializer] Wicked Engine Initialized (%d ms)", (int)std::round(timer.elapsed()));
			}).detach();
	}

	// Check if systems have been initialized or not
	//	system : specify to check a specific system, or leave default to check all systems
	bool IsInitializeFinished(INITIALIZED_SYSTEM system) {
		if (system == INITIALIZED_SYSTEM_COUNT)
		{
			return initializationStarted.load() && !pf::jobsystem::IsBusy(ctx);
		}
		else
		{
			return systems[system].load();
		}
	}
	// Wait for all system initializations to finish
	void WaitForInitializationsToFinish() {
		pf::jobsystem::Wait(ctx);
	}
}