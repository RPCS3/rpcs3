	bool read_string(u32 addr, u32 max_size, std::string& out_string, bool check_pages) noexcept
	{
		if (!max_size)
		{
			return true;
		}

		// Prevent overflow
		const u32 size = 0 - max_size < addr ? (0 - addr) : max_size;
		std::string result;
		result.reserve(std::min<u32>(size, 4096));

		for (u32 i = addr, end = utils::align(addr + size, 4096) - 1; i <= end;)
		{
			if (check_pages && !vm::check_addr(i, vm::page_readable))
			{
				// Invalid string termination
				return false;
			}

			const char* s_start = vm::get_super_ptr<const char>(i);
			const u32 space = std::min<u32>(end - i + 1, 4096 - (i % 4096));
			const char* s_end = s_start + space;
			const char* s_null = std::find(s_start, s_end, '\0');

			result.append(s_start, s_null);

			if (s_null != s_end)
			{
				out_string += result;
				return true;
			}

			i += space;

			if (!i)
			{
				break;
			}
		}

		// Non-null terminated but terminated by size limit (so the string may continue)
		out_string += result;
		return size == max_size;
	}
