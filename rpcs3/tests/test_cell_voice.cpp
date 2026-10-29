#include "stdafx.h"
#include <gtest/gtest.h>
#include "Emu/Cell/Modules/cellVoice.h"
#include "Emu/Audio/voice_input.h"
#include "Emu/Audio/AudioBackend.h"
#include "Emu/emu_callbacks.h"
#include "Emu/system_config.h"
#include "util/media_utils.h"
#include <filesystem>
#include <thread>

#include <algorithm>
#include <array>
#include <cmath>

namespace
{
	constexpr u16 pcm_input = 0x101, voice_output = 0x202, encoded_input_port = 0x303, pcm_output = 0x404;
	constexpr u32 packet_bytes = 19;

	voice_manager::port_t make_port(CellVoicePortType type, CellVoiceBitRate bitrate = CELLVOICE_BITRATE_7300)
	{
		voice_manager::port_t port{};
		port.info.portType = type;
		port.info.volume = 1.0f;
		if (type == CELLVOICE_PORTTYPE_IN_VOICE || type == CELLVOICE_PORTTYPE_OUT_VOICE)
		{
			port.info.voice.bitrate = bitrate;
		}
		else
		{
			port.info.pcmaudio.bufSize = 0x4000;
			port.info.pcmaudio.format.numChannels = 1;
			port.info.pcmaudio.format.sampleAlignment = 2;
			port.info.pcmaudio.format.dataType = CELLVOICE_PCM_SHORT;
			port.info.pcmaudio.format.sampleRate = CELLVOICE_SAMPLINGRATE_16000;
		}
		return port;
	}

	// Opaque payloads exercise host queue ownership and framing, not CELP quality.
	std::vector<u8> packet(u8 marker, u32 size = packet_bytes)
	{
		return std::vector<u8>(size, marker);
	}

	voice_firmware::result encoded(const voice_firmware::request& request, u8 marker)
	{
		voice_firmware::result result;
		result.codec = request.codec;
		result.packet = packet(marker, voice_firmware::find_config(request.bitrate)->bytes);
		return result;
	}

	voice_firmware::result decoded(const voice_firmware::request& request)
	{
		voice_firmware::result result;
		result.codec = request.codec;
		result.pcm.assign(voice_firmware::find_config(request.bitrate)->samples, 1000.0f);
		return result;
	}

	class CellVoiceAsync : public testing::Test
	{
	protected:
		voice_manager manager;
		voice_service service{manager, false};

		void SetUp() override
		{
			manager.is_init = true;
			manager.voice_service_started = true;
			manager.run_state = voice_run_state::running;
		}

		void add_decoder(u16 id = encoded_input_port)
		{
			manager.ports[id] = make_port(CELLVOICE_PORTTYPE_IN_VOICE);
		}

		void add_encoder()
		{
			manager.ports[pcm_input] = make_port(CELLVOICE_PORTTYPE_IN_PCMAUDIO);
			manager.ports[voice_output] = make_port(CELLVOICE_PORTTYPE_OUT_VOICE);
			service.edges.emplace(pcm_input, voice_output);
		}

		void tick(u32 count = 1)
		{
			for (u32 i = 0; i < count; i++) service.tick(manager);
		}

		void write_packet(u16 id, const std::vector<u8>& bytes)
		{
			ASSERT_EQ(service.write(manager.ports.at(id), id, bytes.data(), static_cast<u32>(bytes.size())), bytes.size());
		}

		void feed_pcm(s16 value = 1000)
		{
			std::array<be_t<s16>, voice_service::tick_samples> samples;
			samples.fill(value);
			ASSERT_EQ(service.write(manager.ports.at(pcm_input), pcm_input,
				reinterpret_cast<const u8*>(samples.data()), sizeof(samples)), sizeof(samples));
			tick();
		}

		void wait_for_request()
		{
			for (u32 i = 0; i < 6 && service.requests.empty(); i++) tick();
		}

		std::vector<voice_firmware::request> take_requests()
		{
			// Match the worker: dequeue copied jobs, then commit outside the admission tick.
			std::vector<voice_firmware::request> requests;
			requests.swap(service.requests);
			return requests;
		}
	};
}

TEST_F(CellVoiceAsync, WidebandEncoderKeepsFullRateAndFrameCadence)
{
	for (const auto bitrate : {CELLVOICE_BITRATE_14400, CELLVOICE_BITRATE_16000, CELLVOICE_BITRATE_22533})
	{
		SCOPED_TRACE(bitrate);
		service.clear();
		manager.ports.clear();
		add_encoder();
		manager.ports.at(voice_output).info.voice.bitrate = bitrate;
		const auto& cfg = *voice_firmware::find_config(bitrate);
		u32 total_samples = 0;
		for (u32 period = 0; period < 30; period++)
		{
			feed_pcm(2000);
			auto jobs = take_requests();
			ASSERT_GE(jobs.size(), 1u);
			ASSERT_LE(jobs.size(), 2u);
			EXPECT_EQ(service.data.at(voice_output).codec_pending, jobs.size());
			for (const auto& job : jobs)
			{
				ASSERT_EQ(job.pcm.size(), cfg.samples);
				EXPECT_TRUE(std::all_of(job.pcm.begin(), job.pcm.end(), [](f32 v) { return v == 2000.0f; }));
				total_samples += static_cast<u32>(job.pcm.size());
				service.commit(manager, encoded(job, 1));
			}
			EXPECT_EQ(service.data.at(voice_output).codec_pending, 0u);
			EXPECT_LT(service.data.at(voice_output).pcm.size(), cfg.samples);
		}
		EXPECT_EQ(total_samples, 30 * voice_service::tick_samples);
	}
}

TEST_F(CellVoiceAsync, WidebandDecoderRefillsWithoutGapsOrGrowingBacklog)
{
	for (const auto bitrate : {CELLVOICE_BITRATE_14400, CELLVOICE_BITRATE_16000, CELLVOICE_BITRATE_22533})
	{
		SCOPED_TRACE(bitrate);
		service.clear();
		manager.ports.clear();
		add_decoder();
		manager.ports.at(encoded_input_port).info.voice.bitrate = bitrate;
		const auto& cfg = *voice_firmware::find_config(bitrate);
		u32 samples = 0;
		for (u32 period = 0; period < 90; period++)
		{
			samples += voice_service::tick_samples;
			while (samples >= cfg.samples)
			{
				write_packet(encoded_input_port, packet(1, cfg.bytes));
				samples -= cfg.samples;
			}
			tick();
			auto jobs = take_requests();
			EXPECT_LE(jobs.size(), 2u);
			if (period > 5)
			{
				const auto& signal = service.data.at(encoded_input_port).signal;
				EXPECT_TRUE(std::all_of(signal.begin(), signal.end(), [](f32 v) { return v == 1000.0f; }));
			}
			for (const auto& job : jobs) service.commit(manager, decoded(job));
			EXPECT_LE(service.bytes_queued(encoded_input_port), cfg.bytes * 6);
			EXPECT_LT(service.data.at(encoded_input_port).pcm.size(), voice_service::tick_samples + cfg.samples);
		}
	}
}

TEST_F(CellVoiceAsync, WidebandBatchStaysPendingUntilLastCompletionAndRejectsFailureRemainder)
{
	add_decoder();
	manager.ports.at(encoded_input_port).info.voice.bitrate = CELLVOICE_BITRATE_16000;
	for (u32 i = 0; i < 4; i++) write_packet(encoded_input_port, packet(1, 20));
	wait_for_request();
	auto jobs = take_requests();
	ASSERT_EQ(jobs.size(), 2u);
	service.commit(manager, decoded(jobs[0]));
	EXPECT_EQ(service.data.at(encoded_input_port).codec_pending, 1u);
	tick();
	EXPECT_TRUE(service.requests.empty());
	auto failure = decoded(jobs[1]);
	failure.error = 1;
	service.commit(manager, failure);
	EXPECT_EQ(service.data.at(encoded_input_port).codec_pending, 0u);
	EXPECT_TRUE(service.data.at(encoded_input_port).codec_failed);
	service.commit(manager, decoded(jobs[0]));
	EXPECT_TRUE(service.data.at(encoded_input_port).pcm.empty());
}

TEST_F(CellVoiceAsync, WidebandStopRejectsBothFramesOfAnOutstandingBatch)
{
	add_encoder();
	manager.ports.at(voice_output).info.voice.bitrate = CELLVOICE_BITRATE_16000;
	feed_pcm();
	auto jobs = take_requests();
	ASSERT_EQ(jobs.size(), 2u);
	service.stop(manager);
	for (const auto& job : jobs) service.commit(manager, encoded(job, 1));
	EXPECT_EQ(service.bytes_queued(voice_output), 0u);
	EXPECT_EQ(service.data.at(voice_output).codec_pending, 0u);
}

TEST_F(CellVoiceAsync, DecoderWaitsForAWholePacketAndCopiesItsInput)
{
	add_decoder();
	const auto first = packet(0x31);
	ASSERT_EQ(service.write(manager.ports.at(encoded_input_port), encoded_input_port, first.data(), packet_bytes - 1), packet_bytes - 1);
	tick(6);
	EXPECT_TRUE(service.requests.empty());
	EXPECT_EQ(service.bytes_queued(encoded_input_port), packet_bytes - 1);

	std::vector<u8> remainder{first.back()};
	const auto second = packet(0x52);
	remainder.insert(remainder.end(), second.begin(), second.end());
	remainder.insert(remainder.end(), 5, 0x73);
	write_packet(encoded_input_port, remainder);
	wait_for_request();
	ASSERT_EQ(service.requests.size(), 1u);
	EXPECT_EQ(service.requests.front().packet, first);
	EXPECT_TRUE(service.requests.front().pcm.empty());
	EXPECT_FALSE(service.requests.front().codec.encode);
	EXPECT_EQ(service.bytes_queued(encoded_input_port), packet_bytes + 5);
	std::fill(remainder.begin(), remainder.end(), 0);
	std::fill(service.data.at(encoded_input_port).bytes.begin(), service.data.at(encoded_input_port).bytes.end(), 0);
	EXPECT_EQ(service.requests.front().packet, first);
}

TEST_F(CellVoiceAsync, DecoderKeepsOneJobInFlightAfterWorkerDequeuesIt)
{
	add_decoder();
	write_packet(encoded_input_port, packet(0x11));
	write_packet(encoded_input_port, packet(0x22));
	wait_for_request();
	auto jobs = take_requests();
	ASSERT_EQ(jobs.size(), 1u);
	tick(8);
	EXPECT_TRUE(service.requests.empty());
	EXPECT_TRUE(service.data.at(encoded_input_port).codec_pending);
	EXPECT_EQ(service.bytes_queued(encoded_input_port), packet_bytes);
	service.commit(manager, decoded(jobs.front()));
	EXPECT_FALSE(service.data.at(encoded_input_port).codec_pending);
	EXPECT_EQ(service.data.at(encoded_input_port).pcm.size(), voice_service::tick_samples);
	wait_for_request();
	ASSERT_EQ(service.requests.size(), 1u);
	EXPECT_EQ(service.requests.front().packet, packet(0x22));
	EXPECT_EQ(service.requests.front().codec, jobs.front().codec);
}

TEST_F(CellVoiceAsync, DifferentDecoderPortsCanHaveIndependentJobsInFlight)
{
	constexpr u16 second_input = 0x505;
	add_decoder();
	add_decoder(second_input);
	write_packet(encoded_input_port, packet(1));
	write_packet(second_input, packet(2));
	wait_for_request();
	auto jobs = take_requests();
	ASSERT_EQ(jobs.size(), 2u);
	EXPECT_NE(jobs[0].codec.port, jobs[1].codec.port);
	EXPECT_NE(jobs[0].codec.generation, jobs[1].codec.generation);
	EXPECT_TRUE(service.data.at(encoded_input_port).codec_pending);
	EXPECT_TRUE(service.data.at(second_input).codec_pending);
	service.commit(manager, decoded(jobs[0]));
	EXPECT_FALSE(service.data.at(jobs[0].codec.port).codec_pending);
	EXPECT_TRUE(service.data.at(jobs[1].codec.port).codec_pending);
}

TEST_F(CellVoiceAsync, DecoderSustainsOnePacketPerTickWithoutGapsOrBacklog)
{
	add_decoder();
	write_packet(encoded_input_port, packet(1));
	wait_for_request();
	for (u32 frame = 0; frame < 100; frame++)
	{
		auto jobs = take_requests();
		ASSERT_EQ(jobs.size(), 1u) << "frame " << frame;
		service.commit(manager, decoded(jobs.front()));
		write_packet(encoded_input_port, packet(static_cast<u8>(frame + 2)));
		tick();
		EXPECT_EQ(service.bytes_queued(encoded_input_port), 0u);
		const auto& signal = service.data.at(encoded_input_port).signal;
		EXPECT_TRUE(std::any_of(signal.begin(), signal.end(), [](f32 sample) { return std::abs(sample) > 100.0f; }));
	}
}

TEST_F(CellVoiceAsync, EncoderAcceptsZeroByteWarmupAndDoesNotDuplicatePendingJobs)
{
	add_encoder();
	feed_pcm();
	auto jobs = take_requests();
	ASSERT_EQ(jobs.size(), 1u);
	EXPECT_TRUE(jobs.front().codec.encode);
	EXPECT_EQ(jobs.front().pcm.size(), 160u);
	EXPECT_TRUE(jobs.front().packet.empty());
	EXPECT_TRUE(std::any_of(jobs.front().pcm.begin(), jobs.front().pcm.end(), [](f32 sample) { return std::abs(sample) > 100.0f; }));
	for (u32 i = 0; i < 4; i++) feed_pcm();
	EXPECT_TRUE(service.requests.empty());
	EXPECT_TRUE(service.data.at(voice_output).codec_pending);
	voice_firmware::result warmup;
	warmup.codec = jobs.front().codec;
	service.commit(manager, warmup);
	EXPECT_FALSE(service.data.at(voice_output).codec_failed);
	EXPECT_FALSE(service.data.at(voice_output).codec_pending);
	EXPECT_EQ(service.bytes_queued(voice_output), 0u);
	feed_pcm();
	jobs = take_requests();
	ASSERT_EQ(jobs.size(), 1u);
	EXPECT_EQ(jobs.front().codec, warmup.codec);
	service.commit(manager, encoded(jobs.front(), 0x45));
	EXPECT_EQ(service.bytes_queued(voice_output), packet_bytes);
}

TEST_F(CellVoiceAsync, SustainedSpeechDoesNotBecomeItsOwnNoiseFloor)
{
	add_encoder();
	for (u32 frame = 0; frame < 1500; frame++)
	{
		feed_pcm();
		auto jobs = take_requests();
		ASSERT_EQ(jobs.size(), 1u) << "speech cut off after " << frame * 20 << " ms";
		service.commit(manager, encoded(jobs.front(), 0x35));
	}
	service.data.at(voice_output).noise_db = 55.0f;
	service.stop(manager);
	EXPECT_FLOAT_EQ(service.data.at(voice_output).noise_db, 30.0f);
	EXPECT_FALSE(service.data.at(voice_output).codec_generation);
}

TEST_F(CellVoiceAsync, DeletedAndReusedPortRejectsOldCompletionWithoutClearingNewJob)
{
	add_decoder();
	write_packet(encoded_input_port, packet(1));
	wait_for_request();
	auto old = take_requests();
	ASSERT_EQ(old.size(), 1u);
	manager.ports.erase(encoded_input_port);
	service.remove_port(encoded_input_port);
	service.commit(manager, decoded(old.front()));
	EXPECT_FALSE(service.data.contains(encoded_input_port));
	add_decoder();
	write_packet(encoded_input_port, packet(2));
	wait_for_request();
	auto current = take_requests();
	ASSERT_EQ(current.size(), 1u);
	EXPECT_NE(current.front().codec.generation, old.front().codec.generation);
	auto stale = decoded(old.front());
	stale.error = CELL_VOICE_ERROR_GENERAL;
	service.commit(manager, stale);
	EXPECT_TRUE(service.data.at(encoded_input_port).codec_pending);
	EXPECT_FALSE(service.data.at(encoded_input_port).codec_failed);
	EXPECT_TRUE(service.data.at(encoded_input_port).pcm.empty());
	service.commit(manager, decoded(current.front()));
	EXPECT_FALSE(service.data.at(encoded_input_port).codec_pending);
	EXPECT_EQ(service.data.at(encoded_input_port).pcm.size(), voice_service::tick_samples);
}

TEST_F(CellVoiceAsync, ResetPortDataRejectsCompletionBeforeAndAfterReinitialization)
{
	add_decoder();
	write_packet(encoded_input_port, packet(1));
	wait_for_request();
	auto old = take_requests();
	ASSERT_EQ(old.size(), 1u);
	// cellVoiceResetPort replaces this state while retaining the port and its connections.
	service.data.at(encoded_input_port) = {};
	service.commit(manager, decoded(old.front()));
	EXPECT_TRUE(service.data.at(encoded_input_port).pcm.empty());
	write_packet(encoded_input_port, packet(2));
	wait_for_request();
	auto current = take_requests();
	ASSERT_EQ(current.size(), 1u);
	EXPECT_NE(current.front().codec.generation, old.front().codec.generation);
	service.commit(manager, decoded(old.front()));
	EXPECT_TRUE(service.data.at(encoded_input_port).codec_pending);
	EXPECT_TRUE(service.data.at(encoded_input_port).pcm.empty());
}

TEST_F(CellVoiceAsync, BitrateChangeRejectsCompletionBeforeNextTick)
{
	add_decoder();
	write_packet(encoded_input_port, packet(1));
	wait_for_request();
	auto old = take_requests();
	ASSERT_EQ(old.size(), 1u);
	manager.ports.at(encoded_input_port).info.voice.bitrate = CELLVOICE_BITRATE_5700;
	service.commit(manager, decoded(old.front()));
	EXPECT_TRUE(service.data.at(encoded_input_port).pcm.empty());
	EXPECT_FALSE(service.data.at(encoded_input_port).codec_failed);
}

TEST_F(CellVoiceAsync, BitrateChangeDiscardsQueuedInputFromPreviousFormat)
{
	add_decoder();
	write_packet(encoded_input_port, packet(1));
	tick();
	ASSERT_TRUE(service.requests.empty());
	const u64 old_generation = service.data.at(encoded_input_port).codec_generation;
	ASSERT_NE(old_generation, 0u);
	manager.ports.at(encoded_input_port).info.voice.bitrate = CELLVOICE_BITRATE_5700;
	tick(6);
	EXPECT_TRUE(service.requests.empty());
	EXPECT_EQ(service.bytes_queued(encoded_input_port), 0u);
	write_packet(encoded_input_port, packet(2, 15));
	wait_for_request();
	ASSERT_EQ(service.requests.size(), 1u);
	EXPECT_EQ(service.requests.front().packet, packet(2, 15));
	EXPECT_NE(service.requests.front().codec.generation, old_generation);
}

TEST_F(CellVoiceAsync, BitrateChangeDropsOldOutputAndPreservesNewPendingJob)
{
	add_encoder();
	feed_pcm();
	auto jobs = take_requests();
	ASSERT_EQ(jobs.size(), 1u);
	service.commit(manager, encoded(jobs.front(), 1));
	ASSERT_EQ(service.bytes_queued(voice_output), packet_bytes);
	feed_pcm();
	auto old = take_requests();
	ASSERT_EQ(old.size(), 1u);
	manager.ports.at(voice_output).info.voice.bitrate = CELLVOICE_BITRATE_5700;
	feed_pcm();
	auto current = take_requests();
	ASSERT_EQ(current.size(), 1u);
	EXPECT_EQ(service.bytes_queued(voice_output), 0u);
	EXPECT_NE(current.front().codec.generation, old.front().codec.generation);
	service.commit(manager, encoded(old.front(), 2));
	EXPECT_TRUE(service.data.at(voice_output).codec_pending);
	EXPECT_EQ(service.bytes_queued(voice_output), 0u);
	service.commit(manager, encoded(current.front(), 3));
	std::array<u8, packet_bytes> output{};
	EXPECT_EQ(service.read(manager, manager.ports.at(voice_output), voice_output, output.data(), output.size()), 15u);
	EXPECT_TRUE(std::all_of(output.begin(), output.begin() + 15, [](u8 value) { return value == 3; }));
}

TEST_F(CellVoiceAsync, OutputKeepsNewestWholeFramesAndRounds255ByteReadsDownTo247)
{
	add_encoder();
	for (u8 frame = 0; frame < 55; frame++)
	{
		feed_pcm();
		auto jobs = take_requests();
		ASSERT_EQ(jobs.size(), 1u);
		service.commit(manager, encoded(jobs.front(), frame));
	}
	EXPECT_EQ(service.bytes_queued(voice_output), 50 * packet_bytes);
	std::array<u8, 255> output;
	output.fill(0xee);
	EXPECT_EQ(service.read(manager, manager.ports.at(voice_output), voice_output, output.data(), output.size()), 247u);
	for (u32 i = 0; i < 247; i++) EXPECT_EQ(output[i], 5 + i / packet_bytes);
	EXPECT_TRUE(std::all_of(output.begin() + 247, output.end(), [](u8 value) { return value == 0xee; }));
	EXPECT_EQ(service.bytes_queued(voice_output), 37 * packet_bytes);
	EXPECT_EQ(service.read(manager, manager.ports.at(voice_output), voice_output, output.data(), packet_bytes - 1), 0u);
	EXPECT_EQ(service.bytes_queued(voice_output), 37 * packet_bytes);
	EXPECT_EQ(service.read(manager, manager.ports.at(voice_output), voice_output, output.data(), packet_bytes), packet_bytes);
	EXPECT_TRUE(std::all_of(output.begin(), output.begin() + packet_bytes, [](u8 value) { return value == 18; }));
}

TEST_F(CellVoiceAsync, SilenceAndFreshMutedInputsDoNotCreateEncoderJobs)
{
	add_encoder();
	for (u32 i = 0; i < 8; i++) feed_pcm(0);
	EXPECT_TRUE(service.requests.empty());
	EXPECT_EQ(service.bytes_queued(voice_output), 0u);
	manager.ports.at(pcm_input).info.bMute = 1;
	for (u32 i = 0; i < 8; i++) feed_pcm();
	EXPECT_TRUE(service.requests.empty());
	EXPECT_EQ(service.bytes_queued(voice_output), 0u);
}

TEST_F(CellVoiceAsync, MutingOrPausingActiveOutputRejectsInFlightSpeechAndNewJobs)
{
	for (u32 mode = 0; mode < 3; mode++)
	{
		SCOPED_TRACE(mode);
		service.clear();
		manager.ports.clear();
		add_encoder();
		feed_pcm();
		auto ready = take_requests();
		ASSERT_EQ(ready.size(), 1u);
		service.commit(manager, encoded(ready.front(), 1));
		feed_pcm();
		auto pending = take_requests();
		ASSERT_EQ(pending.size(), 1u);
		feed_pcm();
		feed_pcm();
		ASSERT_TRUE(service.requests.empty());
		if (mode == 0) manager.ports.at(voice_output).info.bMute = 1;
		if (mode == 1) service.mute_all = true;
		if (mode == 2) service.data.at(voice_output).paused = true;
		service.commit(manager, encoded(pending.front(), 2));
		feed_pcm();
		EXPECT_TRUE(service.requests.empty());
		EXPECT_EQ(service.bytes_queued(voice_output), packet_bytes);
		EXPECT_TRUE(service.data.at(voice_output).pcm.empty());
		EXPECT_EQ(service.data.at(voice_output).hangover, 0u);
		EXPECT_FALSE(service.data.at(voice_output).codec_pending);
	}
}

TEST_F(CellVoiceAsync, StopInvalidatesOutstandingCompletion)
{
	add_encoder();
	feed_pcm();
	auto jobs = take_requests();
	ASSERT_EQ(jobs.size(), 1u);
	service.stop(manager);
	service.commit(manager, encoded(jobs.front(), 1));
	EXPECT_TRUE(service.requests.empty());
	EXPECT_EQ(service.bytes_queued(voice_output), 0u);
	EXPECT_FALSE(service.data.at(voice_output).codec_pending);
}

TEST_F(CellVoiceAsync, AttributesValidatePortTypeAndReturnMeasurements)
{
	for (u32 type = CELLVOICE_PORTTYPE_IN_MIC; type <= CELLVOICE_PORTTYPE_OUT_SECONDARY; type++)
	{
		SCOPED_TRACE(type);
		const auto port = make_port(static_cast<CellVoicePortType>(type));
		const u16 id = static_cast<u16>(0x100 + type);
		u32 value = 0xdeadbeef;
		EXPECT_EQ(service.get_port_attr(port, id, CELLVOICE_ATTR_ENERGY_LEVEL, value), CELL_OK);
		EXPECT_EQ(value, 0u);
		EXPECT_EQ(service.set_port_attr(port, id, CELLVOICE_ATTR_ENERGY_LEVEL, 1), CELL_OK);
		const u32 expected = type == CELLVOICE_PORTTYPE_IN_MIC ? CELL_OK : CELL_VOICE_ERROR_ARGUMENT_INVALID;
		EXPECT_EQ(service.set_port_attr(port, id, CELLVOICE_ATTR_VAD, 5), expected);
		value = 0xdeadbeef;
		EXPECT_EQ(service.get_port_attr(port, id, CELLVOICE_ATTR_VAD, value), expected);
		// Get(VAD) returns activity, not the threshold passed to Set(VAD).
		EXPECT_EQ(value, type == CELLVOICE_PORTTYPE_IN_MIC ? 0u : 0xdeadbeef);
	}
}

TEST_F(CellVoiceAsync, EnergyMeasurementCanBeEnabledFrozenAndReenabled)
{
	manager.ports[pcm_input] = make_port(CELLVOICE_PORTTYPE_IN_PCMAUDIO);
	manager.ports[pcm_output] = make_port(CELLVOICE_PORTTYPE_OUT_PCMAUDIO);
	service.edges.emplace(pcm_input, pcm_output);
	const auto& input = manager.ports.at(pcm_input);
	const auto& output = manager.ports.at(pcm_output);
	u32 value = 0;
	feed_pcm(2000);
	ASSERT_EQ(service.get_port_attr(input, pcm_input, CELLVOICE_ATTR_ENERGY_LEVEL, value), CELL_OK);
	EXPECT_EQ(value, 0u); // Disabled by default.
	ASSERT_EQ(service.set_port_attr(input, pcm_input, CELLVOICE_ATTR_ENERGY_LEVEL, 1), CELL_OK);
	ASSERT_EQ(service.set_port_attr(output, pcm_output, CELLVOICE_ATTR_ENERGY_LEVEL, 0xffffffff), CELL_OK);
	feed_pcm(2000);
	ASSERT_EQ(service.get_port_attr(input, pcm_input, CELLVOICE_ATTR_ENERGY_LEVEL, value), CELL_OK);
	const f32 measured = std::bit_cast<f32>(value);
	EXPECT_GT(measured, 40.0f);
	EXPECT_LT(measured, 90.0f);
	u32 output_value = 0;
	ASSERT_EQ(service.get_port_attr(output, pcm_output, CELLVOICE_ATTR_ENERGY_LEVEL, output_value), CELL_OK);
	EXPECT_EQ(output_value, value);
	ASSERT_EQ(service.set_port_attr(input, pcm_input, CELLVOICE_ATTR_ENERGY_LEVEL, 0), CELL_OK);
	feed_pcm(0);
	ASSERT_EQ(service.get_port_attr(input, pcm_input, CELLVOICE_ATTR_ENERGY_LEVEL, value), CELL_OK);
	EXPECT_FLOAT_EQ(std::bit_cast<f32>(value), measured);
	ASSERT_EQ(service.set_port_attr(input, pcm_input, CELLVOICE_ATTR_ENERGY_LEVEL, 1), CELL_OK);
	feed_pcm(0);
	ASSERT_EQ(service.get_port_attr(input, pcm_input, CELLVOICE_ATTR_ENERGY_LEVEL, value), CELL_OK);
	EXPECT_FLOAT_EQ(std::bit_cast<f32>(value), 40.0f);
	service.stop(manager);
	EXPECT_TRUE(service.data.at(pcm_input).energy_enabled);
	service.remove_port(pcm_input);
	ASSERT_EQ(service.get_port_attr(input, pcm_input, CELLVOICE_ATTR_ENERGY_LEVEL, value), CELL_OK);
	EXPECT_EQ(value, 0u);
}

TEST_F(CellVoiceAsync, PcmRoutingPreservesSamplesAndCreatesNoFirmwareJobs)
{
	manager.ports[pcm_input] = make_port(CELLVOICE_PORTTYPE_IN_PCMAUDIO);
	manager.ports[pcm_output] = make_port(CELLVOICE_PORTTYPE_OUT_PCMAUDIO);
	service.edges.emplace(pcm_input, pcm_output);
	std::array<be_t<s16>, voice_service::tick_samples> input;
	for (u32 i = 0; i < input.size(); i++) input[i] = static_cast<s16>(static_cast<s32>(i) * 17 - 2000);
	ASSERT_EQ(service.write(manager.ports.at(pcm_input), pcm_input,
		reinterpret_cast<const u8*>(input.data()), sizeof(input)), sizeof(input));
	tick();
	std::array<be_t<s16>, voice_service::tick_samples> output{};
	EXPECT_EQ(service.read(manager, manager.ports.at(pcm_output), pcm_output,
		reinterpret_cast<u8*>(output.data()), sizeof(output)), sizeof(output));
	for (u32 i = 0; i < input.size(); i++) EXPECT_EQ(static_cast<s16>(output[i]), static_cast<s16>(input[i]));
	EXPECT_EQ(service.bytes_queued(pcm_input), 0u);
	EXPECT_TRUE(service.requests.empty());
	EXPECT_TRUE(service.codec_keys().empty());
}

namespace
{
	class voice_test_audio_backend final : public AudioBackend
	{
	public:
		bool open_ok = true;
		u32 close_count = 0;
		std::string device;

		std::string_view GetName() const override { return "Voice output test"; }
		bool Open(std::string_view id, AudioFreq freq, AudioSampleSize size, AudioChannelCnt channels, audio_channel_layout layout) override
		{
			device = id;
			m_sampling_rate = freq;
			m_sample_size = size;
			m_channels = static_cast<u32>(channels);
			m_layout = layout;
			return open_ok;
		}
		void Close() override
		{
			// Model a final callback while Close waits for the stream to stop.
			// This must be safe even when voice output is being switched.
			alignas(f32) std::array<u8, 64> samples{};
			if (m_write_callback) m_write_callback(static_cast<u32>(samples.size()), samples.data());
			m_playing = false;
			close_count++;
		}
		f64 GetCallbackFrameLen() override { return 0.01; }
		void Play() override { m_playing = true; }
		void Pause() override { m_playing = false; }
	};

	class CellVoiceOutput : public CellVoiceAsync
	{
	protected:
		std::string saved_audio = g_cfg.audio.to_string();
		std::function<std::shared_ptr<AudioBackend>()> saved_factory = g_emu_callbacks.get_audio;
		std::vector<std::shared_ptr<voice_test_audio_backend>> backends;
		bool next_open_ok = true;

		void SetUp() override
		{
			CellVoiceAsync::SetUp();
			g_cfg.audio.voice_output_device.set("");
			g_cfg.audio.audio_device.set("game-speakers");
			g_cfg.audio.convert_to_s16.set(false);
			g_emu_callbacks.get_audio = [this]() -> std::shared_ptr<AudioBackend>
			{
				auto backend = std::make_shared<voice_test_audio_backend>();
				backend->open_ok = next_open_ok;
				backends.push_back(backend);
				return backend;
			};
			manager.ports[pcm_input] = make_port(CELLVOICE_PORTTYPE_IN_PCMAUDIO);
			manager.ports[pcm_output] = make_port(CELLVOICE_PORTTYPE_OUT_SECONDARY);
			service.edges.emplace(pcm_input, pcm_output);
		}

		void TearDown() override
		{
			service.stop(manager);
			g_emu_callbacks.get_audio = saved_factory;
			g_cfg.audio.from_string(saved_audio);
		}
	};
}

TEST_F(CellVoiceOutput, AppliedDeviceChangeReopensOnlyVoiceAndDiscardsQueuedAudio)
{
	tick();
	ASSERT_EQ(backends.size(), 1u);
	EXPECT_EQ(backends[0]->device, "game-speakers");
	service.headset_fifo.assign(1000, 1234.0f);
	service.headset_primed = true;
	service.headset_prev = service.headset_cur = 1234.0f;

	// Match the dynamic-only config update performed by Apply during emulation.
	ASSERT_TRUE(g_cfg.from_string("Audio:\n  Voice Chat Output Device: headset\n", true));
	tick();
	ASSERT_EQ(backends.size(), 2u);
	EXPECT_EQ(backends[0]->close_count, 1u);
	EXPECT_FALSE(backends[0]->IsPlaying());
	EXPECT_EQ(backends[1]->device, "headset");
	EXPECT_TRUE(backends[1]->IsPlaying());
	EXPECT_EQ(g_cfg.audio.audio_device.to_string(), "game-speakers");
	EXPECT_TRUE(std::all_of(service.headset_fifo.begin(), service.headset_fifo.end(), [](f32 sample) { return sample == 0.0f; }));
	EXPECT_EQ(service.headset_prev, 0.0f);
	EXPECT_EQ(service.headset_cur, 0.0f);

	g_cfg.audio.audio_device.set("other-game-speakers");
	tick(3);
	EXPECT_EQ(backends.size(), 2u); // Explicit voice device stays independent.
	g_cfg.audio.voice_output_device.set("");
	tick();
	ASSERT_EQ(backends.size(), 3u);
	EXPECT_EQ(backends[2]->device, "other-game-speakers");
	g_cfg.audio.audio_device.set("new-game-speakers");
	tick();
	ASSERT_EQ(backends.size(), 4u);
	EXPECT_EQ(backends[3]->device, "new-game-speakers");
	g_cfg.audio.voice_output_device.set("@@@default@@@");
	tick();
	ASSERT_EQ(backends.size(), 5u);
	EXPECT_EQ(backends[4]->device, "@@@default@@@");
}

TEST_F(CellVoiceOutput, FailedDeviceCanBeReplacedWithoutRestart)
{
	next_open_ok = false;
	g_cfg.audio.voice_output_device.set("missing-headset");
	tick(3);
	ASSERT_EQ(backends.size(), 1u);
	EXPECT_FALSE(service.headset);
	EXPECT_TRUE(service.headset_failed);

	next_open_ok = true;
	g_cfg.audio.voice_output_device.set("connected-headset");
	tick();
	ASSERT_EQ(backends.size(), 2u);
	EXPECT_FALSE(service.headset_failed);
	ASSERT_TRUE(service.headset);
	EXPECT_TRUE(service.headset->IsPlaying());
	EXPECT_EQ(backends[1]->device, "connected-headset");
}

TEST_F(CellVoiceOutput, FormatAndBackendChangesReopenVoiceStream)
{
	g_cfg.audio.renderer.set(audio_renderer::cubeb);
	tick();
	g_cfg.audio.convert_to_s16.set(true);
	tick();
	ASSERT_EQ(backends.size(), 2u);
	EXPECT_EQ(backends[0]->close_count, 1u);
	EXPECT_EQ(backends[1]->get_sample_size(), sizeof(s16));
	g_cfg.audio.renderer.set(audio_renderer::null);
	tick();
	ASSERT_EQ(backends.size(), 3u);
	EXPECT_EQ(backends[1]->close_count, 1u);
}

TEST(CellVoiceInput, MuteTogglesOnPressAndOverridesPushToTalk)
{
	voice_input::microphone_control control;
	EXPECT_TRUE(control.update(false, false, false, true));
	EXPECT_FALSE(control.update(false, false, true, true));
	EXPECT_FALSE(control.update(false, false, true, true));
	EXPECT_FALSE(control.update(true, true, true, true));
	EXPECT_FALSE(control.update(true, true, false, true));
	EXPECT_TRUE(control.update(true, true, true, true));
	EXPECT_FALSE(control.update(true, false, false, true));
	EXPECT_TRUE(control.update(true, true, false, true));
}

TEST(CellVoiceInput, InputInterceptionClosesGateWithoutChangingMute)
{
	voice_input::microphone_control control;
	EXPECT_FALSE(control.update(false, false, true, false));
	EXPECT_FALSE(control.muted);
	EXPECT_TRUE(control.update(false, false, true, true));
	EXPECT_TRUE(control.update(false, false, false, true));
	EXPECT_FALSE(control.update(false, false, true, true));
	EXPECT_FALSE(control.update(false, false, false, false));
	EXPECT_TRUE(control.muted);
	EXPECT_FALSE(control.update(false, false, false, true));
}

TEST(CellVoiceInput, ReleaseAndRepressPublishesNewGeneration)
{
	voice_input::set_transmitting(true);
	const auto before = voice_input::talk.load();
	voice_input::set_transmitting(false);
	voice_input::set_transmitting(true);
	const auto after = voice_input::talk.load();
	EXPECT_TRUE(after->transmitting);
	EXPECT_EQ(after->generation, before->generation + 2);
}

#ifdef RPCS3_ENABLE_VOICE_TEST_FILE
namespace
{
	class CellVoiceMicrophone : public CellVoiceAsync
	{
	protected:
		std::string saved_audio = g_cfg.audio.to_string();

		void SetUp() override
		{
			CellVoiceAsync::SetUp();
			g_cfg.audio.microphone_type.set(microphone_handler::null);
			g_cfg.audio.microphone_devices.set("");
			voice_input::set_transmitting(true);

			g_cfg.audio.voice_test_file_enabled.set(false);
			g_cfg.audio.voice_test_file.set("");
			voice_input::set_transmitting(true);
			service.mic_file_loaded = true;
			service.mic_file.assign(voice_service::tick_samples, 2000.0f);
			manager.ports[pcm_input] = make_port(CELLVOICE_PORTTYPE_IN_MIC);
			manager.ports[voice_output] = make_port(CELLVOICE_PORTTYPE_OUT_VOICE);
			service.edges.emplace(pcm_input, voice_output);
		}

		void TearDown() override
		{
			service.stop(manager);
			g_cfg.audio.from_string(saved_audio);
			voice_input::set_transmitting(true);
		}

		void use_mp3()
		{
			g_cfg.audio.voice_test_file.set((std::filesystem::path(__FILE__).parent_path() / "data" / "voice-tone.mp3").generic_string());
			g_cfg.audio.voice_test_file_enabled.set(true);
		}

		bool wait_for_file()
		{
			for (u32 i = 0; i < 500; i++)
			{
				tick();
				if (service.mic_present()) return true;
				if (service.mic_failed) return false;
				std::this_thread::sleep_for(std::chrono::milliseconds(10));
			}
			return false;
		}
	};
}

TEST_F(CellVoiceMicrophone, VadThresholdControlsMicrophoneWithoutMutingOtherInputs)
{
	manager.ports[pcm_output] = make_port(CELLVOICE_PORTTYPE_OUT_PCMAUDIO);
	manager.ports[encoded_input_port] = make_port(CELLVOICE_PORTTYPE_IN_PCMAUDIO);
	service.edges.clear();
	service.edges.emplace(pcm_input, pcm_output);
	service.edges.emplace(encoded_input_port, pcm_output);
	tick(); // Complete the initial microphone source-change transition.
	const auto& microphone = manager.ports.at(pcm_input);
	ASSERT_EQ(service.set_port_attr(microphone, pcm_input, CELLVOICE_ATTR_ENERGY_LEVEL, 1), CELL_OK);
	for (const u32 threshold : {8u, 10u, 1u})
	{
		SCOPED_TRACE(threshold);
		ASSERT_EQ(service.set_port_attr(microphone, pcm_input, CELLVOICE_ATTR_VAD, threshold), CELL_OK);
		std::array<be_t<s16>, voice_service::tick_samples> other;
		other.fill(300);
		ASSERT_EQ(service.write(manager.ports.at(encoded_input_port), encoded_input_port,
			reinterpret_cast<const u8*>(other.data()), sizeof(other)), sizeof(other));
		tick();
		std::array<be_t<s16>, voice_service::tick_samples> mixed{};
		ASSERT_EQ(service.read(manager, manager.ports.at(pcm_output), pcm_output,
			reinterpret_cast<u8*>(mixed.data()), sizeof(mixed)), sizeof(mixed));
		for (const auto sample : mixed)
			EXPECT_EQ(static_cast<s16>(sample), threshold == 10 ? 300 : 2300);
		u32 activity = 0;
		ASSERT_EQ(service.get_port_attr(microphone, pcm_input, CELLVOICE_ATTR_VAD, activity), CELL_OK);
		EXPECT_EQ(activity, 9u); // Measurement continues even when the threshold rejects capture.
		u32 energy = 0;
		ASSERT_EQ(service.get_port_attr(microphone, pcm_input, CELLVOICE_ATTR_ENERGY_LEVEL, energy), CELL_OK);
		EXPECT_GT(std::bit_cast<f32>(energy), 40.0f);
	}
}

TEST_F(CellVoiceMicrophone, VadUsesIntegerThresholdAndFirmwareFallback)
{
	const auto& microphone = manager.ports.at(pcm_input);
	for (const u32 threshold : {0u, 11u, 0xffffffffu, 0x3f800000u})
	{
		ASSERT_EQ(service.set_port_attr(microphone, pcm_input, CELLVOICE_ATTR_VAD, threshold), CELL_OK);
		EXPECT_EQ(service.data.at(pcm_input).vad_threshold, threshold);
		tick();
		EXPECT_EQ(service.data.at(pcm_input).vad_threshold, 8u);
	}
	ASSERT_EQ(service.set_port_attr(microphone, pcm_input, CELLVOICE_ATTR_VAD, 1), CELL_OK);
	tick();
	EXPECT_EQ(service.data.at(pcm_input).vad_threshold, 1u);
	service.mic_file.assign(voice_service::tick_samples, 0.0f);
	tick();
	u32 activity = 0xdeadbeef;
	ASSERT_EQ(service.get_port_attr(microphone, pcm_input, CELLVOICE_ATTR_VAD, activity), CELL_OK);
	EXPECT_EQ(activity, 0u);
}

TEST_F(CellVoiceMicrophone, PushToSpeakDiscardsQueuedAndInFlightAudioBeforeNextTick)
{
	voice_input::set_transmitting(false);
	tick(3);
	EXPECT_TRUE(service.requests.empty());
	EXPECT_TRUE(service.mic_fifo.empty());
	voice_input::set_transmitting(true);
	tick();
	auto ready = take_requests();
	ASSERT_EQ(ready.size(), 1u);
	service.commit(manager, encoded(ready.front(), 1));
	tick();
	auto pending = take_requests();
	ASSERT_EQ(pending.size(), 1u);
	ASSERT_EQ(service.bytes_queued(voice_output), packet_bytes);
	voice_input::set_transmitting(false);
	std::array<u8, 255> out{};
	EXPECT_EQ(service.read(manager, manager.ports.at(voice_output), voice_output, out.data(), out.size()), 0u);
	service.commit(manager, encoded(pending.front(), 2));
	EXPECT_EQ(service.bytes_queued(voice_output), 0u);
	tick(20);
	EXPECT_TRUE(service.requests.empty());
	EXPECT_TRUE(service.data.at(voice_output).pcm.empty());
	voice_input::set_transmitting(true);
	tick();
	ASSERT_EQ(service.requests.size(), 1u);
	EXPECT_NE(service.requests.front().codec.generation, pending.front().codec.generation);
}

TEST_F(CellVoiceMicrophone, ReleaseAndRepressBetweenTicksRejectsOldCompletion)
{
	voice_input::set_transmitting(false);
	voice_input::set_transmitting(true);
	tick(2);
	auto pending = take_requests();
	ASSERT_EQ(pending.size(), 1u);
	voice_input::set_transmitting(false);
	voice_input::set_transmitting(true);
	service.commit(manager, encoded(pending.front(), 1));
	EXPECT_EQ(service.bytes_queued(voice_output), 0u);
	tick();
	ASSERT_EQ(service.requests.size(), 1u);
	EXPECT_NE(service.requests.front().codec.generation, pending.front().codec.generation);
}

TEST_F(CellVoiceMicrophone, BindingAndModeChangesApplyLiveWithoutMutingOtherInputs)
{
	voice_input::set_transmitting(false);
	voice_input::set_transmitting(true);
	tick(2);
	auto pending = take_requests();
	ASSERT_EQ(pending.size(), 1u);
	voice_input::set_transmitting(false);
	service.commit(manager, encoded(pending.front(), 1));
	EXPECT_EQ(service.bytes_queued(voice_output), 0u);
	tick();
	EXPECT_TRUE(service.requests.empty());
	// A separate game-supplied PCM route must keep working with the microphone gated.
	manager.ports[encoded_input_port] = make_port(CELLVOICE_PORTTYPE_IN_PCMAUDIO);
	manager.ports[pcm_output] = make_port(CELLVOICE_PORTTYPE_OUT_PCMAUDIO);
	service.edges.emplace(encoded_input_port, pcm_output);
	std::array<be_t<s16>, voice_service::tick_samples> pcm;
	pcm.fill(1000);
	service.write(manager.ports.at(encoded_input_port), encoded_input_port, reinterpret_cast<const u8*>(pcm.data()), sizeof(pcm));
	tick();
	EXPECT_EQ(service.bytes_queued(pcm_output), sizeof(pcm));
	voice_input::set_transmitting(true);
	tick();
	EXPECT_EQ(service.requests.size(), 1u);
}

TEST_F(CellVoiceMicrophone, FailedFileCanBeReplacedLiveAndPhysicalSettingsAreDynamic)
{
	g_cfg.audio.voice_test_file_enabled.set(true);
	g_cfg.audio.voice_test_file.set("/nonexistent-rpcs3-voice-test.mp3");
	tick();
	EXPECT_TRUE(service.mic_failed);
	EXPECT_FALSE(service.mic_present());
	use_mp3();
	ASSERT_TRUE(wait_for_file());
	EXPECT_FALSE(service.mic_failed);
	g_cfg.audio.voice_test_file_enabled.set(false);
	tick(2);
	EXPECT_TRUE(service.mic_present());
	EXPECT_FALSE(service.mic_decoder);
	EXPECT_FALSE(service.requests.empty());
	// Exercise the same dynamic-only deserialization used by Settings > Apply.
	cfg_root live;
	cfg_root updated;
	updated.audio.microphone_type.set(microphone_handler::standard);
	updated.audio.microphone_devices.set("New microphone@@@");
	updated.audio.voice_test_file_enabled.set(true);
	updated.audio.voice_test_file.set("test.mp3");
	ASSERT_TRUE(live.from_string(updated.to_string(), true));
	EXPECT_EQ(live.audio.microphone_type.get(), microphone_handler::standard);
	EXPECT_EQ(live.audio.microphone_devices.get(), "New microphone@@@");
	EXPECT_TRUE(live.audio.voice_test_file_enabled.get());
	EXPECT_EQ(live.audio.voice_test_file.get(), "test.mp3");
}

TEST_F(CellVoiceMicrophone, Mp3ResamplesAndLoopsAcrossTickBoundaries)
{
	service.edges.clear();
	manager.ports[pcm_output] = make_port(CELLVOICE_PORTTYPE_OUT_PCMAUDIO);
	service.edges.emplace(pcm_input, pcm_output);
	use_mp3();
	ASSERT_TRUE(wait_for_file());
	tick(); // Finish the attachment transition.
	std::vector<f32> clip;
	{
		std::lock_guard lock(service.mic_decoder->data_mutex());
		clip.resize(service.mic_decoder->size() / sizeof(f32));
		service.mic_decoder->read(clip.data(), 0, clip.size() * sizeof(f32));
	}
	// The fixture is 137 ms of 44.1 kHz stereo MP3; output is 16 kHz mono.
	ASSERT_GT(clip.size(), 1900u);
	ASSERT_LT(clip.size(), 2600u);
	ASSERT_GT(*std::max_element(clip.begin(), clip.end()), 0.01f);
	service.mic_decoder_pos = 0;
	service.mic_fifo.clear();
	service.mic_primed = false;
	usz position = 0;
	for (u32 i = 0; i < 25; i++)
	{
		tick();
		const auto& signal = service.data.at(pcm_input).signal;
		for (const f32 sample : signal)
		{
			EXPECT_FLOAT_EQ(sample, std::clamp(clip[position], -1.0f, 1.0f) * 32768.0f);
			position = (position + 1) % clip.size();
		}
	}
	EXPECT_TRUE(service.requests.empty());
}

TEST_F(CellVoiceMicrophone, DeviceSelectionChangeDiscardsOldAudioAndRetriesFailedCapture)
{
	tick(2);
	auto pending = take_requests();
	ASSERT_EQ(pending.size(), 1u);
	service.mic_failed = true;
	service.mic_resample_pos = 0.75;
	service.mic_last = 12345.0f;
	g_cfg.audio.microphone_devices.set("Changed device@@@");
	service.commit(manager, encoded(pending.front(), 1));
	EXPECT_EQ(service.bytes_queued(voice_output), 0u);
	tick(2);
	EXPECT_EQ(service.mic_config_device, "Changed device");
	EXPECT_FALSE(service.mic_failed);
	EXPECT_DOUBLE_EQ(service.mic_resample_pos, 0.0);
	EXPECT_FLOAT_EQ(service.mic_last, 0.0f);
	ASSERT_EQ(service.requests.size(), 1u);
	EXPECT_NE(service.requests.front().codec.generation, pending.front().codec.generation);
}

TEST_F(CellVoiceMicrophone, DecoderRejectsAudioBeyondItsMemoryLimit)
{
	utils::audio_decoder decoder(16000, true, 100 * sizeof(f32));
	music_selection_context context;
	context.playlist.push_back((std::filesystem::path(__FILE__).parent_path() / "data" / "voice-tone.mp3").generic_string());
	decoder.set_context(std::move(context));
	decoder.decode();
	for (u32 i = 0; i < 500 && !decoder.track_fully_decoded(); i++)
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	ASSERT_TRUE(decoder.track_fully_decoded());
	EXPECT_TRUE(decoder.has_error());
	std::lock_guard lock(decoder.data_mutex());
	EXPECT_LE(decoder.size(), 100 * sizeof(f32));
}
#endif
