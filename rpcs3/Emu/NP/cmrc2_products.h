#pragma once

#include <shared_mutex>

#include "Emu/Cell/Modules/sceNpCommerce2.h"
#include "Emu/Cell/Modules/sceNp.h"
#include "Emu/NP/np_contexts.h"

namespace product_key
{
	static const std::string version = "Version";

	static const std::string id = "ID";
	static const std::string name = "Name";
	static const std::string image = "Image";
	static const std::string release = "Release Date";
	static const std::string desc_short = "Description Short";
	static const std::string desc_long = "Description Long";
	static const std::string consumable = "Consumable";
	static const std::string cons_count = "Consumable Count";
	static const std::string skus = "SKUs";
}

namespace category_key
{
	static const std::string name = "Name";
	static const std::string image = "Image";
	static const std::string release = "Release Date";
	static const std::string description = "Description";
	static const std::string products = "Products";
	static const std::string sp_name = "SP Name";
}

namespace sku_key
{
	static const std::string name = "Name";
}

inline static const std::string products_db_engine_version = "1.0";

namespace commerce2
{
	struct consumable_info
	{
		bool is_consumable;
		s32 consumable_count;
	};

	struct sku_data
	{
		const std::string id;
		const std::string name;
	};

	struct product_data
	{
		const std::string product_id;
		const std::string name;
		const std::string description_short;
		const std::string description_long;
		const std::string image_uri;
		const std::string sp_name; // Licensee (Author)
		consumable_info c_info;
		u64 release_date;
		std::vector<sku_data> skus;
	};

	struct category_data
	{
		std::string category_id;
		std::string name;
		std::string description;
		std::string image_uri;
		std::string sp_name; // Licensee (Author)
		u64 release_date;
		std::vector<std::string> product_ids;
	};

	class product_manager
	{
	public:
		product_manager() = default;
		~product_manager();

		void load_products(const std::string title_id);
		void unload_products();

		error_code get_product_info(u32 req_id, const std::string_view category_id, const std::string_view product_id);
		error_code get_product_info_list(u32 req_id, vm::cpptr<char> product_ids, u16 product_count);
		error_code get_category_contents(u32 req_id, const std::string_view category_id, u16 list_start_pos, u16 max_product_count);

		consumable_info get_product_consumable_info(const std::string_view product_id);
		std::optional<product_data> get_product_from_sku_id(const std::string_view sku_id);

	private:
		struct sku_hash
		{
			using is_transparent = void;
			usz operator()(std::string_view sku_id) const noexcept
			{
				return std::hash<std::string_view>{}(sku_id);
			}
		};

	private:
		std::vector<product_data> products;
		std::vector<category_data> categories;
		std::unordered_map<std::string, usz, sku_hash, std::equal_to<>> sku_index; // For get_product_from_sku_id()
		mutable std::shared_mutex data_mutex;

		std::optional<product_data> get_product_data(const std::string_view product_id);
		std::optional<category_data> get_category_data(const std::string_view category_id);
	};
}