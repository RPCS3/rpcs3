#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "sceNpCommerce2.h"
#include "sceNp.h"

LOG_CHANNEL(sceNpCommerce2);

template <>
void fmt_class_string<SceNpCommerce2Error>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](SceNpCommerce2Error value)
	{
		switch (value)
		{
		STR_CASE(SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED);
		STR_CASE(SCE_NP_COMMERCE2_ERROR_ALREADY_INITIALIZED);
		STR_CASE(SCE_NP_COMMERCE2_ERROR_INVALID_ARGUMENT);
		STR_CASE(SCE_NP_COMMERCE2_ERROR_UNSUPPORTED_VERSION);
		STR_CASE(SCE_NP_COMMERCE2_ERROR_CTX_MAX);
		STR_CASE(SCE_NP_COMMERCE2_ERROR_INVALID_INDEX);
		STR_CASE(SCE_NP_COMMERCE2_ERROR_INVALID_SKUID);
		STR_CASE(SCE_NP_COMMERCE2_ERROR_INVALID_SKU_NUM);
		STR_CASE(SCE_NP_COMMERCE2_ERROR_INVALID_MEMORY_CONTAINER);
		STR_CASE(SCE_NP_COMMERCE2_ERROR_INSUFFICIENT_MEMORY_CONTAINER);
		STR_CASE(SCE_NP_COMMERCE2_ERROR_OUT_OF_MEMORY);
		STR_CASE(SCE_NP_COMMERCE2_ERROR_CTX_NOT_FOUND);
		STR_CASE(SCE_NP_COMMERCE2_ERROR_CTXID_NOT_AVAILABLE);
		STR_CASE(SCE_NP_COMMERCE2_ERROR_REQ_NOT_FOUND);
		STR_CASE(SCE_NP_COMMERCE2_ERROR_REQID_NOT_AVAILABLE);
		STR_CASE(SCE_NP_COMMERCE2_ERROR_ABORTED);
		STR_CASE(SCE_NP_COMMERCE2_ERROR_RESPONSE_BUF_TOO_SMALL);
		STR_CASE(SCE_NP_COMMERCE2_ERROR_COULD_NOT_RECV_WHOLE_RESPONSE_DATA);
		STR_CASE(SCE_NP_COMMERCE2_ERROR_INVALID_RESULT_DATA);
		STR_CASE(SCE_NP_COMMERCE2_ERROR_UNKNOWN);
		STR_CASE(SCE_NP_COMMERCE2_ERROR_SERVER_MAINTENANCE);
		STR_CASE(SCE_NP_COMMERCE2_ERROR_SERVER_UNKNOWN);
		STR_CASE(SCE_NP_COMMERCE2_ERROR_INSUFFICIENT_BUF_SIZE);
		STR_CASE(SCE_NP_COMMERCE2_ERROR_REQ_MAX);
		STR_CASE(SCE_NP_COMMERCE2_ERROR_INVALID_TARGET_TYPE);
		STR_CASE(SCE_NP_COMMERCE2_ERROR_INVALID_TARGET_ID);
		STR_CASE(SCE_NP_COMMERCE2_ERROR_INVALID_SIZE);
		STR_CASE(SCE_NP_COMMERCE2_ERROR_DATA_NOT_FOUND);
		STR_CASE(SCE_NP_COMMERCE2_SERVER_ERROR_BAD_REQUEST);
		STR_CASE(SCE_NP_COMMERCE2_SERVER_ERROR_UNKNOWN_ERROR);
		STR_CASE(SCE_NP_COMMERCE2_SERVER_ERROR_SESSION_EXPIRED);
		STR_CASE(SCE_NP_COMMERCE2_SERVER_ERROR_ACCESS_PERMISSION_DENIED);
		STR_CASE(SCE_NP_COMMERCE2_SERVER_ERROR_NO_SUCH_CATEGORY);
		STR_CASE(SCE_NP_COMMERCE2_SERVER_ERROR_NO_SUCH_PRODUCT);
		STR_CASE(SCE_NP_COMMERCE2_SERVER_ERROR_NOT_ELIGIBILITY);
		STR_CASE(SCE_NP_COMMERCE2_SERVER_ERROR_INVALID_SKU);
		STR_CASE(SCE_NP_COMMERCE2_SERVER_ERROR_ACCOUNT_SUSPENDED1);
		STR_CASE(SCE_NP_COMMERCE2_SERVER_ERROR_ACCOUNT_SUSPENDED2);
		STR_CASE(SCE_NP_COMMERCE2_SERVER_ERROR_OVER_SPENDING_LIMIT);
		STR_CASE(SCE_NP_COMMERCE2_SERVER_ERROR_INVALID_VOUCHER);
		STR_CASE(SCE_NP_COMMERCE2_SERVER_ERROR_VOUCHER_ALREADY_CONSUMED);
		STR_CASE(SCE_NP_COMMERCE2_SERVER_ERROR_EXCEEDS_AGE_LIMIT_IN_BROWSING);
		STR_CASE(SCE_NP_COMMERCE2_SYSTEM_UTIL_ERROR_INVALID_VOUCHER);
		STR_CASE(SCE_NP_COMMERCE_ERROR_REQ_BUSY);
		}

		return unknown;
	});
}

error_code sceNpCommerce2ExecuteStoreBrowse(s32 targetType, vm::cptr<char> targetId, s32 userdata)
{
	sceNpCommerce2.todo("sceNpCommerce2ExecuteStoreBrowse(targetType=%d, targetId=%s, userdata=%d)", targetType, targetId, userdata);
	return CELL_OK;
}

error_code sceNpCommerce2GetStoreBrowseUserdata(vm::ptr<s32> userdata)
{
	sceNpCommerce2.todo("sceNpCommerce2GetStoreBrowseUserdata(userdata=*0x%x)", userdata);
	return CELL_OK;
}

error_code sceNpCommerce2Init()
{
	sceNpCommerce2.warning("sceNpCommerce2Init()");

	return CELL_OK;
}

error_code sceNpCommerce2Term()
{
	sceNpCommerce2.warning("sceNpCommerce2Term()");

	return CELL_OK;
}

error_code sceNpCommerce2CreateCtx(u32 version, vm::cptr<SceNpId> npId, vm::ptr<SceNpCommerce2Handler> handler, vm::ptr<void> arg, vm::ptr<u32> ctx_id)
{
	sceNpCommerce2.todo("sceNpCommerce2ExecuteStoreBrowse(version=%d, npId=*0x%x, handler=*0x%x, arg=*0x%x, ctx_id=*0x%x)", version, npId, handler, arg, ctx_id);
	return CELL_OK;
}

error_code sceNpCommerce2DestroyCtx(u32 ctx_id)
{
	sceNpCommerce2.todo("sceNpCommerce2DestroyCtx(ctx_id=%d)", ctx_id);
	return CELL_OK;
}

error_code sceNpCommerce2EmptyStoreCheckStart(u32 ctx_id, s32 store_check_type, vm::cptr<char> target_id)
{
	sceNpCommerce2.todo("sceNpCommerce2EmptyStoreCheckStart(ctx_id=%d, store_check_type=%d, target_id=%s)", ctx_id, store_check_type, target_id);
	return CELL_OK;
}

error_code sceNpCommerce2EmptyStoreCheckAbort(u32 ctx_id)
{
	sceNpCommerce2.todo("sceNpCommerce2EmptyStoreCheckAbort(ctx_id=%d)", ctx_id);
	return CELL_OK;
}

error_code sceNpCommerce2EmptyStoreCheckFinish(u32 ctx_id, vm::ptr<int> is_empty)
{
	sceNpCommerce2.todo("sceNpCommerce2EmptyStoreCheckFinish(ctx_id=%d, is_empty=*0x%x)", ctx_id, is_empty);
	return CELL_OK;
}

error_code sceNpCommerce2CreateSessionStart(u32 ctx_id)
{
	sceNpCommerce2.todo("sceNpCommerce2CreateSessionStart(ctx_id=%d)", ctx_id);
	return CELL_OK;
}

error_code sceNpCommerce2CreateSessionAbort(u32 ctx_id)
{
	sceNpCommerce2.todo("sceNpCommerce2CreateSessionAbort(ctx_id=%d)", ctx_id);
	return CELL_OK;
}

error_code sceNpCommerce2CreateSessionFinish(u32 ctx_id, vm::ptr<SceNpCommerce2SessionInfo> sessionInfo)
{
	sceNpCommerce2.todo("sceNpCommerce2CreateSessionFinish(ctx_id=%d, sessionInfo=*0x%x)", ctx_id, sessionInfo);
	return CELL_OK;
}

error_code sceNpCommerce2GetCategoryContentsCreateReq(u32 ctx_id, vm::ptr<u32> req_id)
{
	sceNpCommerce2.todo("sceNpCommerce2GetCategoryContentsCreateReq(ctx_id=%d, req_id=*0x%x)", ctx_id, req_id);
	return CELL_OK;
}

error_code sceNpCommerce2GetCategoryContentsStart(u32 req_id, vm::cptr<char> categoryId, u32 startPosition, u32 maxCountOfResults)
{
	sceNpCommerce2.todo("sceNpCommerce2GetCategoryContentsStart(req_id=%d, categoryId=%s, startPosition=%d, maxCountOfResults=%d)", req_id, categoryId, startPosition, maxCountOfResults);
	return CELL_OK;
}

error_code sceNpCommerce2GetCategoryContentsGetResult(u32 req_id, vm::ptr<void> buf, u64 buf_size, vm::ptr<u64> fill_size) // TODO: correct size types?
{
	sceNpCommerce2.todo("sceNpCommerce2GetCategoryContentsGetResult(req_id=%d, buf=*0x%x, buf_size=%d, fill_size=*0x%x)", req_id, buf, buf_size, fill_size);
	return CELL_OK;
}

error_code sceNpCommerce2InitGetCategoryContentsResult(vm::ptr<SceNpCommerce2GetCategoryContentsResult> result, vm::ptr<void> data, u64 data_size) // TODO: correct size types?
{
	sceNpCommerce2.todo("sceNpCommerce2InitGetCategoryContentsResult(result=*0x%x, data=*0x%x, data_size=%d)", result, data, data_size);
	return CELL_OK;
}

error_code sceNpCommerce2GetCategoryInfo(vm::cptr<SceNpCommerce2GetCategoryContentsResult> result, vm::ptr<SceNpCommerce2CategoryInfo> categoryInfo)
{
	sceNpCommerce2.todo("sceNpCommerce2GetCategoryInfo(result=*0x%x, categoryInfo=*0x%x)", result, categoryInfo);
	return CELL_OK;
}

error_code sceNpCommerce2GetContentInfo(vm::cptr<SceNpCommerce2GetCategoryContentsResult> result, u32 index, vm::ptr<SceNpCommerce2ContentInfo> contentInfo)
{
	sceNpCommerce2.todo("sceNpCommerce2GetContentInfo(result=*0x%x, index=%d, contentInfo=*0x%x)", result, index, contentInfo);
	return CELL_OK;
}

error_code sceNpCommerce2GetCategoryInfoFromContentInfo(vm::cptr<SceNpCommerce2ContentInfo> contentInfo, vm::ptr<SceNpCommerce2CategoryInfo> categoryInfo)
{
	sceNpCommerce2.todo("sceNpCommerce2GetCategoryInfoFromContentInfo(contentInfo=*0x%x, categoryInfo=*0x%x)", contentInfo, categoryInfo);
	return CELL_OK;
}

error_code sceNpCommerce2GetGameProductInfoFromContentInfo(vm::cptr<SceNpCommerce2ContentInfo> contentInfo, vm::ptr<SceNpCommerce2GameProductInfo> gameProductInfo)
{
	sceNpCommerce2.todo("sceNpCommerce2GetGameProductInfoFromContentInfo(contentInfo=*0x%x, gameProductInfo=*0x%x)", contentInfo, gameProductInfo);
	return CELL_OK;
}

error_code sceNpCommerce2DestroyGetCategoryContentsResult(vm::ptr<SceNpCommerce2GetCategoryContentsResult> result)
{
	sceNpCommerce2.todo("sceNpCommerce2DestroyGetCategoryContentsResult(result=*0x%x)", result);
	return CELL_OK;
}

error_code sceNpCommerce2GetProductInfoCreateReq(u32 ctx_id, vm::ptr<u32> req_id)
{
	sceNpCommerce2.todo("sceNpCommerce2GetProductInfoCreateReq(ctx_id=%d, req_id=*0x%x)", ctx_id, req_id);
	return CELL_OK;
}

error_code sceNpCommerce2GetProductInfoStart(u32 req_id, vm::cptr<char> categoryId, vm::cptr<char> productId)
{
	sceNpCommerce2.todo("sceNpCommerce2GetProductInfoStart(req_id=%d, categoryId=%s, productId=%s)", req_id, categoryId, productId);
	return CELL_OK;
}

error_code sceNpCommerce2GetProductInfoGetResult(u32 req_id, vm::ptr<void> buf, u64 buf_size, vm::ptr<u64> fill_size) // TODO: correct size types?
{
	sceNpCommerce2.todo("sceNpCommerce2GetProductInfoGetResult(req_id=%d, buf=*0x%x, buf_size=%d, fill_size=*0x%x)", req_id, buf, buf_size, fill_size);
	return CELL_OK;
}

error_code sceNpCommerce2InitGetProductInfoResult(vm::ptr<SceNpCommerce2GetProductInfoResult> result, vm::ptr<void> data, u64 data_size) // TODO: correct size types?
{
	sceNpCommerce2.todo("sceNpCommerce2InitGetProductInfoResult(result=*0x%x, data=*0x%x, data_size=%d)", result, data, data_size);
	return CELL_OK;
}

error_code sceNpCommerce2GetGameProductInfo(vm::cptr<SceNpCommerce2GetProductInfoResult> result, vm::ptr<SceNpCommerce2GameProductInfo> gameProductInfo)
{
	sceNpCommerce2.todo("sceNpCommerce2GetGameProductInfo(result=*0x%x, gameProductInfo=*0x%x)", result, gameProductInfo);
	return CELL_OK;
}

error_code sceNpCommerce2DestroyGetProductInfoResult(vm::ptr<SceNpCommerce2GetProductInfoResult> result)
{
	sceNpCommerce2.todo("sceNpCommerce2DestroyGetProductInfoResult(result=*0x%x)", result);
	return CELL_OK;
}

error_code sceNpCommerce2GetProductInfoListCreateReq(u32 ctx_id, vm::ptr<u32> req_id)
{
	sceNpCommerce2.todo("sceNpCommerce2GetProductInfoListCreateReq(ctx_id=%d, req_id=*0x%x)", ctx_id, req_id);
	return CELL_OK;
}

error_code sceNpCommerce2GetProductInfoListStart(u32 req_id, vm::cptr<char[]> productIds, u32 productNum)
{
	sceNpCommerce2.todo("sceNpCommerce2GetProductInfoListStart(req_id=%d, productIds=*0x%x, productNum=%d)", req_id, productIds, productNum);
	return CELL_OK;
}

error_code sceNpCommerce2GetProductInfoListGetResult(u32 req_id, vm::ptr<void> buf, u64 buf_size, vm::ptr<u64> fill_size) // TODO: correct size types?
{
	sceNpCommerce2.todo("sceNpCommerce2GetProductInfoListGetResult(req_id=%d, buf=*0x%x, buf_size=%d, fill_size=*0x%x)", req_id, buf, buf_size, fill_size);
	return CELL_OK;
}

error_code sceNpCommerce2InitGetProductInfoListResult(vm::ptr<SceNpCommerce2GetProductInfoListResult> result, vm::ptr<void> data, u64 data_size) // TODO: correct size types?
{
	sceNpCommerce2.todo("sceNpCommerce2InitGetProductInfoListResult(result=*0x%x, data=*0x%x, data_size=%d)", result, data, data_size);
	return CELL_OK;
}

error_code sceNpCommerce2GetGameProductInfoFromGetProductInfoListResult(vm::cptr<SceNpCommerce2GetProductInfoListResult> result, u32 index, vm::ptr<SceNpCommerce2GameProductInfo> gameProductInfo)
{
	sceNpCommerce2.todo("sceNpCommerce2GetGameProductInfoFromGetProductInfoListResult(result=*0x%x, index=%d, data=*0x%x)", result, index, gameProductInfo);
	return CELL_OK;
}

error_code sceNpCommerce2DestroyGetProductInfoListResult(vm::ptr<SceNpCommerce2GetProductInfoListResult> result)
{
	sceNpCommerce2.todo("sceNpCommerce2DestroyGetProductInfoListResult(result=*0x%x)", result);
	return CELL_OK;
}

error_code sceNpCommerce2GetContentRatingInfoFromGameProductInfo(vm::cptr<SceNpCommerce2GameProductInfo> gameProductInfo, vm::ptr<SceNpCommerce2ContentRatingInfo> contentRatingInfo)
{
	sceNpCommerce2.todo("sceNpCommerce2GetContentRatingInfoFromGameProductInfo(gameProductInfo=*0x%x, contentRatingInfo=*0x%x)", gameProductInfo, contentRatingInfo);
	return CELL_OK;
}

error_code sceNpCommerce2GetContentRatingInfoFromCategoryInfo(vm::cptr<SceNpCommerce2CategoryInfo> categoryInfo, vm::ptr<SceNpCommerce2ContentRatingInfo> contentRatingInfo)
{
	sceNpCommerce2.todo("sceNpCommerce2GetContentRatingInfoFromCategoryInfo(categoryInfo=*0x%x, contentRatingInfo=*0x%x)", categoryInfo, contentRatingInfo);
	return CELL_OK;
}

error_code sceNpCommerce2GetContentRatingDescriptor(vm::cptr<SceNpCommerce2ContentRatingInfo> contentRatingInfo, u32 index, vm::ptr<SceNpCommerce2ContentRatingDescriptor> contentRatingDescriptor)
{
	sceNpCommerce2.todo("sceNpCommerce2GetContentRatingDescriptor(contentRatingInfo=*0x%x, index=%d, contentRatingDescriptor=*0x%x)", contentRatingInfo, index, contentRatingDescriptor);
	return CELL_OK;
}

error_code sceNpCommerce2GetGameSkuInfoFromGameProductInfo(vm::cptr<SceNpCommerce2GameProductInfo> gameProductInfo, u32 index, vm::ptr<SceNpCommerce2GameSkuInfo> gameSkuInfo)
{
	sceNpCommerce2.todo("sceNpCommerce2GetGameSkuInfoFromGameProductInfo(gameProductInfo=*0x%x, index=%d, gameSkuInfo=*0x%x)", gameProductInfo, index, gameSkuInfo);
	return CELL_OK;
}

error_code sceNpCommerce2GetPrice(u32 ctx_id, vm::ptr<char> buf, u64 buflen, u32 price) // TODO: correct size types?
{
	sceNpCommerce2.todo("sceNpCommerce2GetPrice(ctx_id=%d, buf=*0x%x, buflen=%d, price=%d)", ctx_id, buf, buflen, price);
	return CELL_OK;
}

error_code sceNpCommerce2DoCheckoutStartAsync(u32 ctx_id, vm::cptr<char[]> sku_ids, u32 sku_num, u32 container)
{
	sceNpCommerce2.todo("sceNpCommerce2DoCheckoutStartAsync(ctx_id=%d, sku_ids=*0x%x, sku_num=%d, container=%d)", ctx_id, sku_ids, sku_num, container);
	return CELL_OK;
}

error_code sceNpCommerce2DoCheckoutFinishAsync(u32 ctx_id)
{
	sceNpCommerce2.todo("sceNpCommerce2DoCheckoutFinishAsync(ctx_id=%d)", ctx_id);
	return CELL_OK;
}

error_code sceNpCommerce2DoProductBrowseStartAsync(u32 ctx_id, vm::cptr<char> product_id, u32 container, vm::cptr<SceNpCommerce2ProductBrowseParam> param)
{
	sceNpCommerce2.todo("sceNpCommerce2DoProductBrowseStartAsync(ctx_id=%d, product_id=%s, container=%d, param=*0x%x)", ctx_id, product_id, container, param);
	return CELL_OK;
}

error_code sceNpCommerce2DoProductBrowseFinishAsync(u32 ctx_id)
{
	sceNpCommerce2.todo("sceNpCommerce2DoProductBrowseFinishAsync(ctx_id=%d)", ctx_id);
	return CELL_OK;
}

error_code sceNpCommerce2DoDlListStartAsync(u32 ctx_id, vm::cptr<char> service_id, vm::cptr<char[]> sku_ids, u32 sku_num, u32 container)
{
	sceNpCommerce2.todo("sceNpCommerce2DoDlListStartAsync(ctx_id=%d, service_id=%s, sku_ids=*0x%x, sku_num=%d, container=%d)", ctx_id, service_id, sku_ids, sku_num, container);
	return CELL_OK;
}

error_code sceNpCommerce2DoDlListFinishAsync(u32 ctx_id)
{
	sceNpCommerce2.todo("sceNpCommerce2DoDlListFinishAsync(ctx_id=%d)", ctx_id);
	return CELL_OK;
}

error_code sceNpCommerce2DoProductCodeStartAsync(u32 ctx_id, u32 container, vm::cptr<SceNpCommerce2ProductCodeParam> param)
{
	sceNpCommerce2.todo("sceNpCommerce2DoProductCodeStartAsync(ctx_id=%d, container=%d, param=*0x%x)", ctx_id, container, param);
	return CELL_OK;
}

error_code sceNpCommerce2DoProductCodeFinishAsync(u32 ctx_id)
{
	sceNpCommerce2.todo("sceNpCommerce2DoProductCodeFinishAsync(ctx_id=%d)", ctx_id);
	return CELL_OK;
}

error_code sceNpCommerce2GetBGDLAvailability(vm::ptr<b8> bgdlAvailability)
{
	sceNpCommerce2.todo("sceNpCommerce2GetBGDLAvailability(bgdlAvailability=*0x%x)", bgdlAvailability);
	return CELL_OK;
}

error_code sceNpCommerce2SetBGDLAvailability(b8 bgdlAvailability)
{
	sceNpCommerce2.todo("sceNpCommerce2SetBGDLAvailability(bgdlAvailability=%d)", bgdlAvailability);
	return CELL_OK;
}

error_code sceNpCommerce2AbortReq(u32 req_id)
{
	sceNpCommerce2.todo("sceNpCommerce2AbortReq(req_id=%d)", req_id);
	return CELL_OK;
}

error_code sceNpCommerce2DestroyReq(u32 req_id)
{
	sceNpCommerce2.todo("sceNpCommerce2DestroyReq(req_id=%d)", req_id);
	return CELL_OK;
}

error_code sceNpCommerce2DoServiceListStartAsync()
{
	sceNpCommerce2.todo("sceNpCommerce2DoServiceListStartAsync()");
	return CELL_OK;
}

error_code sceNpCommerce2DoServiceListFinishAsync()
{
	sceNpCommerce2.todo("sceNpCommerce2DoServiceListFinishAsync()");
	return CELL_OK;
}

DECLARE(ppu_module_manager::sceNpCommerce2)("sceNpCommerce2", []()
{
	REG_FUNC(sceNpCommerce2, sceNpCommerce2ExecuteStoreBrowse);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2GetStoreBrowseUserdata);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2Init);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2Term);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2CreateCtx);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2DestroyCtx);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2EmptyStoreCheckStart);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2EmptyStoreCheckAbort);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2EmptyStoreCheckFinish);
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

	REG_FUNC(sceNpCommerce2, sceNpCommerce2DoServiceListStartAsync);
	REG_FUNC(sceNpCommerce2, sceNpCommerce2DoServiceListFinishAsync);
});
