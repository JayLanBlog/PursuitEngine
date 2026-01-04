#include "1.lua_setparam2c_sample.h"

#include "Module/Logger/backlogger.h"
#include "Module/Luaer/plua.h"

int testSetParam2CSample() {
    Luaer::Initialize();
    std::string script = R"(
        print("Hello from Lua embedded in C++!")
        
        a = 10
        b = 20
        function add(x, y)
            return x + y
        end

        print("Hello from Lua embedded in C++2!")
    )";
    Luaer::RunText(script);
    return 0;
}