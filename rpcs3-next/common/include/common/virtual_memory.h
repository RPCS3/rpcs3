#pragma once
#include <cstddef>

namespace common
{
	namespace vm
	{
		/**
		* Reserve size bytes of virtual memory and returns it.
		* The memory should be commited before usage.
		*/
		void* map(std::size_t size);

		/**
		* Commit page_size bytes of virtual memory starting at pointer.
		* That is, bake reserved memory with physical memory.
		* pointer should belong to a range of reserved memory.
		*/
		void commit_page(void* pointer, std::size_t page_size);

		/**
		* Free memory alloced via map.
		*/
		void unmap(void* pointer, std::size_t size);
	}
}