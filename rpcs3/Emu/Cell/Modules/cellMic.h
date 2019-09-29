#pragma once

#include "Utilities/BEType.h"
#include "Utilities/Thread.h"

#include "3rdparty/OpenAL/include/alext.h"

// Error Codes
enum
{
	CELL_MIC_ERROR_ALREADY_INIT       = 0x80140101,
	CELL_MIC_ERROR_SYSTEM             = 0x80140102,
	CELL_MIC_ERROR_NOT_INIT           = 0x80140103,
	CELL_MIC_ERROR_PARAM              = 0x80140104,
	CELL_MIC_ERROR_PORT_FULL          = 0x80140105,
	CELL_MIC_ERROR_ALREADY_OPEN       = 0x80140106,
	CELL_MIC_ERROR_NOT_OPEN           = 0x80140107,
	CELL_MIC_ERROR_NOT_RUN            = 0x80140108,
	CELL_MIC_ERROR_TRANS_EVENT        = 0x80140109,
	CELL_MIC_ERROR_OPEN               = 0x8014010a,
	CELL_MIC_ERROR_SHAREDMEMORY       = 0x8014010b,
	CELL_MIC_ERROR_MUTEX              = 0x8014010c,
	CELL_MIC_ERROR_EVENT_QUEUE        = 0x8014010d,
	CELL_MIC_ERROR_DEVICE_NOT_FOUND   = 0x8014010e,
	CELL_MIC_ERROR_SYSTEM_NOT_FOUND   = 0x8014010e,
	CELL_MIC_ERROR_FATAL              = 0x8014010f,
	CELL_MIC_ERROR_DEVICE_NOT_SUPPORT = 0x80140110,
};

struct CellMicInputFormat
{
	u8 channelNum;
	u8 subframeSize;
	u8 bitResolution;
	u8 dataType;
	be_t<u32> sampleRate;
};

enum CellMicSignalState : u32
{
	CELL_MIC_SIGSTATE_LOCTALK = 0,
	CELL_MIC_SIGSTATE_FARTALK = 1,
	CELL_MIC_SIGSTATE_NSR     = 3,
	CELL_MIC_SIGSTATE_AGC     = 4,
	CELL_MIC_SIGSTATE_MICENG  = 5,
	CELL_MIC_SIGSTATE_SPKENG  = 6,
};

enum CellMicCommand
{
	CELL_MIC_ATTACH = 2,
	CELL_MIC_DATA   = 5,
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
	CELLMIC_SIGATTR_BKNGAIN,
	CELLMIC_SIGATTR_REVERB,
	CELLMIC_SIGATTR_AGCLEVEL,
	CELLMIC_SIGATTR_VOLUME,
	CELLMIC_SIGATTR_PITCHSHIFT
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
} CellMicType;

template <std::size_t S>
class simple_ringbuf
{
public:
	simple_ringbuf()
	{
		m_container.resize(S);
	}

	bool has_data() const
	{
		return m_used != 0;
	}

	u32 read_bytes(u8* buf, const u32 size)
	{
		u32 to_read = size > m_used ? m_used : size;
		if (!to_read)
			return 0;

		u8* data     = m_container.data();
		u32 new_tail = m_tail + to_read;

		if (new_tail >= S)
		{
			u32 first_chunk_size = S - m_tail;
			std::memcpy(buf, data + m_tail, first_chunk_size);
			std::memcpy(buf + first_chunk_size, data, to_read - first_chunk_size);
			m_tail = (new_tail - S);
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
		ASSERT(size <= S);

		if (u32 over_size = m_used + size; over_size > S)
		{
			m_tail += (over_size - S);
			if (m_tail > S)
				m_tail -= S;

			m_used = S;
		}
		else
		{
			m_used = over_size;
		}

		u8* data     = m_container.data();
		u32 new_head = m_head + size;

		if (new_head >= S)
		{
			u32 first_chunk_size = S - m_head;
			std::memcpy(data + m_head, buf, first_chunk_size);
			std::memcpy(data, buf + first_chunk_size, size - first_chunk_size);
			m_head = (new_head - S);
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
	microphone_device(microphone_handler type);

	void add_device(const std::string& name);

	s32 open_microphone(const u8 type, const u32 dsp_r, const u32 raw_r, const u8 channels = 2);
	s32 close_microphone();

	s32 start_microphone();
	s32 stop_microphone();

	void update_audio();
	bool has_data() const;

	bool is_opened() const { return mic_opened;	}
	bool is_started() const { return mic_started; }
	u8 get_signal_types() const { return signal_types; }
	u8 get_bit_resolution() const {	return bit_resolution; }
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
	u32 attr_gain       = 3;
	u32 attr_volume     = 145;
	u32 attr_agc        = 0;
	u32 attr_chanvol[2] = {145, 145};
	u32 attr_led        = 0;
	u32 attr_dsptype    = 0;

private:
	static void variable_byteswap(const void* src, void* dst, const u32 bytesize);

	u32 capture_audio();

	void get_raw(const u32 num_samples);
	void get_dsp(const u32 num_samples);

private:
	microphone_handler device_type;
	std::vector<std::string> device_name;

	bool mic_opened  = false;
	bool mic_started = false;

	std::vector<ALCdevice*> input_devices;
	std::vector<std::vector<u8>> internal_bufs;
	std::vector<u8> temp_buf;

	// Sampling information provided at opening of mic
	u32 raw_samplingrate = 48000;
	u32 dsp_samplingrate = 48000;
	u32 aux_samplingrate = 48000;
	u8 bit_resolution    = 16;
	u8 num_channels      = 2;

	u8 signal_types = CELLMIC_SIGTYPE_NULL;

	u32 sample_size; // Determined at opening for internal use

	static constexpr std::size_t inbuf_size = 400000; // Default value unknown

	simple_ringbuf<inbuf_size> rbuf_raw;
	simple_ringbuf<inbuf_size> rbuf_dsp;
	simple_ringbuf<inbuf_size> rbuf_aux;
};

class mic_context
{
public:
	void operator()();
	void load_config_and_init();

	u64 event_queue_key = 0;

	std::unordered_map<u8, microphone_device> mic_list;

	shared_mutex mutex;
	atomic_t<u32> init = 0;

	static constexpr auto thread_name = "Microphone Thread"sv;

protected:
	const u64 start_time = get_guest_system_time();
	u64 m_counter        = 0;

	//	u32 signalStateLocalTalk = 9; // value is in range 0-10. 10 indicates talking, 0 indicating none.
	//	u32 signalStateFarTalk = 0; // value is in range 0-10. 10 indicates talking from far away, 0 indicating none.
	//	f32 signalStateNoiseSupression; // value is in decibels
	//	f32 signalStateGainControl;
	//	f32 signalStateMicSignalLevel; // value is in decibels
	//	f32 signalStateSpeakerSignalLevel; // value is in decibels
};

using mic_thread = named_thread<mic_context>;
