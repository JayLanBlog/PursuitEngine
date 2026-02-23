#pragma once
#include "Core/core_include.h"
#include "Engine/Device/graph_driver.h"
#include "Module/Container/pvector.h"
#include "Module/Container/unorderedmap.h"
#include "Module/Container/unorder_set.h"

#include <memory>
#include <string>

namespace pf::video {

	struct Video
	{
		std::string title;
		std::string album;
		std::string artist;
		std::string year;
		std::string comment;
		std::string genre;
		uint32_t padded_width = 0;
		uint32_t padded_height = 0;
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t bit_rate = 0;
		graphics::VideoProfile profile = graphics::VideoProfile::H264;
		vector<uint8_t> sps_datas;
		vector<uint8_t> pps_datas;
		vector<uint8_t> slice_header_datas;
		uint32_t sps_count = 0;
		uint32_t pps_count = 0;
		uint32_t slice_header_count = 0;
		graphics::GPUBuffer data_stream;
		float average_frames_per_second = 0;
		float duration_seconds = 0;
		struct FrameInfo
		{
			uint64_t offset = 0;
			uint64_t size = 0;
			float timestamp_seconds = 0;
			float duration_seconds = 0;
			graphics::VideoFrameType type = graphics::VideoFrameType::Intra;
			uint32_t reference_priority = 0;
			int poc = 0;
			int gop = 0;
			int display_order = 0;
		};
		vector<FrameInfo> frame_infos;
		uint32_t num_dpb_slots = 0;
		inline bool IsValid() const { return data_stream.IsValid(); }
	};

	struct VideoInstance
	{
		const Video* video = nullptr;
		graphics::VideoDecoder decoder;
		struct DPB
		{
			graphics::Texture texture; // raw decoder image array (only can be sampled when device supports coincide mode decoder)
			int subresources_luminance[17] = {};
			int subresources_chrominance[17] = {};
			int poc_status[17] = {};
			int framenum_status[17] = {};
			graphics::ResourceState resource_states[17] = {};
			vector<uint8_t> reference_usage;
			uint8_t next_ref = 0;
			uint8_t next_slot = 0;
			uint8_t current_slot = 0;
		} dpb;
		struct OutputTexture
		{
			graphics::Texture texture; // resolved RGB image
			int subresource_srgb = -1;
			int display_order = -1;

			// Below can be either point to DPB in coincide mode, or separate decoder output in non-coincide mode:
			graphics::Texture src;
			int src_subresource_luminance = -1;
			int src_subresource_chrominance = -1;
		};
		vector<OutputTexture> output_textures_free; // free images that can be reused for display order buffering
		vector<OutputTexture> output_textures_resolve_request; // request image to be resolved
		vector<OutputTexture> output_textures_used; // resolved image for future display ordering use
		OutputTexture output; // Currently displayed RGB image with the latest display order
		int target_display_order = 0; // the current display order that should be visible
		int current_decode_frame = 0; // the latest decoded frame index
		float current_time = 0; // tracking the absolute time of the playback in seconds

		enum class Flags
		{
			Empty = 0,
			Playing = 1 << 0,
			Looped = 1 << 1,
			Mipmapped = 1 << 2,
			NeedsResolve = 1 << 3,
			InitialFirstFrameDecoded = 1 << 4,
			DecoderReset = 1 << 5,
			
		};


		Flags flags = Flags::Empty;
		inline bool IsValid() const { return decoder.IsValid(); }

		// Get texture resource of the latest decoded frame that can be displayed:
		graphics::Texture GetCurrentFrameTexture() const { return output.texture; }
		// Get SRGB subresource view of the latest decoded frame that can be displayed:
		int GetCurrentFrameTextureSRGBSubresource() const { return output.subresource_srgb; }
	};
	
	

	bool CreateVideo(const std::string& filename, Video* video);
	bool CreateVideoMP4(const uint8_t* filedata, size_t filesize, Video* video);
	bool CreateVideoH264RAW(const uint8_t* filedata, size_t filesize, Video* video);
	bool CreateVideoInstance(const Video* video, VideoInstance* instance);

	void UpdateVideo(VideoInstance* instance, float dt);

	bool IsDecodingRequired(const VideoInstance* instance);
	void DecodeVideo(VideoInstance* instance, graphics::CommandList cmd);
	void ResolveVideoToRGB(VideoInstance* instance, graphics::CommandList cmd);

	// Set video instance state to a timer (approximately), this will take efect the next time it is decoded
	void Seek(VideoInstance* instance, float timerSeconds);
}
//
//
//inline pf::video::VideoInstance::Flags operator|(pf::video::VideoInstance::Flags lhs, pf::video::VideoInstance::Flags rhs) {
//	using T = std::underlying_type_t<pf::video::VideoInstance::Flags>;
//	return static_cast<pf::video::VideoInstance::Flags>(static_cast<T>(lhs) | static_cast<T>(rhs));
//}
//
//
//inline pf::video::VideoInstance::Flags operator&(pf::video::VideoInstance::Flags lhs, pf::video::VideoInstance::Flags rhs) {
//	using T = std::underlying_type_t<pf::video::VideoInstance::Flags>;
//	return static_cast<pf::video::VideoInstance::Flags>(static_cast<T>(lhs) & static_cast<T>(rhs));
//}
//
//
//inline pf::video::VideoInstance::Flags operator~(pf::video::VideoInstance::Flags rhs) {
//	using T = std::underlying_type_t<pf::video::VideoInstance::Flags>;
//	return static_cast<pf::video::VideoInstance::Flags>(~static_cast<T>(rhs));
//}
//
//
//inline pf::video::VideoInstance::Flags& operator&=(pf::video::VideoInstance::Flags& lhs, pf::video::VideoInstance::Flags rhs) {
//	lhs = lhs & rhs;
//	return lhs;
//}
//
//
//inline pf::video::VideoInstance::Flags& operator|=(pf::video::VideoInstance::Flags& lhs, pf::video::VideoInstance::Flags rhs) {
//	lhs = lhs | rhs;
//	return lhs;
//}

template<>
struct enable_bitmask_operators<pf::video::VideoInstance::Flags> {
	static const bool enable = true;
};