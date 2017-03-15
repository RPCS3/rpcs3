#pragma once

namespace utils
{
	/**
	* Reserve `size` bytes of virtual memory and returns it.
	* The memory should be commited before usage.
	*/
	void* memory_reserve(std::size_t size);

	/**
	* Commit `size` bytes of virtual memory starting at pointer.
	* That is, bake reserved memory with physical memory.
	* pointer should belong to a range of reserved memory.
	*/
	void memory_commit(void* pointer, std::size_t size);

	/**
	* Decommit all memory committed via commit_page_memory.
	*/
	void memory_decommit(void* pointer, std::size_t size);

	// Memory protection type
	enum class protection
	{
		rw, // Read + write (default)
		ro, // Read only
		no, // No access
		wx, // Read + write + execute
		rx, // Read + execute
	};

	void memory_protect(void* pointer, std::size_t size, protection prot);
}
