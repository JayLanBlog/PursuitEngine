#include "plua.h"
#include <memory>
#include <atomic>
#include <iostream>
#include <vector>
#include <algorithm>

#include "Binder/application_bind.h"
#include "Binder/async_bind.h"
#include "Binder/audio_bind.h"
#include "Binder/back_log_bind.h"
#include "Binder/image_params_bind.h"
#include "Binder/input_bind.h"
#include "Binder/loading_screen_bind.h"
#include "Binder/math_bind.h"
#include "Binder/path_query_bind.h"
#include "Binder/primitive_bind.h"
#include "Binder/render_path_2d_bind.h"
#include "Binder/render_path_3d_bind.h"
#include "Binder/render_path_bind.h"
#include "Binder/scene_bind.h"
#include "Binder/sprite_bind.h"
#include "Binder/sprite_font_bind.h"
#include "Binder/texture_bind.h"
#include "Binder/video_bind.h"
#include "Binder/voxel_grid_bind.h"
#include "Binder/sprite_animation_bind.h"
#include "Binder/render_bind.h"
#include "Binder/trail_renderer_bind.h"
#include "Binder/physics_bind.h"
#include "Binder/network_bind.h"

#include <memory>
#include <Core/pf_version.h>

namespace Luaer {
	static constexpr const char* ERROR_PREFIX = "[Lua Error] ";
	struct LuaInternal {
		lua_State* m_luaState = NULL;
		~LuaInternal()
		{
			if (m_luaState != NULL)
			{
				lua_close(m_luaState);
			}
		}
	};
	LuaInternal& lua_internal()
	{
		static LuaInternal luainternal;
		return luainternal;
	}
	pf::Application* editorApplication = nullptr;
	pf::RenderPath* editorRenderPath = nullptr;
	int IsThisEditor(lua_State* L)
	{
		bool ret = editorApplication != nullptr && editorRenderPath != nullptr;
		Luaer::SSetBool(L, ret);
		return 1;
	}
	int ReturnToEditor(lua_State* L) {
		if (editorApplication != nullptr && editorRenderPath != nullptr)
		{
			KillProcesses();
			editorApplication->ActivatePath(editorRenderPath);
		}
		return 0;
	}

	void PostErrorMsg(lua_State* L)
	{
		const char* str = lua_tostring(L, -1);
		if (str == nullptr)
			return;
		std::string ss;
		ss +=ERROR_PREFIX;
		ss += str;
		//Add Log
	    pf::backlogger::postin(ss,LogLevel::Error);
		lua_pop(L, 1); // remove error message
		//Return Editor
		ReturnToEditor(L);
	}

	void PostErrorMsg()
	{
		PostErrorMsg(lua_internal().m_luaState);
	}


	inline void SignalHelper(lua_State* L, const char* str)
	{
		lua_getglobal(L, "signal");
		lua_pushstring(L, str);
		if (lua_pcall(L, 1, LUA_MULTRET, 0) != LUA_OK)
		{
			PostErrorMsg();
		}
	}

	int Internal_CompileBinaryFile(lua_State* L)
	{
		int argc = SGetArgCount(L);

		if (argc > 1)
		{
			std::string filename_src = SGetString(L, 1);
			std::string filename_dst = SGetString(L, 2);
			pf::vector<uint8_t> data;
			if (CompileFile(filename_src, data))
			{
				pf::helper::FileWrite(filename_dst, data.data(), data.size());
				return 0;
			}
			SError(L, "compilebinaryfile(string filename_src, filename_dst): Source file could not be read, or compilation failed!");
		}
		SError(L, "compilebinaryfile(string filename_src, filename_dst): Not enough arguments!");
		return 0;
	}

	int Internal_DoFile(lua_State* L) {
		int argc = SGetArgCount(L);
		if (argc > 0) {
			uint32_t PID;
			std::string filename = SGetString(L, 1);
			if (argc >= 2)
			{
				PID = SGetInt(L, 2);
			}
			else
			{
				PID = GeneratePID();
			}
			std::string customparameters_prepend;
			if (argc >= 3) customparameters_prepend = SGetString(L, 3);
			std::string customparameters_append;
			if (argc >= 4) customparameters_prepend = SGetString(L, 4);
			
			pf::vector<uint8_t> filedata;

			if (pf::helper::FileRead(filename, filedata)) {
				std::string command = std::string(filedata.begin(), filedata.end());

				PID = AttachScriptParameters(command, filename, PID, customparameters_prepend, customparameters_append);
				lua_settop(L, 0);

				int status = luaL_loadstring(L, command.c_str());

				if (status == 0)
				{
					status = lua_pcall(L, 0, LUA_MULTRET, 0);
				}

				if (status == 0)
				{
					int argc = lua_gettop(L);
					SSetString(L, std::to_string(PID)); 
					return ++argc;
				}
				else
				{
					PostErrorMsg(L);
					return 0;
				}
			}
		}
		else
		{
			SError(L, "dofile(string filename) not enough arguments!");
		}
		return 0;
	}

	int Internal_DoBinaryFile(lua_State* L) {
		int argc = SGetArgCount(L);
		if (argc > 0)
		{
			std::string filename = SGetString(L, 1);
			if (RunBinaryFile(filename))
			{
				return 0;
			}

			SError(L, "dobinaryfile(string filename): File could not be read!");
		}
		SError(L, "dobinaryfile(string filename): Not enough arguments!");
		return 0;
	}


	int IsThisDebugBuild(lua_State* L)
	{
#ifdef _DEBUG
		SSetBool(L, true);
#else
		SSetBool(L, false);
#endif
		return 1;
	}


	int GetVersionMajor(lua_State* L)
	{
		SSetInt(L, pf::version::GetMajor());
		return 1;
	}
	int GetVersionMinor(lua_State* L)
	{
		SSetInt(L, pf::version::GetMinor());
		return 1;
	}
	int GetVersionRevision(lua_State* L)
	{
		SSetInt(L, pf::version::GetRevision());
		return 1;
	}
	int GetVersionString(lua_State* L)
	{
		SSetString(L, pf::version::GetVersionString());
		return 1;
	}
	int GetCreditsString(lua_State* L)
	{
		SSetString(L, pf::version::GetCreditsString());
		return 1;
	}
	int GetSupportersString(lua_State* L)
	{
		SSetString(L, pf::version::GetSupportersString());
		return 1;
	}


	void Initialize() {
		if (lua_internal().m_luaState != nullptr)
			return; // already initialized
		lua_internal().m_luaState = luaL_newstate();
		luaL_openlibs(lua_internal().m_luaState);
		RegisterFunc("dofile", Internal_DoFile);
		RegisterFunc("dobinaryfile", Internal_DoBinaryFile);
		RegisterFunc("compilebinaryfile", Internal_CompileBinaryFile);
		RunText(lua_Globals);
		RegisterFunc("IsThisDebugBuild", IsThisDebugBuild);
		RegisterFunc("IsThisEditor", IsThisEditor);
		RegisterFunc("ReturnToEditor", ReturnToEditor);
		RegisterFunc("IsThisDebugBuild", IsThisDebugBuild);
	
		RegisterFunc("GetVersionMajor", GetVersionMajor);
		RegisterFunc("GetVersionMinor", GetVersionMinor);
		RegisterFunc("GetVersionRevision", GetVersionRevision);
		RegisterFunc("GetVersionString", GetVersionString);
		RegisterFunc("GetCreditsString", GetCreditsString);
		RegisterFunc("GetSupportersString", GetSupportersString);
		Vector_BindLua::Bind();
		Matrix_BindLua::Bind();
		Application_BindLua::Bind();
		Canvas_BindLua::Bind();
		RenderPath_BindLua::Bind();
		RenderPath2D_BindLua::Bind();
		LoadingScreen_BindLua::Bind();
		RenderPath3D_BindLua::Bind();
		Texture_BindLua::Bind();
		renderer::Bind();
		Audio_BindLua::Bind();
		Video_BindLua::Bind();
		VideoInstance_BindLua::Bind();
		Sprite_BindLua::Bind();
		ImageParams_BindLua::Bind();
		SpriteAnim_BindLua::Bind();
		scene::Bind();
		Input_BindLua::Bind();
		SpriteFont_BindLua::Bind();
		Luaer::BackLog::Bind();
		Network_BindLua::Bind();
		primitive::Bind();
		Physics_BindLua::Bind();
		VoxelGrid_BindLua::Bind();
		PathQuery_BindLua::Bind();
		TrailRenderer_BindLua::Bind();
		Async_BindLua::Bind();

	}


	bool RunScript()
	{
		if (lua_pcall(lua_internal().m_luaState, 0, LUA_MULTRET, 0) != LUA_OK)
		{
			PostErrorMsg();
			return false;
		}
		return true;
	}

	lua_State* GetLuaState() {
		return lua_internal().m_luaState;
	}

	//run a script from file
	bool RunFile(const char* filename) {
		pf::vector<uint8_t> filedata;
		if (pf::helper::FileRead(filename, filedata))
		{
			std::string script = std::string(filedata.begin(), filedata.end());
			AttachScriptParameters(script, filename);
			return RunText(script);
		}
		return false;
	}

	//run a binary script from file
	bool RunBinaryFile(const char* filename) {
		pf::vector<uint8_t> filedata;
		if (pf::helper::FileRead(filename, filedata))
		{
			return RunBinaryData(filedata.data(), filedata.size(), filename);
		}
		return false;
	}

	//run a script from param
	bool RunText(const char* script) {
		if (luaL_loadstring(lua_internal().m_luaState, script) == LUA_OK)
		{
			return RunScript();
		}
		PostErrorMsg();
		return false;
	}

	//run binary script
	bool RunBinaryData(const void* data, size_t size, const char* debugname) {
		if (luaL_loadbuffer(lua_internal().m_luaState, (const char*)data, size, debugname) == LUA_OK)
		{
			return RunScript();
		}
		PostErrorMsg();
		return false;
	}

	//register function to use in scripts
	void RegisterFunc(const char* name, lua_CFunction function) {
		lua_register(lua_internal().m_luaState, name, function);
	}

	//set delta time to use with lua
	void SetDeltaTime(double dt) {
		lua_getglobal(lua_internal().m_luaState, "setDeltaTime");
		SSetDouble(lua_internal().m_luaState, dt);
		if (lua_pcall(lua_internal().m_luaState, 1, LUA_MULTRET, 0) != LUA_OK)
		{
			PostErrorMsg();
		}
	}

	//update lua scripts which are waiting for a fixed game tick
	void FixedUpdate() {
		SignalHelper(lua_internal().m_luaState, "wickedengine_fixed_update_tick");
	}

	//update lua scripts which are waiting for a game tick
	void Update() {
		SignalHelper(lua_internal().m_luaState, "wickedengine_update_tick");
	}
	//issue lua drawing commands which are waiting for a render tick
	void Render() {
		SignalHelper(lua_internal().m_luaState, "wickedengine_render_tick");
	}

	//send a signal to lua
	void Signal(const char* name) {
		SignalHelper(lua_internal().m_luaState, name);
	}

	//kill every running background task (coroutine)
	void KillProcesses() {
		RunText("killProcesses();");
	}

	// Generates a unique identifier for a script instance:
	uint32_t GeneratePID() {
		static std::atomic<uint32_t> scriptpid_next{ 0 + 1 };
		return scriptpid_next.fetch_add(1);
	}

	// Adds some local management functions to the script
	//	returns the PID
	uint32_t AttachScriptParameters(std::string& script, const std::string& filename , uint32_t PID, const std::string& customparameters_prepend , const std::string& customparameters_append) {
		static const std::string persistent_inject =
			"local runProcess = function(func) "
			"	success, co = Internal_runProcess(script_file(), script_pid(), func);"
			"	return success, co;"
			"end;"
			"if _ENV.PROCESSES_DATA[script_pid()] == nil then"
			"	_ENV.PROCESSES_DATA[script_pid()] = { _INITIALIZED = -1 };"
			"end;"
			"if _ENV.PROCESSES_DATA[script_pid()]._INITIALIZED < 1 then"
			"	_ENV.PROCESSES_DATA[script_pid()]._INITIALIZED = _ENV.PROCESSES_DATA[script_pid()]._INITIALIZED + 1;"
			"end;";
		// Make sure the file path doesn't contain backslash characters, replace them with forward slash.
		//	- backslash would be recognized by lua as escape character
		//	- the path string could be coming from unknown location (content, programmer, filepicker), so always do this
		std::string filepath = filename;
		std::replace(filepath.begin(), filepath.end(), '\\', '/');
		std::string dynamic_inject = "--[[" + filepath + "--]]";
		dynamic_inject += "local function script_file() return \"" + filepath + "\" end;";
		dynamic_inject += "local function script_pid() return \"" + std::to_string(PID) + "\" end;";
		dynamic_inject += "local function script_dir() return \"" + pf::helper::GetDirectoryFromPath(filepath) + "\" end;";
		dynamic_inject += persistent_inject;
		script = dynamic_inject + customparameters_prepend + script + customparameters_append;
		return PID;
	}

	//get string from lua on stack position
	const char* SGetString(lua_State* L, int stackpos) {
		const char* str = lua_tostring(L, stackpos);
		if (str != nullptr)
			return str;
		return "";
	}
	//check if a value is a string on the stack position
	bool SIsString(lua_State* L, int stackpos) {
		return lua_isstring(L, stackpos) != 0;
	}
	//check if a value is a number on the stack position
	bool SIsNumber(lua_State* L, int stackpos) {
		return lua_isnumber(L, stackpos) != 0;
	}
	//check if a value is a bool on the stack position
	bool SIsBool(lua_State* L, int stackpos) {
		return lua_isboolean(L, stackpos);
	}
	//check if a value is a c function on the stack position
	bool SIsCFunction(lua_State* L, int stackpos) {
		return lua_iscfunction(L, stackpos);
	}
	//check if a value is a userdata on the stack position
	bool SIsUserdata(lua_State* L, int stackpos) {
		return lua_isuserdata(L, stackpos);
	}
	//check if a value is nil on the stack position
	bool SIsNil(lua_State* L, int stackpos) {
		return lua_isnil(L, stackpos);
	}
	//get lua.h type int on the stack position
	int SGetType(lua_State* L, int stackpos) {
		return lua_type(L, stackpos);
	}
	//get int from lua on stack position
	int SGetInt(lua_State* L, int stackpos) {
		return static_cast<int>(SGetLongLong(L, stackpos));
	}
	//get long from lua on stack position
	long SGetLong(lua_State* L, int stackpos) {
		return static_cast<long>(SGetLongLong(L, stackpos));
	}
	//get long long from lua on stack position
	long long SGetLongLong(lua_State* L, int stackpos) {
		return lua_tointeger(L, stackpos);
	}
	//get float from lua on stack position
	float SGetFloat(lua_State* L, int stackpos) {
		return static_cast<float>(SGetDouble(L, stackpos));
	}

	//get double from lua on stack position
	double SGetDouble(lua_State* L, int stackpos) {
		return lua_tonumber(L, stackpos);
	}
	//get bool from lua on stack position
	bool SGetBool(lua_State* L, int stackpos) {
		return lua_toboolean(L, stackpos) != 0;
	}
	//get number of elements in the stack, or index of the top element
	int SGetArgCount(lua_State* L) {
		return lua_gettop(L);
	}
	//get class context information
	void* SGetUserData(lua_State* L) {
		return lua_touserdata(L, 1);
	}

	//push int to lua stack
	void SSetInt(lua_State* L, int data) {
		lua_pushinteger(L, (lua_Integer)data);
	}
	//push long to lua stack
	void SSetLong(lua_State* L, long data) {
		lua_pushinteger(L, (lua_Integer)data);
	}
	//push long long to lua stack
	void SSetLongLong(lua_State* L, long long data) {
		lua_pushinteger(L, (lua_Integer)data);
	}
	//push float to lua stack
	void SSetFloat(lua_State* L, float data) {
		lua_pushnumber(L, (lua_Number)data);
	}

	//push double to lua stack
	void SSetDouble(lua_State* L, double data) {
		lua_pushnumber(L, (lua_Number)data);
	}
	//push string to lua stack
	void SSetString(lua_State* L, const char* data) {
		lua_pushstring(L, data);
	}

	//push bool to lua stack
	void SSetBool(lua_State* L, bool data) {
		lua_pushboolean(L, static_cast<int>(data));
	}
	//push pointer (light userdata) to lua stack
	void SSetPointer(lua_State* L, void* data) {
		lua_pushlightuserdata(L, data);
	}
	//push null to lua stack
	void SSetNull(lua_State* L) {
		lua_pushnil(L);
	}

	int IntProperty::Get(lua_State* L) {
		SSetInt(L, *data);
		return 1;
	}
	int IntProperty::Set(lua_State* L) {
		*data = SGetInt(L, 1);
		return 0;
	}

	int LongProperty::Get(lua_State* L) {
		SSetLong(L, *data);
		return 1;
	}

	int LongProperty::Set(lua_State* L) {
		*data = SGetLong(L, 1);
		return 0;
	}

	int LongLongProperty::Get(lua_State* L) {
		SSetLongLong(L, *data);
		return 1;
	}
	int LongLongProperty::Set(lua_State* L) {
		*data = SGetLongLong(L, 1);
		return 0;
	}

	int FloatProperty::Get(lua_State* L) {
		SSetFloat(L, *data);
		return 1;
	}

	int FloatProperty::Set(lua_State* L) {
		*data = SGetFloat(L, 1);
		return 0;
	}

	int DoubleProperty::Get(lua_State* L) {
		SSetDouble(L, *data);
		return 1;
	}
	int DoubleProperty::Set(lua_State* L) {
		*data = SGetDouble(L, 1);
		return 0;
	}

	int StringProperty::Get(lua_State* L) {
		SSetString(L, *data);
		return 1;
	}

	int StringProperty::Set(lua_State* L) {
		*data = SGetString(L, 1);
		return 0;
	}

	int BoolProperty::Get(lua_State* L) {
		SSetBool(L, *data);
		return 1;
	}

	int BoolProperty::Set(lua_State* L) {
		*data = SGetBool(L, 1);
		return 0;
	}


	int writer(lua_State* L, const void* p, size_t sz, void* ud)
	{
		pf::vector<uint8_t>& dst = *(pf::vector<uint8_t>*)ud;
		for (size_t i = 0; i < sz; ++i)
		{
			dst.push_back(((uint8_t*)p)[i]);
		}
		return LUA_OK;
	}

	bool CompileText(const char* script, pf::vector<uint8_t>& dst)
	{
		if (luaL_loadstring(lua_internal().m_luaState, script) != LUA_OK)
		{
			PostErrorMsg();
			return false;
		}
		dst.clear();
		if (lua_dump(lua_internal().m_luaState, writer, &dst, 0) != LUA_OK)
		{
			PostErrorMsg();
			lua_pop(lua_internal().m_luaState, 1); // lua_dump does not pop the dumped function from stack
			return false;
		}
		lua_pop(lua_internal().m_luaState, 1); // lua_dump does not pop the dumped function from stack
		return true;
	}

	XMFLOAT4 SGetFloat4(lua_State* L, int stackpos)
	{
		return XMFLOAT4(SGetFloat(L, stackpos), SGetFloat(L, stackpos + 1), SGetFloat(L, stackpos + 2), SGetFloat(L, stackpos + 3));
	}

	bool CompileFile(const char* filename, pf::vector<uint8_t>& dst) {
		pf::vector<uint8_t> filedata;
		if (pf::helper::FileRead(filename, filedata))
		{
			std::string script = std::string(filedata.begin(), filedata.end());
			return CompileText(script.c_str(), dst);
		}
		return false;
	}


	void SError(lua_State* L, const std::string& error ) {
		//retrieve line number for error info
		lua_Debug ar;
		lua_getstack(L, 1, &ar);
		lua_getinfo(L, "nSl", &ar);
		int line = ar.currentline;
		std::string ss;
		ss += ERROR_PREFIX;
		ss += "Line " + std::to_string(line) + ": ";
		if (!error.empty())
		{
			ss += error;
		}
		pf::backlogger::postin(ss, LogLevel::Error);
		ReturnToEditor(L);
	}

}

