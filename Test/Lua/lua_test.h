#pragma once
#include <iostream>
#include <string>

// 引入 Lua 头文件，必须包裹在 extern "C" 中
extern "C" {
#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"
}




int PrintCCCC(lua_State* L);

int test_main();