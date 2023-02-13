#include "stdafx.h"
#include "overlay_manager.h"
#include "Emu/System.h"

namespace rsx
{
	namespace overlays
	{
		display_manager::~display_manager()
		{
			if (m_input_thread)
			{
				m_input_thread_abort.store(true);

				*m_input_thread = thread_state::aborting;
				while (*m_input_thread <= thread_state::aborting)
				{
					_mm_pause();
				}
			}
		}

		void display_manager::lock()
		{
			m_list_mutex.lock_shared();
		}

		void display_manager::unlock()
		{
			m_list_mutex.unlock_shared();

			if (!m_uids_to_remove.empty() || !m_type_ids_to_remove.empty())
			{
				std::lock_guard lock(m_list_mutex);
				cleanup_internal();
			}
		}

		std::shared_ptr<overlay> display_manager::get(u32 uid)
		{
			reader_lock lock(m_list_mutex);

			for (const auto& iface : m_iface_list)
			{
				if (iface->uid == uid)
					return iface;
			}

			return {};
		}

		void display_manager::remove(u32 uid)
		{
			if (m_list_mutex.try_lock())
			{
				remove_uid(uid);
				m_list_mutex.unlock();
			}
			else
			{
				m_uids_to_remove.push_back(uid);
			}
		}

		void display_manager::dispose(const std::vector<u32>& uids)
		{
			std::lock_guard lock(m_list_mutex);

			if (!m_uids_to_remove.empty() || !m_type_ids_to_remove.empty())
			{
				cleanup_internal();
			}

			m_dirty_list.erase
			(
				std::remove_if(m_dirty_list.begin(), m_dirty_list.end(), [&uids](std::shared_ptr<overlay>& e)
				{
					return std::find(uids.begin(), uids.end(), e->uid) != uids.end();
				}),
				m_dirty_list.end());
		}

		bool display_manager::remove_type(u32 type_id)
		{
			bool found = false;
			for (auto It = m_iface_list.begin(); It != m_iface_list.end();)
			{
				if (It->get()->type_index == type_id)
				{
					on_overlay_removed(*It);
					m_dirty_list.push_back(std::move(*It));
					It = m_iface_list.erase(It);
					found = true;
				}
				else
				{
					++It;
				}
			}

			return found;
		}

		bool display_manager::remove_uid(u32 uid)
		{
			for (auto It = m_iface_list.begin(); It != m_iface_list.end(); It++)
			{
				const auto e = It->get();
				if (e->uid == uid)
				{
					on_overlay_removed(*It);
					m_dirty_list.push_back(std::move(*It));
					m_iface_list.erase(It);
					return true;
				}
			}

			return false;
		}

		void display_manager::cleanup_internal()
		{
			for (const auto& uid : m_uids_to_remove)
			{
				remove_uid(uid);
			}

			for (const auto& type_id : m_type_ids_to_remove)
			{
				remove_type(type_id);
			}

			m_uids_to_remove.clear();
			m_type_ids_to_remove.clear();
		}

		void display_manager::on_overlay_activated(const std::shared_ptr<overlay>& item)
		{
			if (auto iface = std::dynamic_pointer_cast<user_interface>(item))
			{
				// Kick input thread if not enabled. Expect the interface to attach shortly
				std::lock_guard lock(m_input_thread_lock);

				if (!m_input_thread)
				{
					m_input_thread = std::make_shared<named_thread<overlay_input_thread>>();
					(*m_input_thread)([this]()
					{
						input_thread_loop();
					});
				}
			}
		}

		void display_manager::attach_thread_input(
			u32 uid,
			std::function<void()> on_input_loop_enter,
			std::function<void(s32)> on_input_loop_exit,
			std::function<s32()> input_loop_override)
		{
			if (auto iface = std::dynamic_pointer_cast<user_interface>(get(uid)))
			{
				std::lock_guard lock(m_input_thread_lock);
				m_input_token_stack.emplace_front(
					std::move(iface),
					on_input_loop_enter,
					on_input_loop_exit,
					input_loop_override);
			}
		}

		void display_manager::on_overlay_removed(const std::shared_ptr<overlay>& item)
		{
			if (!dynamic_cast<user_interface*>(item.get()))
			{
				// Not instance of UI, ignore
				return;
			}

			std::lock_guard lock(m_input_thread_lock);
			for (auto& entry : m_input_token_stack)
			{
				if (entry.target->uid == item->uid)
				{
					// Release
					entry.target = {};
					break;
				}
			}

			// The top must never be an empty ref. Pop all empties.
			while (!m_input_token_stack.front().target && m_input_token_stack.size())
			{
				m_input_token_stack.pop_front();
			}
		}

		void display_manager::input_thread_loop()
		{
			while (!m_input_thread_abort)
			{
				input_thread_context_t input_context;
				{
					reader_lock lock(m_input_thread_lock);
					if (!m_input_token_stack.empty())
					{
						input_context = m_input_token_stack.front();
						m_input_token_stack.pop_front();
					}
				}

				if (input_context.target)
				{
					if (input_context.input_loop_prologue)
					{
						input_context.input_loop_prologue();
					}

					s32 result = 0;
					if (!input_context.input_loop_override) [[ likely ]]
					{
						result = input_context.target->run_input_loop();
					}
					else
					{
						result = input_context.input_loop_override();
					}

					if (input_context.input_loop_epilogue)
					{
						input_context.input_loop_epilogue(result);
					}
					else if (result && result != user_interface::selection_code::canceled)
					{
						rsx_log.error("Input loop exited with error code=%d", result);
					}
				}
				else
				{
					thread_ctrl::wait_for(1000);
				}
			}
		}
	}
}
