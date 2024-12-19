#include "stdafx.h"
#include "system_progress.hpp"

shared_ptr<std::string> progress_dialog_string_t::get_string_ptr() const noexcept
{
	shared_ptr<std::string> text;
	data_t old_val = data.load();

	while (true)
	{
		text.reset();

		if (old_val.text_count == 0)
		{
			return text;
		}

		if (old_val.text_index != umax)
		{
			if (text = g_progr_text_queue[old_val.text_index].load(); !text)
			{
				// Search for available text (until the time another thread sets index)
				const u64 queue_size = g_progr_text_queue.size();

				for (u64 i = queue_size - 1;; i--)
				{
					if (i == umax)
					{
						return text;
					}

					if ((text = g_progr_text_queue[i].load()))
					{
						return text;
					}
				}
			}
		}

		auto new_val = data.load();

		if (old_val.update_id == new_val.update_id)
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
			m_text_index = ::narrow<u32>(new_text_index);
			break;
		}
	}

	g_progr_text.data.atomic_op([&](std::common_type_t<progress_dialog_string_t::data_t>& progr)
	{
		progr.update_id++;
		progr.text_index = m_text_index;
		progr.text_count++;
	});
}

scoped_progress_dialog& scoped_progress_dialog::operator=(std::string text) noexcept
{
	// Set text atomically
	g_progr_text_queue[m_text_index].store(make_single_value(std::move(text)));
	return *this;
}

scoped_progress_dialog::~scoped_progress_dialog() noexcept
{
	// Deallocate memory
	g_progr_text_queue[m_text_index].reset();

	auto progr = g_progr_text.data.load();

	while (true)
	{
		u64 restored_text_index = umax;

		if (progr.text_index == m_text_index && !g_progr_text_queue[m_text_index])
		{
			// Search for available text
			// Out of scope of atomic loop to keep atomic_op as clean as possible (for potential future enhacements)
			const u64 queue_size = g_progr_text_queue.size();

			for (u64 i = queue_size - 1;; i--)
			{
				if (i == umax)
				{
					restored_text_index = umax;
					break;
				}

				if (g_progr_text_queue[i].operator bool())
				{
					restored_text_index = i;
					break;
				}
			}
		}

		auto [old, ok] = g_progr_text.data.fetch_op([&](std::common_type_t<progress_dialog_string_t::data_t>& progr0)
		{
			if (progr.update_id != progr0.update_id)
			{
				// Repeat the loop
				return false;
			}

			progr0.text_count--;

			if (progr0.text_index == m_text_index)
			{
				if (restored_text_index == umax)
				{
					progr0.text_index = umax;
					progr0.update_id++;
					return true;
				}

				progr0.text_index = ::narrow<u32>(restored_text_index);
				progr0.update_id++;
			}

			return true;
		});

		progr = old;

		if (!ok)
		{
			continue;
		}

		break;
	}
}
