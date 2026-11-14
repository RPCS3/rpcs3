#pragma once

#include "Emu/Cell/Modules/sceNpCommerce2.h"
#include "Emu/Cell/Modules/sceNp.h"
#include "Emu/NP/cmrc2_products.h"

// Arbitrary reasonable maximum for Flexible Array Members.
// Usually, Product:SKU is 1:1 but (rarely) can be more
inline constexpr usz MAX_SERIALISED_SKUS = 6;

static const u32 SCE_NP_COMMERCE2_LEGAL_DESC_LEN = 52;
static const char LEGAL_TEXT[SCE_NP_COMMERCE2_LEGAL_DESC_LEN] = "No file content is distributed/downloaded by RPCS3.";

namespace commerce2
{
	std::vector<u8> serialise_product_info_normal(const product_data& product);
	std::vector<u8> serialise_product_list(const std::vector<product_data>& products, u16 max_product_count);
	std::vector<u8> serialise_category_contents(const category_data& category, const std::vector<product_data>& products, u16 list_start_pos, u16 max_product_count);

	std::string format_currency_str(const SceNpCommerce2SessionInfo& session);
	std::string get_service_id_from_entitlement_id(const std::string& entitlement_id);

	// Serialisable structs
	struct s_rating_info
	{
		char rating_sys_id[5] = "ESRB";
		char rating_sys_image_url[48] = "https://rpcs3.net/cdn/netplay/DefaultAvatar.png";
		char rating_desc_text[15] = "E for Everyone";
	};

	struct s_sku_info
	{
		char sku_id[SCE_NP_COMMERCE2_SKU_ID_LEN + 1];
		char sku_name[SCE_NP_COMMERCE2_SKU_NAME_LEN + 1];
		char content_link = '\0';	// Necessary to prevent crashing in some cases
	};

	struct s_product_info_normal
	{
		char product_id[SCE_NP_COMMERCE2_PRODUCT_ID_LEN + 1];
		char product_name[SCE_NP_COMMERCE2_PRODUCT_NAME_LEN + 1];
		char product_desc_short[SCE_NP_COMMERCE2_PRODUCT_SHORT_DESCRIPTION_LEN + 1];
		char product_desc_long[SCE_NP_COMMERCE2_PRODUCT_LONG_DESCRIPTION_LEN + 1];
		char image_url[SCE_NP_COMMERCE_PRODUCT_IMAGEURL_LEN + 1];
		char sp_name[SCE_NP_COMMERCE2_SP_NAME_LEN + 1];
		char legal_desc[SCE_NP_COMMERCE2_LEGAL_DESC_LEN + 1];
		s_rating_info rating_info;
		be_t<u64> release_date;
		be_t<u32> sku_count;
		s_sku_info skus[MAX_SERIALISED_SKUS];
	};

	struct s_product_info_thin
	{
		char product_id[SCE_NP_COMMERCE2_PRODUCT_ID_LEN + 1];
		char product_name[SCE_NP_COMMERCE2_PRODUCT_NAME_LEN + 1];
		char product_desc_short[SCE_NP_COMMERCE2_PRODUCT_SHORT_DESCRIPTION_LEN + 1];
		char image_url[SCE_NP_COMMERCE_PRODUCT_IMAGEURL_LEN + 1];
		char sp_name[SCE_NP_COMMERCE2_SP_NAME_LEN + 1];
		be_t<u64> release_date;
		be_t<u32> sku_count;
		s_sku_info skus[MAX_SERIALISED_SKUS];
	};

	struct s_product_list
	{
		char legal_desc[SCE_NP_COMMERCE2_LEGAL_DESC_LEN + 1];
		s_rating_info rating_info;
		be_t<u32> product_count;
		s_product_info_thin products[];
	};

	struct s_category_contents
	{
		char category_id[SCE_NP_COMMERCE2_CATEGORY_ID_LEN + 1];
		char category_name[SCE_NP_COMMERCE2_CATEGORY_NAME_LEN + 1];
		char category_desc[SCE_NP_COMMERCE2_CATEGORY_DESCRIPTION_LEN + 1];
		char image_url[SCE_NP_COMMERCE_CATEGORY_IMAGEURL_LEN + 1];
		char sp_name[SCE_NP_COMMERCE2_SP_NAME_LEN + 1];
		char legal_desc[SCE_NP_COMMERCE2_LEGAL_DESC_LEN + 1];
		s_rating_info rating_info;
		be_t<u64> release_date;
		be_t<u32> total_products;	// Total count of all products in category
		be_t<u32> list_start_pos;
		be_t<u32> product_count;	// Size of products[]
		s_product_info_thin products[];
	};
}