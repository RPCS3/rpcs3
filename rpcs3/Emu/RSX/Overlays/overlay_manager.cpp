#include "stdafx.h"
#include "overlay_manager.h"

namespace rsx
{
	namespace overlays
	{
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
				std::lock_guard lock(m_input_thread_lock);
				m_input_token_stack.emplace_front(std::move(iface));
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
	}
}
