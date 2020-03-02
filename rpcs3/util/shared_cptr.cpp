#include "shared_cptr.hpp"

#include <thread>

stx::atomic_base::ptr_type stx::atomic_base::ref_halve() const noexcept
{
	ptr_type v = val_load();

	while (true)
	{
		if (!(v & c_ref_mask))
		{
			// Nullptr or depleted reference pool
			return 0;
		}
		else if (val_compare_exchange(v, (v & ~c_ref_mask) | (v & c_ref_mask) >> 1))
		{
			break;
		}
	}

	// Return acquired references (rounded towards zero)
	return (v & ~c_ref_mask) | ((v & c_ref_mask) - ((v & c_ref_mask) >> 1) - 1);
}

stx::atomic_base::ptr_type stx::atomic_base::ref_load() const noexcept
{
	ptr_type v = val_load();

	while (true)
	{
		if (!(v & c_ref_mask))
		{
			if (v == 0)
			{
				// Null pointer
				return 0;
			}

			// Busy wait
			std::this_thread::yield();
			v = val_load();
		}
		else if (val_compare_exchange(v, v - 1))
		{
			break;
		}
	}

	// Obtained 1 reference from the atomic pointer
	return v & ~c_ref_mask;
}

void stx::atomic_base::ref_fix(stx::atomic_base::ptr_type& _old) const noexcept
{
	ptr_type old = _old & ~c_ref_mask;
	ptr_type v = val_load();

	while (true)
	{
		if ((v & ~c_ref_mask) != old || (v & c_ref_mask) == c_ref_mask)
		{
			// Can't return a reference to the original atomic pointer, so keep it
			_old += 1;
			return;
		}

		if (val_compare_exchange(v, v + 1))
		{
			break;
		}
	}
}
