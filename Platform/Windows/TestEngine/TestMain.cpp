#include "Lua/lua_test.h"
#include "Opengl/opengl_test.h"
//#include "Vulkan/vk_test.h"
//#include "Engine/Device/vulkan_driver.h"
#include "Volk/volk_test.h"
#include "Volk/Samples/test_vulkan_engine.h"
#include "Shader/shader_compare_test.h"
#include "Util/util_test_include.h"
#include "Event/test_eventmanager.h"
#include "Component/test_component_vk.h"
#include "Run/app_run.h"
#include "Run/samples/sprite_app.h"
#include "Engine/Ecore/scene_component.h"

int main(int argc, char* argv[]) {
	// Test Lua
	//test_main();
	//test_opengl_main();
	//test_vkmain();
	//testSetParam2CSample();
	//test_lua_runfile_sample();
	//test_lua_class_prop_sample();
	//test_lua_class_param_sample();
	//test_timer();
	//test_jobsystem();
	//test_volkmain();
	//XMFLOAT4X4 ft;

	//const XMFLOAT4* data_full =nullptr;
	//XMHALF4* data_packed = nullptr;
    //test_vulkan_egine();
	//test_shader_compare_test(argc,argv);

	//test_eventmanager();
	
	//test_component();

	//pf::arun::TLauncher luancher;
	//luancher.run();
	
	pf::arun::SpriteLauncher launcher;
	launcher.run();
	return 0;
}