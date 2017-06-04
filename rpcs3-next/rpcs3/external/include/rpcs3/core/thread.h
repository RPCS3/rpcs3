#pragma once
#include <common/basic_types.h>
#include <common/atomic.h>
#include <mutex>
#include <vector>
#include <thread>
#include <functional>
#include <string>

namespace rpcs3
{
	using namespace common;

	inline namespace core
	{
		const class thread_ctrl_t* get_current_thread_ctrl();

		// Named thread control class
		class thread_ctrl_t final
		{
			friend class named_thread_t;

			template<typename T> friend void current_thread_register_atexit(T);

			// Thread handler
			std::thread m_thread;

			// Name getter
			const std::function<std::string()> m_name;

			// Functions executed at thread exit (temporarily)
			std::vector<std::function<void()>> m_atexit;

		public:
			thread_ctrl_t(std::function<std::string()> name)
				: m_name(std::move(name))
			{
			}

			thread_ctrl_t(const thread_ctrl_t&) = delete;

			// Get thread name
			std::string get_name() const;
		};

		// Register function at thread exit (temporarily)
		template<typename T> void current_thread_register_atexit(T func)
		{
			extern thread_local thread_ctrl_t* g_tls_this_thread;

			g_tls_this_thread->m_atexit.emplace_back(func);
		}

		class named_thread_t
		{
			// Pointer to managed resource (shared with actual thread)
			std::shared_ptr<thread_ctrl_t> m_thread;

		public:
			// Thread mutex for external use
			std::mutex mutex;

			// Thread condition variable for external use
			std::condition_variable cv;

		public:
			// Initialize in empty state
			named_thread_t() = default;

			// Create named thread
			named_thread_t(std::function<std::string()> name, std::function<void()> func);

			// Deleted copy/move constructors + copy/move operators
			named_thread_t(const named_thread_t&) = delete;

			// Destructor, calls std::terminate if the thread is neither joined nor detached
			virtual ~named_thread_t();

		public:
			// Get thread name
			std::string get_name() const;

			// Create named thread (current state must be empty)
			void start(std::function<std::string()> name, std::function<void()> func);

			// Detach thread -> empty state
			void detach();

			// Join thread -> empty state
			void join();

			// Check whether the thread is not in "empty state"
			bool joinable() const { return m_thread.operator bool(); }

			// Check whether it is the currently running thread
			bool is_current() const;

			// Get internal thread pointer
			const thread_ctrl_t* get_thread_ctrl() const { return m_thread.get(); }
		};

		// Wrapper for named_thread_t, joins automatically in the destructor
		class autojoin_thread_t final
		{
			named_thread_t m_thread;

		public:
			autojoin_thread_t(std::function<std::string()> name, std::function<void()> func)
				: m_thread(std::move(name), std::move(func))
			{
			}

			autojoin_thread_t(const autojoin_thread_t&) = delete;

			~autojoin_thread_t() noexcept(false) // Allow exceptions
			{
				m_thread.join();
			}
		};

		extern const std::function<bool()> SQUEUE_ALWAYS_EXIT;
		extern const std::function<bool()> SQUEUE_NEVER_EXIT;

		bool squeue_test_exit();

		template<typename T, u32 sq_size = 256>
		class squeue_t
		{
			struct squeue_sync_var_t
			{
				struct
				{
					u32 position : 31;
					u32 pop_lock : 1;
				};
				struct
				{
					u32 count : 31;
					u32 push_lock : 1;
				};
			};

			atomic<squeue_sync_var_t> m_sync;

			mutable std::mutex m_rcv_mutex;
			mutable std::mutex m_wcv_mutex;
			mutable std::condition_variable m_rcv;
			mutable std::condition_variable m_wcv;

			T m_data[sq_size];

			enum squeue_sync_var_result : u32
			{
				SQSVR_OK = 0,
				SQSVR_LOCKED = 1,
				SQSVR_FAILED = 2,
			};

		public:
			squeue_t()
				: m_sync(squeue_sync_var_t{})
			{
			}

			u32 get_max_size() const
			{
				return sq_size;
			}

			bool is_full() const
			{
				return m_sync.load().count == sq_size;
			}

			bool push(const T& data, const std::function<bool()>& test_exit)
			{
				u32 pos = 0;

				while (u32 res = m_sync.atomic_op([&pos](squeue_sync_var_t& sync) -> u32
				{
					assert(sync.count <= sq_size);
					assert(sync.position < sq_size);

					if (sync.push_lock)
					{
						return SQSVR_LOCKED;
					}
					if (sync.count == sq_size)
					{
						return SQSVR_FAILED;
					}

					sync.push_lock = 1;
					pos = sync.position + sync.count;
					return SQSVR_OK;
				}))
				{
					if (res == SQSVR_FAILED && (test_exit() || squeue_test_exit()))
					{
						return false;
					}

					std::unique_lock<std::mutex> wcv_lock(m_wcv_mutex);
					m_wcv.wait_for(wcv_lock, std::chrono::milliseconds(1));
				}

				m_data[pos >= sq_size ? pos - sq_size : pos] = data;

				m_sync.atomic_op([](squeue_sync_var_t& sync)
				{
					assert(sync.count <= sq_size);
					assert(sync.position < sq_size);
					assert(sync.push_lock);
					sync.push_lock = 0;
					sync.count++;
				});

				m_rcv.notify_one();
				m_wcv.notify_one();
				return true;
			}

			bool push(const T& data, const volatile bool* do_exit)
			{
				return push(data, [do_exit]() { return do_exit && *do_exit; });
			}

			force_inline bool push(const T& data)
			{
				return push(data, SQUEUE_NEVER_EXIT);
			}

			force_inline bool try_push(const T& data)
			{
				return push(data, SQUEUE_ALWAYS_EXIT);
			}

			bool pop(T& data, const std::function<bool()>& test_exit)
			{
				u32 pos = 0;

				while (u32 res = m_sync.atomic_op([&pos](squeue_sync_var_t& sync) -> u32
				{
					assert(sync.count <= sq_size);
					assert(sync.position < sq_size);

					if (!sync.count)
					{
						return SQSVR_FAILED;
					}
					if (sync.pop_lock)
					{
						return SQSVR_LOCKED;
					}

					sync.pop_lock = 1;
					pos = sync.position;
					return SQSVR_OK;
				}))
				{
					if (res == SQSVR_FAILED && (test_exit() || squeue_test_exit()))
					{
						return false;
					}

					std::unique_lock<std::mutex> rcv_lock(m_rcv_mutex);
					m_rcv.wait_for(rcv_lock, std::chrono::milliseconds(1));
				}

				data = m_data[pos];

				m_sync.atomic_op([](squeue_sync_var_t& sync)
				{
					assert(sync.count <= sq_size);
					assert(sync.position < sq_size);
					assert(sync.pop_lock);
					sync.pop_lock = 0;
					sync.position++;
					sync.count--;
					if (sync.position == sq_size)
					{
						sync.position = 0;
					}
				});

				m_rcv.notify_one();
				m_wcv.notify_one();
				return true;
			}

			bool pop(T& data, const volatile bool* do_exit)
			{
				return pop(data, [do_exit]() { return do_exit && *do_exit; });
			}

			force_inline bool pop(T& data)
			{
				return pop(data, SQUEUE_NEVER_EXIT);
			}

			force_inline bool try_pop(T& data)
			{
				return pop(data, SQUEUE_ALWAYS_EXIT);
			}

			bool peek(T& data, u32 start_pos, const std::function<bool()>& test_exit)
			{
				assert(start_pos < sq_size);
				u32 pos = 0;

				while (u32 res = m_sync.atomic_op([&pos, start_pos](squeue_sync_var_t& sync) -> u32
				{
					assert(sync.count <= sq_size);
					assert(sync.position < sq_size);

					if (sync.count <= start_pos)
					{
						return SQSVR_FAILED;
					}
					if (sync.pop_lock)
					{
						return SQSVR_LOCKED;
					}

					sync.pop_lock = 1;
					pos = sync.position + start_pos;
					return SQSVR_OK;
				}))
				{
					if (res == SQSVR_FAILED && (test_exit() || squeue_test_exit()))
					{
						return false;
					}

					std::unique_lock<std::mutex> rcv_lock(m_rcv_mutex);
					m_rcv.wait_for(rcv_lock, std::chrono::milliseconds(1));
				}

				data = m_data[pos >= sq_size ? pos - sq_size : pos];

				m_sync.atomic_op([](squeue_sync_var_t& sync)
				{
					assert(sync.count <= sq_size);
					assert(sync.position < sq_size);
					assert(sync.pop_lock);
					sync.pop_lock = 0;
				});

				m_rcv.notify_one();
				return true;
			}

			bool peek(T& data, u32 start_pos, const volatile bool* do_exit)
			{
				return peek(data, start_pos, [do_exit]() { return do_exit && *do_exit; });
			}

			force_inline bool peek(T& data, u32 start_pos = 0)
			{
				return peek(data, start_pos, SQUEUE_NEVER_EXIT);
			}

			force_inline bool try_peek(T& data, u32 start_pos = 0)
			{
				return peek(data, start_pos, SQUEUE_ALWAYS_EXIT);
			}

			class squeue_data_t
			{
				T* const m_data;
				const u32 m_pos;
				const u32 m_count;

				squeue_data_t(T* data, u32 pos, u32 count)
					: m_data(data)
					, m_pos(pos)
					, m_count(count)
				{
				}

			public:
				T& operator [] (u32 index)
				{
					assert(index < m_count);
					index += m_pos;
					index = index < sq_size ? index : index - sq_size;
					return m_data[index];
				}
			};

			void process(void(*proc)(squeue_data_t data))
			{
				u32 pos, count;

				while (m_sync.atomic_op([&pos, &count](squeue_sync_var_t& sync) -> u32
				{
					assert(sync.count <= sq_size);
					assert(sync.position < sq_size);

					if (sync.pop_lock || sync.push_lock)
					{
						return SQSVR_LOCKED;
					}

					pos = sync.position;
					count = sync.count;
					sync.pop_lock = 1;
					sync.push_lock = 1;
					return SQSVR_OK;
				}))
				{
					std::unique_lock<std::mutex> rcv_lock(m_rcv_mutex);
					m_rcv.wait_for(rcv_lock, std::chrono::milliseconds(1));
				}

				proc(squeue_data_t(m_data, pos, count));

				m_sync.atomic_op([](squeue_sync_var_t& sync)
				{
					assert(sync.count <= sq_size);
					assert(sync.position < sq_size);
					assert(sync.pop_lock && sync.push_lock);
					sync.pop_lock = 0;
					sync.push_lock = 0;
				});

				m_wcv.notify_one();
				m_rcv.notify_one();
			}

			void clear()
			{
				while (m_sync.atomic_op([](squeue_sync_var_t& sync) -> u32
				{
					assert(sync.count <= sq_size);
					assert(sync.position < sq_size);

					if (sync.pop_lock || sync.push_lock)
					{
						return SQSVR_LOCKED;
					}

					sync.pop_lock = 1;
					sync.push_lock = 1;
					return SQSVR_OK;
				}))
				{
					std::unique_lock<std::mutex> rcv_lock(m_rcv_mutex);
					m_rcv.wait_for(rcv_lock, std::chrono::milliseconds(1));
				}

				m_sync.exchange({});
				m_wcv.notify_one();
				m_rcv.notify_one();
			}
		};
	}
}