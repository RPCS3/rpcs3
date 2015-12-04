#pragma once

namespace memory_helper
{
	/**
	* Reserve size bytes of virtual memory and returns it.
	* The memory should be commited before usage.
	*/
	void* reserve_memory(size_t size);

	/**
	* Commit page_size bytes of virtual memory starting at pointer.
	* That is, bake reserved memory with physical memory.
	* pointer should belong to a range of reserved memory.
	*/
	void commit_page_memory(void* pointer, size_t page_size);

	/**
	* Free memory alloced via reserve_memory.
	*/
	void free_reserved_memory(void* pointer, size_t size);
}