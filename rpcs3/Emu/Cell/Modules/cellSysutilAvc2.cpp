#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/IdManager.h"
#include "Emu/NP/ip_address.h"
#include "Emu/NP/np_handler.h"
#include "Emu/NP/np_helpers.h"
#include "Emu/NP/signaling_handler.h"
#include "Emu/NP/vport0.h"
#include "Emu/system_config.h"
#include "Emu/Audio/audio_utils.h"
#include "util/asm.hpp"

#include "sceNp.h"
#include "sceNp2.h"
#include "cellSysutilAvc2.h"
#include "cellSysutil.h"
#include "cellMic.h"

#include <array>
#include <cmath>
#include <deque>
#include <map>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#include <netinet/in.h>
#endif

LOG_CHANNEL(cellSysutilAvc2);

template<>
void fmt_class_string<CellSysutilAvc2Error>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_AVC2_ERROR_UNKNOWN);
			STR_CASE(CELL_AVC2_ERROR_NOT_SUPPORTED);
			STR_CASE(CELL_AVC2_ERROR_NOT_INITIALIZED);
			STR_CASE(CELL_AVC2_ERROR_ALREADY_INITIALIZED);
			STR_CASE(CELL_AVC2_ERROR_INVALID_ARGUMENT);
			STR_CASE(CELL_AVC2_ERROR_OUT_OF_MEMORY);
			STR_CASE(CELL_AVC2_ERROR_ERROR_BAD_ID);
			STR_CASE(CELL_AVC2_ERROR_INVALID_STATUS);
			STR_CASE(CELL_AVC2_ERROR_TIMEOUT);
			STR_CASE(CELL_AVC2_ERROR_NO_SESSION);
			STR_CASE(CELL_AVC2_ERROR_WINDOW_ALREADY_EXISTS);
			STR_CASE(CELL_AVC2_ERROR_TOO_MANY_WINDOWS);
			STR_CASE(CELL_AVC2_ERROR_TOO_MANY_PEER_WINDOWS);
			STR_CASE(CELL_AVC2_ERROR_WINDOW_NOT_FOUND);
		}

		return unknown;
	});
}

template<>
void fmt_class_string<CellSysutilAvc2AttributeId>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](CellSysutilAvc2AttributeId value)
	{
		switch (value)
		{
			STR_CASE(CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_DETECT_EVENT_TYPE);
			STR_CASE(CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_DETECT_INTERVAL_TIME);
			STR_CASE(CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_DETECT_SIGNAL_LEVEL);
			STR_CASE(CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_MAX_BITRATE);
			STR_CASE(CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_DATA_FEC);
			STR_CASE(CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_PACKET_CONTENTION);
			STR_CASE(CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_DTX_MODE);
			STR_CASE(CELL_SYSUTIL_AVC2_ATTRIBUTE_MIC_STATUS_DETECTION);
			STR_CASE(CELL_SYSUTIL_AVC2_ATTRIBUTE_MIC_SETTING_NOTIFICATION);
			STR_CASE(CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_MUTING_NOTIFICATION);
			STR_CASE(CELL_SYSUTIL_AVC2_ATTRIBUTE_CAMERA_STATUS_DETECTION);
		}

		return unknown;
	});
}

struct avc2_cb_handle_t{};
struct avc2_settings
{
	avc2_settings() = default;

	avc2_settings(const avc2_settings&) = delete;
	avc2_settings& operator=(const avc2_settings&) = delete;

	SAVESTATE_INIT_POS(52);

	shared_mutex mutex_cb;
	vm::ptr<CellSysutilAvc2Callback> avc2_cb{};
	vm::ptr<void> avc2_cb_arg{};

	u32 streaming_mode = CELL_SYSUTIL_AVC2_STREAMING_MODE_NORMAL;
	u8 mic_out_stream_sharing = 0;
	u8 video_stream_sharing = 0;
	u32 total_video_bitrate = 0;
	std::set<u16> voice_muting_players;
	bool voice_muting = 0;
	bool video_muting = 1;
	bool speaker_muting = 0;
	f32 speaker_volume_level = 1.0f; // Linear gain, 1.0 is the original volume

	bool loaded = false;
	bool joined = false;
	bool streaming = false;
	bool voice_detection = false;
	u16 ctx_id = 0;
	u64 room_id = 0;
	u16 local_member_id = 0;
	u16 max_speakers = 16;
	u16 max_players = 16;

	u64 voice_detect_event_type = 0;
	u64 voice_detect_interval_time = 1000;
	u64 voice_detect_signal_level = 9;
	u64 voice_max_bitrate = 28000;
	u64 voice_data_fec = 0;
	u64 voice_packet_contention = 1;
	u64 voice_dtx_mode = 1;
	u64 mic_status_detection = 0;
	u64 mic_setting_notification = 0;
	u64 voice_muting_notification = 0;
	u64 camera_status_detection = 0;

	static bool saveable(bool /*is_writing*/) noexcept
	{
		return GET_SERIALIZATION_VERSION(cellSysutil) != 0;
	}

	avc2_settings(utils::serial& ar) noexcept
	{
		[[maybe_unused]] const s32 version = GET_SERIALIZATION_VERSION(cellSysutil);

		if (version == 0)
		{
			return;
		}

		save(ar);
	}

	void save(utils::serial& ar)
	{
		[[maybe_unused]] const s32 version = GET_OR_USE_SERIALIZATION_VERSION(ar.is_writing(), cellSysutil);

		ar(avc2_cb, avc2_cb_arg, streaming_mode, mic_out_stream_sharing, video_stream_sharing, total_video_bitrate);

		if (ar.is_writing() || version >= 2)
		{
			ar(voice_muting_players, voice_muting, video_muting, speaker_muting, speaker_volume_level);
		}
	}

	void register_cb_call(u32 event, u64 event_param)
	{
		sysutil_register_cb_with_id<avc2_cb_handle_t>([=, this](ppu_thread& cb_ppu) -> s32
			{
				vm::ptr<CellSysutilAvc2Callback> avc2_cb{};
				vm::ptr<void> avc2_cb_arg{};

				{
					std::lock_guard lock(this->mutex_cb);
					avc2_cb = this->avc2_cb;
					avc2_cb_arg = this->avc2_cb_arg;
				}

				if (avc2_cb)
				{
					avc2_cb(cb_ppu, event, event_param, avc2_cb_arg);

					if ((event == CELL_AVC2_EVENT_LOAD_FAILED ||
							event == CELL_AVC2_EVENT_UNLOAD_SUCCEEDED ||
							event == CELL_AVC2_EVENT_UNLOAD_FAILED) &&
						event_param < 2)
					{
						sysutil_unregister_cb_with_id<avc2_cb_handle_t>();

						std::lock_guard lock(this->mutex_cb);
						this->avc2_cb = vm::null;
						this->avc2_cb_arg = vm::null;
					}
				}

				return 0;
			});
	}
};

namespace
{
	constexpr u32 avc2_sample_rate = 16000;
	constexpr u32 avc2_output_rate = 48000;
	constexpr u32 avc2_frame_samples = 320; // 20 ms
	constexpr u8 avc2_wire_version = 2;
	constexpr usz avc2_encoded_frame_size = avc2_frame_samples / 2;
	constexpr usz avc2_common_header_size = 18;
	constexpr u32 avc2_dtx_threshold = 2;
	constexpr u32 avc2_dtx_hangover_frames = 20; // Keep sending for 400 ms after the voice drops below the threshold
	constexpr usz avc2_jitter_prebuffer_frames = 4; // Frames buffered per peer before a talk spurt starts playing
	constexpr usz avc2_jitter_max_frames = 10; // Drop old frames above this to keep the latency bounded
	constexpr usz avc2_output_target_samples = avc2_output_rate / 1000 * 40; // Keep ~40 ms queued for cellAudio

	enum class avc2_packet_type : u8
	{
		audio = 0,
		mic_status = 1,
	};

	class avc2_ring_buffer
	{
	public:
		explicit avc2_ring_buffer(usz capacity)
			: data(capacity)
		{
		}

		usz free_size() const
		{
			std::lock_guard lock(mutex);
			return data.size() - used;
		}

		usz size() const
		{
			std::lock_guard lock(mutex);
			return used;
		}

		usz push(const void* source, usz size)
		{
			if (!source || !size)
				return 0;
			std::lock_guard lock(mutex);
			const usz count = std::min(size, data.size() - used);
			const usz first = std::min(count, data.size() - write_position);
			std::memcpy(data.data() + write_position, source, first);
			std::memcpy(data.data(), static_cast<const u8*>(source) + first, count - first);
			write_position = (write_position + count) % data.size();
			used += count;
			return count;
		}

		usz push_overwrite(const void* source, usz size)
		{
			if (!source || !size)
				return 0;

			std::lock_guard lock(mutex);
			const u8* input = static_cast<const u8*>(source);
			if (size >= data.size())
			{
				input += size - data.size();
				size = data.size();
				read_position = 0;
				write_position = 0;
				used = 0;
			}
			else if (const usz missing = size - std::min(size, data.size() - used))
			{
				read_position = (read_position + missing) % data.size();
				used -= missing;
			}

			const usz first = std::min(size, data.size() - write_position);
			std::memcpy(data.data() + write_position, input, first);
			std::memcpy(data.data(), input + first, size - first);
			write_position = (write_position + size) % data.size();
			used += size;
			return size;
		}

		usz pop(void* destination, usz size)
		{
			if (!destination || !size)
				return 0;
			std::lock_guard lock(mutex);
			const usz count = std::min(size, used);
			const usz first = std::min(count, data.size() - read_position);
			std::memcpy(destination, data.data() + read_position, first);
			std::memcpy(static_cast<u8*>(destination) + first, data.data(), count - first);
			read_position = (read_position + count) % data.size();
			used -= count;
			return count;
		}

		void discard(usz size = umax)
		{
			std::lock_guard lock(mutex);
			const usz count = std::min(size, used);
			read_position = (read_position + count) % data.size();
			used -= count;
		}

	private:
		mutable shared_mutex mutex;
		std::vector<u8> data;
		usz read_position = 0;
		usz write_position = 0;
		usz used = 0;
	};

	static constexpr std::array<s16, 89> ima_step_table = {
		7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31,
		34, 37, 41, 45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143,
		157, 173, 190, 209, 230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658,
		724, 796, 876, 963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
		3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493, 10442, 11487, 12635, 13899,
		15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767
	};

	static constexpr std::array<s8, 16> ima_index_table = {
		-1, -1, -1, -1, 2, 4, 6, 8,
		-1, -1, -1, -1, 2, 4, 6, 8
	};

	template <typename T>
	void append_le(std::vector<u8>& data, T value)
	{
		const le_t<T> converted = value;
		const u8* ptr = reinterpret_cast<const u8*>(&converted);
		data.insert(data.end(), ptr, ptr + sizeof(T));
	}

	template <typename T>
	bool read_le(const std::vector<u8>& data, usz offset, T& value)
	{
		if (offset + sizeof(T) > data.size())
		{
			return false;
		}

		le_t<T> converted{};
		std::memcpy(&converted, data.data() + offset, sizeof(T));
		value = converted;
		return true;
	}

	u8 ima_encode_sample(s16 sample, s32& predictor, s32& index)
	{
		const s32 step = ima_step_table[index];
		s32 difference = static_cast<s32>(sample) - predictor;
		u8 nibble = 0;

		if (difference < 0)
		{
			nibble = 8;
			difference = -difference;
		}

		s32 delta = step >> 3;
		if (difference >= step)
		{
			nibble |= 4;
			difference -= step;
			delta += step;
		}
		if (difference >= (step >> 1))
		{
			nibble |= 2;
			difference -= step >> 1;
			delta += step >> 1;
		}
		if (difference >= (step >> 2))
		{
			nibble |= 1;
			delta += step >> 2;
		}

		predictor += (nibble & 8) ? -delta : delta;
		predictor = std::clamp(predictor, -32768, 32767);
		index = std::clamp(index + ima_index_table[nibble], 0, 88);
		return nibble;
	}

	s16 ima_decode_sample(u8 nibble, s32& predictor, s32& index)
	{
		const s32 step = ima_step_table[index];
		s32 delta = step >> 3;
		if (nibble & 4) delta += step;
		if (nibble & 2) delta += step >> 1;
		if (nibble & 1) delta += step >> 2;

		predictor += (nibble & 8) ? -delta : delta;
		predictor = std::clamp(predictor, -32768, 32767);
		index = std::clamp(index + ima_index_table[nibble & 0xf], 0, 88);
		return static_cast<s16>(predictor);
	}

	struct avc2_encoded_frame
	{
		s16 predictor = 0;
		u8 step_index = 0;
		std::array<u8, avc2_encoded_frame_size> data{};
	};

	avc2_encoded_frame encode_frame(const std::array<s16, avc2_frame_samples>& pcm, s32& predictor, s32& index)
	{
		avc2_encoded_frame result;
		result.predictor = static_cast<s16>(predictor);
		result.step_index = static_cast<u8>(index);

		for (u32 i = 0; i < avc2_frame_samples; i++)
		{
			const u8 nibble = ima_encode_sample(pcm[i], predictor, index);
			if (i & 1)
				result.data[i / 2] |= nibble << 4;
			else
				result.data[i / 2] = nibble;
		}

		return result;
	}

	std::array<s16, avc2_frame_samples> decode_frame(s16 predictor_value, u8 step_index, const u8* data)
	{
		std::array<s16, avc2_frame_samples> pcm{};
		s32 predictor = predictor_value;
		s32 index = std::min<s32>(step_index, 88);

		for (u32 i = 0; i < avc2_frame_samples; i++)
		{
			const u8 byte = data[i / 2];
			const u8 nibble = (i & 1) ? byte >> 4 : byte & 0xf;
			pcm[i] = ima_decode_sample(nibble, predictor, index);
		}

		return pcm;
	}

	u32 signal_level(const std::array<s16, avc2_frame_samples>& pcm)
	{
		f64 sum = 0.0;
		for (const s16 sample : pcm)
		{
			const f64 normalized = static_cast<f64>(sample) / 32768.0;
			sum += normalized * normalized;
		}

		const f64 rms = std::sqrt(sum / pcm.size());
		const f64 db = 20.0 * std::log10(std::max(rms, 0.00001));
		return static_cast<u32>(std::clamp(std::lround((db + 60.0) / 4.0), 0l, 10l));
	}
}

class avc2_voice_engine
{
public:
	static constexpr auto thread_name = "AV Chat2 Voice Thread"sv;

	avc2_voice_engine()
		: output_audio(avc2_output_rate * sizeof(f32) * 2)
		, shared_mic_audio(avc2_sample_rate * sizeof(f32) / 10)
	{
	}

	void operator()()
	{
		while (thread_ctrl::state() != thread_state::aborting)
		{
			thread_ctrl::wait_on(wakey, 0, 5000);
			wakey.store(0);

			std::lock_guard lock(mutex);
			update_enabled_nl();
			process_network_nl();
			refresh_members_nl();

			if (loaded && streaming && voice_chat_enabled)
			{
				capture_nl();
				process_playback_nl();
				send_periodic_status_nl();
			}
		}

		std::lock_guard lock(mutex);
		close_microphone_nl();
	}

	void load(u16 context_id, u16 players, u16 speakers, bool mic_sharing)
	{
		std::lock_guard lock(mutex);
		loaded = true;
		ctx_id = context_id;
		max_players = players;
		max_speakers = speakers;
		mic_out_stream_sharing = mic_sharing;
		logged_first_capture.store(false);
		logged_first_send.store(false);
		logged_first_receive.store(false);
		logged_first_mix.store(false);
		logged_dtx_warning.store(false);
		wake_up();
	}

	void unload()
	{
		std::lock_guard lock(mutex);
		streaming = false;
		joined = false;
		loaded = false;
		room_id = 0;
		local_member_id = 0;
		peers.clear();
		members.clear();
		muted_players.clear();
		capture_pending.clear();
		output_audio.discard();
		shared_mic_audio.discard();
		logged_first_capture.store(false);
		logged_first_send.store(false);
		logged_first_receive.store(false);
		logged_first_mix.store(false);
		logged_dtx_warning.store(false);
		close_microphone_nl();
	}

	bool join_room(u64 new_room_id)
	{
		std::lock_guard lock(mutex);
		auto& nph = g_fxo->get<named_thread<np::np_handler>>();
		const auto member = nph.local_get_memberid(new_room_id, nph.get_npid());
		if (!member)
		{
			return false;
		}

		room_id = new_room_id;
		local_member_id = *member;
		joined = true;
		members.clear();
		refresh_members_nl(true);
		send_mic_status_nl();
		wake_up();
		return true;
	}

	void leave()
	{
		std::lock_guard lock(mutex);
		muted_players.clear();
		joined = false;
		room_id = 0;
		local_member_id = 0;
		peers.clear();
		members.clear();
		output_audio.discard();
	}

	bool is_player_muted(u16 member_id) const
	{
		std::lock_guard lock(mutex);
		return muted_players.contains(member_id);
	}

	// Players muted by the user (not the game). Stored by NPID so the choice survives rejoining a room.
	void set_user_muting(const std::string& npid, bool muted)
	{
		std::lock_guard lock(mutex);
		if (muted)
			user_muted_npids.insert(npid);
		else
			user_muted_npids.erase(npid);
	}

	std::vector<avc2_voice_chat_member> get_voice_chat_members() const
	{
		std::lock_guard lock(mutex);
		std::vector<avc2_voice_chat_member> result;
		if (!joined)
		{
			return result;
		}

		auto& nph = g_fxo->get<named_thread<np::np_handler>>();
		for (const u16 member_id : members)
		{
			if (member_id == local_member_id)
			{
				continue;
			}

			const auto [error, npid] = nph.local_get_npid(room_id, member_id);
			if (error != CELL_OK || !npid)
			{
				continue;
			}

			std::string name = np::npid_to_string(*npid);
			const bool muted = user_muted_npids.contains(name);
			result.push_back({std::move(name), muted});
		}
		return result;
	}

	bool is_joined() const
	{
		std::lock_guard lock(mutex);
		return joined;
	}

	void set_streaming(bool enabled)
	{
		std::lock_guard lock(mutex);
		if (streaming == enabled)
		{
			return;
		}

		streaming = enabled;
		reset_audio_nl();
		send_mic_status_nl();
		wake_up();
	}

	void set_voice_detection(bool enabled)
	{
		std::lock_guard lock(mutex);
		voice_detection = enabled;
		for (auto& [_, peer] : peers)
		{
			peer.talking = false;
			peer.last_event = {};
		}
	}

	void set_voice_muting(bool muted)
	{
		std::lock_guard lock(mutex);
		voice_muting = muted;
		send_mic_status_nl();
	}

	void set_speaker(bool muted, f32 level)
	{
		speaker_muting.store(muted);
		speaker_volume.store(level);
	}

	void set_player_muting(u16 member_id, bool muted)
	{
		std::lock_guard lock(mutex);
		if (muted)
			muted_players.insert(member_id);
		else
			muted_players.erase(member_id);
	}

	void set_attributes(u64 event_type, u64 interval, u64 threshold, bool fec, bool dtx, bool mic_detection)
	{
		std::lock_guard lock(mutex);
		voice_detect_event_type = event_type;
		voice_detect_interval = interval;
		voice_detect_threshold = threshold;
		voice_fec = fec;
		voice_dtx = dtx;
		mic_status_detection = mic_detection;
	}

	u8 microphone_status() const
	{
		std::lock_guard lock(mutex);
		return microphone_status_nl();
	}

	u8 player_microphone_status(u16 member_id) const
	{
		std::lock_guard lock(mutex);
		if (member_id == local_member_id)
		{
			return microphone_status_nl();
		}
		if (const auto found = peers.find(member_id); found != peers.end())
		{
			return found->second.mic_status;
		}
		return CELL_AVC2_MIC_STATUS_UNKNOWN;
	}

	u32 read_shared_mic(void* destination, u32 size)
	{
		if (!destination || !size || !mic_out_stream_sharing)
		{
			return 0;
		}
		return ::narrow<u32>(shared_mic_audio.pop(destination, size));
	}

	void mix(f32* out, u32 samples, u32 channels, f32 master_volume)
	{
		if (!out || !samples || !channels)
		{
			return;
		}

		std::array<f32, 256> mono{};
		AUDIT(samples <= mono.size());
		const u64 read = output_audio.pop(mono.data(), samples * sizeof(f32));
		if (read == 0)
		{
			return;
		}

		if (speaker_muting.load())
		{
			return;
		}

		if (!logged_first_mix.exchange(true))
		{
			cellSysutilAvc2.notice("AV Chat2 mixed the first received voice frame into cellAudio");
		}

		const f32 gain = speaker_volume.load() * master_volume;
		const u32 valid_samples = ::narrow<u32>(read / sizeof(f32));
		for (u32 i = 0; i < valid_samples; i++)
		{
			const f32 sample = std::clamp(mono[i] * gain, -1.0f, 1.0f);
			out[i * channels] += sample;
			if (channels > 1)
			{
				out[i * channels + 1] += sample;
			}
		}
	}

private:
	struct peer_state
	{
		std::map<u16, std::array<s16, avc2_frame_samples>> frames;
		u16 next_sequence = 0;
		bool playback_started = false;
		u8 mic_status = CELL_AVC2_MIC_STATUS_UNKNOWN;
		u32 level = 0;
		bool talking = false;
		steady_clock::time_point last_packet{};
		steady_clock::time_point last_event{};
		std::string npid;
	};

	mutable shared_mutex mutex;
	atomic_t<u32> wakey = 0;
	atomic_t<bool> speaker_muting = false;
	atomic_t<f32> speaker_volume = 1.0f;
	atomic_t<bool> logged_first_capture = false;
	atomic_t<bool> logged_first_send = false;
	atomic_t<bool> logged_first_receive = false;
	atomic_t<bool> logged_first_mix = false;
	atomic_t<bool> logged_dtx_warning = false;
	avc2_ring_buffer output_audio;
	avc2_ring_buffer shared_mic_audio;

	microphone_device microphone{};
	bool microphone_open = false;
	bool loaded = false;
	bool joined = false;
	bool streaming = false;
	bool voice_detection = false;
	bool voice_muting = false;
	bool mic_out_stream_sharing = false;
	bool voice_fec = false;
	bool voice_dtx = true;
	bool mic_status_detection = false;
	u16 ctx_id = 0;
	u16 local_member_id = 0;
	u16 max_players = 16;
	u16 max_speakers = 16;
	u16 outgoing_sequence = 0;
	u64 room_id = 0;
	u64 voice_detect_event_type = 0;
	u64 voice_detect_interval = 1000;
	u64 voice_detect_threshold = 9;
	std::set<u16> members;
	std::set<u16> muted_players;
	std::set<std::string> user_muted_npids;
	std::map<u16, peer_state> peers;
	bool voice_chat_enabled = g_cfg.net.voice_chat_enabled.get();
	std::vector<s16> capture_pending;
	s32 encoder_predictor = 0;
	s32 encoder_index = 0;
	u32 dtx_hangover = 0;
	std::optional<avc2_encoded_frame> dtx_preroll;
	f32 last_output_sample = 0.0f;
	steady_clock::time_point last_status_send{};
	steady_clock::time_point last_member_refresh{};
	steady_clock::time_point local_last_event{};
	steady_clock::time_point last_no_peer_warning{};
	bool local_talking = false;

	void wake_up()
	{
		wakey.store(1);
		wakey.notify_one();
	}

	// Opens or closes the microphone and clears all pending audio to match the streaming state and the voice chat setting
	void reset_audio_nl()
	{
		capture_pending.clear();
		encoder_predictor = 0;
		encoder_index = 0;
		dtx_hangover = 0;
		dtx_preroll.reset();
		last_output_sample = 0.0f;

		if (streaming && voice_chat_enabled)
		{
			open_microphone_nl();
		}
		else
		{
			close_microphone_nl();
			output_audio.discard();
			for (auto& [_, peer] : peers)
			{
				peer.frames.clear();
				peer.playback_started = false;
			}
		}
	}

	void update_enabled_nl()
	{
		const bool enabled = g_cfg.net.voice_chat_enabled.get();
		if (enabled == voice_chat_enabled)
		{
			return;
		}

		voice_chat_enabled = enabled;
		cellSysutilAvc2.notice("AV Chat2 voice chat %s", enabled ? "enabled" : "disabled");
		reset_audio_nl();
		send_mic_status_nl();
	}

	bool open_microphone_nl()
	{
		close_microphone_nl();
		if (g_cfg.audio.microphone_type == microphone_handler::null)
		{
			return false;
		}

		const std::vector<std::string> devices = fmt::split(g_cfg.audio.microphone_devices.to_string(), {"@@@"});
		if (devices.empty() || devices[0].empty())
		{
			cellSysutilAvc2.warning("No microphone device is configured for AV Chat2");
			return false;
		}

		const microphone_handler type = g_cfg.audio.microphone_type.get();
		microphone = microphone_device(type);
		microphone.set_registered(true);
		microphone.add_device(devices[0]);
		if (type == microphone_handler::singstar && devices.size() > 1 && !devices[1].empty())
		{
			microphone.add_device(devices[1]);
		}

		const u8 channels = (type == microphone_handler::singstar || type == microphone_handler::real_singstar) ? 2 : 1;
		if (microphone.open_microphone(CELLMIC_SIGTYPE_RAW, avc2_sample_rate, avc2_sample_rate, channels) != CELL_OK ||
			microphone.start_microphone() != CELL_OK)
		{
			cellSysutilAvc2.error("Failed to open configured microphone for AV Chat2");
			microphone.close_microphone();
			return false;
		}

		microphone_open = true;
		cellSysutilAvc2.notice("AV Chat2 microphone opened: '%s' (%d Hz, %d channel(s))", microphone.get_device_name(), microphone.get_raw_samplingrate(), microphone.get_num_channels());
		return true;
	}

	void close_microphone_nl()
	{
		if (microphone_open)
		{
			microphone.close_microphone();
			microphone_open = false;
		}
	}

	u8 microphone_status_nl() const
	{
		if (!voice_chat_enabled)
		{
			return CELL_AVC2_MIC_STATUS_DETACHED;
		}

		if (!microphone_open)
		{
			const std::vector<std::string> devices = fmt::split(g_cfg.audio.microphone_devices.to_string(), {"@@@"});
			if (!streaming && g_cfg.audio.microphone_type != microphone_handler::null && !devices.empty() && !devices[0].empty())
			{
				return CELL_AVC2_MIC_STATUS_ATTACHED_OFF;
			}
			return CELL_AVC2_MIC_STATUS_DETACHED;
		}
		return streaming && !voice_muting ? CELL_AVC2_MIC_STATUS_ATTACHED_ON : CELL_AVC2_MIC_STATUS_ATTACHED_OFF;
	}

	void capture_nl()
	{
		if (!microphone_open)
		{
			return;
		}

		microphone.update_audio();
		std::array<u8, 8192> raw{};
		const u32 bytes = microphone.read_raw(raw.data(), raw.size());
		const u32 channels = microphone.get_num_channels();
		const u32 frame_bytes = channels * sizeof(s16);
		if (!bytes || !frame_bytes)
		{
			return;
		}

		const f32 mic_gain = audio::get_voice_chat_mic_volume();
		std::vector<be_t<f32>> shared_samples;
		shared_samples.reserve(bytes / frame_bytes);
		for (u32 offset = 0; offset + frame_bytes <= bytes; offset += frame_bytes)
		{
			s32 mixed = 0;
			for (u32 channel = 0; channel < channels; channel++)
			{
				s16 sample = 0;
				if (microphone.get_datatype() == 0)
				{
					le_t<s16> value{};
					std::memcpy(&value, raw.data() + offset + channel * sizeof(s16), sizeof(value));
					sample = value;
				}
				else
				{
					be_t<s16> value{};
					std::memcpy(&value, raw.data() + offset + channel * sizeof(s16), sizeof(value));
					sample = value;
				}
				mixed += sample;
			}

			const f32 scaled = static_cast<f32>(mixed / static_cast<s32>(channels)) * mic_gain;
			const s16 mono = static_cast<s16>(std::clamp(scaled, -32768.0f, 32767.0f));
			capture_pending.push_back(mono);
			if (mic_out_stream_sharing)
			{
				shared_samples.emplace_back(static_cast<f32>(mono) / 32768.0f);
			}
		}

		if (!shared_samples.empty())
		{
			const u64 size = shared_samples.size() * sizeof(be_t<f32>);
			shared_mic_audio.push_overwrite(shared_samples.data(), size);
		}

		const bool user_muted = g_fxo->get<audio::audio_fxo>().voice_chat_mic_muted;
		usz consumed = 0;

		while (capture_pending.size() - consumed >= avc2_frame_samples)
		{
			std::array<s16, avc2_frame_samples> frame{};
			std::copy_n(capture_pending.begin() + consumed, avc2_frame_samples, frame.begin());
			consumed += avc2_frame_samples;

			const u32 level = signal_level(frame);
			if (!logged_first_capture.exchange(true))
			{
				cellSysutilAvc2.notice("AV Chat2 captured the first microphone frame (level=%u)", level);
			}
			maybe_emit_voice_event_nl(local_member_id, user_muted ? 0 : level, local_talking, local_last_event);

			const avc2_encoded_frame encoded = encode_frame(frame, encoder_predictor, encoder_index);

			if (!joined || voice_muting || user_muted)
			{
				dtx_hangover = 0;
				dtx_preroll.reset();
				continue;
			}

			if (level >= avc2_dtx_threshold)
			{
				dtx_hangover = avc2_dtx_hangover_frames;
			}

			if (voice_dtx && !dtx_hangover)
			{
				// Keep the last silent frame so the start of the next word is not cut off
				dtx_preroll = encoded;
				if (!logged_dtx_warning.exchange(true))
				{
					cellSysutilAvc2.notice("AV Chat2 DTX is suppressing silent microphone frames (level=%u)", level);
				}
				continue;
			}

			if (dtx_hangover)
			{
				dtx_hangover--;
			}

			if (dtx_preroll)
			{
				send_audio_frame_nl(*dtx_preroll, outgoing_sequence++);
				dtx_preroll.reset();
			}

			if (!send_audio_frame_nl(encoded, outgoing_sequence++) && steady_clock::now() - last_no_peer_warning >= 5s)
			{
				cellSysutilAvc2.warning("AV Chat2 captured voice but has no active signaling peer to send it to");
				last_no_peer_warning = steady_clock::now();
			}
		}

		capture_pending.erase(capture_pending.begin(), capture_pending.begin() + consumed);
	}

	u32 send_audio_frame_nl(const avc2_encoded_frame& encoded, u16 sequence)
	{
		std::vector<u8> packet;
		packet.reserve(avc2_common_header_size + 5 + encoded.data.size());
		packet.insert(packet.end(), {'A', 'V', 'C', '2'});
		packet.push_back(avc2_wire_version);
		packet.push_back(static_cast<u8>(avc2_packet_type::audio));
		append_le(packet, local_member_id);
		append_le(packet, room_id);
		append_le(packet, sequence);
		append_le(packet, encoded.predictor);
		packet.push_back(encoded.step_index);
		append_le(packet, static_cast<u16>(avc2_frame_samples));
		packet.insert(packet.end(), encoded.data.begin(), encoded.data.end());

		u32 sent = send_packet_to_members_nl(packet);
		if (voice_fec)
		{
			sent += send_packet_to_members_nl(packet);
		}

		if (sent && !logged_first_send.exchange(true))
		{
			cellSysutilAvc2.notice("AV Chat2 sent the first voice frame (room=%llu, member=%u, datagrams=%u)", room_id, local_member_id, sent);
		}
		return sent;
	}

	void send_mic_status_nl()
	{
		if (!joined)
		{
			return;
		}

		std::vector<u8> packet;
		packet.insert(packet.end(), {'A', 'V', 'C', '2'});
		packet.push_back(avc2_wire_version);
		packet.push_back(static_cast<u8>(avc2_packet_type::mic_status));
		append_le(packet, local_member_id);
		append_le(packet, room_id);
		append_le(packet, outgoing_sequence);
		packet.push_back(microphone_status_nl());
		send_packet_to_members_nl(packet);
		last_status_send = steady_clock::now();
	}

	void send_periodic_status_nl()
	{
		if (steady_clock::now() - last_status_send >= 1s)
		{
			send_mic_status_nl();
		}
	}

	u32 send_packet_to_members_nl(const std::vector<u8>& payload)
	{
		if (!joined)
		{
			return 0;
		}

		u32 sent = 0;
		auto& nph = g_fxo->get<named_thread<np::np_handler>>();
		auto& sigh = g_fxo->get<named_thread<signaling_handler>>();
		for (const u16 member_id : members)
		{
			if (member_id == local_member_id)
			{
				continue;
			}

			const auto [error, npid] = nph.local_get_npid(room_id, member_id);
			if (error != CELL_OK || !npid)
			{
				continue;
			}
			const auto conn_id = sigh.get_conn_id_from_npid(*npid);
			if (!conn_id)
			{
				continue;
			}
			const auto info = sigh.get_sig_infos(*conn_id);
			if (!info || info->conn_status != SCE_NP_SIGNALING_CONN_STATUS_ACTIVE || info->room_id != room_id)
			{
				continue;
			}

			std::vector<u8> packet(VPORT_0_HEADER_SIZE + payload.size());
			reinterpret_cast<le_t<u16>&>(packet[0]) = 0;
			packet[2] = SUBSET_AVC2;
			std::memcpy(packet.data() + VPORT_0_HEADER_SIZE, payload.data(), payload.size());

			sockaddr_in destination{};
			destination.sin_family = AF_INET;
			destination.sin_addr.s_addr = info->addr;
			destination.sin_port = std::bit_cast<u16, be_t<u16>>(info->port);
			if (np::is_ipv6_supported() && np::ip_address_translator::is_ipv6(destination.sin_addr.s_addr))
			{
				auto& translator = g_fxo->get<np::ip_address_translator>();
				const auto address6 = translator.get_ipv6_sockaddr(destination.sin_addr.s_addr, destination.sin_port);
				sent += send_packet_from_p2p_port_ipv6(packet, address6);
			}
			else
			{
				sent += send_packet_from_p2p_port_ipv4(packet, destination);
			}
		}
		return sent;
	}

	void process_network_nl()
	{
		for (signaling_message& message : get_avc2_msgs())
		{
			if (!joined || message.data.size() < avc2_common_header_size ||
				std::memcmp(message.data.data(), "AVC2", 4) != 0 || message.data[4] != avc2_wire_version)
			{
				continue;
			}

			u16 member_id = 0;
			u64 packet_room_id = 0;
			u16 sequence = 0;
			if (!read_le(message.data, 6, member_id) || !read_le(message.data, 8, packet_room_id) || !read_le(message.data, 16, sequence) || packet_room_id != room_id)
			{
				continue;
			}

			auto& nph = g_fxo->get<named_thread<np::np_handler>>();
			const auto [npid_error, npid] = nph.local_get_npid(room_id, member_id);
			auto& sigh = g_fxo->get<named_thread<signaling_handler>>();
			const auto conn_id = npid_error == CELL_OK && npid ? sigh.get_conn_id_from_npid(*npid) : std::nullopt;
			const auto info = conn_id ? sigh.get_sig_infos(*conn_id) : std::nullopt;
			if (!info || info->room_id != room_id || info->member_id != member_id || info->addr != message.src_addr ||
				info->port != message.src_port || !members.contains(member_id))
			{
				continue;
			}

			peer_state& peer = peers[member_id];
			if (peer.npid.empty())
			{
				peer.npid = np::npid_to_string(*npid);
			}

			const avc2_packet_type type = static_cast<avc2_packet_type>(message.data[5]);
			if (type == avc2_packet_type::mic_status)
			{
				if (message.data.size() > avc2_common_header_size)
				{
					const u8 new_status = message.data[avc2_common_header_size];
					if (new_status <= CELL_AVC2_MIC_STATUS_UNKNOWN && peer.mic_status != new_status)
					{
						peer.mic_status = new_status;
						if (mic_status_detection)
						{
							g_fxo->get<avc2_settings>().register_cb_call(CELL_AVC2_EVENT_SYSTEM_MIC_DETECTED, (static_cast<u64>(member_id) << 32) | new_status);
						}
					}
				}
				continue;
			}

			if (!voice_chat_enabled || type != avc2_packet_type::audio || message.data.size() < avc2_common_header_size + 5)
			{
				continue;
			}

			s16 predictor = 0;
			u16 sample_count = 0;
			if (!read_le(message.data, 18, predictor) || !read_le(message.data, 21, sample_count) || sample_count != avc2_frame_samples)
			{
				continue;
			}
			const u8 step_index = message.data[20];
			const usz encoded_offset = 23;
			if (message.data.size() < encoded_offset + avc2_encoded_frame_size)
			{
				continue;
			}

			peer.last_packet = steady_clock::now();

			// Late packet whose slot was already played or skipped
			if (peer.playback_started && static_cast<s16>(sequence - peer.next_sequence) < 0)
			{
				continue;
			}

			if (!peer.frames.contains(sequence))
			{
				peer.frames.emplace(sequence, decode_frame(predictor, step_index, message.data.data() + encoded_offset));
				if (!logged_first_receive.exchange(true))
				{
					cellSysutilAvc2.notice("AV Chat2 received the first voice frame (room=%llu, member=%u)", room_id, member_id);
				}
			}
		}
	}

	static auto oldest_frame(peer_state& peer)
	{
		auto oldest = peer.frames.begin();
		for (auto it = peer.frames.begin(); it != peer.frames.end(); ++it)
		{
			if (static_cast<s16>(it->first - oldest->first) < 0)
			{
				oldest = it;
			}
		}
		return oldest;
	}

	void process_playback_nl()
	{
		const auto now = steady_clock::now();

		for (auto& [member_id, peer] : peers)
		{
			if (!peer.playback_started)
			{
				if (peer.frames.empty())
				{
					continue;
				}

				if (peer.frames.size() >= avc2_jitter_prebuffer_frames || now - peer.last_packet > 60ms)
				{
					peer.next_sequence = oldest_frame(peer)->first;
					peer.playback_started = true;
				}
				continue;
			}

			while (peer.frames.size() > avc2_jitter_max_frames)
			{
				const auto oldest = oldest_frame(peer);
				peer.next_sequence = static_cast<u16>(oldest->first + 1);
				peer.frames.erase(oldest);
			}
		}

		while (output_audio.size() < avc2_output_target_samples * sizeof(f32))
		{
			std::array<f32, avc2_frame_samples> mixed{};
			u32 active_speakers = 0;
			bool any_waiting = false;

			for (auto& [member_id, peer] : peers)
			{
				if (!peer.playback_started)
				{
					continue;
				}

				auto frame_it = peer.frames.find(peer.next_sequence);
				if (frame_it == peer.frames.end())
				{
					if (peer.frames.empty())
					{
						// End of the talk spurt
						if (now - peer.last_packet > 200ms)
						{
							peer.playback_started = false;
							maybe_emit_voice_event_nl(member_id, 0, peer.talking, peer.last_event);
						}
						else
						{
							any_waiting = true;
						}
						continue;
					}

					// Lost packet
					frame_it = oldest_frame(peer);
				}

				const auto frame = frame_it->second;
				peer.next_sequence = static_cast<u16>(frame_it->first + 1);
				peer.frames.erase(frame_it);
				peer.level = signal_level(frame);
				maybe_emit_voice_event_nl(member_id, peer.level, peer.talking, peer.last_event);

				if (muted_players.contains(member_id) || user_muted_npids.contains(peer.npid))
				{
					continue;
				}
				for (u32 i = 0; i < avc2_frame_samples; i++)
				{
					mixed[i] += static_cast<f32>(frame[i]) / 32768.0f;
				}
				active_speakers++;
			}

			if (!active_speakers)
			{
				// Either nobody is talking or everyone's next frame is still in flight
				if (!any_waiting)
				{
					last_output_sample = 0.0f;
				}
				break;
			}

			const f32 normalization = 1.0f / std::sqrt(static_cast<f32>(active_speakers));
			std::array<f32, avc2_frame_samples * 3> resampled{};
			f32 previous = last_output_sample;
			for (u32 i = 0; i < avc2_frame_samples; i++)
			{
				const f32 current = std::clamp(mixed[i] * normalization, -1.0f, 1.0f);
				resampled[i * 3] = previous + (current - previous) * (1.0f / 3.0f);
				resampled[i * 3 + 1] = previous + (current - previous) * (2.0f / 3.0f);
				resampled[i * 3 + 2] = current;
				previous = current;
			}
			last_output_sample = previous;

			output_audio.push_overwrite(resampled.data(), resampled.size() * sizeof(f32));
		}
	}

	void maybe_emit_voice_event_nl(u16 member_id, u32 level, bool& talking, steady_clock::time_point& last_event)
	{
		if (!voice_detection || !streaming || !member_id)
		{
			return;
		}

		const auto now = steady_clock::now();
		if (voice_detect_event_type == 0)
		{
			if (last_event.time_since_epoch().count() != 0 && now - last_event < std::chrono::milliseconds(voice_detect_interval))
			{
				return;
			}
			last_event = now;
			g_fxo->get<avc2_settings>().register_cb_call(CELL_AVC2_EVENT_SYSTEM_VOICE_DETECTED, (static_cast<u64>(member_id) << 32) | level);
			return;
		}

		const bool new_talking = level >= voice_detect_threshold;
		if (new_talking != talking)
		{
			talking = new_talking;
			last_event = now;
			g_fxo->get<avc2_settings>().register_cb_call(CELL_AVC2_EVENT_SYSTEM_VOICE_DETECTED, (static_cast<u64>(member_id) << 32) | static_cast<u32>(talking));
		}
	}

	void refresh_members_nl(bool force = false)
	{
		if (!joined || (!force && steady_clock::now() - last_member_refresh < 250ms))
		{
			return;
		}
		last_member_refresh = steady_clock::now();

		auto& nph = g_fxo->get<named_thread<np::np_handler>>();
		const auto [error, current] = nph.local_get_room_memberids(room_id, SCE_NP_MATCHING2_SORT_METHOD_SLOT_NUMBER);
		if (error != CELL_OK)
		{
			return;
		}

		const std::set<u16> updated(current.begin(), current.end());
		for (const u16 member_id : updated)
		{
			if (member_id != local_member_id && !members.contains(member_id))
			{
				g_fxo->get<avc2_settings>().register_cb_call(CELL_AVC2_EVENT_SYSTEM_NEW_MEMBER_JOINED, member_id);
				g_fxo->get<avc2_settings>().register_cb_call(CELL_AVC2_EVENT_SYSTEM_SESSION_ESTABLISHED, member_id);
			}
		}
		for (const u16 member_id : members)
		{
			if (member_id != local_member_id && !updated.contains(member_id))
			{
				g_fxo->get<avc2_settings>().register_cb_call(CELL_AVC2_EVENT_SYSTEM_MEMBER_LEFT, member_id);
				peers.erase(member_id);
			}
		}
		members = updated;
	}
};

using avc2_voice_thread = named_thread<avc2_voice_engine>;

void cellSysutilAvc2MixVoice(f32* out_buffer, u32 sample_count, u32 channel_count, f32 master_volume)
{
	if (auto engine = g_fxo->try_get<avc2_voice_thread>())
	{
		engine->mix(out_buffer, sample_count, channel_count, master_volume);
	}
}

std::vector<avc2_voice_chat_member> cellSysutilAvc2GetVoiceChatMembers()
{
	if (auto engine = g_fxo->try_get<avc2_voice_thread>())
	{
		return engine->get_voice_chat_members();
	}
	return {};
}

void cellSysutilAvc2SetVoiceChatMemberMuting(const std::string& npid, bool muted)
{
	if (auto engine = g_fxo->try_get<avc2_voice_thread>())
	{
		cellSysutilAvc2.notice("User %s voice chat member %s", muted ? "muted" : "unmuted", npid);
		engine->set_user_muting(npid, muted);
	}
}

static bool avc2_is_room_member(u64 room_id, SceNpMatching2RoomMemberId member_id)
{
	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	const auto [error, member_ids] = nph.local_get_room_memberids(room_id, SCE_NP_MATCHING2_SORT_METHOD_SLOT_NUMBER);
	return error == CELL_OK && std::find(member_ids.begin(), member_ids.end(), member_id) != member_ids.end();
}

error_code cellSysutilAvc2GetPlayerInfo(vm::cptr<SceNpMatching2RoomMemberId> player_id, vm::ptr<CellSysutilAvc2PlayerInfo> player_info)
{
	cellSysutilAvc2.trace("cellSysutilAvc2GetPlayerInfo(player_id=*0x%x, player_info=*0x%x)", player_id, player_info);

	if (!player_id || !player_info)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;
	const auto& settings = g_fxo->get<avc2_settings>();
	if (!settings.loaded)
		return CELL_AVC2_ERROR_NOT_INITIALIZED;
	if (!settings.joined)
		return CELL_AVC2_ERROR_NO_SESSION;

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	const auto [error, member_ids] = nph.local_get_room_memberids(settings.room_id, SCE_NP_MATCHING2_SORT_METHOD_SLOT_NUMBER);
	if (error != CELL_OK || std::find(member_ids.begin(), member_ids.end(), static_cast<u16>(*player_id)) == member_ids.end())
		return CELL_AVC2_ERROR_ERROR_BAD_ID;

	player_info->connected = 0;
	if (static_cast<u16>(*player_id) == settings.local_member_id)
	{
		player_info->connected = 1;
	}
	else if (const auto [np_error, npid] = nph.local_get_npid(settings.room_id, *player_id); np_error == CELL_OK && npid)
	{
		auto& sigh = g_fxo->get<named_thread<signaling_handler>>();
		if (const auto conn_id = sigh.get_conn_id_from_npid(*npid))
		{
			const auto info = sigh.get_sig_infos(*conn_id);
			player_info->connected = info && info->conn_status == SCE_NP_SIGNALING_CONN_STATUS_ACTIVE;
		}
	}
	player_info->joined = 1;
	player_info->mic_attached = g_fxo->get<avc2_voice_thread>().player_microphone_status(*player_id);
	player_info->member_id = *player_id;

	return CELL_OK;
}

error_code cellSysutilAvc2JoinChat(vm::cptr<SceNpMatching2RoomId> room_id, vm::ptr<CellSysutilAvc2EventId> eventId, vm::ptr<CellSysutilAvc2EventParam> eventParam)
{
	cellSysutilAvc2.warning("cellSysutilAvc2JoinChat(room_id=*0x%x, eventId=*0x%x, eventParam=*0x%x)", room_id, eventId, eventParam);

	// NOTE: room_id should be null if the current mode is Direct WAN/LAN

	auto& settings = g_fxo->get<avc2_settings>();

	if (!settings.loaded)
		return CELL_AVC2_ERROR_NOT_INITIALIZED;

	u64 id = 0;

	if (room_id)
	{
		id = *room_id;
	}
	else if (settings.streaming_mode == CELL_SYSUTIL_AVC2_STREAMING_MODE_NORMAL)
	{
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;
	}

	const bool success = g_fxo->get<avc2_voice_thread>().join_room(id);
	settings.joined = success;
	settings.room_id = success ? id : 0;
	if (success)
	{
		auto& nph = g_fxo->get<named_thread<np::np_handler>>();
		settings.local_member_id = nph.local_get_memberid(id, nph.get_npid()).value_or(0);
	}
	if (eventId) *eventId = success ? CELL_AVC2_EVENT_JOIN_SUCCEEDED : CELL_AVC2_EVENT_JOIN_FAILED;
	if (eventParam) *eventParam = success ? 0 : CELL_AVC2_EVENT_PARAM_ERROR_ROOM_DOES_NOT_EXIST;

	if (!success)
		return CELL_AVC2_ERROR_NO_SESSION;
	return CELL_OK;
}

error_code cellSysutilAvc2StopStreaming()
{
	cellSysutilAvc2.notice("cellSysutilAvc2StopStreaming()");
	auto& settings = g_fxo->get<avc2_settings>();
	if (!settings.loaded)
		return CELL_AVC2_ERROR_NOT_INITIALIZED;
	settings.streaming = false;
	g_fxo->get<avc2_voice_thread>().set_streaming(false);
	return CELL_OK;
}

error_code cellSysutilAvc2ChangeVideoResolution(u32 resolution)
{
	cellSysutilAvc2.todo("cellSysutilAvc2ChangeVideoResolution(resolution=0x%x)", resolution);
	return CELL_OK;
}

error_code cellSysutilAvc2ShowScreen()
{
	cellSysutilAvc2.todo("cellSysutilAvc2ShowScreen()");
	return CELL_OK;
}

error_code cellSysutilAvc2GetVideoMuting(vm::ptr<u8> muting)
{
	cellSysutilAvc2.todo("cellSysutilAvc2GetVideoMuting(muting=*0x%x)", muting);

	if (!muting)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	const auto& settings = g_fxo->get<avc2_settings>();
	*muting = settings.video_muting;

	return CELL_OK;
}

error_code cellSysutilAvc2GetWindowAttribute(SceNpMatching2RoomMemberId member_id, vm::ptr<CellSysutilAvc2WindowAttribute> attr)
{
	cellSysutilAvc2.todo("cellSysutilAvc2GetWindowAttribute(member_id=0x%x, attr=*0x%x)", member_id, attr);

	if (!attr)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	switch (attr->attr_id)
	{
	case CELL_SYSUTIL_AVC2_WINDOW_ATTRIBUTE_ALPHA:
		break;
	case CELL_SYSUTIL_AVC2_WINDOW_ATTRIBUTE_TRANSITION_TYPE:
		break;
	case CELL_SYSUTIL_AVC2_WINDOW_ATTRIBUTE_TRANSITION_DURATION:
		break;
	case CELL_SYSUTIL_AVC2_WINDOW_ATTRIBUTE_STRING_VISIBLE:
		break;
	case CELL_SYSUTIL_AVC2_WINDOW_ATTRIBUTE_ROTATION:
		break;
	case CELL_SYSUTIL_AVC2_WINDOW_ATTRIBUTE_ZORDER:
		break;
	case CELL_SYSUTIL_AVC2_WINDOW_ATTRIBUTE_SURFACE:
		break;
	default:
		break;
	}

	return CELL_OK;
}

error_code cellSysutilAvc2StopStreaming2(u32 mediaType)
{
	cellSysutilAvc2.notice("cellSysutilAvc2StopStreaming2(mediaType=0x%x)", mediaType);

	if (mediaType != CELL_SYSUTIL_AVC2_VOICE_CHAT && mediaType != CELL_SYSUTIL_AVC2_VIDEO_CHAT)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	if (mediaType == CELL_SYSUTIL_AVC2_VOICE_CHAT)
		return cellSysutilAvc2StopStreaming();
	return CELL_OK;
}

error_code cellSysutilAvc2SetVoiceMuting(u8 muting)
{
	cellSysutilAvc2.notice("cellSysutilAvc2SetVoiceMuting(muting=0x%x)", muting);

	auto& settings = g_fxo->get<avc2_settings>();
	if (!settings.loaded)
		return CELL_AVC2_ERROR_NOT_INITIALIZED;
	if (muting > 1)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;
	settings.voice_muting = muting;
	g_fxo->get<avc2_voice_thread>().set_voice_muting(muting != 0);

	return CELL_OK;
}

error_code cellSysutilAvc2StartVoiceDetection()
{
	cellSysutilAvc2.notice("cellSysutilAvc2StartVoiceDetection()");
	auto& settings = g_fxo->get<avc2_settings>();
	if (!settings.loaded)
		return CELL_AVC2_ERROR_NOT_INITIALIZED;
	settings.voice_detection = true;
	g_fxo->get<avc2_voice_thread>().set_voice_detection(true);
	return CELL_OK;
}

error_code cellSysutilAvc2StopVoiceDetection()
{
	cellSysutilAvc2.notice("cellSysutilAvc2StopVoiceDetection()");
	auto& settings = g_fxo->get<avc2_settings>();
	if (!settings.loaded)
		return CELL_AVC2_ERROR_NOT_INITIALIZED;
	settings.voice_detection = false;
	g_fxo->get<avc2_voice_thread>().set_voice_detection(false);
	return CELL_OK;
}

error_code cellSysutilAvc2GetAttribute(vm::ptr<CellSysutilAvc2Attribute> attr)
{
	cellSysutilAvc2.trace("cellSysutilAvc2GetAttribute(attr=*0x%x)", attr);

	if (!attr)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	const auto& settings = g_fxo->get<avc2_settings>();
	if (!settings.loaded)
		return CELL_AVC2_ERROR_NOT_INITIALIZED;

	switch (attr->attr_id)
	{
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_DETECT_EVENT_TYPE:
		attr->attr_param.int_param = settings.voice_detect_event_type;
		break;
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_DETECT_INTERVAL_TIME:
		attr->attr_param.int_param = settings.voice_detect_interval_time;
		break;
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_DETECT_SIGNAL_LEVEL:
		attr->attr_param.int_param = settings.voice_detect_signal_level;
		break;
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_MAX_BITRATE:
		attr->attr_param.int_param = settings.voice_max_bitrate;
		break;
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_DATA_FEC:
		attr->attr_param.int_param = settings.voice_data_fec;
		break;
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_PACKET_CONTENTION:
		attr->attr_param.int_param = settings.voice_packet_contention;
		break;
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_DTX_MODE:
		attr->attr_param.int_param = settings.voice_dtx_mode;
		break;
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_MIC_STATUS_DETECTION:
		attr->attr_param.int_param = settings.mic_status_detection;
		break;
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_MIC_SETTING_NOTIFICATION:
		attr->attr_param.int_param = settings.mic_setting_notification;
		break;
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_MUTING_NOTIFICATION:
		attr->attr_param.int_param = settings.voice_muting_notification;
		break;
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_CAMERA_STATUS_DETECTION:
		attr->attr_param.int_param = settings.camera_status_detection;
		break;
	default:
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code cellSysutilAvc2SetSpeakerVolumeLevel(f32 level)
{
	cellSysutilAvc2.notice("cellSysutilAvc2SetSpeakerVolumeLevel(level=%f)", level);

	auto& settings = g_fxo->get<avc2_settings>();
	if (!settings.loaded)
		return CELL_AVC2_ERROR_NOT_INITIALIZED;
	if (!std::isfinite(level) || level < 0.0f || level > 40.0f)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;
	settings.speaker_volume_level = level;
	g_fxo->get<avc2_voice_thread>().set_speaker(settings.speaker_muting, level);

	return CELL_OK;
}

error_code cellSysutilAvc2SetWindowString(SceNpMatching2RoomMemberId member_id, vm::cptr<char> string)
{
	cellSysutilAvc2.todo("cellSysutilAvc2SetWindowString(member_id=0x%x, string=%s)", member_id, string);

	if (!string || std::strlen(string.get_ptr()) >= 64)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	return CELL_OK;
}

error_code cellSysutilAvc2EstimateMemoryContainerSize(vm::cptr<CellSysutilAvc2InitParam> initparam, vm::ptr<u32> size)
{
	cellSysutilAvc2.todo("cellSysutilAvc2EstimateMemoryContainerSize(initparam=*0x%x, size=*0x%x)", initparam, size);

	if (!initparam || !size)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	switch (initparam->avc_init_param_version)
	{
	case 100:
	{
		*size = 0x400000;
		break;
	}
	case 110:
	case 120:
	case 130:
	case 140:
	{
		if (initparam->media_type == CELL_SYSUTIL_AVC2_VOICE_CHAT)
		{
			*size = 0x300000;
		}
		else if (initparam->media_type == CELL_SYSUTIL_AVC2_VIDEO_CHAT)
		{
			u32 estimated_size = 0x40e666;
			u32 max_windows    = initparam->video_param.max_video_windows;
			s32 window_count   = max_windows;

			if (initparam->video_param.video_stream_sharing == CELL_SYSUTIL_AVC2_VIDEO_SHARING_MODE_2)
			{
				window_count++;
			}

			if (initparam->video_param.max_video_resolution == CELL_SYSUTIL_AVC2_VIDEO_RESOLUTION_QQVGA)
			{
				estimated_size = (static_cast<u32>(window_count) * 0x12c00 & 0xfff00000) + 0x50e666;
			}
			else if (initparam->video_param.max_video_resolution == CELL_SYSUTIL_AVC2_VIDEO_RESOLUTION_QVGA)
			{
				estimated_size += (static_cast<u32>(window_count) * 0x4b000 & 0xfff00000) + 0x100000;
			}

			if (initparam->video_param.frame_mode == CELL_SYSUTIL_AVC2_FRAME_MODE_NORMAL)
			{
				window_count = max_windows - 1;
			}
			else
			{
				window_count = 1;
			}

			u32 val = max_windows * 10000;

			if (initparam->video_param.max_video_resolution == CELL_SYSUTIL_AVC2_VIDEO_RESOLUTION_QQVGA)
			{
				val += window_count * 0x96000 + 0x10c9e0; // 0x96000 = 160x120x32
			}
			else
			{
				val += static_cast<s32>(static_cast<f64>(window_count) * 1258291.2) + 0x1ed846;
			}

			estimated_size = ((estimated_size + ((static_cast<int>(val) >> 7) + static_cast<u32>(static_cast<int>(val) < 0 && (val & 0x7f) != 0)) * 0x80 + 0x80080) & 0xfff00000) + 0x100000;

			*size = estimated_size;
		}
		else
		{
			*size = 0;
			return CELL_AVC2_ERROR_INVALID_ARGUMENT;
		}
		break;
	}
	default:
	{
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;
	}
	}

	return CELL_OK;
}

error_code cellSysutilAvc2SetVideoMuting(u8 muting)
{
	cellSysutilAvc2.todo("cellSysutilAvc2SetVideoMuting(muting=0x%x)", muting);

	if (muting > 1) // Weird check, lol
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	auto& settings = g_fxo->get<avc2_settings>();
	settings.video_muting = muting;

	return CELL_OK;
}

error_code cellSysutilAvc2SetPlayerVoiceMuting(SceNpMatching2RoomMemberId member_id, u8 muting)
{
	cellSysutilAvc2.trace("cellSysutilAvc2SetPlayerVoiceMuting(member_id=0x%x, muting=0x%x)", member_id, muting);

	auto& settings = g_fxo->get<avc2_settings>();
	if (!settings.loaded)
		return CELL_AVC2_ERROR_NOT_INITIALIZED;
	if (!settings.joined)
		return CELL_AVC2_ERROR_NO_SESSION;
	if (muting > 1)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;
	if (!avc2_is_room_member(settings.room_id, member_id))
		return CELL_AVC2_ERROR_ERROR_BAD_ID;

	if (muting ? settings.voice_muting_players.insert(member_id).second : settings.voice_muting_players.erase(member_id) != 0)
	{
		cellSysutilAvc2.notice("cellSysutilAvc2SetPlayerVoiceMuting: member_id=0x%x is now %s", member_id, muting ? "muted" : "unmuted");
	}
	g_fxo->get<avc2_voice_thread>().set_player_muting(member_id, muting != 0);

	return CELL_OK;
}

error_code cellSysutilAvc2SetStreamingTarget(vm::cptr<CellSysutilAvc2StreamingTarget> target)
{
	cellSysutilAvc2.todo("cellSysutilAvc2SetStreamingTarget(target=*0x%x)", target);

	return CELL_OK;
}

error_code cellSysutilAvc2Unload()
{
	cellSysutilAvc2.notice("cellSysutilAvc2Unload()");

	auto& settings = g_fxo->get<avc2_settings>();

	std::lock_guard lock(settings.mutex_cb);

	if (!settings.avc2_cb)
	{
		return CELL_AVC2_ERROR_NOT_INITIALIZED;
	}

	g_fxo->get<avc2_voice_thread>().unload();
	settings.loaded = false;
	settings.joined = false;
	settings.streaming = false;
	settings.voice_detection = false;
	settings.room_id = 0;
	settings.local_member_id = 0;

	sysutil_unregister_cb_with_id<avc2_cb_handle_t>();
	settings.avc2_cb = vm::null;
	settings.avc2_cb_arg = vm::null;

	return CELL_OK;
}

error_code cellSysutilAvc2UnloadAsync()
{
	cellSysutilAvc2.notice("cellSysutilAvc2UnloadAsync()");

	auto& settings = g_fxo->get<avc2_settings>();
	if (!settings.loaded)
		return CELL_AVC2_ERROR_NOT_INITIALIZED;
	g_fxo->get<avc2_voice_thread>().unload();
	settings.loaded = false;
	settings.joined = false;
	settings.streaming = false;
	settings.voice_detection = false;
	settings.room_id = 0;
	settings.local_member_id = 0;
	settings.register_cb_call(CELL_AVC2_EVENT_UNLOAD_SUCCEEDED, 0);

	return CELL_OK;
}

error_code cellSysutilAvc2DestroyWindow(SceNpMatching2RoomMemberId member_id)
{
	cellSysutilAvc2.todo("cellSysutilAvc2DestroyWindow(member_id=0x%x)", member_id);
	return CELL_OK;
}

error_code cellSysutilAvc2SetWindowPosition(SceNpMatching2RoomMemberId member_id, f32 x, f32 y, f32 z)
{
	cellSysutilAvc2.todo("cellSysutilAvc2SetWindowPosition(member_id=0x%x, x=0x%x, y=0x%x, z=0x%x)", member_id, x, y, z);
	return CELL_OK;
}

error_code cellSysutilAvc2GetSpeakerVolumeLevel(vm::ptr<f32> level)
{
	cellSysutilAvc2.trace("cellSysutilAvc2GetSpeakerVolumeLevel(level=*0x%x)", level);

	if (!level)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	const auto& settings = g_fxo->get<avc2_settings>();
	if (!settings.loaded)
		return CELL_AVC2_ERROR_NOT_INITIALIZED;
	*level = settings.speaker_volume_level;

	return CELL_OK;
}

error_code cellSysutilAvc2IsCameraAttached(vm::ptr<u8> status)
{
	cellSysutilAvc2.todo("cellSysutilAvc2IsCameraAttached(status=*0x%x)", status);

	if (!status)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	*status = CELL_AVC2_CAMERA_STATUS_DETACHED;

	return CELL_OK;
}

error_code cellSysutilAvc2MicRead(vm::ptr<void> ptr, vm::ptr<u32> pSize)
{
	cellSysutilAvc2.trace("cellSysutilAvc2MicRead(ptr=*0x%x, pSize=*0x%x)", ptr, pSize);

	const auto& settings = g_fxo->get<avc2_settings>();
	if (!settings.loaded)
		return CELL_AVC2_ERROR_NOT_INITIALIZED;

	if (!settings.mic_out_stream_sharing)
		return CELL_OK;

	if (!ptr || !pSize)
	{
		// Not checked on real hardware
		cellSysutilAvc2.warning("cellSysutilAvc2MicRead: ptr or pSize is null");

		if (pSize)
		{
			*pSize = 0;
		}

		return CELL_OK;
	}

	*pSize = g_fxo->get<avc2_voice_thread>().read_shared_mic(ptr.get_ptr(), *pSize);

	return CELL_OK;
}

error_code cellSysutilAvc2GetPlayerVoiceMuting(SceNpMatching2RoomMemberId member_id, vm::ptr<u8> muting)
{
	cellSysutilAvc2.trace("cellSysutilAvc2GetPlayerVoiceMuting(member_id=0x%x, muting=*0x%x)", member_id, muting);

	if (!muting)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	const auto& settings = g_fxo->get<avc2_settings>();
	if (!settings.loaded)
		return CELL_AVC2_ERROR_NOT_INITIALIZED;
	if (!settings.joined)
		return CELL_AVC2_ERROR_NO_SESSION;
	if (!avc2_is_room_member(settings.room_id, member_id))
		return CELL_AVC2_ERROR_ERROR_BAD_ID;

	*muting = g_fxo->get<avc2_voice_thread>().is_player_muted(member_id);

	return CELL_OK;
}

error_code cellSysutilAvc2JoinChatRequest(vm::cptr<SceNpMatching2RoomId> room_id)
{
	cellSysutilAvc2.warning("cellSysutilAvc2JoinChatRequest(room_id=*0x%x)", room_id);

	// NOTE: room_id should be null if the current mode is Direct WAN/LAN

	auto& settings = g_fxo->get<avc2_settings>();

	if (!settings.loaded)
		return CELL_AVC2_ERROR_NOT_INITIALIZED;
	if (settings.joined)
		return CELL_AVC2_ERROR_INVALID_STATUS;

	u64 id = 0;

	if (room_id)
	{
		id = *room_id;
	}
	else if (settings.streaming_mode == CELL_SYSUTIL_AVC2_STREAMING_MODE_NORMAL)
	{
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;
	}

	const bool success = g_fxo->get<avc2_voice_thread>().join_room(id);
	settings.joined = success;
	settings.room_id = success ? id : 0;
	if (success)
	{
		auto& nph = g_fxo->get<named_thread<np::np_handler>>();
		settings.local_member_id = nph.local_get_memberid(id, nph.get_npid()).value_or(0);
	}
	settings.register_cb_call(success ? CELL_AVC2_EVENT_JOIN_SUCCEEDED : CELL_AVC2_EVENT_JOIN_FAILED,
		success ? 0 : CELL_AVC2_EVENT_PARAM_ERROR_ROOM_DOES_NOT_EXIST);

	return CELL_OK;
}

error_code cellSysutilAvc2StartStreaming()
{
	cellSysutilAvc2.notice("cellSysutilAvc2StartStreaming()");
	auto& settings = g_fxo->get<avc2_settings>();
	if (!settings.loaded)
		return CELL_AVC2_ERROR_NOT_INITIALIZED;
	settings.streaming = true;
	g_fxo->get<avc2_voice_thread>().set_streaming(true);
	return CELL_OK;
}

error_code cellSysutilAvc2SetWindowAttribute(SceNpMatching2RoomMemberId member_id, vm::cptr<CellSysutilAvc2WindowAttribute> attr)
{
	cellSysutilAvc2.todo("cellSysutilAvc2SetWindowAttribute(member_id=0x%x, attr=*0x%x)", member_id, attr);

	if (!attr)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	switch (attr->attr_id)
	{
	case CELL_SYSUTIL_AVC2_WINDOW_ATTRIBUTE_ALPHA:
		break;
	case CELL_SYSUTIL_AVC2_WINDOW_ATTRIBUTE_TRANSITION_TYPE:
		break;
	case CELL_SYSUTIL_AVC2_WINDOW_ATTRIBUTE_TRANSITION_DURATION:
		break;
	case CELL_SYSUTIL_AVC2_WINDOW_ATTRIBUTE_STRING_VISIBLE:
		break;
	case CELL_SYSUTIL_AVC2_WINDOW_ATTRIBUTE_ROTATION:
		break;
	case CELL_SYSUTIL_AVC2_WINDOW_ATTRIBUTE_ZORDER:
		break;
	case CELL_SYSUTIL_AVC2_WINDOW_ATTRIBUTE_SURFACE:
		break;
	default:
		break;
	}

	return CELL_OK;
}

error_code cellSysutilAvc2GetWindowShowStatus(SceNpMatching2RoomMemberId member_id, vm::ptr<u8> visible)
{
	cellSysutilAvc2.todo("cellSysutilAvc2GetWindowShowStatus(member_id=0x%x, visible=*0x%x)", member_id, visible);

	if (!visible)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	*visible = 0;

	return CELL_OK;
}

error_code cellSysutilAvc2InitParam(u16 version, vm::ptr<CellSysutilAvc2InitParam> option)
{
	cellSysutilAvc2.todo("cellSysutilAvc2InitParam(version=%d, option=*0x%x)", version, option);

	if (!option)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	*option = {};
	option->avc_init_param_version = version;

	switch (version)
	{
	case 100:
	case 110:
	case 120:
	case 130:
	case 140:
		break;
	default:
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code cellSysutilAvc2GetWindowSize(SceNpMatching2RoomMemberId member_id, vm::ptr<f32> width, vm::ptr<f32> height)
{
	cellSysutilAvc2.todo("cellSysutilAvc2GetWindowSize(member_id=0x%x, width=*0x%x, height=*0x%x)", member_id, width, height);

	if (!width || !height)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	return CELL_OK;
}

error_code cellSysutilAvc2SetStreamPriority(u8 priority)
{
	cellSysutilAvc2.todo("cellSysutilAvc2SetStreamPriority(priority=0x%x)", priority);
	if (priority > 15)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;
	return CELL_OK;
}

error_code cellSysutilAvc2LeaveChatRequest()
{
	cellSysutilAvc2.notice("cellSysutilAvc2LeaveChatRequest()");

	auto& settings = g_fxo->get<avc2_settings>();
	if (!settings.loaded)
		return CELL_AVC2_ERROR_NOT_INITIALIZED;
	if (!settings.joined)
		return CELL_AVC2_ERROR_NO_SESSION;
	g_fxo->get<avc2_voice_thread>().leave();
	settings.voice_muting_players.clear();
	settings.joined = false;
	settings.room_id = 0;
	settings.local_member_id = 0;
	settings.register_cb_call(CELL_AVC2_EVENT_LEAVE_SUCCEEDED, 0);

	return CELL_OK;
}

error_code cellSysutilAvc2IsMicAttached(vm::ptr<u8> status)
{
	cellSysutilAvc2.trace("cellSysutilAvc2IsMicAttached(status=*0x%x)", status);

	if (!status)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;
	if (!g_fxo->get<avc2_settings>().loaded)
		return CELL_AVC2_ERROR_NOT_INITIALIZED;

	*status = g_fxo->get<avc2_voice_thread>().microphone_status();

	return CELL_OK;
}

error_code cellSysutilAvc2CreateWindow(SceNpMatching2RoomMemberId member_id)
{
	cellSysutilAvc2.todo("cellSysutilAvc2CreateWindow(member_id=0x%x)", member_id);
	return CELL_OK;
}

error_code cellSysutilAvc2GetSpeakerMuting(vm::ptr<u8> muting)
{
	cellSysutilAvc2.todo("cellSysutilAvc2GetSpeakerMuting(muting=*0x%x)", muting);

	if (!muting)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	const auto& settings = g_fxo->get<avc2_settings>();
	if (!settings.loaded)
		return CELL_AVC2_ERROR_NOT_INITIALIZED;
	*muting = settings.speaker_muting;

	return CELL_OK;
}

error_code cellSysutilAvc2ShowWindow(SceNpMatching2RoomMemberId member_id)
{
	cellSysutilAvc2.todo("cellSysutilAvc2ShowWindow(member_id=0x%x)", member_id);
	return CELL_OK;
}

error_code cellSysutilAvc2SetWindowSize(SceNpMatching2RoomMemberId member_id, f32 width, f32 height)
{
	cellSysutilAvc2.todo("cellSysutilAvc2SetWindowSize(member_id=0x%x, width=0x%x, height=0x%x)", member_id, width, height);
	return CELL_OK;
}

error_code cellSysutilAvc2EnumPlayers(vm::ptr<s32> players_num, vm::ptr<SceNpMatching2RoomMemberId> players_id)
{
	cellSysutilAvc2.trace("cellSysutilAvc2EnumPlayers(players_num=*0x%x, players_id=*0x%x)", players_num, players_id);

	if (!players_num)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;
	const auto& settings = g_fxo->get<avc2_settings>();
	if (!settings.loaded)
		return CELL_AVC2_ERROR_NOT_INITIALIZED;
	if (!settings.joined)
		return CELL_AVC2_ERROR_NO_SESSION;

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();
	const auto [error, member_ids] = nph.local_get_room_memberids(settings.room_id, SCE_NP_MATCHING2_SORT_METHOD_SLOT_NUMBER);
	if (error != CELL_OK)
		return CELL_AVC2_ERROR_NO_SESSION;

	// Apparently this function is supposed to be called twice.
	// Once with null to get the player count and then again to fill the ID list.
	if (players_id)
	{
		const s32 count = std::min<s32>(std::max<s32>(*players_num, 0), ::narrow<s32>(member_ids.size()));
		for (s32 i = 0; i < count; i++)
		{
			players_id[i] = member_ids[i];
		}
		*players_num = count;
	}
	else
	{
		*players_num = ::narrow<s32>(member_ids.size());
	}

	return CELL_OK;
}

error_code cellSysutilAvc2GetWindowString(SceNpMatching2RoomMemberId member_id, vm::ptr<char> string, vm::ptr<u8> len)
{
	cellSysutilAvc2.todo("cellSysutilAvc2GetWindowString(member_id=0x%x, string=*0x%x, len=*0x%x)", member_id, string, len);

	if (!string || !len)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	return CELL_OK;
}

error_code cellSysutilAvc2LeaveChat()
{
	cellSysutilAvc2.notice("cellSysutilAvc2LeaveChat()");
	auto& settings = g_fxo->get<avc2_settings>();
	if (!settings.loaded)
		return CELL_AVC2_ERROR_NOT_INITIALIZED;
	if (!settings.joined)
		return CELL_AVC2_ERROR_NO_SESSION;
	g_fxo->get<avc2_voice_thread>().leave();
	settings.voice_muting_players.clear();
	settings.joined = false;
	settings.room_id = 0;
	settings.local_member_id = 0;
	return CELL_OK;
}

error_code cellSysutilAvc2SetSpeakerMuting(u8 muting)
{
	cellSysutilAvc2.notice("cellSysutilAvc2SetSpeakerMuting(muting=0x%x)", muting);

	auto& settings = g_fxo->get<avc2_settings>();
	if (!settings.loaded)
		return CELL_AVC2_ERROR_NOT_INITIALIZED;
	if (muting > 1)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;
	settings.speaker_muting = muting;
	g_fxo->get<avc2_voice_thread>().set_speaker(muting != 0, settings.speaker_volume_level);

	return CELL_OK;
}

error_code cellSysutilAvc2Load_shared(SceNpMatching2ContextId ctx_id, u32 /*container*/, vm::ptr<CellSysutilAvc2Callback> callback_func, vm::ptr<void> user_data, vm::cptr<CellSysutilAvc2InitParam> init_param)
{
	if (!init_param || !init_param->avc_init_param_version ||
	    !(init_param->avc_init_param_version == 100 ||
	     init_param->avc_init_param_version == 110 ||
	     init_param->avc_init_param_version == 120 ||
	     init_param->avc_init_param_version == 130 ||
	     init_param->avc_init_param_version == 140)
	)
	{
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;
	}

	auto& settings = g_fxo->get<avc2_settings>();

	switch (init_param->media_type)
	{
	case CELL_SYSUTIL_AVC2_VOICE_CHAT:
	{
		if (init_param->max_players < 2 ||
		    init_param->max_players > 64 ||
		    init_param->spu_load_average > 100 ||
		    init_param->voice_param.voice_quality != CELL_SYSUTIL_AVC2_VOICE_QUALITY_NORMAL ||
		    init_param->voice_param.max_speakers == 0 ||
		    init_param->voice_param.max_speakers > 16
		)
		{
			return CELL_AVC2_ERROR_INVALID_ARGUMENT;
		}

		u32 streaming_mode = init_param->direct_streaming_mode;

		if (init_param->avc_init_param_version >= 120)
		{
			switch (init_param->direct_streaming_mode)
			{
			case CELL_SYSUTIL_AVC2_STREAMING_MODE_NORMAL:
				streaming_mode = CELL_SYSUTIL_AVC2_STREAMING_MODE_NORMAL;
				break;
			case CELL_SYSUTIL_AVC2_STREAMING_MODE_DIRECT_WAN:
				break;
			case CELL_SYSUTIL_AVC2_STREAMING_MODE_DIRECT_LAN:
				if (init_param->streaming_mode.mode == CELL_SYSUTIL_AVC2_STREAMING_MODE_NORMAL)
				{
					settings.streaming_mode = streaming_mode;
					return CELL_AVC2_ERROR_INVALID_ARGUMENT;
				}
				break;
			default:
				return CELL_AVC2_ERROR_INVALID_ARGUMENT;
			}
		}
		else if (init_param->avc_init_param_version >= 110)
		{
			switch (init_param->direct_streaming_mode)
			{
			case CELL_SYSUTIL_AVC2_STREAMING_MODE_NORMAL:
			case CELL_SYSUTIL_AVC2_STREAMING_MODE_DIRECT_WAN:
				break;
			case CELL_SYSUTIL_AVC2_STREAMING_MODE_DIRECT_LAN:
			default:
				return CELL_AVC2_ERROR_INVALID_ARGUMENT;
			}
		}
		else
		{
			streaming_mode = settings.streaming_mode;
		}

		settings.streaming_mode = streaming_mode;
		settings.mic_out_stream_sharing = init_param->voice_param.mic_out_stream_sharing;
		settings.ctx_id = ctx_id;
		settings.max_players = init_param->max_players;
		settings.max_speakers = init_param->voice_param.max_speakers;

		if (!callback_func)
		{
			return CELL_AVC2_ERROR_INVALID_ARGUMENT;
		}

		std::lock_guard lock(settings.mutex_cb);

		if (settings.avc2_cb)
		{
			return CELL_AVC2_ERROR_ALREADY_INITIALIZED;
		}

		settings.avc2_cb = callback_func;
		settings.avc2_cb_arg = user_data;
		settings.loaded = true;
		settings.joined = false;
		settings.streaming = false;
		settings.voice_detection = false;
		settings.room_id = 0;
		settings.local_member_id = 0;
		settings.voice_muting_players.clear();
		settings.voice_muting = false;
		settings.speaker_muting = false;
		settings.speaker_volume_level = 1.0f;
		settings.voice_detect_event_type = 0;
		settings.voice_detect_interval_time = 1000;
		settings.voice_detect_signal_level = 9;
		settings.voice_max_bitrate = 28000;
		settings.voice_data_fec = 0;
		settings.voice_packet_contention = 1;
		settings.voice_dtx_mode = 1;
		settings.mic_status_detection = 0;
		settings.mic_setting_notification = 0;
		settings.voice_muting_notification = 0;

		auto& engine = g_fxo->get<avc2_voice_thread>();
		engine.load(ctx_id, settings.max_players, settings.max_speakers, settings.mic_out_stream_sharing != 0);
		engine.set_voice_muting(false);
		engine.set_speaker(false, 1.0f);
		engine.set_attributes(settings.voice_detect_event_type, settings.voice_detect_interval_time,
			settings.voice_detect_signal_level, settings.voice_data_fec != 0, settings.voice_dtx_mode != 0,
			settings.mic_status_detection != 0);
		break;
	}
	case CELL_SYSUTIL_AVC2_VIDEO_CHAT:
	{
		if (false) // TODO: syscall to check container
		{
			return CELL_AVC2_ERROR_INVALID_ARGUMENT;
		}

		if (init_param->avc_init_param_version <= 140)
		{
			if (false) // TODO
			{
				return CELL_AVC2_ERROR_OUT_OF_MEMORY;
			}
		}

		if (callback_func)
		{
			return CELL_AVC2_ERROR_INVALID_ARGUMENT;
		}

		if (init_param->video_param.max_video_windows == 0 ||
			init_param->video_param.max_video_windows > (init_param->video_param.frame_mode == CELL_SYSUTIL_AVC2_FRAME_MODE_NORMAL ? 6 : 16))
		{
			return CELL_AVC2_ERROR_INVALID_ARGUMENT;
		}

		if (init_param->video_param.max_video_bitrate < 1000 || init_param->video_param.max_video_bitrate > 512000)
		{
			return CELL_AVC2_ERROR_INVALID_ARGUMENT;
		}

		if (init_param->video_param.max_video_framerate == 0 || init_param->video_param.max_video_framerate > 30)
		{
			return CELL_AVC2_ERROR_INVALID_ARGUMENT;
		}

		s32 bitrate = 0;

		switch (init_param->video_param.max_video_resolution)
		{
		case CELL_SYSUTIL_AVC2_VIDEO_RESOLUTION_QQVGA:
			bitrate = 76800;
			break;
		case CELL_SYSUTIL_AVC2_VIDEO_RESOLUTION_QVGA:
			bitrate = 307200;
			break;
		default:
			break;
		}

		u32 total_bitrate = 0;

		if (bitrate != 0)
		{
			u32 window_count = init_param->video_param.max_video_windows;

			if (init_param->video_param.video_stream_sharing == CELL_SYSUTIL_AVC2_VIDEO_SHARING_MODE_2)
			{
				window_count++;
			}

			total_bitrate = utils::align<u32>(window_count * bitrate, 0x100000) + 0x100000;
		}

		settings.video_stream_sharing = init_param->video_param.video_stream_sharing;
		settings.total_video_bitrate = total_bitrate;
		break;
	}
	default:
		return CELL_AVC2_ERROR_NOT_SUPPORTED;
	}

	return CELL_OK;
}

error_code cellSysutilAvc2Load(SceNpMatching2ContextId ctx_id, u32 container, vm::ptr<CellSysutilAvc2Callback> callback_func, vm::ptr<void> user_data, vm::cptr<CellSysutilAvc2InitParam> init_param)
{
	cellSysutilAvc2.warning("cellSysutilAvc2Load(ctx_id=0x%x, container=0x%x, callback_func=*0x%x, user_data=*0x%x, init_param=*0x%x)", ctx_id, container, callback_func, user_data, init_param);

	error_code error = cellSysutilAvc2Load_shared(ctx_id, container, callback_func, user_data, init_param);
	if (error != CELL_OK)
		return error;

	return CELL_OK;
}

error_code cellSysutilAvc2LoadAsync(SceNpMatching2ContextId ctx_id, u32 container, vm::ptr<CellSysutilAvc2Callback> callback_func, vm::ptr<void> user_data, vm::cptr<CellSysutilAvc2InitParam> init_param)
{
	cellSysutilAvc2.warning("cellSysutilAvc2LoadAsync(ctx_id=0x%x, container=0x%x, callback_func=*0x%x, user_data=*0x%x, init_param=*0x%x)", ctx_id, container, callback_func, user_data, init_param);

	error_code error = cellSysutilAvc2Load_shared(ctx_id, container, callback_func, user_data, init_param);
	if (error != CELL_OK)
		return error;

	auto& settings = g_fxo->get<avc2_settings>();
	settings.register_cb_call(CELL_AVC2_EVENT_LOAD_SUCCEEDED, 0);

	return CELL_OK;
}

error_code cellSysutilAvc2SetAttribute(vm::cptr<CellSysutilAvc2Attribute> attr)
{
	if (!attr)
	{
		cellSysutilAvc2.todo("cellSysutilAvc2SetAttribute(attr=*0x%x)", attr);
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;
	}

	cellSysutilAvc2.notice("cellSysutilAvc2SetAttribute(%s=%d)", attr->attr_id, attr->attr_param.int_param);
	auto& settings = g_fxo->get<avc2_settings>();
	if (!settings.loaded)
		return CELL_AVC2_ERROR_NOT_INITIALIZED;
	const u64 value = attr->attr_param.int_param;

	switch (attr->attr_id)
	{
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_DETECT_EVENT_TYPE:
		if (value > 1) return CELL_AVC2_ERROR_INVALID_ARGUMENT;
		settings.voice_detect_event_type = value;
		break;
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_DETECT_INTERVAL_TIME:
		if (value < 20 || value > 3000) return CELL_AVC2_ERROR_INVALID_ARGUMENT;
		settings.voice_detect_interval_time = value;
		break;
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_DETECT_SIGNAL_LEVEL:
		if (value > 10) return CELL_AVC2_ERROR_INVALID_ARGUMENT;
		settings.voice_detect_signal_level = value;
		break;
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_MAX_BITRATE:
		if (value != 4000 && value != 8000 && value != 16000 && value != 20000 && value != 24000 && value != 28000)
			return CELL_AVC2_ERROR_INVALID_ARGUMENT;
		settings.voice_max_bitrate = value;
		break;
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_DATA_FEC:
		if (value > 1) return CELL_AVC2_ERROR_INVALID_ARGUMENT;
		settings.voice_data_fec = value;
		break;
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_PACKET_CONTENTION:
		if (value < 1 || value > 15) return CELL_AVC2_ERROR_INVALID_ARGUMENT;
		settings.voice_packet_contention = value;
		break;
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_DTX_MODE:
		if (value > 1) return CELL_AVC2_ERROR_INVALID_ARGUMENT;
		settings.voice_dtx_mode = value;
		break;
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_MIC_STATUS_DETECTION:
		if (value > 1) return CELL_AVC2_ERROR_INVALID_ARGUMENT;
		settings.mic_status_detection = value;
		if (value && settings.local_member_id)
			settings.register_cb_call(CELL_AVC2_EVENT_SYSTEM_MIC_DETECTED,
				(static_cast<u64>(settings.local_member_id) << 32) | g_fxo->get<avc2_voice_thread>().microphone_status());
		break;
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_MIC_SETTING_NOTIFICATION:
		if (value > 1) return CELL_AVC2_ERROR_INVALID_ARGUMENT;
		settings.mic_setting_notification = value;
		break;
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_VOICE_MUTING_NOTIFICATION:
		if (value > 1) return CELL_AVC2_ERROR_INVALID_ARGUMENT;
		settings.voice_muting_notification = value;
		break;
	case CELL_SYSUTIL_AVC2_ATTRIBUTE_CAMERA_STATUS_DETECTION:
		if (value > 1) return CELL_AVC2_ERROR_INVALID_ARGUMENT;
		settings.camera_status_detection = value;
		break;
	default:
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;
	}

	g_fxo->get<avc2_voice_thread>().set_attributes(settings.voice_detect_event_type, settings.voice_detect_interval_time,
		settings.voice_detect_signal_level, settings.voice_data_fec != 0, settings.voice_dtx_mode != 0,
		settings.mic_status_detection != 0);

	return CELL_OK;
}

error_code cellSysutilAvc2Unload2(u32 mediaType)
{
	cellSysutilAvc2.notice("cellSysutilAvc2Unload2(mediaType=0x%x)", mediaType);

	auto& settings = g_fxo->get<avc2_settings>();

	switch (mediaType)
	{
	case CELL_SYSUTIL_AVC2_VOICE_CHAT:
	{
		std::lock_guard lock(settings.mutex_cb);

		if (!settings.avc2_cb)
		{
			return CELL_AVC2_ERROR_NOT_INITIALIZED;
		}

		// TODO: return error if the video chat is still loaded (probably CELL_AVC2_ERROR_INVALID_STATUS)
		sysutil_unregister_cb_with_id<avc2_cb_handle_t>();
			settings.avc2_cb = vm::null;
			settings.avc2_cb_arg = vm::null;
			g_fxo->get<avc2_voice_thread>().unload();
			settings.loaded = false;
			settings.joined = false;
			settings.streaming = false;
			settings.voice_detection = false;
			settings.room_id = 0;
			settings.local_member_id = 0;
		break;
	}
	case CELL_SYSUTIL_AVC2_VIDEO_CHAT:
	{
		// TODO
		break;
	}
	default:
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;
	}

	return CELL_OK;
}

error_code cellSysutilAvc2UnloadAsync2(u32 mediaType)
{
	cellSysutilAvc2.notice("cellSysutilAvc2UnloadAsync2(mediaType=0x%x)", mediaType);

	if (mediaType != CELL_SYSUTIL_AVC2_VOICE_CHAT && mediaType != CELL_SYSUTIL_AVC2_VIDEO_CHAT)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	auto& settings = g_fxo->get<avc2_settings>();
	if (mediaType == CELL_SYSUTIL_AVC2_VOICE_CHAT && !settings.loaded)
		return CELL_AVC2_ERROR_NOT_INITIALIZED;

	if (mediaType == CELL_SYSUTIL_AVC2_VOICE_CHAT)
	{
		g_fxo->get<avc2_voice_thread>().unload();
		settings.loaded = false;
		settings.joined = false;
		settings.streaming = false;
		settings.voice_detection = false;
		settings.register_cb_call(CELL_AVC2_EVENT_UNLOAD_SUCCEEDED, 0);
	}
	else
		settings.register_cb_call(CELL_AVC2_EVENT_UNLOAD_SUCCEEDED, 2);

	return CELL_OK;
}

error_code cellSysutilAvc2StartStreaming2(u32 mediaType)
{
	cellSysutilAvc2.notice("cellSysutilAvc2StartStreaming2(mediaType=0x%x)", mediaType);

	if (mediaType != CELL_SYSUTIL_AVC2_VOICE_CHAT && mediaType != CELL_SYSUTIL_AVC2_VIDEO_CHAT)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	if (mediaType == CELL_SYSUTIL_AVC2_VOICE_CHAT)
		return cellSysutilAvc2StartStreaming();
	return CELL_OK;
}

error_code cellSysutilAvc2HideScreen()
{
	cellSysutilAvc2.todo("cellSysutilAvc2HideScreen()");
	return CELL_OK;
}

error_code cellSysutilAvc2HideWindow(SceNpMatching2RoomMemberId member_id)
{
	cellSysutilAvc2.todo("cellSysutilAvc2HideWindow(member_id=0x%x)", member_id);
	return CELL_OK;
}

error_code cellSysutilAvc2GetVoiceMuting(vm::ptr<u8> muting)
{
	cellSysutilAvc2.trace("cellSysutilAvc2GetVoiceMuting(muting=*0x%x)", muting);

	if (!muting)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	const auto& settings = g_fxo->get<avc2_settings>();
	if (!settings.loaded)
		return CELL_AVC2_ERROR_NOT_INITIALIZED;
	*muting = settings.voice_muting;

	return CELL_OK;
}

error_code cellSysutilAvc2GetScreenShowStatus(vm::ptr<u8> visible)
{
	cellSysutilAvc2.todo("cellSysutilAvc2GetScreenShowStatus(visible=*0x%x)", visible);

	if (!visible)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	*visible = 0;

	return CELL_OK;
}

error_code cellSysutilAvc2GetWindowPosition(SceNpMatching2RoomMemberId member_id, vm::ptr<f32> x, vm::ptr<f32> y, vm::ptr<f32> z)
{
	cellSysutilAvc2.todo("cellSysutilAvc2GetWindowPosition(member_id=0x%x, x=*0x%x, y=*0x%x, z=*0x%x)", member_id, x, y, z);

	if (!x || !y || !z)
		return CELL_AVC2_ERROR_INVALID_ARGUMENT;

	return CELL_OK;
}


DECLARE(ppu_module_manager::cellSysutilAvc2)("cellSysutilAvc2", []()
{
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2GetPlayerInfo);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2JoinChat);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2StopStreaming);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2ChangeVideoResolution);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2ShowScreen);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2GetVideoMuting);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2GetWindowAttribute);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2StopStreaming2);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2SetVoiceMuting);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2StartVoiceDetection);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2UnloadAsync);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2StopVoiceDetection);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2GetAttribute);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2LoadAsync);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2SetSpeakerVolumeLevel);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2SetWindowString);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2EstimateMemoryContainerSize);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2SetVideoMuting);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2SetPlayerVoiceMuting);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2SetStreamingTarget);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2Unload);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2DestroyWindow);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2SetWindowPosition);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2GetSpeakerVolumeLevel);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2IsCameraAttached);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2MicRead);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2GetPlayerVoiceMuting);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2JoinChatRequest);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2StartStreaming);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2SetWindowAttribute);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2GetWindowShowStatus);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2InitParam);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2GetWindowSize);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2SetStreamPriority);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2LeaveChatRequest);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2IsMicAttached);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2CreateWindow);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2GetSpeakerMuting);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2ShowWindow);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2SetWindowSize);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2EnumPlayers);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2GetWindowString);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2LeaveChat);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2SetSpeakerMuting);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2Load);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2SetAttribute);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2UnloadAsync2);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2StartStreaming2);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2HideScreen);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2HideWindow);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2GetVoiceMuting);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2GetScreenShowStatus);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2Unload2);
	REG_FUNC(cellSysutilAvc2, cellSysutilAvc2GetWindowPosition);
});
