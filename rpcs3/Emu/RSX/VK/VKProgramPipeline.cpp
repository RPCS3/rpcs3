#include "stdafx.h"
#include "VKProgramPipeline.h"
#include "VKResourceManager.h"
#include "vkutils/descriptors.h"
#include "vkutils/device.h"

#include "../Program/SPIRVCommon.h"

namespace vk
{
	extern const vk::render_device* get_current_renderer();

	namespace glsl
	{
		using namespace ::glsl;

		bool operator == (const descriptor_slot_t& a, const VkDescriptorImageInfo& b)
		{
			const auto ptr = std::get_if<VkDescriptorImageInfo>(&a);
			return !!ptr &&
				ptr->imageView == b.imageView &&
				ptr->sampler == b.sampler &&
				ptr->imageLayout == b.imageLayout;
		}

		bool operator == (const descriptor_slot_t& a, const VkDescriptorBufferInfo& b)
		{
			const auto ptr = std::get_if<VkDescriptorBufferInfo>(&a);
			return !!ptr &&
				ptr->buffer == b.buffer &&
				ptr->offset == b.offset &&
				ptr->range == b.range;
		}

		bool operator == (const descriptor_slot_t& a, const VkBufferView& b)
		{
			const auto ptr = std::get_if<VkBufferView>(&a);
			return !!ptr && *ptr == b;
		}

		VkDescriptorType to_descriptor_type(program_input_type type)
		{
			switch (type)
			{
			case input_type_uniform_buffer:
				return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			case input_type_texel_buffer:
				return VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
			case input_type_texture:
				return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			case input_type_storage_buffer:
				return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			case input_type_storage_texture:
				return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			default:
				fmt::throw_exception("Unexpected program input type %d", static_cast<int>(type));
			}
		}

		VkShaderStageFlags to_shader_stage_flags(::glsl::program_domain domain)
		{
			switch (domain)
			{
			case glsl_vertex_program:
				return VK_SHADER_STAGE_VERTEX_BIT;
			case glsl_fragment_program:
				return VK_SHADER_STAGE_FRAGMENT_BIT;
			case glsl_compute_program:
				return VK_SHADER_STAGE_COMPUTE_BIT;
			default:
				fmt::throw_exception("Unexpected domain %d", static_cast<int>(domain));
			}
		}

		const char* to_string(::glsl::program_domain domain)
		{
			switch (domain)
			{
			case glsl_vertex_program:
				return "vertex";
			case glsl_fragment_program:
				return "fragment";
			case glsl_compute_program:
				return "compute";
			default:
				fmt::throw_exception("Unexpected domain %d", static_cast<int>(domain));
			}
		}

		void shader::create(::glsl::program_domain domain, const std::string& source)
		{
			type     = domain;
			m_source = source;
		}

		VkShaderModule shader::compile()
		{
			ensure(m_handle == VK_NULL_HANDLE);

			if (!spirv::compile_glsl_to_spv(m_compiled, m_source, type, ::glsl::glsl_rules_vulkan))
			{
				rsx_log.notice("%s", m_source);
				fmt::throw_exception("Failed to compile %s shader", to_string(type));
			}

			VkShaderModuleCreateInfo vs_info;
			vs_info.codeSize = m_compiled.size() * sizeof(u32);
			vs_info.pNext    = nullptr;
			vs_info.sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			vs_info.pCode    = m_compiled.data();
			vs_info.flags    = 0;

			vkCreateShaderModule(*g_render_device, &vs_info, nullptr, &m_handle);

			return m_handle;
		}

		void shader::destroy()
		{
			m_source.clear();
			m_compiled.clear();

			if (m_handle)
			{
				vkDestroyShaderModule(*g_render_device, m_handle, nullptr);
				m_handle = nullptr;
			}
		}

		const std::string& shader::get_source() const
		{
			return m_source;
		}

		const std::vector<u32> shader::get_compiled() const
		{
			return m_compiled;
		}

		VkShaderModule shader::get_handle() const
		{
			return m_handle;
		}

		void program::init()
		{
			m_linked = false;
		}

		program::program(VkDevice dev, const VkGraphicsPipelineCreateInfo& create_info, const std::vector<program_input> &vertex_inputs, const std::vector<program_input>& fragment_inputs)
			: m_device(dev), m_info(create_info)
		{
			init();

			load_uniforms(vertex_inputs);
			load_uniforms(fragment_inputs);
		}

		program::program(VkDevice dev, const VkComputePipelineCreateInfo& create_info, const std::vector<program_input>& compute_inputs)
			: m_device(dev), m_info(create_info)
		{
			init();

			load_uniforms(compute_inputs);
		}

		program::~program()
		{
			vkDestroyPipeline(m_device, m_pipeline, nullptr);

			if (m_pipeline_layout)
			{
				vkDestroyPipelineLayout(m_device, m_pipeline_layout, nullptr);

				for (auto& set : m_sets)
				{
					set.destroy();
				}
			}
		}

		program& program::load_uniforms(const std::vector<program_input>& inputs)
		{
			ensure(!m_linked); // "Cannot change uniforms in already linked program!"

			for (auto &item : inputs)
			{
				ensure(item.set < binding_set_index_max_enum);                         // Ensure we have a valid set id
				ensure(item.location < 128u || item.type == input_type_push_constant); // Arbitrary limit but useful to catch possibly uninitialized values
				m_sets[item.set].m_inputs[item.type].push_back(item);
			}

			return *this;
		}

		program& program::link(bool separate_objects)
		{
			auto p_graphics_info = std::get_if<VkGraphicsPipelineCreateInfo>(&m_info);
			auto p_compute_info = !p_graphics_info ? std::get_if<VkComputePipelineCreateInfo>(&m_info) : nullptr;
			const bool is_graphics_pipe = p_graphics_info != nullptr;

			if (!is_graphics_pipe) [[ likely ]]
			{
				// We only support compute and graphics, so disable this for compute
				separate_objects = false;
			}

			if (!separate_objects)
			{
				// Collapse all sets into set 0 if validation passed
				auto& sink = m_sets[0];
				for (auto& set : m_sets)
				{
					if (&set == &sink)
					{
						continue;
					}

					for (auto& type_arr : set.m_inputs)
					{
						if (type_arr.empty())
						{
							continue;
						}

						auto type = type_arr.front().type;
						auto& dst = sink.m_inputs[type];
						dst.insert(dst.end(), type_arr.begin(), type_arr.end());

						// Clear
						type_arr.clear();
					}
				}

				sink.validate();
				sink.init(m_device);
			}
			else
			{
				for (auto& set : m_sets)
				{
					for (auto& type_arr : set.m_inputs)
					{
						if (type_arr.empty())
						{
							continue;
						}

						// Real set
						set.validate();
						set.init(m_device);
						break;
					}
				}
			}

			create_pipeline_layout();
			ensure(m_pipeline_layout);

			if (is_graphics_pipe)
			{
				VkGraphicsPipelineCreateInfo create_info = *p_graphics_info;
				create_info.layout = m_pipeline_layout;
				CHECK_RESULT(vkCreateGraphicsPipelines(m_device, nullptr, 1, &create_info, nullptr, &m_pipeline));
			}
			else
			{
				VkComputePipelineCreateInfo create_info = *p_compute_info;
				create_info.layout = m_pipeline_layout;
				CHECK_RESULT(vkCreateComputePipelines(m_device, nullptr, 1, &create_info, nullptr, &m_pipeline));
			}

			m_linked = true;
			return *this;
		}

		bool program::has_uniform(program_input_type type, const std::string& uniform_name)
		{
			for (auto& set : m_sets)
			{
				const auto& uniform = set.m_inputs[type];
				return std::any_of(uniform.cbegin(), uniform.cend(), [&uniform_name](const auto& u)
				{
					return u.name == uniform_name;
				});
			}

			return false;
		}

		std::pair<u32, u32> program::get_uniform_location(::glsl::program_domain domain, program_input_type type, const std::string& uniform_name)
		{
			for (unsigned i = 0; i < ::size32(m_sets); ++i)
			{
				const auto& type_arr = m_sets[i].m_inputs[type];
				const auto result = std::find_if(type_arr.cbegin(), type_arr.cend(), [&](const auto& u)
				{
					return u.domain == domain && u.name == uniform_name;
				});

				if (result != type_arr.end())
				{
					return { i, result->location };
				}
			}

			return { umax, umax };
		}

		void program::bind_uniform(const VkDescriptorImageInfo& image_descriptor, u32 set_id, u32 binding_point)
		{
			if (m_sets[set_id].m_descriptor_slots[binding_point] == image_descriptor)
			{
				return;
			}

			m_sets[set_id].notify_descriptor_slot_updated(binding_point, image_descriptor);
		}

		void program::bind_uniform(const VkDescriptorBufferInfo &buffer_descriptor, u32 set_id, u32 binding_point)
		{
			if (m_sets[set_id].m_descriptor_slots[binding_point] == buffer_descriptor)
			{
				return;
			}

			m_sets[set_id].notify_descriptor_slot_updated(binding_point, buffer_descriptor);
		}

		void program::bind_uniform(const VkBufferView &buffer_view, u32 set_id, u32 binding_point)
		{
			if (m_sets[set_id].m_descriptor_slots[binding_point] == buffer_view)
			{
				return;
			}

			m_sets[set_id].notify_descriptor_slot_updated(binding_point, buffer_view);
		}

		void program::bind_uniform_array(const VkDescriptorImageInfo* image_descriptors, VkDescriptorType type, int count, u32 set_id, u32 binding_point)
		{
			auto& set = m_sets[set_id];
			for (int i = 0; i < count; ++i)
			{
				if (set.m_descriptor_slots[binding_point + i] != image_descriptors[i])
				{
					set.notify_descriptor_slot_updated(binding_point + i, image_descriptors[i]);
				}
			}
		}

		void program::create_pipeline_layout()
		{
			ensure(!m_linked);
			ensure(m_pipeline_layout == VK_NULL_HANDLE);

			rsx::simple_array<VkPushConstantRange> push_constants{};
			rsx::simple_array<VkDescriptorSetLayout> set_layouts{};

			for (auto& set : m_sets)
			{
				if (!set.m_device)
				{
					break;
				}

				set.next_descriptor_set(); // Initializes the set layout and allocates first set
				set_layouts.push_back(set.m_descriptor_set_layout);

				for (const auto& input : set.m_inputs[input_type_push_constant])
				{
					const auto& range = input.as_push_constant();
					push_constants.push_back({
						.stageFlags = to_shader_stage_flags(input.domain),
						.offset = range.offset,
						.size = range.size
						});
				}
			}

			VkPipelineLayoutCreateInfo create_info
			{
				.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
				.flags = 0,
				.setLayoutCount = set_layouts.size(),
				.pSetLayouts = set_layouts.data(),
				.pushConstantRangeCount = push_constants.size(),
				.pPushConstantRanges = push_constants.data()
			};
			CHECK_RESULT(vkCreatePipelineLayout(m_device, &create_info, nullptr, &m_pipeline_layout));
		}

		program& program::bind(const vk::command_buffer& cmd, VkPipelineBindPoint bind_point)
		{
			VkDescriptorSet bind_sets[binding_set_index_max_enum];
			unsigned count = 0;

			for (auto& set : m_sets)
			{
				if (!set.m_device)
				{
					break;
				}

				bind_sets[count++] = set.m_descriptor_set.value();   // Current set pointer for binding
				set.m_descriptor_set.on_bind();                      // Notify async queue
				set.next_descriptor_set();                           // Flush queue and update pointers
			}

			vkCmdBindPipeline(cmd, bind_point, m_pipeline);
			vkCmdBindDescriptorSets(cmd, bind_point, m_pipeline_layout, 0, count, bind_sets, 0, nullptr);
			return *this;
		}

		void descriptor_table_t::destroy()
		{
			if (!m_device)
			{
				return;
			}

			vkDestroyDescriptorSetLayout(m_device, m_descriptor_set_layout, nullptr);
			vk::get_resource_manager()->dispose(m_descriptor_pool);
		}

		void descriptor_table_t::init(VkDevice dev)
		{
			m_device = dev;

			size_t bind_slots_count = 0;
			for (auto& type_arr : m_inputs)
			{
				if (type_arr.empty() || type_arr.front().type == input_type_push_constant)
				{
					continue;
				}

				bind_slots_count += type_arr.size();
			}

			m_descriptor_slots.resize(bind_slots_count);
			std::memset(m_descriptor_slots.data(), 0, sizeof(descriptor_slot_t) * bind_slots_count);

			m_descriptors_dirty.resize(bind_slots_count);
			std::fill(m_descriptors_dirty.begin(), m_descriptors_dirty.end(), false);
		}

		VkDescriptorSet descriptor_table_t::allocate_descriptor_set()
		{
			if (!m_descriptor_pool)
			{
				create_descriptor_set_layout();
				create_descriptor_pool();
			}

			return m_descriptor_pool->allocate(m_descriptor_set_layout);
		}

		void descriptor_table_t::next_descriptor_set()
		{
			if (!m_descriptor_set)
			{
				m_descriptor_set = allocate_descriptor_set();
				std::fill(m_descriptors_dirty.begin(), m_descriptors_dirty.end(), false);
				return;
			}

			// Check if we need to actually open a new set
			if (!m_any_descriptors_dirty)
			{
				return;
			}

			auto push_descriptor_slot = [this](unsigned idx)
			{
				const auto& slot = m_descriptor_slots[idx];
				const VkDescriptorType type = m_descriptor_types[idx];
				if (auto ptr = std::get_if<VkDescriptorImageInfo>(&slot))
				{
					m_descriptor_set.push(*ptr, type, idx);
					return;
				}

				if (auto ptr = std::get_if<VkDescriptorBufferInfo>(&slot))
				{
					m_descriptor_set.push(*ptr, type, idx);
					return;
				}

				if (auto ptr = std::get_if<VkBufferView>(&slot))
				{
					m_descriptor_set.push(*ptr, type, idx);
					return;
				}

				fmt::throw_exception("Unexpected descriptor structure at index %u", idx);
			};

			m_copy_cmds.clear();
			rsx::flags32_t type_mask = 0u;

			for (unsigned i = 0; i < m_descriptor_slots.size(); ++i)
			{
				if (m_descriptors_dirty[i])
				{
					// Push
					push_descriptor_slot(i);
					m_descriptors_dirty[i] = false;
					continue;
				}

				m_copy_cmds.push_back({
					.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
					.dstSet = m_descriptor_set.value(),
					.dstBinding = i,
					.descriptorCount = 1,
					.descriptorType = m_descriptor_types[i],
					.pImageInfo = std::get_if<VkDescriptorImageInfo>(&m_descriptor_slots[i]),
					.pBufferInfo = std::get_if<VkDescriptorBufferInfo>(&m_descriptor_slots[i]),
					.pTexelBufferView = std::get_if<VkBufferView>(&m_descriptor_slots[i])
				});

				type_mask |= (1u << m_descriptor_types[i]);
			}

			m_descriptor_set.push(m_copy_cmds, type_mask); // Write previous state
			m_descriptor_set = allocate_descriptor_set();
		}

		void descriptor_table_t::create_descriptor_set_layout()
		{
			ensure(m_descriptor_set_layout == VK_NULL_HANDLE);

			rsx::simple_array<VkDescriptorSetLayoutBinding> bindings;
			bindings.reserve(16);

			m_descriptor_pool_sizes.clear();
			m_descriptor_pool_sizes.reserve(input_type_max_enum);

			for (const auto& type_arr : m_inputs)
			{
				if (type_arr.empty() || type_arr.front().type == input_type_push_constant)
				{
					continue;
				}

				VkDescriptorType type = to_descriptor_type(type_arr.front().type);
				m_descriptor_pool_sizes.push_back({ .type = type });

				for (const auto& input : type_arr)
				{
					VkDescriptorSetLayoutBinding binding
					{
						.binding = input.location,
						.descriptorType = type,
						.descriptorCount = 1,
						.stageFlags = to_shader_stage_flags(input.domain)
					};
					bindings.push_back(binding);

					if (m_descriptor_types.size() < (input.location + 1))
					{
						m_descriptor_types.resize((input.location + 1));
					}

					m_descriptor_types[input.location] = type;
					m_descriptor_pool_sizes.back().descriptorCount++;
				}
			}

			VkDescriptorSetLayoutCreateInfo set_layout_create_info
			{
				.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
				.flags = 0,
				.bindingCount = ::size32(bindings),
				.pBindings = bindings.data()
			};
			CHECK_RESULT(vkCreateDescriptorSetLayout(m_device, &set_layout_create_info, nullptr, &m_descriptor_set_layout));
		}

		void descriptor_table_t::create_descriptor_pool()
		{
			m_descriptor_pool = std::make_unique<descriptor_pool>();
			m_descriptor_pool->create(*vk::get_current_renderer(), m_descriptor_pool_sizes);
		}

		void descriptor_table_t::validate() const
		{
			// Check for overlapping locations
			std::set<u32> taken_locations;

			for (auto& type_arr : m_inputs)
			{
				if (type_arr.empty() ||
					type_arr.front().type == input_type_push_constant)
				{
					continue;
				}

				for (const auto& input : type_arr)
				{
					ensure(taken_locations.find(input.location) == taken_locations.end(), "Overlapping input locations found.");
					taken_locations.insert(input.location);
				}
			}
		}
	}
}
