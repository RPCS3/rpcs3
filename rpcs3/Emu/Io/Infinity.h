#pragma once

#include "Emu/Io/usb_device.h"
#include "Utilities/mutex.h"
#include <array>
#include <queue>

struct infinity_figure
{
	fs::file inf_file;
	std::array<u8, 0x14 * 0x10> data{};
	bool present = false;
	u8 order_added = 255;
	void save();
};

class infinity_base
{
public:
	void get_blank_response(u8 sequence, std::array<u8, 32>& reply_buf);
	void descramble_and_seed(u8* buf, u8 sequence, std::array<u8, 32>& reply_buf);
	void get_next_and_scramble(u8 sequence, std::array<u8, 32>& reply_buf);
	void get_present_figures(u8 sequence, std::array<u8, 32>& reply_buf);
	void query_block(u8 fig_num, u8 block, std::array<u8, 32>& reply_buf, u8 sequence);
	void write_block(u8 fig_num, u8 block, const u8* to_write_buf, std::array<u8, 32>& reply_buf, u8 sequence);
	void get_figure_identifier(u8 fig_num, u8 sequence, std::array<u8, 32>& reply_buf);
	bool has_figure_been_added_removed() const;
	std::array<u8, 32> pop_added_removed_response();

	bool remove_figure(u8 position);
	u32 load_figure(const std::array<u8, 0x14 * 0x10>& buf, fs::file in_file, u8 position);

protected:
	shared_mutex infinity_mutex;
	std::array<infinity_figure, 9> figures;

private:
	u8 generate_checksum(const std::array<u8, 32>& data, int num_of_bytes) const;
	u32 descramble(u64 num_to_descramble);
	u64 scramble(u32 num_to_scramble, u32 garbage);
	void generate_seed(u32 seed);
	u32 get_next();
	infinity_figure& get_figure_by_order(u8 order_added);
	u8 derive_figure_position(u8 position);

	u32 random_a = 0;
	u32 random_b = 0;
	u32 random_c = 0;
	u32 random_d = 0;

	u8 m_figure_order = 0;
	std::queue<std::array<u8, 32>> m_figure_added_removed_responses;
};

extern infinity_base g_infinitybase;

class usb_device_infinity : public usb_device_emulated
{
public:
	usb_device_infinity(const std::array<u8, 7>& location);
	~usb_device_infinity();

	static std::shared_ptr<usb_device> make_instance(u32 controller_index, const std::array<u8, 7>& location);
	static u16 get_num_emu_devices();

	void control_transfer(u8 bmRequestType, u8 bRequest, u16 wValue, u16 wIndex, u16 wLength, u32 buf_size, u8* buf, UsbTransfer* transfer) override;
	void interrupt_transfer(u32 buf_size, u8* buf, u32 endpoint, UsbTransfer* transfer) override;

protected:
	std::queue<std::array<u8, 32>> m_queries;
};
