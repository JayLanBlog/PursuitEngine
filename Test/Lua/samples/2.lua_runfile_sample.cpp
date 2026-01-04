#include "2.lua_runfile_sample.h"
#include "Module/Logger/backlogger.h"
#include "Module/Luaer/plua.h"

const std::string filePath = "E:/WorkLearn/FrameWork/PursuitEngine/Test/Lua/samples/";

int test_lua_runfile_sample() {
	Luaer::Initialize();
	Luaer::RunFile(filePath+"scripts/1.test_add.lua");
	std::string path = std::filesystem::current_path().string(); 
	Logger("%s", path.c_str());
	return 0;
}