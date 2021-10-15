#pragma once

#include "util/atomic.hpp"

namespace stx
{
	// Mutex designed to support 3 categories of concurrent access to protected resource (initialization, finalization and normal use) while holding the "init" state bit
	class init_mutex
	{
		// Set after initialization and removed before finalization
		static const u32 c_init_bit = 0x8000'0000;

		// Contains "reader" count and init bit
		atomic_t<u32> m_state = 0;

	public:
		constexpr init_mutex() noexcept = default;

		class init_lock final
		{
			init_mutex* _this;

		public:
			template <typename... FAndArgs>
			explicit init_lock(init_mutex& mtx, FAndArgs&&... args) noexcept
				: _this(&mtx)
			{
				while (true)
				{
					auto [val, ok] = _this->m_state.fetch_op([](u32& value)
					{
						if (value == 0)
						{
							value = 1;
							return true;
						}

						if constexpr (sizeof...(FAndArgs))
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
						if constexpr (sizeof...(FAndArgs))
						{
							// Forced reset
							val -= c_init_bit - 1;

							while (val != 1)
							{
								// Wait for other users to finish their work
								_this->m_state.wait(val);
								val = _this->m_state;
							}

							// Call specified reset function
							std::invoke(std::forward<FAndArgs>(args)...);
							break;
						}

						// Failure
						_this = nullptr;
						break;
					}

					_this->m_state.wait(val);
				}
			}

			init_lock(const init_lock&) = delete;

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
		};

		// Obtain exclusive lock to initialize protected resource. Waits for ongoing initialization or finalization. Fails if already initialized.
		[[nodiscard]] init_lock init() noexcept
		{
			return init_lock(*this);
		}

		// Same as init, but never fails, and executes provided `on_reset` function if already initialized.
		template <typename F, typename... Args>
		[[nodiscard]] init_lock init_always(F on_reset, Args&&... args) noexcept
		{
			return init_lock(*this, std::move(on_reset), std::forward<Args>(args)...);
		}

		class reset_lock final
		{
			init_mutex* _this;

		public:
			explicit reset_lock(init_mutex& mtx) noexcept
				: _this(&mtx)
			{
				auto [val, ok] = _this->m_state.fetch_op([](u32& value)
				{
					if (value & c_init_bit)
					{
						// Remove initialized state and add "init lock"
						value -= c_init_bit - 1;
						return true;
					}

					return false;
				});

				if (!ok)
				{
					// Failure: not initialized
					_this = nullptr;
					return;
				}

				while (val != 1)
				{
					// Wait for other users to finish their work
					_this->m_state.wait(val);
					val = _this->m_state;
				}
			}

			reset_lock(const reset_lock&) = delete;

			reset_lock& operator=(const reset_lock&) = delete;

			~reset_lock()
			{
				if (_this)
				{
					// Set uninitialized state and remove "init lock"
					_this->m_state -= 1;
					_this->m_state.notify_all();
				}
			}

			explicit operator bool() const& noexcept
			{
				return _this != nullptr;
			}

			explicit operator bool() && = delete;

			void set_init() & noexcept
			{
				if (_this)
				{
					// Set initialized state (TODO?)
					_this->m_state |= c_init_bit;
					_this->m_state.notify_all();
				}
			}
		};

		// Obtain exclusive lock to finalize protected resource. Waits for ongoing use. Fails if not initialized.
		[[nodiscard]] reset_lock reset() noexcept
		{
			return reset_lock(*this);
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
}
