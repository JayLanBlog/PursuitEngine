#pragma once
/*
	get class and filed name
*/
#define varName(x) #x
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <map>
#include <vector>
enum GUIAPI
{
	OPENGL = 0,
	VULKAN = 1
};


#define GRAPHT_API __attribute__((visibility("default")))