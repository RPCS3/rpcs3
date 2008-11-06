/*
  _____     ___ ____ 
   ____|   |    ____|      PSX2 OpenSource Project
  |     ___|   |____       (C)2002, Pedro J. Cabrera (rc5stint@yahoo.com)
  ------------------------------------------------------------------------
  iop_usbd.s		   USB Driver Import functions.

  Figured out from IRX files with symbol tables in Unreal Tournament 
  and JamPak demo discs.
*/

	.text
	.set	noreorder

	.local	usbd_stub
usbd_stub:					# Module Import Information
	.word	0x41e00000		# Import Tag
	.word	0x00000000		# Minor Version?
	.word	0x00000101		# Major Version?
	.ascii	"usbd\0\0\0\0"	# Library ID
	.align	2

	/* initialize USBD.IRX
	 * Note: UsbInit is automatically called first whenever USBD.IRX is loaded.
	 * There should never be a need to reinitialize the driver.  In fact, it may 
	 * not even work.  But I'm providing the function hook anyhow. <shrug>
	 */
	.globl	UsbInit
UsbInit:
	j	$31
	li	$0, 0x00

	/*
	 * These two functions are used to register and unregister device drivers for
	 * listening for USB bus events.  The events are device probe, connect, and disconnect.
	 */

	 # register a USB device driver
	.globl	UsbRegisterDriver
UsbRegisterDriver:
	j	$31
	li	$0, 0x04

	# unregister a USB device driver
	.globl	UsbUnregisterDriver
UsbUnregisterDriver:
	j	$31
	li	$0, 0x05

	/*
	 * This function is used to get the static descriptors for the specific USB 
	 * device.  These descriptors identify the device uniquely and help determine 
	 * what type of device we are dealing with, and what its capabilities and 
	 * features are.
	 */
	.globl	UsbGetDeviceStaticDescriptor
UsbGetDeviceStaticDescriptor:
	j	$31
	li	$0, 0x06

	/*
	 * These two functions are used to assign relevant data to a specific device.  
	 * The type of data is entirely up to the caller.  For example, a particular 
	 * USB device driver may store configuration data for each specific device 
	 * under its control.
	 */

	# set the private data pointer for a device
	.globl	UsbSetDevicePrivateData
UsbSetDevicePrivateData:
	j	$31
	li	$0, 0x07

	# get the private data pointer for a device
	.globl	UsbGetDevicePrivateData
UsbGetDevicePrivateData:
	j	$31
	li	$0, 0x08

	/* 
	 * This function returns an endpoint ID for the device ID and endpoint descriptor 
	 * passed in.  This endpoint ID is then used when transfering data to the device, 
	 * and to close the endpoint.
	 */
	.globl	UsbOpenEndpoint
UsbOpenEndpoint:
	j	$31
	li	$0, 0x09
	
	# close an endpoint
	.globl	UsbCloseEndpoint
UsbCloseEndpoint:
	j	$31
	li	$0, 0x0A

	/*
	 * This function is used for all types of USB data transfers.  Which type of 
	 * transfer is determined by the parameters that are passed in.  The types are:
	 * control, isochronous, interrupt, and bulk transfers.  More details can be 
	 * found in usbd.h.
	 */
	.globl	UsbTransfer
UsbTransfer:
	j	$31
	li	$0, 0x0B

	.globl	UsbOpenBulkEndpoint
UsbOpenBulkEndpoint:
	j	$31
	li	$0, 0x0C

	.word	0
	.word	0
