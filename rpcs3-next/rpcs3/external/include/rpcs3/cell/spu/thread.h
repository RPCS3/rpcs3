#pragma once

#include <common/basic_types.h>
#include <common/platform.h>
#include <rpcs3/cell/common.h>
#include <rpcs3/cell/spu/context.h>
#include <rpcs3/cell/spu/mfc.h>

namespace rpcs3
{
	using namespace common;

	struct lv2_event_queue_t;
	struct lv2_spu_group_t;
	struct lv2_int_tag_t;

	namespace cell
	{
		namespace spu
		{
			enum channel : u32
			{
				RdEventStat = 0,  // Read event status with mask applied
				WrEventMask = 1,  // Write event mask
				WrEventAck = 2,  // Write end of event processing
				RdSigNotify1 = 3,  // Signal notification 1
				RdSigNotify2 = 4,  // Signal notification 2
				WrDec = 7,  // Write decrementer count
				RdDec = 8,  // Read decrementer count
				RdEventMask = 11, // Read event mask
				RdMachStat = 13, // Read SPU run status
				WrSRR0 = 14, // Write SPU machine state save/restore register 0 (SRR0)
				RdSRR0 = 15, // Read SPU machine state save/restore register 0 (SRR0)
				WrOutMbox = 28, // Write outbound mailbox contents
				RdInMbox = 29, // Read inbound mailbox contents
				WrOutIntrMbox = 30, // Write outbound interrupt mailbox contents (interrupting PPU)
			};

			namespace events
			{
				enum : u32
				{
					MS = 0x1000, // Multisource Synchronization event
					A = 0x800,  // Privileged Attention event
					LR = 0x400,  // Lock Line Reservation Lost event
					S1 = 0x200,  // Signal Notification Register 1 available
					S2 = 0x100,  // Signal Notification Register 2 available
					LE = 0x80,   // SPU Outbound Mailbox available
					ME = 0x40,   // SPU Outbound Interrupt Mailbox available
					TM = 0x20,   // SPU Decrementer became negative (?)
					MB = 0x10,   // SPU Inbound mailbox available
					QV = 0x4,    // MFC SPU Command Queue available
					SN = 0x2,    // MFC List Command stall-and-notify event
					TG = 0x1,    // MFC Tag Group status update event

					implemented = LR, // Mask of implemented events

					waiting = 0x80000000, // Originally unused, set when SPU thread starts waiting on ch_event_stat
					//available  = 0x40000000, // Originally unused, channel count of the SPU_RdEventStat channel
					enabled = 0x20000000, // Originally unused, represents "SPU Interrupts Enabled" status
				};
			}

			namespace int0
			{
				// SPU Class 0 Interrupts
				enum : u64
				{
					dma_aligment = (1ull << 0),
					invalid_dma_cmd = (1ull << 1),
					spu_error = (1ull << 2),
				};
			}

			namespace int2
			{
				// SPU Class 2 Interrupts
				enum class int2 : u64
				{
					mailbox = (1ull << 0),
					spu_stop_and_signal = (1ull << 1),
					spu_halt_or_step = (1ull << 2),
					dma_tag_group_completion = (1ull << 3),
					spu_mailbox_threshold = (1ull << 4),
				};
			}

			enum class runcntl
			{
				stop_request = 0,
				run_request = 1,
			};

			// SPU Status Register bits (not accurate)
			namespace status
			{
				enum
				{
					stopped = 0x0,
					running = 0x1,
					stopped_by_stop = 0x2,
					stopped_by_halt = 0x4,
					waiting_for_channel = 0x8,
					single_step = 0x10,
				};
			}

			namespace constants
			{
				enum : u32
				{
					base_low = 0xf0000000,
					offset = 0x100000,
					snr1 = 0x5400c,
					snr2 = 0x5C00c,
				};
			}

			enum
			{
				MFC_LSA_offs = 0x3004,
				MFC_EAH_offs = 0x3008,
				MFC_EAL_offs = 0x300C,
				MFC_Size_Tag_offs = 0x3010,
				MFC_Class_CMD_offs = 0x3014,
				MFC_CMDStatus_offs = 0x3014,
				MFC_QStatus_offs = 0x3104,
				Prxy_QueryType_offs = 0x3204,
				Prxy_QueryMask_offs = 0x321C,
				Prxy_TagStatus_offs = 0x322C,
				SPU_Out_MBox_offs = 0x4004,
				SPU_In_MBox_offs = 0x400C,
				SPU_MBox_Status_offs = 0x4014,
				SPU_RunCntl_offs = 0x401C,
				SPU_Status_offs = 0x4024,
				SPU_NPC_offs = 0x4034,
				SPU_RdSigNotify1_offs = 0x1400C,
				SPU_RdSigNotify2_offs = 0x1C00C,
			};

			struct spu_channel_t
			{
				// set to true if SPU thread must be notified after SPU channel operation
				thread_local static bool notification_required;

				struct sync_var_t
				{
					bool count; // value available
					bool wait; // notification required
					u32 value;
				};

				atomic<sync_var_t> data;

			public:
				// returns true on success
				bool try_push(u32 value)
				{
					const auto old = data.atomic_op([=](sync_var_t& data)
					{
						if ((data.wait = data.count) == false)
						{
							data.count = true;
							data.value = value;
						}
					});

					return !old.count;
				}

				// push performing bitwise OR with previous value, may require notification
				void push_or(u32 value)
				{
					const auto old = data.atomic_op([=](sync_var_t& data)
					{
						data.count = true;
						data.wait = false;
						data.value |= value;
					});

					notification_required = old.wait;
				}

				// push unconditionally (overwriting previous value), may require notification
				void push(u32 value)
				{
					const auto old = data.atomic_op([=](sync_var_t& data)
					{
						data.count = true;
						data.wait = false;
						data.value = value;
					});

					notification_required = old.wait;
				}

				// returns true on success and loaded value
				std::tuple<bool, u32> try_pop()
				{
					const auto old = data.atomic_op([](sync_var_t& data)
					{
						data.wait = !data.count;
						data.count = false;
						data.value = 0; // ???
					});

					return std::tie(old.count, old.value);
				}

				// pop unconditionally (loading last value), may require notification
				u32 pop()
				{
					const auto old = data.atomic_op([](sync_var_t& data)
					{
						data.wait = false;
						data.count = false;
						// value is not cleared and may be read again
					});

					notification_required = old.wait;

					return old.value;
				}

				void set_value(u32 value, bool count = true)
				{
					data.store({ count, false, value });
				}

				u32 get_value()
				{
					return data.load().value;
				}

				u32 get_count()
				{
					return data.load().count;
				}
			};

			struct spu_channel_4_t
			{
				struct sync_var_t
				{
					struct
					{
						u32 waiting : 1;
						u32 count : 3;
					};

					u32 value0;
					u32 value1;
					u32 value2;
				};

				atomic<sync_var_t> values;
				atomic<u32> value3;

			public:
				void clear()
				{
					values = sync_var_t{};
					value3 = 0;
				}

				// push unconditionally (overwriting latest value), returns true if needs signaling
				bool push(u32 value)
				{
					value3 = value; _mm_sfence();

					return values.atomic_op([=](sync_var_t& data) -> bool
					{
						switch (data.count++)
						{
						case 0: data.value0 = value; break;
						case 1: data.value1 = value; break;
						case 2: data.value2 = value; break;
						default: data.count = 4;
						}

						if (data.waiting)
						{
							data.waiting = 0;

							return true;
						}

						return false;
					});
				}

				// returns true on success and two u32 values: data and count after removing the first element
				std::tuple<bool, u32, u32> try_pop()
				{
					return values.atomic_op([this](sync_var_t& data)
					{
						const auto result = std::make_tuple(data.count != 0, u32{ data.value0 }, u32{ data.count - 1u });

						if (data.count != 0)
						{
							data.waiting = 0;
							data.count--;

							data.value0 = data.value1;
							data.value1 = data.value2;
							_mm_lfence();
							data.value2 = this->value3;
						}
						else
						{
							data.waiting = 1;
						}

						return result;
					});
				}

				u32 get_count()
				{
					return values.raw().count;
				}

				void set_values(u32 count, u32 value0, u32 value1 = 0, u32 value2 = 0, u32 value3 = 0)
				{
					this->values.raw() = { 0, count, value0, value1, value2 };
					this->value3 = value3;
				}
			};

			struct spu_int_ctrl_t
			{
				atomic<u64> mask;
				atomic<u64> stat;

				std::shared_ptr<struct lv2_int_tag_t> tag;

				void set(u64 ints);

				void clear(u64 ints);

				void clear()
				{
					mask = 0;
					stat = 0;
					tag = nullptr;
				}
			};

			struct spu_imm_table_t
			{
				v128 fsmb[65536]; // table for FSMB, FSMBI instructions
				v128 fsmh[256]; // table for FSMH instruction
				v128 fsm[16]; // table for FSM instruction

				v128 sldq_pshufb[32]; // table for SHLQBYBI, SHLQBY, SHLQBYI instructions
				v128 srdq_pshufb[32]; // table for ROTQMBYBI, ROTQMBY, ROTQMBYI instructions
				v128 rldq_pshufb[16]; // table for ROTQBYBI, ROTQBY, ROTQBYI instructions

				class scale_table_t
				{
					std::array<__m128, 155 + 174> m_data;

				public:
					scale_table_t()
					{
						for (s32 i = -155; i < 174; i++)
						{
							m_data[i + 155] = _mm_set1_ps(static_cast<float>(exp2(i)));
						}
					}

					force_inline __m128 operator [] (s32 scale) const
					{
						return m_data[scale + 155];
					}
				}
				const scale;

				spu_imm_table_t()
				{
					for (u32 i = 0; i < sizeof(fsm) / sizeof(fsm[0]); i++)
					{
						for (u32 j = 0; j < 4; j++)
						{
							fsm[i]._u32[j] = (i & (1 << j)) ? 0xffffffff : 0;
						}
					}

					for (u32 i = 0; i < sizeof(fsmh) / sizeof(fsmh[0]); i++)
					{
						for (u32 j = 0; j < 8; j++)
						{
							fsmh[i]._u16[j] = (i & (1 << j)) ? 0xffff : 0;
						}
					}

					for (u32 i = 0; i < sizeof(fsmb) / sizeof(fsmb[0]); i++)
					{
						for (u32 j = 0; j < 16; j++)
						{
							fsmb[i]._u8[j] = (i & (1 << j)) ? 0xff : 0;
						}
					}

					for (u32 i = 0; i < sizeof(sldq_pshufb) / sizeof(sldq_pshufb[0]); i++)
					{
						for (u32 j = 0; j < 16; j++)
						{
							sldq_pshufb[i]._u8[j] = static_cast<u8>(j - i);
						}
					}

					for (u32 i = 0; i < sizeof(srdq_pshufb) / sizeof(srdq_pshufb[0]); i++)
					{
						for (u32 j = 0; j < 16; j++)
						{
							srdq_pshufb[i]._u8[j] = (j + i > 15) ? 0xff : static_cast<u8>(j + i);
						}
					}

					for (u32 i = 0; i < sizeof(rldq_pshufb) / sizeof(rldq_pshufb[0]); i++)
					{
						for (u32 j = 0; j < 16; j++)
						{
							rldq_pshufb[i]._u8[j] = static_cast<u8>((j - i) & 0xf);
						}
					}
				}
			};

			extern const spu_imm_table_t g_spu_imm;

			enum FPSCR_EX
			{
				//Single-precision exceptions
				FPSCR_SOVF = 1 << 2,    //Overflow
				FPSCR_SUNF = 1 << 1,    //Underflow
				FPSCR_SDIFF = 1 << 0,   //Different (could be IEEE non-compliant)
				//Double-precision exceptions
				FPSCR_DOVF = 1 << 13,   //Overflow
				FPSCR_DUNF = 1 << 12,   //Underflow
				FPSCR_DINX = 1 << 11,   //Inexact
				FPSCR_DINV = 1 << 10,   //Invalid operation
				FPSCR_DNAN = 1 << 9,    //NaN
				FPSCR_DDENORM = 1 << 8, //Denormal
			};

			//Is 128 bits, but bits 0-19, 24-28, 32-49, 56-60, 64-81, 88-92, 96-115, 120-124 are unused
			struct fpscr_t
			{
				u32 _u32[4];

				std::string to_string() const
				{
					return fmt::format("%08x%08x%08x%08x", _u32[3], _u32[2], _u32[1], _u32[0]);
				}

				void reset()
				{
					*this = {};
				}
				//slice -> 0 - 1 (double-precision slice index)
				//NOTE: slices follow v128 indexing, i.e. slice 0 is RIGHT end of register!
				//roundTo -> FPSCR_RN_*
				void set_slice_rounding(u8 slice, u8 round_to)
				{
					int shift = 8 + 2 * slice;
					//rounding is located in the left end of the FPSCR
					_u32[3] = (_u32[3] & ~(3 << shift)) | (round_to << shift);
				}
				//Slice 0 or 1
				u8 check_slice_rounding(u8 slice) const
				{
					switch (slice)
					{
					case 0:
						return _u32[3] >> 8 & 0x3;

					case 1:
						return _u32[3] >> 10 & 0x3;

					default:
						throw EXCEPTION("Unexpected slice value (%d)", slice);
					}
				}

				//Single-precision exception flags (all 4 slices)
				//slice -> slice number (0-3)
				//exception: FPSCR_S* bitmask
				void set_single_precision_exception_flags(u8 slice, u32 exceptions)
				{
					_u32[slice] |= exceptions;
				}

				//Single-precision divide-by-zero flags (all 4 slices)
				//slice -> slice number (0-3)
				void set_divide_by_zero_flag(u8 slice)
				{
					_u32[0] |= 1 << (8 + slice);
				}

				//Double-precision exception flags
				//slice -> slice number (0-1)
				//exception: FPSCR_D* bitmask
				void set_double_precision_exception_flags(u8 slice, u32 exceptions)
				{
					_u32[1 + slice] |= exceptions;
				}

				// Write the FPSCR
				void write(const v128 & r)
				{
					_u32[3] = r._u32[3] & 0x00000F07;
					_u32[2] = r._u32[2] & 0x00003F07;
					_u32[1] = r._u32[1] & 0x00003F07;
					_u32[0] = r._u32[0] & 0x00000F07;
				}

				// Read the FPSCR
				void read(v128 & r)
				{
					r._u32[3] = _u32[3];
					r._u32[2] = _u32[2];
					r._u32[1] = _u32[1];
					r._u32[0] = _u32[0];
				}
			};

			class thread
			{
				friend class SPURecompilerDecoder;
				friend class spu_recompiler;

				std::mutex mutex;
				std::condition_variable cv;

			public:
				std::array<v128, 128> gpr; // General-Purpose Registers
				fpscr_t fpscr;

				std::unordered_map<u32, std::function<bool(thread& cpu)>> addr_to_hle_function_map;

				mfc::spu_mfc_arg_t ch_mfc_args;

				std::vector<std::pair<u32, mfc::spu_mfc_arg_t>> mfc_queue; // Only used for stalled list transfers

				u32 ch_tag_mask;
				spu_channel_t ch_tag_stat;
				spu_channel_t ch_stall_stat;
				spu_channel_t ch_atomic_stat;

				spu_channel_4_t ch_in_mbox;

				spu_channel_t ch_out_mbox;
				spu_channel_t ch_out_intr_mbox;

				u64 snr_config; // SPU SNR Config Register

				spu_channel_t ch_snr1; // SPU Signal Notification Register 1
				spu_channel_t ch_snr2; // SPU Signal Notification Register 2

				atomic<u32> ch_event_mask;
				atomic<u32> ch_event_stat;
				u32 last_raddr; // Last Reservation Address (0 if not set)

				u64 ch_dec_start_timestamp; // timestamp of writing decrementer value
				u32 ch_dec_value; // written decrementer value

				atomic<u32> run_ctrl; // SPU Run Control register (only provided to get latest data written)
				atomic<u32> status; // SPU Status register
				atomic<u32> npc; // SPU Next Program Counter register

				std::array<spu_int_ctrl_t, 3> int_ctrl; // SPU Class 0, 1, 2 Interrupt Management

				std::weak_ptr<lv2_spu_group_t> tg; // SPU Thread Group

				std::array<std::pair<u32, std::weak_ptr<lv2_event_queue_t>>, 32> spuq; // Event Queue Keys for SPU Thread
				std::weak_ptr<lv2_event_queue_t> spup[64]; // SPU Ports

			private:
				u32 m_pc = 0; // 
				const u32 m_index; // SPU index
				const u32 m_offset; // SPU LS offset

			public:
				void push_snr(u32 number, u32 value)
				{
					// get channel
					const auto channel =
						number == 0 ? &ch_snr1 :
						number == 1 ? &ch_snr2 : throw EXCEPTION("Unexpected");

					// check corresponding SNR register settings
					if ((snr_config >> number) & 1)
					{
						channel->push_or(value);
					}
					else
					{
						channel->push(value);
					}

					if (channel->notification_required)
					{
						// lock for reliable notification
						std::lock_guard<std::mutex> lock(mutex);

						cv.notify_one();
					}
				}

				void do_dma_transfer(u32 cmd, mfc::spu_mfc_arg_t args);
				void do_dma_list_cmd(u32 cmd, mfc::spu_mfc_arg_t args);
				void process_mfc_cmd(u32 cmd);

				u32 get_events(bool waiting = false);
				void set_events(u32 mask);
				void set_interrupt_status(bool enable);
				u32 get_ch_count(u32 ch);
				u32 get_ch_value(u32 ch);
				void set_ch_value(u32 ch, u32 value);

				void stop_and_signal(u32 code);
				void halt();

				// Convert specified SPU LS address to a pointer of specified (possibly converted to BE) type
				template<typename T> inline to_be_t<T>* _ptr(u32 lsa)
				{
					return static_cast<to_be_t<T>*>(vm::base(offset() + lsa));
				}

				// Convert specified SPU LS address to a reference of specified (possibly converted to BE) type
				template<typename T> inline to_be_t<T>& _ref(u32 lsa)
				{
					return *_ptr<T>(lsa);
				}

				void register_hle_function(u32 addr, std::function<bool(thread& cpu)> function)
				{
					addr_to_hle_function_map[addr] = function;
					_ref<u32>(addr) = 0x00000003; // STOP 3
				}

				void unregister_hle_function(u32 addr)
				{
					addr_to_hle_function_map.erase(addr);
				}

				void unregister_hle_functions(u32 start_addr, u32 end_addr)
				{
					for (auto iter = addr_to_hle_function_map.begin(); iter != addr_to_hle_function_map.end();)
					{
						if (iter->first >= start_addr && iter->first <= end_addr)
						{
							iter = addr_to_hle_function_map.erase(iter++);
						}
						else
						{
							iter++;
						}
					}
				}

				std::function<void(thread&)> custom_task;
				std::exception_ptr pending_exception;
				u32 recursion_level = 0;

			protected:
				thread(const std::string& name, std::function<std::string()> thread_name, u32 index, u32 offset);

			public:
				thread(u32 index, u32 entry, const std::string& name = "", u32 stack_size = 0, u32 prio = 0);
				virtual ~thread();

				bool paused() const;

				void dump_info() const;
				u32 pc() const { return m_pc; }
				u32 offset() const { return m_offset; }
				void do_run();
				virtual void task();

				void init_regs();
				void init_stack();
				void close_stack();

				void fast_call(u32 ls_addr);

				std::string regs_to_string() const
				{
					std::string ret = "Registers:\n=========\n";

					for (uint i = 0; i < 128; ++i) ret += fmt::format("GPR[%d] = 0x%s\n", i, gpr[i].to_hex().c_str());

					return ret;
				}

				std::string read_reg_string(const std::string& reg) const
				{
					std::string::size_type first_brk = reg.find('[');

					if (first_brk != std::string::npos)
					{
						long reg_index;
						reg_index = atol(reg.substr(first_brk + 1, reg.length() - 2).c_str());
						if (reg.find("GPR") == 0) return fmt::format("%016llx%016llx", gpr[reg_index]._u64[1], gpr[reg_index]._u64[0]);
					}

					return "";
				}

				bool write_reg_string(const std::string& reg, std::string value)
				{
					while (value.length() < 32) value = "0" + value;
					std::string::size_type first_brk = reg.find('[');
					if (first_brk != std::string::npos)
					{
						long reg_index;
						reg_index = atol(reg.substr(first_brk + 1, reg.length() - 2).c_str());
						if (reg.find("GPR") == 0)
						{
							unsigned long long reg_value0;
							unsigned long long reg_value1;
							try
							{
								reg_value0 = std::stoull(value.substr(16, 31), 0, 16);
								reg_value1 = std::stoull(value.substr(0, 15), 0, 16);
							}
							catch (std::invalid_argument& /*e*/)
							{
								return false;
							}
							gpr[reg_index]._u64[0] = (u64)reg_value0;
							gpr[reg_index]._u64[1] = (u64)reg_value1;
							return true;
						}
					}
					return false;
				}
			};
		}
	}
}