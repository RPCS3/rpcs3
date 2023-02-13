#pragma once

#include "overlays.h"

#include "Emu/IdManager.h"
#include "Utilities/mutex.h"
#include "Utilities/Timer.h"

#include <deque>
#include <set>

namespace rsx
{
	namespace overlays
	{
		struct overlay;

		class display_manager
		{
		private:
			atomic_t<u32> m_uid_ctr = 0;
			std::vector<std::shared_ptr<overlay>> m_iface_list;
			std::vector<std::shared_ptr<overlay>> m_dirty_list;

			shared_mutex m_list_mutex;
			std::vector<u32> m_uids_to_remove;
			std::vector<u32> m_type_ids_to_remove;

			bool remove_type(u32 type_id);

			bool remove_uid(u32 uid);

			void cleanup_internal();

			void on_overlay_activated(const std::shared_ptr<overlay>& item);

			void on_overlay_removed(const std::shared_ptr<overlay>& item);

		public:
			// Disable default construction to make it conditionally available in g_fxo
			explicit display_manager(int) noexcept
			{}

			// Adds an object to the internal list. Optionally removes other objects of the same type.
			// Original handle loses ownership but a usable pointer is returned
			template <typename T>
			std::shared_ptr<T> add(std::shared_ptr<T>& entry, bool remove_existing = true)
			{
				std::lock_guard lock(m_list_mutex);

				entry->uid = m_uid_ctr.fetch_add(1);
				entry->type_index = id_manager::typeinfo::get_index<T>();

				if (remove_existing)
				{
					for (auto It = m_iface_list.begin(); It != m_iface_list.end(); It++)
					{
						if (It->get()->type_index == entry->type_index)
						{
							// Replace
							m_dirty_list.push_back(std::move(*It));
							*It = std::move(entry);
							return std::static_pointer_cast<T>(*It);
						}
					}
				}

				m_iface_list.push_back(std::move(entry));
				on_overlay_activated(m_iface_list.back());
				return std::static_pointer_cast<T>(m_iface_list.back());
			}

			// Allocates object and adds to internal list. Returns pointer to created object
			template <typename T, typename ...Args>
			std::shared_ptr<T> create(Args&&... args)
			{
				auto object = std::make_shared<T>(std::forward<Args>(args)...);
				return add(object);
			}

			// Removes item from list if it matches the uid
			void remove(u32 uid);

			// Removes all objects of this type from the list
			template <typename T>
			void remove()
			{
				const auto type_id = id_manager::typeinfo::get_index<T>();
				if (m_list_mutex.try_lock())
				{
					remove_type(type_id);
					m_list_mutex.unlock();
				}
				else
				{
					m_type_ids_to_remove.push_back(type_id);
				}
			}

			// True if any visible elements to draw exist
			bool has_visible() const
			{
				return !m_iface_list.empty();
			}

			// True if any elements have been deleted but their resources may not have been cleaned up
			bool has_dirty() const
			{
				return !m_dirty_list.empty();
			}

			// Returns current list for reading. Caller must ensure synchronization by first locking the list
			const std::vector<std::shared_ptr<overlay>>& get_views() const
			{
				return m_iface_list;
			}

			// Returns current list of removed objects not yet deallocated for reading.
			// Caller must ensure synchronization by first locking the list
			const std::vector<std::shared_ptr<overlay>>& get_dirty() const
			{
				return m_dirty_list;
			}

			// Deallocate object. Object must first be removed via the remove() functions
			void dispose(const std::vector<u32>& uids);

			// Returns pointer to the object matching the given uid
			std::shared_ptr<overlay> get(u32 uid);

			// Returns pointer to the first object matching the given type
			template <typename T>
			std::shared_ptr<T> get()
			{
				reader_lock lock(m_list_mutex);

				const auto type_id = id_manager::typeinfo::get_index<T>();
				for (const auto& iface : m_iface_list)
				{
					if (iface->type_index == type_id)
					{
						return std::static_pointer_cast<T>(iface);
					}
				}

				return {};
			}

			// Lock for read-only access (BasicLockable)
			void lock();

			// Release read-only lock (BasicLockable). May perform internal cleanup before returning
			void unlock();

		private:
			struct overlay_input_thread
			{
				static constexpr auto thread_name = "Overlay Input Thread"sv;
			};

			struct input_thread_access_token
			{
				std::shared_ptr<user_interface> target;
			};

			std::deque<input_thread_access_token> m_input_token_stack;
			shared_mutex m_input_thread_lock;
		};
	}
}
