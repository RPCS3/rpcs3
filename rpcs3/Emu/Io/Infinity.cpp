#include "stdafx.h"
#include "Infinity.h"

#include <bit>

#include "Crypto/aes.h"
#include "Crypto/sha1.h"

LOG_CHANNEL(infinity_log, "infinity");

infinity_base g_infinitybase;

static constexpr std::array<u8, 32> SHA1_CONSTANT = {
	0xAF, 0x62, 0xD2, 0xEC, 0x04, 0x91, 0x96, 0x8C, 0xC5, 0x2A, 0x1A, 0x71, 0x65, 0xF8, 0x65, 0xFE,
	0x28, 0x63, 0x29, 0x20, 0x44, 0x69, 0x73, 0x6e, 0x65, 0x79, 0x20, 0x32, 0x30, 0x31, 0x33};

void infinity_figure::save()
{
	if (!inf_file)
	{
		infinity_log.error("Tried to save infinity figure to file but no infinity figure is active!");
		return;
	}
	inf_file.seek(0, fs::seek_set);
	inf_file.write(data.data(), 0x14 * 0x10);
}

u8 infinity_base::generate_checksum(const std::array<u8, 32>& data, int num_of_bytes) const
{
	int checksum = 0;
	for (int i = 0; i < num_of_bytes; i++)
	{
		checksum += data[i];
	}
	return (checksum & 0xFF);
}

void infinity_base::get_blank_response(u8 sequence, std::array<u8, 32>& reply_buf)
{
	reply_buf[0] = 0xaa;
	reply_buf[1] = 0x01;
	reply_buf[2] = sequence;
	reply_buf[3] = generate_checksum(reply_buf, 3);
}

void infinity_base::descramble_and_seed(u8* buf, u8 sequence, std::array<u8, 32>& reply_buf)
{
	u64 value = u64(buf[4]) << 56 | u64(buf[5]) << 48 | u64(buf[6]) << 40 | u64(buf[7]) << 32 |
	            u64(buf[8]) << 24 | u64(buf[9]) << 16 | u64(buf[10]) << 8 | u64(buf[11]);
	u32 seed = descramble(value);
	generate_seed(seed);
	get_blank_response(sequence, reply_buf);
}

void infinity_base::get_next_and_scramble(u8 sequence, std::array<u8, 32>& reply_buf)
{
	const u32 next_random = get_next();
	const u64 scrambled_next_random = scramble(next_random, 0);
	reply_buf = {0xAA, 0x09, sequence};
	reply_buf[3] = u8((scrambled_next_random >> 56) & 0xFF);
	reply_buf[4] = u8((scrambled_next_random >> 48) & 0xFF);
	reply_buf[5] = u8((scrambled_next_random >> 40) & 0xFF);
	reply_buf[6] = u8((scrambled_next_random >> 32) & 0xFF);
	reply_buf[7] = u8((scrambled_next_random >> 24) & 0xFF);
	reply_buf[8] = u8((scrambled_next_random >> 16) & 0xFF);
	reply_buf[9] = u8((scrambled_next_random >> 8) & 0xFF);
	reply_buf[10] = u8(scrambled_next_random & 0xFF);
	reply_buf[11] = generate_checksum(reply_buf, 11);
}

u32 infinity_base::descramble(u64 num_to_descramble)
{
	u64 mask = 0x8E55AA1B3999E8AA;
	u32 ret = 0;

	for (int i = 0; i < 64; i++)
	{
		if (mask & 0x8000000000000000)
		{
			ret = (ret << 1) | (num_to_descramble & 0x01);
		}

		num_to_descramble >>= 1;
		mask <<= 1;
	}

	return ret;
}

u64 infinity_base::scramble(u32 num_to_scramble, u32 garbage)
{
	u64 mask = 0x8E55AA1B3999E8AA;
	u64 ret = 0;

	for (int i = 0; i < 64; i++)
	{
		ret <<= 1;

		if ((mask & 1) != 0)
		{
			ret |= (num_to_scramble & 1);
			num_to_scramble >>= 1;
		}
		else
		{
			ret |= (garbage & 1);
			garbage >>= 1;
		}

		mask >>= 1;
	}

	return ret;
}

void infinity_base::generate_seed(u32 seed)
{
	random_a = 0xF1EA5EED;
	random_b = seed;
	random_c = seed;
	random_d = seed;

	for (int i = 0; i < 23; i++)
	{
		get_next();
	}
}

u32 infinity_base::get_next()
{
	u32 a = random_a;
	u32 b = random_b;
	u32 c = random_c;
	u32 ret = std::rotl(random_b, 27);

	const u32 temp = (a + ((ret ^ 0xFFFFFFFF) + 1));
	b ^= std::rotl(c, 17);
	a = random_d;
	c += a;
	ret = b + temp;
	a += temp;

	random_c = a;
	random_a = b;
	random_b = c;
	random_d = ret;

	return ret;
}

void infinity_base::get_present_figures(u8 sequence, std::array<u8, 32>& reply_buf)
{
	int x = 3;
	for (u8 i = 0; i < figures.size(); i++)
	{
		u8 slot = i == 0 ? 0x10 : (i < 4) ? 0x20 :
		                                    0x30;
		if (figures[i].present)
		{
			reply_buf[x] = slot + figures[i].order_added;
			reply_buf[x + 1] = 0x09;
			x += 2;
		}
	}
	reply_buf[0] = 0xaa;
	reply_buf[1] = x - 2;
	reply_buf[2] = sequence;
	reply_buf[x] = generate_checksum(reply_buf, x);
}

infinity_figure& infinity_base::get_figure_by_order(u8 order_added)
{
	for (u8 i = 0; i < figures.size(); i++)
	{
		if (figures[i].order_added == order_added)
		{
			return figures[i];
		}
	}
	return figures[0];
}

u8 infinity_base::derive_figure_position(u8 position)
{
	switch (position)
	{
	case 0:
	case 1:
	case 2:
		return 1;
	case 3:
	case 4:
	case 5:
		return 2;
	case 6:
	case 7:
	case 8:
		return 3;

	default:
		return 0;
	}
}

void infinity_base::query_block(u8 fig_num, u8 block, std::array<u8, 32>& reply_buf, u8 sequence)
{
	std::lock_guard lock(infinity_mutex);

	infinity_figure& figure = get_figure_by_order(fig_num);

	reply_buf[0] = 0xaa;
	reply_buf[1] = 0x12;
	reply_buf[2] = sequence;
	reply_buf[3] = 0x00;
	const u8 file_block = (block == 0) ? 1 : (block * 4);
	if (figure.present && file_block < 20)
	{
		memcpy(&reply_buf[4], figure.data.data() + (16 * file_block), 16);
	}
	reply_buf[20] = generate_checksum(reply_buf, 20);
}

void infinity_base::write_block(u8 fig_num, u8 block, const u8* to_write_buf,
	std::array<u8, 32>& reply_buf, u8 sequence)
{
	std::lock_guard lock(infinity_mutex);

	infinity_figure& figure = get_figure_by_order(fig_num);

	reply_buf[0] = 0xaa;
	reply_buf[1] = 0x02;
	reply_buf[2] = sequence;
	reply_buf[3] = 0x00;
	const u8 file_block = (block == 0) ? 1 : (block * 4);
	if (figure.present && file_block < 20)
	{
		memcpy(figure.data.data() + (file_block * 16), to_write_buf, 16);
		figure.save();
	}
	reply_buf[4] = generate_checksum(reply_buf, 4);
}

void infinity_base::get_figure_identifier(u8 fig_num, u8 sequence, std::array<u8, 32>& reply_buf)
{
	std::lock_guard lock(infinity_mutex);

	infinity_figure& figure = get_figure_by_order(fig_num);

	reply_buf[0] = 0xaa;
	reply_buf[1] = 0x09;
	reply_buf[2] = sequence;
	reply_buf[3] = 0x00;

	if (figure.present)
	{
		memcpy(&reply_buf[4], figure.data.data(), 7);
	}
	reply_buf[11] = generate_checksum(reply_buf, 11);
}

bool infinity_base::has_figure_been_added_removed() const
{
	return !m_figure_added_removed_responses.empty();
}

std::array<u8, 32> infinity_base::pop_added_removed_response()
{
	std::array<u8, 32> response = m_figure_added_removed_responses.front();
	m_figure_added_removed_responses.pop();
	return response;
}

bool infinity_base::remove_figure(u8 position)
{
	std::lock_guard lock(infinity_mutex);
	infinity_figure& figure = figures[position];

	if (!figure.present)
	{
		return false;
	}

	position = derive_figure_position(position);
	if (position == 0)
	{
		return false;
	}

	figure.present = false;

	std::array<u8, 32> figure_change_response = {0xab, 0x04, position, 0x09, figure.order_added, 0x01};
	figure_change_response[6] = generate_checksum(figure_change_response, 6);
	m_figure_added_removed_responses.push(figure_change_response);

	figure.save();
	figure.inf_file.close();
	return true;
}

u32 infinity_base::load_figure(const std::array<u8, 0x14 * 0x10>& buf, fs::file in_file, u8 position)
{
	std::lock_guard lock(infinity_mutex);
	u8 order_added;

	std::vector<u8> sha1_calc = {SHA1_CONSTANT.begin(), SHA1_CONSTANT.end() - 1};
	for (int i = 0; i < 7; i++)
	{
		sha1_calc.push_back(buf[i]);
	}

	sha1_context ctx;
	u8 output[20];

	sha1_starts(&ctx);
	sha1_update(&ctx, sha1_calc.data(), sha1_calc.size());
	sha1_finish(&ctx, output);

	std::array<u8, 16> key{};
	for (int i = 0; i < 4; i++)
	{
		for (int x = 0; x < 4; x++)
		{
			key[x + (i * 4)] = output[(3 - x) + (i * 4)];
		}
	}

	aes_context aes;
	aes_setkey_dec(&aes, key.data(), 128);
	std::array<u8, 16> infinity_decrypted_block{};
	aes_crypt_ecb(&aes, AES_DECRYPT, &buf[16], infinity_decrypted_block.data());

	u32 number = u32(infinity_decrypted_block[1]) << 16 | u32(infinity_decrypted_block[2]) << 8 |
	             u32(infinity_decrypted_block[3]);

	infinity_figure& figure = figures[position];

	figure.inf_file = std::move(in_file);
	memcpy(figure.data.data(), buf.data(), figure.data.size());
	figure.present = true;
	if (figure.order_added == 255)
	{
		figure.order_added = m_figure_order;
		m_figure_order++;
	}
	order_added = figure.order_added;

	position = derive_figure_position(position);
	if (position == 0)
	{
		return 0;
	}

	std::array<u8, 32> figure_change_response = {0xab, 0x04, position, 0x09, order_added, 0x00};
	figure_change_response[6] = generate_checksum(figure_change_response, 6);
	m_figure_added_removed_responses.push(figure_change_response);

	return number;
}

usb_device_infinity::usb_device_infinity(const std::array<u8, 7>& location)
	: usb_device_emulated(location)
{
	device = UsbDescriptorNode(USB_DESCRIPTOR_DEVICE, UsbDeviceDescriptor{0x200, 0x0, 0x0, 0x0, 0x20, 0x0E6F, 0x0129, 0x200, 0x1, 0x2, 0x3, 0x1});
	auto& config0 = device.add_node(UsbDescriptorNode(USB_DESCRIPTOR_CONFIG, UsbDeviceConfiguration{0x29, 0x1, 0x1, 0x0, 0x80, 0xFA}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_INTERFACE, UsbDeviceInterface{0x0, 0x0, 0x2, 0x3, 0x0, 0x0, 0x0}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_HID, UsbDeviceHID{0x0111, 0x00, 0x01, 0x22, 0x001d}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ENDPOINT, UsbDeviceEndpoint{0x81, 0x3, 0x20, 0x1}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ENDPOINT, UsbDeviceEndpoint{0x1, 0x3, 0x20, 0x1}));
}

usb_device_infinity::~usb_device_infinity()
{
}

std::shared_ptr<usb_device> usb_device_infinity::make_instance(u32, const std::array<u8, 7>& location)
{
	return std::make_shared<usb_device_infinity>(location);
}

u16 usb_device_infinity::get_num_emu_devices()
{
	return 1;
}

void usb_device_infinity::control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength, u32 buf_size, u8* buf, UsbTransfer* transfer)
{
	usb_device_emulated::control_transfer(bmRequestType, bRequest, wValue, wIndex, wLength, buf_size, buf, transfer);
}

void usb_device_infinity::interrupt_transfer(u32 buf_size, u8* buf, u32 endpoint, UsbTransfer* transfer)
{
	ensure(buf_size == 0x20);

	transfer->fake = true;
	transfer->expected_count = buf_size;
	transfer->expected_result = HC_CC_NOERR;

	if (endpoint == 0x81)
	{
		// Respond after FF command
		transfer->expected_time = get_timestamp() + 1000;
		if (g_infinitybase.has_figure_been_added_removed())
		{
			memcpy(buf, g_infinitybase.pop_added_removed_response().data(), 0x20);
		}
		else if (!m_queries.empty())
		{
			memcpy(buf, m_queries.front().data(), 0x20);
			m_queries.pop();
		}
	}
	else if (endpoint == 0x01)
	{
		const u8 command = buf[2];
		const u8 sequence = buf[3];

		transfer->expected_time = get_timestamp() + 500;
		std::array<u8, 32> q_result{};

		switch (command)
		{
		case 0x80:
		{
			q_result = {0xaa, 0x15, 0x00, 0x00, 0x0f, 0x01, 0x00, 0x03, 0x02, 0x09, 0x09, 0x43,
				0x20, 0x32, 0x62, 0x36, 0x36, 0x4b, 0x34, 0x99, 0x67, 0x31, 0x93, 0x8c};
			break;
		}
		case 0x81:
		{
			// Initiate Challenge
			g_infinitybase.descramble_and_seed(buf, sequence, q_result);
			break;
		}
		case 0x83:
		{
			// Challenge Response
			g_infinitybase.get_next_and_scramble(sequence, q_result);
			break;
		}
		case 0x90:
		case 0x92:
		case 0x93:
		case 0x95:
		case 0x96:
		{
			// Color commands
			g_infinitybase.get_blank_response(sequence, q_result);
			break;
		}
		case 0xA1:
		{
			// Get Present Figures
			g_infinitybase.get_present_figures(sequence, q_result);
			break;
		}
		case 0xA2:
		{
			// Read Block from Figure
			g_infinitybase.query_block(buf[4], buf[5], q_result, sequence);
			break;
		}
		case 0xA3:
		{
			// Write block to figure
			g_infinitybase.write_block(buf[4], buf[5], &buf[7], q_result, sequence);
			break;
		}
		case 0xB4:
		{
			// Get figure ID
			g_infinitybase.get_figure_identifier(buf[4], sequence, q_result);
			break;
		}
		case 0xB5:
		{
			// Get status?
			g_infinitybase.get_blank_response(sequence, q_result);
			break;
		}
		default:
			infinity_log.error("Unhandled Query Type: 0x%02X", command);
			break;
		}

		m_queries.push(q_result);
	}
}
