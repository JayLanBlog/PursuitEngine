#include "back_log_bind.h"
#include "Module/Logger/backlogger.h"
#include "Module/Luaer/plua.h"
#include "math_bind.h"

#include <string>

namespace Luaer::BackLog {
	
	int backlog_clear(lua_State* L)
	{
		pf::backlogger::clear();
		return 0;
	}

	int backlog_post(lua_State* L) {
		int argc = Luaer::SGetArgCount(L);

		std::string ss;

		for (int i = 1; i <= argc; i++)
		{
			if ( Luaer::SIsString(L, i))
			{
				ss +=  Luaer::SGetString(L, i);
			}
			else if ( Luaer::SIsNumber(L, i))
			{
				double arg =  Luaer::SGetDouble(L, i);
				ss += std::to_string(arg);
			}
			else if ( Luaer::SIsBool(L, i)) {
				bool arg =  Luaer::SGetBool(L, i);
				ss += arg ? "true" : "false";
			}
			else if ( Luaer::SIsNil(L, i)) {
				ss += "nil";
			}
			else
			{
				Vector_BindLua* vec = Luna<Vector_BindLua>::lightcheck(L, i);
				if (vec != nullptr)
				{
					ss += "Vector(" + std::to_string(vec->data.x) + ", " + std::to_string(vec->data.y) + ", " + std::to_string(vec->data.z) + ", " + std::to_string(vec->data.w) + ")";
				}
				else
				{
					Matrix_BindLua* mat = Luna<Matrix_BindLua>::lightcheck(L, i);
					if (mat != nullptr)
					{
						ss += "Matrix(\n";
						ss += "\t" + std::to_string(mat->data._11) + ", " + std::to_string(mat->data._12) + ", " + std::to_string(mat->data._13) + ", " + std::to_string(mat->data._14) + "\n";
						ss += "\t" + std::to_string(mat->data._21) + ", " + std::to_string(mat->data._22) + ", " + std::to_string(mat->data._23) + ", " + std::to_string(mat->data._24) + "\n";
						ss += "\t" + std::to_string(mat->data._31) + ", " + std::to_string(mat->data._32) + ", " + std::to_string(mat->data._33) + ", " + std::to_string(mat->data._34) + "\n";
						ss += "\t" + std::to_string(mat->data._41) + ", " + std::to_string(mat->data._42) + ", " + std::to_string(mat->data._43) + ", " + std::to_string(mat->data._44) + "\n";
						ss += ")";
					}
				}
			}
		}
		if (!ss.empty())
		{
			pf::backlogger::postin(ss);
		}
		return 0;
	}
	int backlog_fontsize(lua_State* L)
	{
		int argc =  Luaer::SGetArgCount(L);

		if (argc > 0)
		{
			pf::backlogger::setFontSize( Luaer::SGetInt(L, 1));
		}
		else
			 Luaer::SError(L, "backlog_fontsize(int val) not enough arguments!");

		return 0;
	}
	int backlog_isactive(lua_State* L)
	{
		 Luaer::SSetBool(L, pf::backlogger::isActive());
		return 1;
	}
	int backlog_fontrowspacing(lua_State* L)
	{
		int argc =  Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			pf::backlogger::setFontRowspacing( Luaer::SGetFloat(L, 1));
		}
		else
			 Luaer::SError(L, "backlog_fontrowspacing(int val) not enough arguments!");
		return 0;
	}
	int backlog_setlevel(lua_State* L)
	{
		int argc =  Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			pf::backlogger::SetLogLevel((LogLevel) Luaer::SGetInt(L, 1));
		}
		else
			 Luaer::SError(L, "backlog_setlevel(int val) not enough arguments!");
		return 0;
	}
	int backlog_lock(lua_State* L)
	{
		pf::backlogger::Lock();
		return 0;
	}
	int backlog_unlock(lua_State* L)
	{
		pf::backlogger::Unlock();
		return 0;
	}
	int backlog_blocklua(lua_State* L)
	{
		pf::backlogger::BlockLuaExecution();
		return 0;
	}
	int backlog_unblocklua(lua_State* L)
	{
		pf::backlogger::UnblockLuaExecution();
		return 0;
	}


	void Bind() {
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luaer::RegisterFunc("backlog_clear", backlog_clear);
			Luaer::RegisterFunc("backlog_post", backlog_post);
			Luaer::RegisterFunc("backlog_fontsize", backlog_fontsize);
			Luaer::RegisterFunc("backlog_isactive", backlog_isactive);
			Luaer::RegisterFunc("backlog_fontrowspacing", backlog_fontrowspacing);
			Luaer::RegisterFunc("backlog_setlevel", backlog_setlevel);
			Luaer::RegisterFunc("backlog_lock", backlog_lock);
			Luaer::RegisterFunc("backlog_unlock", backlog_unlock);
			Luaer::RegisterFunc("backlog_blocklua", backlog_blocklua);
			Luaer::RegisterFunc("backlog_unblocklua", backlog_unblocklua);
		}
	}
}