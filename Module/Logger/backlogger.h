#pragma once
#include "Core/core_include.h"
#include "Module/Filer/file_helper.h"
#include <functional>
#include <filesystem>
#include <string>
#include <cassert>

#include "Engine/Device/graphic.h"
#include "Engine/Ecore/canvas.h"
using namespace pf::graphics;


#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : \
                      (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__))


#define p_log_level(str,level,...) {char text[1024]; snprintf(text, sizeof(text),  " %s - %d :  " str, __FILENAME__, __LINE__,## __VA_ARGS__); pf::backlogger::postin(text, level);}
//#define log_messagebox(str,...) {char text[1024]; snprintf(text, sizeof(text), str, ## __VA_ARGS__); pf::backlogger::postin(text, pf::backlogger::LogLevel::Error); pf::helper::messageBox(text, "Error!");}
#define p_log_warning(str,...) {p_log_level(str,LogLevel::Warning, ## __VA_ARGS__);}
#define p_log_error(str,...) {p_log_level(str, LogLevel::Error, ## __VA_ARGS__);}
#define p_log(str,...) {p_log_level(str, LogLevel::Default, ## __VA_ARGS__);}
#define p_log_assert(cond,str,...) {if(!(cond)){p_log_error(str, ## __VA_ARGS__); assert(cond);}}
#define Logger(str,...) {\
	p_log_level(str, LogLevel::Default, ## __VA_ARGS__);\
}
enum class LogLevel
{
	None,
	Default,
	Warning,
	Error,
};

namespace pf {
	namespace backlogger {
		// Do not modify the order, as this is exposed to LUA scripts as int!
		void Toggle();
		
		void Scroll(int direction);

		std::string getText();

		void postin(const char* input, LogLevel level = LogLevel::Default);
		
		void postin(const std::string& input, LogLevel level = LogLevel::Default);

		LogLevel GetUnseenLogLevelMax();

		void historyPre();

		void historyNext();
		
		bool isActive();

		void Lock();

		void Unlock();

		void BlockLuaExecution();
		
		void UnblockLuaExecution();

		void SaveLogToFile(const std::string& path);

		void setFontRowspacing(float value);

		void setFontSize(int value);

		void SetLogLevel(LogLevel newLevel);

		void historyPrev();
			
		void clear();

		void Update(const pf::Canvas& canvas, float dt = 1.0f / 60.0f);

		void Draw(
			const pf::Canvas& canvas,
			pf::graphics::CommandList cmd,
			pf::graphics::ColorSpace colorspace = pf::graphics::ColorSpace::SRGB
		);

		void DrawOutputText(
			const pf::Canvas& canvas,
			CommandList cmd,
			ColorSpace colorspace
		);

		struct LogEntry
		{
			std::string text;
			LogLevel level = LogLevel::Default;
		};

		// Use getText() instead, unless absolutely necessary.
		void _forEachLogEntry_unsafe(std::function<void(const LogEntry&)> cb);
	}
}