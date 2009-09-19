// Note:  From libusb.

#ifndef __USB_H__
#define __USB_H__

/*
 * 'interface' is defined somewhere in the Windows header files. This macro
 * is deleted here to avoid conflicts and compile errors.
 */

#ifdef interface
#undef interface
#endif

/*
 * PATH_MAX from limits.h can't be used on Windows if the dll and
 * import libraries are build/used by different compilers
 */

#define LIBUSB_PATH_MAX 512


/*
 * USB spec information
 *
 * This is all stuff grabbed from various USB specs and is pretty much
 * not subject to change
 */

/*
 * Device and/or Interface Class codes
 */
#define USB_CLASS_PER_INTERFACE		0	/* for DeviceClass */
#define USB_CLASS_AUDIO			      1
#define USB_CLASS_COMM			      2
#define USB_CLASS_HID			        3
#define USB_CLASS_PRINTER		      7
#define USB_CLASS_MASS_STORAGE		8
#define USB_CLASS_HUB			        9
#define USB_CLASS_DATA			      10
#define USB_CLASS_VENDOR_SPEC		  0xff

/*
 * Descriptor types
 */
#define USB_DT_DEVICE			0x01
#define USB_DT_CONFIG			0x02
#define USB_DT_STRING			0x03
#define USB_DT_INTERFACE	0x04
#define USB_DT_ENDPOINT		0x05

#define USB_DT_HID			0x21
#define USB_DT_REPORT		0x22
#define USB_DT_PHYSICAL	0x23
#define USB_DT_HUB			0x29

/*
 * Descriptor sizes per descriptor type
 */
#define USB_DT_DEVICE_SIZE		18
#define USB_DT_CONFIG_SIZE		9
#define USB_DT_INTERFACE_SIZE		9
#define USB_DT_ENDPOINT_SIZE		7
#define USB_DT_ENDPOINT_AUDIO_SIZE	9	/* Audio extension */
#define USB_DT_HUB_NONVAR_SIZE		7


/* ensure byte-packed structures */
#include <pshpack1.h>


/* All standard descriptors have these 2 fields in common */
struct usb_descriptor_header {
  unsigned char  bLength;
  unsigned char  bDescriptorType;
};

/* String descriptor */
struct usb_string_descriptor {
  unsigned char  bLength;
  unsigned char  bDescriptorType;
  unsigned short wData[1];
};

/* HID descriptor */
struct usb_hid_descriptor {
  unsigned char  bLength;
  unsigned char  bDescriptorType;
  unsigned short bcdHID;
  unsigned char  bCountryCode;
  unsigned char  bNumDescriptors;
};

/* Endpoint descriptor */
#define USB_MAXENDPOINTS	32
struct usb_endpoint_descriptor {
  unsigned char  bLength;
  unsigned char  bDescriptorType;
  unsigned char  bEndpointAddress;
  unsigned char  bmAttributes;
  unsigned short wMaxPacketSize;
  unsigned char  bInterval;
  unsigned char  bRefresh;
  unsigned char  bSynchAddress;

  unsigned char *extra;	/* Extra descriptors */
  int extralen;
};

#define USB_ENDPOINT_ADDRESS_MASK	0x0f    /* in bEndpointAddress */
#define USB_ENDPOINT_DIR_MASK		  0x80

#define USB_ENDPOINT_TYPE_MASK		0x03    /* in bmAttributes */
#define USB_ENDPOINT_TYPE_CONTROL	    0
#define USB_ENDPOINT_TYPE_ISOCHRONOUS	1
#define USB_ENDPOINT_TYPE_BULK		    2
#define USB_ENDPOINT_TYPE_INTERRUPT	  3

/* Interface descriptor */
#define USB_MAXINTERFACES	32
struct usb_interface_descriptor {
  unsigned char  bLength;
  unsigned char  bDescriptorType;
  unsigned char  bInterfaceNumber;
  unsigned char  bAlternateSetting;
  unsigned char  bNumEndpoints;
  unsigned char  bInterfaceClass;
  unsigned char  bInterfaceSubClass;
  unsigned char  bInterfaceProtocol;
  unsigned char  iInterface;

  struct usb_endpoint_descriptor *endpoint;

  unsigned char *extra;	/* Extra descriptors */
  int extralen;
};

#define USB_MAXALTSETTING	128	/* Hard limit */

struct usb_interface {
  struct usb_interface_descriptor *altsetting;

  int num_altsetting;
};

/* Configuration descriptor information.. */
#define USB_MAXCONFIG		8
struct usb_config_descriptor {
  unsigned char  bLength;
  unsigned char  bDescriptorType;
  unsigned short wTotalLength;
  unsigned char  bNumInterfaces;
  unsigned char  bConfigurationValue;
  unsigned char  iConfiguration;
  unsigned char  bmAttributes;
  unsigned char  MaxPower;

  struct usb_interface *interface;

  unsigned char *extra;	/* Extra descriptors */
  int extralen;
};

/* Device descriptor */
struct usb_device_descriptor {
  unsigned char  bLength;
  unsigned char  bDescriptorType;
  unsigned short bcdUSB;
  unsigned char  bDeviceClass;
  unsigned char  bDeviceSubClass;
  unsigned char  bDeviceProtocol;
  unsigned char  bMaxPacketSize0;
  unsigned short idVendor;
  unsigned short idProduct;
  unsigned short bcdDevice;
  unsigned char  iManufacturer;
  unsigned char  iProduct;
  unsigned char  iSerialNumber;
  unsigned char  bNumConfigurations;
};

struct usb_ctrl_setup {
  unsigned char  bRequestType;
  unsigned char  bRequest;
  unsigned short wValue;
  unsigned short wIndex;
  unsigned short wLength;
};

/*
 * Standard requests
 */
#define USB_REQ_GET_STATUS		    0x00
#define USB_REQ_CLEAR_FEATURE	    0x01
/* 0x02 is reserved */
#define USB_REQ_SET_FEATURE		    0x03
/* 0x04 is reserved */
#define USB_REQ_SET_ADDRESS		    0x05
#define USB_REQ_GET_DESCRIPTOR		0x06
#define USB_REQ_SET_DESCRIPTOR		0x07
#define USB_REQ_GET_CONFIGURATION	0x08
#define USB_REQ_SET_CONFIGURATION	0x09
#define USB_REQ_GET_INTERFACE		  0x0A
#define USB_REQ_SET_INTERFACE		  0x0B
#define USB_REQ_SYNCH_FRAME		    0x0C

#define USB_TYPE_STANDARD		(0x00 << 5)
#define USB_TYPE_CLASS			(0x01 << 5)
#define USB_TYPE_VENDOR			(0x02 << 5)
#define USB_TYPE_RESERVED		(0x03 << 5)

#define USB_RECIP_DEVICE		0x00
#define USB_RECIP_INTERFACE	0x01
#define USB_RECIP_ENDPOINT	0x02
#define USB_RECIP_OTHER			0x03

/*
 * Various libusb API related stuff
 */

#define USB_ENDPOINT_IN			0x80
#define USB_ENDPOINT_OUT		0x00

/* Error codes */
#define USB_ERROR_BEGIN			500000

/*
 * This is supposed to look weird. This file is generated from autoconf
 * and I didn't want to make this too complicated.
 */
#define USB_LE16_TO_CPU(x)

/* Data types */
/* struct usb_device; */
/* struct usb_bus; */

struct usb_device {
  struct usb_device *next, *prev;

  char filename[LIBUSB_PATH_MAX];

  struct usb_bus *bus;

  struct usb_device_descriptor descriptor;
  struct usb_config_descriptor *config;

  void *dev;		/* Darwin support */

  unsigned char devnum;

  unsigned char num_children;
  struct usb_device **children;
};

struct usb_bus {
  struct usb_bus *next, *prev;

  char dirname[LIBUSB_PATH_MAX];

  struct usb_device *devices;
  unsigned long location;

  struct usb_device *root_dev;
};

/* Version information, Windows specific */
struct usb_version {
  struct {
    int major;
    int minor;
    int micro;
    int nano;
  } dll;
  struct {
    int major;
    int minor;
    int micro;
    int nano;
  } driver;
};


struct usb_dev_handle;
typedef struct usb_dev_handle usb_dev_handle;

/* Variables */
#ifndef __USB_C__
#define usb_busses usb_get_busses()
#endif



#include <poppack.h>

#ifdef GOAT
#ifdef __cplusplus
extern "C" {
#endif

  /* Function prototypes */

  /* usb.c */
  usb_dev_handle *usb_open(struct usb_device *dev);
  int usb_close(usb_dev_handle *dev);
  int usb_get_string(usb_dev_handle *dev, int index, int langid, char *buf,
                     size_t buflen);
  int usb_get_string_simple(usb_dev_handle *dev, int index, char *buf,
                            size_t buflen);

  /* descriptors.c */
  int usb_get_descriptor_by_endpoint(usb_dev_handle *udev, int ep,
                                     unsigned char type, unsigned char index,
                                     void *buf, int size);
  int usb_get_descriptor(usb_dev_handle *udev, unsigned char type,
                         unsigned char index, void *buf, int size);

  /* <arch>.c */
  int usb_bulk_write(usb_dev_handle *dev, int ep, char *bytes, int size,
                     int timeout);
  int usb_bulk_read(usb_dev_handle *dev, int ep, char *bytes, int size,
                    int timeout);
  int usb_interrupt_write(usb_dev_handle *dev, int ep, char *bytes, int size,
                          int timeout);
  int usb_interrupt_read(usb_dev_handle *dev, int ep, char *bytes, int size,
                         int timeout);
  int usb_control_msg(usb_dev_handle *dev, int requesttype, int request,
                      int value, int index, char *bytes, int size,
                      int timeout);
  int usb_set_configuration(usb_dev_handle *dev, int configuration);
  int usb_claim_interface(usb_dev_handle *dev, int interface);
  int usb_release_interface(usb_dev_handle *dev, int interface);
  int usb_set_altinterface(usb_dev_handle *dev, int alternate);
  int usb_resetep(usb_dev_handle *dev, unsigned int ep);
  int usb_clear_halt(usb_dev_handle *dev, unsigned int ep);
  int usb_reset(usb_dev_handle *dev);

  char *usb_strerror(void);

  void usb_init(void);
  void usb_set_debug(int level);
  int usb_find_busses(void);
  int usb_find_devices(void);
  struct usb_device *usb_device(usb_dev_handle *dev);
  struct usb_bus *usb_get_busses(void);


  /* Windows specific functions */

  #define LIBUSB_HAS_INSTALL_SERVICE_NP 1
  int usb_install_service_np(void);
  void CALLBACK usb_install_service_np_rundll(HWND wnd, HINSTANCE instance,
                                              LPSTR cmd_line, int cmd_show);

  #define LIBUSB_HAS_UNINSTALL_SERVICE_NP 1
  int usb_uninstall_service_np(void);
  void CALLBACK usb_uninstall_service_np_rundll(HWND wnd, HINSTANCE instance,
                                                LPSTR cmd_line, int cmd_show);

  #define LIBUSB_HAS_INSTALL_DRIVER_NP 1
  int usb_install_driver_np(const char *inf_file);
  void CALLBACK usb_install_driver_np_rundll(HWND wnd, HINSTANCE instance,
                                             LPSTR cmd_line, int cmd_show);

  #define LIBUSB_HAS_TOUCH_INF_FILE_NP 1
  int usb_touch_inf_file_np(const char *inf_file);
  void CALLBACK usb_touch_inf_file_np_rundll(HWND wnd, HINSTANCE instance,
                                             LPSTR cmd_line, int cmd_show);

  #define LIBUSB_HAS_INSTALL_NEEDS_RESTART_NP 1
  int usb_install_needs_restart_np(void);

  const struct usb_version *usb_get_version(void);

  int usb_isochronous_setup_async(usb_dev_handle *dev, void **context,
                                  unsigned char ep, int pktsize);
  int usb_bulk_setup_async(usb_dev_handle *dev, void **context,
                           unsigned char ep);
  int usb_interrupt_setup_async(usb_dev_handle *dev, void **context,
                                unsigned char ep);

  int usb_submit_async(void *context, char *bytes, int size);
  int usb_reap_async(void *context, int timeout);
  int usb_reap_async_nocancel(void *context, int timeout);
  int usb_cancel_async(void *context);
  int usb_free_async(void **context);


#ifdef __cplusplus
}
#endif
#endif

#endif /* __USB_H__ */

