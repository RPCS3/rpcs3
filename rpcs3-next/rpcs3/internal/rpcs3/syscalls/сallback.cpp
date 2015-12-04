#include <rpcs3/core/log.h>

#include <rpcs3/vm/vm.h>

void CallbackManager::Register(check_cb_t func)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	m_check_cb.emplace(std::move(func));
}

void CallbackManager::Async(async_cb_t func)
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (!m_cb_thread)
	{
		throw EXCEPTION("Callback thread not found");
	}

	m_async_cb.emplace(std::move(func));

	m_cb_thread->cv.notify_one();
}

CallbackManager::check_cb_t CallbackManager::Check()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	if (m_check_cb.size())
	{
		check_cb_t func = std::move(m_check_cb.front());

		m_check_cb.pop();

		return func;
	}

	return nullptr;
}

void CallbackManager::Init()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	auto task = [this](PPUThread& ppu)
	{
		std::unique_lock<std::mutex> lock(m_mutex);

		while (true)
		{
			CHECK_EMU_STATUS;

			if (!lock)
			{
				lock.lock();
				continue;
			}

			if (m_async_cb.size())
			{
				async_cb_t func = std::move(m_async_cb.front());

				m_async_cb.pop();

				if (lock) lock.unlock();

				func(ppu);

				continue;
			}

			ppu.cv.wait(lock);
		}
	};

	if (vm::get(vm::main)->addr == 0x10000)
	{
		auto thread = idm::make_ptr<PPUThread>("Callback Thread");

		thread->prio = 1001;
		thread->stack_size = 0x10000;
		thread->custom_task = task;
		thread->run();

		m_cb_thread = thread;
	}
}

void CallbackManager::Clear()
{
	std::lock_guard<std::mutex> lock(m_mutex);

	m_check_cb = decltype(m_check_cb){};
	m_async_cb = decltype(m_async_cb){};

	m_cb_thread.reset();
}
