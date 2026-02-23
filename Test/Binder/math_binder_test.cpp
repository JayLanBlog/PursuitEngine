#include "math_binder_test.h"
#include "Binder/math_bind.h"
using namespace Luaer;

int test_math_binder() {

	Luaer::Initialize();
	//Vector_BindLua::Bind();
	//Matrix_BindLua::Bind();
	std::string script = R"(
        print("Hello from Lua embedded in C++!")
		local test = Vector(2.0,2.0,2.0,2.0)
		
		local S = matrix.Scale(Vector(1,2,3))
		local R = matrix.Rotation(Vector(0.2, 0.6))
		local T = matrix.Translation(Vector(0,2,3))
		local M = S:Multiply(R):Multiply(T)
		local scalingMat = matrix.Scale(Vector(1.32,1,1))		

		print(S.GetRow(5).X)
		print(S.GetRow(5).Y)
		print(S.GetRow(5).Z)
		print(S.GetRow(5).W)
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