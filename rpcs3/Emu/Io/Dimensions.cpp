#include "stdafx.h"
#include "Dimensions.h"

#include <bit>
#include <thread>

#include "Crypto/aes.h"
#include "Crypto/sha1.h"
#include "util/asm.hpp"

#include "Emu/Cell/lv2/sys_usbd.h"

LOG_CHANNEL(dimensions_log, "dimensions");

extern void fake_callback_transfer(UsbTransfer* transfer, u32 buf_size);

dimensions_toypad g_dimensionstoypad;

static constexpr std::array<u8, 16> KEY = {0x55, 0xFE, 0xF6, 0xB0, 0x62, 0xBF, 0x0B, 0x41,
	0xC9, 0xB3, 0x7C, 0xB4, 0x97, 0x3E, 0x29, 0x7B};

void dimensions_figure::save()
{
	if (!dim_file)
	{
		dimensions_log.error("Tried to save infinity figure to file but no infinity figure is active!");
		return;
	}
	dim_file.seek(0, fs::seek_set);
	dim_file.write(data.data(), 0x2D * 0x04);
}

u8 dimensions_toypad::generate_checksum(const std::array<u8, 32>& data, u32 num_of_bytes) const
{
	int checksum = 0;
	ensure(num_of_bytes <= data.size());
	for (u8 i = 0; i < num_of_bytes; i++)
	{
		checksum += data[i];
	}
	return (checksum & 0xFF);
}

void dimensions_toypad::get_blank_response(u8 type, u8 sequence, std::array<u8, 32>& reply_buf)
{
	reply_buf[0] = 0x55;
	reply_buf[1] = type;
	reply_buf[2] = sequence;
	reply_buf[3] = generate_checksum(reply_buf, 3);
}

void dimensions_toypad::generate_random_number(const u8* buf, u8 sequence, std::array<u8, 32>& reply_buf)
{
	// Decrypt payload into an 8 byte array
	const std::array<u8, 8> value = decrypt(buf);
	// Seed is the first 4 bytes (little endian) of the decrypted payload
	const u32 seed = read_from_ptr<le_t<u32>>(value.data());
	// Confirmation is the second 4 bytes (big endian) of the decrypted payload
	const u32 conf = read_from_ptr<be_t<u32>>(value.data(), 4);
	// Initialize rng using the seed from decrypted payload
	initialize_rng(seed);
	std::array<u8, 8> value_to_encrypt = {};
	// Encrypt 8 bytes, first 4 bytes is the decrypted confirmation from payload, 2nd 4 bytes are blank
	write_to_ptr<be_t<u32>>(value_to_encrypt.data(), conf);
	const std::array<u8, 8> encrypted = encrypt(value_to_encrypt.data());
	reply_buf[0] = 0x55;
	reply_buf[1] = 0x09;
	reply_buf[2] = sequence;
	// Copy encrypted value to response data
	std::memcpy(&reply_buf[3], encrypted.data(), encrypted.size());
	reply_buf[11] = generate_checksum(reply_buf, 11);
}

void dimensions_toypad::initialize_rng(u32 seed)
{
	random_a = 0xF1EA5EED;
	random_b = seed;
	random_c = seed;
	random_d = seed;

	for (int i = 0; i < 42; i++)
	{
		get_next();
	}
}

u32 dimensions_toypad::get_next()
{
	u32 e = random_a - std::rotl(random_b, 21);
	random_a = random_b ^ std::rotl(random_c, 19);
	random_b = random_c + std::rotl(random_d, 6);
	random_c = random_d + e;
	random_d = e + random_a;
	return random_d;
}

std::array<u8, 8> dimensions_toypad::decrypt(const u8* buf)
{
	// Value to decrypt is separated in to two little endian 32 bit unsigned integers
	u32 data_one = read_from_ptr<le_t<u32>>(buf);
	u32 data_two = read_from_ptr<le_t<u32>>(buf, 4);

	// Use the key as 4 32 bit little endian unsigned integers
	const u32 key_one = read_from_ptr<le_t<u32>>(KEY.data());
	const u32 key_two = read_from_ptr<le_t<u32>>(KEY.data(), 4);
	const u32 key_three = read_from_ptr<le_t<u32>>(KEY.data(), 8);
	const u32 key_four = read_from_ptr<le_t<u32>>(KEY.data(), 12);

	u32 sum = 0xC6EF3720;
	constexpr u32 delta = 0x9E3779B9;

	for (int i = 0; i < 32; i++)
	{
		data_two -= (((data_one << 4) + key_three) ^ (data_one + sum) ^ ((data_one >> 5) + key_four));
		data_one -= (((data_two << 4) + key_one) ^ (data_two + sum) ^ ((data_two >> 5) + key_two));
		sum -= delta;
	}

	ensure(sum == 0, "Decryption failed, sum inequal to 0");

	std::array<u8, 8> decrypted = {u8(data_one & 0xFF), u8((data_one >> 8) & 0xFF),
		u8((data_one >> 16) & 0xFF), u8((data_one >> 24) & 0xFF),
		u8(data_two & 0xFF), u8((data_two >> 8) & 0xFF),
		u8((data_two >> 16) & 0xFF), u8((data_two >> 24) & 0xFF)};
	return decrypted;
}

std::array<u8, 8> dimensions_toypad::encrypt(const u8* buf)
{
	// Value to encrypt is separated in to two little endian 32 bit unsigned integers

	u32 data_one = read_from_ptr<le_t<u32>>(buf);
	u32 data_two = read_from_ptr<le_t<u32>>(buf, 4);

	// Use the key as 4 32 bit little endian unsigned integers
	const u32 key_one = read_from_ptr<le_t<u32>>(KEY.data());
	const u32 key_two = read_from_ptr<le_t<u32>>(KEY.data(), 4);
	const u32 key_three = read_from_ptr<le_t<u32>>(KEY.data(), 8);
	const u32 key_four = read_from_ptr<le_t<u32>>(KEY.data(), 12);

	u32 sum = 0;
	constexpr u32 delta = 0x9E3779B9;

	for (int i = 0; i < 32; i++)
	{
		sum += delta;
		data_one += (((data_two << 4) + key_one) ^ (data_two + sum) ^ ((data_two >> 5) + key_two));
		data_two += (((data_one << 4) + key_three) ^ (data_one + sum) ^ ((data_one >> 5) + key_four));
	}

	ensure(sum == 0xC6EF3720, "Encryption failed, sum inequal to 0xC6EF3720");

	std::array<u8, 8> encrypted = {u8(data_one & 0xFF), u8((data_one >> 8) & 0xFF),
		u8((data_one >> 16) & 0xFF), u8((data_one >> 24) & 0xFF),
		u8(data_two & 0xFF), u8((data_two >> 8) & 0xFF),
		u8((data_two >> 16) & 0xFF), u8((data_two >> 24) & 0xFF)};
	return encrypted;
}

dimensions_figure& dimensions_toypad::get_figure_by_index(u8 index)
{
	for (dimensions_figure& figure : m_figures)
	{
		if (figure.index == index + 1)
		{
			return figure;
		}
	}
	return m_figures[0];
}

void dimensions_toypad::random_uid(u8* uid_buffer)
{
	uid_buffer[0] = 0x04;
	uid_buffer[6] = 0x80;

	for (u8 i = 1; i < 6; i++)
	{
		u8 random = rand() % 255;
		uid_buffer[i] = random;
	}
}

void dimensions_toypad::get_challenge_response(const u8* buf, u8 sequence, std::array<u8, 32>& reply_buf)
{
	// Decrypt payload into an 8 byte array
	const std::array<u8, 8> value = decrypt(buf);
	// Confirmation is the first 4 bytes of the decrypted payload
	const u32 conf = read_from_ptr<be_t<u32>>(value.data());
	// Generate next random number based on RNG
	const u32 next_random = get_next();
	std::array<u8, 8> value_to_encrypt = {};
	// Encrypt an 8 byte array, first 4 bytes are the next random number (little endian)
	// followed by the confirmation from the decrypted payload
	write_to_ptr<le_t<u32>>(value_to_encrypt.data(), next_random);
	write_to_ptr<be_t<u32>>(value_to_encrypt.data(), 4, conf);
	const std::array<u8, 8> encrypted = encrypt(value_to_encrypt.data());
	reply_buf[0] = 0x55;
	reply_buf[1] = 0x09;
	reply_buf[2] = sequence;
	// Copy encrypted value to response data
	std::memcpy(&reply_buf[3], encrypted.data(), encrypted.size());
	reply_buf[11] = generate_checksum(reply_buf, 11);
}

void dimensions_toypad::query_block(u8 index, u8 page, std::array<u8, 32>& reply_buf, u8 sequence)
{
	std::lock_guard lock(dimensions_mutex);

	dimensions_figure& figure = get_figure_by_index(index);

	reply_buf[0] = 0x55;
	reply_buf[1] = 0x12;
	reply_buf[2] = sequence;
	reply_buf[3] = 0x00;
	// Query 4 pages of 4 bytes from the figure, copy this to the response
	if (figure.id != 0 && (4 * page) < ((0x2D * 4) - 16))
	{
		std::memcpy(&reply_buf[4], figure.data.data() + (4 * page), 16);
	}
	reply_buf[20] = generate_checksum(reply_buf, 20);
}

void dimensions_toypad::write_block(u8 index, u8 page, const u8* to_write_buf, std::array<u8, 32>& reply_buf, u8 sequence)
{
	std::lock_guard lock(dimensions_mutex);

	dimensions_figure& figure = get_figure_by_index(index);

	reply_buf[0] = 0x55;
	reply_buf[1] = 0x02;
	reply_buf[2] = sequence;
	reply_buf[3] = 0x00;
	// Copy 4 bytes to the page on the figure requested by the game
	if (figure.id != 0 && page < 0x2D)
	{
		std::memcpy(figure.data.data() + (page * 4), to_write_buf, 4);
		figure.save();
	}
	reply_buf[4] = generate_checksum(reply_buf, 4);
}

void dimensions_toypad::get_model(const u8* buf, u8 sequence, std::array<u8, 32>& reply_buf)
{
	// Decrypt payload to 8 byte array, byte 1 is the index, 4-7 are the confirmation
	const std::array<u8, 8> value = decrypt(buf);
	const u8 index = value[0];
	const u32 conf = read_from_ptr<be_t<u32>>(value.data(), 4);
	dimensions_figure& figure = get_figure_by_index(index);
	std::array<u8, 8> value_to_encrypt = {};
	// Response is the figure's id (little endian) followed by the confirmation from payload
	write_to_ptr<le_t<u32>>(value_to_encrypt.data(), figure.id);
	write_to_ptr<be_t<u32>>(value_to_encrypt.data(), 4, conf);
	const std::array<u8, 8> encrypted = encrypt(value_to_encrypt.data());
	reply_buf[0] = 0x55;
	reply_buf[1] = 0x0a;
	reply_buf[2] = sequence;
	reply_buf[3] = 0x00;
	// Copy encrypted message to response
	std::memcpy(&reply_buf[4], encrypted.data(), encrypted.size());
	reply_buf[12] = generate_checksum(reply_buf, 12);
}

u16 dimensions_toypad::load_figure(const std::array<u8, 0x2D * 0x04>& buf, fs::file in_file, u8 pad, u8 index)
{
	std::lock_guard lock(dimensions_mutex);
	const u16 id = read_from_ptr<be_t<u16>>(buf.data(), 0x0E);

	dimensions_figure& figure = get_figure_by_index(index);
	figure.dim_file = std::move(in_file);
	figure.id = id;
	figure.pad = pad;
	figure.index = index + 1;
	std::memcpy(figure.data.data(), buf.data(), buf.size());
	// When a figure is added to the toypad, respond to the game with the pad they were added to, their index,
	// the direction (0x00 in byte 6 for added) and their UID
	std::array<u8, 32> figure_change_response = {0x56, 0x0b, figure.pad, 0x00, figure.index, 0x00};
	std::memcpy(&figure_change_response[6], buf.data(), 7);
	figure_change_response[13] = generate_checksum(figure_change_response, 13);
	m_figure_added_removed_responses.push(figure_change_response);
	return id;
}

bool dimensions_toypad::remove_figure(u8 pad, u8 index, bool save)
{
	std::lock_guard lock(dimensions_mutex);
	dimensions_figure& figure = get_figure_by_index(index);
	if (figure.index == 255)
	{
		return false;
	}
	// When a figure is removed from the toypad, respond to the game with the pad they were removed from, their index,
	// the direction (0x01 in byte 6 for removed) and their UID
	std::array<u8, 32> figure_change_response = {0x56, 0x0b, figure.pad, 0x00, figure.index, 0x01};
	std::memcpy(&figure_change_response[6], figure.data.data(), 7);
	if (save)
	{
		figure.save();
	}
	figure.dim_file.close();
	figure.index = 255;
	figure.pad = 255;
	figure.id = 0;
	figure_change_response[13] = generate_checksum(figure_change_response, 13);
	m_figure_added_removed_responses.push(figure_change_response);
	return true;
}

bool dimensions_toypad::move_figure(u8 pad, u8 index, u8 old_pad, u8 old_index)
{
	// When moving figures between spaces on the portal, remove any figure from the space they are moving to,
	// then remove them from their current space, then load them to the space they are moving to
	remove_figure(pad, index, true);

	dimensions_figure& figure = get_figure_by_index(old_index);
	const std::array<u8, 0x2D * 0x04> data = figure.data;
	fs::file in_file = std::move(figure.dim_file);

	remove_figure(old_pad, old_index, false);

	load_figure(data, std::move(in_file), pad, index);

	return true;
}

std::optional<std::array<u8, 32>> dimensions_toypad::pop_added_removed_response()
{
	if (m_figure_added_removed_responses.empty())
	{
		return std::nullopt;
	}
	else
	{
		std::array<u8, 32> response = m_figure_added_removed_responses.front();
		m_figure_added_removed_responses.pop();
		return response;
	}
}

usb_device_dimensions::usb_device_dimensions(const std::array<u8, 7>& location)
	: usb_device_emulated(location)
{
	device = UsbDescriptorNode(USB_DESCRIPTOR_DEVICE, UsbDeviceDescriptor{0x200, 0x0, 0x0, 0x0, 0x20, 0x0E6F, 0x0241, 0x200, 0x1, 0x2, 0x3, 0x1});
	auto& config0 = device.add_node(UsbDescriptorNode(USB_DESCRIPTOR_CONFIG, UsbDeviceConfiguration{0x29, 0x1, 0x1, 0x0, 0x80, 0xFA}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_INTERFACE, UsbDeviceInterface{0x0, 0x0, 0x2, 0x3, 0x0, 0x0, 0x0}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_HID, UsbDeviceHID{0x0111, 0x00, 0x01, 0x22, 0x001d}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ENDPOINT, UsbDeviceEndpoint{0x81, 0x03, 0x20, 0x1}));
	config0.add_node(UsbDescriptorNode(USB_DESCRIPTOR_ENDPOINT, UsbDeviceEndpoint{0x01, 0x03, 0x20, 0x1}));
}

usb_device_dimensions::~usb_device_dimensions()
{
}

void usb_device_dimensions::control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength, u32 buf_size, u8* buf, UsbTransfer* transfer)
{
	usb_device_emulated::control_transfer(bmRequestType, bRequest, wValue, wIndex, wLength, buf_size, buf, transfer);
}

void usb_device_dimensions::interrupt_transfer(u32 buf_size, u8* buf, u32 endpoint, UsbTransfer* transfer)
{
	ensure(buf_size == 0x20);

	if (endpoint == 0x81)
	{
		// Read Endpoint, it is required to wait until there is a response to reply with
		// otherwise the game believes that the portal is disconnected. Loop through queries
		// in a seperate thread, then use fake transfer callback method to push the response
		// to the list of other fake response
		std::thread read_message([this, buf, transfer, buf_size]()
			{
				bool responded = false;
				do
				{
					std::unique_lock lock(query_mutex);
					std::optional<std::array<u8, 32>> response = g_dimensionstoypad.pop_added_removed_response();
					if (response)
					{
						std::memcpy(buf, response.value().data(), 0x20);
						responded = true;
					}
					else if (!m_queries.empty())
					{
						std::memcpy(buf, m_queries.front().data(), 0x20);
						m_queries.pop();
						responded = true;
					}
					lock.unlock();
					std::this_thread::sleep_for(100ms);
				} while (!responded);

				fake_callback_transfer(transfer, buf_size);
			});
		read_message.detach();
	}
	else if (endpoint == 0x01)
	{
		dimensions_log.error("Request: %s", fmt::buf_to_hexstring(buf, buf_size));
		// Write endpoint, similar structure of request to the Infinity Base with a command for byte 3,
		// sequence for byte 4, the payload after that, then a checksum for the final byte.
		transfer->fake = true;
		transfer->expected_count = buf_size;
		transfer->expected_result = HC_CC_NOERR;

		const u8 command = buf[2];
		const u8 sequence = buf[3];

		transfer->expected_time = get_timestamp() + 100;
		std::array<u8, 32> q_result{};

		switch (command)
		{
		case 0xB0: // Wake
		{
			// Consistent device response to the wake command
			q_result = {0x55, 0x0e, 0x01, 0x28, 0x63, 0x29,
				0x20, 0x4c, 0x45, 0x47, 0x4f, 0x20,
				0x32, 0x30, 0x31, 0x34, 0x46};
			break;
		}
		case 0xB1: // Seed
		{
			// Initialise a random number generator using the seed provided
			g_dimensionstoypad.generate_random_number(&buf[4], sequence, q_result);
			break;
		}
		case 0xB3: // Challenge
		{
			// Get the next number in the sequence based on the RNG from 0xB1 command
			g_dimensionstoypad.get_challenge_response(&buf[4], sequence, q_result);
			break;
		}
		case 0xC0: // Color
		case 0xC2: // Fade
		case 0xC6: // Fade All
		{
			// Send a blank response to acknowledge color has been sent to toypad
			g_dimensionstoypad.get_blank_response(0x01, sequence, q_result);
			break;
		}
		case 0xD2: // Read
		{
			// Read 4 pages from the figure at index (buf[4]), starting with page buf[5]
			g_dimensionstoypad.query_block(buf[4], buf[5], q_result, sequence);
			break;
		}
		case 0xD3: // Write
		{
			// Write 4 bytes to page buf[5] to the figure at index buf[4]
			g_dimensionstoypad.write_block(buf[4], buf[5], &buf[6], q_result, sequence);
			break;
		}
		case 0xD4: // Model
		{
			// Get the model id of the figure at index buf[4]
			g_dimensionstoypad.get_model(&buf[4], sequence, q_result);
			break;
		}
		case 0xC1: // Get Pad Color
		case 0xC3: // Flash
		case 0xC4: // Fade Random
		case 0xC7: // Flash All
		case 0xC8: // Color All
		case 0xD0: // Tag List
		case 0xE1: // PWD
		case 0xE5: // Active
		case 0xFF: // LEDS Query
		{
			// Further investigation required
			dimensions_log.error("Unimplemented LD Function: 0x%x", command);
			dimensions_log.error("Request: %s", fmt::buf_to_hexstring(buf, buf_size));
			break;
		}
		default:
		{
			dimensions_log.error("Unknown LD Function: 0x%x", command);
			dimensions_log.error("Request: %s", fmt::buf_to_hexstring(buf, buf_size));
			break;
		}
		}
		std::lock_guard lock(query_mutex);
		m_queries.push(q_result);
	}
}

void usb_device_dimensions::isochronous_transfer(UsbTransfer* transfer)
{
	usb_device_emulated::isochronous_transfer(transfer);
}
