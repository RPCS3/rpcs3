#pragma once

#include "util/atomic.hpp"

namespace stx
{
	// Mutex designed to support 3 categories of concurrent access to protected resource (initialization, finalization and normal use) while holding the "init" state bit
	class init_mutex
	{
		// Set after initialization and removed before finalization
		static constexpr u32 c_init_bit = 0x8000'0000;

		// Contains "reader" count and init bit
		atomic_t<u32> m_state = 0;

	public:
		constexpr init_mutex() noexcept = default;

		class init_lock final
		{
			init_mutex* _this;

			template <typename Func, typename... Args>
			void invoke_callback(int invoke_count, Func&& func, Args&&... args) const
			{
				std::invoke(func, invoke_count, *this, std::forward<Args>(args)...);
			}

		public:
			template <typename Forced, typename... FAndArgs>
			explicit init_lock(init_mutex& mtx, Forced&&, FAndArgs&&... args) noexcept
				: _this(&mtx)
			{
				bool invoked_func = false;

				while (true)
				{
					auto [val, ok] = _this->m_state.fetch_op([](u32& value)
					{
						if (value == 0)
						{
							value = 1;
							return true;
						}

						if constexpr (Forced()())
						{
							if (value & c_init_bit)
							{
								value -= c_init_bit - 1;
								return true;
							}
						}

						return false;
					});

					if (val == 0)
					{
						// Success: obtained "init lock"
						break;
					}

					if (val & c_init_bit)
					{
						if constexpr (Forced()())
						{
							// Forced reset
							val -= c_init_bit - 1;

							while (val != 1)
							{
								if constexpr (sizeof...(FAndArgs))
								{
									if (!invoked_func)
									{
										invoke_callback(0, std::forward<FAndArgs>(args)...);
										invoked_func = true;
									}
								}

								// Wait for other users to finish their work
								_this->m_state.wait(val);
								val = _this->m_state;
							}
						}

						// Failure
						_this = nullptr;
						break;
					}

					if constexpr (sizeof...(FAndArgs))
					{
						if (!invoked_func)
						{
							invoke_callback(0, std::forward<FAndArgs>(args)...);
							invoked_func = true;
						}
					}

					_this->m_state.wait(val);
				}

				// Finalization of wait callback
				if constexpr (sizeof...(FAndArgs))
				{
					if (invoked_func)
					{
						invoke_callback(1, std::forward<FAndArgs>(args)...);
					}
				}
			}

			init_lock(const init_lock&) = delete;

			init_lock(init_lock&& lock) noexcept
				: _this(std::exchange(lock._this, nullptr))
			{
			}

			init_lock& operator=(const init_lock&) = delete;

			~init_lock()
			{
				if (_this)
				{
					// Set initialized state and remove "init lock"
					_this->m_state += c_init_bit - 1;
					_this->m_state.notify_all();
				}
			}

			explicit operator bool() const& noexcept
			{
				return _this != nullptr;
			}

			explicit operator bool() && = delete;

			void force_lock(init_mutex* mtx)
			{
				_this = mtx;
			}

			void cancel() & noexcept
			{
				if (_this)
				{
					// Abandon "init lock"
					_this->m_state -= 1;
					_this->m_state.notify_all();
					_this = nullptr;
				}
			}

			init_mutex* get_mutex()
			{
				return _this;
			}
		};

		// Obtain exclusive lock to initialize protected resource. Waits for ongoing initialization or finalization. Fails if already initialized.
		template <typename... FAndArgs>
		[[nodiscard]] init_lock init(FAndArgs&&... args) noexcept
		{
			return init_lock(*this, std::false_type{}, std::forward<FAndArgs>(args)...);
		}

		// Same as init, but never fails, and executes provided `on_reset` function if already initialized.
		template <typename F, typename... Args>
		[[nodiscard]] init_lock init_always(F on_reset, Args&&... args) noexcept
		{
			init_lock lock(*this, std::true_type{});

			if (!lock)
			{
				lock.force_lock(this);
				std::invoke(std::forward<F>(on_reset), std::forward<Args>(args)...);
			}

			return lock;
		}

		class reset_lock final
		{
			init_lock _lock;

		public:
			explicit reset_lock(init_lock&& lock) noexcept
				: _lock(std::move(lock))
			{
			}

			reset_lock(const reset_lock&) = delete;

			reset_lock& operator=(const reset_lock&) = delete;

			~reset_lock()
			{
				if (_lock)
				{
					// Set uninitialized state and remove "init lock"
					_lock.cancel();
				}
			}

			explicit operator bool() const& noexcept
			{
				return !!_lock;
			}

			explicit operator bool() && = delete;

			void set_init() & noexcept
			{
				if (_lock)
				{
					// Set initialized state (TODO?)
					_lock.get_mutex()->m_state |= c_init_bit;
					_lock.get_mutex()->m_state.notify_all();
				}
			}
		};

		// Obtain exclusive lock to finalize protected resource. Waits for ongoing use. Fails if not initialized.
		template <typename F, typename... Args>
		[[nodiscard]] reset_lock reset(F on_reset, Args&&... args) noexcept
		{
			init_lock lock(*this, std::true_type{}, std::forward<F>(on_reset), std::forward<Args>(args)...);

			if (!lock)
			{
				lock.force_lock(this);
			}
			else
			{
				lock.cancel();
			}

			return reset_lock(std::move(lock));
		}

		[[nodiscard]] reset_lock reset() noexcept
		{
			init_lock lock(*this, std::true_type{});

			if (!lock)
			{
				lock.force_lock(this);
			}
			else
			{
				lock.cancel();
			}

			return reset_lock(std::move(lock));
		}

		class access_lock final
		{
			init_mutex* _this;

		public:
			explicit access_lock(init_mutex& mtx) noexcept
				: _this(&mtx)
			{
				auto [val, ok] = _this->m_state.fetch_op([](u32& value)
				{
					if (value & c_init_bit)
					{
						// Add "access lock"
						value += 1;
						return true;
					}

					return false;
				});

				if (!ok)
				{
					_this = nullptr;
					return;
				}
			}

			access_lock(const access_lock&) = delete;

			access_lock& operator=(const access_lock&) = delete;

			~access_lock()
			{
				if (_this)
				{
					// TODO: check condition
					if (--_this->m_state <= 1)
					{
						_this->m_state.notify_all();
					}
				}
			}

			explicit operator bool() const& noexcept
			{
				return _this != nullptr;
			}

			explicit operator bool() && = delete;
		};

		// Obtain shared lock to use protected resource. Fails if not initialized.
		[[nodiscard]] access_lock access() noexcept
		{
			return access_lock(*this);
		}

		// Simple state test. Hard to use, easy to misuse.
		bool volatile_is_initialized() const noexcept
		{
			return (m_state & c_init_bit) != 0;
		}

		// Wait for access()
		void wait_for_initialized() const noexcept
		{
			const u32 state = m_state;

			if (state & c_init_bit)
			{
				return;
			}

			m_state.wait(state);
		}
	};

	using init_lock = init_mutex::init_lock;
	using reset_lock = init_mutex::reset_lock;
	using access_lock = init_mutex::access_lock;
}
