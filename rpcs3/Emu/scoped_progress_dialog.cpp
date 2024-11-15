#include "stdafx.h"
#include "system_progress.hpp"

shared_ptr<std::string> progress_dialog_string_t::get_string_ptr() const noexcept
{
	shared_ptr<std::string> text;
	data_t old_val = data.load();

	while (true)
	{
		text.reset();

		if (old_val.text_index != umax)
		{
			if (auto ptr = g_progr_text_queue[old_val.text_index].load())
			{
				text = ptr;
			}
		}

		auto new_val = data.load();

		if (old_val.text_index == new_val.text_index)
		{
			break;
		}

		old_val = new_val;
	}

	return text;
}

scoped_progress_dialog::scoped_progress_dialog(std::string text) noexcept
{
	shared_ptr<std::string> installed_ptr = make_single_value(std::move(text));
	const shared_ptr<std::string> c_null_ptr;

	for (usz init_size = g_progr_text_queue.size(), new_text_index = std::max<usz>(init_size, 1) - 1;;)
	{
		if (new_text_index >= init_size)
		{
			// Search element in new memeory
			new_text_index++;
		}
		else if (new_text_index == 0)
		{
			// Search element in new memeory
			new_text_index = init_size;
		}
		else
		{
			new_text_index--;
		}

		auto& info = g_progr_text_queue[new_text_index];

		if (!info && info.compare_and_swap_test(c_null_ptr, installed_ptr))
		{
			m_text_index = new_text_index;
			break;
		}
	}

	usz unmap_index = umax;

	g_progr_text.data.atomic_op([&](std::common_type_t<progress_dialog_string_t::data_t>& progr)
	{
		unmap_index = progr.text_index;
		progr.update_id++;
		progr.text_index = m_text_index;
	});

	// Note: unmap_index may be m_text_index if picked up at the destructor
	// Which is technically the newest value to be inserted so it serves its logic
	if (unmap_index != umax && unmap_index != m_text_index)
	{
		g_progr_text_queue[unmap_index].reset();
	}
}

scoped_progress_dialog& scoped_progress_dialog::operator=(std::string text) noexcept
{
	shared_ptr<std::string> installed_ptr = make_single_value(std::move(text));
	const shared_ptr<std::string> c_null_ptr;

	for (usz init_size = g_progr_text_queue.size(), new_text_index = std::max<usz>(init_size, 1) - 1;;)
	{
		if (new_text_index >= init_size)
		{
			// Search element in new memeory
			new_text_index++;
		}
		else if (new_text_index == 0)
		{
			// Search element in new memeory
			new_text_index = init_size;
		}
		else
		{
			new_text_index--;
		}

		auto& info = g_progr_text_queue[new_text_index];

		if (!info && info.compare_and_swap_test(c_null_ptr, installed_ptr))
		{
			m_text_index = new_text_index;
			break;
		}
	}

	usz unmap_index = umax;

	g_progr_text.data.atomic_op([&](std::common_type_t<progress_dialog_string_t::data_t>& progr)
	{
		unmap_index = progr.text_index;
		progr.update_id++;
		progr.text_index = m_text_index;
	});

	// Note: unmap_index may be m_text_index if picked up at the destructor
	// Which is technically the newest value to be inserted so it serves its logic
	if (unmap_index != umax && unmap_index != m_text_index)
	{
		g_progr_text_queue[unmap_index].reset();
	}

	return *this;
}

scoped_progress_dialog::~scoped_progress_dialog() noexcept
{
	bool unmap = false;

	while (true)
	{
		auto progr = g_progr_text.data.load();
		usz restored_text_index = umax;

		if (progr.text_index == m_text_index)
		{
			// Search for available text
			// Out of scope of atomic to keep atomic_op as clean as possible (for potential future enhacements)
			const u64 queue_size = g_progr_text_queue.size();

			for (u64 i = queue_size - 1;; i--)
			{
				if (i == umax)
				{
					restored_text_index = umax;
					break;
				}

				if (i == m_text_index)
				{
					continue;
				}

				if (g_progr_text_queue[i].operator bool())
				{
					restored_text_index = i;
					break;
				}
			}
		}

		if (!g_progr_text.data.atomic_op([&](std::common_type_t<progress_dialog_string_t::data_t>& progr0)
		{
			if (progr.update_id != progr0.update_id)
			{
				// Repeat the loop
				return false;
			}

			unmap = false;

			if (progr0.text_index == m_text_index)
			{
				unmap = true;

				if (restored_text_index == umax)
				{
					progr0.text_index = umax;
					progr0.update_id = 0;
					return true;
				}

				progr0.text_index = restored_text_index;
				progr0.update_id++;
			}

			return true;
		}))
		{
			continue;
		}

		break;
	}

	if (unmap)
	{
		g_progr_text_queue[m_text_index].reset();
	}
}
