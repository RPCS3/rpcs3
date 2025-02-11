#pragma once

#include "Utilities/Thread.h"
#include <string>

struct gdb_cmd;

class cpu_thread;
class ppu_thread;

class gdb_thread
{
	static const u64 ALL_THREADS = 0xffffffffffffffff;
	static const u64 ANY_THREAD = 0;

	int server_socket = -1;
	int client_socket = -1;
	shared_ptr<cpu_thread> selected_thread{};
	u64 continue_ops_thread_id = ANY_THREAD;
	u64 general_ops_thread_id = ANY_THREAD;

	//initialize server socket and start listening
	void start_server();
	//read at most cnt bytes to buf, returns number of bytes actually read
	int read(void* buf, int cnt) const;
	//reads one character
	char read_char();
	//reads pairs of hex characters and returns their integer value
	u8 read_hexbyte();

	// Tries to read command, returns false on error
	bool try_read_cmd(gdb_cmd& out_cmd);

	//reads commands until receiveing one with valid checksum
	//in case of other exception (i.e. wrong first char of command)
	//it will log exception text and return false
	//in that case best for caller would be to stop reading, because
	//chance of getting correct command is low
	bool read_cmd(gdb_cmd& out_cmd);
	//send cnt bytes from buf to client
	void send(const char* buf, int cnt) const;
	//send character to client
	void send_char(char c);
	//acknowledge packet, either as accepted or declined
	void ack(bool accepted);
	//sends command body cmd to client
	void send_cmd(const std::string& cmd);
	//sends command to client until receives positive acknowledgement
	//returns false in case some error happened, and command wasn't sent
	bool send_cmd_ack(const std::string& cmd);
	//appends encoded char c to string str, and returns checksum. encoded byte can occupy 2 bytes
	static u8 append_encoded_char(char c, std::string& str);
	//convert u8 to 2 byte hexademical representation
	static std::string to_hexbyte(u8 i);
	//choose thread, support ALL_THREADS and ANY_THREAD values, returns true if some thread was selected
	bool select_thread(u64 id);
	//returns register value as hex string by register id (in gdb), in case of wrong id returns empty string
	static std::string get_reg(ppu_thread* thread, u32 rid);
	//sets register value to hex string by register id (in gdb), in case of wrong id returns false
	static bool set_reg(ppu_thread* thread, u32 rid, const std::string& value);
	//returns size of register with id rid in bytes, zero if invalid rid is provided
	static u32 get_reg_size(ppu_thread* thread, u32 rid);
	//send reason of stop, returns false if sending response failed
	bool send_reason();

	void wait_with_interrupts();

	//commands
	bool cmd_extended_mode(gdb_cmd& cmd);
	bool cmd_reason(gdb_cmd& cmd);
	bool cmd_supported(gdb_cmd& cmd);
	bool cmd_thread_info(gdb_cmd& cmd);
	bool cmd_current_thread(gdb_cmd& cmd);
	bool cmd_read_register(gdb_cmd& cmd);
	bool cmd_write_register(gdb_cmd& cmd);
	bool cmd_read_memory(gdb_cmd& cmd);
	bool cmd_write_memory(gdb_cmd& cmd);
	bool cmd_read_all_registers(gdb_cmd& cmd);
	bool cmd_write_all_registers(gdb_cmd& cmd);
	bool cmd_set_thread_ops(gdb_cmd& cmd);
	bool cmd_attached_to_what(gdb_cmd& cmd);
	static bool cmd_kill(gdb_cmd& cmd);
	bool cmd_continue_support(gdb_cmd& cmd);
	bool cmd_vcont(gdb_cmd& cmd);
	bool cmd_set_breakpoint(gdb_cmd& cmd);
	bool cmd_remove_breakpoint(gdb_cmd& cmd);

public:
	bool from_breakpoint = true;
	bool paused = false;
	u64 pausedBy = 0;

	gdb_thread() noexcept;
	~gdb_thread();
	gdb_thread(const gdb_thread&) = delete;
	gdb_thread& operator=(const gdb_thread&) = delete;

	void operator()();
	void pause_from(cpu_thread* t);

	static constexpr auto thread_name = "GDB Server"sv;
};

using gdb_server = named_thread<gdb_thread>;
