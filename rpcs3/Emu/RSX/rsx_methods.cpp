#include "stdafx.h"
#include "rsx_methods.h"
#include "RSXThread.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/lv2/sys_rsx.h"


#include "Emu/System.h"
#include "Emu/RSX/NV47/HW/nv47.h"
#include "Emu/RSX/NV47/HW/nv47_sync.hpp"
#include "Emu/RSX/NV47/HW/context_accessors.define.h" // TODO: Context objects belong in FW not HW

namespace rsx
{
	rsx_state method_registers;

	std::array<rsx_method_t, 0x10000 / 4> methods{};
	std::array<u32, 0x10000 / 4> state_signals{};

	void invalid_method(context* ctx, u32 reg, u32 arg)
	{
		//Don't throw, gather information and ignore broken/garbage commands
		//TODO: Investigate why these commands are executed at all. (Heap corruption? Alignment padding?)
		const u32 cmd = RSX(ctx)->get_fifo_cmd();
		rsx_log.error("Invalid RSX method 0x%x (arg=0x%x, start=0x%x, count=0x%x, non-inc=%s)", reg << 2, arg,
		cmd & 0xfffc, (cmd >> 18) & 0x7ff, !!(cmd & RSX_METHOD_NON_INCREMENT_CMD));

		if (g_cfg.core.rsx_fifo_accuracy != rsx_fifo_mode::as_ps3)
		{
			RSX(ctx)->recover_fifo();
		}
	}

	static void trace_method(context* /*ctx*/, u32 reg, u32 arg)
	{
		// For unknown yet valid methods
		rsx_log.trace("RSX method 0x%x (arg=0x%x)", reg << 2, arg);
	}

	void flip_command(context* ctx, u32, u32 arg)
	{
		ensure(RSX(ctx)->isHLE);

		if (RSX(ctx)->vblank_at_flip != umax)
		{
			RSX(ctx)->flip_notification_count++;
		}

		if (auto ptr = RSX(ctx)->queue_handler)
		{
			RSX(ctx)->intr_thread->cmd_list
			({
				{ ppu_cmd::set_args, 1 }, u64{1},
				{ ppu_cmd::lle_call, ptr },
				{ ppu_cmd::sleep, 0 }
			});

			RSX(ctx)->intr_thread->cmd_notify.store(1);
			RSX(ctx)->intr_thread->cmd_notify.notify_one();
		}

		RSX(ctx)->reset();
		RSX(ctx)->on_frame_end(arg);
		RSX(ctx)->request_emu_flip(arg);
		vm::_ptr<atomic_t<u128>>(RSX(ctx)->label_addr + 0x10)->store(u128{});
	}

	void user_command(context* ctx, u32, u32 arg)
	{
		if (!RSX(ctx)->isHLE)
		{
			sys_rsx_context_attribute(0x55555555, 0xFEF, 0, arg, 0, 0);
			return;
		}

		if (auto ptr = RSX(ctx)->user_handler)
		{
			RSX(ctx)->intr_thread->cmd_list
			({
				{ ppu_cmd::set_args, 1 }, u64{arg},
				{ ppu_cmd::lle_call, ptr },
				{ ppu_cmd::sleep, 0 }
			});

			RSX(ctx)->intr_thread->cmd_notify.store(1);
			RSX(ctx)->intr_thread->cmd_notify.notify_one();
		}
	}

	namespace gcm
	{
		template<u32 index>
		struct driver_flip
		{
			static void impl(context*, u32 /*reg*/, u32 arg)
			{
				sys_rsx_context_attribute(0x55555555, 0x102, index, arg, 0, 0);
			}
		};

		template<u32 index>
		struct queue_flip
		{
			static void impl(context* ctx, u32 /*reg*/, u32 arg)
			{
				if (RSX(ctx)->vblank_at_flip != umax)
				{
					RSX(ctx)->flip_notification_count++;
				}

				sys_rsx_context_attribute(0x55555555, 0x103, index, arg, 0, 0);
			}
		};
	}

	namespace fifo
	{
		void draw_barrier(context* ctx, u32, u32)
		{
			if (RSX(ctx)->in_begin_end)
			{
				if (!REGS(ctx)->current_draw_clause.is_disjoint_primitive)
				{
					// Enable primitive barrier request
					REGS(ctx)->current_draw_clause.primitive_barrier_enable = true;
				}
			}
		}
	}

	void rsx_state::init()
	{
		// Reset all regsiters
		registers.fill(0);
		state_signals.fill(0);
		transform_program.fill(0);
		transform_constants = {};
		current_draw_clause = {};
		register_vertex_info = {};

		// Special values set at initialization, these are not set by a context reset
		registers[NV4097_SET_SHADER_PROGRAM] = (0 << 2) | (CELL_GCM_LOCATION_LOCAL + 1);

		for (u32 i = 0; i < 16; i++)
		{
			registers[NV4097_SET_TEXTURE_FORMAT + (i * 8)] = (1 << 16 /* mipmap */) | ((CELL_GCM_TEXTURE_R5G6B5 | CELL_GCM_TEXTURE_SZ | CELL_GCM_TEXTURE_NR) << 8) | (2 << 4 /* 2D */) | (CELL_GCM_LOCATION_LOCAL + 1);
		}

		for (u32 i = 0; i < 4; i++)
		{
			registers[NV4097_SET_VERTEX_TEXTURE_FORMAT + (i * 8)] = (1 << 16 /* mipmap */) | ((CELL_GCM_TEXTURE_X32_FLOAT | CELL_GCM_TEXTURE_LN | CELL_GCM_TEXTURE_NR) << 8) | (2 << 4 /* 2D */) | (CELL_GCM_LOCATION_LOCAL + 1);
		}

		registers[NV406E_SET_CONTEXT_DMA_SEMAPHORE] = CELL_GCM_CONTEXT_DMA_SEMAPHORE_R;
		registers[NV4097_SET_CONTEXT_DMA_SEMAPHORE] = CELL_GCM_CONTEXT_DMA_SEMAPHORE_RW;

		{
			// Commands injected by cellGcmInit
			registers[NV406E_SEMAPHORE_OFFSET] = 0x30;
			registers[NV406E_SEMAPHORE_ACQUIRE] = 0x1;
			registers[NV406E_SET_CONTEXT_DMA_SEMAPHORE] = 0x66616661;
			registers[0x0] = 0x31337000;
			registers[NV4097_SET_CONTEXT_DMA_NOTIFIES] = 0x66604200;
			registers[NV4097_SET_CONTEXT_DMA_A] = 0xfeed0000;
			registers[NV4097_SET_CONTEXT_DMA_B] = 0xfeed0001;
			registers[NV4097_SET_CONTEXT_DMA_COLOR_B] = 0xfeed0000;
			registers[NV4097_SET_CONTEXT_DMA_STATE] = 0x0;
			registers[NV4097_SET_CONTEXT_DMA_COLOR_A] = 0xfeed0000;
			registers[NV4097_SET_CONTEXT_DMA_ZETA] = 0xfeed0000;
			registers[NV4097_SET_CONTEXT_DMA_VERTEX_A] = 0xfeed0000;
			registers[NV4097_SET_CONTEXT_DMA_VERTEX_B] = 0xfeed0001;
			registers[NV4097_SET_CONTEXT_DMA_SEMAPHORE] = 0x66606660;
			registers[NV4097_SET_CONTEXT_DMA_REPORT] = 0x66626660;
			registers[NV4097_SET_CONTEXT_DMA_CLIP_ID] = 0x0;
			registers[NV4097_SET_CONTEXT_DMA_CULL_DATA] = 0x0;
			registers[NV4097_SET_CONTEXT_DMA_COLOR_C] = 0xfeed0000;
			registers[NV4097_SET_CONTEXT_DMA_COLOR_D] = 0xfeed0000;
			registers[NV406E_SET_CONTEXT_DMA_SEMAPHORE] = 0x66616661;
			registers[NV4097_SET_SURFACE_CLIP_HORIZONTAL] = 0x0;
			registers[NV4097_SET_SURFACE_CLIP_VERTICAL] = 0x0;
			registers[NV4097_SET_SURFACE_FORMAT] = 0x121;
			registers[NV4097_SET_SURFACE_PITCH_A] = 0x40;
			registers[NV4097_SET_SURFACE_COLOR_AOFFSET] = 0x0;
			registers[NV4097_SET_SURFACE_ZETA_OFFSET] = 0x0;
			registers[NV4097_SET_SURFACE_COLOR_BOFFSET] = 0x0;
			registers[NV4097_SET_SURFACE_PITCH_B] = 0x40;
			registers[NV4097_SET_SURFACE_COLOR_TARGET] = 0x1;
			registers[0x224 / 4] = 0x80;
			registers[0x228 / 4] = 0x100;
			registers[NV4097_SET_SURFACE_PITCH_Z] = 0x40;
			registers[0x230 / 4] = 0x0;
			registers[NV4097_SET_SURFACE_PITCH_C] = 0x40;
			registers[NV4097_SET_SURFACE_PITCH_D] = 0x40;
			registers[NV4097_SET_SURFACE_COLOR_COFFSET] = 0x0;
			registers[NV4097_SET_SURFACE_COLOR_DOFFSET] = 0x0;
			registers[0x1d80 / 4] = 0x3;
			registers[NV4097_SET_WINDOW_OFFSET] = 0x0;
			registers[0x2bc / 4] = 0x0;
			registers[0x2c0 / 4] = 0xfff0000;
			registers[0x2c4 / 4] = 0xfff0000;
			registers[0x2c8 / 4] = 0xfff0000;
			registers[0x2cc / 4] = 0xfff0000;
			registers[0x2d0 / 4] = 0xfff0000;
			registers[0x2d4 / 4] = 0xfff0000;
			registers[0x2d8 / 4] = 0xfff0000;
			registers[0x2dc / 4] = 0xfff0000;
			registers[0x2e0 / 4] = 0xfff0000;
			registers[0x2e4 / 4] = 0xfff0000;
			registers[0x2e8 / 4] = 0xfff0000;
			registers[0x2ec / 4] = 0xfff0000;
			registers[0x2f0 / 4] = 0xfff0000;
			registers[0x2f4 / 4] = 0xfff0000;
			registers[0x2f8 / 4] = 0xfff0000;
			registers[0x2fc / 4] = 0xfff0000;
			registers[0x1d98 / 4] = 0xfff0000;
			registers[0x1d9c / 4] = 0xfff0000;
			registers[0x1da4 / 4] = 0x0;
			registers[NV4097_SET_CONTROL0] = 0x100000;
			registers[0x1454 / 4] = 0x0;
			registers[NV4097_SET_VERTEX_ATTRIB_OUTPUT_MASK] = 0x3fffff;
			registers[NV4097_SET_FREQUENCY_DIVIDER_OPERATION] = 0x0;
			registers[NV4097_SET_ATTRIB_COLOR] = 0x6144321;
			registers[NV4097_SET_ATTRIB_TEX_COORD] = 0xedcba987;
			registers[NV4097_SET_ATTRIB_TEX_COORD_EX] = 0x6f;
			registers[NV4097_SET_ATTRIB_UCLIP0] = 0x171615;
			registers[NV4097_SET_ATTRIB_UCLIP1] = 0x1b1a19;
			registers[NV4097_SET_TEX_COORD_CONTROL] = 0x0;
			registers[NV4097_SET_TEX_COORD_CONTROL + 1] = 0x0;
			registers[NV4097_SET_TEX_COORD_CONTROL + 2] = 0x0;
			registers[NV4097_SET_TEX_COORD_CONTROL + 3] = 0x0;
			registers[NV4097_SET_TEX_COORD_CONTROL + 4] = 0x0;
			registers[NV4097_SET_TEX_COORD_CONTROL + 5] = 0x0;
			registers[NV4097_SET_TEX_COORD_CONTROL + 6] = 0x0;
			registers[NV4097_SET_TEX_COORD_CONTROL + 7] = 0x0;
			registers[NV4097_SET_TEX_COORD_CONTROL + 8] = 0x0;
			registers[NV4097_SET_TEX_COORD_CONTROL + 9] = 0x0;
			registers[0xa0c / 4] = 0x0;
			registers[0xa60 / 4] = 0x0;
			registers[NV4097_SET_POLY_OFFSET_LINE_ENABLE] = 0x0;
			registers[NV4097_SET_POLY_OFFSET_FILL_ENABLE] = 0x0;
			registers[NV4097_SET_POLYGON_OFFSET_SCALE_FACTOR] = 0x0;
			registers[NV4097_SET_POLYGON_OFFSET_BIAS] = 0x0;
			registers[0x1428 / 4] = 0x1;
			registers[NV4097_SET_SHADER_WINDOW] = 0x1000;
			registers[0x1e94 / 4] = 0x11;
			registers[0x1450 / 4] = 0x80003;
			registers[0x1d64 / 4] = 0x2000000;
			registers[0x145c / 4] = 0x1;
			registers[NV4097_SET_REDUCE_DST_COLOR] = 0x1;
			registers[NV4097_SET_TEXTURE_CONTROL2] = 0x2dc8;
			registers[NV4097_SET_TEXTURE_CONTROL2 + 1] = 0x2dc8;
			registers[NV4097_SET_TEXTURE_CONTROL2 + 2] = 0x2dc8;
			registers[NV4097_SET_TEXTURE_CONTROL2 + 3] = 0x2dc8;
			registers[NV4097_SET_TEXTURE_CONTROL2 + 4] = 0x2dc8;
			registers[NV4097_SET_TEXTURE_CONTROL2 + 5] = 0x2dc8;
			registers[NV4097_SET_TEXTURE_CONTROL2 + 6] = 0x2dc8;
			registers[NV4097_SET_TEXTURE_CONTROL2 + 7] = 0x2dc8;
			registers[NV4097_SET_TEXTURE_CONTROL2 + 8] = 0x2dc8;
			registers[NV4097_SET_TEXTURE_CONTROL2 + 9] = 0x2dc8;
			registers[NV4097_SET_TEXTURE_CONTROL2 + 10] = 0x2dc8;
			registers[NV4097_SET_TEXTURE_CONTROL2 + 11] = 0x2dc8;
			registers[NV4097_SET_TEXTURE_CONTROL2 + 12] = 0x2dc8;
			registers[NV4097_SET_TEXTURE_CONTROL2 + 13] = 0x2dc8;
			registers[NV4097_SET_TEXTURE_CONTROL2 + 14] = 0x2dc8;
			registers[NV4097_SET_TEXTURE_CONTROL2 + 15] = 0x2dc8;
			registers[NV4097_SET_FOG_MODE] = 0x800;
			registers[NV4097_SET_FOG_PARAMS] = 0x0;
			registers[NV4097_SET_FOG_PARAMS + 1] = 0x0;
			registers[NV4097_SET_FOG_PARAMS + 2] = 0x0;
			registers[0x240 / 4] = 0xffff;
			registers[0x244 / 4] = 0x0;
			registers[0x248 / 4] = 0x0;
			registers[0x24c / 4] = 0x0;
			registers[NV4097_SET_ANISO_SPREAD] = 0x10101;
			registers[NV4097_SET_ANISO_SPREAD + 1] = 0x10101;
			registers[NV4097_SET_ANISO_SPREAD + 2] = 0x10101;
			registers[NV4097_SET_ANISO_SPREAD + 3] = 0x10101;
			registers[NV4097_SET_ANISO_SPREAD + 4] = 0x10101;
			registers[NV4097_SET_ANISO_SPREAD + 5] = 0x10101;
			registers[NV4097_SET_ANISO_SPREAD + 6] = 0x10101;
			registers[NV4097_SET_ANISO_SPREAD + 7] = 0x10101;
			registers[NV4097_SET_ANISO_SPREAD + 8] = 0x10101;
			registers[NV4097_SET_ANISO_SPREAD + 9] = 0x10101;
			registers[NV4097_SET_ANISO_SPREAD + 10] = 0x10101;
			registers[NV4097_SET_ANISO_SPREAD + 11] = 0x10101;
			registers[NV4097_SET_ANISO_SPREAD + 12] = 0x10101;
			registers[NV4097_SET_ANISO_SPREAD + 13] = 0x10101;
			registers[NV4097_SET_ANISO_SPREAD + 14] = 0x10101;
			registers[NV4097_SET_ANISO_SPREAD + 15] = 0x10101;
			registers[0x400 / 4] = 0x7421;
			registers[0x404 / 4] = 0x7421;
			registers[0x408 / 4] = 0x7421;
			registers[0x40c / 4] = 0x7421;
			registers[0x410 / 4] = 0x7421;
			registers[0x414 / 4] = 0x7421;
			registers[0x418 / 4] = 0x7421;
			registers[0x41c / 4] = 0x7421;
			registers[0x420 / 4] = 0x7421;
			registers[0x424 / 4] = 0x7421;
			registers[0x428 / 4] = 0x7421;
			registers[0x42c / 4] = 0x7421;
			registers[0x430 / 4] = 0x7421;
			registers[0x434 / 4] = 0x7421;
			registers[0x438 / 4] = 0x7421;
			registers[0x43c / 4] = 0x7421;
			registers[0x440 / 4] = 0x9aabaa98;
			registers[0x444 / 4] = 0x66666789;
			registers[0x448 / 4] = 0x98766666;
			registers[0x44c / 4] = 0x89aabaa9;
			registers[0x450 / 4] = 0x99999999;
			registers[0x454 / 4] = 0x88888889;
			registers[0x458 / 4] = 0x98888888;
			registers[0x45c / 4] = 0x99999999;
			registers[0x460 / 4] = 0x56676654;
			registers[0x464 / 4] = 0x33333345;
			registers[0x468 / 4] = 0x54333333;
			registers[0x46c / 4] = 0x45667665;
			registers[0x470 / 4] = 0xaabbba99;
			registers[0x474 / 4] = 0x66667899;
			registers[0x478 / 4] = 0x99876666;
			registers[0x47c / 4] = 0x99abbbaa;
			registers[NV4097_SET_VERTEX_DATA_BASE_OFFSET] = 0x0;
			registers[NV4097_SET_VERTEX_DATA_BASE_INDEX] = 0x0;
			registers[0xe000 / 4] = 0xcafebabe;
			registers[NV4097_SET_ALPHA_FUNC] = 0x207;
			registers[NV4097_SET_ALPHA_REF] = 0x0;
			registers[NV4097_SET_ALPHA_TEST_ENABLE] = 0x0;
			registers[NV4097_SET_BACK_STENCIL_FUNC] = 0x207;
			registers[NV4097_SET_BACK_STENCIL_FUNC_REF] = 0x0;
			registers[NV4097_SET_BACK_STENCIL_FUNC_MASK] = 0xff;
			registers[NV4097_SET_BACK_STENCIL_MASK] = 0xff;
			registers[NV4097_SET_BACK_STENCIL_OP_FAIL] = 0x1e00;
			registers[NV4097_SET_BACK_STENCIL_OP_ZFAIL] = 0x1e00;
			registers[NV4097_SET_BACK_STENCIL_OP_ZPASS] = 0x1e00;
			registers[NV4097_SET_BLEND_COLOR] = 0x0;
			registers[NV4097_SET_BLEND_COLOR2] = 0x0;
			registers[NV4097_SET_BLEND_ENABLE] = 0x0;
			registers[NV4097_SET_BLEND_ENABLE_MRT] = 0x0;
			registers[NV4097_SET_BLEND_EQUATION] = 0x80068006;
			registers[NV4097_SET_BLEND_FUNC_SFACTOR] = 0x10001;
			registers[NV4097_SET_BLEND_FUNC_DFACTOR] = 0x0;
			registers[NV4097_SET_ZSTENCIL_CLEAR_VALUE] = 0xffffff00;
			registers[NV4097_CLEAR_SURFACE] = 0x0;
			registers[NV4097_NO_OPERATION] = 0x0;
			registers[NV4097_SET_COLOR_MASK] = 0x1010101;
			registers[NV4097_SET_CULL_FACE_ENABLE] = 0x0;
			registers[NV4097_SET_CULL_FACE] = 0x405;
			registers[NV4097_SET_DEPTH_BOUNDS_MIN] = 0x0;
			registers[NV4097_SET_DEPTH_BOUNDS_MAX] = 0x3f800000;
			registers[NV4097_SET_DEPTH_BOUNDS_TEST_ENABLE] = 0x0;
			registers[NV4097_SET_DEPTH_FUNC] = 0x201;
			registers[NV4097_SET_DEPTH_MASK] = 0x1;
			registers[NV4097_SET_DEPTH_TEST_ENABLE] = 0x0;
			registers[NV4097_SET_DITHER_ENABLE] = 0x1;
			registers[NV4097_SET_SHADER_PACKER] = 0x0;
			registers[NV4097_SET_FREQUENCY_DIVIDER_OPERATION] = 0x0;
			registers[NV4097_SET_FRONT_FACE] = 0x901;
			registers[NV4097_SET_LINE_WIDTH] = 0x8;
			registers[NV4097_SET_LOGIC_OP_ENABLE] = 0x0;
			registers[NV4097_SET_LOGIC_OP] = 0x1503;
			registers[NV4097_SET_POINT_SIZE] = 0x3f800000;
			registers[NV4097_SET_POLY_OFFSET_FILL_ENABLE] = 0x0;
			registers[NV4097_SET_POLYGON_OFFSET_SCALE_FACTOR] = 0x0;
			registers[NV4097_SET_POLYGON_OFFSET_BIAS] = 0x0;
			registers[NV4097_SET_RESTART_INDEX_ENABLE] = 0x0;
			registers[NV4097_SET_RESTART_INDEX] = 0xffffffff;
			registers[NV4097_SET_SCISSOR_HORIZONTAL] = 0x10000000;
			registers[NV4097_SET_SCISSOR_VERTICAL] = 0x10000000;
			registers[NV4097_SET_SHADE_MODE] = 0x1d01;
			registers[NV4097_SET_STENCIL_FUNC] = 0x207;
			registers[NV4097_SET_STENCIL_FUNC_REF] = 0x0;
			registers[NV4097_SET_STENCIL_FUNC_MASK] = 0xff;
			registers[NV4097_SET_STENCIL_MASK] = 0xff;
			registers[NV4097_SET_STENCIL_OP_FAIL] = 0x1e00;
			registers[NV4097_SET_STENCIL_OP_ZFAIL] = 0x1e00;
			registers[NV4097_SET_STENCIL_OP_ZPASS] = 0x1e00;
			registers[NV4097_SET_STENCIL_TEST_ENABLE] = 0x0;
			registers[NV4097_SET_TEXTURE_ADDRESS] = 0x30101;
			registers[NV4097_SET_TEXTURE_BORDER_COLOR] = 0x0;
			registers[NV4097_SET_TEXTURE_CONTROL0] = 0x60000;
			registers[NV4097_SET_TEXTURE_FILTER] = 0x2052000;
			registers[NV4097_SET_TEXTURE_ADDRESS + 8] = 0x30101;
			registers[NV4097_SET_TEXTURE_BORDER_COLOR + 8] = 0x0;
			registers[NV4097_SET_TEXTURE_CONTROL0 + 8] = 0x60000;
			registers[NV4097_SET_TEXTURE_FILTER + 8] = 0x2052000;
			registers[NV4097_SET_TEXTURE_ADDRESS + 16] = 0x30101;
			registers[NV4097_SET_TEXTURE_BORDER_COLOR + 16] = 0x0;
			registers[NV4097_SET_TEXTURE_CONTROL0 + 16] = 0x60000;
			registers[NV4097_SET_TEXTURE_FILTER + 16] = 0x2052000;
			registers[NV4097_SET_TEXTURE_ADDRESS + 24] = 0x30101;
			registers[NV4097_SET_TEXTURE_BORDER_COLOR + 24] = 0x0;
			registers[NV4097_SET_TEXTURE_CONTROL0 + 24] = 0x60000;
			registers[NV4097_SET_TEXTURE_FILTER + 24] = 0x2052000;
			registers[NV4097_SET_TEXTURE_ADDRESS + 32] = 0x30101;
			registers[NV4097_SET_TEXTURE_BORDER_COLOR + 32] = 0x0;
			registers[NV4097_SET_TEXTURE_CONTROL0 + 32] = 0x60000;
			registers[NV4097_SET_TEXTURE_FILTER + 32] = 0x2052000;
			registers[NV4097_SET_TEXTURE_ADDRESS + 40] = 0x30101;
			registers[NV4097_SET_TEXTURE_BORDER_COLOR + 40] = 0x0;
			registers[NV4097_SET_TEXTURE_CONTROL0 + 40] = 0x60000;
			registers[NV4097_SET_TEXTURE_FILTER + 40] = 0x2052000;
			registers[NV4097_SET_TEXTURE_ADDRESS + 48] = 0x30101;
			registers[NV4097_SET_TEXTURE_BORDER_COLOR + 48] = 0x0;
			registers[NV4097_SET_TEXTURE_CONTROL0 + 48] = 0x60000;
			registers[NV4097_SET_TEXTURE_FILTER + 48] = 0x2052000;
			registers[NV4097_SET_TEXTURE_ADDRESS + 56] = 0x30101;
			registers[NV4097_SET_TEXTURE_BORDER_COLOR + 56] = 0x0;
			registers[NV4097_SET_TEXTURE_CONTROL0 + 56] = 0x60000;
			registers[NV4097_SET_TEXTURE_FILTER + 56] = 0x2052000;
			registers[NV4097_SET_TEXTURE_ADDRESS + 64] = 0x30101;
			registers[NV4097_SET_TEXTURE_BORDER_COLOR + 64] = 0x0;
			registers[NV4097_SET_TEXTURE_CONTROL0 + 64] = 0x60000;
			registers[NV4097_SET_TEXTURE_FILTER + 64] = 0x2052000;
			registers[NV4097_SET_TEXTURE_ADDRESS + 72] = 0x30101;
			registers[NV4097_SET_TEXTURE_BORDER_COLOR + 72] = 0x0;
			registers[NV4097_SET_TEXTURE_CONTROL0 + 72] = 0x60000;
			registers[NV4097_SET_TEXTURE_FILTER + 72] = 0x2052000;
			registers[NV4097_SET_TEXTURE_ADDRESS + 80] = 0x30101;
			registers[NV4097_SET_TEXTURE_BORDER_COLOR + 80] = 0x0;
			registers[NV4097_SET_TEXTURE_CONTROL0 + 80] = 0x60000;
			registers[NV4097_SET_TEXTURE_FILTER + 80] = 0x2052000;
			registers[NV4097_SET_TEXTURE_ADDRESS + 88] = 0x30101;
			registers[NV4097_SET_TEXTURE_BORDER_COLOR + 88] = 0x0;
			registers[NV4097_SET_TEXTURE_CONTROL0 + 88] = 0x60000;
			registers[NV4097_SET_TEXTURE_FILTER + 88] = 0x2052000;
			registers[NV4097_SET_TEXTURE_ADDRESS + 96] = 0x30101;
			registers[NV4097_SET_TEXTURE_BORDER_COLOR + 96] = 0x0;
			registers[NV4097_SET_TEXTURE_CONTROL0 + 96] = 0x60000;
			registers[NV4097_SET_TEXTURE_FILTER + 96] = 0x2052000;
			registers[NV4097_SET_TEXTURE_ADDRESS + 104] = 0x30101;
			registers[NV4097_SET_TEXTURE_BORDER_COLOR + 104] = 0x0;
			registers[NV4097_SET_TEXTURE_CONTROL0 + 104] = 0x60000;
			registers[NV4097_SET_TEXTURE_FILTER + 104] = 0x2052000;
			registers[NV4097_SET_TEXTURE_ADDRESS + 112] = 0x30101;
			registers[NV4097_SET_TEXTURE_BORDER_COLOR + 112] = 0x0;
			registers[NV4097_SET_TEXTURE_CONTROL0 + 112] = 0x60000;
			registers[NV4097_SET_TEXTURE_FILTER + 112] = 0x2052000;
			registers[NV4097_SET_TEXTURE_ADDRESS + 120] = 0x30101;
			registers[NV4097_SET_TEXTURE_BORDER_COLOR + 120] = 0x0;
			registers[NV4097_SET_TEXTURE_CONTROL0 + 120] = 0x60000;
			registers[NV4097_SET_TEXTURE_FILTER + 120] = 0x2052000;
			registers[NV4097_SET_TWO_SIDED_STENCIL_TEST_ENABLE] = 0x0;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT] = 0x2;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET] = 0x0;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 1] = 0x2;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 1] = 0x0;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 2] = 0x2;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 2] = 0x0;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 3] = 0x2;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 3] = 0x0;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 4] = 0x2;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 4] = 0x0;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 5] = 0x2;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 5] = 0x0;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 6] = 0x2;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 6] = 0x0;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 7] = 0x2;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 7] = 0x0;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 8] = 0x2;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 8] = 0x0;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 9] = 0x2;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 9] = 0x0;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 10] = 0x2;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 10] = 0x0;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 11] = 0x2;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 11] = 0x0;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 12] = 0x2;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 12] = 0x0;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 13] = 0x2;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 13] = 0x0;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 14] = 0x2;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 14] = 0x0;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 15] = 0x2;
			registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 15] = 0x0;
			registers[NV4097_SET_VIEWPORT_HORIZONTAL] = 0x10000000;
			registers[NV4097_SET_VIEWPORT_VERTICAL] = 0x10000000;
			registers[NV4097_SET_CLIP_MIN] = 0x0;
			registers[NV4097_SET_CLIP_MAX] = 0x3f800000;
			registers[NV4097_SET_VIEWPORT_OFFSET + 0] = 0x45000000;
			registers[NV4097_SET_VIEWPORT_OFFSET + 1] = 0x45000000;
			registers[NV4097_SET_VIEWPORT_OFFSET + 2] = 0x3f000000;
			registers[NV4097_SET_VIEWPORT_OFFSET + 3] = 0x0;
			registers[NV4097_SET_VIEWPORT_SCALE + 0] = 0x45000000;
			registers[NV4097_SET_VIEWPORT_SCALE + 1] = 0x45000000;
			registers[NV4097_SET_VIEWPORT_SCALE + 2] = 0x3f000000;
			registers[NV4097_SET_VIEWPORT_SCALE + 3] = 0x0;
			// NOTE: Realhw emits this sequence twice, likely to work around a hardware bug. Similar behavior can be seen in other buggy register blocks
			//registers[NV4097_SET_VIEWPORT_OFFSET + 0] = 0x45000000;
			//registers[NV4097_SET_VIEWPORT_OFFSET + 1] = 0x45000000;
			//registers[NV4097_SET_VIEWPORT_OFFSET + 2] = 0x3f000000;
			//registers[NV4097_SET_VIEWPORT_OFFSET + 3] = 0x0;
			//registers[NV4097_SET_VIEWPORT_SCALE + 0] = 0x45000000;
			//registers[NV4097_SET_VIEWPORT_SCALE + 1] = 0x45000000;
			//registers[NV4097_SET_VIEWPORT_SCALE + 2] = 0x3f000000;
			//registers[NV4097_SET_VIEWPORT_SCALE + 3] = 0x0;
			registers[NV4097_SET_ANTI_ALIASING_CONTROL] = 0xffff0000;
			registers[NV4097_SET_BACK_POLYGON_MODE] = 0x1b02;
			registers[NV4097_SET_COLOR_CLEAR_VALUE] = 0x0;
			registers[NV4097_SET_COLOR_MASK_MRT] = 0x0;
			registers[NV4097_SET_FRONT_POLYGON_MODE] = 0x1b02;
			registers[NV4097_SET_LINE_SMOOTH_ENABLE] = 0x0;
			registers[NV4097_SET_LINE_STIPPLE] = 0x0;
			registers[NV4097_SET_POINT_PARAMS_ENABLE] = 0x0;
			registers[NV4097_SET_POINT_SPRITE_CONTROL] = 0x0;
			registers[NV4097_SET_POLY_SMOOTH_ENABLE] = 0x0;
			registers[NV4097_SET_POLYGON_STIPPLE] = 0x0;
			registers[NV4097_SET_RENDER_ENABLE] = 0x1000000;
			registers[NV4097_SET_USER_CLIP_PLANE_CONTROL] = 0x0;
			registers[NV4097_SET_VERTEX_ATTRIB_INPUT_MASK] = 0xffff;
			registers[NV4097_SET_ZPASS_PIXEL_COUNT_ENABLE] = 0x0;
			registers[NV4097_SET_VERTEX_TEXTURE_ADDRESS] = 0x101;
			registers[NV4097_SET_VERTEX_TEXTURE_BORDER_COLOR] = 0x0;
			registers[NV4097_SET_VERTEX_TEXTURE_CONTROL0] = 0x60000;
			registers[NV4097_SET_VERTEX_TEXTURE_FILTER] = 0x0;
			registers[NV4097_SET_VERTEX_TEXTURE_ADDRESS + 8] = 0x101;
			registers[NV4097_SET_VERTEX_TEXTURE_BORDER_COLOR + 8] = 0x0;
			registers[NV4097_SET_VERTEX_TEXTURE_CONTROL0 + 8] = 0x60000;
			registers[NV4097_SET_VERTEX_TEXTURE_FILTER + 8] = 0x0;
			registers[NV4097_SET_VERTEX_TEXTURE_ADDRESS + 16] = 0x101;
			registers[NV4097_SET_VERTEX_TEXTURE_BORDER_COLOR + 16] = 0x0;
			registers[NV4097_SET_VERTEX_TEXTURE_CONTROL0 + 16] = 0x60000;
			registers[NV4097_SET_VERTEX_TEXTURE_FILTER + 16] = 0x0;
			registers[NV4097_SET_VERTEX_TEXTURE_ADDRESS + 24] = 0x101;
			registers[NV4097_SET_VERTEX_TEXTURE_BORDER_COLOR + 24] = 0x0;
			registers[NV4097_SET_VERTEX_TEXTURE_CONTROL0 + 24] = 0x60000;
			registers[NV4097_SET_VERTEX_TEXTURE_FILTER + 24] = 0x0;
			registers[NV4097_SET_CYLINDRICAL_WRAP] = 0x0;
			registers[NV4097_SET_ZMIN_MAX_CONTROL] = 0x1;
			registers[NV4097_SET_TWO_SIDE_LIGHT_EN] = 0x0;
			registers[NV4097_SET_TRANSFORM_BRANCH_BITS] = 0x0;
			registers[NV4097_SET_NO_PARANOID_TEXTURE_FETCHES] = 0x0;
			registers[0x2000 / 4] = 0x31337303;
			registers[0x2180 / 4] = 0x66604200;
			registers[0x2184 / 4] = 0xfeed0001;
			registers[0x2188 / 4] = 0xfeed0000;
			registers[NV3062_SET_OBJECT] = 0x313371c3;
			registers[NV3062_SET_CONTEXT_DMA_NOTIFIES] = 0x66604200;
			registers[NV3062_SET_CONTEXT_DMA_IMAGE_SOURCE] = 0xfeed0000;
			registers[NV3062_SET_CONTEXT_DMA_IMAGE_DESTIN] = 0xfeed0000;
			registers[0xa000 / 4] = 0x31337808;
			registers[0xa180 / 4] = 0x66604200;
			registers[0xa184 / 4] = 0x0;
			registers[0xa188 / 4] = 0x0;
			registers[0xa18c / 4] = 0x0;
			registers[0xa190 / 4] = 0x0;
			registers[0xa194 / 4] = 0x0;
			registers[0xa198 / 4] = 0x0;
			registers[0xa19c / 4] = 0x313371c3;
			registers[0xa2fc / 4] = 0x3;
			registers[0xa300 / 4] = 0x4;
			registers[0x8000 / 4] = 0x31337a73;
			registers[0x8180 / 4] = 0x66604200;
			registers[0x8184 / 4] = 0xfeed0000;
			registers[0xc000 / 4] = 0x3137af00;
			registers[0xc180 / 4] = 0x66604200;
			registers[NV4097_SET_ZCULL_EN] = 0x3;
			registers[NV4097_SET_ZCULL_STATS_ENABLE] = 0x0;
			registers[NV4097_SET_ZCULL_CONTROL0] = 0x10;
			registers[NV4097_SET_ZCULL_CONTROL1] = 0x1000100;
			registers[NV4097_SET_SCULL_CONTROL] = 0xff000002;
			registers[NV4097_SET_TEXTURE_OFFSET] = 0x0;
			registers[NV4097_SET_TEXTURE_FORMAT] = 0x18429;
			registers[NV4097_SET_TEXTURE_IMAGE_RECT] = 0x80008;
			registers[NV4097_SET_TEXTURE_CONTROL3] = 0x100008;
			registers[NV4097_SET_TEXTURE_CONTROL1] = 0xaae4;
			registers[NV4097_SET_TEXTURE_OFFSET + 8] = 0x0;
			registers[NV4097_SET_TEXTURE_FORMAT + 8] = 0x18429;
			registers[NV4097_SET_TEXTURE_IMAGE_RECT + 8] = 0x80008;
			registers[NV4097_SET_TEXTURE_CONTROL3 + 1] = 0x100008;
			registers[NV4097_SET_TEXTURE_CONTROL1 + 8] = 0xaae4;
			registers[NV4097_SET_TEXTURE_OFFSET + 16] = 0x0;
			registers[NV4097_SET_TEXTURE_FORMAT + 16] = 0x18429;
			registers[NV4097_SET_TEXTURE_IMAGE_RECT + 16] = 0x80008;
			registers[NV4097_SET_TEXTURE_CONTROL3 + 2] = 0x100008;
			registers[NV4097_SET_TEXTURE_CONTROL1 + 16] = 0xaae4;
			registers[NV4097_SET_TEXTURE_OFFSET + 24] = 0x0;
			registers[NV4097_SET_TEXTURE_FORMAT + 24] = 0x18429;
			registers[NV4097_SET_TEXTURE_IMAGE_RECT + 24] = 0x80008;
			registers[NV4097_SET_TEXTURE_CONTROL3 + 3] = 0x100008;
			registers[NV4097_SET_TEXTURE_CONTROL1 + 24] = 0xaae4;
			registers[NV4097_SET_TEXTURE_OFFSET + 32] = 0x0;
			registers[NV4097_SET_TEXTURE_FORMAT + 32] = 0x18429;
			registers[NV4097_SET_TEXTURE_IMAGE_RECT + 32] = 0x80008;
			registers[NV4097_SET_TEXTURE_CONTROL3 + 4] = 0x100008;
			registers[NV4097_SET_TEXTURE_CONTROL1 + 32] = 0xaae4;
			registers[NV4097_SET_TEXTURE_OFFSET + 40] = 0x0;
			registers[NV4097_SET_TEXTURE_FORMAT + 40] = 0x18429;
			registers[NV4097_SET_TEXTURE_IMAGE_RECT + 40] = 0x80008;
			registers[NV4097_SET_TEXTURE_CONTROL3 + 5] = 0x100008;
			registers[NV4097_SET_TEXTURE_CONTROL1 + 40] = 0xaae4;
			registers[NV4097_SET_TEXTURE_OFFSET + 48] = 0x0;
			registers[NV4097_SET_TEXTURE_FORMAT + 48] = 0x18429;
			registers[NV4097_SET_TEXTURE_IMAGE_RECT + 48] = 0x80008;
			registers[NV4097_SET_TEXTURE_CONTROL3 + 6] = 0x100008;
			registers[NV4097_SET_TEXTURE_CONTROL1 + 48] = 0xaae4;
			registers[NV4097_SET_TEXTURE_OFFSET + 56] = 0x0;
			registers[NV4097_SET_TEXTURE_FORMAT + 56] = 0x18429;
			registers[NV4097_SET_TEXTURE_IMAGE_RECT + 56] = 0x80008;
			registers[NV4097_SET_TEXTURE_CONTROL3 + 7] = 0x100008;
			registers[NV4097_SET_TEXTURE_CONTROL1 + 56] = 0xaae4;
			registers[NV4097_SET_TEXTURE_OFFSET + 64] = 0x0;
			registers[NV4097_SET_TEXTURE_FORMAT + 64] = 0x18429;
			registers[NV4097_SET_TEXTURE_IMAGE_RECT + 64] = 0x80008;
			registers[NV4097_SET_TEXTURE_CONTROL3 + 8] = 0x100008;
			registers[NV4097_SET_TEXTURE_CONTROL1 + 64] = 0xaae4;
			registers[NV4097_SET_TEXTURE_OFFSET + 72] = 0x0;
			registers[NV4097_SET_TEXTURE_FORMAT + 72] = 0x18429;
			registers[NV4097_SET_TEXTURE_IMAGE_RECT + 72] = 0x80008;
			registers[NV4097_SET_TEXTURE_CONTROL3 + 9] = 0x100008;
			registers[NV4097_SET_TEXTURE_CONTROL1 + 72] = 0xaae4;
			registers[NV4097_SET_TEXTURE_OFFSET + 80] = 0x0;
			registers[NV4097_SET_TEXTURE_FORMAT + 80] = 0x18429;
			registers[NV4097_SET_TEXTURE_IMAGE_RECT + 80] = 0x80008;
			registers[NV4097_SET_TEXTURE_CONTROL3 + 10] = 0x100008;
			registers[NV4097_SET_TEXTURE_CONTROL1 + 80] = 0xaae4;
			registers[NV4097_SET_TEXTURE_OFFSET + 88] = 0x0;
			registers[NV4097_SET_TEXTURE_FORMAT + 88] = 0x18429;
			registers[NV4097_SET_TEXTURE_IMAGE_RECT + 88] = 0x80008;
			registers[NV4097_SET_TEXTURE_CONTROL3 + 11] = 0x100008;
			registers[NV4097_SET_TEXTURE_CONTROL1 + 88] = 0xaae4;
			registers[NV4097_SET_TEXTURE_OFFSET + 96] = 0x0;
			registers[NV4097_SET_TEXTURE_FORMAT + 96] = 0x18429;
			registers[NV4097_SET_TEXTURE_IMAGE_RECT + 96] = 0x80008;
			registers[NV4097_SET_TEXTURE_CONTROL3 + 12] = 0x100008;
			registers[NV4097_SET_TEXTURE_CONTROL1 + 96] = 0xaae4;
			registers[NV4097_SET_TEXTURE_OFFSET + 104] = 0x0;
			registers[NV4097_SET_TEXTURE_FORMAT + 104] = 0x18429;
			registers[NV4097_SET_TEXTURE_IMAGE_RECT + 104] = 0x80008;
			registers[NV4097_SET_TEXTURE_CONTROL3 + 13] = 0x100008;
			registers[NV4097_SET_TEXTURE_CONTROL1 + 104] = 0xaae4;
			registers[NV4097_SET_TEXTURE_OFFSET + 112] = 0x0;
			registers[NV4097_SET_TEXTURE_FORMAT + 112] = 0x18429;
			registers[NV4097_SET_TEXTURE_IMAGE_RECT + 112] = 0x80008;
			registers[NV4097_SET_TEXTURE_CONTROL3 + 14] = 0x100008;
			registers[NV4097_SET_TEXTURE_CONTROL1 + 112] = 0xaae4;
			registers[NV4097_SET_TEXTURE_OFFSET + 120] = 0x0;
			registers[NV4097_SET_TEXTURE_FORMAT + 120] = 0x18429;
			registers[NV4097_SET_TEXTURE_IMAGE_RECT + 120] = 0x80008;
			registers[NV4097_SET_TEXTURE_CONTROL3 + 15] = 0x100008;
			registers[NV4097_SET_TEXTURE_CONTROL1 + 120] = 0xaae4;
			registers[NV4097_SET_VERTEX_TEXTURE_OFFSET] = 0x0;
			registers[NV4097_SET_VERTEX_TEXTURE_FORMAT] = 0x1bc21;
			registers[NV4097_SET_VERTEX_TEXTURE_CONTROL3] = 0x8;
			registers[NV4097_SET_VERTEX_TEXTURE_IMAGE_RECT] = 0x80008;
			registers[NV4097_SET_VERTEX_TEXTURE_OFFSET + 8] = 0x0;
			registers[NV4097_SET_VERTEX_TEXTURE_FORMAT + 8] = 0x1bc21;
			registers[NV4097_SET_VERTEX_TEXTURE_CONTROL3 + 8] = 0x8;
			registers[NV4097_SET_VERTEX_TEXTURE_IMAGE_RECT + 8] = 0x80008;
			registers[NV4097_SET_VERTEX_TEXTURE_OFFSET + 16] = 0x0;
			registers[NV4097_SET_VERTEX_TEXTURE_FORMAT + 16] = 0x1bc21;
			registers[NV4097_SET_VERTEX_TEXTURE_CONTROL3 + 16] = 0x8;
			registers[NV4097_SET_VERTEX_TEXTURE_IMAGE_RECT + 16] = 0x80008;
			registers[NV4097_SET_VERTEX_TEXTURE_OFFSET + 24] = 0x0;
			registers[NV4097_SET_VERTEX_TEXTURE_FORMAT + 24] = 0x1bc21;
			registers[NV4097_SET_VERTEX_TEXTURE_CONTROL3 + 24] = 0x8;
			registers[NV4097_SET_VERTEX_TEXTURE_IMAGE_RECT + 24] = 0x80008;
			registers[0x230c / 4] = 0x0;
			registers[0x2310 / 4] = 0x0;
			registers[0x2314 / 4] = 0x0;
			registers[0x2318 / 4] = 0x0;
			registers[0x231c / 4] = 0x0;
			registers[0x2320 / 4] = 0x0;
			registers[0x2324 / 4] = 0x101;
			registers[NV3062_SET_COLOR_FORMAT] = 0xa;
			registers[NV3062_SET_PITCH] = 0x400040;
			registers[NV3062_SET_OFFSET_SOURCE] = 0x0;
			registers[NV3062_SET_OFFSET_DESTIN] = 0x0;
			registers[0x8300 / 4] = 0x1000a;
			registers[0x8304 / 4] = 0x0;
			registers[0xc184 / 4] = 0xfeed0000;
			registers[0xc198 / 4] = 0x313371c3;
			registers[0xc2fc / 4] = 0x1;
			registers[0xc300 / 4] = 0x3;
			registers[0xc304 / 4] = 0x3;
			registers[0xc308 / 4] = 0x0;
			registers[0xc30c / 4] = 0x0;
			registers[0xc310 / 4] = 0x0;
			registers[0xc314 / 4] = 0x0;
			registers[0xc318 / 4] = 0x0;
			registers[0xc31c / 4] = 0x0;
			registers[0xc400 / 4] = 0x10002;
			registers[0xc404 / 4] = 0x10000;
			registers[0xc408 / 4] = 0x0;
			registers[0xc40c / 4] = 0x0;
			registers[NV308A_POINT] = 0x0;
			registers[NV308A_SIZE_OUT] = 0x0;
			registers[NV308A_SIZE_IN] = 0x0;
			registers[NV406E_SET_REFERENCE] = umax;

			if (auto rsx = Emu.IsStopped() ? nullptr : get_current_renderer(); rsx && rsx->ctrl)
			{
				// FIXME: Multi-context unaware
				rsx->ctrl->ref = u32{ umax };
			}
		}

		{
			// Signal definitions
			state_signals[NV4097_SET_SHADER_CONTROL] = rsx::fragment_program_state_dirty;
			state_signals[NV4097_SET_TEX_COORD_CONTROL + 0] = rsx::fragment_program_state_dirty;
			state_signals[NV4097_SET_TEX_COORD_CONTROL + 1] = rsx::fragment_program_state_dirty;
			state_signals[NV4097_SET_TEX_COORD_CONTROL + 2] = rsx::fragment_program_state_dirty;
			state_signals[NV4097_SET_TEX_COORD_CONTROL + 3] = rsx::fragment_program_state_dirty;
			state_signals[NV4097_SET_TEX_COORD_CONTROL + 4] = rsx::fragment_program_state_dirty;
			state_signals[NV4097_SET_TEX_COORD_CONTROL + 5] = rsx::fragment_program_state_dirty;
			state_signals[NV4097_SET_TEX_COORD_CONTROL + 6] = rsx::fragment_program_state_dirty;
			state_signals[NV4097_SET_TEX_COORD_CONTROL + 7] = rsx::fragment_program_state_dirty;
			state_signals[NV4097_SET_TEX_COORD_CONTROL + 8] = rsx::fragment_program_state_dirty;
			state_signals[NV4097_SET_TEX_COORD_CONTROL + 9] = rsx::fragment_program_state_dirty;
			state_signals[NV4097_SET_TWO_SIDE_LIGHT_EN] = rsx::fragment_program_state_dirty;
			state_signals[NV4097_SET_POINT_SPRITE_CONTROL] = rsx::fragment_program_state_dirty;
			state_signals[NV4097_SET_USER_CLIP_PLANE_CONTROL] = rsx::vertex_state_dirty;
			state_signals[NV4097_SET_TRANSFORM_BRANCH_BITS] = rsx::vertex_state_dirty;
			state_signals[NV4097_SET_CLIP_MIN] = rsx::invalidate_zclip_bits;
			state_signals[NV4097_SET_CLIP_MAX] = rsx::invalidate_zclip_bits;
			state_signals[NV4097_SET_POINT_SIZE] = rsx::vertex_state_dirty;
			state_signals[NV4097_SET_ALPHA_FUNC] = rsx::fragment_state_dirty;
			state_signals[NV4097_SET_ALPHA_REF] = rsx::fragment_state_dirty;
			state_signals[NV4097_SET_ALPHA_TEST_ENABLE] = rsx::fragment_state_dirty;
			state_signals[NV4097_SET_ANTI_ALIASING_CONTROL] = rsx::fragment_state_dirty | rsx::pipeline_config_dirty;
			state_signals[NV4097_SET_SHADER_PACKER] = rsx::fragment_state_dirty;
			state_signals[NV4097_SET_SHADER_WINDOW] = rsx::fragment_state_dirty;
			state_signals[NV4097_SET_FOG_MODE] = rsx::fragment_state_dirty;
			state_signals[NV4097_SET_SCISSOR_HORIZONTAL] = rsx::scissor_config_state_dirty;
			state_signals[NV4097_SET_SCISSOR_VERTICAL] = rsx::scissor_config_state_dirty;
			state_signals[NV4097_SET_VIEWPORT_HORIZONTAL] = rsx::scissor_config_state_dirty;
			state_signals[NV4097_SET_VIEWPORT_VERTICAL] = rsx::scissor_config_state_dirty;
			state_signals[NV4097_SET_FOG_PARAMS + 0] = rsx::fragment_state_dirty;
			state_signals[NV4097_SET_FOG_PARAMS + 1] = rsx::fragment_state_dirty;
			state_signals[NV4097_SET_VIEWPORT_SCALE + 0] = rsx::vertex_state_dirty;
			state_signals[NV4097_SET_VIEWPORT_SCALE + 1] = rsx::vertex_state_dirty;
			state_signals[NV4097_SET_VIEWPORT_SCALE + 2] = rsx::vertex_state_dirty;
			state_signals[NV4097_SET_VIEWPORT_OFFSET + 0] = rsx::vertex_state_dirty;
			state_signals[NV4097_SET_VIEWPORT_OFFSET + 1] = rsx::vertex_state_dirty;
			state_signals[NV4097_SET_VIEWPORT_OFFSET + 2] = rsx::vertex_state_dirty;
			state_signals[NV4097_SET_POLYGON_STIPPLE] = rsx::fragment_state_dirty;
			state_signals[NV4097_SET_POLYGON_STIPPLE_PATTERN + 0] = rsx::polygon_stipple_pattern_dirty;
			state_signals[NV4097_SET_POLYGON_STIPPLE_PATTERN + 1] = rsx::polygon_stipple_pattern_dirty;
			state_signals[NV4097_SET_POLYGON_STIPPLE_PATTERN + 2] = rsx::polygon_stipple_pattern_dirty;
			state_signals[NV4097_SET_POLYGON_STIPPLE_PATTERN + 3] = rsx::polygon_stipple_pattern_dirty;
			state_signals[NV4097_SET_POLYGON_STIPPLE_PATTERN + 4] = rsx::polygon_stipple_pattern_dirty;
			state_signals[NV4097_SET_POLYGON_STIPPLE_PATTERN + 5] = rsx::polygon_stipple_pattern_dirty;
			state_signals[NV4097_SET_POLYGON_STIPPLE_PATTERN + 6] = rsx::polygon_stipple_pattern_dirty;
			state_signals[NV4097_SET_POLYGON_STIPPLE_PATTERN + 7] = rsx::polygon_stipple_pattern_dirty;
			state_signals[NV4097_SET_POLYGON_STIPPLE_PATTERN + 8] = rsx::polygon_stipple_pattern_dirty;
			state_signals[NV4097_SET_POLYGON_STIPPLE_PATTERN + 9] = rsx::polygon_stipple_pattern_dirty;
			state_signals[NV4097_SET_POLYGON_STIPPLE_PATTERN + 10] = rsx::polygon_stipple_pattern_dirty;
			state_signals[NV4097_SET_POLYGON_STIPPLE_PATTERN + 11] = rsx::polygon_stipple_pattern_dirty;
			state_signals[NV4097_SET_POLYGON_STIPPLE_PATTERN + 12] = rsx::polygon_stipple_pattern_dirty;
			state_signals[NV4097_SET_POLYGON_STIPPLE_PATTERN + 13] = rsx::polygon_stipple_pattern_dirty;
			state_signals[NV4097_SET_POLYGON_STIPPLE_PATTERN + 14] = rsx::polygon_stipple_pattern_dirty;
			state_signals[NV4097_SET_POLYGON_STIPPLE_PATTERN + 15] = rsx::polygon_stipple_pattern_dirty;
			state_signals[NV4097_SET_POLYGON_STIPPLE_PATTERN + 16] = rsx::polygon_stipple_pattern_dirty;
			state_signals[NV4097_SET_POLYGON_STIPPLE_PATTERN + 17] = rsx::polygon_stipple_pattern_dirty;
			state_signals[NV4097_SET_POLYGON_STIPPLE_PATTERN + 18] = rsx::polygon_stipple_pattern_dirty;
			state_signals[NV4097_SET_POLYGON_STIPPLE_PATTERN + 19] = rsx::polygon_stipple_pattern_dirty;
			state_signals[NV4097_SET_POLYGON_STIPPLE_PATTERN + 20] = rsx::polygon_stipple_pattern_dirty;
			state_signals[NV4097_SET_POLYGON_STIPPLE_PATTERN + 21] = rsx::polygon_stipple_pattern_dirty;
			state_signals[NV4097_SET_POLYGON_STIPPLE_PATTERN + 22] = rsx::polygon_stipple_pattern_dirty;
			state_signals[NV4097_SET_POLYGON_STIPPLE_PATTERN + 23] = rsx::polygon_stipple_pattern_dirty;
			state_signals[NV4097_SET_POLYGON_STIPPLE_PATTERN + 24] = rsx::polygon_stipple_pattern_dirty;
			state_signals[NV4097_SET_POLYGON_STIPPLE_PATTERN + 25] = rsx::polygon_stipple_pattern_dirty;
			state_signals[NV4097_SET_POLYGON_STIPPLE_PATTERN + 26] = rsx::polygon_stipple_pattern_dirty;
			state_signals[NV4097_SET_POLYGON_STIPPLE_PATTERN + 27] = rsx::polygon_stipple_pattern_dirty;
			state_signals[NV4097_SET_POLYGON_STIPPLE_PATTERN + 28] = rsx::polygon_stipple_pattern_dirty;
			state_signals[NV4097_SET_POLYGON_STIPPLE_PATTERN + 29] = rsx::polygon_stipple_pattern_dirty;
			state_signals[NV4097_SET_POLYGON_STIPPLE_PATTERN + 30] = rsx::polygon_stipple_pattern_dirty;
			state_signals[NV4097_SET_POLYGON_STIPPLE_PATTERN + 31] = rsx::polygon_stipple_pattern_dirty;
			state_signals[NV4097_SET_POLY_OFFSET_FILL_ENABLE] = rsx::polygon_offset_state_dirty;
			state_signals[NV4097_SET_POLYGON_OFFSET_SCALE_FACTOR] = rsx::polygon_offset_state_dirty;
			state_signals[NV4097_SET_POLYGON_OFFSET_BIAS] = rsx::polygon_offset_state_dirty;
			state_signals[NV4097_SET_DEPTH_BOUNDS_TEST_ENABLE] = rsx::depth_bounds_state_dirty;
			state_signals[NV4097_SET_DEPTH_BOUNDS_MIN] = rsx::depth_bounds_state_dirty;
			state_signals[NV4097_SET_DEPTH_BOUNDS_MAX] = rsx::depth_bounds_state_dirty;
			state_signals[NV4097_SET_CULL_FACE_ENABLE] = rsx::pipeline_config_dirty;
			state_signals[NV4097_SET_ZMIN_MAX_CONTROL] = rsx::pipeline_config_dirty;
			state_signals[NV4097_SET_LOGIC_OP_ENABLE] = rsx::pipeline_config_dirty;
			state_signals[NV4097_SET_LOGIC_OP] = rsx::pipeline_config_dirty;
			state_signals[NV4097_SET_BLEND_ENABLE] = rsx::pipeline_config_dirty;
			state_signals[NV4097_SET_BLEND_ENABLE_MRT] = rsx::pipeline_config_dirty;
			state_signals[NV4097_SET_STENCIL_FUNC] = rsx::pipeline_config_dirty;
			state_signals[NV4097_SET_BACK_STENCIL_FUNC] = rsx::pipeline_config_dirty;
			state_signals[NV4097_SET_RESTART_INDEX_ENABLE] = rsx::pipeline_config_dirty;
		}

		// Sanity checks
		for (size_t id = 0; id < methods.size(); ++id)
		{
			if (methods[id] && state_signals[id])
			{
				rsx_log.error("FIXME: Method register 0x%x is registered as a method and signal. The signal will be ignored.");
			}
		}
	}

	void rsx_state::reset()
	{
		// TODO: Name unnamed registers and constants, better group methods
		registers[NV406E_SET_CONTEXT_DMA_SEMAPHORE] = 0x56616661;

		registers[NV4097_SET_OBJECT] = 0x31337000;
		registers[NV4097_SET_CONTEXT_DMA_NOTIFIES] = 0x66604200;
		registers[NV4097_SET_CONTEXT_DMA_A] = 0xfeed0000;
		registers[NV4097_SET_CONTEXT_DMA_B] = 0xfeed0001;
		registers[NV4097_SET_CONTEXT_DMA_COLOR_B] = 0xfeed0000;
		registers[NV4097_SET_CONTEXT_DMA_STATE] = 0x0;
		registers[NV4097_SET_CONTEXT_DMA_COLOR_A] = 0xfeed0000;
		registers[NV4097_SET_CONTEXT_DMA_ZETA] = 0xfeed0000;
		registers[NV4097_SET_CONTEXT_DMA_VERTEX_A] = 0xfeed0000;
		registers[NV4097_SET_CONTEXT_DMA_VERTEX_B] = 0xfeed0001;
		registers[NV4097_SET_CONTEXT_DMA_SEMAPHORE] = 0x66606660;
		registers[NV4097_SET_CONTEXT_DMA_REPORT] = 0x66626660;
		registers[NV4097_SET_CONTEXT_DMA_CLIP_ID] = 0x0;
		registers[NV4097_SET_CONTEXT_DMA_CULL_DATA] = 0x0;
		registers[NV4097_SET_CONTEXT_DMA_COLOR_C] = 0xfeed0000;
		registers[NV4097_SET_CONTEXT_DMA_COLOR_D] = 0xfeed0000;
		registers[NV406E_SET_CONTEXT_DMA_SEMAPHORE] = 0x66616661;
		registers[NV4097_SET_SURFACE_CLIP_HORIZONTAL] = 0x0;
		registers[NV4097_SET_SURFACE_CLIP_VERTICAL] = 0x0;
		registers[NV4097_SET_SURFACE_FORMAT] = 0x121;
		registers[NV4097_SET_SURFACE_PITCH_A] = 0x40;
		registers[NV4097_SET_SURFACE_COLOR_AOFFSET] = 0x0;
		registers[NV4097_SET_SURFACE_ZETA_OFFSET] = 0x0;
		registers[NV4097_SET_SURFACE_COLOR_BOFFSET] = 0x0;
		registers[NV4097_SET_SURFACE_PITCH_B] = 0x40;
		registers[NV4097_SET_SURFACE_COLOR_TARGET] = 0x1;
		registers[0x224 / 4] = 0x80;
		registers[0x228 / 4] = 0x100;
		registers[NV4097_SET_SURFACE_PITCH_Z] = 0x40;
		registers[0x230 / 4] = 0x0;
		registers[NV4097_SET_SURFACE_PITCH_C] = 0x40;
		registers[NV4097_SET_SURFACE_PITCH_D] = 0x40;
		registers[NV4097_SET_SURFACE_COLOR_COFFSET] = 0x0;
		registers[NV4097_SET_SURFACE_COLOR_DOFFSET] = 0x0;
		registers[0x1d80 / 4] = 0x3;
		registers[NV4097_SET_WINDOW_OFFSET] = 0x0;
		registers[0x02bc / 4] = 0x0;
		registers[0x02c0 / 4] = 0xfff0000;
		registers[0x02c4 / 4] = 0xfff0000;
		registers[0x02c8 / 4] = 0xfff0000;
		registers[0x02cc / 4] = 0xfff0000;
		registers[0x02d0 / 4] = 0xfff0000;
		registers[0x02d4 / 4] = 0xfff0000;
		registers[0x02d8 / 4] = 0xfff0000;
		registers[0x02dc / 4] = 0xfff0000;
		registers[0x02e0 / 4] = 0xfff0000;
		registers[0x02e4 / 4] = 0xfff0000;
		registers[0x02e8 / 4] = 0xfff0000;
		registers[0x02ec / 4] = 0xfff0000;
		registers[0x02f0 / 4] = 0xfff0000;
		registers[0x02f4 / 4] = 0xfff0000;
		registers[0x02f8 / 4] = 0xfff0000;
		registers[0x02fc / 4] = 0xfff0000;
		registers[0x1d98 / 4] = 0xfff0000;
		registers[0x1d9c / 4] = 0xfff0000;
		registers[0x1da4 / 4] = 0x0;
		registers[NV4097_SET_CONTROL0] = 0x100000;
		registers[0x1454 / 4] = 0x0;
		registers[NV4097_SET_VERTEX_ATTRIB_OUTPUT_MASK] = 0x3fffff;
		registers[NV4097_SET_FREQUENCY_DIVIDER_OPERATION] = 0x0;
		registers[NV4097_SET_ATTRIB_COLOR] = 0x6144321;
		registers[NV4097_SET_ATTRIB_TEX_COORD] = 0xedcba987;
		registers[NV4097_SET_ATTRIB_TEX_COORD_EX] = 0x6f;
		registers[NV4097_SET_ATTRIB_UCLIP0] = 0x171615;
		registers[NV4097_SET_ATTRIB_UCLIP1] = 0x1b1a19;
		registers[NV4097_SET_TEX_COORD_CONTROL] = 0x0;
		registers[NV4097_SET_TEX_COORD_CONTROL + 1] = 0x0;
		registers[NV4097_SET_TEX_COORD_CONTROL + 2] = 0x0;
		registers[NV4097_SET_TEX_COORD_CONTROL + 3] = 0x0;
		registers[NV4097_SET_TEX_COORD_CONTROL + 4] = 0x0;
		registers[NV4097_SET_TEX_COORD_CONTROL + 5] = 0x0;
		registers[NV4097_SET_TEX_COORD_CONTROL + 6] = 0x0;
		registers[NV4097_SET_TEX_COORD_CONTROL + 7] = 0x0;
		registers[NV4097_SET_TEX_COORD_CONTROL + 8] = 0x0;
		registers[NV4097_SET_TEX_COORD_CONTROL + 9] = 0x0;
		registers[0xa0c / 4] = 0x0;
		registers[0xa60 / 4] = 0x0;
		registers[NV4097_SET_POLY_OFFSET_LINE_ENABLE] = 0x0;
		registers[NV4097_SET_POLY_OFFSET_FILL_ENABLE] = 0x0;
		registers[NV4097_SET_POLYGON_OFFSET_SCALE_FACTOR] = 0x0;
		registers[NV4097_SET_POLYGON_OFFSET_BIAS] = 0x0;
		registers[0x1428 / 4] = 0x1;
		registers[NV4097_SET_SHADER_WINDOW] = 0x1000;
		registers[0x1e94 / 4] = 0x11;
		registers[0x1450 / 4] = 0x80003;
		registers[0x1d64 / 4] = 0x2000000;
		registers[0x145c / 4] = 0x1;
		registers[NV4097_SET_REDUCE_DST_COLOR] = 0x1;
		registers[NV4097_SET_TEXTURE_CONTROL2] = 0x2dc8;
		registers[NV4097_SET_TEXTURE_CONTROL2 + 1] = 0x2dc8;
		registers[NV4097_SET_TEXTURE_CONTROL2 + 2] = 0x2dc8;
		registers[NV4097_SET_TEXTURE_CONTROL2 + 3] = 0x2dc8;
		registers[NV4097_SET_TEXTURE_CONTROL2 + 4] = 0x2dc8;
		registers[NV4097_SET_TEXTURE_CONTROL2 + 5] = 0x2dc8;
		registers[NV4097_SET_TEXTURE_CONTROL2 + 6] = 0x2dc8;
		registers[NV4097_SET_TEXTURE_CONTROL2 + 7] = 0x2dc8;
		registers[NV4097_SET_TEXTURE_CONTROL2 + 8] = 0x2dc8;
		registers[NV4097_SET_TEXTURE_CONTROL2 + 9] = 0x2dc8;
		registers[NV4097_SET_TEXTURE_CONTROL2 + 10] = 0x2dc8;
		registers[NV4097_SET_TEXTURE_CONTROL2 + 11] = 0x2dc8;
		registers[NV4097_SET_TEXTURE_CONTROL2 + 12] = 0x2dc8;
		registers[NV4097_SET_TEXTURE_CONTROL2 + 13] = 0x2dc8;
		registers[NV4097_SET_TEXTURE_CONTROL2 + 14] = 0x2dc8;
		registers[NV4097_SET_TEXTURE_CONTROL2 + 15] = 0x2dc8;
		registers[NV4097_SET_FOG_MODE] = 0x800;
		registers[NV4097_SET_FOG_PARAMS] = 0x0;
		registers[NV4097_SET_FOG_PARAMS + 1] = 0x0;
		registers[NV4097_SET_FOG_PARAMS + 2] = 0x0;
		registers[0x240 / 4] = 0xffff;
		registers[0x244 / 4] = 0x0;
		registers[0x248 / 4] = 0x0;
		registers[0x24c / 4] = 0x0;
		registers[NV4097_SET_ANISO_SPREAD] = 0x10101;
		registers[NV4097_SET_ANISO_SPREAD + 1] = 0x10101;
		registers[NV4097_SET_ANISO_SPREAD + 2] = 0x10101;
		registers[NV4097_SET_ANISO_SPREAD + 3] = 0x10101;
		registers[NV4097_SET_ANISO_SPREAD + 4] = 0x10101;
		registers[NV4097_SET_ANISO_SPREAD + 5] = 0x10101;
		registers[NV4097_SET_ANISO_SPREAD + 6] = 0x10101;
		registers[NV4097_SET_ANISO_SPREAD + 7] = 0x10101;
		registers[NV4097_SET_ANISO_SPREAD + 8] = 0x10101;
		registers[NV4097_SET_ANISO_SPREAD + 9] = 0x10101;
		registers[NV4097_SET_ANISO_SPREAD + 10] = 0x10101;
		registers[NV4097_SET_ANISO_SPREAD + 11] = 0x10101;
		registers[NV4097_SET_ANISO_SPREAD + 12] = 0x10101;
		registers[NV4097_SET_ANISO_SPREAD + 13] = 0x10101;
		registers[NV4097_SET_ANISO_SPREAD + 14] = 0x10101;
		registers[NV4097_SET_ANISO_SPREAD + 15] = 0x10101;
		registers[0x400 / 4] = 0x7421;
		registers[0x404 / 4] = 0x7421;
		registers[0x408 / 4] = 0x7421;
		registers[0x40c / 4] = 0x7421;
		registers[0x410 / 4] = 0x7421;
		registers[0x414 / 4] = 0x7421;
		registers[0x418 / 4] = 0x7421;
		registers[0x41c / 4] = 0x7421;
		registers[0x420 / 4] = 0x7421;
		registers[0x424 / 4] = 0x7421;
		registers[0x428 / 4] = 0x7421;
		registers[0x42c / 4] = 0x7421;
		registers[0x430 / 4] = 0x7421;
		registers[0x434 / 4] = 0x7421;
		registers[0x438 / 4] = 0x7421;
		registers[0x43c / 4] = 0x7421;
		registers[0x440 / 4] = 0x9aabaa98;
		registers[0x444 / 4] = 0x66666789;
		registers[0x448 / 4] = 0x98766666;
		registers[0x44c / 4] = 0x89aabaa9;
		registers[0x450 / 4] = 0x99999999;
		registers[0x454 / 4] = 0x88888889;
		registers[0x458 / 4] = 0x98888888;
		registers[0x45c / 4] = 0x99999999;
		registers[0x460 / 4] = 0x56676654;
		registers[0x464 / 4] = 0x33333345;
		registers[0x468 / 4] = 0x54333333;
		registers[0x46c / 4] = 0x45667665;
		registers[0x470 / 4] = 0xaabbba99;
		registers[0x474 / 4] = 0x66667899;
		registers[0x478 / 4] = 0x99876666;
		registers[0x47c / 4] = 0x99abbbaa;
		registers[NV4097_SET_VERTEX_DATA_BASE_OFFSET] = 0x0;
		registers[NV4097_SET_VERTEX_DATA_BASE_INDEX] = 0x0;
		registers[GCM_SET_DRIVER_OBJECT] = 0xcafebabe;
		registers[NV4097_SET_ALPHA_FUNC] = 0x207;
		registers[NV4097_SET_ALPHA_REF] = 0x0;
		registers[NV4097_SET_ALPHA_TEST_ENABLE] = 0x0;
		registers[NV4097_SET_BACK_STENCIL_FUNC] = 0x207;
		registers[NV4097_SET_BACK_STENCIL_FUNC_REF] = 0x0;
		registers[NV4097_SET_BACK_STENCIL_FUNC_MASK] = 0xff;
		registers[NV4097_SET_BACK_STENCIL_MASK] = 0xff;
		registers[NV4097_SET_BACK_STENCIL_OP_FAIL] = 0x1e00;
		registers[NV4097_SET_BACK_STENCIL_OP_ZFAIL] = 0x1e00;
		registers[NV4097_SET_BACK_STENCIL_OP_ZPASS] = 0x1e00;
		registers[NV4097_SET_BLEND_COLOR] = 0x0;
		registers[NV4097_SET_BLEND_COLOR2] = 0x0;
		registers[NV4097_SET_BLEND_ENABLE] = 0x0;
		registers[NV4097_SET_BLEND_ENABLE_MRT] = 0x0;
		registers[NV4097_SET_BLEND_EQUATION] = 0x80068006;
		registers[NV4097_SET_BLEND_FUNC_SFACTOR] = 0x10001;
		registers[NV4097_SET_BLEND_FUNC_DFACTOR] = 0x0;
		registers[NV4097_SET_ZSTENCIL_CLEAR_VALUE] = 0xffffff00;
		registers[NV4097_CLEAR_SURFACE] = 0x0;
		registers[NV4097_NO_OPERATION] = 0x0;
		registers[NV4097_SET_COLOR_MASK] = 0x1010101;
		registers[NV4097_SET_CULL_FACE_ENABLE] = 0x0;
		registers[NV4097_SET_CULL_FACE] = 0x405;
		registers[NV4097_SET_DEPTH_BOUNDS_MIN] = 0x0;
		registers[NV4097_SET_DEPTH_BOUNDS_MAX] = 0x3f800000;
		registers[NV4097_SET_DEPTH_BOUNDS_TEST_ENABLE] = 0x0;
		registers[NV4097_SET_DEPTH_FUNC] = 0x201;
		registers[NV4097_SET_DEPTH_MASK] = 0x1;
		registers[NV4097_SET_DEPTH_TEST_ENABLE] = 0x0;
		registers[NV4097_SET_DITHER_ENABLE] = 0x1;
		registers[NV4097_SET_SHADER_PACKER] = 0x0;
		registers[NV4097_SET_FREQUENCY_DIVIDER_OPERATION] = 0x0;
		registers[NV4097_SET_FRONT_FACE] = 0x901;
		registers[NV4097_SET_LINE_WIDTH] = 0x8;
		registers[NV4097_SET_LOGIC_OP_ENABLE] = 0x0;
		registers[NV4097_SET_LOGIC_OP] = 0x1503;
		registers[NV4097_SET_POINT_SIZE] = 0x3f800000;
		registers[NV4097_SET_POLY_OFFSET_FILL_ENABLE] = 0x0;
		registers[NV4097_SET_POLYGON_OFFSET_SCALE_FACTOR] = 0x0;
		registers[NV4097_SET_POLYGON_OFFSET_BIAS] = 0x0;
		registers[NV4097_SET_RESTART_INDEX_ENABLE] = 0x0;
		registers[NV4097_SET_RESTART_INDEX] = 0xffffffff;
		registers[NV4097_SET_SCISSOR_HORIZONTAL] = 0x10000000;
		registers[NV4097_SET_SCISSOR_VERTICAL] = 0x10000000;
		registers[NV4097_SET_SHADE_MODE] = 0x1d01;
		registers[NV4097_SET_STENCIL_FUNC] = 0x207;
		registers[NV4097_SET_STENCIL_FUNC_REF] = 0x0;
		registers[NV4097_SET_STENCIL_FUNC_MASK] = 0xff;
		registers[NV4097_SET_STENCIL_MASK] = 0xff;
		registers[NV4097_SET_STENCIL_OP_FAIL] = 0x1e00;
		registers[NV4097_SET_STENCIL_OP_ZFAIL] = 0x1e00;
		registers[NV4097_SET_STENCIL_OP_ZPASS] = 0x1e00;
		registers[NV4097_SET_STENCIL_TEST_ENABLE] = 0x0;
		registers[NV4097_SET_TEXTURE_ADDRESS] = 0x30101;
		registers[NV4097_SET_TEXTURE_BORDER_COLOR] = 0x0;
		registers[NV4097_SET_TEXTURE_CONTROL0] = 0x60000;
		registers[NV4097_SET_TEXTURE_FILTER] = 0x2052000;
		registers[NV4097_SET_TEXTURE_ADDRESS + 8] = 0x30101;
		registers[NV4097_SET_TEXTURE_BORDER_COLOR + 8] = 0x0;
		registers[NV4097_SET_TEXTURE_CONTROL0 + 8] = 0x60000;
		registers[NV4097_SET_TEXTURE_FILTER + 8] = 0x2052000;
		registers[NV4097_SET_TEXTURE_ADDRESS + 8] = 0x30101;
		registers[NV4097_SET_TEXTURE_BORDER_COLOR + 16] = 0x0;
		registers[NV4097_SET_TEXTURE_CONTROL0 + 16] = 0x60000;
		registers[NV4097_SET_TEXTURE_FILTER + 16] = 0x2052000;
		registers[NV4097_SET_TEXTURE_ADDRESS + 24] = 0x30101;
		registers[NV4097_SET_TEXTURE_BORDER_COLOR + 24] = 0x0;
		registers[NV4097_SET_TEXTURE_CONTROL0 + 24] = 0x60000;
		registers[NV4097_SET_TEXTURE_FILTER + 24] = 0x2052000;
		registers[NV4097_SET_TEXTURE_ADDRESS + 32] = 0x30101;
		registers[NV4097_SET_TEXTURE_BORDER_COLOR + 32] = 0x0;
		registers[NV4097_SET_TEXTURE_CONTROL0 + 32] = 0x60000;
		registers[NV4097_SET_TEXTURE_FILTER + 32] = 0x2052000;
		registers[NV4097_SET_TEXTURE_ADDRESS + 40] = 0x30101;
		registers[NV4097_SET_TEXTURE_BORDER_COLOR + 40] = 0x0;
		registers[NV4097_SET_TEXTURE_CONTROL0 + 40] = 0x60000;
		registers[NV4097_SET_TEXTURE_FILTER + 40] = 0x2052000;
		registers[NV4097_SET_TEXTURE_ADDRESS + 48] = 0x30101;
		registers[NV4097_SET_TEXTURE_BORDER_COLOR + 48] = 0x0;
		registers[NV4097_SET_TEXTURE_CONTROL0 + 48] = 0x60000;
		registers[NV4097_SET_TEXTURE_FILTER + 48] = 0x2052000;
		registers[NV4097_SET_TEXTURE_ADDRESS + 56] = 0x30101;
		registers[NV4097_SET_TEXTURE_BORDER_COLOR + 56] = 0x0;
		registers[NV4097_SET_TEXTURE_CONTROL0 + 56] = 0x60000;
		registers[NV4097_SET_TEXTURE_FILTER + 56] = 0x2052000;
		registers[NV4097_SET_TEXTURE_ADDRESS + 64] = 0x30101;
		registers[NV4097_SET_TEXTURE_BORDER_COLOR + 64] = 0x0;
		registers[NV4097_SET_TEXTURE_CONTROL0 + 64] = 0x60000;
		registers[NV4097_SET_TEXTURE_FILTER + 64] = 0x2052000;
		registers[NV4097_SET_TEXTURE_ADDRESS + 72] = 0x30101;
		registers[NV4097_SET_TEXTURE_BORDER_COLOR + 72] = 0x0;
		registers[NV4097_SET_TEXTURE_CONTROL0 + 72] = 0x60000;
		registers[NV4097_SET_TEXTURE_FILTER + 72] = 0x2052000;
		registers[NV4097_SET_TEXTURE_ADDRESS + 80] = 0x30101;
		registers[NV4097_SET_TEXTURE_BORDER_COLOR + 80] = 0x0;
		registers[NV4097_SET_TEXTURE_CONTROL0 + 80] = 0x60000;
		registers[NV4097_SET_TEXTURE_FILTER + 80] = 0x2052000;
		registers[NV4097_SET_TEXTURE_ADDRESS + 88] = 0x30101;
		registers[NV4097_SET_TEXTURE_BORDER_COLOR + 88] = 0x0;
		registers[NV4097_SET_TEXTURE_CONTROL0 + 88] = 0x60000;
		registers[NV4097_SET_TEXTURE_FILTER + 88] = 0x2052000;
		registers[NV4097_SET_TEXTURE_ADDRESS + 96] = 0x30101;
		registers[NV4097_SET_TEXTURE_BORDER_COLOR + 96] = 0x0;
		registers[NV4097_SET_TEXTURE_CONTROL0 + 96] = 0x60000;
		registers[NV4097_SET_TEXTURE_FILTER + 96] = 0x2052000;
		registers[NV4097_SET_TEXTURE_ADDRESS + 104] = 0x30101;
		registers[NV4097_SET_TEXTURE_BORDER_COLOR + 104] = 0x0;
		registers[NV4097_SET_TEXTURE_CONTROL0 + 104] = 0x60000;
		registers[NV4097_SET_TEXTURE_FILTER + 104] = 0x2052000;
		registers[NV4097_SET_TEXTURE_ADDRESS + 112] = 0x30101;
		registers[NV4097_SET_TEXTURE_BORDER_COLOR + 112] = 0x0;
		registers[NV4097_SET_TEXTURE_CONTROL0 + 112] = 0x60000;
		registers[NV4097_SET_TEXTURE_FILTER + 112] = 0x2052000;
		registers[NV4097_SET_TEXTURE_ADDRESS + 120] = 0x30101;
		registers[NV4097_SET_TEXTURE_BORDER_COLOR + 120] = 0x0;
		registers[NV4097_SET_TEXTURE_CONTROL0 + 120] = 0x60000;
		registers[NV4097_SET_TEXTURE_FILTER + 120] = 0x2052000;
		registers[NV4097_SET_TWO_SIDED_STENCIL_TEST_ENABLE] = 0x0;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT] = 0x2;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET] = 0x0;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 1] = 0x2;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 1] = 0x0;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 2] = 0x2;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 2] = 0x0;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 3] = 0x2;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 3] = 0x0;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 4] = 0x2;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 4] = 0x0;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 5] = 0x2;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 5] = 0x0;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 6] = 0x2;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 6] = 0x0;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 7] = 0x2;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 7] = 0x0;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 8] = 0x2;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 8] = 0x0;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 9] = 0x2;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 9] = 0x0;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 10] = 0x2;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 10] = 0x0;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 11] = 0x2;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 11] = 0x0;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 12] = 0x2;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 12] = 0x0;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 13] = 0x2;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 13] = 0x0;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 14] = 0x2;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 14] = 0x0;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT + 15] = 0x2;
		registers[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET + 15] = 0x0;
		registers[NV4097_SET_VIEWPORT_HORIZONTAL] = 0x10000000;
		registers[NV4097_SET_VIEWPORT_VERTICAL] = 0x10000000;
		registers[NV4097_SET_CLIP_MIN] = 0x0;
		registers[NV4097_SET_CLIP_MAX] = 0x3f800000;
		registers[NV4097_SET_VIEWPORT_OFFSET + 0] = 0x45000000;
		registers[NV4097_SET_VIEWPORT_OFFSET + 1] = 0x45000000;
		registers[NV4097_SET_VIEWPORT_OFFSET + 2] = 0x3f000000;
		registers[NV4097_SET_VIEWPORT_OFFSET + 3] = 0x0;
		registers[NV4097_SET_VIEWPORT_SCALE + 0] = 0x45000000;
		registers[NV4097_SET_VIEWPORT_SCALE + 1] = 0x45000000;
		registers[NV4097_SET_VIEWPORT_SCALE + 2] = 0x3f000000;
		registers[NV4097_SET_VIEWPORT_SCALE + 3] = 0x0;
		// NOTE: Realhw emits this sequence twice, likely to work around a hardware bug. Similar behavior can be seen in other buggy register blocks
		//registers[NV4097_SET_VIEWPORT_OFFSET + 0] = 0x45000000;
		//registers[NV4097_SET_VIEWPORT_OFFSET + 1] = 0x45000000;
		//registers[NV4097_SET_VIEWPORT_OFFSET + 2] = 0x3f000000;
		//registers[NV4097_SET_VIEWPORT_OFFSET + 3] = 0x0;
		//registers[NV4097_SET_VIEWPORT_SCALE + 0] = 0x45000000;
		//registers[NV4097_SET_VIEWPORT_SCALE + 1] = 0x45000000;
		//registers[NV4097_SET_VIEWPORT_SCALE + 2] = 0x3f000000;
		//registers[NV4097_SET_VIEWPORT_SCALE + 3] = 0x0;
		registers[NV4097_SET_ANTI_ALIASING_CONTROL] = 0xffff0000;
		registers[NV4097_SET_BACK_POLYGON_MODE] = 0x1b02;
		registers[NV4097_SET_COLOR_CLEAR_VALUE] = 0x0;
		registers[NV4097_SET_COLOR_MASK_MRT] = 0x0;
		registers[NV4097_SET_FRONT_POLYGON_MODE] = 0x1b02;
		registers[NV4097_SET_LINE_SMOOTH_ENABLE] = 0x0;
		registers[NV4097_SET_LINE_STIPPLE] = 0x0;
		registers[NV4097_SET_POINT_PARAMS_ENABLE] = 0x0;
		registers[NV4097_SET_POINT_SPRITE_CONTROL] = 0x0;
		registers[NV4097_SET_POLY_SMOOTH_ENABLE] = 0x0;
		registers[NV4097_SET_POLYGON_STIPPLE] = 0x0;
		registers[NV4097_SET_RENDER_ENABLE] = 0x1000000;
		registers[NV4097_SET_USER_CLIP_PLANE_CONTROL] = 0x0;
		registers[NV4097_SET_VERTEX_ATTRIB_INPUT_MASK] = 0xffff;
		registers[NV4097_SET_ZPASS_PIXEL_COUNT_ENABLE] = 0x0;
		registers[NV4097_SET_VERTEX_TEXTURE_ADDRESS] = 0x101;
		registers[NV4097_SET_VERTEX_TEXTURE_BORDER_COLOR] = 0x0;
		registers[NV4097_SET_VERTEX_TEXTURE_CONTROL0] = 0x60000;
		registers[NV4097_SET_VERTEX_TEXTURE_FILTER] = 0x0;
		registers[NV4097_SET_VERTEX_TEXTURE_ADDRESS + 8] = 0x101;
		registers[NV4097_SET_VERTEX_TEXTURE_BORDER_COLOR + 8] = 0x0;
		registers[NV4097_SET_VERTEX_TEXTURE_CONTROL0 + 8] = 0x60000;
		registers[NV4097_SET_VERTEX_TEXTURE_FILTER + 8] = 0x0;
		registers[NV4097_SET_VERTEX_TEXTURE_ADDRESS + 16] = 0x101;
		registers[NV4097_SET_VERTEX_TEXTURE_BORDER_COLOR + 16] = 0x0;
		registers[NV4097_SET_VERTEX_TEXTURE_CONTROL0 + 16] = 0x60000;
		registers[NV4097_SET_VERTEX_TEXTURE_FILTER + 16] = 0x0;
		registers[NV4097_SET_VERTEX_TEXTURE_ADDRESS + 24] = 0x101;
		registers[NV4097_SET_VERTEX_TEXTURE_BORDER_COLOR + 24] = 0x0;
		registers[NV4097_SET_VERTEX_TEXTURE_CONTROL0 + 24] = 0x60000;
		registers[NV4097_SET_VERTEX_TEXTURE_FILTER + 24] = 0x0;
		registers[NV4097_SET_CYLINDRICAL_WRAP] = 0x0;
		registers[NV4097_SET_ZMIN_MAX_CONTROL] = 0x1;
		registers[NV4097_SET_TWO_SIDE_LIGHT_EN] = 0x0;
		registers[NV4097_SET_TRANSFORM_BRANCH_BITS] = 0x0;
		registers[NV4097_SET_NO_PARANOID_TEXTURE_FETCHES] = 0x0;

		registers[NV0039_SET_OBJECT] = 0x31337303;
		registers[NV0039_SET_CONTEXT_DMA_NOTIFIES] = 0x66604200;
		registers[NV0039_SET_CONTEXT_DMA_BUFFER_IN] = 0xfeed0001;
		registers[NV0039_SET_CONTEXT_DMA_BUFFER_OUT] = 0xfeed0000;

		registers[NV3062_SET_OBJECT] = 0x313371c3;
		registers[NV3062_SET_CONTEXT_DMA_NOTIFIES] = 0x66604200;
		registers[NV3062_SET_CONTEXT_DMA_IMAGE_SOURCE] = 0xfeed0000;
		registers[NV3062_SET_CONTEXT_DMA_IMAGE_DESTIN] = 0xfeed0000;

		registers[0xa000 / 4] = 0x31337808;
		registers[0xa180 / 4] = 0x66604200;
		registers[0xa184 / 4] = 0x0;
		registers[0xa188 / 4] = 0x0;
		registers[0xa18c / 4] = 0x0;
		registers[0xa190 / 4] = 0x0;
		registers[0xa194 / 4] = 0x0;
		registers[0xa198 / 4] = 0x0;
		registers[0xa19c / 4] = 0x313371c3;
		registers[0xa2fc / 4] = 0x3;
		registers[0xa300 / 4] = 0x4;
		registers[0x8000 / 4] = 0x31337a73;
		registers[0x8180 / 4] = 0x66604200;
		registers[0x8184 / 4] = 0xfeed0000;
		registers[0xc000 / 4] = 0x3137af00;
		registers[0xc180 / 4] = 0x66604200;

		registers[NV406E_SEMAPHORE_OFFSET] = 0x10;
	}

	void rsx_state::decode(u32 reg, u32 value)
	{
		// Store new value and save previous
		latch = std::exchange(registers[reg], value);
	}

	bool rsx_state::test(u32 reg, u32 value) const
	{
		return registers[reg] == value;
	}

	namespace method_detail
	{
		template <u32 Id, u32 Step, u32 Count, template<u32> class T, u32 Index = 0>
		struct bind_range_impl_t
		{
			static inline void impl()
			{
				methods[Id] = &T<Index>::impl;

				if constexpr (Count > 1)
				{
					bind_range_impl_t<Id + Step, Step, Count - 1, T, Index + 1>::impl();
				}
			}
		};

		template <u32 Id, u32 Step, u32 Count, template<u32> class T, u32 Index = 0>
		static inline void bind_range()
		{
			static_assert(Step && Count && Id + u64{Step} * (Count - 1) < 0x10000 / 4);

			bind_range_impl_t<Id, Step, Count, T, Index>::impl();
		}
	}

	// TODO: implement this as virtual function: rsx::thread::init_methods() or something
	// TODO: this is unused
	static const bool s_methods_init = []() -> bool
	{
		using namespace method_detail;

		methods.fill(&invalid_method);

		auto bind = [](u32 id, rsx_method_t func)
		{
			::at32(methods, id) = func;
		};

		auto bind_array = [](u32 id, u32 step, u32 count, rsx_method_t func)
		{
			ensure(step && count && id + u64{step} * (count - 1) < 0x10000 / 4);

			for (u32 i = id; i < id + count * step; i += step)
			{
				methods[i] = func;
			}
		};

		// NV40_CHANNEL_DMA (NV406E)
		methods[NV406E_SET_REFERENCE]                     = nullptr;
		methods[NV406E_SET_CONTEXT_DMA_SEMAPHORE]         = nullptr;
		methods[NV406E_SEMAPHORE_OFFSET]                  = nullptr;
		methods[NV406E_SEMAPHORE_ACQUIRE]                 = nullptr;
		methods[NV406E_SEMAPHORE_RELEASE]                 = nullptr;

		// NV40_CURIE_PRIMITIVE	(NV4097)
		methods[NV4097_SET_OBJECT]                        = nullptr;
		methods[NV4097_NO_OPERATION]                      = nullptr;
		methods[NV4097_NOTIFY]                            = nullptr;
		methods[NV4097_WAIT_FOR_IDLE]                     = nullptr;
		methods[NV4097_PM_TRIGGER]                        = nullptr;
		methods[NV4097_SET_CONTEXT_DMA_NOTIFIES]          = nullptr;
		methods[NV4097_SET_CONTEXT_DMA_A]                 = nullptr;
		methods[NV4097_SET_CONTEXT_DMA_B]                 = nullptr;
		methods[NV4097_SET_CONTEXT_DMA_COLOR_B]           = nullptr;
		methods[NV4097_SET_CONTEXT_DMA_STATE]             = nullptr;
		methods[NV4097_SET_CONTEXT_DMA_COLOR_A]           = nullptr;
		methods[NV4097_SET_CONTEXT_DMA_ZETA]              = nullptr;
		methods[NV4097_SET_CONTEXT_DMA_VERTEX_A]          = nullptr;
		methods[NV4097_SET_CONTEXT_DMA_VERTEX_B]          = nullptr;
		methods[NV4097_SET_CONTEXT_DMA_SEMAPHORE]         = nullptr;
		methods[NV4097_SET_CONTEXT_DMA_REPORT]            = nullptr;
		methods[NV4097_SET_CONTEXT_DMA_CLIP_ID]           = nullptr;
		methods[NV4097_SET_CONTEXT_DMA_CULL_DATA]         = nullptr;
		methods[NV4097_SET_CONTEXT_DMA_COLOR_C]           = nullptr;
		methods[NV4097_SET_CONTEXT_DMA_COLOR_D]           = nullptr;
		methods[NV4097_SET_SURFACE_CLIP_HORIZONTAL]       = nullptr;
		methods[NV4097_SET_SURFACE_CLIP_VERTICAL]         = nullptr;
		methods[NV4097_SET_SURFACE_FORMAT]                = nullptr;
		methods[NV4097_SET_SURFACE_PITCH_A]               = nullptr;
		methods[NV4097_SET_SURFACE_COLOR_AOFFSET]         = nullptr;
		methods[NV4097_SET_SURFACE_ZETA_OFFSET]           = nullptr;
		methods[NV4097_SET_SURFACE_COLOR_BOFFSET]         = nullptr;
		methods[NV4097_SET_SURFACE_PITCH_B]               = nullptr;
		methods[NV4097_SET_SURFACE_COLOR_TARGET]          = nullptr;
		methods[0x224 >> 2]                               = nullptr;
		methods[0x228 >> 2]                               = nullptr;
		methods[0x230 >> 2]                               = nullptr;
		methods[NV4097_SET_SURFACE_PITCH_Z]               = nullptr;
		methods[NV4097_INVALIDATE_ZCULL]                  = nullptr;
		methods[NV4097_SET_CYLINDRICAL_WRAP]              = nullptr;
		methods[NV4097_SET_CYLINDRICAL_WRAP1]             = nullptr;
		methods[0x240 >> 2]                               = nullptr;
		methods[0x244 >> 2]                               = nullptr;
		methods[0x248 >> 2]                               = nullptr;
		methods[0x24C >> 2]                               = nullptr;
		methods[NV4097_SET_SURFACE_PITCH_C]               = nullptr;
		methods[NV4097_SET_SURFACE_PITCH_D]               = nullptr;
		methods[NV4097_SET_SURFACE_COLOR_COFFSET]         = nullptr;
		methods[NV4097_SET_SURFACE_COLOR_DOFFSET]         = nullptr;
		methods[NV4097_SET_WINDOW_OFFSET]                 = nullptr;
		methods[NV4097_SET_WINDOW_CLIP_TYPE]              = nullptr;
		methods[NV4097_SET_WINDOW_CLIP_HORIZONTAL]        = nullptr;
		methods[NV4097_SET_WINDOW_CLIP_VERTICAL]          = nullptr;
		methods[0x2c8 >> 2]                               = nullptr;
		methods[0x2cc >> 2]                               = nullptr;
		methods[0x2d0 >> 2]                               = nullptr;
		methods[0x2d4 >> 2]                               = nullptr;
		methods[0x2d8 >> 2]                               = nullptr;
		methods[0x2dc >> 2]                               = nullptr;
		methods[0x2e0 >> 2]                               = nullptr;
		methods[0x2e4 >> 2]                               = nullptr;
		methods[0x2e8 >> 2]                               = nullptr;
		methods[0x2ec >> 2]                               = nullptr;
		methods[0x2f0 >> 2]                               = nullptr;
		methods[0x2f4 >> 2]                               = nullptr;
		methods[0x2f8 >> 2]                               = nullptr;
		methods[0x2fc >> 2]                               = nullptr;
		methods[NV4097_SET_DITHER_ENABLE]                 = nullptr;
		methods[NV4097_SET_ALPHA_TEST_ENABLE]             = nullptr;
		methods[NV4097_SET_ALPHA_FUNC]                    = nullptr;
		methods[NV4097_SET_ALPHA_REF]                     = nullptr;
		methods[NV4097_SET_BLEND_ENABLE]                  = nullptr;
		methods[NV4097_SET_BLEND_FUNC_SFACTOR]            = nullptr;
		methods[NV4097_SET_BLEND_FUNC_DFACTOR]            = nullptr;
		methods[NV4097_SET_BLEND_COLOR]                   = nullptr;
		methods[NV4097_SET_BLEND_EQUATION]                = nullptr;
		methods[NV4097_SET_COLOR_MASK]                    = nullptr;
		methods[NV4097_SET_STENCIL_TEST_ENABLE]           = nullptr;
		methods[NV4097_SET_STENCIL_MASK]                  = nullptr;
		methods[NV4097_SET_STENCIL_FUNC]                  = nullptr;
		methods[NV4097_SET_STENCIL_FUNC_REF]              = nullptr;
		methods[NV4097_SET_STENCIL_FUNC_MASK]             = nullptr;
		methods[NV4097_SET_STENCIL_OP_FAIL]               = nullptr;
		methods[NV4097_SET_STENCIL_OP_ZFAIL]              = nullptr;
		methods[NV4097_SET_STENCIL_OP_ZPASS]              = nullptr;
		methods[NV4097_SET_TWO_SIDED_STENCIL_TEST_ENABLE] = nullptr;
		methods[NV4097_SET_BACK_STENCIL_MASK]             = nullptr;
		methods[NV4097_SET_BACK_STENCIL_FUNC]             = nullptr;
		methods[NV4097_SET_BACK_STENCIL_FUNC_REF]         = nullptr;
		methods[NV4097_SET_BACK_STENCIL_FUNC_MASK]        = nullptr;
		methods[NV4097_SET_BACK_STENCIL_OP_FAIL]          = nullptr;
		methods[NV4097_SET_BACK_STENCIL_OP_ZFAIL]         = nullptr;
		methods[NV4097_SET_BACK_STENCIL_OP_ZPASS]         = nullptr;
		methods[NV4097_SET_SHADE_MODE]                    = nullptr;
		methods[NV4097_SET_BLEND_ENABLE_MRT]              = nullptr;
		methods[NV4097_SET_COLOR_MASK_MRT]                = nullptr;
		methods[NV4097_SET_LOGIC_OP_ENABLE]               = nullptr;
		methods[NV4097_SET_LOGIC_OP]                      = nullptr;
		methods[NV4097_SET_BLEND_COLOR2]                  = nullptr;
		methods[NV4097_SET_DEPTH_BOUNDS_TEST_ENABLE]      = nullptr;
		methods[NV4097_SET_DEPTH_BOUNDS_MIN]              = nullptr;
		methods[NV4097_SET_DEPTH_BOUNDS_MAX]              = nullptr;
		methods[NV4097_SET_CLIP_MIN]                      = nullptr;
		methods[NV4097_SET_CLIP_MAX]                      = nullptr;
		methods[NV4097_SET_CONTROL0]                      = nullptr;
		methods[NV4097_SET_LINE_WIDTH]                    = nullptr;
		methods[NV4097_SET_LINE_SMOOTH_ENABLE]            = nullptr;
		methods[NV4097_SET_ANISO_SPREAD]                  = nullptr;
		methods[NV4097_SET_SCISSOR_HORIZONTAL]            = nullptr;
		methods[NV4097_SET_SCISSOR_VERTICAL]              = nullptr;
		methods[NV4097_SET_FOG_MODE]                      = nullptr;
		methods[NV4097_SET_FOG_PARAMS]                    = nullptr;
		methods[NV4097_SET_FOG_PARAMS + 1]                = nullptr;
		methods[0x8d8 >> 2]                               = nullptr;
		methods[NV4097_SET_SHADER_PROGRAM]                = nullptr;
		methods[NV4097_SET_VERTEX_TEXTURE_OFFSET]         = nullptr;
		methods[NV4097_SET_VERTEX_TEXTURE_FORMAT]         = nullptr;
		methods[NV4097_SET_VERTEX_TEXTURE_ADDRESS]        = nullptr;
		methods[NV4097_SET_VERTEX_TEXTURE_CONTROL0]       = nullptr;
		methods[NV4097_SET_VERTEX_TEXTURE_CONTROL3]       = nullptr;
		methods[NV4097_SET_VERTEX_TEXTURE_FILTER]         = nullptr;
		methods[NV4097_SET_VERTEX_TEXTURE_IMAGE_RECT]     = nullptr;
		methods[NV4097_SET_VERTEX_TEXTURE_BORDER_COLOR]   = nullptr;
		methods[NV4097_SET_VIEWPORT_HORIZONTAL]           = nullptr;
		methods[NV4097_SET_VIEWPORT_VERTICAL]             = nullptr;
		methods[NV4097_SET_POINT_CENTER_MODE]             = nullptr;
		methods[NV4097_ZCULL_SYNC]                        = nullptr;
		methods[NV4097_SET_VIEWPORT_OFFSET]               = nullptr;
		methods[NV4097_SET_VIEWPORT_OFFSET + 1]           = nullptr;
		methods[NV4097_SET_VIEWPORT_OFFSET + 2]           = nullptr;
		methods[NV4097_SET_VIEWPORT_OFFSET + 3]           = nullptr;
		methods[NV4097_SET_VIEWPORT_SCALE]                = nullptr;
		methods[NV4097_SET_VIEWPORT_SCALE + 1]            = nullptr;
		methods[NV4097_SET_VIEWPORT_SCALE + 2]            = nullptr;
		methods[NV4097_SET_VIEWPORT_SCALE + 3]            = nullptr;
		methods[NV4097_SET_POLY_OFFSET_POINT_ENABLE]      = nullptr;
		methods[NV4097_SET_POLY_OFFSET_LINE_ENABLE]       = nullptr;
		methods[NV4097_SET_POLY_OFFSET_FILL_ENABLE]       = nullptr;
		methods[NV4097_SET_DEPTH_FUNC]                    = nullptr;
		methods[NV4097_SET_DEPTH_MASK]                    = nullptr;
		methods[NV4097_SET_DEPTH_TEST_ENABLE]             = nullptr;
		methods[NV4097_SET_POLYGON_OFFSET_SCALE_FACTOR]   = nullptr;
		methods[NV4097_SET_POLYGON_OFFSET_BIAS]           = nullptr;
		methods[NV4097_SET_VERTEX_DATA_SCALED4S_M]        = nullptr;
		methods[NV4097_SET_TEXTURE_CONTROL2]              = nullptr;
		methods[NV4097_SET_TEX_COORD_CONTROL]             = nullptr;
		methods[NV4097_SET_TRANSFORM_PROGRAM]             = nullptr;
		methods[NV4097_SET_SPECULAR_ENABLE]               = nullptr;
		methods[NV4097_SET_TWO_SIDE_LIGHT_EN]             = nullptr;
		methods[NV4097_CLEAR_ZCULL_SURFACE]               = nullptr;
		methods[NV4097_SET_PERFORMANCE_PARAMS]            = nullptr;
		methods[NV4097_SET_FLAT_SHADE_OP]                 = nullptr;
		methods[NV4097_SET_EDGE_FLAG]                     = nullptr;
		methods[NV4097_SET_USER_CLIP_PLANE_CONTROL]       = nullptr;
		methods[NV4097_SET_POLYGON_STIPPLE]               = nullptr;
		methods[NV4097_SET_POLYGON_STIPPLE_PATTERN]       = nullptr;
		methods[NV4097_SET_VERTEX_DATA3F_M]               = nullptr;
		methods[NV4097_SET_VERTEX_DATA_ARRAY_OFFSET]      = nullptr;
		methods[NV4097_INVALIDATE_VERTEX_CACHE_FILE]      = nullptr;
		methods[NV4097_INVALIDATE_VERTEX_FILE]            = nullptr;
		methods[NV4097_PIPE_NOP]                          = nullptr;
		methods[NV4097_SET_VERTEX_DATA_BASE_OFFSET]       = nullptr;
		methods[NV4097_SET_VERTEX_DATA_BASE_INDEX]        = nullptr;
		methods[NV4097_SET_VERTEX_DATA_ARRAY_FORMAT]      = nullptr;
		methods[NV4097_CLEAR_REPORT_VALUE]                = nullptr;
		methods[NV4097_SET_ZPASS_PIXEL_COUNT_ENABLE]      = nullptr;
		methods[NV4097_GET_REPORT]                        = nullptr;
		methods[NV4097_SET_ZCULL_STATS_ENABLE]            = nullptr;
		methods[NV4097_SET_BEGIN_END]                     = nullptr;
		methods[NV4097_ARRAY_ELEMENT16]                   = nullptr;
		methods[NV4097_ARRAY_ELEMENT32]                   = nullptr;
		methods[NV4097_DRAW_ARRAYS]                       = nullptr;
		methods[NV4097_INLINE_ARRAY]                      = nullptr;
		methods[NV4097_SET_INDEX_ARRAY_ADDRESS]           = nullptr;
		methods[NV4097_SET_INDEX_ARRAY_DMA]               = nullptr;
		methods[NV4097_DRAW_INDEX_ARRAY]                  = nullptr;
		methods[NV4097_SET_FRONT_POLYGON_MODE]            = nullptr;
		methods[NV4097_SET_BACK_POLYGON_MODE]             = nullptr;
		methods[NV4097_SET_CULL_FACE]                     = nullptr;
		methods[NV4097_SET_FRONT_FACE]                    = nullptr;
		methods[NV4097_SET_POLY_SMOOTH_ENABLE]            = nullptr;
		methods[NV4097_SET_CULL_FACE_ENABLE]              = nullptr;
		methods[NV4097_SET_TEXTURE_CONTROL3]              = nullptr;
		methods[NV4097_SET_VERTEX_DATA2F_M]               = nullptr;
		methods[NV4097_SET_VERTEX_DATA2S_M]               = nullptr;
		methods[NV4097_SET_VERTEX_DATA4UB_M]              = nullptr;
		methods[NV4097_SET_VERTEX_DATA4S_M]               = nullptr;
		methods[NV4097_SET_TEXTURE_OFFSET]                = nullptr;
		methods[NV4097_SET_TEXTURE_FORMAT]                = nullptr;
		methods[NV4097_SET_TEXTURE_ADDRESS]               = nullptr;
		methods[NV4097_SET_TEXTURE_CONTROL0]              = nullptr;
		methods[NV4097_SET_TEXTURE_CONTROL1]              = nullptr;
		methods[NV4097_SET_TEXTURE_FILTER]                = nullptr;
		methods[NV4097_SET_TEXTURE_IMAGE_RECT]            = nullptr;
		methods[NV4097_SET_TEXTURE_BORDER_COLOR]          = nullptr;
		methods[NV4097_SET_VERTEX_DATA4F_M]               = nullptr;
		methods[NV4097_SET_COLOR_KEY_COLOR]               = nullptr;
		methods[0x1d04 >> 2]                              = nullptr;
		methods[NV4097_SET_SHADER_CONTROL]                = nullptr;
		methods[NV4097_SET_INDEXED_CONSTANT_READ_LIMITS]  = nullptr;
		methods[NV4097_SET_SEMAPHORE_OFFSET]              = nullptr;
		methods[NV4097_BACK_END_WRITE_SEMAPHORE_RELEASE]  = nullptr;
		methods[NV4097_TEXTURE_READ_SEMAPHORE_RELEASE]    = nullptr;
		methods[NV4097_SET_ZMIN_MAX_CONTROL]              = nullptr;
		methods[NV4097_SET_ANTI_ALIASING_CONTROL]         = nullptr;
		methods[NV4097_SET_SURFACE_COMPRESSION]           = nullptr;
		methods[NV4097_SET_ZCULL_EN]                      = nullptr;
		methods[NV4097_SET_SHADER_WINDOW]                 = nullptr;
		methods[NV4097_SET_ZSTENCIL_CLEAR_VALUE]          = nullptr;
		methods[NV4097_SET_COLOR_CLEAR_VALUE]             = nullptr;
		methods[NV4097_CLEAR_SURFACE]                     = nullptr;
		methods[NV4097_SET_CLEAR_RECT_HORIZONTAL]         = nullptr;
		methods[NV4097_SET_CLEAR_RECT_VERTICAL]           = nullptr;
		methods[NV4097_SET_CLIP_ID_TEST_ENABLE]           = nullptr;
		methods[NV4097_SET_RESTART_INDEX_ENABLE]          = nullptr;
		methods[NV4097_SET_RESTART_INDEX]                 = nullptr;
		methods[NV4097_SET_LINE_STIPPLE]                  = nullptr;
		methods[NV4097_SET_LINE_STIPPLE_PATTERN]          = nullptr;
		methods[NV4097_SET_VERTEX_DATA1F_M]               = nullptr;
		methods[NV4097_SET_TRANSFORM_EXECUTION_MODE]      = nullptr;
		methods[NV4097_SET_RENDER_ENABLE]                 = nullptr;
		methods[NV4097_SET_TRANSFORM_PROGRAM_LOAD]        = nullptr;
		methods[NV4097_SET_TRANSFORM_PROGRAM_START]       = nullptr;
		methods[NV4097_SET_ZCULL_CONTROL0]                = nullptr;
		methods[NV4097_SET_ZCULL_CONTROL1]                = nullptr;
		methods[NV4097_SET_SCULL_CONTROL]                 = nullptr;
		methods[NV4097_SET_POINT_SIZE]                    = nullptr;
		methods[NV4097_SET_POINT_PARAMS_ENABLE]           = nullptr;
		methods[NV4097_SET_POINT_SPRITE_CONTROL]          = nullptr;
		methods[NV4097_SET_TRANSFORM_TIMEOUT]             = nullptr;
		methods[NV4097_SET_TRANSFORM_CONSTANT_LOAD]       = nullptr;
		methods[NV4097_SET_TRANSFORM_CONSTANT]            = nullptr;
		methods[NV4097_SET_FREQUENCY_DIVIDER_OPERATION]   = nullptr;
		methods[NV4097_SET_ATTRIB_COLOR]                  = nullptr;
		methods[NV4097_SET_ATTRIB_TEX_COORD]              = nullptr;
		methods[NV4097_SET_ATTRIB_TEX_COORD_EX]           = nullptr;
		methods[NV4097_SET_ATTRIB_UCLIP0]                 = nullptr;
		methods[NV4097_SET_ATTRIB_UCLIP1]                 = nullptr;
		methods[NV4097_INVALIDATE_L2]                     = nullptr;
		methods[NV4097_SET_REDUCE_DST_COLOR]              = nullptr;
		methods[NV4097_SET_NO_PARANOID_TEXTURE_FETCHES]   = nullptr;
		methods[NV4097_SET_SHADER_PACKER]                 = nullptr;
		methods[NV4097_SET_VERTEX_ATTRIB_INPUT_MASK]      = nullptr;
		methods[NV4097_SET_VERTEX_ATTRIB_OUTPUT_MASK]     = nullptr;
		methods[NV4097_SET_TRANSFORM_BRANCH_BITS]         = nullptr;

		// NV03_MEMORY_TO_MEMORY_FORMAT	(NV0039)
		methods[NV0039_SET_OBJECT]                        = nullptr;
		bind(0x2100 >> 2, trace_method);
		methods[NV0039_SET_CONTEXT_DMA_NOTIFIES]          = nullptr;
		methods[NV0039_SET_CONTEXT_DMA_BUFFER_IN]         = nullptr;
		methods[NV0039_SET_CONTEXT_DMA_BUFFER_OUT]        = nullptr;
		methods[NV0039_OFFSET_IN]                         = nullptr;
		methods[NV0039_OFFSET_OUT]                        = nullptr;
		methods[NV0039_PITCH_IN]                          = nullptr;
		methods[NV0039_PITCH_OUT]                         = nullptr;
		methods[NV0039_LINE_LENGTH_IN]                    = nullptr;
		methods[NV0039_LINE_COUNT]                        = nullptr;
		methods[NV0039_FORMAT]                            = nullptr;
		methods[NV0039_BUFFER_NOTIFY]                     = nullptr;

		// NV30_CONTEXT_SURFACES_2D	(NV3062)
		methods[NV3062_SET_OBJECT]                        = nullptr;
		methods[NV3062_SET_CONTEXT_DMA_NOTIFIES]          = nullptr;
		methods[NV3062_SET_CONTEXT_DMA_IMAGE_SOURCE]      = nullptr;
		methods[NV3062_SET_CONTEXT_DMA_IMAGE_DESTIN]      = nullptr;
		methods[NV3062_SET_COLOR_FORMAT]                  = nullptr;
		methods[NV3062_SET_PITCH]                         = nullptr;
		methods[NV3062_SET_OFFSET_SOURCE]                 = nullptr;
		methods[NV3062_SET_OFFSET_DESTIN]                 = nullptr;

		// NV30_CONTEXT_SURFACE_SWIZZLED (NV309E)
		methods[NV309E_SET_OBJECT]                        = nullptr;
		methods[NV309E_SET_CONTEXT_DMA_NOTIFIES]          = nullptr;
		methods[NV309E_SET_CONTEXT_DMA_IMAGE]             = nullptr;
		methods[NV309E_SET_FORMAT]                        = nullptr;
		methods[NV309E_SET_OFFSET]                        = nullptr;

		// NV30_IMAGE_FROM_CPU (NV308A)
		methods[NV308A_SET_OBJECT]                        = nullptr;
		methods[NV308A_SET_CONTEXT_DMA_NOTIFIES]          = nullptr;
		methods[NV308A_SET_CONTEXT_COLOR_KEY]             = nullptr;
		methods[NV308A_SET_CONTEXT_CLIP_RECTANGLE]        = nullptr;
		methods[NV308A_SET_CONTEXT_PATTERN]               = nullptr;
		methods[NV308A_SET_CONTEXT_ROP]                   = nullptr;
		methods[NV308A_SET_CONTEXT_BETA1]                 = nullptr;
		methods[NV308A_SET_CONTEXT_BETA4]                 = nullptr;
		methods[NV308A_SET_CONTEXT_SURFACE]               = nullptr;
		methods[NV308A_SET_COLOR_CONVERSION]              = nullptr;
		methods[NV308A_SET_OPERATION]                     = nullptr;
		methods[NV308A_SET_COLOR_FORMAT]                  = nullptr;
		methods[NV308A_POINT]                             = nullptr;
		methods[NV308A_SIZE_OUT]                          = nullptr;
		methods[NV308A_SIZE_IN]                           = nullptr;
		methods[NV308A_COLOR]                             = nullptr;

		// NV30_SCALED_IMAGE_FROM_MEMORY (NV3089)
		methods[NV3089_SET_OBJECT]                        = nullptr;
		methods[NV3089_SET_CONTEXT_DMA_NOTIFIES]          = nullptr;
		methods[NV3089_SET_CONTEXT_DMA_IMAGE]             = nullptr;
		methods[NV3089_SET_CONTEXT_PATTERN]               = nullptr;
		methods[NV3089_SET_CONTEXT_ROP]                   = nullptr;
		methods[NV3089_SET_CONTEXT_BETA1]                 = nullptr;
		methods[NV3089_SET_CONTEXT_BETA4]                 = nullptr;
		methods[NV3089_SET_CONTEXT_SURFACE]               = nullptr;
		methods[NV3089_SET_COLOR_CONVERSION]              = nullptr;
		methods[NV3089_SET_COLOR_FORMAT]                  = nullptr;
		methods[NV3089_SET_OPERATION]                     = nullptr;
		methods[NV3089_CLIP_POINT]                        = nullptr;
		methods[NV3089_CLIP_SIZE]                         = nullptr;
		methods[NV3089_IMAGE_OUT_POINT]                   = nullptr;
		methods[NV3089_IMAGE_OUT_SIZE]                    = nullptr;
		methods[NV3089_DS_DX]                             = nullptr;
		methods[NV3089_DT_DY]                             = nullptr;
		methods[NV3089_IMAGE_IN_SIZE]                     = nullptr;
		methods[NV3089_IMAGE_IN_FORMAT]                   = nullptr;
		methods[NV3089_IMAGE_IN_OFFSET]                   = nullptr;
		methods[NV3089_IMAGE_IN]                          = nullptr;

		//Some custom GCM methods
		methods[GCM_SET_DRIVER_OBJECT]                    = nullptr;
		methods[FIFO::FIFO_DRAW_BARRIER >> 2]             = nullptr;

		bind_array(GCM_FLIP_HEAD, 1, 2, nullptr);
		bind_array(GCM_DRIVER_QUEUE, 1, 8, nullptr);

		bind_array(0x400 >> 2, 1, 0x10, nullptr);
		bind_array(0x440 >> 2, 1, 0x20, nullptr);
		bind_array(NV4097_SET_ANISO_SPREAD, 1, 16, nullptr);
		bind_array(NV4097_SET_VERTEX_TEXTURE_OFFSET, 1, 8 * 4, nullptr);
		bind_array(NV4097_SET_VERTEX_DATA_SCALED4S_M, 1, 32, nullptr);
		bind_array(NV4097_SET_TEXTURE_CONTROL2, 1, 16, nullptr);
		bind_array(NV4097_SET_TEX_COORD_CONTROL, 1, 10, nullptr);
		bind_array(NV4097_SET_TRANSFORM_PROGRAM, 1, 32, nullptr);
		bind_array(NV4097_SET_POLYGON_STIPPLE_PATTERN, 1, 32, nullptr);
		bind_array(NV4097_SET_VERTEX_DATA3F_M, 1, 64, nullptr);
		bind_array(NV4097_SET_VERTEX_DATA_ARRAY_OFFSET, 1, 16, nullptr);
		bind_array(NV4097_SET_VERTEX_DATA_ARRAY_FORMAT, 1, 16, nullptr);
		bind_array(NV4097_SET_TEXTURE_CONTROL3, 1, 16, nullptr);
		bind_array(NV4097_SET_VERTEX_DATA2F_M, 1, 32, nullptr);
		bind_array(NV4097_SET_VERTEX_DATA2S_M, 1, 16, nullptr);
		bind_array(NV4097_SET_VERTEX_DATA4UB_M, 1, 16, nullptr);
		bind_array(NV4097_SET_VERTEX_DATA4S_M, 1, 32, nullptr);
		bind_array(NV4097_SET_TEXTURE_OFFSET, 1, 8 * 16, nullptr);
		bind_array(NV4097_SET_VERTEX_DATA4F_M, 1, 64, nullptr);
		bind_array(NV4097_SET_VERTEX_DATA1F_M, 1, 16, nullptr);
		bind_array(NV4097_SET_COLOR_KEY_COLOR, 1, 16, nullptr);

		// Unknown (NV4097?)
		bind(0x171c >> 2, trace_method);

		// NV406E
		bind(NV406E_SET_REFERENCE, nv406e::set_reference);
		bind(NV406E_SEMAPHORE_ACQUIRE, nv406e::semaphore_acquire);
		bind(NV406E_SEMAPHORE_RELEASE, nv406e::semaphore_release);

		// NV4097
		bind(NV4097_SET_CULL_FACE, nv4097::set_face_property);
		bind(NV4097_SET_FRONT_FACE, nv4097::set_face_property);
		bind(NV4097_TEXTURE_READ_SEMAPHORE_RELEASE, nv4097::texture_read_semaphore_release);
		bind(NV4097_BACK_END_WRITE_SEMAPHORE_RELEASE, nv4097::back_end_write_semaphore_release);
		bind(NV4097_SET_BEGIN_END, nv4097::set_begin_end);
		bind(NV4097_CLEAR_SURFACE, nv4097::clear);
		bind(NV4097_DRAW_ARRAYS, nv4097::draw_arrays);
		bind(NV4097_DRAW_INDEX_ARRAY, nv4097::draw_index_array);
		bind(NV4097_INLINE_ARRAY, nv4097::draw_inline_array);
		bind(NV4097_ARRAY_ELEMENT16, nv4097::set_array_element16);
		bind(NV4097_ARRAY_ELEMENT32, nv4097::set_array_element32);
		bind_range<NV4097_SET_VERTEX_DATA_SCALED4S_M, 1, 32, nv4097::set_vertex_data_scaled4s_m>();
		bind_range<NV4097_SET_VERTEX_DATA4UB_M, 1, 16, nv4097::set_vertex_data4ub_m>();
		bind_range<NV4097_SET_VERTEX_DATA1F_M, 1, 16, nv4097::set_vertex_data1f_m>();
		bind_range<NV4097_SET_VERTEX_DATA2F_M, 1, 32, nv4097::set_vertex_data2f_m>();
		bind_range<NV4097_SET_VERTEX_DATA3F_M, 1, 64, nv4097::set_vertex_data3f_m>();
		bind_range<NV4097_SET_VERTEX_DATA4F_M, 1, 64, nv4097::set_vertex_data4f_m>();
		bind_range<NV4097_SET_VERTEX_DATA2S_M, 1, 16, nv4097::set_vertex_data2s_m>();
		bind_range<NV4097_SET_VERTEX_DATA4S_M, 1, 32, nv4097::set_vertex_data4s_m>();
		bind(NV4097_SET_TRANSFORM_CONSTANT_LOAD, nv4097::set_transform_constant_load);
		bind_array(NV4097_SET_TRANSFORM_CONSTANT, 1, 32, nv4097::set_transform_constant::impl);
		bind_array(NV4097_SET_TRANSFORM_PROGRAM, 1, 32, nv4097::set_transform_program::impl);
		bind(NV4097_GET_REPORT, nv4097::get_report);
		bind(NV4097_CLEAR_REPORT_VALUE, nv4097::clear_report_value);
		bind(NV4097_SET_SURFACE_CLIP_HORIZONTAL, nv4097::set_surface_dirty_bit);
		bind(NV4097_SET_SURFACE_CLIP_VERTICAL, nv4097::set_surface_dirty_bit);
		bind(NV4097_SET_SURFACE_COLOR_AOFFSET, nv4097::set_surface_dirty_bit);
		bind(NV4097_SET_SURFACE_COLOR_BOFFSET, nv4097::set_surface_dirty_bit);
		bind(NV4097_SET_SURFACE_COLOR_COFFSET, nv4097::set_surface_dirty_bit);
		bind(NV4097_SET_SURFACE_COLOR_DOFFSET, nv4097::set_surface_dirty_bit);
		bind(NV4097_SET_SURFACE_COLOR_TARGET, nv4097::set_surface_dirty_bit);
		bind(NV4097_SET_SURFACE_ZETA_OFFSET, nv4097::set_surface_dirty_bit);
		bind(NV4097_SET_CONTEXT_DMA_COLOR_A, nv4097::set_surface_dirty_bit);
		bind(NV4097_SET_CONTEXT_DMA_COLOR_B, nv4097::set_surface_dirty_bit);
		bind(NV4097_SET_CONTEXT_DMA_COLOR_C, nv4097::set_surface_dirty_bit);
		bind(NV4097_SET_CONTEXT_DMA_COLOR_D, nv4097::set_surface_dirty_bit);
		bind(NV4097_SET_CONTEXT_DMA_ZETA, nv4097::set_surface_dirty_bit);
		bind(NV4097_NOTIFY, nv4097::set_notify);
		bind(NV4097_SET_SURFACE_FORMAT, nv4097::set_surface_format);
		bind(NV4097_SET_SURFACE_PITCH_A, nv4097::set_surface_dirty_bit);
		bind(NV4097_SET_SURFACE_PITCH_B, nv4097::set_surface_dirty_bit);
		bind(NV4097_SET_SURFACE_PITCH_C, nv4097::set_surface_dirty_bit);
		bind(NV4097_SET_SURFACE_PITCH_D, nv4097::set_surface_dirty_bit);
		bind(NV4097_SET_SURFACE_PITCH_Z, nv4097::set_surface_dirty_bit);
		bind(NV4097_SET_WINDOW_OFFSET, nv4097::set_surface_dirty_bit);
		bind_range<NV4097_SET_TEXTURE_OFFSET, 8, 16, nv4097::set_fragment_texture_dirty_bit>();
		bind_range<NV4097_SET_TEXTURE_FORMAT, 8, 16, nv4097::set_fragment_texture_dirty_bit>();
		bind_range<NV4097_SET_TEXTURE_ADDRESS, 8, 16, nv4097::set_fragment_texture_dirty_bit>();
		bind_range<NV4097_SET_TEXTURE_CONTROL0, 8, 16, nv4097::set_fragment_texture_dirty_bit>();
		bind_range<NV4097_SET_TEXTURE_CONTROL1, 8, 16, nv4097::set_fragment_texture_dirty_bit>();
		bind_range<NV4097_SET_TEXTURE_FILTER, 8, 16, nv4097::set_fragment_texture_dirty_bit>();
		bind_range<NV4097_SET_TEXTURE_IMAGE_RECT, 8, 16, nv4097::set_fragment_texture_dirty_bit>();
		bind_range<NV4097_SET_TEXTURE_BORDER_COLOR, 8, 16, nv4097::set_fragment_texture_dirty_bit>();
		bind_range<NV4097_SET_TEXTURE_CONTROL2, 1, 16, nv4097::set_fragment_texture_dirty_bit>();
		bind_range<NV4097_SET_TEXTURE_CONTROL3, 1, 16, nv4097::set_fragment_texture_dirty_bit>();
		bind_range<NV4097_SET_VERTEX_TEXTURE_OFFSET, 8, 4, nv4097::set_vertex_texture_dirty_bit>();
		bind_range<NV4097_SET_VERTEX_TEXTURE_FORMAT, 8, 4, nv4097::set_vertex_texture_dirty_bit>();
		bind_range<NV4097_SET_VERTEX_TEXTURE_ADDRESS, 8, 4, nv4097::set_vertex_texture_dirty_bit>();
		bind_range<NV4097_SET_VERTEX_TEXTURE_CONTROL0, 8, 4, nv4097::set_vertex_texture_dirty_bit>();
		bind_range<NV4097_SET_VERTEX_TEXTURE_CONTROL3, 8, 4, nv4097::set_vertex_texture_dirty_bit>();
		bind_range<NV4097_SET_VERTEX_TEXTURE_FILTER, 8, 4, nv4097::set_vertex_texture_dirty_bit>();
		bind_range<NV4097_SET_VERTEX_TEXTURE_IMAGE_RECT, 8, 4, nv4097::set_vertex_texture_dirty_bit>();
		bind_range<NV4097_SET_VERTEX_TEXTURE_BORDER_COLOR, 8, 4, nv4097::set_vertex_texture_dirty_bit>();
		bind(NV4097_SET_RENDER_ENABLE, nv4097::set_render_mode);
		bind(NV4097_SET_ZCULL_EN, nv4097::set_zcull_render_enable);
		bind(NV4097_SET_ZCULL_STATS_ENABLE, nv4097::set_zcull_stats_enable);
		bind(NV4097_SET_ZPASS_PIXEL_COUNT_ENABLE, nv4097::set_zcull_pixel_count_enable);
		bind(NV4097_CLEAR_ZCULL_SURFACE, nv4097::clear_zcull);
		bind(NV4097_SET_DEPTH_TEST_ENABLE, nv4097::set_surface_options_dirty_bit);
		bind(NV4097_SET_DEPTH_FUNC, nv4097::set_surface_options_dirty_bit);
		bind(NV4097_SET_DEPTH_MASK, nv4097::set_surface_options_dirty_bit);
		bind(NV4097_SET_COLOR_MASK, nv4097::set_color_mask);
		bind(NV4097_SET_COLOR_MASK_MRT, nv4097::set_surface_options_dirty_bit);
		bind(NV4097_SET_TWO_SIDED_STENCIL_TEST_ENABLE, nv4097::set_surface_options_dirty_bit);
		bind(NV4097_SET_STENCIL_TEST_ENABLE, nv4097::set_surface_options_dirty_bit);
		bind(NV4097_SET_STENCIL_MASK, nv4097::set_surface_options_dirty_bit);
		bind(NV4097_SET_STENCIL_OP_ZPASS, nv4097::set_stencil_op);
		bind(NV4097_SET_STENCIL_OP_FAIL, nv4097::set_stencil_op);
		bind(NV4097_SET_STENCIL_OP_ZFAIL, nv4097::set_stencil_op);
		bind(NV4097_SET_BACK_STENCIL_MASK, nv4097::set_surface_options_dirty_bit);
		bind(NV4097_SET_BACK_STENCIL_OP_ZPASS, nv4097::set_stencil_op);
		bind(NV4097_SET_BACK_STENCIL_OP_FAIL, nv4097::set_stencil_op);
		bind(NV4097_SET_BACK_STENCIL_OP_ZFAIL, nv4097::set_stencil_op);
		bind(NV4097_WAIT_FOR_IDLE, nv4097::sync);
		bind(NV4097_INVALIDATE_L2, nv4097::set_shader_program_dirty);
		bind(NV4097_SET_SHADER_PROGRAM, nv4097::set_shader_program_dirty);

		bind(NV4097_SET_TRANSFORM_PROGRAM_START, nv4097::set_transform_program_start);
		bind(NV4097_SET_VERTEX_ATTRIB_OUTPUT_MASK, nv4097::set_vertex_attribute_output_mask);
		bind(NV4097_SET_VERTEX_DATA_BASE_OFFSET, nv4097::set_vertex_base_offset);
		bind(NV4097_SET_VERTEX_DATA_BASE_INDEX, nv4097::set_index_base_offset);
		bind_range<NV4097_SET_VERTEX_DATA_ARRAY_OFFSET, 1, 16, nv4097::set_vertex_array_offset>();
		bind(NV4097_SET_INDEX_ARRAY_DMA, nv4097::check_index_array_dma);
		bind(NV4097_SET_BLEND_EQUATION, nv4097::set_blend_equation);
		bind(NV4097_SET_BLEND_FUNC_SFACTOR, nv4097::set_blend_factor);
		bind(NV4097_SET_BLEND_FUNC_DFACTOR, nv4097::set_blend_factor);

		//NV308A (0xa400..0xbffc!)
		bind_array(NV308A_COLOR, 1, 256 * 7, nv308a::color::impl);

		//NV3089
		bind(NV3089_IMAGE_IN, nv3089::image_in);

		//NV0039
		bind(NV0039_BUFFER_NOTIFY, nv0039::buffer_notify);

		// lv1 hypervisor
		bind_array(GCM_SET_USER_COMMAND, 1, 2, user_command);
		bind_range<GCM_FLIP_HEAD, 1, 2, gcm::driver_flip>();
		bind_range<GCM_DRIVER_QUEUE, 1, 8, gcm::queue_flip>();

		// custom methods
		bind(GCM_FLIP_COMMAND, flip_command);

		// FIFO
		bind(FIFO::FIFO_DRAW_BARRIER >> 2, fifo::draw_barrier);

		// REGS(ctx)->init();
		method_registers.init();

		return true;
	}();
}
