#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/SysCalls/Modules.h"

#include "sceNpCommerce2.h"

extern Module sceNpCommerce2;

struct sceNpCommerce2Internal
{
	bool m_bSceNpCommerce2Initialized;

	sceNpCommerce2Internal()
		: m_bSceNpCommerce2Initialized(false)
	{
	}
};

sceNpCommerce2Internal sceNpCommerce2Instance;

int sceNpCommerce2ExecuteStoreBrowse()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2GetStoreBrowseUserdata()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2Init()
{
	sceNpCommerce2.Warning("sceNpCommerce2Init()");

	if (sceNpCommerce2Instance.m_bSceNpCommerce2Initialized)
		return SCE_NP_COMMERCE2_ERROR_ALREADY_INITIALIZED;

	sceNpCommerce2Instance.m_bSceNpCommerce2Initialized = true;

	return CELL_OK;
}

int sceNpCommerce2Term()
{
	sceNpCommerce2.Warning("sceNpCommerce2Term()");

	if (!sceNpCommerce2Instance.m_bSceNpCommerce2Initialized)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	sceNpCommerce2Instance.m_bSceNpCommerce2Initialized = false;

	return CELL_OK;
}

int sceNpCommerce2CreateCtx()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2DestroyCtx()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2CreateSessionStart()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2CreateSessionAbort()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2CreateSessionFinish()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2GetCategoryContentsCreateReq()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2GetCategoryContentsStart()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2GetCategoryContentsGetResult()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2InitGetCategoryContentsResult()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2GetCategoryInfo()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2GetContentInfo()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2GetCategoryInfoFromContentInfo()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2GetGameProductInfoFromContentInfo()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2DestroyGetCategoryContentsResult()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2GetProductInfoCreateReq()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2GetProductInfoStart()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2GetProductInfoGetResult()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2InitGetProductInfoResult()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2GetGameProductInfo()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2DestroyGetProductInfoResult()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2GetProductInfoListCreateReq()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2GetProductInfoListStart()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2GetProductInfoListGetResult()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2InitGetProductInfoListResult()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2GetGameProductInfoFromGetProductInfoListResult()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2DestroyGetProductInfoListResult()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2GetContentRatingInfoFromGameProductInfo()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2GetContentRatingInfoFromCategoryInfo()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2GetContentRatingDescriptor()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2GetGameSkuInfoFromGameProductInfo()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2GetPrice()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2DoCheckoutStartAsync()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2DoCheckoutFinishAsync()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2DoProductBrowseStartAsync()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2DoProductBrowseFinishAsync()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2DoDlListStartAsync()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2DoDlListFinishAsync()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2DoProductCodeStartAsync()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2DoProductCodeFinishAsync()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2GetBGDLAvailability()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2SetBGDLAvailability()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2AbortReq()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

int sceNpCommerce2DestroyReq()
{
	UNIMPLEMENTED_FUNC(sceNpCommerce2);
	return CELL_OK;
}

Module sceNpCommerce2("sceNpCommerce2", []()
{
	sceNpCommerce2Instance.m_bSceNpCommerce2Initialized = false;

	REG_FUNC(sceNpCommerce2, sceNpCommerce2ExecuteStoreBrowse);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2GetStoreBrowseUserdata);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2Init);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2Term);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2CreateCtx);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2DestroyCtx);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2CreateSessionStart);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2CreateSessionAbort);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2CreateSessionFinish);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2GetCategoryContentsCreateReq);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2GetCategoryContentsStart);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2GetCategoryContentsGetResult);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2InitGetCategoryContentsResult);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2GetCategoryInfo);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2GetContentInfo);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2GetCategoryInfoFromContentInfo);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2GetGameProductInfoFromContentInfo);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2DestroyGetCategoryContentsResult);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2GetProductInfoCreateReq);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2GetProductInfoStart);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2GetProductInfoGetResult);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2InitGetProductInfoResult);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2GetGameProductInfo);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2DestroyGetProductInfoResult);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2GetProductInfoListCreateReq);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2GetProductInfoListStart);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2GetProductInfoListGetResult);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2InitGetProductInfoListResult);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2GetGameProductInfoFromGetProductInfoListResult);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2DestroyGetProductInfoListResult);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2GetContentRatingInfoFromGameProductInfo);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2GetContentRatingInfoFromCategoryInfo);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2GetContentRatingDescriptor);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2GetGameSkuInfoFromGameProductInfo);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2GetPrice);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2DoCheckoutStartAsync);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2DoCheckoutFinishAsync);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2DoProductBrowseStartAsync);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2DoProductBrowseFinishAsync);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2DoDlListStartAsync);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2DoDlListFinishAsync);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2DoProductCodeStartAsync);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2DoProductCodeFinishAsync);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2GetBGDLAvailability);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2SetBGDLAvailability);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2AbortReq);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2DestroyReq);
});
