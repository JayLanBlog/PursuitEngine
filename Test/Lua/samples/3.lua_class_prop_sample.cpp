#include "3.lua_class_prop_sample.h"
#include "Module/Logger/backlogger.h"
#include "Module/Luaer/plua.h"
#include "Module/Luaer/pluna.h"

class Test {
public :
	Test() = default;
	int a = 0;
	int b = 0;
	//Param p;
	Test(int a1, int b2):a(a1),b(b2) {}
};


class TestLuaBind {
public:
	Test test;
	inline static constexpr char className[] = "Test";
	static Luna<TestLuaBind>::FunctionType methods[];
	static Luna<TestLuaBind>::PropertyType properties[];

	TestLuaBind(const Test& t) : test(t) {

	}

	TestLuaBind(lua_State* L) {
		int a = 0 , b =0;
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			a = Luaer::SGetInt(L, 1);
			if (argc > 1)
			{
				b = Luaer::SGetInt(L, 2);
			}
		}
		//sprite = wi::Sprite(name, mask);
		test = Test(a,b);
	}


	static void Bind() {
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<TestLuaBind>::Register(Luaer::GetLuaState());
		}
	}

	int Geta(lua_State* L)
	{
		Luaer::SSetFloat(L, test.a);
		return 1;
	}

	int Seta(lua_State* L)
	{

		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			test.a = Luaer::SGetFloat(L, 1);
		}
		else
		{
			Luaer::SError(L, "SetRotation(float x) not enough arguments!");
		}
		return 0;
	}


	int Getb(lua_State* L)
	{
		Luaer::SSetFloat(L, test.b);
		return 1;
	}

	int Setb(lua_State* L)
	{

		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			test.b = Luaer::SGetFloat(L, 1);
		}
		else
		{
			Luaer::SError(L, "SetRotation(float x) not enough arguments!");
		}
		return 0;
	}
};

Luna<TestLuaBind>::FunctionType TestLuaBind::methods[] = {
	{ NULL, NULL }
};

Luna<TestLuaBind>::PropertyType TestLuaBind::properties[] = {
	lunaproperty(TestLuaBind, a),
	lunaproperty(TestLuaBind, b),
	{ NULL, NULL }
};

int test_lua_class_prop_sample() {
	Luaer::Initialize();
	std::string script = R"(
        print("Hello from Lua embedded in C++!")
        local test = Test(1,2);
		print(test.a)
		print(test.b)
        a = 10
        b = 20
        function add(x, y)
            return x + y
        end
        print("Hello from Lua embedded in C++2!")
    )";
	TestLuaBind::Bind();
	Luaer::RunText(script);
	//lua_close(Luaer::GetLuaState());
	return 0;
}