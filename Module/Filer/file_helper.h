#pragma once
#include "Core/core_include.h"
#include "Core/platform.h"
#include "Module/Container/pvector.h"
#include "Engine/Device/graph_driver.h"
#include <string>
#include <functional>
#if VECTOR_TYPE
namespace std
{
	template < typename, typename > class vector;
}
#endif //	VECTOR_TYPE
namespace pf{
	namespace helper {

		enum class DebugLevel
		{
			Normal,
			Warning,
			Error
		};

		template <class T>
		constexpr void hash_combine(std::size_t& seed, const T& v)
		{
			std::hash<T> hasher;
			seed ^= hasher(v) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
		}

		constexpr size_t string_hash(const char* input)
		{
			// https://stackoverflow.com/questions/2111667/compile-time-string-hashing
			size_t hash = sizeof(size_t) == 8 ? 0xcbf29ce484222325 : 0x811c9dc5;
			const size_t prime = sizeof(size_t) == 8 ? 0x00000100000001b3 : 0x01000193;

			while(*input)
			{
				hash ^= static_cast<size_t>(*input);
				hash *= prime;
				++input;
			}
			return hash;
		}

		std::string toUpper(const std::string& s);
		
		std::string toLower(const std::string& s);
		
		std::wstring toUpper(const std::wstring& s);
		
		std::wstring toLower(const std::wstring& s);
		
	/*	
		void messageBox(const std::string& msg, const std::string& caption = "Warning!");
		enum class MessageBoxResult
		{
			OK,
			Cancel,
			Yes,
			No,
			Abort,
			Retry,
			Ignore
		};
	*/

		bool FileRead(const std::string& fileName, pf::vector<uint8_t>& data, size_t max_read = ~0ull, size_t offset = 0);

		bool FileWrite(const std::string& fileName, const uint8_t* data, size_t size);
		
		bool FileExists(const std::string& fileName);

		bool DirectoryExists(const std::string& fileName);

		uint64_t FileTimestamp(const std::string& fileName);

		bool FileCopy(const std::string& filename_src, const std::string& filename_dst);

		void GetFileNamesInDirectory(const std::string& directory, std::function<void(std::string fileName)> onSuccess, const std::string& filter_extension = "");

		void GetFolderNamesInDirectory(const std::string& directory, std::function<void(std::string folderName)> onSuccess);

		// Converts a file into a C++ header file that contains the file contents as byte array.
		//	dataName : the byte array's name
		bool Bin2H(const uint8_t* data, size_t size, const std::string& dst_filename, const char* dataName);

		// Converts a file into a C++ source file that contains the file contents as byte array and using extern.
		//	dataName : the byte array's name
		//	Note: size is exported as name_size where name is the dataName that you give to it
		bool Bin2CPP(const uint8_t* data, size_t size, const std::string& dst_filename, const char* dataName);

		void StringConvert(const std::string& from, std::wstring& to);

		void StringConvert(const std::wstring& from, std::string& to);

		std::string GetDirectoryFromPath(const std::string& fullPath);

		void SplitPath(const std::string& fullPath, std::string& dir, std::string& fileName);

		std::string GetCurrentPath();

		void DebugOut(const std::string& str, DebugLevel level);

		std::string GetPlatformErrorString(platform::error_type code);

		int StringConvert(const wchar_t* from, char* to, int dest_size_in_characters);

		bool Decompress(const uint8_t* src_data, size_t src_size, vector<uint8_t>& dst_data);
		

		std::string GetExtensionFromFileName(const std::string& filename);


		std::string RemoveExtension(const std::string& filename);

		void MakePathRelative(const std::string& rootdir, std::string& path);

		std::string GetPathRelative(const std::string& rootdir, const std::string& path);
		

		std::string GetFileNameFromPath(const std::string& fullPath);

		std::string ReplaceExtension(const std::string& filename, const std::string& extension);

		void DirectoryCreate(const std::string& path);

		void MakePathAbsolute(std::string& path);

		std::string GetMemorySizeText(size_t sizeInBytes);
		//bool saveTextureToMemoryFile(const graphics::Texture& texture, const std::string& fileExtension, vector<uint8_t>& filedata);

		//bool saveTextureToMemory(const graphics::Texture& texture, vector<uint8_t>& texturedata);


		//bool saveTextureToMemoryFile(const vector<uint8_t>& texturedata, const graphics::TextureDesc& desc, const std::string& fileExtension, vector<uint8_t>& filedata);
		

		void StringConvert(const std::string& from, std::wstring& to);

		void StringConvert(const std::wstring& from, std::string& to);

		// Parameter - to - must be pre-allocated!
		// dest_size_in_characters : number of characters in the pre-allocated string memory
		// returns result string length
		int StringConvert(const char* from, wchar_t* to, int dest_size_in_characters);

		// Parameter - to - must be pre-allocated!
		// dest_size_in_characters : number of characters in the pre-allocated string memory
		// returns result string length
		int StringConvert(const wchar_t* from, char* to, int dest_size_in_characters);

		// Returns string for paste operation
		std::wstring GetClipboardText();

		// Copies text to clipboard
		void SetClipboardText(const std::wstring& wstr);

		struct MemoryUsage
		{
			uint64_t total_physical = 0;	// size of physical memory on whole system (in bytes)
			uint64_t total_virtual = 0;		// size of virtual address space on whole system (in bytes)
			uint64_t process_physical = 0;	// size of currently committed physical memory by application (in bytes)
			uint64_t process_virtual = 0;	// size of currently mapped virtual memory by application (in bytes)
		};
		MemoryUsage GetMemoryUsage();

		// Puts the current thread to sleeping state for a given time (OS can overtake)
		void Sleep(float milliseconds);

		void QuickSleep(float milliseconds);
	}
}