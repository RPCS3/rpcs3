#include "stdafx.h"
#include "Emu/NP/cmrc2_products.h"
#include "Emu/NP/cmrc2_util.h"
#include "Emu/NP/np_contexts.h"
#include "Emu/System.h"
#include "Emu/system_utils.hpp"

#include "Utilities/File.h"
#include "util/yaml.hpp"
#include "util/logs.hpp"

LOG_CHANNEL(cmrc_products, "cmrc2_products");

namespace commerce2
{
	product_manager::~product_manager()
	{
		this->unload_products();
	}

	void product_manager::unload_products()
	{
		products.clear();
		categories.clear();
		sku_index.clear();
	}

	void product_manager::load_products(const std::string title_id)
	{
		if (title_id.empty())
		{
			cmrc_products.error("Cannot load commerce products; no title ID provided");
			return;
		}

		std::string path = rpcs3::utils::get_commerce_dir_by_serial(title_id) + "/product_db.yml";

		fs::file products_file{path, fs::read + fs::create};
		if (!products_file)
		{
			cmrc_products.error("Failed to acquire file %s", path);
			return;
		}

		auto [root, error] = yaml_load(products_file.to_string());

		if (!error.empty())
		{
			cmrc_products.error("Error parsing %s: %s", path, error);
			return;
		}

		// Version checking
		std::string version;
		if (const auto version_node = root[product_key::version])
		{
			version = version_node.Scalar();

			if (version != products_db_engine_version)
			{
				cmrc_products.error("File version %s does not match products DB engine target version %s (location: %s, file: %s)",
					version, products_db_engine_version, get_yaml_node_location(version_node), path);
				return;
			}

			// We don't need the Version node in local memory anymore
			root.remove(product_key::version);
		}
		else
		{
			cmrc_products.error("No '%s' entry found. Products DB engine version = %s (file: %s)", product_key::version, products_db_engine_version, path);
			return;
		}

		// Go through each category in file
		for (auto pair : root)
		{
			// Get Category ID
			std::string err;
			const std::string& cat_id_str = pair.first.Scalar();

			if (cat_id_str.empty())
			{
				cmrc_products.error("Skipping empty category (location: %s, file: %s)",
					get_yaml_node_location(pair.first), path);
				continue;
			}

			if (const auto yml_type = pair.second.Type(); yml_type != YAML::NodeType::Map)
			{
				cmrc_products.error("Skipping category %s: expected Map, found %s (location: %s, file: %s)",
					cat_id_str, yml_type, get_yaml_node_location(pair.second), path);
				continue;
			}

			// Get 'Name' field (Optional - defaults to ID)
			std::string name = std::string(cat_id_str);
			const auto name_field = pair.second[category_key::name];
			if (name_field && name_field.IsScalar())
			{
				name = name_field.Scalar();
			}

			// Get 'Description' field (Optional)
			std::string description = "-";
			const auto descr_field = pair.second[category_key::description];
			if (descr_field && descr_field.IsScalar())
			{
				description = descr_field.Scalar();
			}

			// Get 'Image' field (Optional)
			std::string image = "-";
			const auto image_field = pair.second[category_key::image];
			if (image_field && image_field.IsScalar())
			{
				image = image_field.Scalar();
			}

			// Get 'SP Name' field (Optional)
			std::string sp_name = "RPCS3";
			const auto spname_field = pair.second[category_key::sp_name];
			if (spname_field && spname_field.IsScalar())
			{
				sp_name = spname_field.Scalar();
			}

			// Get 'Release Date' field (Optional)
			u64 release_date = 0;
			const auto released_field = pair.second[category_key::release];
			if (released_field)
			{
				u64 parsed_release_date = get_yaml_node_value<u64>(released_field, err);
				if (err.empty())
				{
					release_date = parsed_release_date;
				}
				else
				{
					cmrc_products.error("Category %s: Invalid value for 'Release Date' field, expected 64-bit timestamp but got '%s' (location: %s, file: %s)",
						cat_id_str, released_field.Scalar(), get_yaml_node_location(released_field), path);
				}
			}

			// Get 'Products' field
			const auto products_field = pair.second[category_key::products];
			if (!products_field || !products_field.IsDefined())
			{
				cmrc_products.error("Skipping category %s: Missing 'Products' field (location: %s, file: %s)",
					cat_id_str, get_yaml_node_location(pair.second), path);
				continue;
			}

			if (const auto yml_type = products_field.Type(); yml_type != YAML::NodeType::Sequence)
			{
				cmrc_products.error("Skipping category %s: expected Sequence for 'Products' field, found %s (location: %s, file: %s)",
					cat_id_str, yml_type, get_yaml_node_location(products_field), path);
				continue;
			}

			// Go through each Product in Category
			std::vector<std::string> cat_product_ids;
			for (const YAML::Node& product : products_field)
			{
				if (const auto yml_type = product.Type(); yml_type != YAML::NodeType::Map)
				{
					cmrc_products.error("Skipping product in category %s: expected Map, found %s (location: %s, file: %s)",
						cat_id_str, yml_type, get_yaml_node_location(product), path);
					continue;
				}

				// Get 'ID' field
				std::string product_id;
				const auto prod_id_field = product[product_key::id];
				if (prod_id_field && prod_id_field.IsScalar())
				{
					product_id = prod_id_field.Scalar();
				}
				else
				{
					cmrc_products.error("Skipping product in category %s: Missing or invalid 'ID' field (location: %s, file: %s)",
						cat_id_str, get_yaml_node_location(product), path);
					continue;
				}

				// Exit early if total products in category >= UINT16_MAX
				// - The firmware is unable to handle more than this
				if (cat_product_ids.size() >= UINT16_MAX)
				{
					cmrc_products.error("Skipping product %s (and any after) in category %s: More than %u products in one category disallowed (location: %s, file: %s)",
						product_id, cat_id_str, UINT16_MAX, get_yaml_node_location(product), path);
					break;
				}

				// Get 'Name' field (Optional - defaults to ID)
				std::string prod_name = std::string(product_id);
				const auto prod_name_field = product[product_key::name];
				if (prod_name_field && prod_name_field.IsScalar())
				{
					prod_name = prod_name_field.Scalar();
				}

				// Get 'Description Short' field (Optional)
				std::string prod_desc_short = "-";
				const auto prod_desc_short_field = product[product_key::desc_short];
				if (prod_desc_short_field && prod_desc_short_field.IsScalar())
				{
					prod_desc_short = prod_desc_short_field.Scalar();
				}

				// Get 'Description Long' field (Optional)
				std::string prod_desc_long = "-";
				const auto prod_desc_long_field = product[product_key::desc_long];
				if (prod_desc_long_field && prod_desc_long_field.IsScalar())
				{
					prod_desc_long = prod_desc_long_field.Scalar();
				}

				// Get 'Image' field (Optional)
				std::string prod_image = "-";
				const auto prod_image_field = product[product_key::image];
				if (prod_image_field && prod_image_field.IsScalar())
				{
					prod_image = prod_image_field.Scalar();
				}

				// Get 'Release Date' field (Optional)
				u64 prod_release_date = 0;
				const auto prod_released_field = product[product_key::release];
				if (prod_released_field)
				{
					std::string err;
					u64 parsed_date = get_yaml_node_value<u64>(prod_released_field, err);
					if (err.empty())
					{
						prod_release_date = parsed_date;
					}
					else
					{
						cmrc_products.error("Product %s in category %s: Invalid value for 'Release Date' field, expected 64-bit timestamp but got '%s' (location: %s, file: %s)",
							prod_name, cat_id_str, prod_released_field.Scalar(), get_yaml_node_location(prod_released_field), path);
					}
				}

				// Get 'Consumable' field (Optional - defaults to False)
				bool is_consumable = false;
				const auto prod_consumable_field = product[product_key::consumable];
				if (prod_consumable_field)
				{
					std::string err;
					bool parsed_consumable = get_yaml_node_value<bool>(prod_consumable_field, err);
					if (!err.empty() || (prod_consumable_field.Scalar() != "true" && prod_consumable_field.Scalar() != "false"))
					{
						cmrc_products.error("Product %s in category %s: Invalid value for 'Consumable' field, expected true/false but got '%s' (location: %s, file: %s)",
							prod_name, cat_id_str, prod_consumable_field.Scalar(), get_yaml_node_location(prod_consumable_field), path);
					}
					else
					{
						is_consumable = parsed_consumable;
					}
				}

				// Get 'Consumable Count' field (Optional)
				s32 consumable_count = -1;
				const auto prod_cons_count_field = product[product_key::cons_count];
				if (prod_cons_count_field)
				{
					std::string err;
					s32 parsed_cons_count = get_yaml_node_value<s32>(prod_cons_count_field, err);
					if (err.empty())
					{
						consumable_count = parsed_cons_count;
					}
					else
					{
						cmrc_products.error("Product %s in category %s: Invalid value for 'Consumable Count' field, expected 32-bit integer but got '%s' (location: %s, file: %s)",
							prod_name, cat_id_str, prod_cons_count_field.Scalar(), get_yaml_node_location(prod_cons_count_field), path);
					}
				}

				// Get 'SKUs' field
				std::vector<sku_data> skus;
				const auto skus_field = product[product_key::skus];
				if (!skus_field || !skus_field.IsDefined())
				{
					cmrc_products.error("Product %s in category %s: Missing or invalid 'SKUs' field (location: %s, file: %s)",
						prod_name, cat_id_str, get_yaml_node_location(skus_field), path);
					// Empty SKUs fine, don't need to skip product registration
				}
				else if (const auto yml_type = skus_field.Type(); yml_type != YAML::NodeType::Sequence)
				{
					cmrc_products.error("Product %s in category %s: expected Sequence for 'SKUs' field, found %s (location: %s, file: %s)",
						prod_name, cat_id_str, yml_type, get_yaml_node_location(skus_field), path);
					// Invalid SKUs fine, don't need to skip product registration
				}
				else
				{
					// Go through each SKU in Product
					for (const auto& sku : skus_field)
					{
						if (const auto sku_yml_type = sku.Type(); sku_yml_type != YAML::NodeType::Map)
						{
							cmrc_products.error("SKU in product %s: expected Map, found %s (location: %s, file: %s)",
								prod_name, sku_yml_type, get_yaml_node_location(sku), path);
							continue;
						}

						// Each SKU is a single key-value pair: ID -> Name
						for (const auto& sku_pair : sku)
						{
							std::string sku_id = sku_pair.first.Scalar();
							std::string sku_name = sku_pair.second.Scalar();

							if (sku_id.empty())
							{
								cmrc_products.error("Empty SKU ID in product %s (location: %s, file: %s)",
									prod_name, get_yaml_node_location(sku_pair.first), path);
								continue;
							}

							sku_data sku_obj{
								.id = sku_id,
								.name = sku_name};

							skus.push_back(sku_obj);

							// Update sku_index (for reverse-lookup)
							usz product_index = products.size();
							sku_index.emplace(sku_id, product_index);
						}
					}
				}

				auto cons_info = consumable_info{
					.is_consumable = is_consumable,
					.consumable_count = consumable_count
				};

				// Finalise product
				products.push_back(product_data{
					.product_id = product_id,
					.name = prod_name,
					.description_short = prod_desc_short,
					.description_long = prod_desc_long,
					.image_uri = prod_image,
					.sp_name = sp_name,
					.c_info = cons_info,
					.release_date = prod_release_date,
					.skus = skus});

				// Add product ID to category
				cat_product_ids.push_back(product_id);
			}

			// Finalise category
			categories.push_back(category_data{
				.category_id = cat_id_str,
				.name = name,
				.description = description,
				.image_uri = image,
				.sp_name = sp_name,
				.release_date = release_date,
				.product_ids = cat_product_ids});
		}

		cmrc_products.success("Loaded %u products from %u categories in file: %s", products.size(), categories.size(), path);
	}

	error_code product_manager::get_product_info(u32 req_id, const std::string_view category_id, const std::string_view product_id)
	{
		const auto& req_ctx = get_commerce2_lookup_request_context(req_id);

		if (!req_ctx)
			return SCE_NP_COMMERCE2_ERROR_REQ_NOT_FOUND;

		if (req_ctx->aborted)
			return SCE_NP_COMMERCE2_ERROR_ABORTED;

		const auto& category_data = get_category_data(category_id);
		if (!category_data.has_value())
		{
			req_ctx->result = SCE_NP_COMMERCE2_SERVER_ERROR_NO_SUCH_CATEGORY;
			return CELL_OK;
		}

		const auto& found_product_data = get_product_data(product_id);
		if (!found_product_data.has_value())
		{
			req_ctx->result = SCE_NP_COMMERCE2_SERVER_ERROR_NO_SUCH_PRODUCT;
			return CELL_OK;
		}

		req_ctx->result = CELL_OK;
		req_ctx->result_data = serialise_product_info_normal(found_product_data.value());
		
		return CELL_OK;
	}

	error_code product_manager::get_product_info_list(u32 req_id, vm::cpptr<char> product_ids, u16 product_count)
	{
		const auto& req_ctx = get_commerce2_lookup_request_context(req_id);

		if (!req_ctx)
			return SCE_NP_COMMERCE2_ERROR_REQ_NOT_FOUND;

		if (req_ctx->aborted)
			return SCE_NP_COMMERCE2_ERROR_ABORTED;

		std::vector<product_data> products;
		for (u32 i = 0; i < product_count; ++i)
		{
			const char* product_id = product_ids[i].get_ptr();
			if (!product_id)
				continue;

			const auto& product_data = get_product_data(product_id);
			if (product_data.has_value())
			{
				products.push_back(product_data.value());
			}
		}

		req_ctx->result = CELL_OK;
		req_ctx->result_data = serialise_product_list(products, product_count);

		return CELL_OK;
	}

	error_code product_manager::get_category_contents(u32 req_id, const std::string_view category_id, u16 list_start_pos, u16 max_product_count)
	{
		const auto& req_ctx = get_commerce2_lookup_request_context(req_id);

		if (!req_ctx)
			return SCE_NP_COMMERCE2_ERROR_REQ_NOT_FOUND;

		if (req_ctx->aborted)
			return SCE_NP_COMMERCE2_ERROR_ABORTED;

		const auto& found_category_data = get_category_data(category_id);
		if (!found_category_data.has_value())
		{
			req_ctx->result = SCE_NP_COMMERCE2_SERVER_ERROR_NO_SUCH_CATEGORY;
			return CELL_OK;
		}

		// Collect product data
		std::vector<product_data> products;
		for (const auto& product_id : found_category_data->product_ids)
		{
			const auto& product_data = get_product_data(product_id);
			if (product_data.has_value())
			{
				products.push_back(product_data.value());
			}
		}

		req_ctx->result = CELL_OK;
		req_ctx->result_data = serialise_category_contents(found_category_data.value(), products, list_start_pos, max_product_count);

		return CELL_OK;
	}

	consumable_info product_manager::get_product_consumable_info(const std::string_view product_id)
	{
		const auto& product_data = get_product_data(product_id);
		if (product_data.has_value())
		{
			return product_data->c_info;
		}

		return consumable_info{
			.is_consumable = false,
			.consumable_count = 1
		};
	}

	std::optional<product_data> product_manager::get_product_from_sku_id(std::string_view sku_id)
	{
		std::shared_lock lock(data_mutex);

		const auto it = sku_index.find(sku_id);
		if (it == sku_index.end())
		{
			cmrc_products.warning("No product data found with SKU ID: %s", std::string(sku_id).c_str());
			return {};
		}

		return products[it->second];
	}

	/* private */
	std::optional<product_data> product_manager::get_product_data(const std::string_view product_id)
	{
		std::shared_lock lock(data_mutex);

		const auto prod_it = std::find_if(products.begin(), products.end(),
			[product_id](const product_data& pdata)
			{
				return pdata.product_id == product_id;
			});

		if (prod_it != products.end())
		{
			return *prod_it;
		}

		cmrc_products.warning("No product data found with ID: %s", product_id);
		return {};
	}

	std::optional<category_data> product_manager::get_category_data(const std::string_view category_id)
	{
		std::shared_lock lock(data_mutex);

		const auto cat_it = std::find_if(categories.begin(), categories.end(),
			[category_id](const category_data& cdata)
			{
				return cdata.category_id == category_id;
			});

		if (cat_it != categories.end())
		{
			return *cat_it;
		}

		cmrc_products.warning("No category data found with ID: %s", category_id);
		return {};
	}

} // namespace commerce2