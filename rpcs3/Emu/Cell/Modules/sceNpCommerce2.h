#pragma once

#include "cellRtc.h"

// Return codes
enum SceNpCommerce2Error
{
	SCE_NP_COMMERCE2_ERROR_NOT_INITIALIZED                      = 0x80023001,
	SCE_NP_COMMERCE2_ERROR_ALREADY_INITIALIZED                  = 0x80023002,
	SCE_NP_COMMERCE2_ERROR_INVALID_ARGUMENT                     = 0x80023003,
	SCE_NP_COMMERCE2_ERROR_UNSUPPORTED_VERSION                  = 0x80023004,
	SCE_NP_COMMERCE2_ERROR_CTX_MAX                              = 0x80023005,
	SCE_NP_COMMERCE2_ERROR_INVALID_INDEX                        = 0x80023006,
	SCE_NP_COMMERCE2_ERROR_INVALID_SKUID                        = 0x80023007,
	SCE_NP_COMMERCE2_ERROR_INVALID_SKU_NUM                      = 0x80023008,
	SCE_NP_COMMERCE2_ERROR_INVALID_MEMORY_CONTAINER             = 0x80023009,
	SCE_NP_COMMERCE2_ERROR_INSUFFICIENT_MEMORY_CONTAINER        = 0x8002300a,
	SCE_NP_COMMERCE2_ERROR_OUT_OF_MEMORY                        = 0x8002300b,
	SCE_NP_COMMERCE2_ERROR_CTX_NOT_FOUND                        = 0x8002300c,
	SCE_NP_COMMERCE2_ERROR_CTXID_NOT_AVAILABLE                  = 0x8002300d,
	SCE_NP_COMMERCE2_ERROR_REQ_NOT_FOUND                        = 0x8002300e,
	SCE_NP_COMMERCE2_ERROR_REQID_NOT_AVAILABLE                  = 0x8002300f,
	SCE_NP_COMMERCE2_ERROR_ABORTED                              = 0x80023010,
	SCE_NP_COMMERCE2_ERROR_RESPONSE_BUF_TOO_SMALL               = 0x80023012,
	SCE_NP_COMMERCE2_ERROR_COULD_NOT_RECV_WHOLE_RESPONSE_DATA   = 0x80023013,
	SCE_NP_COMMERCE2_ERROR_INVALID_RESULT_DATA                  = 0x80023014,
	SCE_NP_COMMERCE2_ERROR_UNKNOWN                              = 0x80023015,
	SCE_NP_COMMERCE2_ERROR_SERVER_MAINTENANCE                   = 0x80023016,
	SCE_NP_COMMERCE2_ERROR_SERVER_UNKNOWN                       = 0x80023017,
	SCE_NP_COMMERCE2_ERROR_INSUFFICIENT_BUF_SIZE                = 0x80023018,
	SCE_NP_COMMERCE2_ERROR_REQ_MAX                              = 0x80023019,
	SCE_NP_COMMERCE2_ERROR_INVALID_TARGET_TYPE                  = 0x8002301a,
	SCE_NP_COMMERCE2_ERROR_INVALID_TARGET_ID                    = 0x8002301b,
	SCE_NP_COMMERCE2_ERROR_INVALID_SIZE                         = 0x8002301c,
	SCE_NP_COMMERCE2_ERROR_DATA_NOT_FOUND                       = 0x80023087,
	SCE_NP_COMMERCE2_SERVER_ERROR_BAD_REQUEST                   = 0x80023101,
	SCE_NP_COMMERCE2_SERVER_ERROR_UNKNOWN_ERROR                 = 0x80023102,
	SCE_NP_COMMERCE2_SERVER_ERROR_SESSION_EXPIRED               = 0x80023105,
	SCE_NP_COMMERCE2_SERVER_ERROR_ACCESS_PERMISSION_DENIED      = 0x80023107,
	SCE_NP_COMMERCE2_SERVER_ERROR_NO_SUCH_CATEGORY              = 0x80023110,
	SCE_NP_COMMERCE2_SERVER_ERROR_NO_SUCH_PRODUCT               = 0x80023111,
	SCE_NP_COMMERCE2_SERVER_ERROR_NOT_ELIGIBILITY               = 0x80023113,
	SCE_NP_COMMERCE2_SERVER_ERROR_INVALID_SKU                   = 0x8002311a,
	SCE_NP_COMMERCE2_SERVER_ERROR_ACCOUNT_SUSPENDED1            = 0x8002311b,
	SCE_NP_COMMERCE2_SERVER_ERROR_ACCOUNT_SUSPENDED2            = 0x8002311c,
	SCE_NP_COMMERCE2_SERVER_ERROR_OVER_SPENDING_LIMIT           = 0x80023120,
	SCE_NP_COMMERCE2_SERVER_ERROR_INVALID_VOUCHER               = 0x8002312f,
	SCE_NP_COMMERCE2_SERVER_ERROR_VOUCHER_ALREADY_CONSUMED      = 0x80023130,
	SCE_NP_COMMERCE2_SERVER_ERROR_EXCEEDS_AGE_LIMIT_IN_BROWSING = 0x80023139,
	SCE_NP_COMMERCE2_SYSTEM_UTIL_ERROR_INVALID_VOUCHER          = 0x80024002,
};

// Event types
enum
{
	SCE_NP_COMMERCE2_EVENT_REQUEST_ERROR            = 0x0001,
	SCE_NP_COMMERCE2_EVENT_CREATE_SESSION_DONE      = 0x0011,
	SCE_NP_COMMERCE2_EVENT_CREATE_SESSION_ABORT     = 0x0012,
	SCE_NP_COMMERCE2_EVENT_DO_CHECKOUT_STARTED      = 0x0021,
	SCE_NP_COMMERCE2_EVENT_DO_CHECKOUT_SUCCESS      = 0x0022,
	SCE_NP_COMMERCE2_EVENT_DO_CHECKOUT_BACK         = 0x0023,
	SCE_NP_COMMERCE2_EVENT_DO_CHECKOUT_FINISHED     = 0x0024,
	SCE_NP_COMMERCE2_EVENT_DO_DL_LIST_STARTED       = 0x0031,
	SCE_NP_COMMERCE2_EVENT_DO_DL_LIST_SUCCESS       = 0x0032,
	SCE_NP_COMMERCE2_EVENT_DO_DL_LIST_FINISHED      = 0x0034,
	SCE_NP_COMMERCE2_EVENT_DO_PROD_BROWSE_STARTED   = 0x0041,
	SCE_NP_COMMERCE2_EVENT_DO_PROD_BROWSE_SUCCESS   = 0x0042,
	SCE_NP_COMMERCE2_EVENT_DO_PROD_BROWSE_BACK      = 0x0043,
	SCE_NP_COMMERCE2_EVENT_DO_PROD_BROWSE_FINISHED  = 0x0044,
	SCE_NP_COMMERCE2_EVENT_DO_PROD_BROWSE_OPENED    = 0x0045,
	SCE_NP_COMMERCE2_EVENT_DO_PRODUCT_CODE_STARTED  = 0x0051,
	SCE_NP_COMMERCE2_EVENT_DO_PRODUCT_CODE_SUCCESS  = 0x0052,
	SCE_NP_COMMERCE2_EVENT_DO_PRODUCT_CODE_BACK     = 0x0053,
	SCE_NP_COMMERCE2_EVENT_DO_PRODUCT_CODE_FINISHED = 0x0054,
	SCE_NP_COMMERCE2_EVENT_EMPTY_STORE_CHECK_DONE   = 0x0061,
	SCE_NP_COMMERCE2_EVENT_EMPTY_STORE_CHECK_ABORT  = 0x0062,
	SCE_NP_COMMERCE2_EVENT_RESERVED01_STARTED       = 0x0071,
	SCE_NP_COMMERCE2_EVENT_RESERVED01_SUCCESS       = 0x0072,
	SCE_NP_COMMERCE2_EVENT_RESERVED01_BACK          = 0x0073,
	SCE_NP_COMMERCE2_EVENT_RESERVED01_FINISHED      = 0x0074,
};

// Category data type
enum SceNpCommerce2CategoryDataType
{
	SCE_NP_COMMERCE2_CAT_DATA_TYPE_THIN = 0,
	SCE_NP_COMMERCE2_CAT_DATA_TYPE_NORMAL,
	SCE_NP_COMMERCE2_CAT_DATA_TYPE_MAX
};

// Game product data type
enum SceNpCommerce2GameProductDataType
{
	SCE_NP_COMMERCE2_GAME_PRODUCT_DATA_TYPE_THIN = 0,
	SCE_NP_COMMERCE2_GAME_PRODUCT_DATA_TYPE_NORMAL,
	SCE_NP_COMMERCE2_GAME_PRODUCT_DATA_TYPE_MAX
};

// SKU data type
enum SceNpCommerce2GameSkuDataType
{
	SCE_NP_COMMERCE2_GAME_SKU_DATA_TYPE_THIN = 0,
	SCE_NP_COMMERCE2_GAME_SKU_DATA_TYPE_NORMAL,
	SCE_NP_COMMERCE2_GAME_SKU_DATA_TYPE_MAX
};

// Store stuff
enum
{
	SCE_NP_COMMERCE2_STORE_IS_NOT_EMPTY = 0,
	SCE_NP_COMMERCE2_STORE_IS_EMPTY = 1,

	SCE_NP_COMMERCE2_STORE_CHECK_TYPE_CATEGORY = 1,

	SCE_NP_COMMERCE2_STORE_BROWSE_TYPE_CATEGORY     = 1,
	SCE_NP_COMMERCE2_STORE_BROWSE_TYPE_PRODUCT      = 2,
	SCE_NP_COMMERCE2_STORE_BROWSE_TYPE_PRODUCT_CODE = 3,
};

// Content Stuff
enum
{
	SCE_NP_COMMERCE2_CONTENT_TYPE_CATEGORY = 1,
	SCE_NP_COMMERCE2_CONTENT_TYPE_PRODUCT  = 2,

	SCE_NP_COMMERCE2_CONTENT_RATING_DESC_TYPE_ICON = 1,
	SCE_NP_COMMERCE2_CONTENT_RATING_DESC_TYPE_TEXT = 2,
};

// Game SKU
enum
{
	SCE_NP_COMMERCE2_SKU_CHECKOUT_MAX = 16,
	SCE_NP_COMMERCE2_SKU_DL_LIST_MAX  = 16,

	SCE_NP_COMMERCE2_SKU_PURCHASABILITY_FLAG_ON  = 1,
	SCE_NP_COMMERCE2_SKU_PURCHASABILITY_FLAG_OFF = 0,

	SCE_NP_COMMERCE2_SKU_ANN_PURCHASED_CANNOT_PURCHASE_AGAIN = 0x80000000,
	SCE_NP_COMMERCE2_SKU_ANN_PURCHASED_CAN_PURCHASE_AGAIN    = 0x40000000,
	SCE_NP_COMMERCE2_SKU_ANN_IN_THE_CART                     = 0x20000000,
	SCE_NP_COMMERCE2_SKU_ANN_CONTENTLINK_SKU                 = 0x10000000,
	SCE_NP_COMMERCE2_SKU_ANN_CREDIT_CARD_REQUIRED            = 0x08000000,
	SCE_NP_COMMERCE2_SKU_ANN_CHARGE_IMMEDIATELY              = 0x04000000,
};

// Constants for commerce functions and structures
enum
{
	SCE_NP_COMMERCE2_VERSION                                = 2,
	SCE_NP_COMMERCE2_CTX_MAX                                = 1,
	SCE_NP_COMMERCE2_REQ_MAX                                = 1,
	SCE_NP_COMMERCE2_CURRENCY_CODE_LEN                      = 3,
	SCE_NP_COMMERCE2_CURRENCY_SYMBOL_LEN                    = 3,
	SCE_NP_COMMERCE2_THOUSAND_SEPARATOR_LEN                 = 4,
	SCE_NP_COMMERCE2_DECIMAL_LETTER_LEN                     = 4,
	SCE_NP_COMMERCE2_SP_NAME_LEN                            = 256,
	SCE_NP_COMMERCE2_CATEGORY_ID_LEN                        = 56,
	SCE_NP_COMMERCE2_CATEGORY_NAME_LEN                      = 256,
	SCE_NP_COMMERCE2_CATEGORY_DESCRIPTION_LEN               = 1024,
	SCE_NP_COMMERCE2_PRODUCT_ID_LEN                         = 48,
	SCE_NP_COMMERCE2_PRODUCT_NAME_LEN                       = 256,
	SCE_NP_COMMERCE2_PRODUCT_SHORT_DESCRIPTION_LEN          = 1024,
	SCE_NP_COMMERCE2_PRODUCT_LONG_DESCRIPTION_LEN           = 4000,
	SCE_NP_COMMERCE2_SKU_ID_LEN                             = 56,
	SCE_NP_COMMERCE2_SKU_NAME_LEN                           = 180,
	SCE_NP_COMMERCE2_URL_LEN                                = 256,
	SCE_NP_COMMERCE2_RATING_SYSTEM_ID_LEN                   = 16,
	SCE_NP_COMMERCE2_RATING_DESCRIPTION_LEN                 = 60,
	SCE_NP_COMMERCE2_RECV_BUF_SIZE                          = 262144,
	SCE_NP_COMMERCE2_PRODUCT_CODE_BLOCK_LEN                 = 4,
	SCE_NP_COMMERCE2_PRODUCT_CODE_INPUT_MODE_USER_INPUT     = 0,
	SCE_NP_COMMERCE2_PRODUCT_CODE_INPUT_MODE_CODE_SPECIFIED = 1,
	SCE_NP_COMMERCE2_GETCAT_MAX_COUNT                       = 60,
	SCE_NP_COMMERCE2_GETPRODLIST_MAX_COUNT                  = 60,
	SCE_NP_COMMERCE2_DO_CHECKOUT_MEMORY_CONTAINER_SIZE      = 10485760,
	SCE_NP_COMMERCE2_DO_PROD_BROWSE_MEMORY_CONTAINER_SIZE   = 16777216,
	SCE_NP_COMMERCE2_DO_DL_LIST_MEMORY_CONTAINER_SIZE       = 10485760,
	SCE_NP_COMMERCE2_DO_PRODUCT_CODE_MEMORY_CONTAINER_SIZE  = 16777216,
	SCE_NP_COMMERCE2_SYM_POS_PRE                            = 0,
	SCE_NP_COMMERCE2_SYM_POS_POST                           = 1,
};

// Common structure used when receiving data
struct SceNpCommerce2CommonData
{
	be_t<u32> version;
	be_t<u32> buf_head;
	be_t<u32> buf_size;
	be_t<u32> data;
	be_t<u32> data_size;
	be_t<u32> data2;
	be_t<u32> reserved[4];
};

// Structure indicating the range of results obtained
struct SceNpCommerce2Range
{
	be_t<u32> startPosition;
	be_t<u32> count;
	be_t<u32> totalCountOfResults;
};

// Structure for session information
struct SceNpCommerce2SessionInfo
{
	s8 currencyCode[SCE_NP_COMMERCE2_CURRENCY_CODE_LEN + 1];
	be_t<u32> decimals;
	s8 currencySymbol[SCE_NP_COMMERCE2_CURRENCY_SYMBOL_LEN + 1];
	be_t<u32> symbolPosition;
	b8 symbolWithSpace;
	u8 padding1[3];
	s8 thousandSeparator[SCE_NP_COMMERCE2_THOUSAND_SEPARATOR_LEN + 1];
	s8 decimalLetter[SCE_NP_COMMERCE2_DECIMAL_LETTER_LEN + 1];
	u8 padding2[1];
	be_t<u32> reserved[4];
};

// Structure for category information
struct SceNpCommerce2CategoryInfo
{
	SceNpCommerce2CommonData commonData;
	SceNpCommerce2CategoryDataType dataType;
	s8 categoryId;
	CellRtcTick releaseDate;
	s8 categoryName;
	s8 categoryDescription;
	s8 imageUrl;
	s8 spName;
	be_t<u32> countOfSubCategory;
	be_t<u32> countOfProduct;
};

// Structure for content information within the category
struct SceNpCommerce2ContentInfo
{
	SceNpCommerce2CommonData commonData;
	be_t<u32> contentType;
};

// Structure for initialized product data
struct SceNpCommerce2GetProductInfoResult
{
	SceNpCommerce2CommonData commonData;
};

// Structure for game product information
struct SceNpCommerce2GameProductInfo
{
	SceNpCommerce2CommonData commonData;
	SceNpCommerce2GameProductDataType dataType;
	s8 productId;
	be_t<u32> countOfSku;
	u8 padding[4];
	CellRtcTick releaseDate;
	s8 productName;
	s8 productShortDescription;
	s8 imageUrl;
	s8 spName;
	s8 productLongDescription;
	s8 legalDescription;
};

// Structure for initialized product info list
struct SceNpCommerce2GetProductInfoListResult
{
	SceNpCommerce2CommonData commonData;
	be_t<u32> countOfProductInfo;
};

// Structure for rating information
struct SceNpCommerce2ContentRatingInfo
{
	SceNpCommerce2CommonData commonData;
	s8 ratingSystemId;
	s8 imageUrl;
	be_t<u32> countOfContentRatingDescriptor;
};

// Structure for a rating descriptor
struct SceNpCommerce2ContentRatingDescriptor
{
	SceNpCommerce2CommonData commonData;
	be_t<u32> descriptorType;
	s8 imageUrl;
	s8 contentRatingDescription;
};

// Structure for SKU information
struct SceNpCommerce2GameSkuInfo
{
	SceNpCommerce2CommonData commonData;
	SceNpCommerce2GameSkuDataType dataType;
	s8 skuId;
	be_t<u32> skuType;
	be_t<u32> countUntilExpiration;
	be_t<u32> timeUntilExpiration;
	be_t<u32> purchasabilityFlag;
	be_t<u32> annotation;
	b8 downloadable;
	u8 padding[3];
	be_t<u32> price;
	s8 skuName;
	s8 productId;
	s8 contentLinkUrl;
	be_t<u32> countOfRewardInfo;
	be_t<u32> reserved[8];
};

// Structure of parameters for in-game product browsing
struct SceNpCommerce2ProductBrowseParam
{
	be_t<u32> size;
};

// Structure of parameters for promotion code input
struct SceNpCommerce2ProductCodeParam
{
	be_t<u32> size;
	be_t<u32> inputMode;
	s8 code1[SCE_NP_COMMERCE2_PRODUCT_CODE_BLOCK_LEN + 1];
	s8 padding1[3];
	s8 code2[SCE_NP_COMMERCE2_PRODUCT_CODE_BLOCK_LEN + 1];
	s8 padding2[3];
	s8 code3[SCE_NP_COMMERCE2_PRODUCT_CODE_BLOCK_LEN + 1];
	s8 padding3[3];
};

struct SceNpCommerce2GetCategoryContentsResult
{
	SceNpCommerce2CommonData commonData;
	SceNpCommerce2Range rangeOfContents;
};

using SceNpCommerce2Handler = void(u32 ctx_id, u32 subject_id, s32 event, s32 error_code, vm::ptr<void> arg);
