#pragma once

#include "overlays.h"

#include "Emu/IdManager.h"
#include "Utilities/mutex.h"
#include "Utilities/Thread.h"
#include "Utilities/lockless.h"

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
			lf_queue<u32> m_uids_to_remove;
			lf_queue<u32> m_type_ids_to_remove;
			atomic_t<u32> m_pending_removals_count = 0;

			bool remove_type(u32 type_id);

			bool remove_uid(u32 uid);

			void cleanup_internal();

			void on_overlay_activated(const std::shared_ptr<overlay>& item);

			void on_overlay_removed(const std::shared_ptr<overlay>& item);

		public:
			// Disable default construction to make it conditionally available in g_fxo
			explicit display_manager(int) noexcept;

			~display_manager();

			// Adds an object to the internal list. Optionally removes other objects of the same type.
			// Original handle loses ownership but a usable pointer is returned
			template <typename T>
			std::shared_ptr<T> add(std::shared_ptr<T>& entry, bool remove_existing = true)
			{
				std::lock_guard lock(m_list_mutex);

				entry->uid = m_uid_ctr.fetch_add(1);
				entry->type_index = stx::typeindex<id_manager::typeinfo, T>();

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
				const auto type_id = stx::typeindex<id_manager::typeinfo, T>();
				if (m_list_mutex.try_lock())
				{
					remove_type(type_id);
					m_list_mutex.unlock();
					return;
				}

				// Enqueue
				m_type_ids_to_remove.push(type_id);
				m_pending_removals_count++;
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

				const auto type_id = stx::typeindex<id_manager::typeinfo, T>();
				for (const auto& iface : m_iface_list)
				{
					if (iface->type_index == type_id)
					{
						return std::static_pointer_cast<T>(iface);
					}
				}

				return {};
			}

			// Lock for exclusive access (BasicLockable)
			void lock();

			// Release lock (BasicLockable). May perform internal cleanup before returning
			void unlock();

			// Lock for shared access (reader-lock)
			void lock_shared();

			// Unlock for shared access (reader-lock)
			void unlock_shared();

			// Enable input thread attach to the specified interface
			void attach_thread_input(
				u32 uid,                                                 // The input target
				const std::string_view& name,                            // The name of the target
				std::function<void()> on_input_loop_enter = nullptr,     // [optional] What to do before running the input routine
				std::function<void(s32)> on_input_loop_exit = nullptr,   // [optional] What to do with the result if any
				std::function<s32()> input_loop_override = nullptr);     // [optional] What to do during the input loop. By default calls user_interface::run_input_loop

		private:
			struct overlay_input_thread
			{
				static constexpr auto thread_name = "Overlay Input Thread"sv;
			};

			struct input_thread_context_t
			{
				// Ctor
				input_thread_context_t(
					const std::string_view& name,
					std::shared_ptr<user_interface> iface,
					std::function<void()> on_input_loop_enter,
					std::function<void(s32)> on_input_loop_exit,
					std::function<s32()> input_loop_override)
					: name(name)
					, target(iface)
					, input_loop_prologue(on_input_loop_enter)
					, input_loop_epilogue(on_input_loop_exit)
					, input_loop_override(input_loop_override)
					, prologue_completed(false)
				{}

				// Attributes
				std::string_view name;
				std::shared_ptr<user_interface> target;
				std::function<void()> input_loop_prologue;
				std::function<void(s32)> input_loop_epilogue;
				std::function<s32()> input_loop_override;

				// Runtime stats
				bool prologue_completed;
			};

			lf_queue<input_thread_context_t> m_input_token_stack;
			atomic_t<bool> m_input_thread_abort = false;
			atomic_t<bool> m_input_thread_interrupted = false;
			shared_mutex m_input_stack_guard;

			std::shared_ptr<named_thread<overlay_input_thread>> m_input_thread;
			void input_thread_loop();
		};
	}
}
