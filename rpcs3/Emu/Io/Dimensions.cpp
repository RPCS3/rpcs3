#include "stdafx.h"
#include "Dimensions.h"

#include <bit>

#include "Emu/Cell/lv2/sys_usbd.h"

LOG_CHANNEL(dimensions_log, "dimensions");

dimensions_toypad g_dimensionstoypad;

static constexpr std::array<u8, 16> COMMAND_KEY = {0x55, 0xFE, 0xF6, 0xB0, 0x62, 0xBF, 0x0B, 0x41,
	0xC9, 0xB3, 0x7C, 0xB4, 0x97, 0x3E, 0x29, 0x7B};

static constexpr std::array<u8, 17> CHAR_CONSTANT = {0xB7, 0xD5, 0xD7, 0xE6, 0xE7, 0xBA, 0x3C,
	0xA8, 0xD8, 0x75, 0x47, 0x68, 0xCF, 0x23, 0xE9, 0xFE, 0xAA};

static constexpr std::array<u8, 25> PWD_CONSTANT = {0x28, 0x63, 0x29, 0x20, 0x43, 0x6F, 0x70, 0x79,
	0x72, 0x69, 0x67, 0x68, 0x74, 0x20, 0x4C, 0x45, 0x47, 0x4F, 0x20, 0x32, 0x30, 0x31, 0x34, 0xAA, 0xAA};

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

u8 dimensions_toypad::generate_checksum(const std::array<u8, 32>& data, u32 num_of_bytes)
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
	const std::array<u8, 8> value = decrypt(buf, std::nullopt);
	// Seed is the first 4 bytes (little endian) of the decrypted payload
	const u32 seed = read_from_ptr<le_t<u32>>(value);
	// Confirmation is the second 4 bytes (big endian) of the decrypted payload
	const u32 conf = read_from_ptr<be_t<u32>>(value, 4);
	// Initialize rng using the seed from decrypted payload
	initialize_rng(seed);
	std::array<u8, 8> value_to_encrypt = {};
	// Encrypt 8 bytes, first 4 bytes is the decrypted confirmation from payload, 2nd 4 bytes are blank
	write_to_ptr<be_t<u32>>(value_to_encrypt, conf);
	const std::array<u8, 8> encrypted = encrypt(value_to_encrypt.data(), std::nullopt);
	reply_buf[0] = 0x55;
	reply_buf[1] = 0x09;
	reply_buf[2] = sequence;
	// Copy encrypted value to response data
	std::memcpy(&reply_buf[3], encrypted.data(), encrypted.size());
	reply_buf[11] = generate_checksum(reply_buf, 11);
}

void dimensions_toypad::initialize_rng(u32 seed)
{
	m_random_a = 0xF1EA5EED;
	m_random_b = seed;
	m_random_c = seed;
	m_random_d = seed;

	for (int i = 0; i < 42; i++)
	{
		get_next();
	}
}

u32 dimensions_toypad::get_next()
{
	const u32 e = m_random_a - std::rotl(m_random_b, 21);
	m_random_a = m_random_b ^ std::rotl(m_random_c, 19);
	m_random_b = m_random_c + std::rotl(m_random_d, 6);
	m_random_c = m_random_d + e;
	m_random_d = e + m_random_a;
	return m_random_d;
}

std::array<u8, 8> dimensions_toypad::decrypt(const u8* buf, std::optional<std::array<u8, 16>> key)
{
	// Value to decrypt is separated in to two little endian 32 bit unsigned integers
	u32 data_one = read_from_ptr<le_t<u32>>(buf);
	u32 data_two = read_from_ptr<le_t<u32>>(buf, 4);

	// Use the key as 4 32 bit little endian unsigned integers
	u32 key_one;
	u32 key_two;
	u32 key_three;
	u32 key_four;

	if (key)
	{
		key_one = read_from_ptr<le_t<u32>>(key.value());
		key_two = read_from_ptr<le_t<u32>>(key.value(), 4);
		key_three = read_from_ptr<le_t<u32>>(key.value(), 8);
		key_four = read_from_ptr<le_t<u32>>(key.value(), 12);
	}
	else
	{
		key_one = read_from_ptr<le_t<u32>>(COMMAND_KEY);
		key_two = read_from_ptr<le_t<u32>>(COMMAND_KEY, 4);
		key_three = read_from_ptr<le_t<u32>>(COMMAND_KEY, 8);
		key_four = read_from_ptr<le_t<u32>>(COMMAND_KEY, 12);
	}

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

std::array<u8, 8> dimensions_toypad::encrypt(const u8* buf, std::optional<std::array<u8, 16>> key)
{
	// Value to encrypt is separated in to two little endian 32 bit unsigned integers

	u32 data_one = read_from_ptr<le_t<u32>>(buf);
	u32 data_two = read_from_ptr<le_t<u32>>(buf, 4);

	// Use the key as 4 32 bit little endian unsigned integers
	u32 key_one;
	u32 key_two;
	u32 key_three;
	u32 key_four;

	if (key)
	{
		key_one = read_from_ptr<le_t<u32>>(key.value());
		key_two = read_from_ptr<le_t<u32>>(key.value(), 4);
		key_three = read_from_ptr<le_t<u32>>(key.value(), 8);
		key_four = read_from_ptr<le_t<u32>>(key.value(), 12);
	}
	else
	{
		key_one = read_from_ptr<le_t<u32>>(COMMAND_KEY);
		key_two = read_from_ptr<le_t<u32>>(COMMAND_KEY, 4);
		key_three = read_from_ptr<le_t<u32>>(COMMAND_KEY, 8);
		key_four = read_from_ptr<le_t<u32>>(COMMAND_KEY, 12);
	}

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

std::array<u8, 16> dimensions_toypad::generate_figure_key(const std::array<u8, 0x2D * 0x04>& buf)
{
	std::array<u8, 7> uid = {buf[0], buf[1], buf[2], buf[4], buf[5], buf[6], buf[7]};

	std::array<u8, 16> figure_key = {};

	write_to_ptr<be_t<u32>>(figure_key, scramble(uid, 3));
	write_to_ptr<be_t<u32>>(figure_key, 4, scramble(uid, 4));
	write_to_ptr<be_t<u32>>(figure_key, 8, scramble(uid, 5));
	write_to_ptr<be_t<u32>>(figure_key, 12, scramble(uid, 6));

	return figure_key;
}

u32 dimensions_toypad::scramble(const std::array<u8, 7>& uid, u8 count)
{
	std::vector<u8> to_scramble;
	to_scramble.reserve(uid.size() + CHAR_CONSTANT.size());
	for (u8 x : uid)
	{
		to_scramble.push_back(x);
	}
	for (u8 c : CHAR_CONSTANT)
	{
		to_scramble.push_back(c);
	}
	::at32(to_scramble, count * 4 - 1) = 0xaa;

	return read_from_ptr<be_t<u32>>(dimensions_randomize(to_scramble, count).data());
}

std::array<u8, 4> dimensions_toypad::dimensions_randomize(const std::vector<u8>& key, u8 count)
{
	u32 scrambled = 0;
	for (u8 i = 0; i < count; i++)
	{
		const u32 v4 = std::rotr(scrambled, 25);
		const u32 v5 = std::rotr(scrambled, 10);
		const u32 b = read_from_ptr<le_t<u32>>(key, i * 4);
		scrambled = b + v4 + v5 - scrambled;
	}
	return {u8(scrambled & 0xFF), u8(scrambled >> 8 & 0xFF), u8(scrambled >> 16 & 0xFF), u8(scrambled >> 24 & 0xFF)};
}

u32 dimensions_toypad::get_figure_id(const std::array<u8, 0x2D * 0x04>& buf)
{
	const std::array<u8, 16> figure_key = generate_figure_key(buf);

	const std::array<u8, 8> decrypted = decrypt(&buf[36 * 4], figure_key);

	const u32 fig_num = read_from_ptr<le_t<u32>>(decrypted);
	// Characters have their model number encrypted in page 36
	if (fig_num < 1000)
	{
		return fig_num;
	}
	// Vehicles/Gadgets have their model number written as little endian in page 36
	return read_from_ptr<le_t<u32>>(buf, 36 * 4);
}

dimensions_figure& dimensions_toypad::get_figure_by_index(u8 index)
{
	return ::at32(m_figures, index);
}

void dimensions_toypad::random_uid(u8* uid_buffer)
{
	uid_buffer[0] = 0x04;
	uid_buffer[7] = 0x80;

	for (u8 i = 1; i < 7; i++)
	{
		u8 random = rand() % 255;
		uid_buffer[i] = random;
	}
}

void dimensions_toypad::get_challenge_response(const u8* buf, u8 sequence, std::array<u8, 32>& reply_buf)
{
	// Decrypt payload into an 8 byte array
	const std::array<u8, 8> value = decrypt(buf, std::nullopt);
	// Confirmation is the first 4 bytes of the decrypted payload
	const u32 conf = read_from_ptr<be_t<u32>>(value);
	// Generate next random number based on RNG
	const u32 next_random = get_next();
	std::array<u8, 8> value_to_encrypt = {};
	// Encrypt an 8 byte array, first 4 bytes are the next random number (little endian)
	// followed by the confirmation from the decrypted payload
	write_to_ptr<le_t<u32>>(value_to_encrypt, next_random);
	write_to_ptr<be_t<u32>>(value_to_encrypt, 4, conf);
	const std::array<u8, 8> encrypted = encrypt(value_to_encrypt.data(), std::nullopt);
	reply_buf[0] = 0x55;
	reply_buf[1] = 0x09;
	reply_buf[2] = sequence;
	// Copy encrypted value to response data
	std::memcpy(&reply_buf[3], encrypted.data(), encrypted.size());
	reply_buf[11] = generate_checksum(reply_buf, 11);
}

void dimensions_toypad::query_block(u8 index, u8 page, std::array<u8, 32>& reply_buf, u8 sequence)
{
	std::lock_guard lock(m_dimensions_mutex);

	reply_buf[0] = 0x55;
	reply_buf[1] = 0x12;
	reply_buf[2] = sequence;
	reply_buf[3] = 0x00;

	// Index from game begins at 1 rather than 0, so minus 1 here
	if (const u8 figure_index = index - 1; figure_index < dimensions_figure_count)
	{
		const dimensions_figure& figure = get_figure_by_index(figure_index);

		// Query 4 pages of 4 bytes from the figure, copy this to the response
		if (figure.index != 255 && (4 * page) < ((0x2D * 4) - 16))
		{
			std::memcpy(&reply_buf[4], figure.data.data() + (4 * page), 16);
		}
	}
	reply_buf[20] = generate_checksum(reply_buf, 20);
}

void dimensions_toypad::write_block(u8 index, u8 page, const u8* to_write_buf, std::array<u8, 32>& reply_buf, u8 sequence)
{
	std::lock_guard lock(m_dimensions_mutex);

	reply_buf[0] = 0x55;
	reply_buf[1] = 0x02;
	reply_buf[2] = sequence;
	reply_buf[3] = 0x00;

	// Index from game begins at 1 rather than 0, so minus 1 here
	if (const u8 figure_index = index - 1; figure_index < dimensions_figure_count)
	{
		dimensions_figure& figure = get_figure_by_index(figure_index);

		// Copy 4 bytes to the page on the figure requested by the game
		if (figure.index != 255 && page < 0x2D)
		{
			// Id is written to page 36
			if (page == 36)
			{
				figure.id = read_from_ptr<le_t<u32>>(to_write_buf);
			}
			std::memcpy(figure.data.data() + (page * 4), to_write_buf, 4);
			figure.save();
		}
	}
	reply_buf[4] = generate_checksum(reply_buf, 4);
}

void dimensions_toypad::get_model(const u8* buf, u8 sequence, std::array<u8, 32>& reply_buf)
{
	// Decrypt payload to 8 byte array, byte 1 is the index, 4-7 are the confirmation
	const std::array<u8, 8> value = decrypt(buf, std::nullopt);
	const u8 index = value[0];
	const u32 conf = read_from_ptr<be_t<u32>>(value, 4);
	std::array<u8, 8> value_to_encrypt = {};
	// Response is the figure's id (little endian) followed by the confirmation from payload
	// Index from game begins at 1 rather than 0, so minus 1 here
	if (const u8 figure_index = index - 1; figure_index < dimensions_figure_count)
	{
		const dimensions_figure& figure = get_figure_by_index(figure_index);
		write_to_ptr<le_t<u32>>(value_to_encrypt, figure.id);
	}
	write_to_ptr<be_t<u32>>(value_to_encrypt, 4, conf);
	const std::array<u8, 8> encrypted = encrypt(value_to_encrypt.data(), std::nullopt);
	reply_buf[0] = 0x55;
	reply_buf[1] = 0x0a;
	reply_buf[2] = sequence;
	reply_buf[3] = 0x00;
	// Copy encrypted message to response
	std::memcpy(&reply_buf[4], encrypted.data(), encrypted.size());
	reply_buf[12] = generate_checksum(reply_buf, 12);
}

u32 dimensions_toypad::load_figure(const std::array<u8, 0x2D * 0x04>& buf, fs::file in_file, u8 pad, u8 index, bool lock)
{
	if (lock)
	{
		m_dimensions_mutex.lock();
	}

	const u32 id = get_figure_id(buf);

	dimensions_figure& figure = get_figure_by_index(index);
	figure.dim_file = std::move(in_file);
	figure.id = id;
	figure.pad = pad;
	figure.index = index + 1;
	std::memcpy(figure.data.data(), buf.data(), buf.size());
	// When a figure is added to the toypad, respond to the game with the pad they were added to, their index,
	// the direction (0x00 in byte 6 for added) and their UID
	std::array<u8, 32> figure_change_response = {0x56, 0x0b, figure.pad, 0x00, figure.index, 0x00,
		buf[0], buf[1], buf[2], buf[4], buf[5], buf[6], buf[7]};
	figure_change_response[13] = generate_checksum(figure_change_response, 13);
	m_figure_added_removed_responses.push(std::move(figure_change_response));

	if (lock)
	{
		m_dimensions_mutex.unlock();
	}
	return id;
}

bool dimensions_toypad::remove_figure(u8 pad, u8 index, bool full_remove, bool lock)
{
	dimensions_figure& figure = get_figure_by_index(index);
	if (figure.index == 255)
	{
		return false;
	}

	if (lock)
	{
		m_dimensions_mutex.lock();
	}

	// When a figure is removed from the toypad, respond to the game with the pad they were removed from, their index,
	// the direction (0x01 in byte 6 for removed) and their UID
	if (full_remove)
	{
		std::array<u8, 32> figure_change_response = {0x56, 0x0b, pad, 0x00, figure.index, 0x01,
			figure.data[0], figure.data[1], figure.data[2],
			figure.data[4], figure.data[5], figure.data[6], figure.data[7]};
		figure_change_response[13] = generate_checksum(figure_change_response, 13);
		m_figure_added_removed_responses.push(std::move(figure_change_response));
		figure.save();
		figure.dim_file.close();
	}

	figure.index = 255;
	figure.pad = 255;
	figure.id = 0;

	if (lock)
	{
		m_dimensions_mutex.unlock();
	}
	return true;
}

bool dimensions_toypad::temp_remove(u8 index)
{
	std::lock_guard lock(m_dimensions_mutex);

	const dimensions_figure& figure = get_figure_by_index(index);
	if (figure.index == 255)
		return false;

	// Send a response to the game that the figure has been "Picked up" from existing slot,
	// until either the movement is cancelled, or user chooses a space to move to
	std::array<u8, 32> figure_change_response = {0x56, 0x0b, figure.pad, 0x00, figure.index, 0x01,
		figure.data[0], figure.data[1], figure.data[2],
		figure.data[4], figure.data[5], figure.data[6], figure.data[7]};

	figure_change_response[13] = generate_checksum(figure_change_response, 13);
	m_figure_added_removed_responses.push(std::move(figure_change_response));

	return true;
}

bool dimensions_toypad::cancel_remove(u8 index)
{
	std::lock_guard lock(m_dimensions_mutex);

	dimensions_figure& figure = get_figure_by_index(index);
	if (figure.index == 255)
		return false;

	// Cancel the previous movement of the figure
	std::array<u8, 32> figure_change_response = {0x56, 0x0b, figure.pad, 0x00, figure.index, 0x00,
		figure.data[0], figure.data[1], figure.data[2],
		figure.data[4], figure.data[5], figure.data[6], figure.data[7]};

	figure_change_response[13] = generate_checksum(figure_change_response, 13);
	m_figure_added_removed_responses.push(std::move(figure_change_response));

	return true;
}

bool dimensions_toypad::move_figure(u8 pad, u8 index, u8 old_pad, u8 old_index)
{
	if (old_index == index)
	{
		// Don't bother removing and loading again, just send response to the game
		cancel_remove(index);
		return true;
	}

	std::lock_guard lock(m_dimensions_mutex);

	// When moving figures between spaces on the toypad, remove any figure from the space they are moving to,
	// then remove them from their current space, then load them to the space they are moving to
	remove_figure(pad, index, true, false);

	dimensions_figure& figure = get_figure_by_index(old_index);
	const std::array<u8, 0x2D * 0x04> data = figure.data;
	fs::file in_file = std::move(figure.dim_file);

	remove_figure(old_pad, old_index, false, false);

	load_figure(data, std::move(in_file), pad, index, false);

	return true;
}

bool dimensions_toypad::create_blank_character(std::array<u8, 0x2D * 0x04>& buf, u16 id)
{
	random_uid(buf.data());
	buf[3] = id & 0xFF;

	// Only characters are created with their ID encrypted and stored in pages 36 and 37,
	// as well as a password stored in page 43. Blank tags have their information populated
	// by the game when it calls the write_block command.
	if (id != 0)
	{
		// LEGO Dimensions figures use NTAG213 tag types, and the UID for these is stored in
		// bytes 0, 1, 2, 4, 5, 6 and 7 (out of 180 bytes)
		std::array<u8, 7> uid = {buf[0], buf[1], buf[2], buf[4], buf[5], buf[6], buf[7]};
		const std::array<u8, 16> figure_key = generate_figure_key(buf);

		std::array<u8, 8> value_to_encrypt = {};
		write_to_ptr<le_t<u32>>(value_to_encrypt, id);
		write_to_ptr<le_t<u32>>(value_to_encrypt, 4, id);

		std::array<u8, 8> encrypted = encrypt(value_to_encrypt.data(), figure_key);

		std::memcpy(&buf[36 * 4], &encrypted[0], 4);
		std::memcpy(&buf[37 * 4], &encrypted[4], 4);

		std::memcpy(&buf[43 * 4], pwd_generate(uid).data(), 4);
	}
	else
	{
		// Page 38 is used as verification for blank tags
		write_to_ptr<be_t<u16>>(buf.data(), 38 * 4, 1);
	}

	return true;
}

std::array<u8, 4> dimensions_toypad::pwd_generate(const std::array<u8, 7>& uid)
{
	std::vector<u8> pwd_calc = {PWD_CONSTANT.begin(), PWD_CONSTANT.end() - 1};
	for (u8 i = 0; i < uid.size(); i++)
	{
		pwd_calc.insert(pwd_calc.begin() + i, uid[i]);
	}

	return dimensions_randomize(pwd_calc, 8);
}

std::optional<std::array<u8, 32>> dimensions_toypad::pop_added_removed_response()
{
	std::lock_guard lock(m_dimensions_mutex);

	if (m_figure_added_removed_responses.empty())
	{
		return std::nullopt;
	}

	std::array<u8, 32> response = m_figure_added_removed_responses.front();
	m_figure_added_removed_responses.pop();
	return response;
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

std::shared_ptr<usb_device> usb_device_dimensions::make_instance(u32, const std::array<u8, 7>& location)
{
	return std::make_shared<usb_device_dimensions>(location);
}

u16 usb_device_dimensions::get_num_emu_devices()
{
	return 1;
}

void usb_device_dimensions::control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength, u32 buf_size, u8* buf, UsbTransfer* transfer)
{
	usb_device_emulated::control_transfer(bmRequestType, bRequest, wValue, wIndex, wLength, buf_size, buf, transfer);
}

void usb_device_dimensions::interrupt_transfer(u32 buf_size, u8* buf, u32 endpoint, UsbTransfer* transfer)
{
	ensure(buf_size == 0x20);

	transfer->fake = true;
	transfer->expected_count = buf_size;
	transfer->expected_result = HC_CC_NOERR;

	switch (endpoint)
	{
	case 0x81:
	{
		// Read Endpoint, if a request has not been sent via the write endpoint, set expected result as
		// EHCI_CC_HALTED so the game doesn't report the Toypad as being disconnected.
		std::lock_guard lock(m_query_mutex);
		std::optional<std::array<u8, 32>> response = g_dimensionstoypad.pop_added_removed_response();
		if (response)
		{
			std::memcpy(buf, response.value().data(), 0x20);
		}
		else if (!m_queries.empty())
		{
			std::memcpy(buf, m_queries.front().data(), 0x20);
			m_queries.pop();
		}
		else
		{
			transfer->expected_count = 0;
			transfer->expected_result = EHCI_CC_HALTED;
		}
		break;
	}
	case 0x01:
	{
		// Write endpoint, similar structure of request to the Infinity Base with a command for byte 3,
		// sequence for byte 4, the payload after that, then a checksum for the final byte.

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
		case 0xC1: // Get Pad Color
		case 0xC2: // Fade
		case 0xC3: // Flash
		case 0xC4: // Fade Random
		case 0xC6: // Fade All
		case 0xC7: // Flash All
		case 0xC8: // Color All
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
		std::lock_guard lock(m_query_mutex);
		m_queries.push(q_result);
		break;
	}
	default:
		break;
	}
}

void usb_device_dimensions::isochronous_transfer(UsbTransfer* transfer)
{
	usb_device_emulated::isochronous_transfer(transfer);
}
