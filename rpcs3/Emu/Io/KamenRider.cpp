#include "stdafx.h"
#include "KamenRider.h"

LOG_CHANNEL(kamen_rider_log, "kamen_rider");

rider_gate g_ridergate;

void kamen_rider_figure::save()
{
	if (!kamen_file)
	{
		kamen_rider_log.error("Tried to save kamen rider figure to file but no kamen rider figure is active!");
		return;
	}
	kamen_file.seek(0, fs::seek_set);
	kamen_file.write(data.data(), 0x14 * 0x10);
}

u8 rider_gate::generate_checksum(const std::array<u8, 64>& data, u32 num_of_bytes) const
{
	ensure(num_of_bytes <= data.size());
	int checksum = 0;
	for (u32 i = 0; i < num_of_bytes; i++)
	{
		checksum += data[i];
	}
	return (checksum & 0xFF);
}

kamen_rider_figure& rider_gate::get_figure_by_uid(const std::array<u8, 7> uid)
{
	for (kamen_rider_figure& figure : figures)
	{
		if (figure.uid == uid)
		{
			return figure;
		}
	}
	return figures[7];
}

void rider_gate::get_blank_response(u8 command, u8 sequence, std::array<u8, 64>& reply_buf)
{
	reply_buf = {0x55, 0x02, command, sequence};
	reply_buf[4] = generate_checksum(reply_buf, 4);
}

void rider_gate::wake_rider_gate(std::array<u8, 64>& reply_buf, u8 command, u8 sequence)
{
	std::lock_guard lock(kamen_mutex);

	m_is_awake = true;
	reply_buf = {0x55, 0x1a, command, sequence, 0x00, 0x07, 0x00, 0x03, 0x02,
		0x09, 0x20, 0x03, 0xf5, 0x00, 0x19, 0x42, 0x52, 0xb7,
		0xb9, 0xa1, 0xae, 0x2b, 0x88, 0x42, 0x05, 0xfe, 0xe0, 0x1c, 0xac};
}

void rider_gate::get_list_tags(std::array<u8, 64>& reply_buf, u8 command, u8 sequence)
{
	std::lock_guard lock(kamen_mutex);

	reply_buf = {0x55, 0x02, command, sequence};
	u8 index = 4;
	for (const kamen_rider_figure& figure : figures)
	{
		if (figure.present)
		{
			reply_buf[index] = 0x09;
			memcpy(&reply_buf[index + 1], figure.data.data(), 7);
			index += 8;
			reply_buf[1] += 8;
		}
	}
	reply_buf[index] = generate_checksum(reply_buf, index);
}

void rider_gate::query_block(std::array<u8, 64>& reply_buf, u8 command, u8 sequence, const u8* uid, u8 sector, u8 block)
{
	std::lock_guard lock(kamen_mutex);

	reply_buf = {0x55, 0x13, command, sequence, 0x00};

	const std::array<u8, 7> uid_array = {uid[0], uid[1], uid[2], uid[3], uid[4], uid[5], uid[6]};

	const kamen_rider_figure& figure = get_figure_by_uid(uid_array);
	if (figure.present)
	{
		if (sector < 5 && block < 4)
		{
			memcpy(&reply_buf[5], &figure.data[(sector * 4 * 16) + (block * 16)], 16);
		}
	}
	reply_buf[21] = generate_checksum(reply_buf, 21);
}

void rider_gate::write_block(std::array<u8, 64>& replyBuf, u8 command, u8 sequence, const u8* uid, u8 sector, u8 block, const u8* to_write_buf)
{
	std::lock_guard lock(kamen_mutex);

	const std::array<u8, 7> uid_array = {uid[0], uid[1], uid[2], uid[3], uid[4], uid[5], uid[6]};

	kamen_rider_figure& figure = get_figure_by_uid(uid_array);
	if (figure.present)
	{
		if (sector < 5 && block < 4)
		{
			memcpy(&figure.data[(sector * 4 * 16) + (block * 16)], to_write_buf, 16);
		}
	}

	get_blank_response(command, sequence, replyBuf);
}

std::optional<std::array<u8, 64>> rider_gate::pop_added_removed_response()
{
	std::lock_guard lock(kamen_mutex);

	if (m_figure_added_removed_responses.empty())
	{
		return std::nullopt;
	}

	std::array<u8, 64> response = m_figure_added_removed_responses.front();
	m_figure_added_removed_responses.pop();
	return response;
}

bool rider_gate::remove_figure(u8 index)
{
	std::lock_guard lock(kamen_mutex);

	auto& figure = figures[index];

	if (figure.present)
	{
		figure.present = false;
		figure.save();
		figure.kamen_file.close();
		if (m_is_awake)
		{
			std::array<u8, 64> figure_removed_response = {0x56, 0x09, 0x09, 0x00};
			memcpy(&figure_removed_response[4], figure.uid.data(), figure.uid.size());
			figure_removed_response[11] = generate_checksum(figure_removed_response, 11);
			m_figure_added_removed_responses.push(std::move(figure_removed_response));
		}
		figure.uid = {};
		return true;
	}

	return false;
}

u8 rider_gate::load_figure(const std::array<u8, 0x14 * 0x10>& buf, fs::file in_file)
{
	std::lock_guard lock(kamen_mutex);

	u8 found_slot = 0xFF;

	// mimics spot retaining on the portal
	for (auto i = 0; i < 7; i++)
	{
		if (!figures[i].present)
		{
			if (i < found_slot)
			{
				found_slot = i;
			}
		}
	}

	if (found_slot != 0xFF)
	{
		auto& figure = figures[found_slot];
		memcpy(figure.data.data(), buf.data(), buf.size());
		figure.kamen_file = std::move(in_file);
		figure.uid = {buf[0], buf[1], buf[2], buf[3], buf[4], buf[5], buf[6]};
		figure.present = true;

		if (m_is_awake)
		{
			std::array<u8, 64> figure_added_response = {0x56, 0x09, 0x09, 0x01};
			memcpy(&figure_added_response[4], figure.uid.data(), figure.uid.size());
			figure_added_response[11] = generate_checksum(figure_added_response, 11);
			m_figure_added_removed_responses.push(std::move(figure_added_response));
		}
	}
	return found_slot;
}

usb_device_kamen_rider::usb_device_kamen_rider(const std::array<u8, 7>& location)
	: usb_device_emulated(location)
{
	device = UsbDescriptorNode(USB_DESCRIPTOR_DEVICE, UsbDeviceDescriptor{0x200, 0x0, 0x0, 0x0, 0x40, 0x0E6F, 0x200A, 0x100, 0x1, 0x2, 0x3, 0x1});
	auto& config0 = device.add_node(UsbDescriptorNode(USB_DESCRIPTOR_CONFIG, UsbDeviceConfiguration{0x29, 0x1, 0x1, 0x0, 0x80, 0xFA}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_INTERFACE, UsbDeviceInterface{0x0, 0x0, 0x2, 0x3, 0x0, 0x0, 0x0}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ENDPOINT, UsbDeviceEndpoint{0x81, 0x3, 0x40, 0x1}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ENDPOINT, UsbDeviceEndpoint{0x1, 0x3, 0x40, 0x1}));
}

usb_device_kamen_rider::~usb_device_kamen_rider()
{
}

std::shared_ptr<usb_device> usb_device_kamen_rider::make_instance(u32, const std::array<u8, 7>& location)
{
	return std::make_shared<usb_device_kamen_rider>(location);
}

u16 usb_device_kamen_rider::get_num_emu_devices()
{
	return 1;
}

void usb_device_kamen_rider::control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength, u32 buf_size, u8* buf, UsbTransfer* transfer)
{
	usb_device_emulated::control_transfer(bmRequestType, bRequest, wValue, wIndex, wLength, buf_size, buf, transfer);
}

void usb_device_kamen_rider::interrupt_transfer(u32 buf_size, u8* buf, u32 endpoint, UsbTransfer* transfer)
{
	ensure(buf_size == 0x40);

	transfer->fake = true;
	transfer->expected_count = buf_size;
	transfer->expected_result = HC_CC_NOERR;

	if (endpoint == 0x81)
	{
		// Respond after FF command
		transfer->expected_time = get_timestamp() + 1000;
		std::optional<std::array<u8, 64>> response = g_ridergate.pop_added_removed_response();
		if (response)
		{
			memcpy(buf, response.value().data(), 0x40);
		}
		else if (!m_queries.empty())
		{
			memcpy(buf, m_queries.front().data(), 0x20);
			m_queries.pop();
		}
		else
		{
			transfer->expected_count = 0;
			transfer->expected_result = EHCI_CC_HALTED;
		}
	}
	else if (endpoint == 0x01)
	{
		const u8 command = buf[2];
		const u8 sequence = buf[3];

		std::array<u8, 64> q_result{};

		switch (command)
		{
		case 0xB0: // Wake
		{
			g_ridergate.wake_rider_gate(q_result, command, sequence);
			break;
		}
		case 0xC0:
		case 0xC3: // Color Commands
		{
			g_ridergate.get_blank_response(command, sequence, q_result);
			break;
		}
		case 0xD0: // Tag List
		{
			// Return list of figure UIDs, separated by an 09
			g_ridergate.get_list_tags(q_result, command, sequence);
			break;
		}
		case 0xD2: // Read
		{
			// Read 16 bytes from figure with UID buf[4] - buf[10]
			g_ridergate.query_block(q_result, command, sequence, &buf[4], buf[11], buf[12]);
			break;
		}
		case 0xD3:
		{
			// Write 16 bytes to figure with UID buf[4] - buf[10]
			g_ridergate.write_block(q_result, command, sequence, &buf[4], buf[11], buf[12], &buf[13]);
			break;
		}
		default:
			kamen_rider_log.error("Unhandled Query Type: 0x%02X", command);
			break;
		}
		m_queries.push(std::move(q_result));
	}
}
