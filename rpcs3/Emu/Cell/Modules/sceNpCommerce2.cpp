#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/IdManager.h"

#include "sceNpCommerce2.h"
#include "sceNp.h"
#include "cellSysutil.h"

#include "Emu/Cell/lv2/sys_process.h"

#include "Emu/NP/np_handler.h"
#include "Emu/NP/np_contexts.h"
#include "Emu/NP/cmrc2_util.h"
#include "util/cctype.hpp"

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
		}

		return unknown;
	});
}

void initialize_common_data(SceNpCommerce2CommonData* common_data, u32 data_addr, u32 data_size)
{
	/*
	* HACK: (kinda)
	* On the PS3, this struct stores pointers to areas within the opaque buffer holding the response.
	* A real response is a TLV, and each member points to a nested tag within that TLV.
	* 
	* This impl stores pointers to binarised structs (or its field members) in the opaque buffer.
	* These are then statically-cast back for type-safety and clearer usage.
	* 
	* Because function calls of different 'lookup types' need the same struct (e.g. SKU data),
	* each member stores a unique struct-type that's carried across call-boundaries.
	* reserved[0] is the most hacky aspect of this.
	*
	* - Subsequent function calls lack a req/ctx ID, so an np_context cannot be used.
	* - Indicator/Type-flag bytes before each serialised struct may be more verbose/raw.
	* - TLVs lack type-safety and clarity
	* - Protobuf (??)
	* 
	* buf_head		= s_category_info		| s_product_info_normal | s_product_list
	* data			= s_product_info_normal | s_product_info_thin
	* data2			= s_sku_info
	* reserved[0]	= s_rating_info
	*/
	std::memset(common_data, 0, sizeof(SceNpCommerce2CommonData));
	common_data->version = SCE_NP_COMMERCE2_VERSION;
	common_data->buf_head.set(data_addr);
	common_data->buf_size = data_size;
	common_data->data = vm::null;
	common_data->data2 = vm::null;
	common_data->reserved[0] = 0;
}

error_code check_data(cmrc2_lookup_type type, vm::ptr<void> data, u32 data_size)
{
	if (!data)
		return SCE_NP_COMMERCE2_ERROR_DATA_NOT_FOUND;

	switch (type)
	{
	case cmrc2_lookup_type::category_contents:
		if (data_size < sizeof(commerce2::s_category_contents))
			return SCE_NP_COMMERCE2_ERROR_INVALID_RESULT_DATA;
		break;

	case cmrc2_lookup_type::product_info:
		if (data_size < sizeof(commerce2::s_product_info_normal))
			return SCE_NP_COMMERCE2_ERROR_INVALID_RESULT_DATA;
		break;

	case cmrc2_lookup_type::product_list:
		if (data_size < sizeof(commerce2::s_product_list))
			return SCE_NP_COMMERCE2_ERROR_INVALID_RESULT_DATA;
		break;
	}

	return CELL_OK;
}

error_code sceNpCommerce2ExecuteStoreBrowse(s32 targetType, vm::cptr<char> targetId, s32 userdata)
{
	sceNpCommerce2.todo("sceNpCommerce2ExecuteStoreBrowse(targetType=%d, targetId=%s, userdata=%d)", targetType, targetId, userdata);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	if (targetType < SCE_NP_COMMERCE2_STORE_BROWSE_TYPE_CATEGORY || targetType > SCE_NP_COMMERCE2_STORE_BROWSE_TYPE_PRODUCT_CODE)
		return SCE_NP_COMMERCE2_ERROR_INVALID_TARGET_TYPE;

	if (targetType < SCE_NP_COMMERCE2_STORE_BROWSE_TYPE_PRODUCT_CODE && (!targetId || !targetId[0]))
		return SCE_NP_COMMERCE2_ERROR_INVALID_TARGET_ID;

	return CELL_OK;
}

error_code sceNpCommerce2GetStoreBrowseUserdata(vm::ptr<s32> userdata)
{
	sceNpCommerce2.todo("sceNpCommerce2GetStoreBrowseUserdata(userdata=*0x%x)", userdata);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	if (!userdata)
		return SCE_NP_COMMERCE2_ERROR_INVALID_ARGUMENT;

	*userdata = 0; // TODO

	return CELL_OK;
}

error_code sceNpCommerce2Init()
{
	sceNpCommerce2.warning("sceNpCommerce2Init()");

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_ALREADY_INITIALIZED;

	if (!nph.is_NP_init)
		return SCE_NP_ERROR_NOT_INITIALIZED;

	nph.is_NP_Com2_init = true;

	return CELL_OK;
}

error_code sceNpCommerce2Term()
{
	sceNpCommerce2.warning("sceNpCommerce2Term()");

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	nph.is_NP_Com2_init = false;

	return CELL_OK;
}

error_code sceNpCommerce2CreateCtx(u32 version, vm::cptr<SceNpId> npId, vm::ptr<SceNpCommerce2Handler> handler, vm::ptr<void> arg, vm::ptr<u32> ctx_id)
{
	sceNpCommerce2.warning("sceNpCommerce2CreateCtx(version=%d, npId=*0x%x, handler=*0x%x, arg=*0x%x, ctx_id=*0x%x)", version, npId, handler, arg, ctx_id);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	if (version > SCE_NP_COMMERCE2_VERSION)
		return SCE_NP_COMMERCE2_ERROR_UNSUPPORTED_VERSION;

	if (!npId || !ctx_id)
		return SCE_NP_COMMERCE2_ERROR_INVALID_ARGUMENT;

	s32 id = create_commerce2_context(version, npId, handler, arg);
	if (id < 0)
	{
		return id;
	}

	*ctx_id = id;

	return CELL_OK;
}

s32 sceNpCommerce2DestroyCtx(u32 ctx_id)
{
	sceNpCommerce2.warning("sceNpCommerce2DestroyCtx(ctx_id=%d)", ctx_id);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	if (!destroy_commerce2_context(ctx_id))
	{
		return SCE_NP_COMMERCE2_ERROR_CTX_NOT_FOUND;
	}

	return CELL_OK;
}

error_code sceNpCommerce2EmptyStoreCheckStart(u32 ctx_id, s32 store_check_type, vm::cptr<char> target_id)
{
	sceNpCommerce2.todo("sceNpCommerce2EmptyStoreCheckStart(ctx_id=%d, store_check_type=%d, target_id=*0x%x(%s))", ctx_id, store_check_type, target_id, target_id);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	if (!target_id || !target_id[0])
		return SCE_NP_COMMERCE2_ERROR_INVALID_TARGET_ID;

	if (store_check_type != SCE_NP_COMMERCE2_STORE_CHECK_TYPE_CATEGORY)
		return SCE_NP_COMMERCE2_ERROR_INVALID_ARGUMENT;

	const auto ctx = get_commerce2_context(ctx_id);
	if (!ctx)
		return SCE_NP_COMMERCE2_ERROR_CTX_NOT_FOUND;

	ctx->queue_callback(SCE_NP_COMMERCE2_EVENT_EMPTY_STORE_CHECK_DONE, 0);

	return CELL_OK;
}

error_code sceNpCommerce2EmptyStoreCheckAbort(u32 ctx_id)
{
	sceNpCommerce2.todo("sceNpCommerce2EmptyStoreCheckAbort(ctx_id=%d)", ctx_id);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	const auto ctx = get_commerce2_context(ctx_id);
	if (!ctx)
		return SCE_NP_COMMERCE2_ERROR_CTX_NOT_FOUND;

	ctx->queue_callback(SCE_NP_COMMERCE2_EVENT_EMPTY_STORE_CHECK_ABORT, 0);

	return CELL_OK;
}

error_code sceNpCommerce2EmptyStoreCheckFinish(u32 ctx_id, vm::ptr<s32> is_empty)
{
	sceNpCommerce2.todo("sceNpCommerce2EmptyStoreCheckFinish(ctx_id=%d, is_empty=*0x%x)", ctx_id, is_empty);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	if (!is_empty)
		return SCE_NP_COMMERCE2_ERROR_INVALID_ARGUMENT;

	*is_empty = SCE_NP_COMMERCE2_STORE_IS_NOT_EMPTY;
	return CELL_OK;
}

error_code sceNpCommerce2CreateSessionStart(u32 ctx_id)
{
	sceNpCommerce2.notice("sceNpCommerce2CreateSessionStart(ctx_id=%d)", ctx_id);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	const auto ctx = get_commerce2_context(ctx_id);
	if (!ctx)
		return SCE_NP_COMMERCE2_ERROR_CTX_NOT_FOUND;

	if (!ctx->session_created)
	{
		std::unique_lock lock(ctx->mutex, std::try_to_lock);

		if (!lock.owns_lock())
		{
			return SCE_NP_COMMERCE_ERROR_REQ_BUSY;
		}

		nph.set_commerce_session_info(&ctx->session_info);

		ctx->session_created = true;
	}

	ctx->queue_callback(SCE_NP_COMMERCE2_EVENT_CREATE_SESSION_DONE, 0);

	return CELL_OK;
}

error_code sceNpCommerce2CreateSessionAbort(u32 ctx_id)
{
	sceNpCommerce2.warning("sceNpCommerce2CreateSessionAbort(ctx_id=%d)", ctx_id);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;
	
	const auto ctx = get_commerce2_context(ctx_id);
	if (!ctx)
		return SCE_NP_COMMERCE2_ERROR_CTX_NOT_FOUND;

	ctx->queue_callback(SCE_NP_COMMERCE2_EVENT_CREATE_SESSION_ABORT, 0);

	return CELL_OK;
}

s32 sceNpCommerce2CreateSessionFinish(u32 ctx_id, vm::ptr<SceNpCommerce2SessionInfo> sessionInfo)
{
	sceNpCommerce2.notice("sceNpCommerce2CreateSessionFinish(ctx_id=%d, sessionInfo=*0x%x)", ctx_id, sessionInfo);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	const auto ctx = get_commerce2_context(ctx_id);
	if (!ctx)
		return SCE_NP_COMMERCE2_ERROR_CTX_NOT_FOUND;

	if (!ctx->session_created)
	{
		s32 sdk_ver{};
		[[maybe_unused]] s32 ret = process_get_sdk_version(process_getpid(), sdk_ver);

		if (sdk_ver < 0x300000)
		{
			std::memcpy(sessionInfo.get_ptr(), &ctx->session_info, 8); // currencyCode, decimals
		}
		else
		{
			std::memcpy(sessionInfo.get_ptr(), &ctx->session_info, sizeof(SceNpCommerce2SessionInfo));
		}
	}

	return CELL_OK;
}

error_code sceNpCommerce2GetCategoryContentsCreateReq(u32 ctx_id, vm::ptr<u32> req_id)
{
	sceNpCommerce2.notice("sceNpCommerce2GetCategoryContentsCreateReq(ctx_id=%d, req_id=*0x%x)", ctx_id, req_id);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	if (!req_id)
		return SCE_NP_COMMERCE2_ERROR_INVALID_ARGUMENT;

	const auto ctx = get_commerce2_context(ctx_id);
	if (!ctx)
		return SCE_NP_COMMERCE2_ERROR_CTX_NOT_FOUND;

	s32 id = create_commerce2_lookup_request_context(ctx_id, cmrc2_lookup_type::category_contents);
	if (id < 0)
	{
		return id;
	}

	*req_id = id;

	return CELL_OK;
}

error_code sceNpCommerce2GetCategoryContentsStart(u32 req_id, vm::cptr<char> categoryId, u32 startPosition, u32 maxCountOfResults)
{
	sceNpCommerce2.notice("sceNpCommerce2GetCategoryContentsStart(req_id=%d, categoryId=%s, startPosition=%d, maxCountOfResults=%d)", req_id, categoryId, startPosition, maxCountOfResults);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	if (!categoryId || maxCountOfResults > SCE_NP_COMMERCE2_GETCAT_MAX_COUNT)
		return SCE_NP_COMMERCE2_ERROR_INVALID_ARGUMENT;

	if (startPosition > SCE_NP_COMMERCE2_GETCAT_MAX_COUNT)
		return SCE_NP_COMMERCE2_ERROR_INVALID_INDEX;

	return nph.get_product_manager().get_category_contents(req_id, categoryId.get_ptr(), startPosition, maxCountOfResults);
}

error_code get_result(u32 req_id, cmrc2_lookup_type type, vm::ptr<void> buf, u32 buf_size, vm::ptr<u32> fill_size)
{
	if (!buf || !buf_size || !fill_size)
		return SCE_NP_COMMERCE2_ERROR_INVALID_ARGUMENT;

	const auto& req_ctx = get_commerce2_lookup_request_context(req_id);

	if (!req_ctx || req_ctx->type != type)
		return SCE_NP_COMMERCE2_ERROR_REQ_NOT_FOUND;

	if (req_ctx->result != CELL_OK)
		return req_ctx->result;

	std::vector<u8> result_data = req_ctx->result_data;

	if (result_data.empty())
		return SCE_NP_COMMERCE2_ERROR_DATA_NOT_FOUND;

	if (result_data.size() > buf_size)
		return SCE_NP_COMMERCE2_ERROR_RESPONSE_BUF_TOO_SMALL;

	*fill_size = result_data.size();
	std::memcpy(buf.get_ptr(), result_data.data(), result_data.size());

	return CELL_OK;
}

error_code sceNpCommerce2GetCategoryContentsGetResult(u32 req_id, vm::ptr<void> buf, u32 buf_size, vm::ptr<u32> fill_size)
{
	sceNpCommerce2.notice("sceNpCommerce2GetCategoryContentsGetResult(req_id=%d, buf=*0x%x, buf_size=%d, fill_size=*0x%x)", req_id, buf, buf_size, fill_size);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	if (buf_size < SCE_NP_COMMERCE2_RECV_BUF_SIZE)
		return SCE_NP_COMMERCE2_ERROR_INSUFFICIENT_BUF_SIZE;

	return get_result(req_id, cmrc2_lookup_type::category_contents, buf, buf_size, fill_size);
}

error_code sceNpCommerce2InitGetCategoryContentsResult(vm::ptr<SceNpCommerce2GetCategoryContentsResult> result, vm::ptr<void> data, u32 data_size)
{
	sceNpCommerce2.notice("sceNpCommerce2InitGetCategoryContentsResult(result=*0x%x, data=*0x%x, data_size=%d)", result, data, data_size);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	if (!result || !data || !data_size)
		return SCE_NP_COMMERCE2_ERROR_INVALID_ARGUMENT;

	error_code error = check_data(cmrc2_lookup_type::category_contents, data, data_size);
	if (error < 0)
		return error;

	std::memset(result.get_ptr(), 0, sizeof(SceNpCommerce2GetCategoryContentsResult));
	initialize_common_data(&result->commonData, data.addr(), data_size);

	const auto category_data = vm::static_ptr_cast<const commerce2::s_category_contents>(data);

	// Set rating_info ptr in reserved[0]
	result->commonData.reserved[0] = category_data.ptr(&commerce2::s_category_contents::rating_info).addr();

	result->rangeOfContents = SceNpCommerce2Range{
		.startPosition = category_data->list_start_pos,
		.count = category_data->product_count,
		.totalCountOfResults = category_data->total_products};

	return CELL_OK;
}

error_code sceNpCommerce2GetCategoryInfo(vm::cptr<SceNpCommerce2GetCategoryContentsResult> result, vm::ptr<SceNpCommerce2CategoryInfo> categoryInfo)
{
	sceNpCommerce2.notice("sceNpCommerce2GetCategoryInfo(result=*0x%x, categoryInfo=*0x%x)", result, categoryInfo);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	if (!result || !categoryInfo)
		return SCE_NP_COMMERCE2_ERROR_INVALID_ARGUMENT;

	std::memset(categoryInfo.get_ptr(), 0, sizeof(SceNpCommerce2CategoryInfo));
	std::memcpy(&categoryInfo->commonData, &result->commonData, sizeof(SceNpCommerce2CommonData));

	// Get category_contents from commonData->buf_head
	const auto category_data = vm::static_ptr_cast<const commerce2::s_category_contents>(result->commonData.buf_head);

	categoryInfo->dataType = SCE_NP_COMMERCE2_CAT_DATA_TYPE_NORMAL;
	categoryInfo->releaseDate = CellRtcTick(category_data->release_date);
	categoryInfo->countOfSubCategory = 0; // TODO - subcategories
	categoryInfo->countOfProduct = category_data->product_count;

	// Struct is backed by commonData
	categoryInfo->categoryId.set(category_data.ptr(&commerce2::s_category_contents::category_id).addr());
	categoryInfo->categoryName.set(category_data.ptr(&commerce2::s_category_contents::category_name).addr());
	categoryInfo->categoryDescription.set(category_data.ptr(&commerce2::s_category_contents::category_desc).addr());
	categoryInfo->imageUrl.set(category_data.ptr(&commerce2::s_category_contents::image_url).addr());
	categoryInfo->spName.set(category_data.ptr(&commerce2::s_category_contents::sp_name).addr());

	return CELL_OK;
}

error_code sceNpCommerce2GetContentInfo(vm::cptr<SceNpCommerce2GetCategoryContentsResult> result, u32 index, vm::ptr<SceNpCommerce2ContentInfo> contentInfo)
{
	sceNpCommerce2.notice("sceNpCommerce2GetContentInfo(result=*0x%x, index=%d, contentInfo=*0x%x)", result, index, contentInfo);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	if (!result || !contentInfo)
		return SCE_NP_COMMERCE2_ERROR_INVALID_ARGUMENT;

	std::memset(contentInfo.get_ptr(), 0, sizeof(SceNpCommerce2ContentInfo));
	std::memcpy(&contentInfo->commonData, &result->commonData, sizeof(SceNpCommerce2CommonData));

	// Get category_contents from commonData->buf_head
	const auto category_data = vm::static_ptr_cast<const commerce2::s_category_contents>(contentInfo->commonData.buf_head);

	contentInfo->contentType = SCE_NP_COMMERCE2_CONTENT_TYPE_PRODUCT; // TODO - subcategories

	// Set product_info ptr in commonData->data
	auto product_ptr = category_data.ptr(&commerce2::s_category_contents::products, index);
	contentInfo->commonData.data = vm::static_ptr_cast<void>(product_ptr);
	contentInfo->commonData.data_size = sizeof(commerce2::s_product_info_thin);

	return CELL_OK;
}

error_code sceNpCommerce2GetCategoryInfoFromContentInfo(vm::cptr<SceNpCommerce2ContentInfo> contentInfo, vm::ptr<SceNpCommerce2CategoryInfo> categoryInfo)
{
	sceNpCommerce2.notice("sceNpCommerce2GetCategoryInfoFromContentInfo(contentInfo=*0x%x, categoryInfo=*0x%x)", contentInfo, categoryInfo);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	if (!contentInfo || !categoryInfo)
		return SCE_NP_COMMERCE2_ERROR_INVALID_ARGUMENT;

	// TODO - subcategories

	return CELL_OK;
}

error_code sceNpCommerce2GetGameProductInfoFromContentInfo(vm::cptr<SceNpCommerce2ContentInfo> contentInfo, vm::ptr<SceNpCommerce2GameProductInfo> gameProductInfo)
{
	sceNpCommerce2.warning("sceNpCommerce2GetGameProductInfoFromContentInfo(contentInfo=*0x%x, gameProductInfo=*0x%x)", contentInfo, gameProductInfo);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	if (!contentInfo || !gameProductInfo)
		return SCE_NP_COMMERCE2_ERROR_INVALID_ARGUMENT;

	std::memset(gameProductInfo.get_ptr(), 0, sizeof(SceNpCommerce2GameProductInfo));
	std::memcpy(&gameProductInfo->commonData, &contentInfo->commonData, sizeof(SceNpCommerce2CommonData));

	// Get product_info from commonData->data
	const auto product_data = vm::static_ptr_cast<const commerce2::s_product_info_thin>(contentInfo->commonData.data);

	gameProductInfo->dataType = SCE_NP_COMMERCE2_GAME_PRODUCT_DATA_TYPE_THIN;
	gameProductInfo->countOfSku = product_data->sku_count;
	gameProductInfo->releaseDate = CellRtcTick(product_data->release_date);

	// Struct is backed by commonData
	gameProductInfo->productId.set(product_data.ptr(&commerce2::s_product_info_thin::product_id).addr());
	gameProductInfo->productName.set(product_data.ptr(&commerce2::s_product_info_thin::product_name).addr());
	gameProductInfo->productShortDescription.set(product_data.ptr(&commerce2::s_product_info_thin::product_desc_short).addr());
	gameProductInfo->spName.set(product_data.ptr(&commerce2::s_product_info_thin::sp_name).addr());
	gameProductInfo->imageUrl.set(product_data.ptr(&commerce2::s_product_info_thin::image_url).addr());

	// Set legal desc from category_contents
	auto category_ptr = vm::static_ptr_cast<const commerce2::s_category_contents>(contentInfo->commonData.buf_head); 
	gameProductInfo->legalDescription.set(category_ptr.ptr(&commerce2::s_category_contents::legal_desc).addr());

	// Set SKUs ptr into commonData->data2
	auto skus_ptr = product_data.ptr(&commerce2::s_product_info_thin::skus);
	gameProductInfo->commonData.data2 = vm::static_ptr_cast<void>(skus_ptr);

	return CELL_OK;
}

error_code sceNpCommerce2DestroyGetCategoryContentsResult(vm::ptr<SceNpCommerce2GetCategoryContentsResult> result)
{
	sceNpCommerce2.notice("sceNpCommerce2DestroyGetCategoryContentsResult(result=*0x%x)", result);
	return CELL_OK;
}

error_code sceNpCommerce2GetProductInfoCreateReq(u32 ctx_id, vm::ptr<u32> req_id)
{
	sceNpCommerce2.notice("sceNpCommerce2GetProductInfoCreateReq(ctx_id=%d, req_id=*0x%x)", ctx_id, req_id);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	if (!req_id)
		return SCE_NP_COMMERCE2_ERROR_INVALID_ARGUMENT;

	const auto ctx = get_commerce2_context(ctx_id);
	if (!ctx)
		return SCE_NP_COMMERCE2_ERROR_CTX_NOT_FOUND;

	s32 id = create_commerce2_lookup_request_context(ctx_id, cmrc2_lookup_type::product_info);
	if (id < 0)
	{
		return id;
	}

	*req_id = id;

	return CELL_OK;
}

error_code sceNpCommerce2GetProductInfoStart(u32 req_id, vm::cptr<char> categoryId, vm::cptr<char> productId)
{
	sceNpCommerce2.notice("sceNpCommerce2GetProductInfoStart(req_id=%d, categoryId=%s, productId=%s)", req_id, categoryId, productId);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	if (!productId)
		return SCE_NP_COMMERCE2_ERROR_INVALID_ARGUMENT;

	return nph.get_product_manager().get_product_info(req_id, categoryId.get_ptr(), productId.get_ptr());
}

error_code sceNpCommerce2GetProductInfoGetResult(u32 req_id, vm::ptr<void> buf, u32 buf_size, vm::ptr<u32> fill_size)
{
	sceNpCommerce2.notice("sceNpCommerce2GetProductInfoGetResult(req_id=%d, buf=*0x%x, buf_size=%d, fill_size=*0x%x)", req_id, buf, buf_size, fill_size);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	if (buf_size < SCE_NP_COMMERCE2_RECV_BUF_SIZE)
		return SCE_NP_COMMERCE2_ERROR_INSUFFICIENT_BUF_SIZE;

	return get_result(req_id, cmrc2_lookup_type::product_info, buf, buf_size, fill_size);
}

error_code sceNpCommerce2InitGetProductInfoResult(vm::ptr<SceNpCommerce2GetProductInfoResult> result, vm::ptr<void> data, u32 data_size)
{
	sceNpCommerce2.notice("sceNpCommerce2InitGetProductInfoResult(result=*0x%x, data=*0x%x, data_size=%d)", result, data, data_size);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	if (!result || !data || !data_size)
		return SCE_NP_COMMERCE2_ERROR_INVALID_ARGUMENT;

	error_code error = check_data(cmrc2_lookup_type::product_info, data, data_size);
	if (error < 0)
		return error;

	std::memset(result.get_ptr(), 0, sizeof(SceNpCommerce2GetProductInfoResult));
	initialize_common_data(&result->commonData, data.addr(), data_size);

	const auto product_data = vm::static_ptr_cast<const commerce2::s_product_info_normal>(data);

	// Store rating_info ptr in reserved[0]
	result->commonData.reserved[0] = product_data.ptr(&commerce2::s_product_info_normal::rating_info).addr();

	return CELL_OK;
}

error_code sceNpCommerce2GetGameProductInfo(vm::cptr<SceNpCommerce2GetProductInfoResult> result, vm::ptr<SceNpCommerce2GameProductInfo> gameProductInfo)
{
	sceNpCommerce2.warning("sceNpCommerce2GetGameProductInfo(result=*0x%x, gameProductInfo=*0x%x)", result, gameProductInfo);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	if (!result || !gameProductInfo)
		return SCE_NP_COMMERCE2_ERROR_INVALID_ARGUMENT;
	
	std::memset(gameProductInfo.get_ptr(), 0, sizeof(SceNpCommerce2GameProductInfo));
	std::memcpy(&gameProductInfo->commonData, &result->commonData, sizeof(SceNpCommerce2CommonData));

	// Get product_info from commonData->buf_head
	const auto product_data = vm::static_ptr_cast<const commerce2::s_product_info_normal>(result->commonData.buf_head);

	gameProductInfo->dataType = SCE_NP_COMMERCE2_GAME_PRODUCT_DATA_TYPE_NORMAL;
	gameProductInfo->countOfSku = product_data->sku_count;
	gameProductInfo->releaseDate = CellRtcTick(product_data->release_date);

	// Struct is backed by commonData
	gameProductInfo->productId.set(product_data.ptr(&commerce2::s_product_info_normal::product_id).addr());
	gameProductInfo->productName.set(product_data.ptr(&commerce2::s_product_info_normal::product_name).addr());
	gameProductInfo->productShortDescription.set(product_data.ptr(&commerce2::s_product_info_normal::product_desc_short).addr());
	gameProductInfo->productLongDescription.set(product_data.ptr(&commerce2::s_product_info_normal::product_desc_long).addr());
	gameProductInfo->spName.set(product_data.ptr(&commerce2::s_product_info_normal::sp_name).addr());
	gameProductInfo->imageUrl.set(product_data.ptr(&commerce2::s_product_info_normal::image_url).addr());
	gameProductInfo->legalDescription.set(product_data.ptr(&commerce2::s_product_info_normal::legal_desc).addr());

	// Set product_info ptr into commonData->data
	gameProductInfo->commonData.data = gameProductInfo->commonData.buf_head;
	gameProductInfo->commonData.data_size = gameProductInfo->commonData.buf_size;

	// Set SKUs ptr into commonData->data2
	auto skus_ptr = product_data.ptr(&commerce2::s_product_info_normal::skus);
	gameProductInfo->commonData.data2 = vm::static_ptr_cast<void>(skus_ptr);

	return CELL_OK;
}

error_code sceNpCommerce2DestroyGetProductInfoResult(vm::ptr<SceNpCommerce2GetProductInfoResult> result)
{
	sceNpCommerce2.notice("sceNpCommerce2DestroyGetProductInfoResult(result=*0x%x)", result);
	return CELL_OK;
}

error_code sceNpCommerce2GetProductInfoListCreateReq(u32 ctx_id, vm::ptr<u32> req_id)
{
	sceNpCommerce2.notice("sceNpCommerce2GetProductInfoListCreateReq(ctx_id=%d, req_id=*0x%x)", ctx_id, req_id);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	if (!req_id)
		return SCE_NP_COMMERCE2_ERROR_INVALID_ARGUMENT;

	const auto ctx = get_commerce2_context(ctx_id);
	if (!ctx)
		return SCE_NP_COMMERCE2_ERROR_CTX_NOT_FOUND;

	s32 id = create_commerce2_lookup_request_context(ctx_id, cmrc2_lookup_type::product_list);
	if (id < 0)
	{
		return id;
	}

	*req_id = id;

	return CELL_OK;
}

error_code sceNpCommerce2GetProductInfoListStart(u32 req_id, vm::cpptr<char> productIds, u32 productNum)
{
	sceNpCommerce2.notice("sceNpCommerce2GetProductInfoListStart(req_id=%d, productIds=*0x%x, productNum=%d)", req_id, productIds, productNum);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	if (!productIds || !productNum || productNum > SCE_NP_COMMERCE2_GETPRODLIST_MAX_COUNT)
		return SCE_NP_COMMERCE2_ERROR_INVALID_ARGUMENT;

	return nph.get_product_manager().get_product_info_list(req_id, productIds, productNum);
}

error_code sceNpCommerce2GetProductInfoListGetResult(u32 req_id, vm::ptr<void> buf, u32 buf_size, vm::ptr<u32> fill_size)
{
	sceNpCommerce2.notice("sceNpCommerce2GetProductInfoListGetResult(req_id=%d, buf=*0x%x, buf_size=%d, fill_size=*0x%x)", req_id, buf, buf_size, fill_size);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	if (buf_size < SCE_NP_COMMERCE2_RECV_BUF_SIZE)
		return SCE_NP_COMMERCE2_ERROR_INSUFFICIENT_BUF_SIZE;

	return get_result(req_id, cmrc2_lookup_type::product_list, buf, buf_size, fill_size);
}

error_code sceNpCommerce2InitGetProductInfoListResult(vm::ptr<SceNpCommerce2GetProductInfoListResult> result, vm::ptr<void> data, u32 data_size)
{
	sceNpCommerce2.notice("sceNpCommerce2InitGetProductInfoListResult(result=*0x%x, data=*0x%x, data_size=%d)", result, data, data_size);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	if (!result || !data || !data_size)
		return SCE_NP_COMMERCE2_ERROR_INVALID_ARGUMENT;

	error_code error = check_data(cmrc2_lookup_type::product_list, data, data_size);
	if (error < 0)
		return error;

	const auto product_list = vm::static_ptr_cast<const commerce2::s_product_list>(data);

	std::memset(result.get_ptr(), 0, sizeof(SceNpCommerce2GetProductInfoListResult));
	initialize_common_data(&result->commonData, data.addr(), data_size);

	// Store rating_info ptr in reserved[0]
	result->commonData.reserved[0] = product_list.ptr(&commerce2::s_product_list::rating_info).addr();

	result->countOfProductInfo = product_list->product_count;

	return CELL_OK;
}

error_code sceNpCommerce2GetGameProductInfoFromGetProductInfoListResult(vm::cptr<SceNpCommerce2GetProductInfoListResult> result, u32 index, vm::ptr<SceNpCommerce2GameProductInfo> gameProductInfo)
{
	sceNpCommerce2.warning("sceNpCommerce2GetGameProductInfoFromGetProductInfoListResult(result=*0x%x, index=%d, data=*0x%x)", result, index, gameProductInfo);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	if (!result || !gameProductInfo)
		return SCE_NP_COMMERCE2_ERROR_INVALID_ARGUMENT;

	std::memset(gameProductInfo.get_ptr(), 0, sizeof(SceNpCommerce2GameProductInfo));
	std::memcpy(&gameProductInfo->commonData, &result->commonData, sizeof(SceNpCommerce2CommonData));

	// Get product_list from commonData->buf_head
	const auto product_list = vm::static_ptr_cast<const commerce2::s_product_list>(result->commonData.buf_head);
	const auto* product = &product_list->products[index];
	const auto product_ptr = product_list.ptr(&commerce2::s_product_list::products, index);

	gameProductInfo->dataType = SCE_NP_COMMERCE2_GAME_PRODUCT_DATA_TYPE_THIN;
	gameProductInfo->countOfSku = product->sku_count;
	gameProductInfo->releaseDate = CellRtcTick(product->release_date);

	// Struct is backed by commonData
	gameProductInfo->productId.set(product_ptr.ptr(&commerce2::s_product_info_thin::product_id).addr());
	gameProductInfo->productName.set(product_ptr.ptr(&commerce2::s_product_info_thin::product_name).addr());
	gameProductInfo->productShortDescription.set(product_ptr.ptr(&commerce2::s_product_info_thin::product_desc_short).addr());
	gameProductInfo->spName.set(product_ptr.ptr(&commerce2::s_product_info_thin::sp_name).addr());
	gameProductInfo->imageUrl.set(product_ptr.ptr(&commerce2::s_product_info_thin::image_url).addr());

	// Set legal desc from product_list
	gameProductInfo->legalDescription.set(product_list.ptr(&commerce2::s_product_list::legal_desc).addr());

	// Set SKUs ptr into commonData->data2
	auto skus_ptr = product_ptr.ptr(&commerce2::s_product_info_thin::skus);
	gameProductInfo->commonData.data2 = vm::static_ptr_cast<void>(skus_ptr);

	return CELL_OK;
}

error_code sceNpCommerce2DestroyGetProductInfoListResult(vm::ptr<SceNpCommerce2GetProductInfoListResult> result)
{
	sceNpCommerce2.notice("sceNpCommerce2DestroyGetProductInfoListResult(result=*0x%x)", result);
	return CELL_OK;
}

error_code sceNpCommerce2GetContentRatingInfoFromGameProductInfo(vm::cptr<SceNpCommerce2GameProductInfo> gameProductInfo, vm::ptr<SceNpCommerce2ContentRatingInfo> contentRatingInfo)
{
	sceNpCommerce2.notice("sceNpCommerce2GetContentRatingInfoFromGameProductInfo(gameProductInfo=*0x%x, contentRatingInfo=*0x%x)", gameProductInfo, contentRatingInfo);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	if (!gameProductInfo || !contentRatingInfo)
		return SCE_NP_COMMERCE2_ERROR_INVALID_ARGUMENT;

	std::memset(contentRatingInfo.get_ptr(), 0, sizeof(SceNpCommerce2ContentRatingInfo));
	std::memcpy(&contentRatingInfo->commonData, &gameProductInfo->commonData, sizeof(SceNpCommerce2CommonData));

	// Get rating_info ptr from commonData->reserved[0]
	const auto rating_info = vm::cptr<commerce2::s_rating_info>::make(gameProductInfo->commonData.reserved[0]);

	contentRatingInfo->countOfContentRatingDescriptor = 2;
	contentRatingInfo->ratingSystemId.set(rating_info.ptr(&commerce2::s_rating_info::rating_sys_id).addr());
	contentRatingInfo->imageUrl.set(rating_info.ptr(&commerce2::s_rating_info::rating_sys_image_url).addr());

	return CELL_OK;
}

error_code sceNpCommerce2GetContentRatingInfoFromCategoryInfo(vm::cptr<SceNpCommerce2CategoryInfo> categoryInfo, vm::ptr<SceNpCommerce2ContentRatingInfo> contentRatingInfo)
{
	sceNpCommerce2.notice("sceNpCommerce2GetContentRatingInfoFromCategoryInfo(categoryInfo=*0x%x, contentRatingInfo=*0x%x)", categoryInfo, contentRatingInfo);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	if (!categoryInfo || !contentRatingInfo)
		return SCE_NP_COMMERCE2_ERROR_INVALID_ARGUMENT;

	std::memset(contentRatingInfo.get_ptr(), 0, sizeof(SceNpCommerce2ContentRatingInfo));
	std::memcpy(&contentRatingInfo->commonData, &categoryInfo->commonData, sizeof(SceNpCommerce2CommonData));

	// Get rating_info ptr from commonData->reserved[0]
	const auto rating_info = vm::cptr<commerce2::s_rating_info>::make(categoryInfo->commonData.reserved[0]);

	contentRatingInfo->countOfContentRatingDescriptor = 2;
	contentRatingInfo->ratingSystemId.set(rating_info.ptr(&commerce2::s_rating_info::rating_sys_id).addr());
	contentRatingInfo->imageUrl.set(rating_info.ptr(&commerce2::s_rating_info::rating_sys_image_url).addr());

	return CELL_OK;
}

error_code sceNpCommerce2GetContentRatingDescriptor(vm::cptr<SceNpCommerce2ContentRatingInfo> contentRatingInfo, u32 index, vm::ptr<SceNpCommerce2ContentRatingDescriptor> contentRatingDescriptor)
{
	sceNpCommerce2.warning("sceNpCommerce2GetContentRatingDescriptor(contentRatingInfo=*0x%x, index=%d, contentRatingDescriptor=*0x%x)", contentRatingInfo, index, contentRatingDescriptor);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	if (!contentRatingInfo || !contentRatingDescriptor)
		return SCE_NP_COMMERCE2_ERROR_INVALID_ARGUMENT;

	std::memset(contentRatingDescriptor.get_ptr(), 0, sizeof(SceNpCommerce2ContentRatingDescriptor));
	std::memcpy(&contentRatingDescriptor->commonData, &contentRatingInfo->commonData, sizeof(SceNpCommerce2CommonData));

	// Get rating_info ptr from commonData->reserved[0]
	const auto rating_info = vm::cptr<commerce2::s_rating_info>::make(contentRatingInfo->commonData.reserved[0]);

	// HACK - Handle indices manually (only ever 2: one icon, one description)
	if (index == 0)
	{
		contentRatingDescriptor->descriptorType = SCE_NP_COMMERCE2_CONTENT_RATING_DESC_TYPE_ICON;
		contentRatingDescriptor->imageUrl.set(rating_info.ptr(&commerce2::s_rating_info::rating_sys_image_url).addr());
	}
	else
	{
		contentRatingDescriptor->descriptorType = SCE_NP_COMMERCE2_CONTENT_RATING_DESC_TYPE_TEXT;
		contentRatingDescriptor->contentRatingDescription.set(rating_info.ptr(&commerce2::s_rating_info::rating_desc_text).addr());
	}

	return CELL_OK;
}

error_code sceNpCommerce2GetGameSkuInfoFromGameProductInfo(vm::cptr<SceNpCommerce2GameProductInfo> gameProductInfo, u32 index, vm::ptr<SceNpCommerce2GameSkuInfo> gameSkuInfo)
{
	sceNpCommerce2.warning("sceNpCommerce2GetGameSkuInfoFromGameProductInfo(gameProductInfo=*0x%x, index=%d, gameSkuInfo=*0x%x)", gameProductInfo, index, gameSkuInfo);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	if (!gameProductInfo || !gameSkuInfo)
		return SCE_NP_COMMERCE2_ERROR_INVALID_ARGUMENT;

	std::memset(gameSkuInfo.get_ptr(), 0, sizeof(SceNpCommerce2GameSkuInfo));
	std::memcpy(&gameSkuInfo->commonData, &gameProductInfo->commonData, sizeof(SceNpCommerce2CommonData));

	// Get sku_info from commonData->data2
	const auto sku_info = vm::static_ptr_cast<const commerce2::s_sku_info>(gameProductInfo->commonData.data2);

	SceNpCommerce2GameSkuDataType dataType = SCE_NP_COMMERCE2_GAME_SKU_DATA_TYPE_THIN;
	if (gameProductInfo->dataType == SCE_NP_COMMERCE2_GAME_PRODUCT_DATA_TYPE_NORMAL)
	{
		dataType = SCE_NP_COMMERCE2_GAME_SKU_DATA_TYPE_NORMAL;
	}

	// Consumption information
	u32 purchasability_flag;
	u32 annotation;

	bool is_owned = nph.has_entitlement(sku_info->sku_id);
	auto consumable_info = nph.get_product_manager().get_product_consumable_info(gameProductInfo->productId.get_ptr());

	if (consumable_info.is_consumable)
	{
		purchasability_flag = SCE_NP_COMMERCE2_SKU_PURCHASABILITY_FLAG_ON;
		annotation = is_owned ? SCE_NP_COMMERCE2_SKU_ANN_PURCHASED_CAN_PURCHASE_AGAIN : 0;
	}
	else
	{
		purchasability_flag = is_owned ? SCE_NP_COMMERCE2_SKU_PURCHASABILITY_FLAG_OFF : SCE_NP_COMMERCE2_SKU_PURCHASABILITY_FLAG_ON;
		annotation = is_owned ? SCE_NP_COMMERCE2_SKU_ANN_PURCHASED_CANNOT_PURCHASE_AGAIN : 0;
	}

	gameSkuInfo->dataType = dataType;
	gameSkuInfo->countUntilExpiration = consumable_info.consumable_count;
	gameSkuInfo->timeUntilExpiration = 999999999;
	gameSkuInfo->purchasabilityFlag = purchasability_flag;
	gameSkuInfo->annotation = annotation;
	gameSkuInfo->downloadable = false;
	gameSkuInfo->price = 0;

	// Struct is backed by commonData
	gameSkuInfo->skuId.set(sku_info.ptr(&commerce2::s_sku_info::sku_id).addr());
	gameSkuInfo->skuName.set(sku_info.ptr(&commerce2::s_sku_info::sku_name).addr());
	gameSkuInfo->contentLinkUrl.set(sku_info.ptr(&commerce2::s_sku_info::content_link).addr());
	gameSkuInfo->productId = gameProductInfo->productId;

	return CELL_OK;
}

error_code sceNpCommerce2GetPrice(u32 ctx_id, vm::ptr<char> buf, u32 buflen, u32 price)
{
	sceNpCommerce2.notice("sceNpCommerce2GetPrice(ctx_id=%d, buf=*0x%x, buflen=%d, price=%d)", ctx_id, buf, buflen, price);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	const auto ctx = get_commerce2_context(ctx_id);
	if (!ctx)
		return SCE_NP_COMMERCE2_ERROR_CTX_NOT_FOUND;

	std::memset(buf.get_ptr(), 0, buflen);
	std::string fmt_price = commerce2::format_currency_str(ctx->session_info);
	std::memcpy(buf.get_ptr(), fmt_price.c_str(), std::min(buflen, static_cast<u32>(fmt_price.size())));

	return CELL_OK;
}

error_code sceNpCommerce2DoCheckoutStartAsync(u32 ctx_id, vm::cpptr<char> sku_ids, u32 sku_num, u32 container)
{
	sceNpCommerce2.notice("sceNpCommerce2DoCheckoutStartAsync(ctx_id=%d, sku_ids=*0x%x, sku_num=%d, container=%d)", ctx_id, sku_ids, sku_num, container);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	if (sku_num > SCE_NP_COMMERCE2_SKU_CHECKOUT_MAX)
		return SCE_NP_COMMERCE2_ERROR_INVALID_SKU_NUM;

	for (u32 i = 0; i < sku_num; i++)
	{
		if (!sku_ids || !sku_ids[i] || !sku_ids[i][0])
			return SCE_NP_COMMERCE2_ERROR_INVALID_SKUID;
	}

	const auto ctx = get_commerce2_context(ctx_id);
	if (!ctx)
		return SCE_NP_COMMERCE2_ERROR_CTX_NOT_FOUND;

	ctx->queue_callback(SCE_NP_COMMERCE2_EVENT_DO_CHECKOUT_STARTED, 0);

	if (nph.add_entitlements(sku_ids, sku_num))
	{
		ctx->queue_callback(SCE_NP_COMMERCE2_EVENT_DO_CHECKOUT_SUCCESS, 0);
	}
	else
	{
		ctx->queue_callback(SCE_NP_COMMERCE2_EVENT_REQUEST_ERROR, 0);
	}

	return CELL_OK;
}

error_code sceNpCommerce2DoCheckoutFinishAsync(u32 ctx_id)
{
	sceNpCommerce2.notice("sceNpCommerce2DoCheckoutFinishAsync(ctx_id=%d)", ctx_id);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	const auto ctx = get_commerce2_context(ctx_id);
	if (!ctx)
		return SCE_NP_COMMERCE2_ERROR_CTX_NOT_FOUND;

	ctx->queue_callback(SCE_NP_COMMERCE2_EVENT_DO_CHECKOUT_FINISHED, 0);

	return CELL_OK;
}

error_code sceNpCommerce2DoProductBrowseStartAsync(u32 ctx_id, vm::cptr<char> product_id, u32 container, vm::cptr<SceNpCommerce2ProductBrowseParam> param)
{
	sceNpCommerce2.todo("sceNpCommerce2DoProductBrowseStartAsync(ctx_id=%d, product_id=%s, container=%d, param=*0x%x)", ctx_id, product_id, container, param);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	if (!product_id || !product_id[0])
		return SCE_NP_COMMERCE2_ERROR_INVALID_ARGUMENT;

	if (param && param->size != sizeof(SceNpCommerce2ProductBrowseParam))
		return SCE_NP_COMMERCE2_ERROR_INVALID_SIZE;

	if (false) // TODO
		return SCE_NP_COMMERCE2_ERROR_INVALID_MEMORY_CONTAINER;

	if (false) // TODO
		return SCE_NP_COMMERCE2_ERROR_INSUFFICIENT_MEMORY_CONTAINER;

	const auto ctx = get_commerce2_context(ctx_id);
	if (!ctx)
		return SCE_NP_COMMERCE2_ERROR_CTX_NOT_FOUND;

	return CELL_OK;
}

error_code sceNpCommerce2DoProductBrowseFinishAsync(u32 ctx_id)
{
	sceNpCommerce2.todo("sceNpCommerce2DoProductBrowseFinishAsync(ctx_id=%d)", ctx_id);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	const auto ctx = get_commerce2_context(ctx_id);
	if (!ctx)
		return SCE_NP_COMMERCE2_ERROR_CTX_NOT_FOUND;

	return CELL_OK;
}

error_code sceNpCommerce2DoDlListStartAsync(u32 ctx_id, vm::cptr<char> service_id, vm::cpptr<char> sku_ids, u32 sku_num, u32 container)
{
	sceNpCommerce2.todo("sceNpCommerce2DoDlListStartAsync(ctx_id=%d, service_id=%s, sku_ids=*0x%x, sku_num=%d, container=%d)", ctx_id, service_id, sku_ids, sku_num, container);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	if (sku_num > SCE_NP_COMMERCE2_SKU_CHECKOUT_MAX)
		return SCE_NP_COMMERCE2_ERROR_INVALID_SKU_NUM;

	if (sku_num == 0)
	{
		if (!service_id)
			return SCE_NP_COMMERCE2_ERROR_INVALID_ARGUMENT;
	}
	else
	{
		for (u32 i = 0; i < sku_num; i++)
		{
			if (!sku_ids || !sku_ids[i] || !sku_ids[i][0])
				return SCE_NP_COMMERCE2_ERROR_INVALID_SKUID;
		}
	}

	if (false) // TODO
		return SCE_NP_COMMERCE2_ERROR_INVALID_MEMORY_CONTAINER;

	if (false) // TODO
		return SCE_NP_COMMERCE2_ERROR_INSUFFICIENT_MEMORY_CONTAINER;

	const auto ctx = get_commerce2_context(ctx_id);
	if (!ctx)
		return SCE_NP_COMMERCE2_ERROR_CTX_NOT_FOUND;

	return CELL_OK;
}

error_code sceNpCommerce2DoDlListFinishAsync(u32 ctx_id)
{
	sceNpCommerce2.todo("sceNpCommerce2DoDlListFinishAsync(ctx_id=%d)", ctx_id);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	const auto ctx = get_commerce2_context(ctx_id);
	if (!ctx)
		return SCE_NP_COMMERCE2_ERROR_CTX_NOT_FOUND;

	return CELL_OK;
}

error_code sceNpCommerce2DoProductCodeStartAsync(u32 ctx_id, u32 container, vm::cptr<SceNpCommerce2ProductCodeParam> param)
{
	sceNpCommerce2.todo("sceNpCommerce2DoProductCodeStartAsync(ctx_id=%d, container=%d, param=*0x%x)", ctx_id, container, param);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	if (!param)
		return SCE_NP_COMMERCE2_ERROR_INVALID_ARGUMENT;

	if (param->size != sizeof(SceNpCommerce2ProductCodeParam))
		return SCE_NP_COMMERCE2_ERROR_INVALID_SIZE;

	if (param->inputMode != SCE_NP_COMMERCE2_PRODUCT_CODE_INPUT_MODE_USER_INPUT)
	{
		if (param->inputMode != SCE_NP_COMMERCE2_PRODUCT_CODE_INPUT_MODE_CODE_SPECIFIED)
			return SCE_NP_COMMERCE2_ERROR_INVALID_TARGET_TYPE;

		if (!param->code1[0] || !param->code2[0] || !param->code3[0])
			return SCE_NP_COMMERCE2_ERROR_INVALID_TARGET_ID;

		for (u32 i = 0; i < SCE_NP_COMMERCE2_PRODUCT_CODE_BLOCK_LEN; i++)
		{
			if (!utils::isalnum(param->code1[i]) || !utils::isalnum(param->code2[i]) || !utils::isalnum(param->code3[i]))
				return SCE_NP_COMMERCE2_ERROR_INVALID_TARGET_ID;
		}
	}

	if (false) // TODO
		return SCE_NP_COMMERCE2_ERROR_INVALID_MEMORY_CONTAINER;

	if (false) // TODO
		return SCE_NP_COMMERCE2_ERROR_INSUFFICIENT_MEMORY_CONTAINER;

	const auto ctx = get_commerce2_context(ctx_id);
	if (!ctx)
		return SCE_NP_COMMERCE2_ERROR_CTX_NOT_FOUND;

	return CELL_OK;
}

error_code sceNpCommerce2DoProductCodeFinishAsync(u32 ctx_id)
{
	sceNpCommerce2.todo("sceNpCommerce2DoProductCodeFinishAsync(ctx_id=%d)", ctx_id);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	const auto ctx = get_commerce2_context(ctx_id);
	if (!ctx)
		return SCE_NP_COMMERCE2_ERROR_CTX_NOT_FOUND;

	return CELL_OK;
}

error_code sceNpCommerce2GetBGDLAvailability(vm::ptr<b8> bgdlAvailability)
{
	sceNpCommerce2.todo("sceNpCommerce2GetBGDLAvailability(bgdlAvailability=*0x%x)", bgdlAvailability);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	if (!bgdlAvailability)
		return SCE_NP_COMMERCE2_ERROR_INVALID_ARGUMENT;

	*bgdlAvailability = false; // TODO

	return CELL_OK;
}

error_code sceNpCommerce2SetBGDLAvailability(b8 bgdlAvailability)
{
	sceNpCommerce2.todo("sceNpCommerce2SetBGDLAvailability(bgdlAvailability=%d)", bgdlAvailability);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	return CELL_OK;
}

error_code sceNpCommerce2AbortReq(u32 req_id)
{
	sceNpCommerce2.warning("sceNpCommerce2AbortReq(req_id=%d)", req_id);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	destroy_commerce2_lookup_request_context(req_id);

	return CELL_OK;
}

error_code sceNpCommerce2DestroyReq(u32 req_id)
{
	sceNpCommerce2.warning("sceNpCommerce2DestroyReq(req_id=%d)", req_id);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	destroy_commerce2_lookup_request_context(req_id);

	return CELL_OK;
}

error_code sceNpCommerce2DoServiceListStartAsync(u32 ctx_id, vm::ptr<char> serviceId, u32 container, vm::ptr<SceNpCommerce2ProductBrowseParam> param)
{
	sceNpCommerce2.todo("sceNpCommerce2DoServiceListStartAsync(ctx_id=%d, serviceId=*0x%x, container=%d, param=*0x%x)", ctx_id, serviceId, container, param);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	if (!param || !serviceId)
		return SCE_NP_COMMERCE2_ERROR_INVALID_ARGUMENT;

	if (param->size != sizeof(SceNpCommerce2ProductBrowseParam))
		return SCE_NP_COMMERCE2_ERROR_INVALID_SIZE;

	if (!serviceId[0])
		return SCE_NP_COMMERCE2_ERROR_INVALID_TARGET_ID;

	if (false) // TODO
		return SCE_NP_COMMERCE2_ERROR_INVALID_MEMORY_CONTAINER;

	if (false) // TODO
		return SCE_NP_COMMERCE2_ERROR_INSUFFICIENT_MEMORY_CONTAINER;

	const auto ctx = get_commerce2_context(ctx_id);
	if (!ctx)
		return SCE_NP_COMMERCE2_ERROR_CTX_NOT_FOUND;

	return CELL_OK;
}

error_code sceNpCommerce2DoServiceListFinishAsync(u32 ctx_id)
{
	sceNpCommerce2.todo("sceNpCommerce2DoServiceListFinishAsync(ctx_id=%d)", ctx_id);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_NP_Com2_init)
		return SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED;

	const auto ctx = get_commerce2_context(ctx_id);
	if (!ctx)
		return SCE_NP_COMMERCE2_ERROR_CTX_NOT_FOUND;

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
