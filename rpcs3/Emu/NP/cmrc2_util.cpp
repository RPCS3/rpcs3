#include "stdafx.h"
#include "Emu/NP/cmrc2_util.h"

namespace commerce2
{
	static s_sku_info serialise_sku(const sku_data& sku)
	{
		s_sku_info serialised_sku{};
		std::memcpy(&serialised_sku.sku_id, sku.id.c_str(), sku.id.size());
		std::memcpy(&serialised_sku.sku_name, sku.name.c_str(), sku.name.size());
		return serialised_sku;
	}

	static s_product_info_thin serialise_product_info_thin(const product_data& product)
	{
		s_product_info_thin serialised_product{};

		std::memcpy(&serialised_product.product_id, product.product_id.c_str(), product.product_id.size());
		std::memcpy(&serialised_product.product_name, product.name.c_str(), product.name.size());
		std::memcpy(&serialised_product.product_desc_short, product.description_short.c_str(), product.description_short.size());
		std::memcpy(&serialised_product.image_url, product.image_uri.c_str(), product.image_uri.size());
		std::memcpy(&serialised_product.sp_name, product.sp_name.c_str(), product.sp_name.size());

		serialised_product.release_date = product.release_date;
		serialised_product.sku_count = product.skus.size();

		for (usz i = 0; i < std::min(product.skus.size(), MAX_SERIALISED_SKUS); ++i)
		{
			serialised_product.skus[i] = serialise_sku(product.skus[i]);
		}

		return serialised_product;
	}

	std::vector<u8> serialise_product_info_normal(const product_data& product)
	{
		u32 total_size = sizeof(s_product_info_normal);

		std::vector<u8> buffer(total_size);
		s_product_info_normal* serialised_product = reinterpret_cast<s_product_info_normal*>(buffer.data());

		std::memcpy(&serialised_product->product_id, product.product_id.c_str(), product.product_id.size());
		std::memcpy(&serialised_product->product_name, product.name.c_str(), product.name.size());
		std::memcpy(&serialised_product->product_desc_short, product.description_short.c_str(), product.description_short.size());
		std::memcpy(&serialised_product->product_desc_long, product.description_long.c_str(), product.description_long.size());
		std::memcpy(&serialised_product->image_url, product.image_uri.c_str(), product.image_uri.size());
		std::memcpy(&serialised_product->sp_name, product.sp_name.c_str(), product.sp_name.size());
		std::memcpy(&serialised_product->legal_desc, LEGAL_TEXT, SCE_NP_COMMERCE2_LEGAL_DESC_LEN);

		serialised_product->rating_info = {};
		serialised_product->release_date = product.release_date;
		serialised_product->sku_count = std::min(static_cast<u32>(product.skus.size()), static_cast<u32>(MAX_SERIALISED_SKUS));

		for (size_t i = 0; i < serialised_product->sku_count; ++i)
		{
			serialised_product->skus[i] = serialise_sku(product.skus[i]);
		}

		return buffer;
	}

	std::vector<u8> serialise_product_list(const std::vector<product_data>& products, u16 max_product_count)
	{
		u16 product_count = 0;
		if (max_product_count > 0)
		{
			product_count = std::min({static_cast<u16>(products.size()), max_product_count, static_cast<u16>(SCE_NP_COMMERCE2_GETPRODLIST_MAX_COUNT)});
		}

		u32 products_size = product_count * sizeof(s_product_info_normal);
		u32 total_size = sizeof(be_t<u32>) + products_size;

		std::vector<u8> buffer(total_size);
		s_product_list* serialised_products = reinterpret_cast<s_product_list*>(buffer.data());

		std::memcpy(&serialised_products->legal_desc, LEGAL_TEXT, SCE_NP_COMMERCE2_LEGAL_DESC_LEN);
		serialised_products->rating_info = {};
		serialised_products->product_count = product_count;

		for (u32 i = 0; i < product_count; ++i)
		{
			serialised_products->products[i] = serialise_product_info_thin(products[i]);
		}

		return buffer;
	}

	std::vector<u8> serialise_category_contents(const category_data& category, const std::vector<product_data>& products, u16 list_start_pos, u16 max_product_count)
	{
		u16 product_count = 0;
		if (max_product_count > 0 && list_start_pos < products.size())
		{
			u16 available = products.size() - list_start_pos;
			product_count = std::min({available, max_product_count, static_cast<u16>(SCE_NP_COMMERCE2_GETCAT_MAX_COUNT)});
		}

		u32 products_size = product_count * sizeof(s_product_info_thin);
		u32 total_size = sizeof(s_category_contents) + products_size;

		std::vector<u8> buffer(total_size);
		s_category_contents* serialised_category = reinterpret_cast<s_category_contents*>(buffer.data());

		std::memcpy(&serialised_category->category_id, category.category_id.c_str(), category.category_id.size());
		std::memcpy(&serialised_category->category_name, category.name.c_str(), category.name.size());
		std::memcpy(&serialised_category->category_desc, category.description.c_str(), category.description.size());
		std::memcpy(&serialised_category->image_url, category.image_uri.c_str(), category.image_uri.size());
		std::memcpy(&serialised_category->sp_name, category.sp_name.c_str(), category.sp_name.size());
		std::memcpy(&serialised_category->legal_desc, LEGAL_TEXT, SCE_NP_COMMERCE2_LEGAL_DESC_LEN);

		serialised_category->rating_info = {};
		serialised_category->release_date = category.release_date;
		serialised_category->total_products = products.size();
		serialised_category->list_start_pos = list_start_pos;
		serialised_category->product_count = product_count;

		for (u32 i = 0; i < product_count; ++i)
		{
			serialised_category->products[i] = serialise_product_info_thin(products[list_start_pos + i]);
		}

		return buffer;
	}

	std::string format_currency_str(const SceNpCommerce2SessionInfo& session)
	{
		// Price is always 0
		std::string amount = "0";

		if (session.decimals > 0)
		{
			amount += session.decimalLetter[0];
			amount += std::string(session.decimals, '0');
		}

		std::string symbol(reinterpret_cast<const char*>(session.currencySymbol));
		std::string space = session.symbolWithSpace ? " " : "";

		std::string result;
		if (session.symbolPosition == SCE_NP_COMMERCE2_SYM_POS_PRE)
			result = symbol + space + amount;
		else
			result = amount + space + symbol;

		return result;
	}

	std::string get_service_id_from_entitlement_id(const std::string& entitlement_id)
	{
		const usz delim_pos = entitlement_id.rfind('-');

		if (delim_pos == std::string::npos)
		{
			return {};
		}

		return entitlement_id.substr(0, delim_pos);
	}
}