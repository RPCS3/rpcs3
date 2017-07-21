#include "stdafx.h"
#include "Utilities/sysinfo.h"
#include "Emu/Memory/vm.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/lv2/sys_sync.h"
#include "MFC.h"

const bool s_use_rtm = utils::has_rtm();

template <>
void fmt_class_string<MFC>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](MFC cmd)
	{
		switch (cmd)
		{
		case MFC_PUT_CMD: return "PUT";
		case MFC_PUTB_CMD: return "PUTB";
		case MFC_PUTF_CMD: return "PUTF";
		case MFC_PUTS_CMD: return "PUTS";
		case MFC_PUTBS_CMD: return "PUTBS";
		case MFC_PUTFS_CMD: return "PUTFS";
		case MFC_PUTR_CMD: return "PUTR";
		case MFC_PUTRB_CMD: return "PUTRB";
		case MFC_PUTRF_CMD: return "PUTRF";
		case MFC_GET_CMD: return "GET";
		case MFC_GETB_CMD: return "GETB";
		case MFC_GETF_CMD: return "GETF";
		case MFC_GETS_CMD: return "GETS";
		case MFC_GETBS_CMD: return "GETBS";
		case MFC_GETFS_CMD: return "GETFS";
		case MFC_PUTL_CMD: return "PUTL";
		case MFC_PUTLB_CMD: return "PUTLB";
		case MFC_PUTLF_CMD: return "PUTLF";
		case MFC_PUTRL_CMD: return "PUTRL";
		case MFC_PUTRLB_CMD: return "PUTRLB";
		case MFC_PUTRLF_CMD: return "PUTRLF";
		case MFC_GETL_CMD: return "GETL";
		case MFC_GETLB_CMD: return "GETLB";
		case MFC_GETLF_CMD: return "GETLF";

		case MFC_GETLLAR_CMD: return "GETLLAR";
		case MFC_PUTLLC_CMD: return "PUTLLC";
		case MFC_PUTLLUC_CMD: return "PUTLLUC";
		case MFC_PUTQLLUC_CMD: return "PUTQLLUC";

		case MFC_SNDSIG_CMD: return "SNDSIG";
		case MFC_SNDSIGB_CMD: return "SNDSIGB";
		case MFC_SNDSIGF_CMD: return "SNDSIGF";
		case MFC_BARRIER_CMD: return "BARRIER";
		case MFC_EIEIO_CMD: return "EIEIO";
		case MFC_SYNC_CMD: return "SYNC";
		}

		return unknown;
	});
}

mfc_thread::mfc_thread()
	: cpu_thread(0)
{
}

mfc_thread::~mfc_thread()
{
}

std::string mfc_thread::get_name() const
{
	return "MFC Thread";
}

void mfc_thread::cpu_task()
{
	vm::passive_lock(*this);

	u32 no_updates = 0;

	while (!m_spus.empty() || m_spuq.size() != 0)
	{
		// Add or remove destroyed SPU threads
		while (m_spuq.size())
		{
			auto& thread_ptr = m_spuq[0];

			// Look for deleted threads if nullptr received
			for (auto it = m_spus.cbegin(); !thread_ptr && it != m_spus.cend();)
			{
				if (test(it->get()->state, cpu_flag::exit))
				{
					it = m_spus.erase(it);
				}
				else
				{
					it++;
				}
			}

			// Add thread
			if (thread_ptr)
			{
				m_spus.emplace_back(std::move(thread_ptr));
			}

			m_spuq.end_pop();
			no_updates = 0;
		}

		test_state();

		// Process SPU threads
		for (const auto& thread_ptr : m_spus)
		{
			SPUThread& spu = *thread_ptr;

			const auto proxy_size = spu.mfc_proxy.size();
			const auto queue_size = spu.mfc_queue.size();

			if (proxy_size)
			{
				const auto& cmd = spu.mfc_proxy[0];

				spu.do_dma_transfer(cmd);

				if (cmd.cmd & MFC_START_MASK && !spu.status.test_and_set(SPU_STATUS_RUNNING))
				{
					spu.run();
				}

				spu.mfc_proxy.end_pop();
				no_updates = 0;
			}

			test_state();

			if (queue_size)
			{
				auto& cmd = spu.mfc_queue[0];

				if ((cmd.cmd & ~(MFC_BARRIER_MASK | MFC_FENCE_MASK)) == MFC_PUTQLLUC_CMD)
				{
					auto& data = vm::ps3::_ref<decltype(spu.rdata)>(cmd.eal);
					const auto to_write = spu._ref<decltype(spu.rdata)>(cmd.lsa & 0x3ffff);

					cmd.size = 0;
					no_updates = 0;

					vm::reservation_acquire(cmd.eal, 128);

					// Store unconditionally
					if (s_use_rtm && utils::transaction_enter())
					{
						if (!vm::reader_lock{vm::try_to_lock})
						{
							_xabort(0);
						}

						data = to_write;
						vm::reservation_update(cmd.eal, 128);
						vm::notify(cmd.eal, 128);
						_xend();
					}
					else
					{
						vm::writer_lock lock(0);
						data = to_write;
						vm::reservation_update(cmd.eal, 128);
						vm::notify(cmd.eal, 128);
					}
				}
				else if (cmd.cmd & MFC_LIST_MASK)
				{
					struct list_element
					{
						be_t<u16> sb; // Stall-and-Notify bit (0x8000)
						be_t<u16> ts; // List Transfer Size
						be_t<u32> ea; // External Address Low
					};

					if (cmd.size && (spu.ch_stall_mask & (1u << cmd.tag)) == 0)
					{
						cmd.lsa &= 0x3fff0;

						const list_element item = spu._ref<list_element>(cmd.eal & 0x3fff8);

						const u32 size = item.ts;
						const u32 addr = item.ea;

						if (size)
						{
							spu_mfc_cmd transfer;
							transfer.eal = addr;
							transfer.eah = 0;
							transfer.lsa = cmd.lsa | (addr & 0xf);
							transfer.tag = cmd.tag;
							transfer.cmd = MFC(cmd.cmd & ~MFC_LIST_MASK);
							transfer.size = size;

							spu.do_dma_transfer(transfer);
							cmd.lsa += std::max<u32>(size, 16);
						}

						cmd.eal += 8;
						cmd.size -= 8;
						no_updates = 0;

						if (item.sb & 0x8000)
						{
							spu.ch_stall_stat.push_or(spu, 1 << cmd.tag);

							const u32 evt = spu.ch_event_stat.fetch_or(SPU_EVENT_SN);

							if (evt & SPU_EVENT_WAITING)
							{
								spu.notify();
							}
							else if (evt & SPU_EVENT_INTR_ENABLED)
							{
								spu.state += cpu_flag::suspend;
							}
						}
					}
				}
				else if (LIKELY(cmd.size))
				{
					spu.do_dma_transfer(cmd);
					cmd.size = 0;
				}
				else if (UNLIKELY((cmd.cmd & ~0xc) == MFC_BARRIER_CMD))
				{
					// TODO (MFC_BARRIER_CMD, MFC_EIEIO_CMD, MFC_SYNC_CMD)
					_mm_mfence();
				}

				if (!cmd.size)
				{
					spu.mfc_queue.end_pop();
					no_updates = 0;
				}
			}

			test_state();

			if (spu.ch_tag_upd)
			{
				// Mask incomplete transfers
				u32 completed = spu.ch_tag_mask;

				for (u32 i = 0; i < spu.mfc_queue.size(); i++)
				{
					const auto& _cmd = spu.mfc_queue[i];

					if (_cmd.size)
					{
						if (spu.ch_tag_upd == 1)
						{
							completed &= ~(1u << _cmd.tag);
						}
						else
						{
							completed = 0;
							break;
						}
					}
				}

				if (completed && spu.ch_tag_upd.exchange(0))
				{
					spu.ch_tag_stat.push(spu, completed);
					no_updates = 0;
				}
			}

			test_state();
		}

		if (no_updates++)
		{
			if (no_updates >= 3)
			{
				if (m_spuq.size())
				{
					no_updates = 0;
				}

				for (const auto& thread_ptr : m_spus)
				{
					SPUThread& spu = *thread_ptr;

					if (spu.mfc_proxy.size())
					{
						no_updates = 0;
						break;
					}

					if (spu.mfc_queue.size())
					{
						auto& cmd = spu.mfc_queue[0];

						if ((cmd.cmd & MFC_LIST_MASK) == 0 || (spu.ch_stall_mask & (1u << cmd.tag)) == 0)
						{
							no_updates = 0;
							break;
						}
					}

					if (spu.ch_tag_upd)
					{
						no_updates = 0;
						break;
					}
				}

				if (no_updates)
				{
					vm::temporary_unlock(*this);
					thread_ctrl::wait_for(100);
				}
			}
			else
			{
				vm::reader_lock lock;
				vm::notify_all();
			}
		}
	}

	vm::passive_unlock(*this);
	state += cpu_flag::stop;
}

void mfc_thread::add_spu(spu_ptr _spu)
{
	while (!m_spuq.try_push(std::move(_spu)))
	{
		busy_wait();
		continue;
	}

	run();
}
