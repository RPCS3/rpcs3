#pragma once



// SysCalls

struct DeviceListUnknownDataType
{
	u8 unk1;
	u8 deviceID;
	u8 unk3;
	u8 unk4;
} ;

/* 0x01 device descriptor */
struct deviceDescriptor
{
	u8 bLength;              /* descriptor size in bytes */
	u8 bDescriptorType;      /* constant USB_DESCRIPTOR_TYPE_DEVICE */
	u16 bcdUSB;              /* USB spec release compliance number */
	u8 bDeviceClass;         /* class code */
	u8 bDeviceSubClass;      /* subclass code */
	u8 bDeviceProtocol;      /* protocol code */
	u8 bMaxPacketSize0;      /* max packet size for endpoint 0 */
	u16 idVendor;            /* USB-IF Vendor ID (VID) */
	u16 idProduct;           /* Product ID (PID) */
	u16 bcdDevice;           /* device release number */
	u8 iManufacturer;        /* manufacturer string descriptor index */
	u8 iProduct;             /* product string desccriptor index */
	u8 iSerialNumber;        /* serial number string descriptor index */
	u8 bNumConfigurations;   /* number of configurations */
};

struct usbDevice
{
	DeviceListUnknownDataType basicDevice;
	s32 descSize;
	deviceDescriptor descriptor;
};

s32 sys_usbd_initialize(vm::ptr<u32> handle);
s32 sys_usbd_finalize();
s32 sys_usbd_get_device_list(u32 handle, vm::ptr<DeviceListUnknownDataType> device_list, char unknown);
s32 sys_usbd_get_descriptor_size(u32 handle, u8 deviceNumber);
s32 sys_usbd_get_descriptor(u32 handle, u8 deviceNumber, vm::ptr<deviceDescriptor> descriptor, s64 descSize);
s32 sys_usbd_register_ldd();
s32 sys_usbd_unregister_ldd();
s32 sys_usbd_open_pipe(u32 handle, vm::ptr<void> endDisc);
s32 sys_usbd_open_default_pipe();
s32 sys_usbd_close_pipe();
s32 sys_usbd_receive_event(ppu_thread& ppu, u32 handle, vm::ptr<u64> arg1, vm::ptr<u64> arg2, vm::ptr<u64> arg3);
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
s32 sys_usbd_register_extra_ldd(u32 handle, vm::ptr<void> lddOps, u16 strLen, u16 vendorID, u16 productID, u16 unk1);
