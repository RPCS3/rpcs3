#pragma once

// SysCalls

s32 sys_usbd_initialize();
s32 sys_usbd_finalize();
s32 sys_usbd_get_device_list();
s32 sys_usbd_get_descriptor_size();
s32 sys_usbd_get_descriptor();
s32 sys_usbd_register_ldd();
s32 sys_usbd_unregister_ldd();
s32 sys_usbd_open_pipe();
s32 sys_usbd_open_default_pipe();
s32 sys_usbd_close_pipe();
s32 sys_usbd_receive_event();
s32 sys_usbd_detect_event();
s32 sys_usbd_attach();
s32 sys_usbd_transfer_data();
s32 sys_usbd_isochronous_transfer_data();
s32 sys_usbd_get_transfer_status();
s32 sys_usbd_get_isochronous_transfer_status();
s32 sys_usbd_get_device_location();
s32 sys_usbd_send_event();
s32 sys_usbd_allocate_memory();
s32 sys_usbd_free_memory();
s32 sys_usbd_get_device_speed();
s32 sys_usbd_register_extra_ldd();
