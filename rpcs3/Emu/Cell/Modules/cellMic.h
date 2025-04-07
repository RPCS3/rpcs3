#pragma once

#include "Utilities/Thread.h"
#include "3rdparty/OpenAL/openal-soft/include/AL/alc.h"
#include "Utilities/mutex.h"

// Error Codes
enum CellMicInError : u32
{
	CELL_MICIN_ERROR_ALREADY_INIT       = 0x80140101,
	CELL_MICIN_ERROR_DEVICE             = 0x80140102,
	CELL_MICIN_ERROR_NOT_INIT           = 0x80140103,
	CELL_MICIN_ERROR_PARAM              = 0x80140104,
	CELL_MICIN_ERROR_PORT_FULL          = 0x80140105,
	CELL_MICIN_ERROR_ALREADY_OPEN       = 0x80140106,
	CELL_MICIN_ERROR_NOT_OPEN           = 0x80140107,
	CELL_MICIN_ERROR_NOT_RUN            = 0x80140108,
	CELL_MICIN_ERROR_TRANS_EVENT        = 0x80140109,
	CELL_MICIN_ERROR_OPEN               = 0x8014010a,
	CELL_MICIN_ERROR_SHAREDMEMORY       = 0x8014010b,
	CELL_MICIN_ERROR_MUTEX              = 0x8014010c,
	CELL_MICIN_ERROR_EVENT_QUEUE        = 0x8014010d,
	CELL_MICIN_ERROR_DEVICE_NOT_FOUND   = 0x8014010e,
	CELL_MICIN_ERROR_FATAL              = 0x8014010f,
	CELL_MICIN_ERROR_DEVICE_NOT_SUPPORT = 0x80140110,
	// CELL_MICIN_ERROR_SYSTEM             = CELL_MICIN_ERROR_DEVICE,
	// CELL_MICIN_ERROR_SYSTEM_NOT_FOUND   = CELL_MICIN_ERROR_DEVICE_NOT_FOUND,
	// CELL_MICIN_ERROR_SYSTEM_NOT_SUPPORT = CELL_MICIN_ERROR_DEVICE_NOT_SUPPORT
};

enum CellMicInErrorDsp : u32
{
	CELL_MICIN_ERROR_DSP              = 0x80140200,
	CELL_MICIN_ERROR_DSP_ASSERT       = 0x80140201,
	CELL_MICIN_ERROR_DSP_PATH         = 0x80140202,
	CELL_MICIN_ERROR_DSP_FILE         = 0x80140203,
	CELL_MICIN_ERROR_DSP_PARAM        = 0x80140204,
	CELL_MICIN_ERROR_DSP_MEMALLOC     = 0x80140205,
	CELL_MICIN_ERROR_DSP_POINTER      = 0x80140206,
	CELL_MICIN_ERROR_DSP_FUNC         = 0x80140207,
	CELL_MICIN_ERROR_DSP_MEM          = 0x80140208,
	CELL_MICIN_ERROR_DSP_ALIGN16      = 0x80140209,
	CELL_MICIN_ERROR_DSP_ALIGN128     = 0x8014020a,
	CELL_MICIN_ERROR_DSP_EAALIGN128   = 0x8014020b,
	CELL_MICIN_ERROR_DSP_LIB_HANDLER  = 0x80140216,
	CELL_MICIN_ERROR_DSP_LIB_INPARAM  = 0x80140217,
	CELL_MICIN_ERROR_DSP_LIB_NOSPU    = 0x80140218,
	CELL_MICIN_ERROR_DSP_LIB_SAMPRATE = 0x80140219,
};

enum CellMicSignalState : u32
{
	CELLMIC_SIGSTATE_LOCTALK = 0,
	CELLMIC_SIGSTATE_FARTALK = 1,
	CELLMIC_SIGSTATE_NSR     = 3,
	CELLMIC_SIGSTATE_AGC     = 4,
	CELLMIC_SIGSTATE_MICENG  = 5,
	CELLMIC_SIGSTATE_SPKENG  = 6,
};

enum CellMicCommand
{
	CELLMIC_INIT = 0,
	CELLMIC_END,
	CELLMIC_ATTACH,
	CELLMIC_DETACH,
	CELLMIC_SWITCH,
	CELLMIC_DATA,
	CELLMIC_OPEN,
	CELLMIC_CLOSE,
	CELLMIC_START,
	CELLMIC_STOP,
	CELLMIC_QUERY,
	CELLMIC_CONFIG,
	CELLMIC_CALLBACK,
	CELLMIC_RESET,
	CELLMIC_STATUS,
	CELLMIC_IPC,
	CELLMIC_CALLBACK2,
	CELLMIC_WEAK,
	CELLMIC_INIT2,
};

enum CellMicDeviceAttr : u32
{
	CELLMIC_DEVATTR_LED     = 9,
	CELLMIC_DEVATTR_GAIN    = 10,
	CELLMIC_DEVATTR_VOLUME  = 201,
	CELLMIC_DEVATTR_AGC     = 202,
	CELLMIC_DEVATTR_CHANVOL = 301,
	CELLMIC_DEVATTR_DSPTYPE = 302,
};

enum CellMicSignalAttr : u32
{
	CELLMIC_SIGATTR_BKNGAIN    = 0,
	CELLMIC_SIGATTR_REVERB     = 9,
	CELLMIC_SIGATTR_AGCLEVEL   = 26,
	CELLMIC_SIGATTR_VOLUME     = 301,
	CELLMIC_SIGATTR_PITCHSHIFT = 331
};

enum CellMicSignalType : u8
{
	CELLMIC_SIGTYPE_NULL = 0,
	CELLMIC_SIGTYPE_DSP  = 1,
	CELLMIC_SIGTYPE_AUX  = 2,
	CELLMIC_SIGTYPE_RAW  = 4,
};

enum CellMicType : s32
{
	CELLMIC_TYPE_UNDEF     = -1,
	CELLMIC_TYPE_UNKNOWN   = 0,
	CELLMIC_TYPE_EYETOY1   = 1,
	CELLMIC_TYPE_EYETOY2   = 2,
	CELLMIC_TYPE_USBAUDIO  = 3,
	CELLMIC_TYPE_BLUETOOTH = 4,
	CELLMIC_TYPE_A2DP      = 5,
};

enum
{
	CELL_MAX_MICS = 8,
	MAX_MICS_PERMISSABLE = 4,

	NullDeviceID = -1,

	CELL_MIC_STARTFLAG_LATENCY_4 = 0x00000001,
	CELL_MIC_STARTFLAG_LATENCY_2 = 0x00000002,
	CELL_MIC_STARTFLAG_LATENCY_1 = 0x00000003,
};

enum : u64
{
	SYSMICIN_KEYBASE             = 0x8000CA7211071000ULL,
	EQUEUE_KEY_MICIN_ACCESSPOINT = 0x8000CA7211072abcULL,
	LIBMIC_KEYBASE               = 0x8000000000000100ULL,
};

struct CellMicInputFormatI
{
	u8 channelNum;
	u8 subframeSize;
	u8 bitResolution;
	u8 dataType;
	be_t<u32> sampleRate;
};

struct CellMicInputStream
{
	be_t<u32> uiBufferBottom;
	be_t<u32> uiBufferSize;
	be_t<u32> uiBuffer;
};

struct CellMicInputDefinition
{
	be_t<u32> uiDevId;
	CellMicInputStream data;
	CellMicInputFormatI aux_format;
	CellMicInputFormatI raw_format;
	CellMicInputFormatI sig_format;
};

struct CellMicStatus
{
	be_t<s32> raw_samprate;
	be_t<s32> dsp_samprate;
	be_t<s32> dsp_volume;
	be_t<s32> isStart;
	be_t<s32> isOpen;
	be_t<s32> local_voice;
	be_t<s32> remote_voice;
	be_t<f32> mic_energy;
	be_t<f32> spk_energy;
};


// --- End of cell definitions ---


template <usz Size>
class simple_ringbuf
{
public:
	simple_ringbuf()
	{
		m_container.resize(Size);
	}

	bool has_data() const
	{
		return m_used != 0;
	}

	u32 read_bytes(u8* buf, const u32 size)
	{
		const u32 to_read = size > m_used ? m_used : size;
		if (!to_read)
			return 0;

		ensure(buf);

		u8* data = m_container.data();
		const u32 new_tail = m_tail + to_read;

		if (new_tail >= Size)
		{
			const u32 first_chunk_size = Size - m_tail;
			std::memcpy(buf, data + m_tail, first_chunk_size);
			std::memcpy(buf + first_chunk_size, data, to_read - first_chunk_size);
			m_tail = (new_tail - Size);
		}
		else
		{
			std::memcpy(buf, data + m_tail, to_read);
			m_tail = new_tail;
		}

		m_used -= to_read;

		return to_read;
	}

	void write_bytes(const u8* buf, const u32 size)
	{
		if (size == 0)
		{
			return;
		}

		ensure(size <= Size);

		const u32 over_size = m_used + size;

		if (over_size > Size)
		{
			m_tail += (over_size - Size);
			if (m_tail > Size)
				m_tail -= Size;

			m_used = Size;
		}
		else
		{
			m_used = over_size;
		}

		u8* data = m_container.data();
		const u32 new_head = m_head + size;

		if (new_head >= Size)
		{
			const u32 first_chunk_size = Size - m_head;
			std::memcpy(data + m_head, buf, first_chunk_size);
			std::memcpy(data, buf + first_chunk_size, size - first_chunk_size);
			m_head = (new_head - Size);
		}
		else
		{
			std::memcpy(data + m_head, buf, size);
			m_head = new_head;
		}
	}

protected:
	std::vector<u8> m_container;
	u32 m_head = 0, m_tail = 0, m_used = 0;
};

class microphone_device
{
public:
	microphone_device(microphone_handler type = microphone_handler::null);

	void add_device(const std::string& name);

	void set_registered(bool registered) { mic_registered = registered; };

	error_code open_microphone(const u8 type, const u32 dsp_r, const u32 raw_r, const u8 channels = 2);
	error_code close_microphone();

	error_code start_microphone();
	error_code stop_microphone();

	std::string get_device_name() const { return devices.empty() ? "" : devices.front().name; }

	void update_audio();
	bool has_data() const;

	f32 calculate_energy_level();

	bool is_registered() const { return mic_registered; }
	bool is_opened() const { return mic_opened; }
	bool is_started() const { return mic_started; }
	u8 get_signal_types() const { return signal_types; }
	constexpr u8 get_bit_resolution() const { return bit_resolution; }
	u32 get_raw_samplingrate() const { return raw_samplingrate; }
	u8 get_num_channels() const { return num_channels; }
	u8 get_datatype() const
	{
		switch(device_type)
		{
		case microphone_handler::real_singstar:
		case microphone_handler::singstar:
			return 0; // LE
		default:
			return 1; // BE
		}
	}

	u32 read_raw(u8* buf, u32 size) { return rbuf_raw.read_bytes(buf, size); }
	u32 read_dsp(u8* buf, u32 size) { return rbuf_dsp.read_bytes(buf, size); }

	// Microphone attributes
	u32 attr_gain       = 3;                      // 0 to 5.   Default is 3.
	u32 attr_volume     = 145;                    // 0 to 241. Default is 145.
	u32 attr_agc        = 0;                      // 0 to 241. Default is 0.
	std::array<u32, 2> attr_chanvol = {100, 100}; // 0 to 100. Default is ?.
	u32 attr_led        = 0;
	u32 attr_dsptype    = 0;

private:
	template <u32 bytesize>
	static inline void variable_byteswap(const void* src, void* dst);
	inline u32 convert_16_bit_pcm_to_float(const std::vector<u8>& buffer, u32 num_bytes);

#ifndef WITHOUT_OPENAL
	void enumerate_devices();
	ALCdevice* open_device(const std::string& name, u32 samplingrate, ALCenum num_al_channels, u32 buf_size);
#endif

	u32 capture_audio();

	void get_data(const u32 num_samples);
	void get_raw(const u32 num_samples);
	void get_dsp(const u32 num_samples);

	microphone_handler device_type = microphone_handler::null;

	bool mic_registered = false;
	bool mic_opened  = false;
	bool mic_started = false;

	struct mic_device
	{
		std::string name;
		ALCdevice* device = nullptr;
		std::vector<u8> buf;
	};

	std::vector<std::string> enumerated_devices;
	std::vector<mic_device> devices;
	std::vector<u8> temp_buf;
	std::vector<u8> float_buf;

	// Sampling information provided at opening of mic
	u32 raw_samplingrate = 48000;
	u32 dsp_samplingrate = 48000;
	u32 aux_samplingrate = 48000;
	u8 num_channels      = 2;

	u8 signal_types = CELLMIC_SIGTYPE_NULL;

	u32 sample_size = 0; // Determined at opening for internal use

	static constexpr u8 bit_resolution = 16;
	static constexpr usz inbuf_size = 400000; // Default value unknown
	static constexpr usz float_buf_size = inbuf_size * (sizeof(f32) / sizeof(s16));

	simple_ringbuf<inbuf_size> rbuf_raw;
	simple_ringbuf<float_buf_size> rbuf_dsp;
	simple_ringbuf<inbuf_size> rbuf_aux;
};

class mic_context
{
public:
	void operator()();
	void load_config_and_init();
	void wake_up();

	// Returns index of registered device
	u32 register_device(const std::string& device_name);
	void unregister_device(u32 dev_num);
	bool check_device(u32 dev_num);

	u64 event_queue_key = 0;
	u64 event_queue_source = 0;

	std::array<microphone_device, CELL_MAX_MICS> mic_list{};

	shared_mutex mutex;
	atomic_t<u8> init = 0;

	static constexpr auto thread_name = "Microphone Thread"sv;

protected:
	atomic_t<u32> wakey = 0;

	//	u32 signalStateLocalTalk = 9; // value is in range 0-10. 10 indicates talking, 0 indicating none.
	//	u32 signalStateFarTalk = 0; // value is in range 0-10. 10 indicates talking from far away, 0 indicating none.
	//	f32 signalStateNoiseSupression; // value is in decibels
	//	f32 signalStateGainControl;
	//	f32 signalStateMicSignalLevel; // value is in decibels
	//	f32 signalStateSpeakerSignalLevel; // value is in decibels
};

using mic_thread = named_thread<mic_context>;
