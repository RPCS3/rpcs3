#pragma once
#include <functional>
#include <deque>
#include <list>

enum class event_result
{
	skip,
	handled
};

class events_queue
{
	std::deque<std::function<void()>> m_queue;

public:
	/*template<typename Type>
	events_queue& operator += (event<Type> &evt)
	{
		evt.set_queue(this);
		return *this;
	}
*/
	void invoke(std::function<void()> function)
	{
		m_queue.push_back(function);
	}

	void flush()
	{
		while (!m_queue.empty())
		{
			std::function<void()> function = m_queue.front();
			function();
			m_queue.pop_front();
		}
	}
};

template<typename ...AT>
class event
{
	using func_t = std::function<event_result(AT...)>;
	using entry_t = typename std::list<func_t>::iterator;

	std::list<func_t> handlers;
	events_queue& m_queue;

public:
	event(events_queue* queue = nullptr) : m_queue(*queue)
	{
	}

	void invoke(AT... args)
	{
		m_queue.invoke(std::bind([&](AT... eargs)
		{
			for (auto &&handler : handlers)
			{
				if (handler(eargs...) == event_result::handled)
					break;
			}
		}, args...));
	}

	void operator()(AT... args)
	{
		invoke(args...);
	}

	entry_t bind(func_t func)
	{
		handlers.push_front(func);
		return handlers.begin();
	}

	template<typename T>
	entry_t bind(T *caller, event_result(T::*callback)(AT...))
	{
		return bind([=](AT... args) { return (caller->*callback)(args...); });
	}

	template<typename T>
	entry_t bind(T *caller, void(T::*callback)(AT...))
	{
		return bind([=](AT... args) { (caller->*callback)(args...); return event_result::skip; });
	}

	void unbind(entry_t entry)
	{
		handlers.erase(entry);
	}

	entry_t operator +=(func_t func)
	{
		return bind(func);
	}

	void operator -=(entry_t what)
	{
		return unbind(what);
	}
};

template<>
class event<void>
{
	using func_t = std::function<event_result()>;
	using entry_t = std::list<func_t>::iterator;

	std::list<func_t> m_listeners;
	events_queue* m_queue;

	void invoke_listeners()
	{
		for (auto &&listener : m_listeners)
		{
			if (listener() == event_result::handled)
				break;
		}
	}

public:
	event(events_queue* queue = nullptr) : m_queue(queue)
	{
	}

	void invoke()
	{
		if (m_queue)
			m_queue->invoke([=]() { invoke_listeners(); });
		else
			invoke_listeners();
	}

	void operator()()
	{
		invoke();
	}

	entry_t bind(func_t func)
	{
		m_listeners.push_front(func);
		return m_listeners.begin();
	}

	template<typename T>
	entry_t bind(T *caller, event_result(T::*callback)())
	{
		return bind([=]() { return (caller->*callback)(); });
	}

	template<typename T>
	entry_t bind(T *caller, void(T::*callback)())
	{
		return bind([=]() { (caller->*callback)(); return event_result::skip; });
	}

	void unbind(entry_t what)
	{
		m_listeners.erase(what);
	}

	entry_t operator +=(func_t func)
	{
		return bind(func);
	}

	void operator -=(entry_t what)
	{
		return unbind(what);
	}
};

class event_binder_t
{
	template<typename Type>
	class binder_impl_t
	{
		event_binder_t *m_binder;
		event<Type> *m_event;

	public:
		binder_impl_t(event_binder_t *binder, event<Type> *evt)
			: m_binder(binder)
			, m_event(evt)
		{
		}
	};

public:
	template<typename Type>
	binder_impl_t<Type> operator()(event<Type>& evt) const
	{
		return{ this, &evt };
	}
};

template<typename T>
class combined_data;

template<typename T>
class local_data
{
public:
	using type = T;

protected:
	type m_data;

	void set(type value)
	{
		m_data = value;
	}

	type get() const
	{
		return m_data;
	}

	bool equals(T value) const
	{
		return get() == value;
	}

	bool invoke_event(type value)
	{
		return false;
	}

	friend combined_data<T>;
};

template<typename T>
class combined_data
{
public:
	using type = T;

protected:
	local_data<type> m_local_data;
	std::function<void(type)> m_invoke_event_function;
	std::function<type()> m_get_function;

	bool invoke_event(type value)
	{
		if (m_invoke_event_function)
		{
			m_invoke_event_function(value);
			return true;
		}

		return false;
	}

	void set(type value)
	{
		m_local_data.set(value);
	}

	type get() const
	{
		if (m_get_function)
		{
			return m_get_function();
		}

		return m_local_data.get();
	}

	type get_local() const
	{
		return m_local_data.get();
	}

	bool equals(T value) const
	{
		return get_local() == value;
	}

public:
	void invoke_event_function(std::function<void(type)> function)
	{
		m_invoke_event_function = function;
	}

	void get_function(std::function<type()> function)
	{
		m_get_function = function;
	}
};

template<typename T, typename base_type_ = local_data<T>>
class data_event : public base_type_
{
public:
	using type = T;
	using base_type = base_type_;

protected:
	event_result dochange(type new_value)
	{
		auto old_value = get();
		base_type::set(new_value);
		onchanged(old_value);
		return event_result::handled;
	}

public:
	event<type> onchange;
	event<type> onchanged;

	data_event(events_queue *queue = nullptr)
		: onchange(queue)
		, onchanged(queue)
	{
		onchange.bind(this, &data_event::dochange);
		base_type::set({});
	}

	type get() const
	{
		return base_type::get();
	}

	type operator()() const
	{
		return get();
	}

	void change(type value, bool use_custom_invoke_event = true)
	{
		if (!base_type::equals(value))
		{
			if (!use_custom_invoke_event || !base_type::invoke_event(value))
			{
				onchange(value);
			}
		}
	}

	operator const type() const
	{
		return get();
	}

	operator type()
	{
		return get();
	}

	data_event& operator = (type value)
	{
		change(value);
		return *this;
	}

	template<typename aType> auto operator + (aType value) const { return get() + value; }
	template<typename aType> auto operator - (aType value) const { return get() - value; }
	template<typename aType> auto operator * (aType value) const { return get() * value; }
	template<typename aType> auto operator / (aType value) const { return get() / value; }
	template<typename aType> auto operator % (aType value) const { return get() % value; }
	template<typename aType> auto operator & (aType value) const { return get() & value; }
	template<typename aType> auto operator | (aType value) const { return get() | value; }
	template<typename aType> auto operator ^ (aType value) const { return get() ^ value; }

	template<typename aType> data_event& operator += (aType value) { return *this = get() + value; }
	template<typename aType> data_event& operator -= (aType value) { return *this = get() - value; }
	template<typename aType> data_event& operator *= (aType value) { return *this = get() * value; }
	template<typename aType> data_event& operator /= (aType value) { return *this = get() / value; }
	template<typename aType> data_event& operator %= (aType value) { return *this = get() % value; }
	template<typename aType> data_event& operator &= (aType value) { return *this = get() & value; }
	template<typename aType> data_event& operator |= (aType value) { return *this = get() | value; }
	template<typename aType> data_event& operator ^= (aType value) { return *this = get() ^ value; }

	data_event& operator ++()
	{
		type value = get();
		return *this = ++value;
	}

	type operator ++(int)
	{
		type value = get();
		type result = value;
		*this = value++;
		return result;
	}

	data_event& operator --()
	{
		type value = get();
		return *this = --value;
	}

	type operator --(int)
	{
		type value = get();
		type result = value;
		*this = value--;
		return result;
	}
};

struct with_event_binder
{
	event_binder_t event_binder;
};

struct test_obj
{
	void test(int)
	{
		event<int> i;
		auto it = i.bind(this, &test_obj::test);
		i(5);
		i.unbind(it);
		i(6);
	}
};
