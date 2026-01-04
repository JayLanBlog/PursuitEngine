#include "lua_test.h"
int PrintCCCC(lua_State* L) {

    std::cout << "Lua PrintCCCC" << std::endl;
    return 1;
}

int test_main() {
    // 1. 创建 Lua 虚拟机 (State)
    lua_State* L = luaL_newstate();

    // 2. 加载标准库 (print, math 等)
    luaL_openlibs(L);
    lua_register(L, "PrintCC", PrintCCCC);
    // 3. 执行一段 Lua 脚本
    std::string script = R"(
        print("Hello from Lua embedded in C++!")
        PrintCC();
        a = 10
        b = 20
        function add(x, y)
            return x + y
        end
    )";

    // 执行字符串中的 Lua 代码
    if (luaL_dostring(L, script.c_str()) != LUA_OK) {
        std::cerr << "Error: " << lua_tostring(L, -1) << std::endl;
        lua_pop(L, 1); // 弹出错误信息
    }

    // 4. 从 C++ 获取 Lua 变量
    lua_getglobal(L, "a");
    if (lua_isinteger(L, -1)) {
        std::cout << "Lua variable a = " << lua_tointeger(L, -1) << std::endl;
    }
    lua_pop(L, 1); // 弹出栈顶

    // 5. 调用 Lua 函数
    lua_getglobal(L, "add"); // 压入函数
    lua_pushinteger(L, 100); // 压入参数 1
    lua_pushinteger(L, 50);  // 压入参数 2

    // 调用函数 (2个参数，1个返回值)
    if (lua_pcall(L, 2, 1, 0) != LUA_OK) {
        std::cerr << "Call Error: " << lua_tostring(L, -1) << std::endl;
    }
    else {
        // 获取返回值
        int result = (int)lua_tointeger(L, -1);
        std::cout << "Lua function add(100, 50) result = " << result << std::endl;
        lua_pop(L, 1); // 弹出返回值
    }

    // 6. 关闭虚拟机
    lua_close(L);

    return 0;
}