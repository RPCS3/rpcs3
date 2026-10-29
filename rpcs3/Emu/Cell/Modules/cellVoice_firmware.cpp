#include "stdafx.h"
#include "cellVoice_firmware.h"

#include "Emu/Cell/PPUModule.h"
#include "Emu/Cell/lv2/sys_prx.h"
#include "Emu/Cell/lv2/sys_timer.h"
#include "Emu/IdManager.h"
#include "cellAdec.h"
#include "cellCelp8Enc.h"
#include "cellSpurs.h"
#include "cellSysmodule.h"
#include "util/serialization.hpp"

#include <cmath>
#include <map>

LOG_CHANNEL(cellVoice_firmware);

extern u32 ppu_get_exported_func_addr(u32 fnid, const std::string& module_name);
extern error_code cellSysmoduleLoadModule(ppu_thread&, u16 id);
extern error_code cellSysmoduleUnloadModule(ppu_thread&, u16 id);
extern error_code cellAdecQueryAttr(ppu_thread&, vm::ptr<CellAdecType>, vm::ptr<CellAdecAttr>);
extern error_code cellAdecOpenExt(ppu_thread&, vm::ptr<CellAdecType>, vm::ptr<CellAdecResourceEx>, vm::ptr<CellAdecCb>, vm::pptr<AdecContext>);
extern error_code cellAdecStartSeq(ppu_thread&, vm::ptr<AdecContext>, vm::ptr<void>);
extern error_code cellAdecDecodeAu(ppu_thread&, vm::ptr<AdecContext>, vm::ptr<CellAdecAuInfo>);
extern error_code cellAdecGetPcmItem(ppu_thread&, vm::ptr<AdecContext>, vm::pptr<CellAdecPcmItem>);
extern error_code cellAdecGetPcm(ppu_thread&, vm::ptr<AdecContext>, vm::ptr<f32>);
extern error_code cellAdecEndSeq(ppu_thread&, vm::ptr<AdecContext>);
extern error_code cellAdecClose(ppu_thread&, vm::ptr<AdecContext>);

namespace voice_firmware
{
	namespace
	{
		// Public CELP8 framing parameters, not codec implementation data.
		constexpr config configurations[] =
		{
			{3850, 0, 320, 20},
			{4650, 2, 320, 24},
			{5700, 6, 160, 15},
			{7300, 12, 160, 19},
		};

		enum encoder_op : u32 { query, open, close, start_sequence, end_sequence, encode_frame, wait_output, get_au, op_count };
		constexpr u32 encoder_nids[op_count] =
		{
			0x2d677e0c, 0xcd48ad62, 0xfd2566b4, 0x0f6ab57b,
			0xbbbc2c1c, 0x2099f86e, 0x29da1ea6, 0x48c5020d,
		};
		enum spurs_op : u32 { spurs_attribute, spurs_initialize, spurs_finalize, spurs_op_count };
		constexpr u32 spurs_nids[spurs_op_count] = {0x95180230, 0xaa6269a8, 0xca4c4600};
		constexpr u32 pcm_capacity = 320 * sizeof(f32);
		constexpr u32 packet_capacity = 128;
		constexpr u32 callback_offset = pcm_capacity + packet_capacity;
		constexpr u32 auxiliary_size = callback_offset + 128;
		constexpr u32 decoder_frame_done = (1u << CELL_ADEC_MSG_TYPE_AUDONE) | (1u << CELL_ADEC_MSG_TYPE_PCMOUT);
		constexpr u16 module_ids[] = {CELL_SYSMODULE_CELP8ENC, CELL_SYSMODULE_ADEC_CELP8, CELL_SYSMODULE_SPURS};

		struct callback_state
		{
			atomic_t<be_t<u32>> events;
			atomic_t<be_t<u32>> error;
		};

		s32 decoder_callback(vm::ptr<AdecContext>, CellAdecMsgType type, s32 message, vm::ptr<void> arg)
		{
			const auto state = vm::static_ptr_cast<callback_state>(arg);
			if (type == CELL_ADEC_MSG_TYPE_ERROR || (type == CELL_ADEC_MSG_TYPE_PCMOUT && message))
			{
				state->error = message ? static_cast<u32>(message) : static_cast<u32>(CELL_ADEC_ERROR_FATAL);
			}
			if (static_cast<u32>(type) <= CELL_ADEC_MSG_TYPE_SEQDONE)
			{
				state->events.fetch_or(1u << type);
			}
			return CELL_OK;
		}

		bool interrupted(ppu_thread& ppu)
		{
			return ppu.test_stopped();
		}

		u32 allocate(ppu_thread& ppu, u32 size)
		{
			ppu.state += cpu_flag::wait;
			return vm::alloc(size, vm::user64k);
		}

		void release(ppu_thread& ppu, u32& address)
		{
			if (address)
			{
				ppu.state += cpu_flag::wait;
				ensure(vm::dealloc(address, vm::user64k));
				address = 0;
			}
		}

		bool contains(const lv2_prx& prx, u32 address, u32 size, u32 flags)
		{
			return std::any_of(prx.segs.begin(), prx.segs.end(), [&](const ppu_segment& segment)
			{
				return segment.type == 1 && (segment.flags & flags) == flags && address >= segment.addr
					&& u64{address} + size <= u64{segment.addr} + segment.size;
			});
		}

		bool descriptor_valid(const lv2_prx& prx, u32 address)
		{
			if ((address & 3) || !contains(prx, address, 8, 4) || !vm::check_addr(address, vm::page_readable, 8))
			{
				return false;
			}
			const u32 code = vm::read32(address);
			const u32 toc = vm::read32(address + 4);
			return !(code & 3) && contains(prx, code, 4, 1) && vm::check_addr(toc, vm::page_readable, 4);
		}

		u32 module_owner(u32 address, std::string_view filename)
		{
			const auto [prx, id] = idm::select<lv2_obj, lv2_prx>([&](u32 id, lv2_prx& candidate) -> u32
			{
				return candidate.name == filename && candidate.state == PRX_STATE_STARTED
					&& contains(candidate, address, 4, 4) ? id : 0;
			});
			return id;
		}

		s32 wait_callback(ppu_thread& ppu, vm::ptr<callback_state> state, u32 expected, bool check_error = true)
		{
			// Count guest sleeps rather than host elapsed time, so pausing the emulator
			// does not turn a valid pending decode into a timeout.
			for (u32 attempts = 0; attempts < 5000; attempts++)
			{
				if (check_error && state->error.load())
					return state->error.load();
				if ((state->events.load() & expected) == expected)
					return CELL_OK;
				sys_timer_usleep(ppu, 1000);
				if (interrupted(ppu))
					return CELL_ECANCELED;
			}
			return CELL_ETIMEDOUT;
		}
	}

	const config* find_config(u32 bitrate)
	{
		for (const auto& value : configurations)
		{
			if (value.bitrate == bitrate) return &value;
		}
		return nullptr;
	}

	void backend_register()
	{
		REG_HIDDEN_FUNC(decoder_callback);
	}

	struct backend::impl
	{
		struct pool
		{
			u32 address = 0;
			u32 users = 0;
			bool initialized = false;
			bool initializing = false;
		};

		struct codec
		{
			u32 bitrate = 0;
			u32 arena = 0;
			u32 allocation_size = 0;
			u32 work_size = 0;
			u32 handle = 0;
			u32 pool_index = 0;
			u32 fault = 0;
			bool pool_acquired = false;
			bool opening = false;
			bool sequence = false;
			bool in_flight = false;
			bool pcm_item_taken = false;
			bool ending = false;

			u32 pcm_address() const { return arena + utils::align(work_size, 128); }
			u32 packet_address() const { return pcm_address() + pcm_capacity; }
			vm::ptr<callback_state> callback() const { return vm::ptr<callback_state>::make(pcm_address() + callback_offset); }
		};

		bool started = false;
		bool validated = false;
		u8 module_refs = 0;
		u32 encoder_module = 0, decoder_module = 0, spurs_module = 0;
		std::array<u32, op_count> functions{}; // Re-resolved after loading a savestate.
		std::array<u32, spurs_op_count> spurs_functions{};
		std::vector<pool> pools;
		std::map<key, codec> codecs;

		template <typename F>
		vm::ptr<F> function(encoder_op op) const
		{
			return vm::ptr<F>::make(functions[op]);
		}

		template <typename F>
		vm::ptr<F> function(spurs_op op) const
		{
			return vm::ptr<F>::make(spurs_functions[op]);
		}

		s32 validate()
		{
			for (u32 i = 0; i < spurs_op_count; i++)
				spurs_functions[i] = ppu_get_exported_func_addr(spurs_nids[i], "cellSpurs");
			const u32 spurs_id = module_owner(spurs_functions[spurs_initialize], "libsre.sprx");
			const auto spurs = idm::get_unlocked<lv2_obj, lv2_prx>(spurs_id);
			if (!spurs || (spurs_module && spurs_module != spurs_id)
				|| !std::all_of(spurs_functions.begin(), spurs_functions.end(), [&](u32 address) { return descriptor_valid(*spurs, address); }))
			{
				cellVoice_firmware.error("Firmware SPURS exports are unavailable (missing firmware or HLE override)");
				return CELL_ENOSYS;
			}
			for (u32 i = 0; i < op_count; i++)
			{
				functions[i] = ppu_get_exported_func_addr(encoder_nids[i], "cellCelp8Enc");
			}
			const u32 encoder_id = module_owner(functions[query], "libcelp8enc.sprx");
			const auto encoder = idm::get_unlocked<lv2_obj, lv2_prx>(encoder_id);
			if (!encoder || (encoder_module && encoder_module != encoder_id)
				|| !std::all_of(functions.begin(), functions.end(), [&](u32 address) { return descriptor_valid(*encoder, address); }))
			{
				cellVoice_firmware.error("Firmware CELP8 encoder exports are unavailable (missing firmware or HLE override)");
				return CELL_ENOSYS;
			}

			const auto var = ppu_module_manager::cellCelp8Dec.variables.find(0xf0190c6c);
			const u32 table = var == ppu_module_manager::cellCelp8Dec.variables.end() || !var->second.export_addr ? 0 : *var->second.export_addr;
			const u32 decoder_id = module_owner(table, "libcelp8dec.sprx");
			const auto decoder = idm::get_unlocked<lv2_obj, lv2_prx>(decoder_id);
			if (!decoder || (decoder_module && decoder_module != decoder_id)
				|| !contains(*decoder, table, sizeof(CellAdecCoreOps), 4)
				|| !vm::check_addr(table, vm::page_readable, sizeof(CellAdecCoreOps)))
			{
				cellVoice_firmware.error("Firmware CELP8 decoder operations are unavailable (missing firmware or HLE override)");
				return CELL_ENOSYS;
			}
			for (u32 i = 0; i < sizeof(CellAdecCoreOps) / sizeof(u32); i++)
			{
				if (!descriptor_valid(*decoder, vm::read32(table + i * sizeof(u32))))
				{
					cellVoice_firmware.error("Invalid firmware CELP8 decoder operation %u", i);
					return CELL_ENOSYS;
				}
			}
			encoder_module = encoder_id;
			decoder_module = decoder_id;
			spurs_module = spurs_id;
			validated = true;
			return CELL_OK;
		}

		s32 acquire_pool(ppu_thread& ppu, u32& index)
		{
			for (index = 0; index < pools.size(); index++)
			{
				if (pools[index].initialized && pools[index].users < 16)
				{
					pools[index].users++;
					return CELL_OK;
				}
			}
			if (pools.size() >= 3) return CELL_ENOMEM;
			auto& owner = pools.emplace_back();
			owner.address = allocate(ppu, 0x10000);
			if (!owner.address)
			{
				pools.pop_back();
				return CELL_ENOMEM;
			}
			std::memset(vm::base(owner.address), 0, sizeof(CellSpurs));
			ppu.state += cpu_flag::wait;
			vm::var<CellSpursAttribute> attr;
			s32 error = function<s32(vm::ptr<CellSpursAttribute>, u32, u32, u32, s32, s32, u32)>(spurs_attribute)
				(ppu, +attr, 2, 0x20010, 1, 128, 2000, 0);
			if (!error)
			{
				owner.initializing = true;
				// Match the implementation used by the codecs' cellSpurs imports.
				// A firmware taskset cannot run inside an HLE-created SPURS kernel.
				error = function<s32(vm::ptr<CellSpurs>, vm::cptr<CellSpursAttribute>)>(spurs_initialize)
					(ppu, vm::ptr<CellSpurs>::make(owner.address), +attr);
			}
			if (interrupted(ppu)) return CELL_ECANCELED;
			owner.initializing = false;
			if (error)
			{
				release(ppu, owner.address);
				pools.pop_back();
				return error;
			}
			owner.initialized = true;
			owner.users = 1;
			return CELL_OK;
		}

		s32 open_codec(ppu_thread& ppu, const key& id, codec& value)
		{
			const config& cfg = *find_config(value.bitrate);
			ppu.state += cpu_flag::wait;
			vm::var<CellAdecType> type;
			type->audioCodecType = CELL_ADEC_TYPE_CELP8;
			if (id.encode)
			{
				vm::var<CellCelp8EncAttr> attr;
				const s32 error = function<s32(vm::ptr<CellCelp8EncAttr>)>(query)(ppu, attr);
				if (interrupted(ppu)) return CELL_ECANCELED;
				if (error) return error;
				value.work_size = attr->workMemSize;
			}
			else
			{
				vm::var<CellAdecAttr> attr;
				const s32 error = ppu_execute<&cellAdecQueryAttr>(ppu, +type, +attr);
				if (interrupted(ppu)) return CELL_ECANCELED;
				if (error) return error;
				value.work_size = attr->workMemSize;
			}
			if (!value.work_size || value.work_size > 16 * 1024 * 1024) return CELL_ENOMEM;
			value.allocation_size = utils::align(utils::align(value.work_size, 128) + auxiliary_size, 0x10000);
			value.arena = allocate(ppu, value.allocation_size);
			if (!value.arena) return CELL_ENOMEM;
			std::memset(vm::base(value.arena), 0, value.allocation_size);
			if (const s32 error = acquire_pool(ppu, value.pool_index)) return error;
			value.pool_acquired = true;

			ppu.state += cpu_flag::wait;
			vm::var<u32> handle;
			*handle = 0;
			s32 error;
			value.opening = true;
			if (id.encode)
			{
				vm::var<CellCelp8EncResourceEx> resource{};
				resource->totalMemSize = value.work_size;
				resource->startAddr = vm::cast(value.arena);
				resource->spurs = vm::cast(pools[value.pool_index].address);
				std::fill(std::begin(resource->priority), std::end(resource->priority), 15);
				resource->maxContention = 1;
				error = function<s32(vm::ptr<CellCelp8EncResourceEx>, vm::ptr<u32>)>(open)(ppu, resource, handle);
			}
			else
			{
				vm::var<CellAdecResourceEx> resource{};
				resource->totalMemSize = value.work_size;
				resource->startAddr = value.arena;
				resource->ppuThreadPriority = 1000;
				resource->ppuThreadStackSize = 0x10000;
				resource->spurs_addr = pools[value.pool_index].address;
				resource->priority = 0x0f0f0f0f0f0f0f0full;
				resource->maxContention = 1;
				vm::var<CellAdecCb> callback{};
				callback->cbFunc = vm::cast(g_fxo->get<ppu_function_manager>().func_addr(FIND_FUNC(decoder_callback)));
				callback->cbArg = vm::cast(value.callback().addr());
				error = ppu_execute<&cellAdecOpenExt>(ppu, +type, +resource, +callback, vm::pptr<AdecContext>::make(handle.addr()));
			}
			value.handle = *handle;
			if (interrupted(ppu)) return CELL_ECANCELED;
			value.opening = false;
			if (error || !value.handle) return error ? error : static_cast<s32>(CELL_EFAULT);

			ppu.state += cpu_flag::wait;
			if (id.encode)
			{
				vm::var<CellCelp8EncParam> param{};
				param->excitationMode = CELL_CELP8ENC_EXCITATION_MODE_MPE;
				param->sampleRate = CELL_CELP8ENC_FS_8kHz;
				param->configuration = cfg.configuration;
				param->wordSize = CELL_CELP8ENC_WORD_SZ_FLOAT;
				param->outBuff = vm::cast(value.packet_address());
				param->outSize = packet_capacity;
				error = function<s32(vm::ptr<void>, vm::ptr<CellCelp8EncParam>)>(start_sequence)(ppu, vm::cast(value.handle), param);
			}
			else
			{
				vm::var<CellAdecParamCelp8> param{};
				param->excitationMode = CELL_ADEC_CELP8_EXCITATION_MODE_MPE;
				param->sampleRate = CELL_ADEC_FS_8kHz;
				param->configuration = cfg.configuration;
				param->wordSize = CELL_ADEC_CELP8_WORD_SZ_FLOAT;
				error = ppu_execute<&cellAdecStartSeq>(ppu, vm::ptr<AdecContext>::make(value.handle), vm::cast(param.addr()));
			}
			if (!error) value.sequence = true;
			return interrupted(ppu) ? static_cast<s32>(CELL_ECANCELED) : error;
		}

		s32 consume_pcm(ppu_thread& ppu, codec& value, std::vector<f32>* output)
		{
			ppu.state += cpu_flag::wait;
			vm::var<vm::ptr<CellAdecPcmItem>> item;
			s32 error = ppu_execute<&cellAdecGetPcmItem>(ppu, vm::ptr<AdecContext>::make(value.handle), +item);
			if (interrupted(ppu)) return CELL_ECANCELED;
			if (error) return error;
			value.pcm_item_taken = true;
			const u32 size = (*item)->size;
			const u32 status = (*item)->status;
			const bool valid = size == find_config(value.bitrate)->samples * sizeof(f32) && !status;
			// Even an erroneous PCM item must be released before the decoder can stop.
			error = ppu_execute<&cellAdecGetPcm>(ppu, vm::ptr<AdecContext>::make(value.handle),
				valid && output ? vm::ptr<f32>::make(value.pcm_address()) : vm::null);
			if (interrupted(ppu)) return CELL_ECANCELED;
			if (error) return error;
			value.pcm_item_taken = false;
			value.in_flight = false;
			if (!valid) return status ? static_cast<s32>(status) : static_cast<s32>(CELL_EINVAL);
			if (output)
			{
				const auto samples = vm::ptr<f32>::make(value.pcm_address());
				output->resize(size / sizeof(f32));
				for (u32 i = 0; i < output->size(); i++)
				{
					const f32 sample = samples[i];
					if (!std::isfinite(sample)) return CELL_EINVAL;
					(*output)[i] = sample * 32768.0f;
				}
			}
			return CELL_OK;
		}

		s32 close_codec(ppu_thread& ppu, const key& id, codec& value)
		{
			// A stopped PPU can leave a guest open call before its ownership result
			// is published. Keep that allocation for process teardown, not reuse.
			if (value.opening) return CELL_ECANCELED;
			if (value.handle)
			{
				if (!id.encode && value.in_flight)
				{
					if (const s32 error = wait_callback(ppu, value.callback(), decoder_frame_done, false)) return error;
					if (value.pcm_item_taken)
					{
						const s32 error = ppu_execute<&cellAdecGetPcm>(ppu, vm::ptr<AdecContext>::make(value.handle), vm::null);
						if (interrupted(ppu)) return CELL_ECANCELED;
						if (error) return error;
						value.pcm_item_taken = value.in_flight = false;
					}
					else if (const s32 error = consume_pcm(ppu, value, nullptr); error && value.in_flight) return error;
				}
				if (value.sequence)
				{
					s32 error = CELL_OK;
					if (id.encode)
					{
						error = function<s32(vm::ptr<void>)>(end_sequence)(ppu, vm::cast(value.handle));
					}
					else
					{
						if (!value.ending)
						{
							value.callback()->events = 0;
							value.callback()->error = 0;
							error = ppu_execute<&cellAdecEndSeq>(ppu, vm::ptr<AdecContext>::make(value.handle));
							if (!error) value.ending = true;
						}
						if (!error && !interrupted(ppu)) error = wait_callback(ppu, value.callback(), 1u << CELL_ADEC_MSG_TYPE_SEQDONE);
					}
					if (interrupted(ppu)) return CELL_ECANCELED;
					if (error) return error;
					value.sequence = value.in_flight = value.ending = false;
				}
				const s32 error = id.encode
					? function<s32(vm::ptr<void>)>(close)(ppu, vm::cast(value.handle))
					: static_cast<s32>(ppu_execute<&cellAdecClose>(ppu, vm::ptr<AdecContext>::make(value.handle)));
				if (interrupted(ppu)) return CELL_ECANCELED;
				if (error) return error;
				value.handle = 0;
			}
			release(ppu, value.arena);
			if (value.pool_acquired)
			{
				ensure(value.pool_index < pools.size() && pools[value.pool_index].users);
				pools[value.pool_index].users--;
				value.pool_acquired = false;
			}
			return CELL_OK;
		}
	};

	backend::backend() : m_impl(std::make_unique<impl>()) {}
	backend::~backend() = default;

	s32 backend::start(ppu_thread& ppu)
	{
		if (interrupted(ppu)) return CELL_ECANCELED;
		auto& state = *m_impl;
		if (state.started) return state.validated ? CELL_OK : state.validate();
		if (state.module_refs || !state.codecs.empty() || !state.pools.empty())
		{
			if (const s32 error = stop(ppu)) return error;
		}
		// Keep SPURS loaded before opening any codec and until every pool is closed.
		// Otherwise a later game load can rebind codec imports during a live stream.
		for (const u32 i : {2u, 0u, 1u})
		{
			if (state.module_refs & (1u << i)) continue;
			const s32 error = ppu_execute<&cellSysmoduleLoadModule>(ppu, module_ids[i]);
			if (!error) state.module_refs |= 1u << i;
			if (interrupted(ppu)) return CELL_ECANCELED;
			if (error)
			{
				const s32 cleanup_error = stop(ppu);
				if (cleanup_error) cellVoice_firmware.error("CELP8 startup cleanup failed: 0x%x", cleanup_error);
				return error;
			}
		}
		if (const s32 error = state.validate())
		{
			const s32 cleanup_error = stop(ppu);
			if (cleanup_error) cellVoice_firmware.error("CELP8 startup cleanup failed: 0x%x", cleanup_error);
			return error;
		}
		state.started = true;
		return CELL_OK;
	}

	result backend::process(ppu_thread& ppu, const request& input)
	{
		result output{input.codec};
		if (interrupted(ppu)) { output.error = CELL_ECANCELED; return output; }
		auto& state = *m_impl;
		const config* cfg = find_config(input.bitrate);
		if (!state.started || !cfg || (input.codec.encode ? input.pcm.size() != cfg->samples : input.packet.size() != cfg->bytes))
		{
			output.error = CELL_EINVAL;
			return output;
		}
		if (!state.validated)
		{
			if (const s32 error = state.validate()) { output.error = error; return output; }
		}
		auto [it, inserted] = state.codecs.try_emplace(input.codec);
		auto& value = it->second;
		if (inserted)
		{
			value.bitrate = input.bitrate;
			value.fault = state.open_codec(ppu, input.codec, value);
			if (value.fault && !interrupted(ppu))
			{
				// Preserve the fault for this generation, but release any resources
				// whose close completed. A failed close retains their ownership.
				const s32 cleanup_error = state.close_codec(ppu, input.codec, value);
				if (cleanup_error) cellVoice_firmware.error("CELP8 open cleanup failed: 0x%x", cleanup_error);
			}
		}
		if (value.fault || value.bitrate != input.bitrate)
		{
			output.error = value.fault ? value.fault : static_cast<u32>(CELL_EINVAL);
			return output;
		}

		s32 error = CELL_OK;
		ppu.state += cpu_flag::wait;
		if (input.codec.encode)
		{
			const auto pcm = vm::ptr<f32>::make(value.pcm_address());
			for (u32 i = 0; i < cfg->samples; i++)
			{
				if (!std::isfinite(input.pcm[i])) { output.error = CELL_EINVAL; return output; }
				pcm[i] = std::clamp(input.pcm[i] / 32768.0f, -1.0f, 1.0f);
			}
			vm::var<CellCelp8EncPcmInfo> info;
			info->startAddr = vm::cast(value.pcm_address());
			info->size = cfg->samples * sizeof(f32);
			error = state.function<s32(vm::ptr<void>, vm::ptr<CellCelp8EncPcmInfo>)>(encode_frame)(ppu, vm::cast(value.handle), info);
			if (!error) value.in_flight = true;
			if (!error && !interrupted(ppu)) error = state.function<s32(vm::ptr<void>)>(wait_output)(ppu, vm::cast(value.handle));
			if (!error && !interrupted(ppu))
			{
				ppu.state += cpu_flag::wait;
				vm::var<CellCelp8EncAuInfo> au;
				error = state.function<s32(vm::ptr<void>, vm::ptr<void>, vm::ptr<CellCelp8EncAuInfo>)>(get_au)(ppu, vm::cast(value.handle), vm::cast(value.packet_address()), au);
				if (!error && !interrupted(ppu))
				{
					value.in_flight = false;
					if ((au->size && au->size != cfg->bytes) || au->startAddr.addr() != value.packet_address()) error = CELL_EINVAL;
					else
					{
						const auto packet = vm::ptr<u8>::make(value.packet_address());
						output.packet.assign(packet.get_ptr(), packet.get_ptr() + au->size);
					}
				}
			}
		}
		else
		{
			value.callback()->events = 0;
			value.callback()->error = 0;
			std::memcpy(vm::base(value.packet_address()), input.packet.data(), input.packet.size());
			vm::var<CellAdecAuInfo> au;
			au->startAddr = vm::cast(value.packet_address());
			au->size = cfg->bytes;
			au->pts = {CODEC_TS_INVALID, CODEC_TS_INVALID};
			// CELP8 uses the low seven bits as a channel index (0..15). Each
			// handle belongs to one stream; host generations never enter this field.
			au->userData = 0;
			error = ppu_execute<&cellAdecDecodeAu>(ppu, vm::ptr<AdecContext>::make(value.handle), +au);
			if (!error) value.in_flight = true;
			if (!error && !interrupted(ppu)) error = wait_callback(ppu, value.callback(), decoder_frame_done);
			if (!error && !interrupted(ppu)) error = state.consume_pcm(ppu, value, &output.pcm);
		}
		if (interrupted(ppu)) error = CELL_ECANCELED;
		if (error)
		{
			value.fault = output.error = error;
			output.packet.clear();
			output.pcm.clear();
			cellVoice_firmware.error("CELP8 %s failed for voice port 0x%x: 0x%x", input.codec.encode ? "encode" : "decode", input.codec.port, error);
		}
		return output;
	}

	s32 backend::reconcile(ppu_thread& ppu, const std::vector<key>& live_keys)
	{
		if (interrupted(ppu)) return CELL_ECANCELED;
		auto& state = *m_impl;
		// Reconciliation can close a restored stream before another frame is sent.
		if (!state.codecs.empty() && !state.validated)
		{
			if (const s32 error = state.validate()) return error;
		}
		s32 first_error = CELL_OK;
		for (auto it = state.codecs.begin(); it != state.codecs.end();)
		{
			if (std::find(live_keys.begin(), live_keys.end(), it->first) != live_keys.end()) { ++it; continue; }
			const s32 error = state.close_codec(ppu, it->first, it->second);
			if (interrupted(ppu)) return CELL_ECANCELED;
			if (error)
			{
				if (!first_error) first_error = error;
				++it;
				continue;
			}
			it = state.codecs.erase(it);
		}
		return first_error;
	}

	s32 backend::stop(ppu_thread& ppu)
	{
		if (interrupted(ppu)) return CELL_ECANCELED;
		auto& state = *m_impl;
		if (!state.validated && std::any_of(state.pools.begin(), state.pools.end(), [](const impl::pool& owner) { return owner.initialized; }))
			if (const s32 error = state.validate()) return error;
		state.started = false;
		if (const s32 error = reconcile(ppu, {})) return error;
		for (auto& owner : state.pools)
		{
			if (owner.initializing) return CELL_ECANCELED;
			if (owner.initialized)
			{
				const s32 error = state.function<s32(vm::ptr<CellSpurs>)>(spurs_finalize)(ppu, vm::ptr<CellSpurs>::make(owner.address));
				if (interrupted(ppu)) return CELL_ECANCELED;
				if (error) return error;
				owner.initialized = false;
			}
			release(ppu, owner.address);
		}
		state.pools.clear();
		for (const u32 i : {1u, 0u, 2u})
		{
			if (!(state.module_refs & (1u << i))) continue;
			const s32 error = ppu_execute<&cellSysmoduleUnloadModule>(ppu, module_ids[i]);
			if (interrupted(ppu)) return CELL_ECANCELED;
			if (error) return error;
			state.module_refs &= ~(1u << i);
		}
		state.started = state.validated = false;
		state.encoder_module = state.decoder_module = state.spurs_module = 0;
		state.functions.fill(0);
		state.spurs_functions.fill(0);
		return CELL_OK;
	}

	void backend::save(utils::serial& ar)
	{
		auto& state = *m_impl;
		ar(state.started, state.module_refs, state.encoder_module, state.decoder_module, state.spurs_module);
		u32 count = static_cast<u32>(state.pools.size());
		ar(count);
		ensure(count <= 3);
		if (!ar.is_writing()) state.pools.resize(count);
		for (auto& owner : state.pools) ar(owner.address, owner.users, owner.initialized, owner.initializing);
		count = static_cast<u32>(state.codecs.size());
		ar(count);
		ensure(count <= 128);
		if (!ar.is_writing()) state.codecs.clear();
		auto transfer = [&](key& id, impl::codec& value)
		{
			ar(id.port, id.generation, id.encode, value.bitrate, value.arena, value.allocation_size, value.work_size,
				value.handle, value.pool_index, value.fault, value.pool_acquired, value.opening,
				value.sequence, value.in_flight, value.pcm_item_taken, value.ending);
		};
		if (ar.is_writing())
		{
			for (auto& [id, value] : state.codecs)
			{
				auto copied_id = id;
				transfer(copied_id, value);
			}
		}
		else
		{
			for (u32 i = 0; i < count; i++)
			{
				key id;
				impl::codec value;
				transfer(id, value);
				ensure(find_config(value.bitrate));
				ensure(state.codecs.emplace(id, value).second);
			}
			state.validated = false;
			state.functions.fill(0);
			state.spurs_functions.fill(0);
		}
	}
}
