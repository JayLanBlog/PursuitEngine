#include "renderer.h"
#include "Engine/Shaders/ShaderCompiler.h"

#include <algorithm>
#include <atomic>
#include <mutex>
#include <Module/Util/spin_lock.h>

using namespace pf::graphics;
namespace pf::renderer {


#ifdef SHADERDUMP_ENABLED
	// Note: when using Shader Dump, use relative directory, because the dump will contain relative names too
	std::string SHADERPATH = "shaders/";
	std::string SHADERSOURCEPATH = "../WickedEngine/shaders/";
#else
	// Note: when NOT using Shader Dump, use absolute directory, to avoid the case when something (eg. file dialog) overrides working directory
	std::string SHADERPATH = pf::helper::GetCurrentPath() + "/shaders/";
	std::string SHADERSOURCEPATH = SHADER_INTEROP_PATH;
#endif // SHADERDUMP_ENABLED


	GraphicsDevice*& device = GetDevice();


	std::atomic<size_t> SHADER_ERRORS{ 0 };
	std::atomic<size_t> SHADER_MISSING{ 0 };


	SpinLock deferredMIPGenLock;
	vector<std::pair<Texture, bool>> deferredMIPGens;
	vector<std::pair<Texture, Texture>> deferredBCQueue;

	bool LoadShader(
		pf::graphics::ShaderStage stage,
		pf::graphics::Shader& shader,
		const std::string& filename,
		pf::graphics::ShaderModel minshadermodel ,
		const vector<std::string>& permutation_defines 
	) {
		std::string shaderbinaryfilename = SHADERPATH + filename;

		if (!permutation_defines.empty())
		{
			std::string ext = helper::GetExtensionFromFileName(shaderbinaryfilename);
			shaderbinaryfilename = helper::RemoveExtension(shaderbinaryfilename);
			for (auto& def : permutation_defines)
			{
				shaderbinaryfilename += "_" + def;
			}
			shaderbinaryfilename += "." + ext;
		}
		if (device != nullptr)
		{
#ifdef SHADERDUMP_ENABLED
			// Loading shader from precompiled dump:
			auto it = wiShaderDump::shaderdump.find(shaderbinaryfilename);
			if (it != wiShaderDump::shaderdump.end())
			{
				wi::vector<uint8_t> decompressed;
				bool success = wi::helper::Decompress(it->second.data, it->second.size, decompressed);
				if (success)
				{
					return device->CreateShader(stage, decompressed.data(), decompressed.size(), &shader);
				}
				wi::backlog::post("shader dump decompression failure: " + shaderbinaryfilename, wi::backlog::LogLevel::Error);
			}
			else
			{
				wi::backlog::post("shader dump doesn't contain shader: " + shaderbinaryfilename, wi::backlog::LogLevel::Error);
			}
#endif // SHADERDUMP_ENABLED
		}
		
		shadercompiler::RegisterShader(shaderbinaryfilename);

		if (shadercompiler::IsShaderOutdated(shaderbinaryfilename)) {
			shadercompiler::CompilerInput input;
			input.format = device->GetShaderFormat();
			input.stage = stage;
			input.minshadermodel = minshadermodel;
			input.defines = permutation_defines;

			std::string sourcedir = SHADERSOURCEPATH;
			helper::MakePathAbsolute(sourcedir);
			input.include_directories.push_back(sourcedir);
			input.include_directories.push_back(sourcedir + helper::GetDirectoryFromPath(filename));
			input.shadersourcefilename = helper::ReplaceExtension(sourcedir + filename, "hlsl");

			shadercompiler::CompilerOutput output;
			shadercompiler::Compile(input, output);

			if (output.IsValid())
			{
				shadercompiler::SaveShaderAndMetadata(shaderbinaryfilename, output);

				if (!output.error_message.empty())
				{
					//backlog::post(output.error_message, wi::backlog::LogLevel::Warning);
				}
				//backlog::post("shader compiled: " + shaderbinaryfilename);
				return device->CreateShader(stage, output.shaderdata, output.shadersize, &shader);
			}
			else
			{
				//wi::backlog::post("shader compile FAILED: " + shaderbinaryfilename + "\n" + output.error_message, wi::backlog::LogLevel::Error);
				SHADER_ERRORS.fetch_add(1);
			}

		}

		if (device != nullptr)
		{
			vector<uint8_t> buffer;
			if (helper::FileRead(shaderbinaryfilename, buffer))
			{
				bool success = device->CreateShader(stage, buffer.data(), buffer.size(), &shader);
				if (success)
				{
					device->SetName(&shader, shaderbinaryfilename.c_str());
				}
				return success;
			}
			else
			{
				SHADER_MISSING.fetch_add(1);
			}
		}

		return false;
	}

	void AddDeferredMIPGen(const graphics::Texture& texture, bool preserve_coverage ) {
		deferredMIPGenLock.lock();
		deferredMIPGens.push_back(std::make_pair(texture, preserve_coverage));
		deferredMIPGenLock.unlock();
	}

	void AddDeferredBlockCompression(const graphics::Texture& texture_src, const graphics::Texture& texture_bc) {
		deferredMIPGenLock.lock();
		deferredBCQueue.push_back(std::make_pair(texture_src, texture_bc));
		deferredMIPGenLock.unlock();
	}
}