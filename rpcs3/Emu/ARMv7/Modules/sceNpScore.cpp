#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceNpScore;

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceNpScore, #name, name)

psv_log_base sceNpScore("SceNpScore", []()
{
	sceNpScore.on_load = nullptr;
	sceNpScore.on_unload = nullptr;
	sceNpScore.on_stop = nullptr;

	//REG_FUNC(0x0433069F, sceNpScoreInit);
	//REG_FUNC(0x2050F98F, sceNpScoreTerm);
	//REG_FUNC(0x5685F225, sceNpScoreCreateTitleCtx);
	//REG_FUNC(0xD30D1993, sceNpScoreCreateRequest);
	//REG_FUNC(0xF52EA88A, sceNpScoreDeleteTitleCtx);
	//REG_FUNC(0xFFF24BB1, sceNpScoreDeleteRequest);
	//REG_FUNC(0x320C0277, sceNpScoreRecordScore);
	//REG_FUNC(0x24B09634, sceNpScoreRecordScoreAsync);
	//REG_FUNC(0xC2862B67, sceNpScoreRecordGameData);
	//REG_FUNC(0x40573917, sceNpScoreRecordGameDataAsync);
	//REG_FUNC(0xDFAD64D3, sceNpScoreGetGameData);
	//REG_FUNC(0xCE416993, sceNpScoreGetGameDataAsync);
	//REG_FUNC(0x427D3412, sceNpScoreGetRankingByRange);
	//REG_FUNC(0xC45E3FCD, sceNpScoreGetRankingByRangeAsync);
	//REG_FUNC(0xBAE55B34, sceNpScoreGetRankingByNpId);
	//REG_FUNC(0x45CD1D00, sceNpScoreGetRankingByNpIdAsync);
	//REG_FUNC(0x871F28AA, sceNpScoreGetRankingByNpIdPcId);
	//REG_FUNC(0xCE3A9544, sceNpScoreGetRankingByNpIdPcIdAsync);
	//REG_FUNC(0xA7E93CE1, sceNpScoreAbortRequest);
	//REG_FUNC(0x31733BF3, sceNpScoreWaitAsync);
	//REG_FUNC(0x9F2A7AC9, sceNpScorePollAsync);
	//REG_FUNC(0x00F90E7B, sceNpScoreGetBoardInfo);
	//REG_FUNC(0x3CD9974E, sceNpScoreGetBoardInfoAsync);
	//REG_FUNC(0xA0C94D46, sceNpScoreCensorComment);
	//REG_FUNC(0xAA0BBF8E, sceNpScoreCensorCommentAsync);
	//REG_FUNC(0x6FD2041A, sceNpScoreSanitizeComment);
	//REG_FUNC(0x15981858, sceNpScoreSanitizeCommentAsync);
	//REG_FUNC(0x5EF44841, sceNpScoreSetTimeout);
	//REG_FUNC(0x53D77883, sceNpScoreSetPlayerCharacterId);
});
