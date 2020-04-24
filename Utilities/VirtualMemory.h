#pragma once

#include "types.h"

namespace utils
{
	// Memory protection type
	enum class protection
	{
		rw, // Read + write (default)
		ro, // Read only
		no, // No access
		wx, // Read + write + execute
		rx, // Read + execute
	};

	/**
	* Reserve `size` bytes of virtual memory and returns it.
	* The memory should be commited before usage.
	*/
	void* memory_reserve(std::size_t size, void* use_addr = nullptr);

	/**
	* Commit `size` bytes of virtual memory starting at pointer.
	* That is, bake reserved memory with physical memory.
	* pointer should belong to a range of reserved memory.
	*/
	void memory_commit(void* pointer, std::size_t size, protection prot = protection::rw);

	// Decommit all memory committed via commit_page_memory.
	void memory_decommit(void* pointer, std::size_t size);

	// Decommit all memory and commit it again.
	void memory_reset(void* pointer, std::size_t size, protection prot = protection::rw);

	// Free memory after reserved by memory_reserve, should specify original size
	void memory_release(void* pointer, std::size_t size);

	// Set memory protection
	void memory_protect(void* pointer, std::size_t size, protection prot);

	// Shared memory handle
	class shm
	{
#ifdef _WIN32
		void* m_handle;
#else
		int m_file;
#endif
		u32 m_size;
		u32 m_flags;

	public:
		explicit shm(u32 size, u32 flags = 0);

		shm(const shm&) = delete;

		shm& operator=(const shm&) = delete;

		~shm();

		// Map shared memory
		u8* map(void* ptr, protection prot = protection::rw) const;

		// Map shared memory over reserved memory region, which is unsafe (non-atomic) under Win32
		u8* map_critical(void* ptr, protection prot = protection::rw);

		// Unmap shared memory
		void unmap(void* ptr) const;

		// Unmap shared memory, undoing map_critical
		void unmap_critical(void* ptr);

		u32 size() const
		{
			return m_size;
		}

		// Flags are unspecified, consider it userdata
		u32 flags() const
		{
			return m_flags;
		}
	};
}
