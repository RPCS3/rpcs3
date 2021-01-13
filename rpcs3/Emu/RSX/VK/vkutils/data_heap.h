#pragma once

#include "../../Common/ring_buffer_helper.h"
#include "../VulkanAPI.h"
#include "buffer_object.h"
#include "commands.h"

#include <memory>
#include <vector>

namespace vk
{
	class data_heap : public ::data_heap
	{
	private:
		usz initial_size = 0;
		bool mapped = false;
		void* _ptr = nullptr;

		bool notify_on_grow = false;

		std::unique_ptr<buffer> shadow;
		std::vector<VkBufferCopy> dirty_ranges;

	protected:
		bool grow(usz size) override;

	public:
		std::unique_ptr<buffer> heap;

		// NOTE: Some drivers (RADV) use heavyweight OS map/unmap routines that are insanely slow
		// Avoid mapping/unmapping to keep these drivers from stalling
		// NOTE2: HOST_CACHED flag does not keep the mapped ptr around in the driver either

		void create(VkBufferUsageFlags usage, usz size, const char* name, usz guard = 0x10000, VkBool32 notify = VK_FALSE);
		void destroy();

		void* map(usz offset, usz size);
		void unmap(bool force = false);

		void sync(const vk::command_buffer& cmd);

		// Properties
		bool is_dirty() const;
		bool is_critical() const override;
	};

	extern data_heap* get_upload_heap();
}
