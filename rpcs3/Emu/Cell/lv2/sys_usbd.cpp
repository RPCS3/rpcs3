#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"

#include "Emu/Cell/ErrorCodes.h"
#include "sys_usbd.h"

namespace vm { using namespace ps3; }

logs::channel sys_usbd("sys_usbd");

s32 sys_usbd_initialize()
{
	sys_usbd.todo("sys_usbd_initialize()");
	return CELL_OK;
}

s32 sys_usbd_finalize()
{
	sys_usbd.todo("sys_usbd_finalize()");
	return CELL_OK;
}

s32 sys_usbd_get_device_list()
{
	sys_usbd.todo("sys_usbd_get_device_list()");
	return CELL_OK;
}

s32 sys_usbd_get_descriptor_size()
{
	sys_usbd.todo("sys_usbd_get_descriptor_size()");
	return CELL_OK;
}

s32 sys_usbd_get_descriptor()
{
	sys_usbd.todo("sys_usbd_get_descriptor()");
	return CELL_OK;
}

s32 sys_usbd_register_ldd()
{
	sys_usbd.todo("sys_usbd_register_ldd()");
	return CELL_OK;
}

s32 sys_usbd_unregister_ldd()
{
	sys_usbd.todo("sys_usbd_unregister_ldd()");
	return CELL_OK;
}

s32 sys_usbd_open_pipe()
{
	sys_usbd.todo("sys_usbd_open_pipe()");
	return CELL_OK;
}

s32 sys_usbd_open_default_pipe()
{
	sys_usbd.todo("sys_usbd_open_default_pipe()");
	return CELL_OK;
}

s32 sys_usbd_close_pipe()
{
	sys_usbd.todo("sys_usbd_close_pipe()");
	return CELL_OK;
}

s32 sys_usbd_receive_event()
{
	sys_usbd.todo("sys_usbd_receive_event()");
	return CELL_OK;
}

s32 sys_usbd_detect_event()
{
	sys_usbd.todo("sys_usbd_detect_event()");
	return CELL_OK;
}

s32 sys_usbd_attach()
{
	sys_usbd.todo("sys_usbd_attach()");
	return CELL_OK;
}

s32 sys_usbd_transfer_data()
{
	sys_usbd.todo("sys_usbd_transfer_data()");
	return CELL_OK;
}

s32 sys_usbd_isochronous_transfer_data()
{
	sys_usbd.todo("sys_usbd_isochronous_transfer_data()");
	return CELL_OK;
}

s32 sys_usbd_get_transfer_status()
{
	sys_usbd.todo("sys_usbd_get_transfer_status()");
	return CELL_OK;
}

s32 sys_usbd_get_isochronous_transfer_status()
{
	sys_usbd.todo("sys_usbd_get_isochronous_transfer_status()");
	return CELL_OK;
}

s32 sys_usbd_get_device_location()
{
	sys_usbd.todo("sys_usbd_get_device_location()");
	return CELL_OK;
}

s32 sys_usbd_send_event()
{
	sys_usbd.todo("sys_usbd_send_event()");
	return CELL_OK;
}

s32 sys_usbd_allocate_memory()
{
	sys_usbd.todo("sys_usbd_allocate_memory()");
	return CELL_OK;
}

s32 sys_usbd_free_memory()
{
	sys_usbd.todo("sys_usbd_free_memory()");
	return CELL_OK;
}

s32 sys_usbd_get_device_speed()
{
	sys_usbd.todo("sys_usbd_get_device_speed()");
	return CELL_OK;
}

s32 sys_usbd_register_extra_ldd()
{
	sys_usbd.todo("sys_usbd_register_extra_ldd()");
	return CELL_OK;
}
