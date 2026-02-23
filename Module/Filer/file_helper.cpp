#include "file_helper.h"

#include <thread>
#include <locale>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <vector>
#include <iostream>
#include <cstdlib>	
#include <filesystem>

#if defined(_WIN32)
#include <direct.h>
#include <Psapi.h> // GetProcessMemoryInfo
#include <Commdlg.h> // openfile
#include <WinBase.h>
#endif // _WIN32
#include "Module/Logger/backlogger.h"
#if defined(__FREEBSD__)
#include <kvm.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#pragma comment(lib,"kvm")
#elif defined(PLATFORM_LINUX)
#include <sys/sysinfo.h>
#endif // PLATFORM_LINUX

#ifdef PLATFORM_WINDOWS_DESKTOP
#include <comdef.h> // com_error
#endif 
#include <Utility/zstd/zstd.h>
#include <Utility/dds.h>
#include <Engine/Ecore/color.h>
#include <Utility/lodepng.h>
namespace pf {
	namespace helper {
		std::string GetExtensionFromFileName(const std::string& filename)
		{
			size_t idx = filename.rfind('.');
			if (idx != std::string::npos)
			{
				std::string extension = filename.substr(idx + 1);
				return extension;
			}
			// No extension found
			return "";
		}

		std::string RemoveExtension(const std::string& filename)
		{
			size_t idx = filename.rfind('.');

			if (idx == std::string::npos)
			{
				// extension not found:
				return filename;
			}
			return filename.substr(0, idx);
		}


#ifdef _WIN32
		// On windows we need to expand UTF8 strings to UTF16 when passing it to WinAPI:
		std::wstring ToNativeString(const std::string& fileName)
		{
			std::wstring fileName_wide;
			StringConvert(fileName, fileName_wide);
			return fileName_wide;
		}
#else
#define ToNativeString(x) (x)
#endif // _WIN32
		std::string FromWString(const std::wstring& fileName)
		{
			std::string fileName_u8;
			StringConvert(fileName, fileName_u8);
			return fileName_u8;
		}


		void MakePathRelative(const std::string& rootdir, std::string& path) {
			if (rootdir.empty() || path.empty())
			{
				return;
			}

			std::filesystem::path filepath = ToNativeString(path);
			if (filepath.is_absolute())
			{
				std::filesystem::path rootpath = ToNativeString(rootdir);
				std::filesystem::path relative = std::filesystem::relative(filepath, rootpath);
				if (!relative.empty())
				{
					StringConvert(relative.generic_wstring(), path);
				}
			}

		}

		std::string GetPathRelative(const std::string& rootdir, const std::string& path) {
			std::string ret = path;
			MakePathRelative(rootdir, ret);
			return ret;
		}


		template<template<typename T, typename A> typename vector_interface>
		bool FileReadIn(const std::string& fileName, vector_interface<uint8_t, std::allocator<uint8_t>>& data, size_t max_read, size_t offset)
		{
#if defined(PLATFORM_LINUX) || defined(PLATFORM_PS5)
			std::string filepath = fileName;
			std::replace(filepath.begin(), filepath.end(), '\\', '/'); // Linux cannot handle backslash in file path, need to convert it to forward slash
			std::ifstream file(filepath, std::ios::binary | std::ios::ate);
#else
			std::ifstream file(ToNativeString(fileName), std::ios::binary | std::ios::ate);
#endif // PLATFORM_LINUX || PLATFORM_PS5

			if (file.is_open())
			{
				size_t dataSize = (size_t)file.tellg() - offset;
				dataSize = std::min(dataSize, max_read);
				file.seekg((std::streampos)offset);
				data.resize(dataSize);
				file.read((char*)data.data(), dataSize);
				file.close();
				return true;
			}

			pf::backlogger::postin("File not found: " + fileName, LogLevel::Warning);
			return false;
		}

		std::string toUpper(const std::string& s) {
			std::string result;
			std::locale loc;
			for (size_t i = 0; i < s.length(); ++i)
			{
				result += std::toupper(s.at(i), loc);
			}
			return result;
		}

		std::string toLower(const std::string& s) {
			std::string result;
			std::locale loc;
			for (size_t i = 0; i < s.length(); ++i)
			{
				result += std::tolower(s.at(i), loc);
			}
			return result;
		}

		std::wstring toUpper(const std::wstring& s) {
			std::wstring result;
			std::locale loc;
			for (size_t i = 0; i < s.length(); ++i)
			{
				result += std::toupper(s.at(i), loc);
			}
			return result;
		}

		std::wstring toLower(const std::wstring& s) {
			std::wstring result;
			std::locale loc;
			for (size_t i = 0; i < s.length(); ++i)
			{
				result += std::tolower(s.at(i), loc);
			}
			return result;
		}

		bool FileRead(const std::string& fileName, pf::vector<uint8_t>& data, size_t max_read, size_t offset) {
			return FileReadIn(fileName, data, max_read, offset);
		}

		bool FileWrite(const std::string& fileName, const uint8_t* data, size_t size) {
			if (size <= 0)
			{
				return false;
			}

			std::ofstream file(ToNativeString(fileName), std::ios::binary | std::ios::trunc);
			if (file.is_open())
			{
				file.write((const char*)data, (std::streamsize)size);
				file.close();
				return true;
			}

			return false;
		}


		bool FileExists(const std::string& fileName){
			bool exists = std::filesystem::exists(ToNativeString(fileName));
			//std::filesystem::exists(ToNativeString(fileName));
			return exists;
		}

		bool DirectoryExists(const std::string& fileName) {
			bool exists = std::filesystem::exists(ToNativeString(fileName));
			return exists;
		}

		uint64_t FileTimestamp(const std::string& fileName) {
			if (!FileExists(fileName))
				return 0;
			auto tim = std::filesystem::last_write_time(ToNativeString(fileName));
			return std::chrono::duration_cast<std::chrono::duration<uint64_t>>(tim.time_since_epoch()).count();
		}

		bool FileCopy(const std::string& filename_src, const std::string& filename_dst) {
			return std::filesystem::copy_file(ToNativeString(filename_src), ToNativeString(filename_dst), std::filesystem::copy_options::overwrite_existing);
		}

		void GetFileNamesInDirectory(const std::string& directory, std::function<void(std::string fileName)> onSuccess, const std::string& filter_extension) {
			std::filesystem::path directory_path = ToNativeString(directory);
			if (!std::filesystem::exists(directory_path))
				return;
			
			for (const auto& entry : std::filesystem::directory_iterator(directory_path))
			{
				if (entry.is_directory())
					continue;
				std::string filename = FromWString(entry.path().filename().generic_wstring());
				if (filter_extension.empty() || toUpper(GetExtensionFromFileName(filename)).compare(toUpper(filter_extension)) == 0)
				{
					onSuccess(directory + filename);
				}
			}
		}

		void GetFolderNamesInDirectory(const std::string& directory, std::function<void(std::string folderName)> onSuccess) {
			std::filesystem::path directory_path = ToNativeString(directory);
			if (!std::filesystem::exists(directory_path))
				return;

			for (const auto& entry : std::filesystem::directory_iterator(directory_path))
			{
				if (!entry.is_directory())
					continue;
				std::string filename = FromWString(entry.path().filename().generic_wstring());
				onSuccess(directory + filename);
			}
		}

		// Converts a file into a C++ header file that contains the file contents as byte array.
		//	dataName : the byte array's name
		bool Bin2H(const uint8_t* data, size_t size, const std::string& dst_filename, const char* dataName) {
			std::string ss;
			ss += "const unsigned char ";
			ss += dataName;
			ss += "[] = {";
			for (size_t i = 0; i < size; ++i)
			{
				if (i % 32 == 0)
				{
					ss += "\n";
				}
				ss += std::to_string((uint32_t)data[i]) + ",";
			}
			ss += "\n};\n";
			return FileWrite(dst_filename, (uint8_t*)ss.c_str(), ss.length());
		}

		// Converts a file into a C++ source file that contains the file contents as byte array and using extern.
		//	dataName : the byte array's name
		//	Note: size is exported as name_size where name is the dataName that you give to it
		bool Bin2CPP(const uint8_t* data, size_t size, const std::string& dst_filename, const char* dataName) {
			std::string ss;
			ss += "extern const unsigned char ";
			ss += dataName;
			ss += "[] = {";
			for (size_t i = 0; i < size; ++i)
			{
				if (i % 32 == 0)
				{
					ss += "\n";
				}
				ss += std::to_string((uint32_t)data[i]) + ",";
			}
			ss += "\n};\n";
			ss += "extern const unsigned long long ";
			ss += dataName;
			ss += "_size = sizeof(";
			ss += dataName;
			ss += ");";
			return FileWrite(dst_filename, (uint8_t*)ss.c_str(), ss.length());
		}

		void StringConvert(const std::string& from, std::wstring& to) {
			to.clear();
			size_t i = 0;
			while (i < from.size())
			{
				uint32_t codepoint = 0;
				unsigned char c = from[i];

				if (c < 0x80)
				{
					codepoint = c;
					i += 1;
				}
				else if ((c & 0xE0) == 0xC0)
				{
					if (i + 1 >= from.size())
						break;
					codepoint = ((c & 0x1F) << 6) | (from[i + 1] & 0x3F);
					i += 2;
				}
				else if ((c & 0xF0) == 0xE0)
				{
					if (i + 2 >= from.size())
						break;
					codepoint = ((c & 0x0F) << 12) | ((from[i + 1] & 0x3F) << 6) | (from[i + 2] & 0x3F);
					i += 3;
				}
				else if ((c & 0xF8) == 0xF0)
				{
					if (i + 3 >= from.size())
						break;
					codepoint = ((c & 0x07) << 18) | ((from[i + 1] & 0x3F) << 12) | ((from[i + 2] & 0x3F) << 6) | (from[i + 3] & 0x3F);
					i += 4;
				}
				else
				{
					++i;
					continue;
				}

				if constexpr (sizeof(wchar_t) >= 4)
				{
					to += static_cast<wchar_t>(codepoint);
				}
				else
				{
					if (codepoint <= 0xFFFF)
					{
						to += static_cast<wchar_t>(codepoint);
					}
					else
					{
						codepoint -= 0x10000;
						to += static_cast<wchar_t>((codepoint >> 10) + 0xD800);
						to += static_cast<wchar_t>((codepoint & 0x3FF) + 0xDC00);
					}
				}
			}
		}
		std::string GetCurrentPath() {
#ifdef PLATFORM_PS5
			return "/app0";
#else
			auto path = std::filesystem::current_path();
			return FromWString(path.generic_wstring());
#endif // PLATFORM_PS5
		}


		int StringConvert(const wchar_t* from, char* to, int dest_size_in_characters) {
			if (!from || !to || dest_size_in_characters <= 0)
				return 0;

			int written = 0;
			for (int i = 0; from[i] != 0 && written < dest_size_in_characters - 1; ++i)
			{
				uint32_t codepoint = 0;
				wchar_t wc = from[i];

				if constexpr (sizeof(wchar_t) >= 4)
				{
					codepoint = static_cast<uint32_t>(wc);
				}
				else
				{
					if (wc >= 0xD800 && wc <= 0xDBFF)
					{
						wchar_t wc_low = from[i + 1];
						if (wc_low >= 0xDC00 && wc_low <= 0xDFFF)
						{
							codepoint = ((static_cast<uint32_t>(wc - 0xD800) << 10) | (static_cast<uint32_t>(wc_low - 0xDC00))) + 0x10000;
							++i;
						}
						else
						{
							codepoint = wc;
						}
					}
					else
					{
						codepoint = static_cast<uint32_t>(wc);
					}
				}

				if (codepoint <= 0x7F)
				{
					if (written + 1 >= dest_size_in_characters)
						break;
					to[written++] = static_cast<char>(codepoint);
				}
				else if (codepoint <= 0x7FF)
				{
					if (written + 2 >= dest_size_in_characters)
						break;
					to[written++] = static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F));
					to[written++] = static_cast<char>(0x80 | (codepoint & 0x3F));
				}
				else if (codepoint <= 0xFFFF)
				{
					if (written + 3 >= dest_size_in_characters)
						break;
					to[written++] = static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F));
					to[written++] = static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
					to[written++] = static_cast<char>(0x80 | (codepoint & 0x3F));
				}
				else if (codepoint <= 0x10FFFF)
				{
					if (written + 4 >= dest_size_in_characters)
						break;
					to[written++] = static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07));
					to[written++] = static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
					to[written++] = static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
					to[written++] = static_cast<char>(0x80 | (codepoint & 0x3F));
				}
			}

			if (written < dest_size_in_characters)
				to[written] = '\0';

			return written;
		}

		std::string GetPlatformErrorString(platform::error_type code) {
			std::string str;

#ifdef PLATFORM_WINDOWS_DESKTOP
			_com_error err(code);
			LPCTSTR errMsg = err.ErrorMessage();
			wchar_t wtext[1024] = {};
			_snwprintf_s(wtext, arraysize(wtext), arraysize(wtext), L"0x%08x (%s)", code, errMsg);
			char text[1024] = {};
			StringConvert(wtext, text, arraysize(text));
			str = text;
#endif // _WIN32

#ifdef PLATFORM_XBOX
			char text[1024] = {};
			snprintf(text, arraysize(text), "HRESULT error: 0x%08x", code);
			str = text;
#endif // PLATFORM_XBOX

			return str;
		}
		void SplitPath(const std::string& fullPath, std::string& dir, std::string& fileName)
		{
			size_t found;
			found = fullPath.find_last_of("/\\");
			dir = fullPath.substr(0, found + 1);
			fileName = fullPath.substr(found + 1);
		}
		void MakePathAbsolute(std::string& path)
		{
			std::filesystem::path absolute = std::filesystem::absolute(ToNativeString(path));
			if (!absolute.empty())
			{
				StringConvert(absolute.generic_wstring(), path);
			}
		}

		void DirectoryCreate(const std::string& path)
		{
			std::filesystem::create_directories(ToNativeString(path));
		}

		std::string ReplaceExtension(const std::string& filename, const std::string& extension)
		{
			size_t idx = filename.rfind('.');

			if (idx == std::string::npos)
			{
				// extension not found, append it:
				return filename + "." + extension;
			}
			return filename.substr(0, idx + 1) + extension;
		}

		std::string GetFileNameFromPath(const std::string& fullPath)
		{
			if (fullPath.empty())
			{
				return fullPath;
			}

			std::string ret, empty;
			SplitPath(fullPath, empty, ret);
			return ret;
		}

		std::string GetDirectoryFromPath(const std::string& fullPath) {
			if (fullPath.empty())
			{
				return fullPath;
			}

			std::string ret, empty;
			SplitPath(fullPath, ret, empty);
			return ret;
		}

		bool Decompress(const uint8_t* src_data, size_t src_size, vector<uint8_t>& dst_data){

			size_t res = ZSTD_getFrameContentSize(src_data, src_size);
			if (ZSTD_isError(res))
				return false;
			dst_data.resize(res);
			res = ZSTD_decompress(dst_data.data(), dst_data.size(), src_data, src_size);
			return ZSTD_isError(res) == 0;
		}

		std::string GetMemorySizeText(size_t sizeInBytes)
		{
			std::stringstream ss;
			ss << std::fixed << std::setprecision(1);
			if (sizeInBytes >= 1024ull * 1024ull * 1024ull)
			{
				ss << (double)sizeInBytes / 1024.0 / 1024.0 / 1024.0 << " GB";
			}
			else if (sizeInBytes >= 1024ull * 1024ull)
			{
				ss << (double)sizeInBytes / 1024.0 / 1024.0 << " MB";
			}
			else if (sizeInBytes >= 1024ull)
			{
				ss << (double)sizeInBytes / 1024.0 << " KB";
			}
			else
			{
				ss << sizeInBytes << " bytes";
			}
			return ss.str();
		}






	/*	bool saveTextureToMemoryFile(const graphics::Texture& texture, const std::string& fileExtension, vector<uint8_t>& filedata)
		{
			using namespace graphics;
			TextureDesc desc = texture.GetDesc();
			vector<uint8_t> texturedata;
			if (saveTextureToMemory(texture, texturedata))
			{
				return saveTextureToMemoryFile(texturedata, desc, fileExtension, filedata);
			}
			return false;
		}*/

		//bool saveTextureToMemoryFile(const vector<uint8_t>& texturedata, const graphics::TextureDesc& desc, const std::string& fileExtension, vector<uint8_t>& filedata)
		//{
		//	using namespace graphics;
		//	const uint32_t data_stride = GetFormatStride(desc.format);

		//	std::string extension = helper::toUpper(fileExtension);

		//	if (extension.compare("DDS") == 0)
		//	{
		//		filedata.resize(sizeof(dds::Header) + texturedata.size());
		//		dds::DXGI_FORMAT dds_format = dds::DXGI_FORMAT_UNKNOWN;
		//		switch (desc.format)
		//		{
		//		case graphics::Format::R32G32B32A32_FLOAT:
		//			dds_format = dds::DXGI_FORMAT_R32G32B32A32_FLOAT;
		//			break;
		//		case graphics::Format::R32G32B32A32_UINT:
		//			dds_format = dds::DXGI_FORMAT_R32G32B32A32_UINT;
		//			break;
		//		case graphics::Format::R32G32B32A32_SINT:
		//			dds_format = dds::DXGI_FORMAT_R32G32B32A32_SINT;
		//			break;
		//		case graphics::Format::R32G32B32_FLOAT:
		//			dds_format = dds::DXGI_FORMAT_R32G32B32_FLOAT;
		//			break;
		//		case graphics::Format::R32G32B32_UINT:
		//			dds_format = dds::DXGI_FORMAT_R32G32B32_UINT;
		//			break;
		//		case graphics::Format::R32G32B32_SINT:
		//			dds_format = dds::DXGI_FORMAT_R32G32B32_SINT;
		//			break;
		//		case graphics::Format::R16G16B16A16_FLOAT:
		//			dds_format = dds::DXGI_FORMAT_R16G16B16A16_FLOAT;
		//			break;
		//		case Format::R16G16B16A16_UNORM:
		//			dds_format = dds::DXGI_FORMAT_R16G16B16A16_UNORM;
		//			break;
		//		case graphics::Format::R16G16B16A16_UINT:
		//			dds_format = dds::DXGI_FORMAT_R16G16B16A16_UINT;
		//			break;
		//		case graphics::Format::R16G16B16A16_SNORM:
		//			dds_format = dds::DXGI_FORMAT_R16G16B16A16_SNORM;
		//			break;
		//		case graphics::Format::R16G16B16A16_SINT:
		//			dds_format = dds::DXGI_FORMAT_R16G16B16A16_SINT;
		//			break;
		//		case graphics::Format::R32G32_FLOAT:
		//			dds_format = dds::DXGI_FORMAT_R32G32_FLOAT;
		//			break;
		//		case graphics::Format::R32G32_UINT:
		//			dds_format = dds::DXGI_FORMAT_R32G32_UINT;
		//			break;
		//		case graphics::Format::R32G32_SINT:
		//			dds_format = dds::DXGI_FORMAT_R32G32_SINT;
		//			break;
		//		case graphics::Format::R10G10B10A2_UNORM:
		//			dds_format = dds::DXGI_FORMAT_R10G10B10A2_UNORM;
		//			break;
		//		case graphics::Format::R10G10B10A2_UINT:
		//			dds_format = dds::DXGI_FORMAT_R10G10B10A2_UINT;
		//			break;
		//		case graphics::Format::R11G11B10_FLOAT:
		//			dds_format = dds::DXGI_FORMAT_R11G11B10_FLOAT;
		//			break;
		//		case graphics::Format::R8G8B8A8_UNORM:
		//			dds_format = dds::DXGI_FORMAT_R8G8B8A8_UNORM;
		//			break;
		//		case graphics::Format::R8G8B8A8_UNORM_SRGB:
		//			dds_format = dds::DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
		//			break;
		//		case graphics::Format::R8G8B8A8_UINT:
		//			dds_format = dds::DXGI_FORMAT_R8G8B8A8_UINT;
		//			break;
		//		case graphics::Format::R8G8B8A8_SNORM:
		//			dds_format = dds::DXGI_FORMAT_R8G8B8A8_SNORM;
		//			break;
		//		case graphics::Format::R8G8B8A8_SINT:
		//			dds_format = dds::DXGI_FORMAT_R8G8B8A8_SINT;
		//			break;
		//		case graphics::Format::B8G8R8A8_UNORM:
		//			dds_format = dds::DXGI_FORMAT_B8G8R8A8_UNORM;
		//			break;
		//		case graphics::Format::B8G8R8A8_UNORM_SRGB:
		//			dds_format = dds::DXGI_FORMAT_R16G16_SINT;
		//			break;
		//		case graphics::Format::R16G16_FLOAT:
		//			dds_format = dds::DXGI_FORMAT_R16G16_FLOAT;
		//			break;
		//		case graphics::Format::R16G16_UNORM:
		//			dds_format = dds::DXGI_FORMAT_R16G16_UNORM;
		//			break;
		//		case graphics::Format::R16G16_UINT:
		//			dds_format = dds::DXGI_FORMAT_R16G16_UINT;
		//			break;
		//		case graphics::Format::R16G16_SNORM:
		//			dds_format = dds::DXGI_FORMAT_R16G16_SNORM;
		//			break;
		//		case graphics::Format::R16G16_SINT:
		//			dds_format = dds::DXGI_FORMAT_R16G16_SINT;
		//			break;
		//		case graphics::Format::D32_FLOAT:
		//		case graphics::Format::R32_FLOAT:
		//			dds_format = dds::DXGI_FORMAT_R32_FLOAT;
		//			break;
		//		case graphics::Format::R32_UINT:
		//			dds_format = dds::DXGI_FORMAT_R32_UINT;
		//			break;
		//		case graphics::Format::R32_SINT:
		//			dds_format = dds::DXGI_FORMAT_R32_SINT;
		//			break;
		//		case graphics::Format::R9G9B9E5_SHAREDEXP:
		//			dds_format = dds::DXGI_FORMAT_R9G9B9E5_SHAREDEXP;
		//			break;
		//		case graphics::Format::R8G8_UNORM:
		//			dds_format = dds::DXGI_FORMAT_R8G8_UNORM;
		//			break;
		//		case graphics::Format::R8G8_UINT:
		//			dds_format = dds::DXGI_FORMAT_R8G8_UINT;
		//			break;
		//		case graphics::Format::R8G8_SNORM:
		//			dds_format = dds::DXGI_FORMAT_R8G8_SNORM;
		//			break;
		//		case graphics::Format::R8G8_SINT:
		//			dds_format = dds::DXGI_FORMAT_R8G8_SINT;
		//			break;
		//		case graphics::Format::R16_FLOAT:
		//			dds_format = dds::DXGI_FORMAT_R16_FLOAT;
		//			break;
		//		case graphics::Format::D16_UNORM:
		//		case graphics::Format::R16_UNORM:
		//			dds_format = dds::DXGI_FORMAT_R16_UNORM;
		//			break;
		//		case graphics::Format::R16_UINT:
		//			dds_format = dds::DXGI_FORMAT_R16_UINT;
		//			break;
		//		case graphics::Format::R16_SNORM:
		//			dds_format = dds::DXGI_FORMAT_R16_SNORM;
		//			break;
		//		case graphics::Format::R16_SINT:
		//			dds_format = dds::DXGI_FORMAT_R16_SINT;
		//			break;
		//		case graphics::Format::R8_UNORM:
		//			dds_format = dds::DXGI_FORMAT_R8_UNORM;
		//			break;
		//		case graphics::Format::R8_UINT:
		//			dds_format = dds::DXGI_FORMAT_R8_UINT;
		//			break;
		//		case graphics::Format::R8_SNORM:
		//			dds_format = dds::DXGI_FORMAT_R8_SNORM;
		//			break;
		//		case graphics::Format::R8_SINT:
		//			dds_format = dds::DXGI_FORMAT_R8_SINT;
		//			break;
		//		case graphics::Format::BC1_UNORM:
		//			dds_format = dds::DXGI_FORMAT_BC1_UNORM;
		//			break;
		//		case graphics::Format::BC1_UNORM_SRGB:
		//			dds_format = dds::DXGI_FORMAT_BC1_UNORM_SRGB;
		//			break;
		//		case graphics::Format::BC2_UNORM:
		//			dds_format = dds::DXGI_FORMAT_BC2_UNORM;
		//			break;
		//		case graphics::Format::BC2_UNORM_SRGB:
		//			dds_format = dds::DXGI_FORMAT_BC2_UNORM_SRGB;
		//			break;
		//		case graphics::Format::BC3_UNORM:
		//			dds_format = dds::DXGI_FORMAT_BC3_UNORM;
		//			break;
		//		case graphics::Format::BC3_UNORM_SRGB:
		//			dds_format = dds::DXGI_FORMAT_BC3_UNORM_SRGB;
		//			break;
		//		case graphics::Format::BC4_UNORM:
		//			dds_format = dds::DXGI_FORMAT_BC4_UNORM;
		//			break;
		//		case graphics::Format::BC4_SNORM:
		//			dds_format = dds::DXGI_FORMAT_BC4_SNORM;
		//			break;
		//		case graphics::Format::BC5_UNORM:
		//			dds_format = dds::DXGI_FORMAT_BC5_UNORM;
		//			break;
		//		case graphics::Format::BC5_SNORM:
		//			dds_format = dds::DXGI_FORMAT_BC5_SNORM;
		//			break;
		//		case graphics::Format::BC6H_UF16:
		//			dds_format = dds::DXGI_FORMAT_BC6H_UF16;
		//			break;
		//		case graphics::Format::BC6H_SF16:
		//			dds_format = dds::DXGI_FORMAT_BC6H_SF16;
		//			break;
		//		case graphics::Format::BC7_UNORM:
		//			dds_format = dds::DXGI_FORMAT_BC7_UNORM;
		//			break;
		//		case graphics::Format::BC7_UNORM_SRGB:
		//			dds_format = dds::DXGI_FORMAT_BC7_UNORM_SRGB;
		//			break;
		//		default:
		//			assert(0);
		//			return false;
		//		}
		//		dds::write_header(
		//			filedata.data(),
		//			dds_format,
		//			desc.width,
		//			desc.type == TextureDesc::Type::TEXTURE_1D ? 0 : desc.height,
		//			desc.mip_levels,
		//			desc.array_size,
		//			has_flag(desc.misc_flags, ResourceMiscFlag::TEXTURECUBE),
		//			desc.type == TextureDesc::Type::TEXTURE_3D ? desc.depth : 0
		//		);
		//		std::memcpy(filedata.data() + sizeof(dds::Header), texturedata.data(), texturedata.size());
		//		return true;
		//	}


		//	const bool is_png = extension.compare("PNG") == 0;

		//	if (is_png)
		//	{
		//		if (desc.format == Format::R16_UNORM || desc.format == Format::R16_UINT)
		//		{
		//			// Specialized handling for 16-bit single channel PNG:
		//			vector<uint8_t> src_bigendian = texturedata;
		//			uint16_t* dest = (uint16_t*)src_bigendian.data();
		//			for (uint32_t i = 0; i < desc.width * desc.height; ++i)
		//			{
		//				uint16_t r = dest[i];
		//				r = (r >> 8) | ((r & 0xFF) << 8); // little endian to big endian
		//				dest[i] = r;
		//			}
		//			unsigned error = lodepng::encode(filedata, src_bigendian, desc.width, desc.height, LCT_GREY, 16);
		//			return error == 0;
		//		}
		//		if (desc.format == Format::R16G16_UNORM || desc.format == Format::R16G16_UINT)
		//		{
		//			// Specialized handling for 16-bit PNG:
		//			//	Two channel RG data is expanded to RGBA (2-channel PNG is not good because that is interpreted as red and alpha)
		//			vector<uint8_t> src_bigendian = texturedata;
		//			const uint32_t* src_rg = (const uint32_t*)src_bigendian.data();
		//			vector<Color16> dest_rgba(desc.width * desc.height);
		//			for (uint32_t i = 0; i < desc.width * desc.height; ++i)
		//			{
		//				uint32_t rg = src_rg[i];
		//				Color16& rgba = dest_rgba[i];
		//				uint16_t r = rg & 0xFFFF;
		//				r = (r >> 8) | ((r & 0xFF) << 8); // little endian to big endian
		//				uint16_t g = (rg >> 16u) & 0xFFFF;
		//				g = (g >> 8) | ((g & 0xFF) << 8); // little endian to big endian
		//				rgba = Color16(r, g, 0xFFFF, 0xFFFF);
		//			}
		//			unsigned error = lodepng::encode(filedata, (const unsigned char*)dest_rgba.data(), desc.width, desc.height, LCT_RGBA, 16);
		//			return error == 0;
		//		}
		//		if (desc.format == Format::R16G16B16A16_UNORM || desc.format == Format::R16G16B16A16_UINT)
		//		{
		//			// Specialized handling for 16-bit PNG:
		//			vector<uint8_t> src_bigendian = texturedata;
		//			Color16* dest = (Color16*)src_bigendian.data();
		//			for (uint32_t i = 0; i < desc.width * desc.height; ++i)
		//			{
		//				Color16 rgba = dest[i];
		//				uint16_t r = rgba.getR();
		//				r = (r >> 8) | ((r & 0xFF) << 8); // little endian to big endian
		//				uint16_t g = rgba.getG();
		//				g = (g >> 8) | ((g & 0xFF) << 8); // little endian to big endian
		//				uint16_t b = rgba.getB();
		//				b = (b >> 8) | ((b & 0xFF) << 8); // little endian to big endian
		//				uint16_t a = rgba.getA();
		//				a = (a >> 8) | ((a & 0xFF) << 8); // little endian to big endian
		//				rgba = Color16(r, g, b, a);
		//				dest[i] = rgba;
		//			}
		//			unsigned error = lodepng::encode(filedata, src_bigendian, desc.width, desc.height, LCT_RGBA, 16);
		//			return error == 0;
		//		}
		//	}

		//	struct MipDesc
		//	{
		//		const uint8_t* address = nullptr;
		//		uint32_t width = 0;
		//		uint32_t height = 0;
		//		uint32_t depth = 0;
		//	};
		//	pf::vector<MipDesc> mips;
		//	mips.reserve(desc.mip_levels);

		//	uint32_t data_count = 0;
		//	uint32_t mip_width = desc.width;
		//	uint32_t mip_height = desc.height;
		//	uint32_t mip_depth = desc.depth;
		//	for (uint32_t mip = 0; mip < desc.mip_levels; ++mip)
		//	{
		//		MipDesc& mipdesc = mips.emplace_back();
		//		mipdesc.address = texturedata.data() + data_count * data_stride;
		//		data_count += mip_width * mip_height * mip_depth;
		//		mipdesc.width = mip_width;
		//		mipdesc.height = mip_height;
		//		mipdesc.depth = mip_depth;
		//		mip_width = std::max(1u, mip_width / 2);
		//		mip_height = std::max(1u, mip_height / 2);
		//		mip_depth = std::max(1u, mip_depth / 2);
		//	}

		//	int dst_channel_count = 4;
		//	if (desc.format == Format::R10G10B10A2_UNORM)
		//	{
		//		// This will be converted first to rgba8 before saving to common format:
		//		uint32_t* data32 = (uint32_t*)texturedata.data();

		//		for (uint32_t i = 0; i < data_count; ++i)
		//		{
		//			uint32_t pixel = data32[i];
		//			float r = ((pixel >> 0) & 1023) / 1023.0f;
		//			float g = ((pixel >> 10) & 1023) / 1023.0f;
		//			float b = ((pixel >> 20) & 1023) / 1023.0f;
		//			float a = ((pixel >> 30) & 3) / 3.0f;

		//			uint32_t rgba8 = 0;
		//			rgba8 |= (uint32_t)(r * 255.0f) << 0;
		//			rgba8 |= (uint32_t)(g * 255.0f) << 8;
		//			rgba8 |= (uint32_t)(b * 255.0f) << 16;
		//			rgba8 |= (uint32_t)(a * 255.0f) << 24;

		//			data32[i] = rgba8;
		//		}
		//	}
		//	else if (desc.format == Format::R32G32B32A32_FLOAT)
		//	{
		//		// This will be converted first to rgba8 before saving to common format:
		//		XMFLOAT4* dataSrc = (XMFLOAT4*)texturedata.data();
		//		uint32_t* data32 = (uint32_t*)texturedata.data();

		//		for (uint32_t i = 0; i < data_count; ++i)
		//		{
		//			XMFLOAT4 pixel = dataSrc[i];
		//			float r = std::max(0.0f, std::min(pixel.x, 1.0f));
		//			float g = std::max(0.0f, std::min(pixel.y, 1.0f));
		//			float b = std::max(0.0f, std::min(pixel.z, 1.0f));
		//			float a = std::max(0.0f, std::min(pixel.w, 1.0f));

		//			uint32_t rgba8 = 0;
		//			rgba8 |= (uint32_t)(r * 255.0f) << 0;
		//			rgba8 |= (uint32_t)(g * 255.0f) << 8;
		//			rgba8 |= (uint32_t)(b * 255.0f) << 16;
		//			rgba8 |= (uint32_t)(a * 255.0f) << 24;

		//			data32[i] = rgba8;
		//		}
		//	}
		//	else if (desc.format == Format::R16G16B16A16_FLOAT)
		//	{
		//		// This will be converted first to rgba8 before saving to common format:
		//		XMHALF4* dataSrc = (XMHALF4*)texturedata.data();
		//		uint32_t* data32 = (uint32_t*)texturedata.data();

		//		for (uint32_t i = 0; i < data_count; ++i)
		//		{
		//			XMHALF4 pixel = dataSrc[i];
		//			float r = std::max(0.0f, std::min(XMConvertHalfToFloat(pixel.x), 1.0f));
		//			float g = std::max(0.0f, std::min(XMConvertHalfToFloat(pixel.y), 1.0f));
		//			float b = std::max(0.0f, std::min(XMConvertHalfToFloat(pixel.z), 1.0f));
		//			float a = std::max(0.0f, std::min(XMConvertHalfToFloat(pixel.w), 1.0f));

		//			uint32_t rgba8 = 0;
		//			rgba8 |= (uint32_t)(r * 255.0f) << 0;
		//			rgba8 |= (uint32_t)(g * 255.0f) << 8;
		//			rgba8 |= (uint32_t)(b * 255.0f) << 16;
		//			rgba8 |= (uint32_t)(a * 255.0f) << 24;

		//			data32[i] = rgba8;
		//		}
		//	}
		//	else if (desc.format == Format::R16G16B16A16_UNORM || desc.format == Format::R16G16B16A16_UINT)
		//	{
		//		// This will be converted first to rgba8 before saving to common format:
		//		pf::Color16* dataSrc = (pf::Color16*)texturedata.data();
		//		pf::Color* data32 = (pf::Color*)texturedata.data();

		//		for (uint32_t i = 0; i < data_count; ++i)
		//		{
		//			pf::Color16 pixel16 = dataSrc[i];
		//			data32[i] = pf::Color::fromFloat4(pixel16.toFloat4());
		//		}
		//	}
		//	else if (desc.format == Format::R11G11B10_FLOAT)
		//	{
		//		// This will be converted first to rgba8 before saving to common format:
		//		XMFLOAT3PK* dataSrc = (XMFLOAT3PK*)texturedata.data();
		//		uint32_t* data32 = (uint32_t*)texturedata.data();

		//		for (uint32_t i = 0; i < data_count; ++i)
		//		{
		//			XMFLOAT3PK pixel = dataSrc[i];
		//			XMVECTOR V = XMLoadFloat3PK(&pixel);
		//			XMFLOAT3 pixel3;
		//			XMStoreFloat3(&pixel3, V);
		//			float r = std::max(0.0f, std::min(pixel3.x, 1.0f));
		//			float g = std::max(0.0f, std::min(pixel3.y, 1.0f));
		//			float b = std::max(0.0f, std::min(pixel3.z, 1.0f));
		//			float a = 1;

		//			uint32_t rgba8 = 0;
		//			rgba8 |= (uint32_t)(r * 255.0f) << 0;
		//			rgba8 |= (uint32_t)(g * 255.0f) << 8;
		//			rgba8 |= (uint32_t)(b * 255.0f) << 16;
		//			rgba8 |= (uint32_t)(a * 255.0f) << 24;

		//			data32[i] = rgba8;
		//		}
		//	}
		//	else if (desc.format == Format::R9G9B9E5_SHAREDEXP)
		//	{
		//		// This will be converted first to rgba8 before saving to common format:
		//		XMFLOAT3SE* dataSrc = (XMFLOAT3SE*)texturedata.data();
		//		uint32_t* data32 = (uint32_t*)texturedata.data();

		//		for (uint32_t i = 0; i < data_count; ++i)
		//		{
		//			XMFLOAT3SE pixel = dataSrc[i];
		//			XMVECTOR V = XMLoadFloat3SE(&pixel);
		//			XMFLOAT3 pixel3;
		//			XMStoreFloat3(&pixel3, V);
		//			float r = std::max(0.0f, std::min(pixel3.x, 1.0f));
		//			float g = std::max(0.0f, std::min(pixel3.y, 1.0f));
		//			float b = std::max(0.0f, std::min(pixel3.z, 1.0f));
		//			float a = 1;

		//			uint32_t rgba8 = 0;
		//			rgba8 |= (uint32_t)(r * 255.0f) << 0;
		//			rgba8 |= (uint32_t)(g * 255.0f) << 8;
		//			rgba8 |= (uint32_t)(b * 255.0f) << 16;
		//			rgba8 |= (uint32_t)(a * 255.0f) << 24;

		//			data32[i] = rgba8;
		//		}
		//	}
		//	else if (desc.format == Format::B8G8R8A8_UNORM || desc.format == Format::B8G8R8A8_UNORM_SRGB)
		//	{
		//		// This will be converted first to rgba8 before saving to common format:
		//		uint32_t* data32 = (uint32_t*)texturedata.data();

		//		for (uint32_t i = 0; i < data_count; ++i)
		//		{
		//			uint32_t pixel = data32[i];
		//			uint8_t b = (pixel >> 0u) & 0xFF;
		//			uint8_t g = (pixel >> 8u) & 0xFF;
		//			uint8_t r = (pixel >> 16u) & 0xFF;
		//			uint8_t a = (pixel >> 24u) & 0xFF;
		//			data32[i] = r | (g << 8u) | (b << 16u) | (a << 24u);
		//		}
		//	}
		//	else if (desc.format == Format::R8_UNORM)
		//	{
		//		// This can be saved by reducing target channel count, no conversion needed
		//		dst_channel_count = 1;
		//	}
		//	else if (desc.format == Format::R8G8_UNORM)
		//	{
		//		// This can be saved by reducing target channel count, no conversion needed
		//		dst_channel_count = 2;
		//	}
		//	else
		//	{
		//		assert(desc.format == Format::R8G8B8A8_UNORM || desc.format == Format::R8G8B8A8_UNORM_SRGB); // If you need to save other texture format, implement data conversion for it
		//	}

		//	if (!extension.compare("ICO"))
		//	{
		//		const uint32_t minsize = 32;

		//		size_t filesize = sizeof(ico::ICONDIR);

		//		ico::ICONDIR icondir = { 0,1,0 };

		//		for (auto& mip : mips)
		//		{
		//			if (mip.width > 256 || mip.height > 256)
		//				continue;
		//			if (mip.width < minsize || mip.height < minsize)
		//				break;
		//			icondir.idCount++;
		//			filesize += sizeof(ico::ICONDIRENTRY);
		//		}

		//		if (icondir.idCount < 1)
		//		{
		//			p_log_assert(0, "No valid images were found that can be added to ICO file format!");
		//			return false;
		//		}

		//		uint32_t imageDataOffset = (uint32_t)filesize;

		//		for (auto& mip : mips)
		//		{
		//			if (mip.width > 256 || mip.height > 256)
		//				continue;
		//			if (mip.width < minsize || mip.height < minsize)
		//				break;
		//			const uint32_t pixelCount = mip.width * mip.height;
		//			const uint32_t rgbDataSize = pixelCount * 4; // 32-bit RGBA
		//			const uint32_t maskSize = ((mip.width + 7) / 8) * mip.height; // 1-bit mask, padded to byte
		//			const uint32_t imageDataSize = sizeof(ico::BITMAPINFOHEADER) + rgbDataSize + maskSize;
		//			filesize += imageDataSize;
		//		}

		//		filedata.resize(filesize);
		//		uint8_t* ptr = filedata.data();

		//		std::memcpy(ptr, &icondir, sizeof(ico::ICONDIR));
		//		ptr += sizeof(ico::ICONDIR);

		//		for (auto& mip : mips)
		//		{
		//			if (mip.width > 256 || mip.height > 256)
		//				continue;
		//			if (mip.width < minsize || mip.height < minsize)
		//				break;

		//			const uint32_t pixelCount = mip.width * mip.height;
		//			const uint32_t rgbDataSize = pixelCount * 4; // 32-bit RGBA
		//			const uint32_t maskSize = ((mip.width + 7) / 8) * mip.height; // 1-bit mask, padded to byte
		//			const uint32_t imageDataSize = sizeof(ico::BITMAPINFOHEADER) + rgbDataSize + maskSize;

		//			ico::ICONDIRENTRY iconEntry = {
		//				static_cast<uint8_t>(mip.width > 255 ? 0 : mip.width), // Width (0 for 256+)
		//				static_cast<uint8_t>(mip.height > 255 ? 0 : mip.height), // Height (0 for 256+)
		//				0, // Color count (0 for 32-bit)
		//				0, // Reserved
		//				1, // Color planes
		//				32, // Bits per pixel
		//				imageDataSize, // Size of image data
		//				imageDataOffset // Offset to image data
		//			};
		//			std::memcpy(ptr, &iconEntry, sizeof(ico::ICONDIRENTRY));
		//			ptr += sizeof(ico::ICONDIRENTRY);
		//			imageDataOffset += imageDataSize;
		//		}

		//		for (auto& mip : mips)
		//		{
		//			if (mip.width > 256 || mip.height > 256)
		//				continue;
		//			if (mip.width < minsize || mip.height < minsize)
		//				break;

		//			const uint32_t pixelCount = mip.width * mip.height;
		//			const uint32_t rgbDataSize = pixelCount * 4; // 32-bit RGBA
		//			const uint32_t maskSize = ((mip.width + 7) / 8) * mip.height; // 1-bit mask, padded to byte

		//			ico::BITMAPINFOHEADER bmpHeader = {
		//				sizeof(ico::BITMAPINFOHEADER), // Size of header
		//				int32_t(mip.width), // Width
		//				int32_t(mip.height * 2), // Height (doubled for XOR + AND mask)
		//				1, // Planes
		//				32, // Bits per pixel
		//				0, // No compression
		//				rgbDataSize + maskSize, // Image size
		//				0, // X pixels per meter
		//				0, // Y pixels per meter
		//				0, // Colors used
		//				0  // Important colors
		//			};
		//			std::memcpy(ptr, &bmpHeader, sizeof(ico::BITMAPINFOHEADER));
		//			ptr += sizeof(ico::BITMAPINFOHEADER);

		//			// Convert RGBA to BGRA and write XOR mask (flipped vertically)
		//			for (uint32_t y = mip.height; y > 0; --y)
		//			{
		//				const uint8_t* src = mip.address + (y - 1) * mip.width * 4;
		//				for (uint32_t x = 0; x < mip.width; ++x)
		//				{
		//					// Convert RGBA to BGRA
		//					ptr[0] = src[2]; // B
		//					ptr[1] = src[1]; // G
		//					ptr[2] = src[0]; // R
		//					ptr[3] = src[3]; // A
		//					ptr += 4;
		//					src += 4;
		//				}
		//			}

		//			// Write AND mask (1-bit transparency mask)
		//			for (uint32_t y = mip.height; y > 0; --y)
		//			{
		//				const uint8_t* src = mip.address + (y - 1) * mip.width * 4;
		//				for (uint32_t x = 0; x < mip.width; x += 8)
		//				{
		//					uint8_t maskByte = 0;
		//					for (uint32_t bit = 0; bit < 8 && (x + bit) < mip.width; ++bit)
		//					{
		//						// Set bit to 0 if pixel is opaque (alpha > 0), 1 if transparent
		//						if (src[(x + bit) * 4 + 3] == 0)
		//						{
		//							maskByte |= (1 << (7 - bit));
		//						}
		//					}
		//					*ptr++ = maskByte;
		//				}
		//			}
		//		}

		//		return true;
		//	}



		//}





		void DebugOut(const std::string& str,DebugLevel level)
		{
#ifndef _WIN32
			std::wstring wstr = ToNativeString(str);
			OutputDebugString(wstr.c_str());
#else
			switch (level)
			{
			default:
			case DebugLevel::Normal:
				std::cout << str;
				std::flush(std::cout);
				break;
			case DebugLevel::Warning:
				std::clog << str;
				std::flush(std::clog);
				break;
			case DebugLevel::Error:
				std::cerr << str;
				std::flush(std::cerr);
				break;
			}
#endif // _WIN32
		}

		void StringConvert(const std::wstring& from, std::string& to) {
			to.clear();
			for (size_t i = 0; i < from.size(); ++i)
			{
				uint32_t codepoint = 0;
				wchar_t wc = from[i];

				if constexpr (sizeof(wchar_t) >= 4)
				{
					codepoint = static_cast<uint32_t>(wc);
				}
				else
				{
					if (wc >= 0xD800 && wc <= 0xDBFF)
					{
						if (i + 1 < from.size())
						{
							wchar_t wc_low = from[i + 1];
							if (wc_low >= 0xDC00 && wc_low <= 0xDFFF)
							{
								codepoint = ((static_cast<uint32_t>(wc - 0xD800) << 10) | (static_cast<uint32_t>(wc_low - 0xDC00))) + 0x10000;
								++i;
							}
						}
					}
					else
					{
						codepoint = static_cast<uint32_t>(wc);
					}
				}

				if (codepoint <= 0x7F)
				{
					to += static_cast<char>(codepoint);
				}
				else if (codepoint <= 0x7FF)
				{
					to += static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F));
					to += static_cast<char>(0x80 | (codepoint & 0x3F));
				}
				else if (codepoint <= 0xFFFF)
				{
					to += static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F));
					to += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
					to += static_cast<char>(0x80 | (codepoint & 0x3F));
				}
				else if (codepoint <= 0x10FFFF)
				{
					to += static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07));
					to += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
					to += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
					to += static_cast<char>(0x80 | (codepoint & 0x3F));
				}
			}
		}



		MemoryUsage GetMemoryUsage()
		{
			MemoryUsage mem;
#if defined(_WIN32)
			// https://stackoverflow.com/questions/63166/how-to-determine-cpu-and-memory-consumption-from-inside-a-process
			MEMORYSTATUSEX memInfo = {};
			memInfo.dwLength = sizeof(MEMORYSTATUSEX);
			BOOL ret = GlobalMemoryStatusEx(&memInfo);
			assert(ret);
			mem.total_physical = memInfo.ullTotalPhys;
			mem.total_virtual = memInfo.ullTotalVirtual;

			PROCESS_MEMORY_COUNTERS_EX pmc = {};
			GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc));
			mem.process_physical = pmc.WorkingSetSize;
			mem.process_virtual = pmc.PrivateUsage;
#elif defined(__FREEBSD__)
			static uint64_t phys_mem = 0;
			static uint64_t swap_mem = 0;
			size_t sysctl_buf_s;
			int page_size = getpagesize();
			if (phys_mem == 0)
			{
				int phys_mem_name[] = { CTL_HW, HW_PHYSMEM };
				sysctl_buf_s = sizeof(phys_mem);
				sysctl(phys_mem_name, 2, &phys_mem, &sysctl_buf_s, NULL, 0);
				kvm_t* kvm;
				if ((kvm = kvm_open(NULL, "/dev/null", "/dev/null", O_RDONLY, "GetMemoryUsage")) != NULL)
				{
					struct kvm_swap swap;
					if (kvm_getswapinfo(kvm, &swap, 1, 0) == 0)
						swap_mem = swap.ksw_total * page_size;
					kvm_close(kvm);
				}
			}
			mem.total_physical = phys_mem;
			mem.total_virtual = swap_mem;
			struct kinfo_proc kinfo;
			int proc_name[] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, getpid() };
			sysctl_buf_s = sizeof(struct kinfo_proc);
			if (sysctl(proc_name, 4, &kinfo, &sysctl_buf_s, NULL, 0) == 0)
			{
				mem.process_physical = kinfo.ki_rssize * page_size;
			}
#elif defined(PLATFORM_LINUX)
			struct sysinfo info;
			constexpr int PAGE_SIZE = 4096;
			if (sysinfo(&info) == 0)
			{
				unsigned long phys = info.totalram - info.totalswap;
				mem.total_physical = phys * info.mem_unit;
				mem.total_virtual = info.totalswap * info.mem_unit;
			}
			unsigned long l;
			std::ifstream statm("/proc/self/statm");
			// Format of statm:
			// size resident shared trs lrs drs dt
			// see linux Documentation/filesystems/proc.rst

			// we want "resident", the second number, so just read the first one
			// and discard it
			statm >> l;
			statm >> l;
			mem.process_physical = l * PAGE_SIZE;
			// there doesn't seem to be an easy way to determine
			// swapped out memory
#elif defined(PLATFORM_PS5)
			pf::graphics::GraphicsDevice::MemoryUsage gpumem = pf::graphics::GetDevice()->GetMemoryUsage();
			mem.process_physical = mem.total_physical = gpumem.budget;
			mem.process_virtual = mem.total_virtual = gpumem.usage;
#endif // defined(_WIN32)
			return mem;
		}


		std::wstring GetClipboardText()
		{
			std::wstring wstr;

#ifdef PLATFORM_WINDOWS_DESKTOP
			if (!::OpenClipboard(NULL))
				return wstr;
			HANDLE wbuf_handle = ::GetClipboardData(CF_UNICODETEXT);
			if (wbuf_handle == NULL)
			{
				::CloseClipboard();
				return wstr;
			}
			if (const WCHAR* wbuf_global = (const WCHAR*)::GlobalLock(wbuf_handle))
			{
				wstr = wbuf_global;
			}
			::GlobalUnlock(wbuf_handle);
			::CloseClipboard();
#endif // PLATFORM_WINDOWS_DESKTOP

			return wstr;
		}

		void Sleep(float milliseconds)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds((int)milliseconds));
		}

		void QuickSleep(float milliseconds)
		{
			const std::chrono::high_resolution_clock::time_point t1 = std::chrono::high_resolution_clock::now();
			const double seconds = double(milliseconds) / 1000.0;
			const int sleep_millisec_accuracy = 1;
			const double sleep_sec_accuracy = double(sleep_millisec_accuracy) / 1000.0;
			while (std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::high_resolution_clock::now() - t1).count() < seconds)
			{
				if (seconds - (std::chrono::high_resolution_clock::now() - t1).count() > sleep_sec_accuracy)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(sleep_millisec_accuracy));
				}
			}
		}
		void SetClipboardText(const std::wstring& wstr)
		{
#ifdef PLATFORM_WINDOWS_DESKTOP
			if (!::OpenClipboard(NULL))
				return;
			const int wbuf_length = (int)wstr.length() + 1;
			HGLOBAL wbuf_handle = ::GlobalAlloc(GMEM_MOVEABLE, (SIZE_T)wbuf_length * sizeof(WCHAR));
			if (wbuf_handle == NULL)
			{
				::CloseClipboard();
				return;
			}
			WCHAR* wbuf_global = (WCHAR*)::GlobalLock(wbuf_handle);
			std::memcpy(wbuf_global, wstr.c_str(), wbuf_length * sizeof(wchar_t));
			::GlobalUnlock(wbuf_handle);
			::EmptyClipboard();
			if (::SetClipboardData(CF_UNICODETEXT, wbuf_handle) == NULL)
				::GlobalFree(wbuf_handle);
			::CloseClipboard();
#endif // PLATFORM_WINDOWS_DESKTOP
		}
		
		int StringConvert(const char* from, wchar_t* to, int dest_size_in_characters)
		{
			if (!from || !to || dest_size_in_characters <= 0)
				return 0;

			const unsigned char* src = reinterpret_cast<const unsigned char*>(from);
			int written = 0;

			while (*src && written < dest_size_in_characters - 1)
			{
				uint32_t codepoint = 0;
				unsigned char c = *src;

				if (c < 0x80)
				{
					codepoint = c;
					++src;
				}
				else if ((c & 0xE0) == 0xC0)
				{
					if (!src[1])
						break;
					codepoint = ((c & 0x1F) << 6) | (src[1] & 0x3F);
					src += 2;
				}
				else if ((c & 0xF0) == 0xE0)
				{
					if (!src[1] || !src[2])
						break;
					codepoint = ((c & 0x0F) << 12) | ((src[1] & 0x3F) << 6) | (src[2] & 0x3F);
					src += 3;
				}
				else if ((c & 0xF8) == 0xF0)
				{
					if (!src[1] || !src[2] || !src[3])
						break;
					codepoint = ((c & 0x07) << 18) | ((src[1] & 0x3F) << 12) | ((src[2] & 0x3F) << 6) | (src[3] & 0x3F);
					src += 4;
				}
				else
				{
					++src;
					continue;
				}

				if constexpr (sizeof(wchar_t) >= 4)
				{
					to[written++] = static_cast<wchar_t>(codepoint);
				}
				else
				{
					if (codepoint <= 0xFFFF)
					{
						to[written++] = static_cast<wchar_t>(codepoint);
					}
					else
					{
						if (written + 1 >= dest_size_in_characters - 1)
							break;
						codepoint -= 0x10000;
						to[written++] = static_cast<wchar_t>((codepoint >> 10) + 0xD800);
						to[written++] = static_cast<wchar_t>((codepoint & 0x3FF) + 0xDC00);
					}
				}
			}

			to[written] = 0;
			return written;
		}

		
	}
}