#pragma once

#include "Emu/Io/usb_device.h"
#include "Utilities/mutex.h"
#include <array>
#include <optional>
#include <queue>

static constexpr size_t dimensions_figure_count = 7;

struct dimensions_figure
{
	fs::file dim_file;
	std::array<u8, 0x2D * 0x04> data{};
	u8 index = 255;
	u8 pad = 255;
	u32 id = 0;
	void save();
};

class dimensions_toypad
{
public:
	static void get_blank_response(u8 type, u8 sequence, std::array<u8, 32>& reply_buf);
	void generate_random_number(const u8* buf, u8 sequence, std::array<u8, 32>& reply_buf);
	void initialize_rng(u32 seed);
	void get_challenge_response(const u8* buf, u8 sequence, std::array<u8, 32>& reply_buf);
	void query_block(u8 index, u8 page, std::array<u8, 32>& reply_buf, u8 sequence);
	void write_block(u8 index, u8 page, const u8* to_write_buf, std::array<u8, 32>& reply_buf, u8 sequence);
	void get_model(const u8* buf, u8 sequence, std::array<u8, 32>& reply_buf);
	std::optional<std::array<u8, 32>> pop_added_removed_response();

	bool remove_figure(u8 pad, u8 index, bool full_remove, bool lock);
	bool temp_remove(u8 index);
	bool cancel_remove(u8 index);
	u32 load_figure(const std::array<u8, 0x2D * 0x04>& buf, fs::file in_file, u8 pad, u8 index, bool lock);
	bool move_figure(u8 pad, u8 index, u8 old_pad, u8 old_index);
	static bool create_blank_character(std::array<u8, 0x2D * 0x04>& buf, u16 id);

protected:
	shared_mutex m_dimensions_mutex;
	std::array<dimensions_figure, dimensions_figure_count> m_figures{};

private:
	static void random_uid(u8* uid_buffer);
	static u8 generate_checksum(const std::array<u8, 32>& data, u32 num_of_bytes);
	static std::array<u8, 8> decrypt(const u8* buf, std::optional<std::array<u8, 16>> key);
	static std::array<u8, 8> encrypt(const u8* buf, std::optional<std::array<u8, 16>> key);
	static std::array<u8, 16> generate_figure_key(const std::array<u8, 0x2D * 0x04>& buf);
	static u32 scramble(const std::array<u8, 7>& uid, u8 count);
	static std::array<u8, 4> pwd_generate(const std::array<u8, 7>& uid);
	static std::array<u8, 4> dimensions_randomize(const std::vector<u8>& key, u8 count);
	static u32 get_figure_id(const std::array<u8, 0x2D * 0x04>& buf);
	u32 get_next();
	dimensions_figure& get_figure_by_index(u8 index);

	u32 m_random_a{};
	u32 m_random_b{};
	u32 m_random_c{};
	u32 m_random_d{};

	u8 m_figure_order = 0;
	std::queue<std::array<u8, 32>> m_figure_added_removed_responses;
};

extern dimensions_toypad g_dimensionstoypad;

class usb_device_dimensions : public usb_device_emulated
{
public:
	usb_device_dimensions(const std::array<u8, 7>& location);
	~usb_device_dimensions();

	static std::shared_ptr<usb_device> make_instance(u32 controller_index, const std::array<u8, 7>& location);
	static u16 get_num_emu_devices();

	void control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength, u32 buf_size, u8* buf, UsbTransfer* transfer) override;
	void interrupt_transfer(u32 buf_size, u8* buf, u32 endpoint, UsbTransfer* transfer) override;
	void isochronous_transfer(UsbTransfer* transfer) override;

protected:
	shared_mutex m_query_mutex;
	std::queue<std::array<u8, 32>> m_queries;
};
