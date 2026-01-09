#include "stdafx.h"
#include "Emu/system_config.h"
#include "Emu/IdManager.h"
#include "Emu/Io/usb_microphone.h"
#include "Emu/Cell/lv2/sys_usbd.h"
#include "Emu/Cell/Modules/cellMic.h"
#include "Input/pad_thread.h"

LOG_CHANNEL(usb_mic_log);

usb_device_mic::usb_device_mic(u32 controller_index, const std::array<u8, 7>& location, MicType mic_type)
	: usb_device_emulated(location)
	, m_controller_index(controller_index)
	, m_mic_type(mic_type)
	, m_sample_rate(8000)
	, m_volume(0, 0)
{
	switch (mic_type)
	{
	case MicType::SingStar:
	{
		device = UsbDescriptorNode(USB_DESCRIPTOR_DEVICE,
			UsbDeviceDescriptor {
				.bcdUSB             = 0x0110,
				.bDeviceClass       = 0x00,
				.bDeviceSubClass    = 0x00,
				.bDeviceProtocol    = 0x00,
				.bMaxPacketSize0    = 0x08,
				.idVendor           = 0x1415,
				.idProduct          = 0x0000,
				.bcdDevice          = 0x0001,
				.iManufacturer      = 0x01,
				.iProduct           = 0x02,
				.iSerialNumber      = 0x00,
				.bNumConfigurations = 0x01});
		auto& config0 = device.add_node(UsbDescriptorNode(USB_DESCRIPTOR_CONFIG,
			UsbDeviceConfiguration {
				.wTotalLength        = 0x00b1,
				.bNumInterfaces      = 0x02,
				.bConfigurationValue = 0x01,
				.iConfiguration      = 0x00,
				.bmAttributes        = 0x80,
				.bMaxPower           = 0x2d}));
		config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_INTERFACE,
			UsbDeviceInterface {
				.bInterfaceNumber   = 0x00,
				.bAlternateSetting  = 0x00,
				.bNumEndpoints      = 0x00,
				.bInterfaceClass    = 0x01,
				.bInterfaceSubClass = 0x01,
				.bInterfaceProtocol = 0x00,
				.iInterface         = 0x00}));

		static constexpr u8 audio_if0_header[] = {
			0x09,        // bLength
			0x24,        // bDescriptorType (See Next Line)
			0x01,        // bDescriptorSubtype (CS_INTERFACE -> HEADER)
			0x00, 0x01,  // bcdADC 1.00
			0x28, 0x00,  // wTotalLength 36
			0x01,        // binCollection 0x01
			0x01,        // baInterfaceNr 1
		};
		config0.add_node(UsbDescriptorNode(audio_if0_header[0], audio_if0_header[1], &audio_if0_header[2]));

		config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ACI,
			UsbAudioInputTerminal {
				.bDescriptorSubtype = 0x02, // INPUT_TERMINAL
				.bTerminalID        = 0x01,
				.wTerminalType      = 0x0201, // Microphone
				.bAssocTerminal     = 0x02,
				.bNrChannels        = 0x02,
				.wChannelConfig     = 0x0003,
				.iChannelNames      = 0x00,
				.iTerminal          = 0x00}));
		config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ACI,
			UsbAudioOutputTerminal {
				.bDescriptorSubtype = 0x03, // OUTPUT_TERMINAL
				.bTerminalID        = 0x02,
				.wTerminalType      = 0x0101, // USB Streaming
				.bAssocTerminal     = 0x01,
				.bSourceID          = 0x03,
				.iTerminal          = 0x00}));

		static constexpr u8 audio_if0_feature[] = {
			0x0A,        // bLength
			0x24,        // bDescriptorType (See Next Line)
			0x06,        // bDescriptorSubtype (CS_INTERFACE -> FEATURE_UNIT)
			0x03,        // bUnitID
			0x01,        // bSourceID
			0x01,        // bControlSize 1
			0x01, 0x02,  // bmaControls[0] (Mute,Volume)
			0x02, 0x00,  // bmaControls[1] (Volume,None)
		};
		config0.add_node(UsbDescriptorNode(audio_if0_feature[0], audio_if0_feature[1], &audio_if0_feature[2]));

		config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_INTERFACE,
			UsbDeviceInterface {
				.bInterfaceNumber   = 0x01,
				.bAlternateSetting  = 0x00,
				.bNumEndpoints      = 0x00,
				.bInterfaceClass    = 0x01,
				.bInterfaceSubClass = 0x02,
				.bInterfaceProtocol = 0x00,
				.iInterface         = 0x00}));
		config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_INTERFACE,
			UsbDeviceInterface {
				.bInterfaceNumber   = 0x01,
				.bAlternateSetting  = 0x01,
				.bNumEndpoints      = 0x01,
				.bInterfaceClass    = 0x01,
				.bInterfaceSubClass = 0x02,
				.bInterfaceProtocol = 0x00,
				.iInterface         = 0x00}));
		config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ACI,
			UsbAudioInterface {
				.bDescriptorSubtype = 0x01, // AS_GENERAL
				.bTerminalLink      = 0x02,
				.bDelay             = 0x01,
				.wFormatTag         = 0x0001}));

		static constexpr u8 audio_if1_alt1_type[] = {
			0x17,        // bLength
			0x24,        // bDescriptorType (See Next Line)
			0x02,        // bDescriptorSubtype (CS_INTERFACE -> FORMAT_TYPE)
			0x01,        // bFormatType 1
			0x01,        // bNrChannels (Mono)
			0x02,        // bSubFrameSize 2
			0x10,        // bBitResolution 16
			0x05,        // bSamFreqType 5
			0x40, 0x1F, 0x00,  // tSamFreq[1]  8000 Hz
			0x11, 0x2B, 0x00,  // tSamFreq[2] 11025 Hz
			0x22, 0x56, 0x00,  // tSamFreq[3] 22050 Hz
			0x44, 0xAC, 0x00,  // tSamFreq[4] 44100 Hz
			0x80, 0xBB, 0x00,  // tSamFreq[5] 48000 Hz
		};
		config0.add_node(UsbDescriptorNode(audio_if1_alt1_type[0], audio_if1_alt1_type[1], &audio_if1_alt1_type[2]));

		config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ENDPOINT,
			UsbAudioEndpoint {
				.bEndpointAddress = 0x81,
				.bmAttributes     = 0x05,
				.wMaxPacketSize   = 0x0064,
				.bInterval        = 0x01,
				.bRefresh         = 0x00,
				.bSynchAddress    = 0x00}));
		config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ENDPOINT_ASI,
			UsbAudioStreamingEndpoint {
				.bDescriptorSubtype = 0x01, // EP_GENERAL
				.bmAttributes       = 0x01, // Sampling Freq Control
				.bLockDelayUnits    = 0x00,
				.wLockDelay         = 0x0000}));
		config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_INTERFACE,
			UsbDeviceInterface {
				.bInterfaceNumber   = 0x01,
				.bAlternateSetting  = 0x02,
				.bNumEndpoints      = 0x01,
				.bInterfaceClass    = 0x01,
				.bInterfaceSubClass = 0x02,
				.bInterfaceProtocol = 0x00,
				.iInterface         = 0x00}));
		config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ACI,
			UsbAudioInterface {
				.bDescriptorSubtype = 0x01, // AS_GENERAL
				.bTerminalLink      = 0x02,
				.bDelay             = 0x01,
				.wFormatTag         = 0x0001}));

		static constexpr u8 audio_if1_alt2_type[] = {
			0x17,        // bLength
			0x24,        // bDescriptorType (See Next Line)
			0x02,        // bDescriptorSubtype (CS_INTERFACE -> FORMAT_TYPE)
			0x01,        // bFormatType 1
			0x02,        // bNrChannels (Stereo)
			0x02,        // bSubFrameSize 2
			0x10,        // bBitResolution 16
			0x05,        // bSamFreqType 5
			0x40, 0x1F, 0x00,  // tSamFreq[1]  8000 Hz
			0x11, 0x2B, 0x00,  // tSamFreq[2] 11025 Hz
			0x22, 0x56, 0x00,  // tSamFreq[3] 22050 Hz
			0x44, 0xAC, 0x00,  // tSamFreq[4] 44100 Hz
			0x80, 0xBB, 0x00,  // tSamFreq[5] 48000 Hz
		};
		config0.add_node(UsbDescriptorNode(audio_if1_alt2_type[0], audio_if1_alt2_type[1], &audio_if1_alt2_type[2]));

		config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ENDPOINT,
			UsbAudioEndpoint {
				.bEndpointAddress = 0x81,
				.bmAttributes     = 0x05,
				.wMaxPacketSize   = 0x00c8,
				.bInterval        = 0x01,
				.bRefresh         = 0x00,
				.bSynchAddress    = 0x00}));
		config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ENDPOINT_ASI,
			UsbAudioStreamingEndpoint {
				.bDescriptorSubtype = 0x01, // EP_GENERAL
				.bmAttributes       = 0x01, // Sampling Freq Control
				.bLockDelayUnits    = 0x00,
				.wLockDelay         = 0x0000}));

		add_string("Nam Tai E&E Products Ltd.");
		add_string("USBMIC Serial# 012345678");
		break;
	}
	case MicType::Logitech:
	{
		device = UsbDescriptorNode(USB_DESCRIPTOR_DEVICE,
			UsbDeviceDescriptor {
				.bcdUSB             = 0x0200,
				.bDeviceClass       = 0x00,
				.bDeviceSubClass    = 0x00,
				.bDeviceProtocol    = 0x00,
				.bMaxPacketSize0    = 0x08,
				.idVendor           = 0x046d,
				.idProduct          = 0x0a03,
				.bcdDevice          = 0x0102,
				.iManufacturer      = 0x01,
				.iProduct           = 0x02,
				.iSerialNumber      = 0x00,
				.bNumConfigurations = 0x01});
		auto& config0 = device.add_node(UsbDescriptorNode(USB_DESCRIPTOR_CONFIG,
			UsbDeviceConfiguration {
				.wTotalLength        = 0x0079,
				.bNumInterfaces      = 0x02,
				.bConfigurationValue = 0x01,
				.iConfiguration      = 0x03,
				.bmAttributes        = 0x80,
				.bMaxPower           = 0x1e}));
		config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_INTERFACE,
			UsbDeviceInterface {
				.bInterfaceNumber   = 0x00,
				.bAlternateSetting  = 0x00,
				.bNumEndpoints      = 0x00,
				.bInterfaceClass    = 0x01,
				.bInterfaceSubClass = 0x01,
				.bInterfaceProtocol = 0x00,
				.iInterface         = 0x00}));

		static constexpr u8 audio_if0_header[] = {
			0x09,        // bLength
			0x24,        // bDescriptorType (See Next Line)
			0x01,        // bDescriptorSubtype (CS_INTERFACE -> HEADER)
			0x00, 0x01,  // bcdADC 1.00
			0x27, 0x00,  // wTotalLength 39
			0x01,        // binCollection 0x01
			0x01,        // baInterfaceNr 1
		};
		config0.add_node(UsbDescriptorNode(audio_if0_header[0], audio_if0_header[1], &audio_if0_header[2]));

		config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ACI,
			UsbAudioInputTerminal {
				.bDescriptorSubtype = 0x02, // INPUT_TERMINAL
				.bTerminalID        = 0x0d,
				.wTerminalType      = 0x0201, // Microphone
				.bAssocTerminal     = 0x00,
				.bNrChannels        = 0x01,
				.wChannelConfig     = 0x0000,
				.iChannelNames      = 0x00,
				.iTerminal          = 0x00}));
		static constexpr u8 audio_if0_feature[] = {
			0x09,        // bLength
			0x24,        // bDescriptorType (See Next Line)
			0x06,        // bDescriptorSubtype (CS_INTERFACE -> FEATURE_UNIT)
			0x02,        // bUnitID
			0x0d,        // bSourceID
			0x01,        // bControlSize 1
			0x03,        // bmaControls[0] (Mute,Volume)
			0x00,        // bmaControls[1] (None)
			0x00,        // iFeature
		};
		config0.add_node(UsbDescriptorNode(audio_if0_feature[0], audio_if0_feature[1], &audio_if0_feature[2]));

		config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ACI,
			UsbAudioOutputTerminal {
				.bDescriptorSubtype = 0x03, // OUTPUT_TERMINAL
				.bTerminalID        = 0x0a,
				.wTerminalType      = 0x0101, // USB Streaming
				.bAssocTerminal     = 0x00,
				.bSourceID          = 0x02,
				.iTerminal          = 0x00}));
		config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_INTERFACE,
			UsbDeviceInterface {
				.bInterfaceNumber   = 0x01,
				.bAlternateSetting  = 0x00,
				.bNumEndpoints      = 0x00,
				.bInterfaceClass    = 0x01,
				.bInterfaceSubClass = 0x02,
				.bInterfaceProtocol = 0x00,
				.iInterface         = 0x00}));
		config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_INTERFACE,
			UsbDeviceInterface {
				.bInterfaceNumber   = 0x01,
				.bAlternateSetting  = 0x01,
				.bNumEndpoints      = 0x01,
				.bInterfaceClass    = 0x01,
				.bInterfaceSubClass = 0x02,
				.bInterfaceProtocol = 0x00,
				.iInterface         = 0x00}));
		config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ACI,
			UsbAudioInterface {
				.bDescriptorSubtype = 0x01, // AS_GENERAL
				.bTerminalLink      = 0x0a,
				.bDelay             = 0x00,
				.wFormatTag         = 0x0001}));

		static constexpr u8 audio_if1_alt1_type[] = {
			0x17,        // bLength
			0x24,        // bDescriptorType (See Next Line)
			0x02,        // bDescriptorSubtype (CS_INTERFACE -> FORMAT_TYPE)
			0x01,        // bFormatType 1
			0x01,        // bNrChannels (Mono)
			0x02,        // bSubFrameSize 2
			0x10,        // bBitResolution 16
			0x05,        // bSamFreqType 5
			0x40, 0x1F, 0x00,  // tSamFreq[1]  8000 Hz
			0x11, 0x2B, 0x00,  // tSamFreq[2] 11025 Hz
			0x22, 0x56, 0x00,  // tSamFreq[4] 22050 Hz
			0x44, 0xAC, 0x00,  // tSamFreq[6] 44100 Hz
			0x80, 0xBB, 0x00,  // tSamFreq[7] 48000 Hz
		};
		config0.add_node(UsbDescriptorNode(audio_if1_alt1_type[0], audio_if1_alt1_type[1], &audio_if1_alt1_type[2]));

		config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ENDPOINT,
			UsbAudioEndpoint {
				.bEndpointAddress = 0x84,
				.bmAttributes     = 0x0d,
				.wMaxPacketSize   = 0x0060,
				.bInterval        = 0x01,
				.bRefresh         = 0x00,
				.bSynchAddress    = 0x00}));
		config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ENDPOINT_ASI,
			UsbAudioStreamingEndpoint {
				.bDescriptorSubtype = 0x01, // EP_GENERAL
				.bmAttributes       = 0x01, // Sampling Freq Control
				.bLockDelayUnits    = 0x02,
				.wLockDelay         = 0x0001}));

		add_string("Logitech");
		add_string("Logitech USB Microphone");
		break;
	}
	case MicType::Rocksmith:
	{
		device = UsbDescriptorNode(USB_DESCRIPTOR_DEVICE,
			UsbDeviceDescriptor {
				.bcdUSB             = 0x0110,
				.bDeviceClass       = 0x00,
				.bDeviceSubClass    = 0x00,
				.bDeviceProtocol    = 0x00,
				.bMaxPacketSize0    = 0x10,
				.idVendor           = 0x12ba,
				.idProduct          = 0x00ff,
				.bcdDevice          = 0x0100,
				.iManufacturer      = 0x01,
				.iProduct           = 0x02,
				.iSerialNumber      = 0x00,
				.bNumConfigurations = 0x01});
		auto& config0 = device.add_node(UsbDescriptorNode(USB_DESCRIPTOR_CONFIG,
			UsbDeviceConfiguration {
				.wTotalLength        = 0x0098,
				.bNumInterfaces      = 0x03,
				.bConfigurationValue = 0x01,
				.iConfiguration      = 0x00,
				.bmAttributes        = 0x80,
				.bMaxPower           = 0x32}));
		config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_INTERFACE,
			UsbDeviceInterface {
				.bInterfaceNumber   = 0x00,
				.bAlternateSetting  = 0x00,
				.bNumEndpoints      = 0x00,
				.bInterfaceClass    = 0x01,
				.bInterfaceSubClass = 0x01,
				.bInterfaceProtocol = 0x00,
				.iInterface         = 0x00}));

		static constexpr u8 audio_if0_header[] = {
			0x09,        // bLength
			0x24,        // bDescriptorType (See Next Line)
			0x01,        // bDescriptorSubtype (CS_INTERFACE -> HEADER)
			0x00, 0x01,  // bcdADC 1.00
			0x27, 0x00,  // wTotalLength 39
			0x01,        // binCollection 0x01
			0x01,        // baInterfaceNr 1
		};
		config0.add_node(UsbDescriptorNode(audio_if0_header[0], audio_if0_header[1], &audio_if0_header[2]));

		config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ACI,
			UsbAudioInputTerminal {
				.bDescriptorSubtype = 0x02, // INPUT_TERMINAL
				.bTerminalID        = 0x02,
				.wTerminalType      = 0x0201, // Microphone
				.bAssocTerminal     = 0x00,
				.bNrChannels        = 0x01,
				.wChannelConfig     = 0x0001,
				.iChannelNames      = 0x00,
				.iTerminal          = 0x00}));
		config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ACI,
			UsbAudioOutputTerminal {
				.bDescriptorSubtype = 0x03, // OUTPUT_TERMINAL
				.bTerminalID        = 0x07,
				.wTerminalType      = 0x0101, // USB Streaming
				.bAssocTerminal     = 0x00,
				.bSourceID          = 0x0a,
				.iTerminal          = 0x00}));

		static constexpr u8 audio_if0_feature[] = {
			0x09,        // bLength
			0x24,        // bDescriptorType (See Next Line)
			0x06,        // bDescriptorSubtype (CS_INTERFACE -> FEATURE_UNIT)
			0x0a,        // bUnitID
			0x02,        // bSourceID
			0x01,        // bControlSize 1
			0x03,        // bmaControls[0] (Mute,Volume)
			0x00,        // bmaControls[1] (None)
			0x00,        // iFeature
		};
		config0.add_node(UsbDescriptorNode(audio_if0_feature[0], audio_if0_feature[1], &audio_if0_feature[2]));

		config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_INTERFACE,
			UsbDeviceInterface {
				.bInterfaceNumber   = 0x01,
				.bAlternateSetting  = 0x00,
				.bNumEndpoints      = 0x00,
				.bInterfaceClass    = 0x01,
				.bInterfaceSubClass = 0x02,
				.bInterfaceProtocol = 0x00,
				.iInterface         = 0x00}));
		config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_INTERFACE,
			UsbDeviceInterface {
				.bInterfaceNumber   = 0x01,
				.bAlternateSetting  = 0x01,
				.bNumEndpoints      = 0x01,
				.bInterfaceClass    = 0x01,
				.bInterfaceSubClass = 0x02,
				.bInterfaceProtocol = 0x00,
				.iInterface         = 0x00}));
		config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ACI,
			UsbAudioInterface {
				.bDescriptorSubtype = 0x01, // AS_GENERAL
				.bTerminalLink      = 0x07,
				.bDelay             = 0x01,
				.wFormatTag         = 0x0001}));

		static constexpr u8 audio_if1_alt1_type[] = {
			0x1d,        // bLength
			0x24,        // bDescriptorType (See Next Line)
			0x02,        // bDescriptorSubtype (CS_INTERFACE -> FORMAT_TYPE)
			0x01,        // bFormatType 1
			0x01,        // bNrChannels (Mono)
			0x02,        // bSubFrameSize 2
			0x10,        // bBitResolution 16
			0x07,        // bSamFreqType 5
			0x40, 0x1F, 0x00,  // tSamFreq[1]  8000 Hz
			0x11, 0x2B, 0x00,  // tSamFreq[2] 11025 Hz
			0x80, 0x3e, 0x00,  // tSamFreq[3] 16000 Hz
			0x22, 0x56, 0x00,  // tSamFreq[4] 22050 Hz
			0x00, 0x7d, 0x00,  // tSamFreq[5] 32000 Hz
			0x44, 0xAC, 0x00,  // tSamFreq[6] 44100 Hz
			0x80, 0xBB, 0x00,  // tSamFreq[7] 48000 Hz
		};
		config0.add_node(UsbDescriptorNode(audio_if1_alt1_type[0], audio_if1_alt1_type[1], &audio_if1_alt1_type[2]));

		config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ENDPOINT,
			UsbAudioEndpoint {
				.bEndpointAddress = 0x82,
				.bmAttributes     = 0x0d,
				.wMaxPacketSize   = 0x0064,
				.bInterval        = 0x01,
				.bRefresh         = 0x00,
				.bSynchAddress    = 0x00}));
		config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ENDPOINT_ASI,
			UsbAudioStreamingEndpoint {
				.bDescriptorSubtype = 0x01, // EP_GENERAL
				.bmAttributes       = 0x01, // Sampling Freq Control
				.bLockDelayUnits    = 0x00,
				.wLockDelay         = 0x0000}));
		config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_INTERFACE,
			UsbDeviceInterface {
				.bInterfaceNumber   = 0x02,
				.bAlternateSetting  = 0x00,
				.bNumEndpoints      = 0x01,
				.bInterfaceClass    = 0x03,
				.bInterfaceSubClass = 0x00,
				.bInterfaceProtocol = 0x00,
				.iInterface         = 0x00}));
		config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_HID,
			UsbDeviceHID {
				.bcdHID            = 0x0111,
				.bCountryCode      = 0x00,
				.bNumDescriptors   = 0x01,
				.bDescriptorType   = 0x22,
				.wDescriptorLength = 0x001a}));
		config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ENDPOINT,
			UsbDeviceEndpoint {
				.bEndpointAddress = 0x87,
				.bmAttributes     = 0x03,
				.wMaxPacketSize   = 0x0010,
				.bInterval        = 0x01}));

		add_string("Hercules.");
		add_string("Rocksmith USB Guitar Adapter");
		break;
	}
	}
}

std::shared_ptr<usb_device> usb_device_mic::make_singstar(u32 controller_index, const std::array<u8, 7>& location)
{
	return std::make_shared<usb_device_mic>(controller_index, location, MicType::SingStar);
}

std::shared_ptr<usb_device> usb_device_mic::make_logitech(u32 controller_index, const std::array<u8, 7>& location)
{
	return std::make_shared<usb_device_mic>(controller_index, location, MicType::Logitech);
}

std::shared_ptr<usb_device> usb_device_mic::make_rocksmith(u32 controller_index, const std::array<u8, 7>& location)
{
	return std::make_shared<usb_device_mic>(controller_index, location, MicType::Rocksmith);
}

u16 usb_device_mic::get_num_emu_devices()
{
	return 1;
}

void usb_device_mic::control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength, u32 buf_size, u8* buf, UsbTransfer* transfer)
{
	transfer->fake            = true;
	transfer->expected_count  = buf_size;
	transfer->expected_result = HC_CC_NOERR;
	transfer->expected_time   = get_timestamp() + 1000;

	switch (bmRequestType)
	{
	case 0U /*silences warning*/ | LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE: // 0x21
		switch (bRequest)
		{
		case SET_CUR:
		{
			ensure(buf_size >= 2);
			const u8 ch = wValue & 0xff;
			if (ch == 0)
			{
				m_volume[0] = (buf[1] << 8) | buf[0];
				m_volume[1] = (buf[1] << 8) | buf[0];
				usb_mic_log.notice("Set Cur Volume[%d]: 0x%04x (%d dB)", ch, m_volume[0], m_volume[0] / 256);
			}
			else if (ch == 1)
			{
				m_volume[0] = (buf[1] << 8) | buf[0];
				usb_mic_log.notice("Set Cur Volume[%d]: 0x%04x (%d dB)", ch, m_volume[0], m_volume[0] / 256);
			}
			else if (ch == 2)
			{
				m_volume[1] = (buf[1] << 8) | buf[0];
				usb_mic_log.notice("Set Cur Volume[%d]: 0x%04x (%d dB)", ch, m_volume[1], m_volume[1] / 256);
			}
			break;
		}
		default:
			usb_mic_log.error("Unhandled Request: 0x%02X/0x%02X", bmRequestType, bRequest);
			break;
		}
		break;
	case 0U /*silences warning*/ | LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_ENDPOINT: // 0x22
		switch (bRequest)
		{
		case SET_CUR:
			ensure(buf_size >= 3);
			m_sample_rate = (buf[2] << 16) | (buf[1] << 8) | buf[0];
			usb_mic_log.notice("Set Sample Rate: %d", m_sample_rate);
			break;
		default:
			usb_mic_log.error("Unhandled Request: 0x%02X/0x%02X", bmRequestType, bRequest);
			break;
		}
		break;
	case 0U /*silences warning*/ | LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE: // 0xa1
		switch (bRequest)
		{
		case GET_CUR:
		{
			ensure(buf_size >= 2);
			const u8 ch = wValue & 0xff;
			if (ch == 2)
			{
				buf[0] = (m_volume[1]     ) & 0xff;
				buf[1] = (m_volume[1] >> 8) & 0xff;
				usb_mic_log.notice("Get Cur Volume[%d]: 0x%04x (%d dB)", ch, m_volume[1], m_volume[1] / 256);
			}
			else
			{
				buf[0] = (m_volume[0]     ) & 0xff;
				buf[1] = (m_volume[0] >> 8) & 0xff;
				usb_mic_log.notice("Get Cur Volume[%d]: 0x%04x (%d dB)", ch, m_volume[0], m_volume[0] / 256);
			}
			break;
		}
		case GET_MIN:
		{
			ensure(buf_size >= 2);
			constexpr s16 minVol = 0xff00;
			buf[0] = (minVol     ) & 0xff;
			buf[1] = (minVol >> 8) & 0xff;
			usb_mic_log.notice("Get Min Volume: 0x%04x (%d dB)", minVol, minVol / 256);
			break;
		}
		case GET_MAX:
		{
			ensure(buf_size >= 2);
			constexpr s16 maxVol = 0x0100;
			buf[0] = (maxVol     ) & 0xff;
			buf[1] = (maxVol >> 8) & 0xff;
			usb_mic_log.notice("Get Max Volume: 0x%04x (%d dB)", maxVol, maxVol / 256);
			break;
		}
		default:
			usb_mic_log.error("Unhandled Request: 0x%02X/0x%02X", bmRequestType, bRequest);
			break;
		}
		break;
	case 0U /*silences warning*/ | LIBUSB_ENDPOINT_IN | LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_ENDPOINT: // 0xa2
		switch (bRequest)
		{
		case GET_CUR:
			ensure(buf_size >= 3);
			buf[0] = (m_sample_rate      ) & 0xff;
			buf[1] = (m_sample_rate >>  8) & 0xff;
			buf[2] = (m_sample_rate >> 16) & 0xff;
			usb_mic_log.notice("Get Sample Rate: %d", m_sample_rate);
			break;
		default:
			usb_mic_log.error("Unhandled Request: 0x%02X/0x%02X", bmRequestType, bRequest);
			break;
		}
		break;
	default:
		usb_device_emulated::control_transfer(bmRequestType, bRequest, wValue, wIndex, wLength, buf_size, buf, transfer);
		break;
	}

	usb_mic_log.trace("control_transfer: req=0x%02X/0x%02X, val=0x%02x, idx=0x%02x, len=0x%02x, [%s]",
		bmRequestType, bRequest, wValue, wIndex, wLength, fmt::buf_to_hexstring(buf, buf_size));
}

void usb_device_mic::isochronous_transfer(UsbTransfer* transfer)
{
	transfer->fake = true;
	transfer->expected_count = 0;
	transfer->expected_result = HC_CC_NOERR;
	transfer->expected_time = get_timestamp() + 1000;

	const bool stereo = (m_mic_type == MicType::SingStar && current_altsetting == 2);

	auto& mic_thr = g_fxo->get<mic_thread>();
	const std::lock_guard lock(mic_thr.mutex);
	if (!mic_thr.init)
	{
	    usb_mic_log.notice("mic init");
		mic_thr.load_config_and_init();
		mic_thr.init = 1;
	}
	if (!mic_thr.check_device(0))
	{
	    usb_mic_log.notice("mic check");
	}
	microphone_device& device = ::at32(mic_thr.mic_list, 0);
	if (!device.is_opened())
	{
	    usb_mic_log.notice("mic open");
		device.open_microphone(CELLMIC_SIGTYPE_RAW, m_sample_rate, m_sample_rate, stereo ? 2 : 1);
	}
	if (!device.is_started())
	{
	    usb_mic_log.notice("mic start");
		device.start_microphone();
	}

	u8* buf = static_cast<u8*>(transfer->iso_request.buf.get_ptr());
	for (u32 index = 0; index < transfer->iso_request.num_packets; index++)
	{
		const u16 inlen = transfer->iso_request.packets[index] >> 4;
		ensure(inlen >= (stereo ? 192 : 96));
		const u32 outlen = device.read_raw(buf, stereo ? 192 : 96);
		buf += outlen;
		transfer->iso_request.packets[index] = (outlen & 0xFFF) << 4;
		usb_mic_log.trace("    isochronous_transfer: dev=%d, buf=0x%x, start=0x%x, pks=0x%x idx=0x%x, inlen=0x%x, outlen=0x%x",
			static_cast<u8>(m_mic_type), transfer->iso_request.buf, transfer->iso_request.start_frame, transfer->iso_request.num_packets, index, inlen, outlen);
	}
}
