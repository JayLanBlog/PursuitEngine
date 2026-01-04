#include "video.h"


namespace pf::video {

	bool CreateVideo(const std::string& filename, Video* video) {
		
		return true;
	}
	bool CreateVideoMP4(const uint8_t* filedata, size_t filesize, Video* video) {

		return true;
	}
	bool CreateVideoH264RAW(const uint8_t* filedata, size_t filesize, Video* video) {

		return true;
	}
	bool CreateVideoInstance(const Video* video, VideoInstance* instance) {

		return true;
	}

	void UpdateVideo(VideoInstance* instance, float dt) {
	
	}

	bool IsDecodingRequired(const VideoInstance* instance) {

		return true;
	}
	void DecodeVideo(VideoInstance* instance, graphics::CommandList cmd) {
	
	}
	void ResolveVideoToRGB(VideoInstance* instance, graphics::CommandList cmd) {
	
	}

	// Set video instance state to a timer (approximately), this will take efect the next time it is decoded
	void Seek(VideoInstance* instance, float timerSeconds) {
	
	}
}