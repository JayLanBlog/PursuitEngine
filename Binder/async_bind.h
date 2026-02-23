#pragma once
#include "Module/Luaer/plua.h"
#include "Module/Luaer/pluna.h"
#include "Module/Util/job_system.h"
namespace Luaer {
	class Async_BindLua
	{
	public:
		pf::jobsystem::context ctx;
		inline static constexpr char className[] = "Async";
		static Luna<Async_BindLua>::FunctionType methods[];
		static Luna<Async_BindLua>::PropertyType properties[];

		Async_BindLua() = default;
		Async_BindLua(lua_State* L) {}

		int Wait(lua_State* L);
		int IsCompleted(lua_State* L);

		static void Bind();
	};
}