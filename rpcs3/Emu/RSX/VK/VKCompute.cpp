#include "VKCompute.h"
#include "VKHelpers.h"
#include "VKRenderPass.h"
#include "vkutils/buffer_object.h"
#include "VKPipelineCompiler.h"
#include <thread>
#include <vector>
#include <atomic>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <cmath>

#define VK_MAX_COMPUTE_TASKS 8192   // Max number of jobs per frame

namespace vk
{
namespace cpu_compute {

template <typename Worker>
inline void parallel_for(size_t n, size_t numThreads, Worker&& worker)
{
	if (n == 0) return;
	numThreads = std::max<size_t>(1, std::min(numThreads, n));
	std::vector<std::thread> threads;
	threads.reserve(numThreads);
	size_t chunk = n / numThreads;
	size_t rem = n % numThreads;
	size_t start = 0;
	for (size_t t = 0; t < numThreads; ++t)
	{
		size_t sz = chunk + (t < rem ? 1 : 0);
		size_t s = start;
		size_t e = s + sz;
		threads.emplace_back([s,e, &worker](){ for (size_t i = s; i < e; ++i) worker(i); });
		start = e;
	}
	for (auto &th : threads) th.join();
}

inline void scatter_d24x8_cpu(uint32_t* base_words,
                              size_t total_words,
                              size_t block_length_words,
                              size_t z_offset_words,
                              size_t s_offset_words,
                              size_t numThreads = std::thread::hardware_concurrency())
{
    if (!base_words) {
		rsx_log.error("scatter_d24x8_cpu: null buffer");
		return;
	}
    if (block_length_words == 0) return;

    if ((block_length_words + z_offset_words) > total_words) {
        rsx_log.error("scatter_d24x8_cpu: z-offset out of range");
		return;
    }

    size_t stencil_word_count = (block_length_words + 3) / 4;
    if ((stencil_word_count + s_offset_words) > total_words) {
        rsx_log.error("scatter_d24x8_cpu: stencil offset out of range");
		return;
    }

    cpu_compute::parallel_for(block_length_words, numThreads, [&](size_t index) {
        uint32_t value = base_words[index];
        base_words[index + z_offset_words] = (value >> 8);

        size_t stencil_offset = index >> 2;
        uint32_t stencil_shift = static_cast<uint32_t>((index & 3) << 3);
        uint32_t stencil_mask = (value & 0xFFu) << stencil_shift;

        uint32_t* stencil_word_ptr = base_words + (stencil_offset + s_offset_words);

#if defined(__cpp_lib_atomic_ref)
        std::atomic_ref<uint32_t> ref(*stencil_word_ptr);
        ref.fetch_or(stencil_mask, std::memory_order_relaxed);
#else
        std::atomic<uint32_t>* atomic_ptr = reinterpret_cast<std::atomic<uint32_t>*>(stencil_word_ptr);
        uint32_t oldv = atomic_ptr->load(std::memory_order_relaxed);
        uint32_t newv;
        do {
            newv = oldv | stencil_mask;
        } while (!atomic_ptr->compare_exchange_weak(oldv, newv, std::memory_order_relaxed));
#endif
    });
}

// parallel sum for u32 source; returns 64-bit sum
inline uint64_t parallel_sum_u32(const uint32_t* src, size_t count, size_t numThreads = std::thread::hardware_concurrency())
{
    if (count == 0) return 0;
    numThreads = std::max<size_t>(1, std::min(numThreads, count));
    std::vector<uint64_t> partials(numThreads, 0);

    size_t chunk = count / numThreads;
    size_t rem = count % numThreads;
    size_t start = 0;
    std::vector<std::thread> threads;
    threads.reserve(numThreads);

	for (size_t t = 0; t < numThreads; ++t)
	{
		size_t sz = chunk + (t < rem ? 1 : 0);
		size_t s = start;
		size_t e = s + sz;
		threads.emplace_back([=, &partials]() {
			uint64_t acc = 0;
			const uint32_t* p = src + s;
			size_t len = e - s;
			// plain loop — compiler will usually vectorize
			for (size_t i = 0; i < len; ++i) acc += p[i];
			partials[t] = acc;
		});
		start = e;
	}
    for (auto &th : threads) th.join();

    uint64_t total = 0;
    for (auto v : partials) total += v;
    return total;
}

} // namespace cpu_compute

	std::vector<glsl::program_input> compute_task::get_inputs()
	{
		std::vector<glsl::program_input> result;
		for (unsigned i = 0; i < ssbo_count; ++i)
		{
			const auto input = glsl::program_input::make
			(
				::glsl::glsl_compute_program,
				"ssbo" + std::to_string(i),
				glsl::program_input_type::input_type_storage_buffer,
				0,
				i
			);
			result.push_back(input);
		}

		if (use_push_constants && push_constants_size > 0)
		{
			const auto input = glsl::program_input::make
			(
				::glsl::glsl_compute_program,
				"push_constants",
				glsl::program_input_type::input_type_push_constant,
				0,
				0,
				glsl::push_constant_ref{ .offset = 0, .size = push_constants_size }
			);
			result.push_back(input);
		}

		return result;
	}

	void compute_task::create()
	{
		if (!initialized)
		{
			switch (vk::get_driver_vendor())
			{
			case vk::driver_vendor::unknown:
			case vk::driver_vendor::INTEL:
			case vk::driver_vendor::ANV:
				// Intel hw has 8 threads, but LDS allocation behavior makes optimal group size between 64 and 256
				// Based on intel's own OpenCL recommended settings
				unroll_loops = true;
				optimal_kernel_size = 1;
				optimal_group_size = 128;
				break;
			case vk::driver_vendor::LAVAPIPE:
			case vk::driver_vendor::V3DV:
			case vk::driver_vendor::PANVK:
			case vk::driver_vendor::ARM_MALI:
				// TODO: Actually bench this. Using 32 for now to match other common configurations.
			case vk::driver_vendor::DOZEN:
				// Actual optimal size depends on the D3D device. Use 32 since it should work well on both AMD and NVIDIA
			case vk::driver_vendor::NVIDIA:
			case vk::driver_vendor::NVK:
				// Warps are multiples of 32. Increasing kernel depth seems to hurt performance (Nier, Big Duck sample)
				unroll_loops = true;
				optimal_kernel_size = 1;
				optimal_group_size = 128; //32;
				break;
			case vk::driver_vendor::AMD:
			case vk::driver_vendor::RADV:
				// Wavefronts are multiples of 64. (RDNA also supports wave32)
				unroll_loops = false;
				optimal_kernel_size = 1;
				optimal_group_size = 64;
				break;
			case vk::driver_vendor::MVK:
			case vk::driver_vendor::HONEYKRISP:
				unroll_loops = true;
				optimal_kernel_size = 1;
				optimal_group_size = 256;
				break;
			}

			const auto& gpu = vk::g_render_device->gpu();
			max_invocations_x = gpu.get_limits().maxComputeWorkGroupCount[0];

			initialized = true;
		}
	}

	void compute_task::destroy()
	{
		if (initialized)
		{
			m_shader.destroy();
			m_program.reset();
			m_param_buffer.reset();

			initialized = false;
		}
	}

	void compute_task::load_program(const vk::command_buffer& cmd)
	{
		if (!m_program)
		{
			m_shader.create(::glsl::program_domain::glsl_compute_program, m_src);
			auto handle = m_shader.compile();

			VkPipelineShaderStageCreateInfo shader_stage{};
			shader_stage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			shader_stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
			shader_stage.module = handle;
			shader_stage.pName = "main";

			VkComputePipelineCreateInfo create_info
			{
				.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
				.stage = {
					.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
					.stage = VK_SHADER_STAGE_COMPUTE_BIT,
					.module = handle,
					.pName = "main"
				},
			};

			auto compiler = vk::get_pipe_compiler();
			m_program = compiler->compile(create_info, vk::pipe_compiler::COMPILE_INLINE, {}, get_inputs());
		}

		bind_resources(cmd);
		m_program->bind(cmd, VK_PIPELINE_BIND_POINT_COMPUTE);
	}

	void compute_task::run(const vk::command_buffer& cmd, u32 invocations_x, u32 invocations_y, u32 invocations_z)
	{
		// CmdDispatch is outside renderpass scope only
		if (vk::is_renderpass_open(cmd))
		{
			vk::end_renderpass(cmd);
		}

		load_program(cmd);
		vkCmdDispatch(cmd, invocations_x, invocations_y, invocations_z);
	}

	void compute_task::run(const vk::command_buffer& cmd, u32 num_invocations)
	{
		u32 invocations_x, invocations_y;
		if (num_invocations > max_invocations_x)
		{
			// AMD hw reports an annoyingly small maximum number of invocations in the X dimension
			// Split the 1D job into 2 dimensions to accomodate this
			invocations_x = static_cast<u32>(floor(std::sqrt(num_invocations)));
			invocations_y = invocations_x;

			if (num_invocations % invocations_x) invocations_y++;
		}
		else
		{
			invocations_x = num_invocations;
			invocations_y = 1;
		}

		run(cmd, invocations_x, invocations_y, 1);
	}

	cs_shuffle_base::cs_shuffle_base()
	{
		work_kernel =
			"		value = data[index];\n"
			"		data[index] = %f(value);\n";

		loop_advance =
			"		index++;\n";

		suffix =
			"}\n";
	}

	void cs_shuffle_base::build(const char* function_name, u32 _kernel_size)
	{
		// Initialize to allow detecting optimal settings
		create();

		kernel_size = _kernel_size? _kernel_size : optimal_kernel_size;

		m_src =
		#include "../Program/GLSLSnippets/ShuffleBytes.glsl"
		;

		const auto parameters_size = utils::align(push_constants_size, 16) / 16;
		const std::pair<std::string_view, std::string> syntax_replace[] =
		{
			{ "%loc", "0" },
			{ "%set", "set = 0"},
			{ "%ws", std::to_string(optimal_group_size) },
			{ "%ks", std::to_string(kernel_size) },
			{ "%vars", variables },
			{ "%f", function_name },
			{ "%md", method_declarations },
			{ "%ub", use_push_constants? "layout(push_constant) uniform ubo{ uvec4 params[" + std::to_string(parameters_size) + "]; };\n" : "" },
		};

		m_src = fmt::replace_all(m_src, syntax_replace);
		work_kernel = fmt::replace_all(work_kernel, syntax_replace);

		if (kernel_size <= 1)
		{
			m_src += "	{\n" + work_kernel + "	}\n";
		}
		else if (unroll_loops)
		{
			work_kernel += loop_advance + "\n";

			m_src += std::string
			(
				"	//Unrolled loop\n"
				"	{\n"
			);

			// Assemble body with manual loop unroll to try loweing GPR usage
			for (u32 n = 0; n < kernel_size; ++n)
			{
				m_src += work_kernel;
			}

			m_src += "	}\n";
		}
		else
		{
			m_src += "	for (int loop = 0; loop < KERNEL_SIZE; ++loop)\n";
			m_src += "	{\n";
			m_src += work_kernel;
			m_src += loop_advance;
			m_src += "	}\n";
		}

		m_src += suffix;
	}

	void cs_shuffle_base::bind_resources(const vk::command_buffer& cmd)
	{
		set_parameters(cmd);
		m_program->bind_uniform({ *m_data, m_data_offset, m_data_length }, 0, 0);
	}

	void cs_shuffle_base::set_parameters(const vk::command_buffer& cmd)
	{
		if (!m_params.empty())
		{
			ensure(use_push_constants);
			vkCmdPushConstants(cmd, m_program->layout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, m_params.size_bytes32(), m_params.data());
		}
	}

	void cs_shuffle_base::run(const vk::command_buffer& cmd, const vk::buffer* data, u32 data_length, u32 data_offset)
	{
		m_data = data;
		m_data_offset = data_offset;
		m_data_length = data_length;

		const auto num_bytes_per_invocation = optimal_group_size * kernel_size * 4;
		const auto num_bytes_to_process = rsx::align2(data_length, num_bytes_per_invocation);
		const auto num_invocations = num_bytes_to_process / num_bytes_per_invocation;

		if ((num_bytes_to_process + data_offset) > data->size())
		{
			// Technically robust buffer access should keep the driver from crashing in OOB situations
			rsx_log.error("Inadequate buffer length submitted for a compute operation."
				"Required=%d bytes, Available=%d bytes", num_bytes_to_process, data->size());
		}

		compute_task::run(cmd, num_invocations);
	}

	cs_interleave_task::cs_interleave_task()
	{
		use_push_constants = true;
		push_constants_size = 16;

		variables =
			"	uint block_length = params[0].x >> 2;\n"
			"	uint z_offset = params[0].y >> 2;\n"
			"	uint s_offset = params[0].z >> 2;\n"
			"	uint depth;\n"
			"	uint stencil;\n"
			"	uint stencil_shift;\n"
			"	uint stencil_offset;\n";
	}

	void cs_interleave_task::bind_resources(const vk::command_buffer& cmd)
	{
		set_parameters(cmd);
		m_program->bind_uniform({ *m_data, m_data_offset, m_ssbo_length }, 0, 0);
	}

	void cs_interleave_task::run(const vk::command_buffer& /*cmd*/,
								const vk::buffer* data,
								u32 data_offset,
								u32 data_length,
								u32 zeta_offset,
								u32 stencil_offset)
	{
		size_t hw = std::max<size_t>(1, std::thread::hardware_concurrency());
		size_t numThreads = std::min<size_t>(hw, 8);

		m_params = { data_length, zeta_offset - data_offset, stencil_offset - data_offset, 0 };
		ensure(stencil_offset > data_offset);

		void* mapped = const_cast<vk::buffer*>(data)->map(data_offset, data_length);
		if (!mapped) {
			rsx_log.error("cs_interleave_task::run: failed to map buffer");
			return;
		}

		uint8_t* base_bytes = reinterpret_cast<uint8_t*>(mapped);
		uint8_t* region_start = base_bytes + data_offset;
		uint32_t* words = reinterpret_cast<uint32_t*>(region_start);

		const size_t region_total_words = (data->size() - data_offset) / 4;
		const size_t block_length_words = data_length / 4;
		const size_t z_offset_words = (zeta_offset - data_offset) / 4;
		const size_t s_offset_words = (stencil_offset - data_offset) / 4;

		if ((block_length_words + z_offset_words) > region_total_words) {
			const_cast<vk::buffer*>(data)->unmap();
			rsx_log.error("cs_interleave_task::run: z_offset out of bounds");
			return;
		}
		size_t stencil_word_count = (block_length_words + 3) / 4;
		if ((stencil_word_count + s_offset_words) > region_total_words) {
			const_cast<vk::buffer*>(data)->unmap();
			rsx_log.error("cs_interleave_task::run: stencil offset out of bounds");
			return;
		}

		cpu_compute::scatter_d24x8_cpu(
			/*base_words=*/ words,
			/*total_words=*/ region_total_words,
			/*block_length_words=*/ block_length_words,
			/*z_offset_words=*/ z_offset_words,
			/*s_offset_words=*/ s_offset_words,
			/*numThreads=*/ numThreads
		);

		const_cast<vk::buffer*>(data)->unmap();
	}

	cs_scatter_d24x8::cs_scatter_d24x8()
	{
		work_kernel =
			"		if (index >= block_length)\n"
			"			return;\n"
			"\n"
			"		value = data[index];\n"
			"		data[index + z_offset] = (value >> 8);\n"
			"		stencil_offset = (index / 4);\n"
			"		stencil_shift = (index % 4) * 8;\n"
			"		stencil = (value & 0xFF) << stencil_shift;\n"
			"		atomicOr(data[stencil_offset + s_offset], stencil);\n";

		cs_shuffle_base::build("");
	}

	cs_aggregator::cs_aggregator()
	{
		ssbo_count = 2;

		create();

		m_src =
			"#version 450\n"
			"layout(local_size_x = %ws, local_size_y = 1, local_size_z = 1) in;\n\n"

			"layout(set=0, binding=0, std430) readonly buffer ssbo0{ uint src[]; };\n"
			"layout(set=0, binding=1, std430) writeonly buffer ssbo1{ uint result; };\n\n"

			"void main()\n"
			"{\n"
			"	if (gl_GlobalInvocationID.x < src.length())\n"
			"	{\n"
			"		atomicAdd(result, src[gl_GlobalInvocationID.x]);\n"
			"	}\n"
			"}\n";

		const std::pair<std::string_view, std::string> syntax_replace[] =
		{
			{ "%ws", std::to_string(optimal_group_size) },
		};

		m_src = fmt::replace_all(m_src, syntax_replace);
	}

	void cs_aggregator::bind_resources(const vk::command_buffer& /*cmd*/)
	{
		m_program->bind_uniform({ *src, 0, block_length }, 0, 0);
		m_program->bind_uniform({ *dst, 0, 4 }, 0, 1);
	}

	void cs_aggregator::run(const vk::command_buffer& /*cmd*/, const vk::buffer* dst, const vk::buffer* src, u32 num_words)
	{
		this->dst = dst;
		this->src = src;
		word_count = num_words;
		block_length = num_words * 4;

		size_t hw = std::max<size_t>(1, std::thread::hardware_concurrency());
		size_t numThreads = std::min<size_t>(hw, 8);

		void* src_map = const_cast<vk::buffer*>(src)->map(0, num_words * 4);
		if (!src_map) {
			rsx_log.error("cs_aggregator::run: failed to map src buffer");
			return;
		}

		const uint32_t* src_words = reinterpret_cast<const uint32_t*>(src_map);
		uint64_t sum = cpu_compute::parallel_sum_u32(src_words, static_cast<size_t>(num_words), numThreads);

		const_cast<vk::buffer*>(src)->unmap();

		void* dst_map = const_cast<vk::buffer*>(dst)->map(0, 4);
		if (!dst_map) {
			rsx_log.error("cs_aggregator::run: failed to map dst buffer");
			return;
		}

		uint32_t result32 = static_cast<uint32_t>(sum);
		std::memcpy(dst_map, &result32, sizeof(result32));

		const_cast<vk::buffer*>(dst)->unmap();
	}
}
