#include "4.lua_class_param_sample.h"
#include "Module/Logger/backlogger.h"
#include "Module/Luaer/plua.h"
#include "Module/Luaer/pluna.h"

class Test {
public:
	Test() = default;
	int a = 0;
	int b = 0;
	//Param p;
	Test(int a1, int b2) :a(a1), b(b2) {}
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
		int a = 0, b = 0;
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
		test = Test(a, b);
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


class PTest {
public:
	PTest() = default;
	Test tParam;

	PTest(int a, int b) {
		tParam = Test(a,b);
	}
};

class PTestLuaBind {
public:
	PTest pParam;
	inline static constexpr char className[] = "PTest";
	static Luna<PTestLuaBind>::FunctionType methods[];
	static Luna<PTestLuaBind>::PropertyType properties[];

	int GetpParam(lua_State* L)
	{
		Logger("GetpParam");
		//Luaer::SSetFloat(L, test.b);
		Luna<TestLuaBind>::push(L, pParam.tParam);
		return 1;
	}

	PTestLuaBind(lua_State* L) {
		int a = 0, b = 0;
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			a = Luaer::SGetInt(L, 1);
			if (argc > 1)
			{
				b = Luaer::SGetInt(L, 2);
			}
		}
		pParam = PTest(a,b);
	}

	int SetpParam(lua_State* L)
	{
		Logger("SetpParam");
		int argc = Luaer::SGetArgCount(L);
		if (argc > 0)
		{
			TestLuaBind* params = Luna<TestLuaBind>::check(L, 1);
			if (params != nullptr)
			{
				pParam.tParam = params->test;
			}
		}
		else
		{
			Luaer::SError(L, "SetRotation(float x) not enough arguments!");
		}
		return 0;
	}

	static void Bind() {
		Logger("Bind");
		static bool initialized = false;
		if (!initialized)
		{
			initialized = true;
			Luna<PTestLuaBind>::Register(Luaer::GetLuaState());
		}
	}
};

Luna<PTestLuaBind>::FunctionType PTestLuaBind::methods[] = {
	{ NULL, NULL }
};

Luna<PTestLuaBind>::PropertyType PTestLuaBind::properties[] = {
	lunaproperty(PTestLuaBind, pParam),
	{ NULL, NULL }
};

int test_lua_class_param_sample() {
	Logger("test_lua_class_param_sample");
	Luaer::Initialize();
	std::string script = R"(
        print("Hello from Lua embedded in C++!")
        local test = Test(1,2);
		print(test.a)
		print(test.b)
		local pt = PTest(3,4);
		print(pt.pParam.a);
		print(pt.pParam.b);
		pt.pParam = Test(4,2);
	
        a = 10
        b = 20
        function add(x, y)
            return x + y
        end
        print("Hello from Lua embedded in C++2!")
    )";
	TestLuaBind::Bind();
	PTestLuaBind::Bind();
	Luaer::RunText(script);
	// 6. ¹Ø±ÕÐéÄâ»ú
	//lua_close(Luaer::GetLuaState());
	return 0;
}