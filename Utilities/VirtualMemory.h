#pragma once

// Failure codes for the functions
enum
{
	VM_SUCCESS = 0,
	VM_FAILURE = -1,
};

namespace memory_helper
{
	/**
	* Reserve size bytes of virtual memory and returns it.
	* The memory should be commited before usage.
	*
	* Returns the base address of the allocated region of pages, if successful.
	* Returns (void*)VM_FAILURE, if unsuccessful.
	*/
	void* reserve_memory(size_t size);

	/**
	* Commit page_size bytes of virtual memory starting at pointer.
	* That is, bake reserved memory with physical memory.
	* pointer should belong to a range of reserved memory.
	*
	* Returns VM_SUCCESS, if successful.
	* Returns VM_FAILURE, if unsuccessful.
	*/
	s32 commit_page_memory(void* pointer, size_t page_size);

	/**
	* Free memory alloced via reserve_memory.
	*
	* Returns VM_SUCCESS, if successful.
	* Returns VM_FAILURE, if unsuccessful.
	*/
	s32 free_reserved_memory(void* pointer, size_t size);
}