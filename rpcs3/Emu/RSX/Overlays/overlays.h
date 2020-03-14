#pragma once
#include "overlay_animation.h"
#include "overlay_controls.h"

#include "../../../Utilities/Thread.h"
#include "Emu/Memory/vm.h"
#include "Emu/IdManager.h"

#include "Utilities/Timer.h"

#include <list>
#include <thread>

// Utils
extern u64 get_system_time();

// Definition of user interface implementations
namespace rsx
{
	namespace overlays
	{
		// Non-interactable UI element
		struct overlay
		{
			u32 uid = UINT32_MAX;
			u32 type_index = UINT32_MAX;

			u16 virtual_width = 1280;
			u16 virtual_height = 720;

			u32 min_refresh_duration_us = 16600;
			atomic_t<bool> visible = false;

			virtual ~overlay() = default;

			virtual void update() {}
			virtual compiled_resource get_compiled() = 0;

			void refresh();
		};

		// Interactable UI element
		class user_interface : public overlay
		{
		public:
			// Move this somewhere to avoid duplication
			enum selection_code
			{
				new_save = -1,
				canceled = -2,
				error = -3
			};

			enum pad_button : u8
			{
				dpad_up = 0,
				dpad_down,
				dpad_left,
				dpad_right,
				select,
				start,
				triangle,
				circle,
				square,
				cross,
				L1,
				R1,
				L2,
				R2,

				pad_button_max_enum
			};

		protected:
			Timer input_timer;
			atomic_t<bool> exit = false;
			atomic_t<u64> thread_bits = 0;

			static thread_local u64 g_thread_bit;

			u64 alloc_thread_bit();

			std::function<void(s32 status)> on_close;

		public:
			s32 return_code = 0; // CELL_OK

		public:
			void update() override {}

			compiled_resource get_compiled() override = 0;

			virtual void on_button_pressed(pad_button /*button_press*/) {}

			void close(bool use_callback, bool stop_pad_interception);

			s32 run_input_loop();
		};

		class display_manager
		{
		private:
			atomic_t<u32> m_uid_ctr = 0;
			std::vector<std::shared_ptr<overlay>> m_iface_list;
			std::vector<std::shared_ptr<overlay>> m_dirty_list;

			shared_mutex m_list_mutex;
			std::vector<u32> m_uids_to_remove;
			std::vector<u32> m_type_ids_to_remove;

			bool remove_type(u32 type_id)
			{
				bool found = false;
				for (auto It = m_iface_list.begin(); It != m_iface_list.end();)
				{
					if (It->get()->type_index == type_id)
					{
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

			bool remove_uid(u32 uid)
			{
				for (auto It = m_iface_list.begin(); It != m_iface_list.end(); It++)
				{
					const auto e = It->get();
					if (e->uid == uid)
					{
						m_dirty_list.push_back(std::move(*It));
						m_iface_list.erase(It);
						return true;
					}
				}

				return false;
			}

			void cleanup_internal()
			{
				for (const auto &uid : m_uids_to_remove)
				{
					remove_uid(uid);
				}

				for (const auto &type_id : m_type_ids_to_remove)
				{
					remove_type(type_id);
				}

				m_uids_to_remove.clear();
				m_type_ids_to_remove.clear();
			}

		public:
			// Disable default construction to make it conditionally available in g_fxo
			explicit display_manager(int) noexcept
			{
			}

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
			void remove(u32 uid)
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
			void dispose(const std::vector<u32>& uids)
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
					m_dirty_list.end()
				);
			}

			// Returns pointer to the object matching the given uid
			std::shared_ptr<overlay> get(u32 uid)
			{
				reader_lock lock(m_list_mutex);

				for (const auto& iface : m_iface_list)
				{
					if (iface->uid == uid)
						return iface;
				}

				return {};
			}

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
			void lock()
			{
				m_list_mutex.lock_shared();
			}

			// Release read-only lock (BasicLockable). May perform internal cleanup before returning
			void unlock()
			{
				m_list_mutex.unlock_shared();

				if (!m_uids_to_remove.empty() || !m_type_ids_to_remove.empty())
				{
					std::lock_guard lock(m_list_mutex);
					cleanup_internal();
				}
			}
		};
	}
}
