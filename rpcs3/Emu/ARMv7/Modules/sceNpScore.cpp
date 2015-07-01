#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceNpScore.h"

s32 sceNpScoreInit(s32 threadPriority, s32 cpuAffinityMask, vm::ptr<void> option)
{
	throw EXCEPTION("");
}

s32 sceNpScoreTerm(ARMv7Context&)
{
	throw EXCEPTION("");
}

s32 sceNpScoreCreateTitleCtx(vm::cptr<SceNpCommunicationId> titleId, vm::cptr<SceNpCommunicationPassphrase> passphrase, vm::cptr<SceNpId> selfNpId)
{
	throw EXCEPTION("");
}

s32 sceNpScoreDeleteTitleCtx(s32 titleCtxId)
{
	throw EXCEPTION("");
}

s32 sceNpScoreCreateRequest(s32 titleCtxId)
{
	throw EXCEPTION("");
}

s32 sceNpScoreDeleteRequest(s32 reqId)
{
	throw EXCEPTION("");
}

s32 sceNpScoreAbortRequest(s32 reqId)
{
	throw EXCEPTION("");
}

s32 sceNpScoreSetTimeout(s32 id, s32 resolveRetry, s32 resolveTimeout, s32 connTimeout, s32 sendTimeout, s32 recvTimeout)
{
	throw EXCEPTION("");
}

s32 sceNpScoreSetPlayerCharacterId(s32 id, s32 pcId)
{
	throw EXCEPTION("");
}

s32 sceNpScoreGetBoardInfo(s32 reqId, u32 boardId, vm::ptr<SceNpScoreBoardInfo> boardInfo, vm::ptr<void> option)
{
	throw EXCEPTION("");
}

s32 sceNpScoreRecordScore(
	s32 reqId,
	u32 boardId,
	s64 score,
	vm::cptr<SceNpScoreComment> scoreComment,
	vm::cptr<SceNpScoreGameInfo> gameInfo,
	vm::ptr<u32> tmpRank,
	vm::cptr<u64> compareDate,
	vm::ptr<void> option)
{
	throw EXCEPTION("");
}

s32 sceNpScoreRecordGameData(
	s32 reqId,
	u32 boardId,
	s64 score,
	u32 totalSize,
	u32 sendSize,
	vm::cptr<void> data,
	vm::ptr<void> option)
{
	throw EXCEPTION("");
}

s32 sceNpScoreGetGameData(
	s32 reqId,
	u32 boardId,
	vm::cptr<SceNpId> npId,
	vm::ptr<u32> totalSize,
	u32 recvSize,
	vm::ptr<void> data,
	vm::ptr<void> option)
{
	throw EXCEPTION("");
}

s32 sceNpScoreGetRankingByNpId(
	s32 reqId,
	u32 boardId,
	vm::cptr<SceNpId> npIdArray,
	u32 npIdArraySize,
	vm::ptr<SceNpScorePlayerRankData> rankArray,
	u32 rankArraySize,
	vm::ptr<SceNpScoreComment> commentArray,
	u32 commentArraySize,
	vm::ptr<SceNpScoreGameInfo> infoArray,
	u32 infoArraySize,
	u32 arrayNum,
	vm::ptr<u64> lastSortDate,
	vm::ptr<u32> totalRecord,
	vm::ptr<void> option)
{
	throw EXCEPTION("");
}

s32 sceNpScoreGetRankingByRange(
	s32 reqId,
	u32 boardId,
	u32 startSerialRank,
	vm::ptr<SceNpScoreRankData> rankArray,
	u32 rankArraySize,
	vm::ptr<SceNpScoreComment> commentArray,
	u32 commentArraySize,
	vm::ptr<SceNpScoreGameInfo> infoArray,
	u32 infoArraySize,
	u32 arrayNum,
	vm::ptr<u64> lastSortDate,
	vm::ptr<u32> totalRecord,
	vm::ptr<void> option)
{
	throw EXCEPTION("");
}


s32 sceNpScoreGetRankingByNpIdPcId(
	s32 reqId,
	u32 boardId,
	vm::cptr<SceNpScoreNpIdPcId> idArray,
	u32 idArraySize,
	vm::ptr<SceNpScorePlayerRankData> rankArray,
	u32 rankArraySize,
	vm::ptr<SceNpScoreComment> commentArray,
	u32 commentArraySize,
	vm::ptr<SceNpScoreGameInfo> infoArray,
	u32 infoArraySize,
	u32 arrayNum,
	vm::ptr<u64> lastSortDate,
	vm::ptr<u32> totalRecord,
	vm::ptr<void> option)
{
	throw EXCEPTION("");
}

s32 sceNpScoreCensorComment(s32 reqId, vm::cptr<char> comment, vm::ptr<void> option)
{
	throw EXCEPTION("");
}

s32 sceNpScoreSanitizeComment(s32 reqId, vm::cptr<char> comment, vm::ptr<char> sanitizedComment, vm::ptr<void> option)
{
	throw EXCEPTION("");
}

s32 sceNpScoreWaitAsync(s32 id, vm::ptr<s32> result)
{
	throw EXCEPTION("");
}

s32 sceNpScorePollAsync(s32 reqId, vm::ptr<s32> result)
{
	throw EXCEPTION("");
}

s32 sceNpScoreGetBoardInfoAsync(s32 reqId, u32 boardId, vm::ptr<SceNpScoreBoardInfo> boardInfo, vm::ptr<void> option)
{
	throw EXCEPTION("");
}

s32 sceNpScoreRecordScoreAsync(
	s32 reqId,
	u32 boardId,
	s64 score,
	vm::cptr<SceNpScoreComment> scoreComment,
	vm::cptr<SceNpScoreGameInfo> gameInfo,
	vm::ptr<u32> tmpRank,
	vm::cptr<u64> compareDate,
	vm::ptr<void> option)
{
	throw EXCEPTION("");
}

s32 sceNpScoreRecordGameDataAsync(
	s32 reqId,
	u32 boardId,
	s64 score,
	u32 totalSize,
	u32 sendSize,
	vm::cptr<void> data,
	vm::ptr<void> option)
{
	throw EXCEPTION("");
}

s32 sceNpScoreGetGameDataAsync(
	s32 reqId,
	u32 boardId,
	vm::cptr<SceNpId> npId,
	vm::ptr<u32> totalSize,
	u32 recvSize,
	vm::ptr<void> data,
	vm::ptr<void> option)
{
	throw EXCEPTION("");
}

s32 sceNpScoreGetRankingByNpIdAsync(
	s32 reqId,
	u32 boardId,
	vm::cptr<SceNpId> npIdArray,
	u32 npIdArraySize,
	vm::ptr<SceNpScorePlayerRankData> rankArray,
	u32 rankArraySize,
	vm::ptr<SceNpScoreComment> commentArray,
	u32 commentArraySize,
	vm::ptr<SceNpScoreGameInfo> infoArray,
	u32 infoArraySize,
	u32 arrayNum,
	vm::ptr<u64> lastSortDate,
	vm::ptr<u32> totalRecord,
	vm::ptr<void> option)
{
	throw EXCEPTION("");
}

s32 sceNpScoreGetRankingByRangeAsync(
	s32 reqId,
	u32 boardId,
	u32 startSerialRank,
	vm::ptr<SceNpScoreRankData> rankArray,
	u32 rankArraySize,
	vm::ptr<SceNpScoreComment> commentArray,
	u32 commentArraySize,
	vm::ptr<SceNpScoreGameInfo> infoArray,
	u32 infoArraySize,
	u32 arrayNum,
	vm::ptr<u64> lastSortDate,
	vm::ptr<u32> totalRecord,
	vm::ptr<void> option)
{
	throw EXCEPTION("");
}

s32 sceNpScoreGetRankingByNpIdPcIdAsync(
	s32 reqId,
	u32 boardId,
	vm::cptr<SceNpScoreNpIdPcId> idArray,
	u32 idArraySize,
	vm::ptr<SceNpScorePlayerRankData> rankArray,
	u32 rankArraySize,
	vm::ptr<SceNpScoreComment> commentArray,
	u32 commentArraySize,
	vm::ptr<SceNpScoreGameInfo> infoArray,
	u32 infoArraySize,
	u32 arrayNum,
	vm::ptr<u64> lastSortDate,
	vm::ptr<u32> totalRecord,
	vm::ptr<void> option)
{
	throw EXCEPTION("");
}

s32 sceNpScoreCensorCommentAsync(s32 reqId, vm::cptr<char> comment, vm::ptr<void> option)
{
	throw EXCEPTION("");
}

s32 sceNpScoreSanitizeCommentAsync(s32 reqId, vm::cptr<char> comment, vm::ptr<char> sanitizedComment, vm::ptr<void> option)
{
	throw EXCEPTION("");
}

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceNpScore, #name, name)

psv_log_base sceNpScore("SceNpScore", []()
{
	sceNpScore.on_load = nullptr;
	sceNpScore.on_unload = nullptr;
	sceNpScore.on_stop = nullptr;
	sceNpScore.on_error = nullptr;

	REG_FUNC(0x0433069F, sceNpScoreInit);
	REG_FUNC(0x2050F98F, sceNpScoreTerm);
	REG_FUNC(0x5685F225, sceNpScoreCreateTitleCtx);
	REG_FUNC(0xD30D1993, sceNpScoreCreateRequest);
	REG_FUNC(0xF52EA88A, sceNpScoreDeleteTitleCtx);
	REG_FUNC(0xFFF24BB1, sceNpScoreDeleteRequest);
	REG_FUNC(0x320C0277, sceNpScoreRecordScore);
	REG_FUNC(0x24B09634, sceNpScoreRecordScoreAsync);
	REG_FUNC(0xC2862B67, sceNpScoreRecordGameData);
	REG_FUNC(0x40573917, sceNpScoreRecordGameDataAsync);
	REG_FUNC(0xDFAD64D3, sceNpScoreGetGameData);
	REG_FUNC(0xCE416993, sceNpScoreGetGameDataAsync);
	REG_FUNC(0x427D3412, sceNpScoreGetRankingByRange);
	REG_FUNC(0xC45E3FCD, sceNpScoreGetRankingByRangeAsync);
	REG_FUNC(0xBAE55B34, sceNpScoreGetRankingByNpId);
	REG_FUNC(0x45CD1D00, sceNpScoreGetRankingByNpIdAsync);
	REG_FUNC(0x871F28AA, sceNpScoreGetRankingByNpIdPcId);
	REG_FUNC(0xCE3A9544, sceNpScoreGetRankingByNpIdPcIdAsync);
	REG_FUNC(0xA7E93CE1, sceNpScoreAbortRequest);
	REG_FUNC(0x31733BF3, sceNpScoreWaitAsync);
	REG_FUNC(0x9F2A7AC9, sceNpScorePollAsync);
	REG_FUNC(0x00F90E7B, sceNpScoreGetBoardInfo);
	REG_FUNC(0x3CD9974E, sceNpScoreGetBoardInfoAsync);
	REG_FUNC(0xA0C94D46, sceNpScoreCensorComment);
	REG_FUNC(0xAA0BBF8E, sceNpScoreCensorCommentAsync);
	REG_FUNC(0x6FD2041A, sceNpScoreSanitizeComment);
	REG_FUNC(0x15981858, sceNpScoreSanitizeCommentAsync);
	REG_FUNC(0x5EF44841, sceNpScoreSetTimeout);
	REG_FUNC(0x53D77883, sceNpScoreSetPlayerCharacterId);
});
