#include "arguments.h"
#include "Module/Filer/file_helper.h"
#include "Module/Container/unorder_set.h"

#include <sstream>
#include <iterator>
// This can be used to parse and retrieve startup/command arguments of the application
namespace pf::arguments
{
	unordered_set<std::string> params;
	
	void Parse(const wchar_t* args) {
		std::wstring from = args;
		std::string to;
		pf::helper::StringConvert(from, to);

		std::istringstream iss(to);

		params =
		{
			std::istream_iterator<std::string>{iss},
			std::istream_iterator<std::string>{}
		};
	}
	 
	void Parse(int argc, char* argv[]) {
		for (int i = 1; i < argc; i++)
		{
			params.insert(std::string(argv[i]));
		}
	}

	bool HasArgument(const std::string& value) {
		return params.find(value) != params.end();
	}
}


