#include "stdafx.h"
#include <gmock/gmock.h>
#include "Emu/Cell/Modules/cellDmuxPamf.h"
#include "Crypto/sha1.h"
#include "test_dmux_pamf.h"

using namespace testing;

struct DmuxPamfMock : dmux_pamf_base
{
	MOCK_METHOD(bool, on_au_found, (u8 stream_id, u8 private_stream_id, u32 user_data, std::span<const u8> au, u64 pts, u64 dts, bool rap, u8 au_specific_info_size, (std::array<u8, 0x10>) au_specific_info_buf), (override));
	MOCK_METHOD(bool, on_demux_done, (), (override));
	MOCK_METHOD(void, on_fatal_error, (), (override));
	MOCK_METHOD(bool, on_flush_done, (u8 stream_id, u8 private_stream_id, u32 user_data), (override));
	MOCK_METHOD(bool, on_prog_end, (), (override));
	MOCK_METHOD(void, on_au_queue_full, (), (override));
};

struct DmuxPamfTest : Test
{
	StrictMock<DmuxPamfMock> demuxer;
	MockFunction<void(u32 check_point_number)> check_point;
	std::array<u8, 0x3000> au_queue_buffer{};
	InSequence seq;
};

MATCHER_P(Sha1Is, expected_sha1, "") { std::array<u8, 20> sha1_buf{}; sha1(arg.data(), arg.size(), sha1_buf.data()); return sha1_buf == expected_sha1; }

struct DmuxPamfSinglePack : DmuxPamfTest, WithParamInterface<std::tuple<u8, u8, bool, u32, std::array<u8, 0x800>, AccessUnit, std::optional<AccessUnit>>> {};

TEST_P(DmuxPamfSinglePack, Test)
{
	const auto& [stream_id, private_stream_id, is_avc, au_max_size, stream, au_1, au_2] = GetParam();

	EXPECT_CALL(check_point, Call(0));
	EXPECT_CALL(demuxer, on_au_found(stream_id, private_stream_id, 0x12345678, Sha1Is(au_1.data_sha1), au_1.pts, au_1.dts, au_1.rap, au_1.specific_info_size, au_1.specific_info_buf)).WillOnce(Return(true));

	if (au_2)
	{
		EXPECT_CALL(demuxer, on_au_found(stream_id, private_stream_id, 0x12345678, Sha1Is(au_2->data_sha1), au_2->pts, au_2->dts, au_2->rap, au_2->specific_info_size, au_2->specific_info_buf));
	}

	demuxer.enable_es(stream_id, private_stream_id, is_avc, au_queue_buffer, au_max_size, false, 0x12345678);
	demuxer.set_stream(stream, false);
	check_point.Call(0);
	demuxer.process_next_pack();
}

// These streams are made of a single pack that contains two access units (one for the user data stream)
INSTANTIATE_TEST_SUITE_P(Basic, DmuxPamfSinglePack, Values
(
	std::tuple{ 0xe0, 0x00, true, 0x800, AVC_SINGLE_PACK_STREAM, AVC_SINGLE_PACK_EXPECTED_AU_1, AVC_SINGLE_PACK_EXPECTED_AU_2 },
	std::tuple{ 0xe0, 0x00, false, 0x800, M2V_SINGLE_PACK_STREAM, M2V_SINGLE_PACK_EXPECTED_AU_1, M2V_SINGLE_PACK_EXPECTED_AU_2 },
	std::tuple{ 0xbd, 0x00, false, 0x800, ATRACX_SINGLE_PACK_STREAM, ATRACX_SINGLE_PACK_EXPECTED_AU_1, ATRACX_SINGLE_PACK_EXPECTED_AU_2 },
	std::tuple{ 0xbd, 0x30, false, 0x800, AC3_SINGLE_PACK_STREAM, AC3_SINGLE_PACK_EXPECTED_AU_1, AC3_SINGLE_PACK_EXPECTED_AU_2 },
	std::tuple{ 0xbd, 0x40, false, 0x369, LPCM_SINGLE_PACK_STREAM, LPCM_SINGLE_PACK_EXPECTED_AU_1, LPCM_SINGLE_PACK_EXPECTED_AU_2 },
	std::tuple{ 0xbd, 0x20, false, 0x800, USER_DATA_SINGLE_PACK_STREAM, USER_DATA_SINGLE_PACK_EXPECTED_AU, std::nullopt } // There can be at most one user data access unit in a single pack
));

// Tests buggy behavior: timestamps should be unsigned, but get sign-extended from s32 to u64 on LLE. They probably forgot about integer promotion
INSTANTIATE_TEST_SUITE_P(TimeStampSignExtended, DmuxPamfSinglePack, Values
(
	std::tuple{ 0xbd, 0x20, false, 0x800, TIME_STAMP_SIGN_EXTENDED_STREAM, TIME_STAMP_SIGN_EXTENDED_EXPECTED_AU, std::nullopt }
));

// Tests buggy behavior: if there is no Pack Start Code at the beginning of the current stream section, LLE will search for the code.
// However, if it finds one, it will not set the reading position to where the code was found. It continues reading from the beginning of the stream section instead.
// Test disabled since this is not currently implemented
INSTANTIATE_TEST_SUITE_P(DISABLED_PackStartCodeNotAtBeginning, DmuxPamfSinglePack, Values
(
	std::tuple{ 0xe0, 0x00, true, 0x800, []() { auto pack = AVC_SINGLE_PACK_STREAM; std::copy_n(pack.begin(), 4, pack.begin() + 0x93); std::fill_n(pack.begin(), 4, 0); return pack; }(), AVC_SINGLE_PACK_EXPECTED_AU_1, AVC_SINGLE_PACK_EXPECTED_AU_2 }
));

struct DmuxPamfMultiplePackSingleAu : DmuxPamfTest, WithParamInterface<std::tuple<u8, u8, bool, u32, std::vector<u8>, AccessUnit>> {};

TEST_P(DmuxPamfMultiplePackSingleAu, Test)
{
	const auto& [stream_id, private_stream_id, is_avc, au_max_size, stream, au] = GetParam();

	EXPECT_CALL(check_point, Call(0));
	EXPECT_CALL(demuxer, on_au_found(stream_id, private_stream_id, 0x12345678, Sha1Is(au.data_sha1), au.pts, au.dts, au.rap, au.specific_info_size, au.specific_info_buf)).WillOnce(Return(true));

	demuxer.enable_es(stream_id, private_stream_id, is_avc, au_queue_buffer, au_max_size, false, 0x12345678);
	demuxer.set_stream(stream, false);

	// Demultiplex all packs until the last one
	for (u32 i = 0; i < stream.size() - 0x800; i += 0x800)
	{
		demuxer.process_next_pack();
	}

	// The next call to process_next_pack() should produce an access unit
	check_point.Call(0);
	demuxer.process_next_pack();
}

// These streams contain a single access unit that is split across four packs
INSTANTIATE_TEST_SUITE_P(Basic, DmuxPamfMultiplePackSingleAu, Values
(
	std::tuple{ 0xe0, 0x00, true, 0x1800, AVC_LARGE_AU_STREAM, AVC_LARGE_AU_EXPECTED_AU },
	std::tuple{ 0xe0, 0x00, false, 0x1800, M2V_LARGE_AU_STREAM, M2V_LARGE_AU_EXPECTED_AU },
	std::tuple{ 0xbd, 0x00, false, 0x1800, ATRACX_LARGE_AU_STREAM, ATRACX_LARGE_AU_EXPECTED_AU },
	std::tuple{ 0xbd, 0x30, false, 0x1800, AC3_LARGE_AU_STREAM, AC3_LARGE_AU_EXPECTED_AU },
	std::tuple{ 0xbd, 0x40, false, 0x1400, LPCM_LARGE_AU_STREAM, LPCM_LARGE_AU_EXPECTED_AU },
	std::tuple{ 0xbd, 0x20, false, 0x1800, USER_DATA_LARGE_AU_STREAM, USER_DATA_LARGE_AU_EXPECTED_AU }
));

// PES packets with audio streams contain a proprietary header before the actual stream.
// The last two bytes of the header indicate how much of the stream should be skipped
// (note that I have not found a single PAMF stream in any game where this is not 0).
// These streams test if this is implemented properly, the first access unit should not be output
INSTANTIATE_TEST_SUITE_P(SkipByes, DmuxPamfMultiplePackSingleAu, Values
(
	std::tuple{ 0xbd, 0x00, false, 0x1800, ATRACX_SKIP_BYTES_STREAM, ATRACX_SKIP_BYTES_EXPECTED_AU },
	std::tuple{ 0xbd, 0x30, false, 0x1800, AC3_SKIP_BYTES_STREAM, AC3_SKIP_BYTES_EXPECTED_AU },
	std::tuple{ 0xbd, 0x40, false, 0xb40, LPCM_SKIP_BYTES_STREAM, LPCM_SKIP_BYTES_EXPECTED_AU }
));

// Tests buggy behavior: if the start of an access unit in the last three bytes of the previous PES packet and the demuxer was not cutting out an access unit before that,
// LLE will always output all three bytes, even if the access unit starts at the second last or last byte
INSTANTIATE_TEST_SUITE_P(FirstDilimiterSplit, DmuxPamfMultiplePackSingleAu, Values
(
	std::tuple{ 0xe0, 0x00, true, 0x1800, AVC_BEGIN_OF_AU_SPLIT_STREAM, AVC_BEGIN_OF_AU_SPLIT_EXPECTED_AU },
	std::tuple{ 0xe0, 0x00, false, 0x1800, M2V_BEGIN_OF_AU_SPLIT_STREAM, M2V_BEGIN_OF_AU_SPLIT_EXPECTED_AU },
	std::tuple{ 0xbd, 0x00, false, 0x1800, ATRACX_BEGIN_OF_AU_SPLIT_STREAM, ATRACX_BEGIN_OF_AU_SPLIT_EXPECTED_AU },
	std::tuple{ 0xbd, 0x30, false, 0x1800, AC3_BEGIN_OF_AU_SPLIT_STREAM, AC3_BEGIN_OF_AU_SPLIT_EXPECTED_AU }
));

// The distance of the access units in these streams do not match the size indicated in the ATRAC3plus ATS header/AC3 frmsizecod + fscod fields
// LLE repeatedly cuts off one byte at the beginning of the PES packet until the sizes match and clears the current access unit
INSTANTIATE_TEST_SUITE_P(SizeMismatch, DmuxPamfMultiplePackSingleAu, Values
(
	std::tuple{ 0xbd, 0x00, false, 0x608, ATRACX_SIZE_MISMATCH_STREAM, ATRACX_SIZE_MISMATCH_EXPECTED_AU },
	std::tuple{ 0xbd, 0x30, false, 0x600, AC3_SIZE_MISMATCH_STREAM, AC3_SIZE_MISMATCH_EXPECTED_AU }
));

// Tests buggy behavior: if an M2V sequence end code is found, it should be included in the current access unit, unlike all other delimiters.
// If the sequence end code is split across two packs, LLE isn't properly including it in the access unit.
// Disabled because this behavior isn't properly implemented currently
INSTANTIATE_TEST_SUITE_P(DISABLED_M2vSequenceEndSplit, DmuxPamfMultiplePackSingleAu, Values
(
	std::tuple{ 0xe0, 0x00, false, 0x1000, M2V_SEQUENCE_END_SPLIT_STREAM, M2V_SEQUENCE_END_SPLIT_EXPECTED_AU }
));

struct DmuxPamfSplitDelimiter : DmuxPamfTest, WithParamInterface<std::tuple<u32, u32, bool, u32, std::array<u8, 0x2000>, AccessUnit, AccessUnit, AccessUnit, AccessUnit>> {};

TEST_P(DmuxPamfSplitDelimiter, Test)
{
	const auto& [stream_id, private_stream_id, is_avc, au_max_size, stream, au_1, au_2, au_3, au_4] = GetParam();

	EXPECT_CALL(check_point, Call(0));
	EXPECT_CALL(demuxer, on_au_found(stream_id, private_stream_id, 0x12345678, Sha1Is(au_1.data_sha1), au_1.pts, au_1.dts, au_1.rap, au_1.specific_info_size, au_1.specific_info_buf)).WillOnce(Return(true));
	EXPECT_CALL(check_point, Call(1));
	EXPECT_CALL(demuxer, on_au_found(stream_id, private_stream_id, 0x12345678, Sha1Is(au_2.data_sha1), au_2.pts, au_2.dts, au_2.rap, au_2.specific_info_size, au_2.specific_info_buf)).WillOnce(Return(true));
	EXPECT_CALL(check_point, Call(2));
	EXPECT_CALL(demuxer, on_au_found(stream_id, private_stream_id, 0x12345678, Sha1Is(au_3.data_sha1), au_3.pts, au_3.dts, au_3.rap, au_3.specific_info_size, au_3.specific_info_buf)).WillOnce(Return(true));
	EXPECT_CALL(demuxer, on_au_found(stream_id, private_stream_id, 0x12345678, Sha1Is(au_4.data_sha1), au_4.pts, au_4.dts, au_4.rap, au_4.specific_info_size, au_4.specific_info_buf)).WillOnce(Return(true));

	demuxer.enable_es(stream_id, private_stream_id, is_avc, au_queue_buffer, au_max_size, false, 0x12345678);
	demuxer.set_stream(stream, false);
	demuxer.process_next_pack();
	check_point.Call(0);
	demuxer.process_next_pack();
	check_point.Call(1);
	demuxer.process_next_pack();
	check_point.Call(2);
	demuxer.process_next_pack();
}

// The delimiters of each stream is split across two packs. Tests, if it is still found and the access units are extracted properly
INSTANTIATE_TEST_SUITE_P(Basic, DmuxPamfSplitDelimiter, Values
(
	std::tuple{ 0xe0, 0x00, true, 0x800, AVC_MULTIPLE_PACK_STREAM, AVC_MULTIPLE_PACK_EXPECTED_AU_1, AVC_MULTIPLE_PACK_EXPECTED_AU_2, AVC_MULTIPLE_PACK_EXPECTED_AU_3, AVC_MULTIPLE_PACK_EXPECTED_AU_4 },
	std::tuple{ 0xe0, 0x00, false, 0x800, M2V_MULTIPLE_PACK_STREAM, M2V_MULTIPLE_PACK_EXPECTED_AU_1, M2V_MULTIPLE_PACK_EXPECTED_AU_2, M2V_MULTIPLE_PACK_EXPECTED_AU_3, M2V_MULTIPLE_PACK_EXPECTED_AU_4 },
	std::tuple{ 0xbd, 0x00, false, 0x800, ATRACX_MULTIPLE_PACK_STREAM, ATRACX_MULTIPLE_PACK_EXPECTED_AU_1, ATRACX_MULTIPLE_PACK_EXPECTED_AU_2, ATRACX_MULTIPLE_PACK_EXPECTED_AU_3, ATRACX_MULTIPLE_PACK_EXPECTED_AU_4 },
	std::tuple{ 0xbd, 0x30, false, 0x800, AC3_MULTIPLE_PACK_STREAM, AC3_MULTIPLE_PACK_EXPECTED_AU_1, AC3_MULTIPLE_PACK_EXPECTED_AU_2, AC3_MULTIPLE_PACK_EXPECTED_AU_3, AC3_MULTIPLE_PACK_EXPECTED_AU_4 }
));

struct DmuxPamfNoAccessUnits : DmuxPamfTest, WithParamInterface<std::tuple<u32, u32, bool, std::array<u8, 0x2000>>> {};

TEST_P(DmuxPamfNoAccessUnits, Test)
{
	const auto& [stream_id, private_stream_id, is_avc, stream] = GetParam();

	demuxer.enable_es(stream_id, private_stream_id, is_avc, au_queue_buffer, 0x1000, false, 0x12345678);
	demuxer.set_stream(stream, false);
	demuxer.process_next_pack();
	demuxer.process_next_pack();
	demuxer.process_next_pack();
	demuxer.process_next_pack();
}

// Tests buggy behavior: LLE doesn't handle multiple consecutive PES packets with less than three bytes correctly.
// These streams contain access units, but they are not found on LLE
INSTANTIATE_TEST_SUITE_P(TinyPackets, DmuxPamfNoAccessUnits, Values
(
	std::tuple{ 0xe0, 0x00, true, AVC_TINY_PACKETS_STREAM },
	std::tuple{ 0xe0, 0x00, false, M2V_TINY_PACKETS_STREAM }
));

struct DmuxPamfInvalidStream : DmuxPamfTest, WithParamInterface<std::array<u8, 0x800>> {};

TEST_P(DmuxPamfInvalidStream, Test)
{
	demuxer.set_stream(GetParam(), false);
	demuxer.enable_es(0xbd, 0x00, false, au_queue_buffer, 0x800, false, 0);
	demuxer.enable_es(0xbd, 0x20, false, au_queue_buffer, 0x800, false, 0);
	demuxer.enable_es(0xbd, 0x40, false, au_queue_buffer, 0x800, false, 0);
	EXPECT_FALSE(demuxer.process_next_pack());
}

// Tests if invalid streams are handled correctly.
// Each of these should fail a different check in process_next_pack()
// (LLE doesn't actually check anything, it would just read random things in the SPU local store)
INSTANTIATE_TEST_SUITE_P(Instance, DmuxPamfInvalidStream, Values
(
	[]() consteval { auto pack = AVC_SINGLE_PACK_STREAM; pack[0xe] = 0x01; return pack; }(),                                                                  // Invalid code after pack header
	[]() consteval { auto pack = AVC_SINGLE_PACK_STREAM; pack[0x12] = 0x07; pack[0x13] = 0xe7; return pack; }(),                                              // System header size too large
	[]() consteval { auto pack = AVC_SINGLE_PACK_STREAM; pack[0x20] = 0x01; return pack; }(),                                                                 // Invalid code after system header
	[]() consteval { auto pack = AVC_SINGLE_PACK_STREAM; pack[0x24] = 0x07; pack[0x25] = 0xd2; return pack; }(),                                              // Private stream 2 size too large
	[]() consteval { auto pack = AVC_SINGLE_PACK_STREAM; pack[0x114] = 0x01; return pack; }(),                                                                // Invalid code after private stream 2
	[]() consteval { auto pack = AVC_SINGLE_PACK_STREAM; pack[0x117] = 0xbe; return pack; }(),                                                                // Invalid stream type
	[]() consteval { auto pack = AVC_SINGLE_PACK_STREAM; pack[0x119] = 0xe7; return pack; }(),                                                                // PES packet size too large
	[]() consteval { auto pack = AVC_SINGLE_PACK_STREAM; pack[0x118] = 0x00; pack[0x119] = 0x03; pack[0x11c] = 0x00; return pack; }(),                        // PES packet header size too large
	[]() consteval { auto pack = USER_DATA_SINGLE_PACK_STREAM; pack[0x118] = 0x00; pack[0x119] = 0x15; return pack; }(),                                      // User data stream too small
	[]() consteval { auto pack = LPCM_SINGLE_PACK_STREAM; pack[0x118] = 0x00; pack[0x119] = 0x20; pack[0x12c] = 0x00; pack[0x12d] = 0x00; return pack; }(),   // LPCM stream too small
	[]() consteval { auto pack = ATRACX_SINGLE_PACK_STREAM; pack[0x118] = 0x00; pack[0x119] = 0x13; pack[0x12c] = 0x00; pack[0x12d] = 0x00; return pack; }(), // Stream header too small
	[]() consteval { auto pack = AVC_SINGLE_PACK_STREAM; pack[0x118] = 0x00; pack[0x119] = 0x03; pack[0x11c] = 0x00; return pack; }()                         // PES packet header size too large
));

// Tests if the program end code is properly detected and the corresponding event is fired
TEST_F(DmuxPamfTest, ProgEnd)
{
	EXPECT_CALL(check_point, Call(0));
	EXPECT_CALL(demuxer, on_prog_end).WillOnce(Return(false)); // Since this returns false, the event should be fired again immediately after the next process_next_pack() call
	EXPECT_CALL(check_point, Call(1));
	EXPECT_CALL(demuxer, on_prog_end).WillOnce(Return(true));

	constexpr std::array<u8, 0x800> stream = { 0x00, 0x00, 0x01, 0xb9 };

	demuxer.set_stream(stream, false);
	check_point.Call(0);
	demuxer.process_next_pack();
	check_point.Call(1);
	demuxer.process_next_pack();
}

// Tests if the output queue properly checks whether there is enough space to write an au_chunk to the queue
TEST_F(DmuxPamfTest, QueueNoSpaceForAuChunk)
{
	EXPECT_CALL(check_point, Call(0));
	EXPECT_CALL(demuxer, on_au_found).WillOnce(Return(true));
	EXPECT_CALL(demuxer, on_au_queue_full);
	EXPECT_CALL(check_point, Call(1));
	EXPECT_CALL(demuxer, on_au_queue_full);

	// Set a large au_max_size, this causes the back pointer of the queue to wrap around to the beginning after the first access units
	demuxer.enable_es(0xe0, 0x00, true, au_queue_buffer, 0x2c14, false, 0);

	demuxer.set_stream(AVC_SINGLE_PACK_STREAM, false);

	// Since the first access unit is behind the back pointer and there is no space to write the next au_chunk to the queue, this should cause the au_queue_full event to be fired
	check_point.Call(0);
	demuxer.process_next_pack();

	// Since we didn't release the first access unit to remove it from the queue, this should result in au_queue_full again
	check_point.Call(1);
	demuxer.process_next_pack();
}

// Tests if access units are properly removed from the queue
TEST_F(DmuxPamfTest, QueueReleaseAu)
{
	EXPECT_CALL(check_point, Call(0));
	EXPECT_CALL(demuxer, on_au_found).WillOnce(Return(true));
	EXPECT_CALL(demuxer, on_au_queue_full);
	EXPECT_CALL(check_point, Call(1));
	EXPECT_CALL(demuxer, on_au_found).WillOnce(Return(true));

	demuxer.enable_es(0xe0, 0x00, true, au_queue_buffer, 0x2c14, false, 0);
	demuxer.set_stream(AVC_SINGLE_PACK_STREAM, false);
	check_point.Call(0);
	demuxer.process_next_pack();
	demuxer.release_au(0xe0, 0x00, 0x3ed); // Should remove the first access unit from the queue
	check_point.Call(1);
	demuxer.process_next_pack(); // Since there is space again in the queue, the second access unit should now be found
}

// LLE adds 0x10 bytes to the size of the au_chunk when checking whether there is enough space.
TEST_F(DmuxPamfTest, QueueNoSpaceSPUShenanigans)
{
	EXPECT_CALL(check_point, Call(0));
	EXPECT_CALL(demuxer, on_au_found).Times(2).WillRepeatedly(Return(true));
	EXPECT_CALL(demuxer, on_au_queue_full);
	EXPECT_CALL(check_point, Call(1));
	EXPECT_CALL(demuxer, on_au_queue_full);

	std::array<u8, 0x800> small_au_queue_buffer{};

	demuxer.enable_es(0xe0, 0x00, true, small_au_queue_buffer, 0x400, false, 0);
	demuxer.set_stream(AVC_SINGLE_PACK_STREAM, false);
	check_point.Call(0);
	demuxer.process_next_pack();
	demuxer.release_au(0xe0, 0x00, 0x3fc); // 0xf bytes more than needed, but this should still result in au_queue_full
	demuxer.set_stream(AVC_SINGLE_PACK_STREAM, false);
	check_point.Call(1);
	demuxer.process_next_pack();
}

// After completing an access unit, it should be checked whether au_max_size fits into the queue.
TEST_F(DmuxPamfTest, QueueNoSpaceForMaxAu)
{
	EXPECT_CALL(check_point, Call(0));
	EXPECT_CALL(demuxer, on_au_found).WillOnce(Return(true));
	EXPECT_CALL(demuxer, on_au_queue_full);
	EXPECT_CALL(check_point, Call(1));
	EXPECT_CALL(demuxer, on_au_found).WillOnce(Return(true));
	EXPECT_CALL(demuxer, on_au_queue_full);

	std::array<u8, 0x400> small_au_queue_buffer{};

	demuxer.enable_es(0xe0, 0x00, true, small_au_queue_buffer, 0x400, false, 0);
	demuxer.set_stream(AVC_SINGLE_PACK_STREAM, false);
	check_point.Call(0);
	demuxer.process_next_pack();
	demuxer.release_au(0xe0, 0x00, 0x300); // This does not remove the entire access unit from the queue

	// Since there is still data behind the back pointer of the queue, it can't wrap around again.
	// Because au_max_size won't fit between the back pointer and the end of the queue buffer after finding the next access unit,
	// this should cause au_queue_full before starting to extract the next access unit
	check_point.Call(1);
	demuxer.process_next_pack();
}

// Tests if a fatal error is produced if an access unit in the stream is larger than au_max_size
TEST_F(DmuxPamfTest, QueueAuLargerThanMaxSize)
{
	EXPECT_CALL(check_point, Call(0));
	EXPECT_CALL(demuxer, on_fatal_error);
	EXPECT_CALL(demuxer, on_au_queue_full);
	EXPECT_CALL(check_point, Call(1));
	EXPECT_CALL(demuxer, on_fatal_error);
	EXPECT_CALL(demuxer, on_au_queue_full);

	std::array<u8, 0x800> small_au_queue_buffer{};

	demuxer.enable_es(0xe0, 0x00, true, small_au_queue_buffer, 0x800, false, 0); // au_max_size is smaller than the access unit in the stream
	demuxer.set_stream(AVC_LARGE_AU_STREAM, false);
	demuxer.process_next_pack();
	check_point.Call(0);
	demuxer.process_next_pack();
	check_point.Call(1);
	demuxer.process_next_pack();
}

// LLE sets au_max_size to 0x800 if it is too large (no fatal error or anything else)
TEST_F(DmuxPamfTest, EnableEsAuMaxSizeTooLarge)
{
	EXPECT_CALL(check_point, Call(0));
	EXPECT_CALL(demuxer, on_au_found).Times(2).WillRepeatedly(Return(true));

	demuxer.enable_es(0xe0, 0x00, true, au_queue_buffer, std::numeric_limits<u32>::max(), false, 0); // The access unit max size will be set to 0x800 if it is larger than the queue buffer size or UINT32_MAX
	demuxer.set_stream(AVC_SINGLE_PACK_STREAM, false);

	// Normally, this would cause the event au_queue_full because au_max_size wouldn't fit in the queue.
	// However, since au_max_size gets set to 0x800, this doesn't occur
	check_point.Call(0);
	demuxer.process_next_pack();
}

// Tests if the demux_done event is fired when process_next_pack() is called after the end of the stream has been reached
TEST_F(DmuxPamfTest, DemuxDone)
{
	EXPECT_CALL(check_point, Call(0));
	EXPECT_CALL(demuxer, on_demux_done).WillOnce(Return(true));

	demuxer.enable_es(0xe0, 0x00, true, au_queue_buffer, 0x1800, false, 0);
	demuxer.set_stream({ AVC_MULTIPLE_PACK_STREAM.begin(), 0x800 }, false);
	demuxer.process_next_pack();
	check_point.Call(0);
	demuxer.process_next_pack();
	demuxer.process_next_pack(); // Since on_demux_done was already called and returned true, it should not produce another demux_done event
}

// process_next_pack() should skip through the stream until it finds a start code or the end of the stream is reached
TEST_F(DmuxPamfTest, DemuxDoneEmptyStream)
{
	EXPECT_CALL(check_point, Call(0));
	EXPECT_CALL(demuxer, on_demux_done).WillOnce(Return(true));

	constexpr std::array<u8, 0x10000> stream{};

	demuxer.set_stream(stream, false);

	// Should immediately cause demux_done since there is no start code in the stream
	check_point.Call(0);
	demuxer.process_next_pack();
}

// Tests if the demuxer handles an input stream that is a continuation of the previous one correctly
TEST_F(DmuxPamfTest, StreamContinuity)
{
	EXPECT_CALL(check_point, Call(0));
	EXPECT_CALL(demuxer, on_au_found).WillOnce(Return(true));

	demuxer.enable_es(0xe0, 0x00, true, au_queue_buffer, 0x1800, false, 0);
	demuxer.set_stream({ AVC_MULTIPLE_PACK_STREAM.begin(), 0x800 }, false);
	demuxer.process_next_pack();
	demuxer.set_stream({ AVC_MULTIPLE_PACK_STREAM.begin() + 0x800, AVC_MULTIPLE_PACK_STREAM.end() }, true);
	check_point.Call(0);
	demuxer.process_next_pack();
}

// Flushing an elementary stream manually produces an au_found event
TEST_F(DmuxPamfTest, Flush)
{
	EXPECT_CALL(check_point, Call(0));
	EXPECT_CALL(demuxer, on_au_found(0xe0, 0x00, 0, Sha1Is(AVC_FLUSH_EXPECTED_AU_SHA1), 0x15f90, 0x159b2, true, 0, Each(0))).WillOnce(Return(true));
	EXPECT_CALL(demuxer, on_flush_done).Times(10).WillRepeatedly(Return(false)); // The flush_done event is fired repeatedly in a loop until it returns true
	EXPECT_CALL(demuxer, on_flush_done).WillOnce(Return(true));

	demuxer.enable_es(0xe0, 0x00, true, au_queue_buffer, 0x800, false, 0);
	demuxer.set_stream(AVC_MULTIPLE_PACK_STREAM, false);
	demuxer.process_next_pack();
	check_point.Call(0);
	demuxer.flush_es(0xe0, 0x00);
	demuxer.process_next_pack(); // The elementary stream should be reset after a flush, so this should not find the next access unit
}

// Tests resetting the stream
// On LLE, it is necessary to reset the stream before setting a new one, if the current stream has not been entirely consumed yet
TEST_F(DmuxPamfTest, ResetStream)
{
	EXPECT_CALL(check_point, Call(0));
	EXPECT_CALL(demuxer, on_demux_done).WillOnce(Return(true));

	demuxer.enable_es(0xe0, 0x00, true, au_queue_buffer, 0x1800, false, 0);
	demuxer.set_stream(AVC_MULTIPLE_PACK_STREAM, false);
	demuxer.reset_stream();
	check_point.Call(0);
	demuxer.process_next_pack(); // Should produce a demux_done event and shouldn't find an access unit since the stream was reset
	demuxer.process_next_pack();
}

// Disabling an elementary stream should clear the access unit queue and internal state, and the disabled stream should not be demultiplexed anymore
TEST_F(DmuxPamfTest, DisableEs)
{
	demuxer.enable_es(0xe0, 0x00, true, au_queue_buffer, 0x1800, false, 0);
	demuxer.set_stream(AVC_MULTIPLE_PACK_STREAM, false);
	demuxer.process_next_pack();
	demuxer.disable_es(0xe0, 0x00);

	// Should not find an access unit since the elementary stream was disabled
	demuxer.process_next_pack();
	demuxer.process_next_pack();
	demuxer.process_next_pack();
}

// Resetting an elementary stream should clear the queue and internal state, like disabling, except the elementary stream remains enabled
TEST_F(DmuxPamfTest, ResetEs)
{
	EXPECT_CALL(check_point, Call(0));
	EXPECT_CALL(demuxer, on_au_found).WillOnce(Return(0));

	demuxer.enable_es(0xe0, 0x00, true, au_queue_buffer, 0x1800, false, 0);
	demuxer.set_stream(AVC_MULTIPLE_PACK_STREAM, false);
	demuxer.process_next_pack();
	demuxer.reset_es(0xe0, 0x00, nullptr);
	demuxer.process_next_pack(); // Should not find an access unit since the output queue and the current access unit state was cleared
	check_point.Call(0);
	demuxer.process_next_pack();
}

// If reset_es is called with an address, only the most recent access unit should be removed from the queue. Nothing else should be reset
TEST_F(DmuxPamfTest, ResetEsSingleAu)
{
	EXPECT_CALL(check_point, Call(0));
	EXPECT_CALL(demuxer, on_au_found).Times(2).WillRepeatedly(Return(true));
	EXPECT_CALL(demuxer, on_au_queue_full);
	EXPECT_CALL(check_point, Call(1));
	EXPECT_CALL(demuxer, on_au_found).WillOnce(Return(true));
	EXPECT_CALL(demuxer, on_au_queue_full);

	std::array<u8, 0x800> small_au_queue_buffer{};

	demuxer.enable_es(0xe0, 0x00, true, small_au_queue_buffer, 0x400, false, 0);
	demuxer.set_stream(AVC_SINGLE_PACK_STREAM, false);
	check_point.Call(0);
	demuxer.process_next_pack();
	demuxer.set_stream(AVC_SINGLE_PACK_STREAM, false);
	demuxer.reset_es(0xe0, 0x00, small_au_queue_buffer.data() + 0x400);
	check_point.Call(1);
	demuxer.process_next_pack();
}

// Tests the raw elementary stream mode, which skips all MPEG program stream related parsing
// (this isn't actually available through the abstraction layer cellDmux/libdmux.sprx, but the related PPU functions are exported in libdmuxpamf.sprx)
TEST_F(DmuxPamfTest, RawEs)
{
	// Zero out MPEG-PS related stuff from the stream, so that only the elementary stream remains
	constexpr std::array<u8, 0x800> stream = []() consteval { auto pack = AVC_SINGLE_PACK_STREAM; std::fill_n(pack.begin(), 0x12a, 0); return pack; }();

	EXPECT_CALL(check_point, Call(0));
	EXPECT_CALL(demuxer, on_au_found(0xe0, 0x00, 0, Sha1Is(AVC_SINGLE_PACK_EXPECTED_AU_1.data_sha1), std::numeric_limits<u64>::max(), std::numeric_limits<u64>::max(), false, _, _)).WillOnce(Return(true));
	EXPECT_CALL(demuxer, on_au_found(0xe0, 0x00, 0, Sha1Is(AVC_SINGLE_PACK_EXPECTED_AU_2.data_sha1), std::numeric_limits<u64>::max(), std::numeric_limits<u64>::max(), false, _, _)).WillOnce(Return(true));

	demuxer.enable_es(0xe0, 0x00, true, au_queue_buffer, 0x400, true, 0);
	demuxer.set_stream(stream, false);

	// In raw elementary stream mode, the demuxer still processes the stream in chunks of 0x800 bytes.
	// The first call to process_next_pack() each chunk does nothing, it only sets the internal state to elementary stream parsing
	demuxer.process_next_pack();
	check_point.Call(0);
	demuxer.process_next_pack();
}

// If any of the event handlers return false (on LLE this happens if the event queue is full), the state of process_next_pack() should be saved and return immediately.
// The next time process_next_pack() is called, it should resume where it left off
TEST_F(DmuxPamfTest, EventHandlersReturnFalse)
{
	EXPECT_CALL(check_point, Call(0));
	EXPECT_CALL(demuxer, on_au_found(0xe0, 0x00, 0, Sha1Is(AVC_SINGLE_PACK_EXPECTED_AU_1.data_sha1), AVC_SINGLE_PACK_EXPECTED_AU_1.pts, AVC_SINGLE_PACK_EXPECTED_AU_1.dts, AVC_SINGLE_PACK_EXPECTED_AU_1.rap, _, _)).WillOnce(Return(false));
	EXPECT_CALL(check_point, Call(1));
	EXPECT_CALL(demuxer, on_au_found(0xe0, 0x00, 0, Sha1Is(AVC_SINGLE_PACK_EXPECTED_AU_1.data_sha1), AVC_SINGLE_PACK_EXPECTED_AU_1.pts, AVC_SINGLE_PACK_EXPECTED_AU_1.dts, AVC_SINGLE_PACK_EXPECTED_AU_1.rap, _, _)).WillOnce(Return(true));
	EXPECT_CALL(demuxer, on_au_found(0xe0, 0x00, 0, Sha1Is(AVC_SINGLE_PACK_EXPECTED_AU_2.data_sha1), AVC_SINGLE_PACK_EXPECTED_AU_2.pts, AVC_SINGLE_PACK_EXPECTED_AU_2.dts, AVC_SINGLE_PACK_EXPECTED_AU_2.rap, _, _)).WillOnce(Return(false));
	EXPECT_CALL(check_point, Call(2));
	EXPECT_CALL(demuxer, on_au_found(0xe0, 0x00, 0, Sha1Is(AVC_SINGLE_PACK_EXPECTED_AU_2.data_sha1), AVC_SINGLE_PACK_EXPECTED_AU_2.pts, AVC_SINGLE_PACK_EXPECTED_AU_2.dts, AVC_SINGLE_PACK_EXPECTED_AU_2.rap, _, _)).WillOnce(Return(true));

	EXPECT_CALL(check_point, Call(3));
	EXPECT_CALL(demuxer, on_demux_done).WillOnce(Return(false));
	EXPECT_CALL(check_point, Call(4));
	EXPECT_CALL(demuxer, on_demux_done).WillOnce(Return(true));

	demuxer.enable_es(0xe0, 0x00, true, au_queue_buffer, 0x800, false, 0);
	demuxer.set_stream(AVC_SINGLE_PACK_STREAM, false);
	check_point.Call(0);
	demuxer.process_next_pack();
	check_point.Call(1);
	demuxer.process_next_pack();
	check_point.Call(2);
	demuxer.process_next_pack();

	check_point.Call(3);
	demuxer.process_next_pack();
	check_point.Call(4);
	demuxer.process_next_pack();
	demuxer.process_next_pack();
	demuxer.process_next_pack();
}

// Tests if invalid stream ids are handled properly
TEST_F(DmuxPamfTest, InvalidStreamIds)
{
	const auto test = [&](u8 stream_id, u8 private_stream_id)
	{
		EXPECT_FALSE(demuxer.release_au(stream_id, private_stream_id, 0x800));
		EXPECT_FALSE(demuxer.disable_es(stream_id, private_stream_id));
		EXPECT_FALSE(demuxer.reset_es(stream_id, private_stream_id, nullptr));
		EXPECT_FALSE(demuxer.flush_es(stream_id, private_stream_id));
	};

	// Trying to use or disable already disabled elementary streams should result in failures
	for (u32 stream_id = 0xe0; stream_id < 0xf0; stream_id++)
	{
		test(stream_id, 0x00);
	}
	for (u32 private_stream_id = 0x00; private_stream_id < 0x10; private_stream_id++)
	{
		test(0xbd, private_stream_id);
	}
	for (u32 private_stream_id = 0x20; private_stream_id < 0x50; private_stream_id++)
	{
		test(0xbd, private_stream_id);
	}

	// Enabling all possible elementary streams
	for (u32 stream_id = 0xe0; stream_id < 0xf0; stream_id++)
	{
		EXPECT_TRUE(demuxer.enable_es(stream_id, 0x00, true, au_queue_buffer, 0x800, false, 0));
	}
	for (u32 private_stream_id = 0x00; private_stream_id < 0x10; private_stream_id++)
	{
		EXPECT_TRUE(demuxer.enable_es(0xbd, private_stream_id, true, au_queue_buffer, 0x800, false, 0));
	}
	for (u32 private_stream_id = 0x20; private_stream_id < 0x50; private_stream_id++)
	{
		EXPECT_TRUE(demuxer.enable_es(0xbd, private_stream_id, true, au_queue_buffer, 0x800, false, 0));
	}

	// Attempting to enable them again should result in failures
	for (u32 stream_id = 0xe0; stream_id < 0xf0; stream_id++)
	{
		EXPECT_FALSE(demuxer.enable_es(stream_id, 0x00, true, au_queue_buffer, 0x800, false, 0));
	}
	for (u32 private_stream_id = 0x00; private_stream_id < 0x10; private_stream_id++)
	{
		EXPECT_FALSE(demuxer.enable_es(0xbd, private_stream_id, true, au_queue_buffer, 0x800, false, 0));
	}
	for (u32 private_stream_id = 0x20; private_stream_id < 0x50; private_stream_id++)
	{
		EXPECT_FALSE(demuxer.enable_es(0xbd, private_stream_id, true, au_queue_buffer, 0x800, false, 0));
	}

	// Testing all possible invalid IDs
	for (u32 stream_id = 0; stream_id < 0xbd; stream_id++)
	{
		EXPECT_FALSE(demuxer.enable_es(stream_id, 0x00, true, au_queue_buffer, 0x800, false, 0));
		test(stream_id, 0x00);
	}
	for (u32 stream_id = 0xbe; stream_id < 0xe0; stream_id++)
	{
		EXPECT_FALSE(demuxer.enable_es(stream_id, 0x00, true, au_queue_buffer, 0x800, false, 0));
		test(stream_id, 0x00);
	}
	for (u32 stream_id = 0xf0; stream_id <= 0xff; stream_id++)
	{
		EXPECT_FALSE(demuxer.enable_es(stream_id, 0x00, true, au_queue_buffer, 0x800, false, 0));
		test(stream_id, 0x00);
	}
	for (u32 private_stream_id = 0x10; private_stream_id < 0x20; private_stream_id++)
	{
		EXPECT_FALSE(demuxer.enable_es(0xbd, private_stream_id, true, au_queue_buffer, 0x800, false, 0));
		test(0xbd, private_stream_id);
	}
	for (u32 private_stream_id = 0x50; private_stream_id <= 0xff; private_stream_id++)
	{
		EXPECT_FALSE(demuxer.enable_es(0xbd, private_stream_id, true, au_queue_buffer, 0x800, false, 0));
		test(0xbd, private_stream_id);
	}
}
