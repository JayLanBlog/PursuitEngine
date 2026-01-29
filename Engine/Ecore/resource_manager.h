#pragma once
#include "audio.h"
#include "archive.h"	
#include "Core/core_include.h"
#include "Engine/Device/graph_driver.h"

#include "Module/Util/job_system.h"
#include "video.h"

#include <memory>
#include <string>



namespace pf
{
	// This can hold an asset
	//	It can be loaded from file or memory using pf::resourcemanager::Load()
	struct Resource
	{
		std::shared_ptr<void> internal_state;
		inline bool IsValid() const { return internal_state.get() != nullptr; }
		const vector<uint8_t>& GetFileData() const;
		const graphics::Texture& GetTexture() const;
		const audio::Sound& GetSound() const;
		const std::string& GetScript() const;
		size_t GetScriptHash() const;
		const video::Video& GetVideo() const;
		int GetTextureSRGBSubresource() const;
		int GetFontStyle() const;

		void SetFileData(const vector<uint8_t>& data);
		void SetFileData(vector<uint8_t>&& data);
		// Allows to set a Texture to the resource from outside
		//	srgb_subresource: you can provide a subresource for SRGB view if the texture is going to be used as SRGB with the GetTextureSRGBSubresource() (optional)
		void SetTexture(const graphics::Texture& texture, int srgb_subresource = -1);
		void SetTextureVirtual(const graphics::GPUBuffer& tile_pool, const graphics::Texture& residency, const graphics::Texture& feedback); // for virtual texture, bind additional resources to have same lifetime
		void SetSound(const audio::Sound& sound);
		void SetScript(const std::string& script);
		void SetVideo(const video::Video& script);

		// Resource marked for recreate on resourcemanager::Load()
		void SetOutdated();

		// Let the streaming system know the required resolution of this resource
		void StreamingRequestResolution(uint32_t resolution);
	};

	namespace resourcemanager
	{
		enum class Mode
		{
			NO_EMBEDDING,		// default behaviour, serialization will not embed resource file datas
			EMBED_FILE_DATA,	// serialization will embed file datas if possible

			// legacy modes:
			DISCARD_FILEDATA_AFTER_LOAD = NO_EMBEDDING,					// default behaviour: file data will be discarded after loaded. This will not allow serialization of embedded resources, but less memory will be used overall
			ALLOW_RETAIN_FILEDATA = EMBED_FILE_DATA,					// allows keeping the file datas around. This mode allows serialization of embedded resources
			ALLOW_RETAIN_FILEDATA_BUT_DISABLE_EMBEDDING = NO_EMBEDDING	// allows keeping file datas, but they won't be embedded by serializer
		};
		void SetMode(Mode param);
		Mode GetMode();
		vector<std::string> GetSupportedImageExtensions();
		vector<std::string> GetSupportedSoundExtensions();
		vector<std::string> GetSupportedVideoExtensions();
		vector<std::string> GetSupportedScriptExtensions();
		vector<std::string> GetSupportedFontStyleExtensions();

		// Order of these must not change as the flags can be serialized!
		enum class Flags
		{
			NONE = 0,
			IMPORT_COLORGRADINGLUT = 1 << 0, // image import will convert resource to 3D color grading LUT
			IMPORT_RETAIN_FILEDATA = 1 << 1, // file data of the resource will be kept in memory, you will be able to use Resource::GetFileData()
			IMPORT_NORMALMAP = 1 << 2, // image import will try to use optimal normal map encoding
			IMPORT_BLOCK_COMPRESSED = 1 << 3, // image import will request block compression for uncompressed or transcodable formats
			IMPORT_DELAY = 1 << 4, // delay importing resource until later, for example when proper flags can be determined.
			STREAMING = 1 << 5, // use streaming if possible
		};

		// Load a resource
		//	name : file name of resource
		//	flags : specify flags that modify behaviour (optional)
		//	filedata : pointer to file data, if file was loaded manually (optional)
		//	filesize : size of file data, if file was loaded manually (optional)
		//	container_filename : if name is not the name of source file, set the source file name here
		//	container_fileoffset : if using container_filename, you can give the offset for the resource within the file here
		Resource Load(
			const std::string& name,
			Flags flags = Flags::NONE,
			const uint8_t* filedata = nullptr,
			size_t filesize = ~0ull,
			const std::string& container_filename = "",
			size_t container_fileoffset = 0
		);
		//// Check if a resource is currently loaded
		//bool Contains(const std::string& name);
		//// Invalidate all resources
		//void Clear();

		//// Set threshold relative to memory budget for streaming
		////	If memory usage is below threshold, streaming will work regularly
		////	If memory usage is above threshold, streaming will try to reduce usage
		//void SetStreamingMemoryThreshold(float value);
		//float GetStreamingMemoryThreshold();

		//// Update all streaming resources, call it once per frame on the main thread
		////	Launching or finalizing background streaming jobs is attempted here
		//void UpdateStreamingResources(float dt);

		//// Returns true if any of the loaded resources are outdated compared to their files
		//bool CheckResourcesOutdated();

		//// Reload all resources that are outdated
		//void ReloadOutdatedResources();

		struct ResourceSerializer
		{
			vector<Resource> resources;
		};

		//void Serialize_READ(Archive& archive, ResourceSerializer& resources);
		//void Serialize_WRITE(Archive& archive, const unordered_set<std::string>& resource_names);
	}

}

template<>
struct enable_bitmask_operators<pf::resourcemanager::Flags> {
	static const bool enable = true;
};
