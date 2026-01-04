#include "audio.h"
#include "Module/Logger/backlogger.h"
#include "Module/Filer/file_helper.h"
#include "Module/Util/p_timer.h"
#include "Module/Container/pvector.h"

#include <sstream>
template<typename T>
static constexpr T AlignTo(T value, T alignment)
{
	return ((value + alignment - T(1)) / alignment) * alignment;
}
#ifdef _WIN32

#include <wrl/client.h> // ComPtr
#include <xaudio2.h>
#include <xaudio2fx.h>
#include <x3daudio.h>
#pragma comment(lib,"xaudio2.lib")

//Little-Endian things:
#define fourccRIFF 'FFIR'
#define fourccDATA 'atad'
#define fourccFMT ' tmf'
#define fourccWAVE 'EVAW'
#define fourccXWMA 'AMWX'
#define fourccDPDS 'sdpd'

#define xaudio_assert(cond, fname) { log_assert(cond, "XAudio2 error: %s failed with %s (%s:%d)", fname, helper::GetPlatformErrorString(hr).c_str(), relative_path(__FILE__), __LINE__); }
#define xaudio_check(call) [&]() { HRESULT hr = call; xaudio_assert(SUCCEEDED(hr), extract_function_name(#call).c_str()); return hr; }()


namespace pf::audio {
	static const XAUDIO2FX_REVERB_I3DL2_PARAMETERS reverbPresets[] =
	{
		XAUDIO2FX_I3DL2_PRESET_DEFAULT,
		XAUDIO2FX_I3DL2_PRESET_GENERIC,
		XAUDIO2FX_I3DL2_PRESET_FOREST,
		XAUDIO2FX_I3DL2_PRESET_PADDEDCELL,
		XAUDIO2FX_I3DL2_PRESET_ROOM,
		XAUDIO2FX_I3DL2_PRESET_BATHROOM,
		XAUDIO2FX_I3DL2_PRESET_LIVINGROOM,
		XAUDIO2FX_I3DL2_PRESET_STONEROOM,
		XAUDIO2FX_I3DL2_PRESET_AUDITORIUM,
		XAUDIO2FX_I3DL2_PRESET_CONCERTHALL,
		XAUDIO2FX_I3DL2_PRESET_CAVE,
		XAUDIO2FX_I3DL2_PRESET_ARENA,
		XAUDIO2FX_I3DL2_PRESET_HANGAR,
		XAUDIO2FX_I3DL2_PRESET_CARPETEDHALLWAY,
		XAUDIO2FX_I3DL2_PRESET_HALLWAY,
		XAUDIO2FX_I3DL2_PRESET_STONECORRIDOR,
		XAUDIO2FX_I3DL2_PRESET_ALLEY,
		XAUDIO2FX_I3DL2_PRESET_CITY,
		XAUDIO2FX_I3DL2_PRESET_MOUNTAINS,
		XAUDIO2FX_I3DL2_PRESET_QUARRY,
		XAUDIO2FX_I3DL2_PRESET_PLAIN,
		XAUDIO2FX_I3DL2_PRESET_PARKINGLOT,
		XAUDIO2FX_I3DL2_PRESET_SEWERPIPE,
		XAUDIO2FX_I3DL2_PRESET_UNDERWATER,
		XAUDIO2FX_I3DL2_PRESET_SMALLROOM,
		XAUDIO2FX_I3DL2_PRESET_MEDIUMROOM,
		XAUDIO2FX_I3DL2_PRESET_LARGEROOM,
		XAUDIO2FX_I3DL2_PRESET_MEDIUMHALL,
		XAUDIO2FX_I3DL2_PRESET_LARGEHALL,
		XAUDIO2FX_I3DL2_PRESET_PLATE,
	};

	struct AudioInternal {
		bool success = false;
		Microsoft::WRL::ComPtr<IXAudio2> audioEngine;
		IXAudio2MasteringVoice* masteringVoice = nullptr;
		XAUDIO2_VOICE_DETAILS masteringVoiceDetails = {};
		IXAudio2SubmixVoice* submixVoices[SUBMIX_TYPE_COUNT] = {};
		X3DAUDIO_HANDLE audio3D = {};
		Microsoft::WRL::ComPtr<IUnknown> reverbEffect;
		IXAudio2SubmixVoice* reverbSubmix = nullptr;
		uint32_t termination_data = 0;
		XAUDIO2_BUFFER termination_mark = {};
		AudioInternal()
		{
			Timer timer;

			HRESULT hr;
			hr = xaudio_check(CoInitializeEx(NULL, COINIT_MULTITHREADED));
			if (!SUCCEEDED(hr))
			{
				return;
			}

			hr = xaudio_check(XAudio2Create(&audioEngine, 0, XAUDIO2_USE_DEFAULT_PROCESSOR));
			if (!SUCCEEDED(hr))
			{
				return;
			}

#ifdef _DEBUG
			XAUDIO2_DEBUG_CONFIGURATION debugConfig = {};
			debugConfig.TraceMask = XAUDIO2_LOG_ERRORS | XAUDIO2_LOG_WARNINGS;
			debugConfig.BreakMask = XAUDIO2_LOG_ERRORS | XAUDIO2_LOG_WARNINGS;
			audioEngine->SetDebugConfiguration(&debugConfig);
#endif // _DEBUG

			hr = xaudio_check(audioEngine->CreateMasteringVoice(&masteringVoice));
			if (!SUCCEEDED(hr))
			{
				return;
			}

			masteringVoice->GetVoiceDetails(&masteringVoiceDetails);

			// Without clamping sample rate, it was crashing 32bit 192kHz audio devices
			if (masteringVoiceDetails.InputSampleRate > 48000)
				masteringVoiceDetails.InputSampleRate = 48000;

			for (int i = 0; i < SUBMIX_TYPE_COUNT; ++i)
			{
				hr = xaudio_check(audioEngine->CreateSubmixVoice(
					&submixVoices[i],
					masteringVoiceDetails.InputChannels,
					masteringVoiceDetails.InputSampleRate,
					0,
					0,
					0,
					0
				));

				if (!SUCCEEDED(hr))
				{
					return;
				}
			}

			DWORD channelMask;
			masteringVoice->GetChannelMask(&channelMask);
			hr = xaudio_check(X3DAudioInitialize(channelMask, X3DAUDIO_SPEED_OF_SOUND, audio3D));
			if (!SUCCEEDED(hr))
			{
				return;
			}

			// Reverb setup:
			{
				hr = xaudio_check(XAudio2CreateReverb(&reverbEffect));
				if (!SUCCEEDED(hr))
				{
					return;
				}

				XAUDIO2_EFFECT_DESCRIPTOR effects[] = { { reverbEffect.Get(), TRUE, 1 } };
				XAUDIO2_EFFECT_CHAIN effectChain = { arraysize(effects), effects };
				hr = xaudio_check(audioEngine->CreateSubmixVoice(
					&reverbSubmix,
					1, // reverb is mono
					masteringVoiceDetails.InputSampleRate,
					0,
					0,
					nullptr,
					&effectChain
				));
				if (!SUCCEEDED(hr))
				{
					return;
				}

				XAUDIO2FX_REVERB_PARAMETERS native;
				ReverbConvertI3DL2ToNative(&reverbPresets[REVERB_PRESET_DEFAULT], &native);
				HRESULT hr = xaudio_check(reverbSubmix->SetEffectParameters(0, &native, sizeof(native)));
				if (!SUCCEEDED(hr))
				{
					return;
				}
			}

			termination_mark.Flags = XAUDIO2_END_OF_STREAM;
			termination_mark.pAudioData = (const BYTE*)&termination_data;
			termination_mark.AudioBytes = sizeof(termination_data);

			success = true;
			log("wi::audio Initialized [XAudio2] (%d ms)", (int)std::round(timer.elapsed()));
		}

		~AudioInternal()
		{
			if (reverbSubmix != nullptr)
				reverbSubmix->DestroyVoice();

			for (int i = 0; i < SUBMIX_TYPE_COUNT; ++i)
			{
				if (submixVoices[i] != nullptr)
					submixVoices[i]->DestroyVoice();
			}

			if (masteringVoice != nullptr)
				masteringVoice->DestroyVoice();

			audioEngine->StopEngine();

			CoUninitialize();
		}

		constexpr bool IsValid() const { return success; }
	};

	static std::shared_ptr<AudioInternal> audio_internal;

	void Initialize()
	{
		audio_internal = std::make_shared<AudioInternal>();
	}

	struct SoundInternal
	{
		std::shared_ptr<AudioInternal> audio;
		WAVEFORMATEX wfx = {};
		pf::vector<uint8_t> audioData;
	};

	struct SoundInstanceInternal : public IXAudio2VoiceCallback
	{
		std::shared_ptr<AudioInternal> audio;
		std::shared_ptr<SoundInternal> soundinternal;
		IXAudio2SourceVoice* sourceVoice = nullptr;
		XAUDIO2_VOICE_DETAILS voiceDetails = {};
		vector<float> outputMatrix;
		vector<float> channelAzimuths;
		XAUDIO2_BUFFER buffer = {};
		bool ended = true;

		~SoundInstanceInternal()
		{
			sourceVoice->Stop();
			sourceVoice->DestroyVoice();
		}

		// Called just before this voice's processing pass begins.
		STDMETHOD_(void, OnVoiceProcessingPassStart) (THIS_ UINT32 BytesRequired)
		{
		}

		// Called just after this voice's processing pass ends.
		STDMETHOD_(void, OnVoiceProcessingPassEnd) (THIS)
		{
		}

		// Called when this voice has just finished playing a buffer stream
		// (as marked with the XAUDIO2_END_OF_STREAM flag on the last buffer).
		STDMETHOD_(void, OnStreamEnd) (THIS)
		{
			ended = true;
		}

		// Called when this voice is about to start processing a new buffer.
		STDMETHOD_(void, OnBufferStart) (THIS_ void* pBufferContext)
		{
			ended = false;
		}

		// Called when this voice has just finished processing a buffer.
		// The buffer can now be reused or destroyed.
		STDMETHOD_(void, OnBufferEnd) (THIS_ void* pBufferContext)
		{
		}

		// Called when this voice has just reached the end position of a loop.
		STDMETHOD_(void, OnLoopEnd) (THIS_ void* pBufferContext)
		{
		}

		// Called in the event of a critical error during voice processing,
		// such as a failing xAPO or an error from the hardware XMA decoder.
		// The voice may have to be destroyed and re-created to recover from
		// the error.  The callback arguments report which buffer was being
		// processed when the error occurred, and its HRESULT code.
		STDMETHOD_(void, OnVoiceError) (THIS_ void* pBufferContext, HRESULT Error)
		{
		}
	};
	SoundInternal* to_internal(const Sound* param)
	{
		return static_cast<SoundInternal*>(param->internal_state.get());
	}
	SoundInstanceInternal* to_internal(const SoundInstance* param)
	{
		return static_cast<SoundInstanceInternal*>(param->internal_state.get());
	}

	bool FindChunk(const uint8_t* data, DWORD fourcc, DWORD& dwChunkSize, DWORD& dwChunkDataPosition)
	{
		size_t pos = 0;

		DWORD dwChunkType;
		DWORD dwChunkDataSize;
		DWORD dwRIFFDataSize = 0;
		DWORD dwFileType;
		DWORD bytesRead = 0;
		DWORD dwOffset = 0;

		while (true)
		{
			memcpy(&dwChunkType, data + pos, sizeof(DWORD));
			pos += sizeof(DWORD);
			memcpy(&dwChunkDataSize, data + pos, sizeof(DWORD));
			pos += sizeof(DWORD);

			switch (dwChunkType)
			{
			case fourccRIFF:
				dwRIFFDataSize = dwChunkDataSize;
				dwChunkDataSize = 4;
				memcpy(&dwFileType, data + pos, sizeof(DWORD));
				pos += sizeof(DWORD);
				break;

			default:
				pos += dwChunkDataSize;
			}

			dwOffset += sizeof(DWORD) * 2;

			if (dwChunkType == fourcc)
			{
				dwChunkSize = dwChunkDataSize;
				dwChunkDataPosition = dwOffset;
				return true;
			}

			dwOffset += dwChunkDataSize;

			if (bytesRead >= dwRIFFDataSize) return false;

		}

		return true;

	}

	bool CreateSound(const std::string& filename, Sound* sound)
	{
		vector<uint8_t> filedata;
		bool success = helper::FileRead(filename, filedata);
		if (!success)
		{
			return false;
		}
		return CreateSound(filedata.data(), filedata.size(), sound);
	}
	bool CreateSound(const uint8_t* data, size_t size, Sound* sound)
	{
		if (audio_internal == nullptr || !audio_internal->IsValid())
			return false;
		std::shared_ptr<SoundInternal> soundinternal = std::make_shared<SoundInternal>();
		soundinternal->audio = audio_internal;
		sound->internal_state = soundinternal;

		DWORD dwChunkSize;
		DWORD dwChunkPosition;

		bool success;

		success = FindChunk(data, fourccRIFF, dwChunkSize, dwChunkPosition);
		if (success)
		{
			// Wav decoder:
			DWORD filetype;
			memcpy(&filetype, data + dwChunkPosition, sizeof(DWORD));
			if (filetype != fourccWAVE)
			{
				assert(0);
				return false;
			}

			success = FindChunk(data, fourccFMT, dwChunkSize, dwChunkPosition);
			if (!success)
			{
				assert(0);
				return false;
			}
			memcpy(&soundinternal->wfx, data + dwChunkPosition, dwChunkSize);
			soundinternal->wfx.wFormatTag = WAVE_FORMAT_PCM;

			success = FindChunk(data, fourccDATA, dwChunkSize, dwChunkPosition);
			if (!success)
			{
				assert(0);
				return false;
			}

			soundinternal->audioData.resize(dwChunkSize);
			memcpy(soundinternal->audioData.data(), data + dwChunkPosition, dwChunkSize);
		}
		else
		{
			// Ogg decoder:
			int channels = 0;
			int sample_rate = 0;
			short* output = nullptr;
			//TO DO : int samples = stb_vorbis_decode_memory(data, (int)size, &channels, &sample_rate, &output);
			int samples = 0;
			if (samples < 0)
			{
				assert(0);
				return false;
			}

			// WAVEFORMATEX: https://docs.microsoft.com/en-us/previous-versions/dd757713(v=vs.85)?redirectedfrom=MSDN
			soundinternal->wfx.wFormatTag = WAVE_FORMAT_PCM;
			soundinternal->wfx.nChannels = (WORD)channels;
			soundinternal->wfx.nSamplesPerSec = (DWORD)sample_rate;
			soundinternal->wfx.wBitsPerSample = sizeof(short) * 8;
			soundinternal->wfx.nBlockAlign = (WORD)channels * sizeof(short); // is this right?
			soundinternal->wfx.nAvgBytesPerSec = soundinternal->wfx.nSamplesPerSec * soundinternal->wfx.nBlockAlign;

			size_t output_size = size_t(samples * channels) * sizeof(short);
			soundinternal->audioData.resize(output_size);
			memcpy(soundinternal->audioData.data(), output, output_size);

			free(output);
		}

		return true;
	}
	bool CreateSoundInstance(const Sound* sound, SoundInstance* instance)
	{
		if (audio_internal == nullptr || !audio_internal->IsValid())
			return false;
		if (sound == nullptr || !sound->IsValid())
			return false;
		HRESULT hr;
		const auto& soundinternal = std::static_pointer_cast<SoundInternal>(sound->internal_state);
		std::shared_ptr<SoundInstanceInternal> instanceinternal = std::make_shared<SoundInstanceInternal>();
		instance->internal_state = instanceinternal;

		instanceinternal->audio = audio_internal;
		instanceinternal->soundinternal = soundinternal;

		XAUDIO2_SEND_DESCRIPTOR SFXSend[] = {
			{ XAUDIO2_SEND_USEFILTER, instanceinternal->audio->submixVoices[instance->type] },
			{ XAUDIO2_SEND_USEFILTER, instanceinternal->audio->reverbSubmix }, // this should be last to enable/disable reverb simply
		};
		XAUDIO2_VOICE_SENDS SFXSendList = {
			(instance->IsEnableReverb() && instanceinternal->audio->reverbSubmix != nullptr) ? (uint32_t)arraysize(SFXSend) : 1,
			SFXSend
		};

		hr = xaudio_check(instanceinternal->audio->audioEngine->CreateSourceVoice(&instanceinternal->sourceVoice, &soundinternal->wfx,
			0, XAUDIO2_DEFAULT_FREQ_RATIO, instanceinternal.get(), &SFXSendList, NULL));

		if (FAILED(hr))
		{
			return false;
		}

		instanceinternal->sourceVoice->GetVoiceDetails(&instanceinternal->voiceDetails);

		instanceinternal->outputMatrix.resize(size_t(instanceinternal->voiceDetails.InputChannels) * size_t(instanceinternal->audio->masteringVoiceDetails.InputChannels));
		instanceinternal->channelAzimuths.resize(instanceinternal->voiceDetails.InputChannels);
		for (size_t i = 0; i < instanceinternal->channelAzimuths.size(); ++i)
		{
			instanceinternal->channelAzimuths[i] = X3DAUDIO_2PI * float(i) / float(instanceinternal->channelAzimuths.size());
		}

		const uint32_t bytes_per_second = soundinternal->wfx.nSamplesPerSec * soundinternal->wfx.nChannels * sizeof(short);
		instanceinternal->buffer.pAudioData = soundinternal->audioData.data();
		instanceinternal->buffer.AudioBytes = (uint32_t)soundinternal->audioData.size();
		if (instance->begin > 0)
		{
			const uint32_t bytes_from_beginning = AlignTo(std::min(instanceinternal->buffer.AudioBytes, uint32_t(instance->begin * bytes_per_second)), 4u);
			instanceinternal->buffer.pAudioData += bytes_from_beginning;
			instanceinternal->buffer.AudioBytes -= bytes_from_beginning;
		}
		if (instance->length > 0)
		{
			instanceinternal->buffer.AudioBytes = AlignTo(std::min(instanceinternal->buffer.AudioBytes, uint32_t(instance->length * bytes_per_second)), 4u);
		}

		uint32_t num_remaining_samples = instanceinternal->buffer.AudioBytes / (soundinternal->wfx.nChannels * sizeof(short));
		if (instance->loop_begin > 0)
		{
			instanceinternal->buffer.LoopBegin = AlignTo(std::min(num_remaining_samples, uint32_t(instance->loop_begin * soundinternal->wfx.nSamplesPerSec)), 4u);
			num_remaining_samples -= instanceinternal->buffer.LoopBegin;
		}
		instanceinternal->buffer.LoopLength = AlignTo(std::min(num_remaining_samples, uint32_t(instance->loop_length * soundinternal->wfx.nSamplesPerSec)), 4u);

		instanceinternal->buffer.Flags = XAUDIO2_END_OF_STREAM;
		instanceinternal->buffer.LoopCount = instance->IsLooped() ? XAUDIO2_LOOP_INFINITE : 0;

		hr = xaudio_check(instanceinternal->sourceVoice->SubmitSourceBuffer(&instanceinternal->buffer));

		if (FAILED(hr))
		{
			return false;
		}

		return true;
	}
	void Play(SoundInstance* instance)
	{
		if (instance != nullptr && instance->IsValid())
		{
			auto instanceinternal = to_internal(instance);
			xaudio_check(instanceinternal->sourceVoice->Start());

		}
	}
	void Pause(SoundInstance* instance)
	{
		if (instance != nullptr && instance->IsValid())
		{
			auto instanceinternal = to_internal(instance);
			xaudio_check(instanceinternal->sourceVoice->Stop()); // preserves cursor position

		}
	}
	void Stop(SoundInstance* instance)
	{
		if (instance != nullptr && instance->IsValid())
		{
			auto instanceinternal = to_internal(instance);
			xaudio_check(instanceinternal->sourceVoice->Stop()); // preserves cursor position

			xaudio_check(instanceinternal->sourceVoice->FlushSourceBuffers()); // reset submitted audio buffer

			if (!instanceinternal->ended) // if already ended, don't submit end again, it can cause high pitched jerky sound
			{
				xaudio_check(instanceinternal->sourceVoice->SubmitSourceBuffer(&audio_internal->termination_mark)); // mark this as terminated, this resets XAUDIO2_VOICE_STATE::SamplesPlayed to zero

			}
			xaudio_check(instanceinternal->sourceVoice->SubmitSourceBuffer(&instanceinternal->buffer)); // resubmit

		}
	}
	void SetVolume(float volume, SoundInstance* instance)
	{
		if (instance == nullptr || !instance->IsValid())
		{
			if (audio_internal->masteringVoice != nullptr)
				xaudio_check(audio_internal->masteringVoice->SetVolume(volume));
		}
		else
		{
			auto instanceinternal = to_internal(instance);
			if (instanceinternal->sourceVoice != nullptr)
				xaudio_check(instanceinternal->sourceVoice->SetVolume(volume));
		}
	}
	float GetVolume(const SoundInstance* instance)
	{
		float volume = 0;
		if (instance == nullptr || !instance->IsValid())
		{
			if (audio_internal->masteringVoice != nullptr)
				audio_internal->masteringVoice->GetVolume(&volume);
		}
		else
		{
			auto instanceinternal = to_internal(instance);
			if (instanceinternal->sourceVoice != nullptr)
				instanceinternal->sourceVoice->GetVolume(&volume);
		}
		return volume;
	}
	void ExitLoop(SoundInstance* instance)
	{
		if (instance != nullptr && instance->IsValid())
		{
			auto instanceinternal = to_internal(instance);
			if (instanceinternal->buffer.LoopCount == 0)
				return;
			xaudio_check(instanceinternal->sourceVoice->ExitLoop());

			if (instanceinternal->ended)
			{
				instanceinternal->buffer.LoopCount = 0;
				xaudio_check(instanceinternal->sourceVoice->SubmitSourceBuffer(&instanceinternal->buffer));
			}
		}
	}
	bool IsEnded(SoundInstance* instance)
	{
		if (instance != nullptr && instance->IsValid())
		{
			auto instanceinternal = to_internal(instance);
			return instanceinternal->ended;
		}
		return false;
	}

	SampleInfo GetSampleInfo(const Sound* sound)
	{
		SampleInfo info = {};
		if (sound != nullptr && sound->IsValid())
		{
			auto soundinternal = to_internal(sound);
			info.channel_count = soundinternal->wfx.nChannels;
			info.samples = (const short*)soundinternal->audioData.data();
			info.sample_count = soundinternal->audioData.size() / (info.channel_count * sizeof(short));
			info.sample_rate = soundinternal->wfx.nSamplesPerSec;
		}
		return info;
	}
	uint64_t GetTotalSamplesPlayed(const SoundInstance* instance)
	{
		if (instance != nullptr && instance->IsValid())
		{
			auto instanceinternal = to_internal(instance);
			XAUDIO2_VOICE_STATE state = {};
			if (instanceinternal->sourceVoice != nullptr)
				instanceinternal->sourceVoice->GetState(&state, 0);
			return state.SamplesPlayed;
		}
		return 0ull;
	}

	void SetSubmixVolume(SUBMIX_TYPE type, float volume)
	{
		if (audio_internal->submixVoices[type] != nullptr)
			xaudio_check(audio_internal->submixVoices[type]->SetVolume(volume));
	}
	float GetSubmixVolume(SUBMIX_TYPE type)
	{
		float volume;
		audio_internal->submixVoices[type]->GetVolume(&volume);
		return volume;
	}

	void Update3D(SoundInstance* instance, const SoundInstance3D& instance3D)
	{
		if (instance != nullptr && instance->IsValid())
		{
			auto instanceinternal = to_internal(instance);

			X3DAUDIO_LISTENER listener = {};
			/*
			TO DO:
			listener.Position = instance3D.listenerPos;
			listener.OrientFront = instance3D.listenerFront;
			listener.OrientTop = instance3D.listenerUp;
			listener.Velocity = instance3D.listenerVelocity;
			*/

			X3DAUDIO_EMITTER emitter = {};
			/*
			TO DO:
			emitter.Position = instance3D.emitterPos;
			emitter.OrientFront = instance3D.emitterFront;
			emitter.OrientTop = instance3D.emitterUp;
			emitter.Velocity = instance3D.emitterVelocity;
			emitter.InnerRadius = instance3D.emitterRadius;
			emitter.InnerRadiusAngle = X3DAUDIO_PI / 4.0f;
			emitter.ChannelCount = instanceinternal->voiceDetails.InputChannels;
			emitter.pChannelAzimuths = instanceinternal->channelAzimuths.data();
			emitter.ChannelRadius = 0.1f;
			emitter.CurveDistanceScaler = 1;
			emitter.DopplerScaler = 1;
			*/

			UINT32 flags = 0;
			flags |= X3DAUDIO_CALCULATE_MATRIX;
			flags |= X3DAUDIO_CALCULATE_LPF_DIRECT;
			flags |= X3DAUDIO_CALCULATE_REVERB;
			flags |= X3DAUDIO_CALCULATE_LPF_REVERB;
			flags |= X3DAUDIO_CALCULATE_DOPPLER;
			//flags |= X3DAUDIO_CALCULATE_DELAY;
			//flags |= X3DAUDIO_CALCULATE_EMITTER_ANGLE;
			//flags |= X3DAUDIO_CALCULATE_ZEROCENTER;
			//flags |= X3DAUDIO_CALCULATE_REDIRECT_TO_LFE;

			X3DAUDIO_DSP_SETTINGS settings = {};
			settings.SrcChannelCount = instanceinternal->voiceDetails.InputChannels;
			settings.DstChannelCount = instanceinternal->audio->masteringVoiceDetails.InputChannels;
			settings.pMatrixCoefficients = instanceinternal->outputMatrix.data();

			X3DAudioCalculate(instanceinternal->audio->audio3D, &listener, &emitter, flags, &settings);

			xaudio_check(instanceinternal->sourceVoice->SetFrequencyRatio(settings.DopplerFactor));

			xaudio_check(instanceinternal->sourceVoice->SetOutputMatrix(
				instanceinternal->audio->submixVoices[instance->type],
				settings.SrcChannelCount,
				settings.DstChannelCount,
				settings.pMatrixCoefficients
			));


			XAUDIO2_FILTER_PARAMETERS FilterParametersDirect = { LowPassFilter, 2.0f * sinf(X3DAUDIO_PI / 6.0f * settings.LPFDirectCoefficient), 1.0f };
			xaudio_check(instanceinternal->sourceVoice->SetOutputFilterParameters(instanceinternal->audio->submixVoices[instance->type], &FilterParametersDirect));


			if (instance->IsEnableReverb() && instanceinternal->audio->reverbSubmix != nullptr)
			{
				xaudio_check(instanceinternal->sourceVoice->SetOutputMatrix(instanceinternal->audio->reverbSubmix, settings.SrcChannelCount, 1, &settings.ReverbLevel));

				XAUDIO2_FILTER_PARAMETERS FilterParametersReverb = { LowPassFilter, 2.0f * sinf(X3DAUDIO_PI / 6.0f * settings.LPFReverbCoefficient), 1.0f };
				xaudio_check(instanceinternal->sourceVoice->SetOutputFilterParameters(instanceinternal->audio->reverbSubmix, &FilterParametersReverb));

			}
		}
	}

	void SetReverb(REVERB_PRESET preset)
	{
		XAUDIO2FX_REVERB_PARAMETERS native;
		ReverbConvertI3DL2ToNative(&reverbPresets[preset], &native);
		xaudio_check(audio_internal->reverbSubmix->SetEffectParameters(0, &native, sizeof(native)));

	}
}

#endif