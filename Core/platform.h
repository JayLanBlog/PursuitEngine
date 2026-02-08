#pragma once

#ifdef _WIN32

#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <SDKDDKVer.h>
#include <windows.h>


#if WINAPI_FAMILY == WINAPI_FAMILY_GAMES
#define PLATFORM_XBOX
#else
#define PLATFORM_WINDOWS_DESKTOP
#endif // WINAPI_FAMILY_GAMES
#define wiLoadLibrary(name) LoadLibraryA(name)
#define wiGetProcAddress(handle,name) GetProcAddress(handle, name)
#elif defined(__SCE__)
#define PLATFORM_PS5
#else
#define PLATFORM_LINUX
#include <dlfcn.h>
#define wiLoadLibrary(name) dlopen(name, RTLD_LAZY)
#define wiGetProcAddress(handle,name) dlsym(handle, name)
typedef void* HMODULE;
#endif // _WIN32
#define ENABLESDL3

#ifdef ENABLESDL3
#include <SDL3/SDL.h>
#include "SDL3/SDL_vulkan.h"
#endif

#define WICKEDENGINE_BUILD_VULKAN

namespace platform
{
#ifdef _WIN32 

#ifdef ENABLESDL3
	using window_type = SDL_Window*;
	using error_type = int;

#else
	using window_type = HWND;
	using error_type = HRESULT;

#endif // ENABLESDL3

#elif defined(SDL2)
	using window_type = SDL_Window*;
	using error_type = int;
#else
	using window_type = void*;
	using error_type = int;
#endif // _WIN32

	inline void Exit()
	{
#ifdef _WIN32
#ifdef ENABLESDL3
		SDL_Event quit_event;
		quit_event.type = SDL_EVENT_QUIT;
		SDL_PushEvent(&quit_event);
#else
		PostQuitMessage(0);
#endif
#endif // _WIN32
#ifdef SDL2
		SDL_Event quit_event;
		quit_event.type = SDL_QUIT;
		SDL_PushEvent(&quit_event);
#endif
	}

	struct WindowProperties
	{
		int width = 0;
		int height = 0;
		float dpi = 96;
	};
	inline void GetWindowProperties(window_type window, WindowProperties* dest)
	{
#ifdef PLATFORM_WINDOWS_DESKTOP

#ifdef ENABLESDL3
		float scale = SDL_GetWindowDisplayScale(window);
		dest->dpi = scale * 96.0f;

	/*	int window_width, window_height;
		SDL_GetWindowSize(window, &dest->width, &dest->height);
		SDL_GetWindowSizeInPixels(window, &window_width, &window_height);
		dest->dpi = ((float)dest->width / (float)window_width) * 96.f;*/

#else
		dest->dpi = (float)GetDpiForWindow(window);
#endif
#endif // WINDOWS_DESKTOP

#ifdef PLATFORM_XBOX
		dest->dpi = 96.f;
#endif // PLATFORM_XBOX

#if defined(PLATFORM_WINDOWS_DESKTOP) || defined(PLATFORM_XBOX)
#ifdef ENABLESDL3
		int w, h;
		SDL_GetWindowSize(window, &w, &h);
		dest->width = w;
		dest->height = h;
#else
		RECT rect;
		GetClientRect(window, &rect);
		dest->width = int(rect.right - rect.left);
		dest->height = int(rect.bottom - rect.top);
#endif
#endif // PLATFORM_WINDOWS_DESKTOP || PLATFORM_XBOX

#ifdef PLATFORM_LINUX
		int window_width, window_height;
		SDL_GetWindowSize(window, &window_width, &window_height);
		SDL_Vulkan_GetDrawableSize(window, &dest->width, &dest->height);
		dest->dpi = ((float)dest->width / (float)window_width) * 96.f;
#endif // PLATFORM_LINUX
	}
}