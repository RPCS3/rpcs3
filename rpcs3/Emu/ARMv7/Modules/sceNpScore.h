#pragma once

#include "sceNpCommon.h"

struct SceNpScoreGameInfo
{
	le_t<u32> infoSize;
	u8 pad[4];
	u8 data[192];
};

struct SceNpScoreComment
{
	char utf8Comment[64];
};

struct SceNpScoreRankData
{
	SceNpId npId;
	u8 reserved[49];
	u8 pad0[3];
	le_t<s32> pcId;
	le_t<u32> serialRank;
	le_t<u32> rank;
	le_t<u32> highestRank;
	le_t<s32> hasGameData;
	u8 pad1[4];
	le_t<s64> scoreValue;
	le_t<u64> recordDate;
};

struct SceNpScorePlayerRankData
{
	le_t<s32> hasData;
	u8 pad0[4];
	SceNpScoreRankData rankData;
};

struct SceNpScoreBoardInfo
{
	le_t<u32> rankLimit;
	le_t<u32> updateMode;
	le_t<u32> sortMode;
	le_t<u32> uploadNumLimit;
	le_t<u32> uploadSizeLimit;
};

struct SceNpScoreNpIdPcId
{
	SceNpId npId;
	le_t<s32> pcId;
	u8 pad[4];
};

extern psv_log_base sceNpScore;
