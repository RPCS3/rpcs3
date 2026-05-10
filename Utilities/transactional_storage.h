#include "util/types.hpp"
#include <vector>
#include <mutex>
#include "Emu/Cell/timers.hpp"

// Thread-safe object pool with garbage collection
class universal_pool
{
public:

	universal_pool(u32 gc_interval = 10000) : gc_interval(gc_interval)
	{
	}

	~universal_pool()
	{
		std::lock_guard lock(mutex);
		storage.clear();
	}

	universal_pool(const universal_pool&) = delete;
	universal_pool& operator=(const universal_pool&) = delete;

	void set_gc_interval(u32 new_val)
	{
		gc_interval = new_val;
	}

	template <typename F>
	requires (std::invocable<F&> && std::is_same_v<std::invoke_result_t<F&>, std::shared_ptr<void>>)
	void add_op(F func)
	{
		std::lock_guard lock(mutex);
		if (std::shared_ptr<void> new_val = std::invoke(func); new_val)
		{
			storage.push_back(new_val);
		}
		delete_unused();
	}

	void force_gc()
	{
		std::lock_guard lock(mutex);
		delete_unused();
	}

private:

	void delete_unused()
	{
		const u32 gc_int = gc_interval.observe();

		if (u64 crnt_time = get_system_time(); gc_int == 0 || crnt_time > gc_last_time + gc_int)
		{
			gc_last_time = crnt_time;
			storage.erase
			(
				std::remove_if(storage.begin(), storage.end(), [](auto& obj) { return obj.use_count() <= 1; }),
				storage.end()
			);
		}
	}

	shared_mutex mutex{};
	std::vector<std::shared_ptr<void>> storage{};
	u64 gc_last_time = get_system_time();
	atomic_t<u32> gc_interval = 0;
};

template<typename T>
class transactional_storage
{
public:

	transactional_storage(std::shared_ptr<universal_pool> pool, std::shared_ptr<T> obj = std::make_shared<T>())
	{
		ensure(pool && obj);

		this->pool = pool;
		add(obj);
	}

	transactional_storage(const transactional_storage&) = delete;
	transactional_storage& operator=(const transactional_storage&) = delete;

	transactional_storage(transactional_storage&& other)
		: pool(std::move(other.pool))
	{
		std::unique_lock lock_other{other.current_mutex};
		const std::shared_ptr<T> other_current = other.current;
		other.current = nullptr;
		lock_other.unlock();

		std::lock_guard lock{current_mutex};
		current = other_current;
	}

	transactional_storage& operator=(transactional_storage&& other)
	{
		if (this == &other) return *this;

		pool = std::move(other.pool);

		std::unique_lock lock_other{other.current_mutex};
		const std::shared_ptr<T> other_current = other.current;
		other.current = nullptr;
		lock_other.unlock();

		std::lock_guard lock{current_mutex};
		current = other_current;

		return *this;
	}

	std::shared_ptr<const T> get_current()
	{
		reader_lock lock(current_mutex);
		return current;
	}

	void add(std::shared_ptr<T> obj)
	{
		if (!obj)
		{
			return;
		}

		pool->add_op([&]() -> std::shared_ptr<void>
		{
			{
				std::lock_guard lock{current_mutex};
				current = obj;
			}
			return std::move(obj);
		});
	}

	template <typename F>
	requires (std::invocable<F&> && std::is_same_v<std::invoke_result_t<F&>, std::shared_ptr<T>>)
	void add_op(F func)
	{
		pool->add_op([&]() -> std::shared_ptr<void>
		{
			std::shared_ptr<T> obj = std::invoke(func);
			if (obj)
			{
				std::lock_guard lock{current_mutex};
				current = obj;
			}
			return obj;
		});
	}

private:

	shared_mutex current_mutex{};
	std::shared_ptr<T> current{};
	std::shared_ptr<universal_pool> pool{};
};
