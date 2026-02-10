#include "math_binder_test.h"
#include "Binder/math_bind.h"
using namespace Luaer;

int test_math_binder() {

	Luaer::Initialize();
	Vector_BindLua::Bind();
	Matrix_BindLua::Bind();
	std::string script = R"(
        print("Hello from Lua embedded in C++!")
		 local test = Vector(1111.0);
		print(test.X)
		print(test.Y)
		print(test.Z)
		print(test.W)
        print("Hello from Lua embedded in C++2!")
    )";

	Luaer::RunText(script);
	// 6. ¹Ø±ÕÐéÄâ»ú
	//lua_close(Luaer::GetLuaState());
	return 0;

	return 0;
}