#pragma once
#include "atomic.h"

// run endless loop for debugging
__forceinline static void deadlock()
{
	while (true)
	{
		std::this_thread::yield();
	}
}

template<typename T>
class ref_t;

template<typename T>
class refcounter_t // non-relocateable "smart" pointer with ref counter
{
public:
	typedef T type, * p_type;
	typedef refcounter_t<T> rc_type;

	// counter > 0, ptr != nullptr : object exists and shared
	// counter > 0, ptr == nullptr : object exists and shared, but not owned by refcounter_t
	// counter == 0, ptr != nullptr : object exists and not shared
	// counter == 0, ptr == nullptr : object doesn't exist
	// counter < 0 : bad state, used to provoke error for debugging

	struct sync_var_t
	{
		s64 counter;
		p_type ptr;
	};

private:
	atomic_le_t<sync_var_t> m_var;

	friend class ref_t<T>;

	// try to share object (increment counter), returns nullptr if doesn't exist or cannot be shared
	__forceinline p_type ref_inc()
	{
		p_type out_ptr;

		m_var.atomic_op([&out_ptr](sync_var_t& v)
		{
			assert(v.counter >= 0);

			if ((out_ptr = v.ptr))
			{
				v.counter++;
			}
		});

		return out_ptr;
	}

	// try to release previously shared object (decrement counter), returns true if should be deleted
	__forceinline bool ref_dec()
	{
		bool do_delete;

		m_var.atomic_op([&do_delete](sync_var_t& v)
		{
			assert(v.counter > 0);

			do_delete = !--v.counter && !v.ptr;
		});

		return do_delete;
	}

public:
	refcounter_t()
	{
		// initialize ref counter
		m_var.write_relaxed({ 0, nullptr });
	}

	~refcounter_t()
	{
		// set bad state
		auto ref = m_var.exchange({ -1, nullptr });

		// finalize
		if (ref.counter)
		{
			deadlock();
		}
		else if (ref.ptr)
		{
			delete ref.ptr;
		}
	}

	refcounter_t(const rc_type& right) = delete;
	refcounter_t(rc_type&& right_rv) = delete;

	rc_type& operator =(const rc_type& right) = delete;
	rc_type& operator =(rc_type&& right_rv) = delete;

public:
	// try to set new object (if it doesn't exist)
	bool try_set(p_type ptr)
	{
		return m_var.compare_and_swap_test({ 0, nullptr }, { 0, ptr });
	}

	// try to remove object (if exists)
	bool try_remove()
	{
		bool out_res;
		p_type out_ptr;

		m_var.atomic_op([&out_res, &out_ptr](sync_var_t& v)
		{
			out_res = (out_ptr = v.ptr);

			if (v.counter)
			{
				out_ptr = nullptr;
			}

			v.ptr = nullptr;
		});

		if (out_ptr)
		{
			delete out_ptr;
		}

		return out_res;
	}
};

template<typename T>
class ref_t
{
public:
	typedef T type, * p_type;
	typedef refcounter_t<T> * rc_type;

private:
	rc_type m_rc;
	p_type m_ptr;

public:
	ref_t()
		: m_rc(nullptr)
		, m_ptr(nullptr)
	{
	}

	ref_t(rc_type rc)
		: m_rc(rc)
		, m_ptr(rc->ref_inc())
	{
	}

	~ref_t()
	{
		if (m_ptr && m_rc->ref_dec())
		{
			delete m_ptr;
		}
	}

	ref_t(const ref_t& right) = delete;

	ref_t(ref_t&& right_rv)
		: m_rc(right_rv.m_rc)
		, m_ptr(right_rv.m_ptr)
	{
		right_rv.m_rc = nullptr;
		right_rv.m_ptr = nullptr;
	}

	ref_t& operator =(const ref_t& right) = delete;
	ref_t& operator =(ref_t&& right_rv) = delete;

public:
	T& operator *() const
	{
		return *m_ptr;
	}

	T* operator ->() const
	{
		return m_ptr;
	}

	explicit operator bool() const
	{
		return m_ptr;
	}
};
