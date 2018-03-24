#ifdef _MSC_VER
#include "stdafx.h"
#include "stdafx_d3d12.h"
#include "D3D12MemoryHelpers.h"
#include "Utilities/VirtualMemory.h"


void data_cache::store_and_protect_data(u64 key, u32 start, size_t size, u8 format, size_t w, size_t h, size_t d, size_t m, ComPtr<ID3D12Resource> data)
{
	std::lock_guard<shared_mutex> lock(m_mut);
	m_address_to_data[key] = std::make_pair(texture_entry(format, w, h, d, m), data);
	protect_data(key, start, size);
}

void data_cache::protect_data(u64 key, u32 start, size_t size)
{
	/// align start to 4096 byte
	static const u32 memory_page_size = 4096;
	u32 protected_range_start = start & ~(memory_page_size - 1);
	u32 protected_range_size = (u32)align(size, memory_page_size);
	m_protected_ranges.push_back(std::make_tuple(key, protected_range_start, protected_range_size));
	utils::memory_protect(vm::base(protected_range_start), protected_range_size, utils::protection::ro);
}

bool data_cache::invalidate_address(u32 addr)
{
	// In case 2 threads write to texture memory
	std::lock_guard<shared_mutex> lock(m_mut);
	bool handled = false;
	auto It = m_protected_ranges.begin(), E = m_protected_ranges.end();
	for (; It != E;)
	{
		auto currentIt = It;
		++It;
		auto protectedTexture = *currentIt;
		u32 protectedRangeStart = std::get<1>(protectedTexture), protectedRangeSize = std::get<2>(protectedTexture);
		if (addr >= protectedRangeStart && addr <= protectedRangeSize + protectedRangeStart)
		{
			u64 texadrr = std::get<0>(protectedTexture);
			m_address_to_data[texadrr].first.m_is_dirty = true;

			utils::memory_protect(vm::base(protectedRangeStart), protectedRangeSize, utils::protection::rw);
			m_protected_ranges.erase(currentIt);
			handled = true;
		}
	}
	return handled;
}

std::pair<texture_entry, ComPtr<ID3D12Resource> > *data_cache::find_data_if_available(u64 key)
{
	std::lock_guard<shared_mutex> lock(m_mut);
	auto It = m_address_to_data.find(key);
	if (It == m_address_to_data.end())
		return nullptr;
	return &It->second;
}

void data_cache::unprotect_all()
{
	std::lock_guard<shared_mutex> lock(m_mut);
	for (auto &protectedTexture : m_protected_ranges)
	{
		u32 protectedRangeStart = std::get<1>(protectedTexture), protectedRangeSize = std::get<2>(protectedTexture);
		utils::memory_protect(vm::base(protectedRangeStart), protectedRangeSize, utils::protection::rw);
	}
}

ComPtr<ID3D12Resource> data_cache::remove_from_cache(u64 key)
{
	auto result = m_address_to_data[key].second;
	m_address_to_data.erase(key);
	return result;
}

void resource_storage::reset()
{
	descriptors_heap_index = 0;
	current_sampler_index = 0;
	sampler_descriptors_heap_index = 0;
	render_targets_descriptors_heap_index = 0;
	depth_stencil_descriptor_heap_index = 0;

	CHECK_HRESULT(command_allocator->Reset());
	set_new_command_list();
}

void resource_storage::set_new_command_list()
{
	CHECK_HRESULT(command_list->Reset(command_allocator.Get(), nullptr));

	ID3D12DescriptorHeap *descriptors[] =
	{
		descriptors_heap.Get(),
		sampler_descriptor_heap[sampler_descriptors_heap_index].Get(),
	};
	command_list->SetDescriptorHeaps(2, descriptors);
}

void resource_storage::init(ID3D12Device *device)
{
	in_use = false;
	m_device = device;
	ram_framebuffer = nullptr;
	// Create a global command allocator
	CHECK_HRESULT(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(command_allocator.GetAddressOf())));

	CHECK_HRESULT(m_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, command_allocator.Get(), nullptr, IID_PPV_ARGS(command_list.GetAddressOf())));
	CHECK_HRESULT(command_list->Close());

	D3D12_DESCRIPTOR_HEAP_DESC descriptor_heap_desc = { D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 50000, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE };
	CHECK_HRESULT(device->CreateDescriptorHeap(&descriptor_heap_desc, IID_PPV_ARGS(&descriptors_heap)));

	D3D12_DESCRIPTOR_HEAP_DESC sampler_heap_desc = { D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER , 2048, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE };
	CHECK_HRESULT(device->CreateDescriptorHeap(&sampler_heap_desc, IID_PPV_ARGS(&sampler_descriptor_heap[0])));
	CHECK_HRESULT(device->CreateDescriptorHeap(&sampler_heap_desc, IID_PPV_ARGS(&sampler_descriptor_heap[1])));

	D3D12_DESCRIPTOR_HEAP_DESC ds_descriptor_heap_desc = { D3D12_DESCRIPTOR_HEAP_TYPE_DSV , 10000};
	device->CreateDescriptorHeap(&ds_descriptor_heap_desc, IID_PPV_ARGS(&depth_stencil_descriptor_heap));

	D3D12_DESCRIPTOR_HEAP_DESC rtv_descriptor_heap_desc = { D3D12_DESCRIPTOR_HEAP_TYPE_RTV , 10000 };
	device->CreateDescriptorHeap(&rtv_descriptor_heap_desc, IID_PPV_ARGS(&render_targets_descriptors_heap));

	frame_finished_handle = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
	fence_value = 0;
	CHECK_HRESULT(device->CreateFence(fence_value++, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(frame_finished_fence.GetAddressOf())));
}

void resource_storage::wait_and_clean()
{
	if (in_use)
		WaitForSingleObjectEx(frame_finished_handle, INFINITE, FALSE);
	else
		CHECK_HRESULT(command_list->Close());

	reset();

	dirty_textures.clear();

	ram_framebuffer = nullptr;
}

void resource_storage::release()
{
	dirty_textures.clear();
	// NOTE: Should be released only after gfx pipeline last command has been finished.
	CloseHandle(frame_finished_handle);
}

#endif
