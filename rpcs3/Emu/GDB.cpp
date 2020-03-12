#include "stdafx.h"

#include "GDB.h"
#include "util/logs.hpp"
#include "Utilities/StrUtil.h"
#include "Emu/Memory/vm.h"
#include "Emu/System.h"
#include "Emu/system_config.h"
#include "Emu/IdManager.h"
#include "Emu/CPU/CPUThread.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/RawSPUThread.h"
#include "Emu/Cell/SPUThread.h"

#ifdef _WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <afunix.h> // sockaddr_un
#else
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/un.h> // sockaddr_un
#endif

#include <algorithm>
#include <regex>
#include <charconv>

extern void ppu_set_breakpoint(u32 addr);
extern void ppu_remove_breakpoint(u32 addr);

LOG_CHANNEL(GDB);

#ifndef _WIN32
int closesocket(int s)
{
	return close(s);
}

void set_nonblocking(int s)
{
	fcntl(s, F_SETFL, fcntl(s, F_GETFL) | O_NONBLOCK);
}

#define sscanf_s sscanf
#define HEX_U32 "x"
#define HEX_U64 "lx"
#else

void set_nonblocking(int s)
{
	u_long mode = 1;
	ioctlsocket(s, FIONBIO, &mode);
}

#define HEX_U32 "lx"
#define HEX_U64 "llx"
#endif

struct gdb_cmd
{
	std::string cmd;
	std::string data;
	u8 checksum;
};

bool check_errno_again()
{
#ifdef _WIN32
	int err = GetLastError();
	return (err == WSAEWOULDBLOCK);
#else
	int err = errno;
	return (err == EAGAIN) || (err == EWOULDBLOCK);
#endif
}

std::string u32_to_hex(u32 i) {
	return fmt::format("%" HEX_U32, i);
}

std::string u64_to_padded_hex(u64 value) {
	return fmt::format("%.16" HEX_U64, value);
}

std::string u32_to_padded_hex(u32 value) {
	return fmt::format("%.8" HEX_U32, value);
}

u8 hex_to_u8(std::string val) {
	u8 result;
	sscanf_s(val.c_str(), "%02hhX", &result);
	return result;
}

u32 hex_to_u32(std::string val) {
	u32 result;
	sscanf_s(val.c_str(), "%" HEX_U32, &result);
	return result;
}

u64 hex_to_u64(std::string val) {
	u64 result;
	sscanf_s(val.c_str(), "%" HEX_U64, &result);
	return result;
}

void gdb_thread::start_server()
{
	// IPv4 address:port in format 127.0.0.1:2345
	static const std::regex ipv4_regex("^([0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3})\\:([0-9]{1,5})$");

	auto [sname, sshared] = g_cfg.misc.gdb_server.get();

	if (sname[0] == '\0')
	{
		// Empty string or starts with null: GDB server disabled
		GDB.notice("GDB Server is disabled.");
		return;
	}

	// Try to detect socket type
	std::smatch match;

	if (std::regex_match(sname, match, ipv4_regex))
	{
		struct addrinfo hints{};
		struct addrinfo* info;
		hints.ai_flags    = AI_PASSIVE;
		hints.ai_socktype = SOCK_STREAM;

		std::string bind_addr = match[1].str();
		std::string bind_port = match[2].str();

		if (getaddrinfo(bind_addr.c_str(), bind_port.c_str(), &hints, &info) == 0)
		{
			server_socket = static_cast<int>(socket(info->ai_family, info->ai_socktype, info->ai_protocol));

			if (server_socket == -1)
			{
				GDB.error("Error creating IP socket for '%s'.", sname);
				freeaddrinfo(info);
				return;
			}

			set_nonblocking(server_socket);

			if (bind(server_socket, info->ai_addr, static_cast<int>(info->ai_addrlen)) != 0)
			{
				GDB.error("Failed to bind socket on '%s'.", sname);
				freeaddrinfo(info);
				return;
			}

			freeaddrinfo(info);

			if (listen(server_socket, 1) != 0)
			{
				GDB.error("Failed to listen on '%s'.", sname);
				return;
			}

			GDB.notice("Started listening on '%s'.", sname);
			return;
		}
	}

	// Fallback to UNIX socket
	server_socket = static_cast<int>(socket(AF_UNIX, SOCK_STREAM, 0));

	if (server_socket == -1)
	{
		GDB.error("Failed to create Unix socket. Possibly unsupported.");
		return;
	}

	// Delete existing socket (TODO?)
	fs::remove_file(sname);

	set_nonblocking(server_socket);

	sockaddr_un unix_saddr;
	unix_saddr.sun_family = AF_UNIX;
	strcpy_trunc(unix_saddr.sun_path, sname);

	if (bind(server_socket, reinterpret_cast<struct sockaddr*>(&unix_saddr), sizeof(unix_saddr)) != 0)
	{
		GDB.error("Failed to bind Unix socket '%s'.", sname);
		return;
	}

	if (listen(server_socket, 1) != 0)
	{
		GDB.error("Failed to listen on Unix socket '%s'.", sname);
		return;
	}

	GDB.notice("Started listening on Unix socket '%s'.", sname);
}

int gdb_thread::read(void* buf, int cnt)
{
	while (thread_ctrl::state() != thread_state::aborting)
	{
		int result = recv(client_socket, reinterpret_cast<char*>(buf), cnt, 0);

		if (result == -1)
		{
			if (check_errno_again())
			{
				thread_ctrl::wait_for(5000);
				continue;
			}

			GDB.error("Error during socket read.");
			fmt::throw_exception("Error during socket read" HERE);
		}
		return result;
	}
	return 0;
}

char gdb_thread::read_char()
{
	char result;
	int cnt = read(&result, 1);
	if (!cnt) {
		fmt::throw_exception("Tried to read char, but no data was available" HERE);
	}
	return result;
}

u8 gdb_thread::read_hexbyte()
{
	std::string s;
	s += read_char();
	s += read_char();
	return hex_to_u8(s);
}

bool gdb_thread::try_read_cmd(gdb_cmd& out_cmd)
{
	char c = read_char();
	//interrupt
	if (c == 0x03) [[unlikely]] {
		out_cmd.cmd = '\x03';
		out_cmd.data = "";
		out_cmd.checksum = 0;
		return true;
	}
	if (c != '$') [[unlikely]] {
		//gdb starts conversation with + for some reason
		if (c == '+') {
			c = read_char();
		}
		if (c != '$') {
			fmt::throw_exception("Expected start of packet character '$', got '%c' instead" HERE, c);
		}
	}
	//clear packet data
	out_cmd.cmd = "";
	out_cmd.data = "";
	out_cmd.checksum = 0;
	bool cmd_part = true;
	u8 checksum = 0;
	while(true) {
		c = read_char();
		if (c == '#') {
			break;
		}
		checksum = (checksum + reinterpret_cast<u8&>(c)) % 256;
		//escaped char
		if (c == '}') {
			c = read_char() ^ 0x20;
			checksum = (checksum + reinterpret_cast<u8&>(c)) % 256;
		}
		//cmd-data splitters
		if (cmd_part && ((c == ':') || (c == '.') || (c == ';'))) {
			cmd_part = false;
		}
		if (cmd_part) {
			out_cmd.cmd += c;
			//only q and v commands can have multi-char command
			if ((out_cmd.cmd.length() == 1) && (c != 'q') && (c != 'v')) {
				cmd_part = false;
			}
		} else {
			out_cmd.data += c;
		}
	}
	out_cmd.checksum = read_hexbyte();
	return out_cmd.checksum == checksum;
}

bool gdb_thread::read_cmd(gdb_cmd& out_cmd)
{
	while (true)
	{
		if (try_read_cmd(out_cmd))
		{
			ack(true);
			return true;
		}

		ack(false);
	}
}

void gdb_thread::send(const char* buf, int cnt)
{
	GDB.trace("Sending %s (%d bytes).", buf, cnt);

	while (thread_ctrl::state() != thread_state::aborting)
	{
		int res = ::send(client_socket, buf, cnt, 0);
		if (res == -1)
		{
			if (check_errno_again())
			{
				thread_ctrl::wait_for(5000);
				continue;
			}
			GDB.error("Failed sending %d bytes.", cnt);
			return;
		}
		return;
	}
}

void gdb_thread::send_char(char c)
{
	send(&c, 1);
}

void gdb_thread::ack(bool accepted)
{
	send_char(accepted ? '+' : '-');
}

void gdb_thread::send_cmd(const std::string& cmd)
{
	u8 checksum = 0;
	std::string buf;
	buf.reserve(cmd.length() + 4);
	buf += "$";
	for (std::size_t i = 0; i < cmd.length(); ++i)
	{
		checksum = (checksum + append_encoded_char(cmd[i], buf)) % 256;
	}
	buf += "#";
	buf += to_hexbyte(checksum);
	send(buf.c_str(), static_cast<int>(buf.length()));
}

bool gdb_thread::send_cmd_ack(const std::string& cmd)
{
	while (true) {
		send_cmd(cmd);
		char c = read_char();
		if (c == '+') [[likely]]
			return true;
		if (c != '-') [[unlikely]] {
			GDB.error("Wrong acknowledge character received: '%c'.", c);
			return false;
		}
		GDB.warning("Client rejected our cmd.");
	}
}

u8 gdb_thread::append_encoded_char(char c, std::string& str)
{
	u8 checksum = 0;
	if ((c == '#') || (c == '$') || (c == '}')) [[unlikely]] {
		str += '}';
		c ^= 0x20;
		checksum = '}';
	}
	checksum = (checksum + reinterpret_cast<u8&>(c)) % 256;
	str += c;
	return checksum;
}

std::string gdb_thread::to_hexbyte(u8 i)
{
	std::string result = "00";
	u8 i1 = i & 0xF;
	u8 i2 = i >> 4;
	result[0] = i2 > 9 ? 'a' + i2 - 10 : '0' + i2;
	result[1] = i1 > 9 ? 'a' + i1 - 10 : '0' + i1;
	return result;
}

bool gdb_thread::select_thread(u64 id)
{
	//in case we have none at all
	selected_thread.reset();
	const auto on_select = [&](u32, cpu_thread& cpu)
	{
		return (id == ALL_THREADS) || (id == ANY_THREAD) || (cpu.id == id);
	};
	if (auto ppu = idm::select<named_thread<ppu_thread>>(on_select))
	{
		selected_thread = ppu.ptr;
		return true;
	}
	GDB.warning("Unable to select thread! Is the emulator running?");
	return false;
}

std::string gdb_thread::get_reg(ppu_thread* thread, u32 rid)
{
	//ids from gdb/features/rs6000/powerpc-64.c
	//pc
	switch (rid) {
	case 64:
		return u64_to_padded_hex(thread->cia);
	//msr?
	case 65:
		return std::string(16, 'x');
	case 66:
		return u32_to_padded_hex(thread->cr.pack());
	case 67:
		return u64_to_padded_hex(thread->lr);
	case 68:
		return u64_to_padded_hex(thread->ctr);
	//xer
	case 69:
		return std::string(8, 'x');
	//fpscr
	case 70:
		return std::string(8, 'x');
	default:
		if (rid > 70) return "";
		return (rid > 31)
			? u64_to_padded_hex(std::bit_cast<u64>(thread->fpr[rid - 32])) //fpr
			: u64_to_padded_hex(thread->gpr[rid]); //gpr
	}
}

bool gdb_thread::set_reg(ppu_thread* thread, u32 rid, std::string value)
{
	switch (rid) {
	case 64:
		thread->cia = static_cast<u32>(hex_to_u64(value));
		return true;
		//msr?
	case 65:
		return true;
	case 66:
		thread->cr.unpack(hex_to_u32(value));
		return true;
	case 67:
		thread->lr = hex_to_u64(value);
		return true;
	case 68:
		thread->ctr = hex_to_u64(value);
		return true;
		//xer
	case 69:
		return true;
		//fpscr
	case 70:
		return true;
	default:
		if (rid > 70) return false;
		if (rid > 31) {
			u64 val = hex_to_u64(value);
			thread->fpr[rid - 32] = std::bit_cast<f64>(val);
		} else {
			thread->gpr[rid] = hex_to_u64(value);
		}
		return true;
	}
}

u32 gdb_thread::get_reg_size(ppu_thread* thread, u32 rid)
{
	switch (rid) {
	case 66:
	case 69:
	case 70:
		return 4;
	default:
		if (rid > 70) {
			return 0;
		}
		return 8;
	}
}

bool gdb_thread::send_reason()
{
	return send_cmd_ack("S05");
}

void gdb_thread::wait_with_interrupts()
{
	char c;
	while (!paused)
	{
		int result = recv(client_socket, &c, 1, 0);

		if (result == -1)
		{
			if (check_errno_again())
			{
				thread_ctrl::wait_for(5000);
				continue;
			}

			GDB.error("Error during socket read.");
			fmt::throw_exception("Error during socket read" HERE);
		} else if (c == 0x03) {
			paused = true;
		}
	}
}

bool gdb_thread::cmd_extended_mode(gdb_cmd& cmd)
{
	return send_cmd_ack("OK");
}

bool gdb_thread::cmd_reason(gdb_cmd& cmd)
{
	return send_reason();
}

bool gdb_thread::cmd_supported(gdb_cmd& cmd)
{
	return send_cmd_ack("PacketSize=1200");
}

bool gdb_thread::cmd_thread_info(gdb_cmd& cmd)
{
	std::string result;
	const auto on_select = [&](u32, cpu_thread& cpu)
	{
		if (result.length()) {
			result += ",";
		}
		result += u64_to_padded_hex(static_cast<u64>(cpu.id));
	};
	idm::select<named_thread<ppu_thread>>(on_select);
	//idm::select<named_thread<spu_thread>>(on_select);

	//todo: this may exceed max command length
	result = "m" + result + "l";

	return send_cmd_ack(result);;
}

bool gdb_thread::cmd_current_thread(gdb_cmd& cmd)
{
	return send_cmd_ack(selected_thread.expired() ? "" : ("QC" + u64_to_padded_hex(selected_thread.lock()->id)));
}

bool gdb_thread::cmd_read_register(gdb_cmd& cmd)
{
	if (!select_thread(general_ops_thread_id)) {
		return send_cmd_ack("E02");
	}
	auto th = selected_thread.lock();
	if (th->id_type() == 1) {
		auto ppu = static_cast<named_thread<ppu_thread>*>(th.get());
		u32 rid = hex_to_u32(cmd.data);
		std::string result = get_reg(ppu, rid);
		if (!result.length()) {
			GDB.warning("Wrong register id %d.", rid);
			return send_cmd_ack("E01");
		}
		return send_cmd_ack(result);
	}
	GDB.warning("Unimplemented thread type %d.", th->id_type());
	return send_cmd_ack("");
}

bool gdb_thread::cmd_write_register(gdb_cmd& cmd)
{
	if (!select_thread(general_ops_thread_id)) {
		return send_cmd_ack("E02");
	}
	auto th = selected_thread.lock();
	if (th->id_type() == 1) {
		auto ppu = static_cast<named_thread<ppu_thread>*>(th.get());
		size_t eq_pos = cmd.data.find('=');
		if (eq_pos == umax) {
			GDB.warning("Wrong write_register cmd data '%s'.", cmd.data);
			return send_cmd_ack("E02");
		}
		u32 rid = hex_to_u32(cmd.data.substr(0, eq_pos));
		std::string value = cmd.data.substr(eq_pos + 1);
		if (!set_reg(ppu, rid, value)) {
			GDB.warning("Wrong register id %d.", rid);
			return send_cmd_ack("E01");
		}
		return send_cmd_ack("OK");
	}
	GDB.warning("Unimplemented thread type %d.", th->id_type());
	return send_cmd_ack("");
}

bool gdb_thread::cmd_read_memory(gdb_cmd& cmd)
{
	size_t s = cmd.data.find(',');
	u32 addr = hex_to_u32(cmd.data.substr(0, s));
	u32 len = hex_to_u32(cmd.data.substr(s + 1));
	std::string result;
	result.reserve(len * 2);
	for (u32 i = 0; i < len; ++i) {
		if (vm::check_addr(addr)) {
			result += to_hexbyte(vm::read8(addr + i));
		} else {
			break;
			//result += "xx";
		}
	}
	if (len && !result.length()) {
		//nothing read
		return send_cmd_ack("E01");
	}
	return send_cmd_ack(result);
}

bool gdb_thread::cmd_write_memory(gdb_cmd& cmd)
{
	size_t s = cmd.data.find(',');
	size_t s2 = cmd.data.find(':');
	if ((s == umax) || (s2 == umax)) {
		GDB.warning("Malformed write memory request received: '%s'.", cmd.data);
		return send_cmd_ack("E01");
	}
	u32 addr = hex_to_u32(cmd.data.substr(0, s));
	u32 len = hex_to_u32(cmd.data.substr(s + 1, s2 - s - 1));
	const char* data_ptr = (cmd.data.c_str()) + s2 + 1;
	for (u32 i = 0; i < len; ++i) {
		if (vm::check_addr(addr + i, 1, vm::page_writable)) {
			u8 val;
			int res = sscanf_s(data_ptr, "%02hhX", &val);
			if (!res) {
				GDB.warning("Couldn't read u8 from string '%s'.", data_ptr);
				return send_cmd_ack("E02");
			}
			data_ptr += 2;
			vm::write8(addr + i, val);
		} else {
			return send_cmd_ack("E03");
		}
	}
	return send_cmd_ack("OK");
}

bool gdb_thread::cmd_read_all_registers(gdb_cmd& cmd)
{
	std::string result;
	select_thread(general_ops_thread_id);

	auto th = selected_thread.lock();
	if (th->id_type() == 1) {
		auto ppu = static_cast<named_thread<ppu_thread>*>(th.get());
		//68 64-bit registers, and 3 32-bit
		result.reserve(68*16 + 3*8);
		for (int i = 0; i < 71; ++i) {
			result += get_reg(ppu, i);
		}
		return send_cmd_ack(result);
	}
	GDB.warning("Unimplemented thread type %d.", th->id_type());
	return send_cmd_ack("");
}

bool gdb_thread::cmd_write_all_registers(gdb_cmd& cmd)
{
	select_thread(general_ops_thread_id);
	auto th = selected_thread.lock();
	if (th->id_type() == 1) {
		auto ppu = static_cast<named_thread<ppu_thread>*>(th.get());
		int ptr = 0;
		for (int i = 0; i < 71; ++i) {
			int sz = get_reg_size(ppu, i);
			set_reg(ppu, i, cmd.data.substr(ptr, sz * 2));
			ptr += sz * 2;
		}
		return send_cmd_ack("OK");
	}
	GDB.warning("Unimplemented thread type %d.", th->id_type());
	return send_cmd_ack("E01");
}

bool gdb_thread::cmd_set_thread_ops(gdb_cmd& cmd)
{
	char type = cmd.data[0];
	std::string thread = cmd.data.substr(1);
	u64 id = thread == "-1" ? ALL_THREADS : hex_to_u64(thread);
	if (type == 'c') {
		continue_ops_thread_id = id;
	} else {
		general_ops_thread_id = id;
	}
	if (select_thread(id)) {
		return send_cmd_ack("OK");
	}
	GDB.warning("Client asked to use thread 0x%x for %s, but no matching thread was found.", id, type == 'c' ? "continue ops" : "general ops");
	return send_cmd_ack("E01");
}

bool gdb_thread::cmd_attached_to_what(gdb_cmd& cmd)
{
	//creating processes from client is not available yet
	return send_cmd_ack("1");
}

bool gdb_thread::cmd_kill(gdb_cmd& cmd)
{
	Emu.Stop();
	return true;
}

bool gdb_thread::cmd_continue_support(gdb_cmd& cmd)
{
	return send_cmd_ack("vCont;c;s;C;S");
}

bool gdb_thread::cmd_vcont(gdb_cmd& cmd)
{
	//todo: handle multiple actions and thread ids
	this->from_breakpoint = false;
	if (cmd.data[1] == 'c' || cmd.data[1] == 's') {
		select_thread(continue_ops_thread_id);
		auto ppu = std::static_pointer_cast<named_thread<ppu_thread>>(selected_thread.lock());
		paused = false;
		if (cmd.data[1] == 's') {
			ppu->state += cpu_flag::dbg_step;
		}
		ppu->state -= cpu_flag::dbg_pause;
		//special case if app didn't start yet (only loaded)
		if (!Emu.IsPaused() && !Emu.IsRunning()) {
			Emu.Run(true);
		}
		if (Emu.IsPaused()) {
			Emu.Resume();
		} else {
			thread_ctrl::notify(*ppu);
		}
		wait_with_interrupts();
		//we are in all-stop mode
		Emu.Pause();
		select_thread(pausedBy);
		// we have to remove dbg_pause from thread that paused execution, otherwise
		// it will be paused forever (Emu.Resume only removes dbg_global_pause)
		ppu = std::static_pointer_cast<named_thread<ppu_thread>>(selected_thread.lock());
		ppu->state -= cpu_flag::dbg_pause;
		return send_reason();
	}
	return send_cmd_ack("");
}

static const u32 INVALID_PTR = 0xffffffff;

bool gdb_thread::cmd_set_breakpoint(gdb_cmd& cmd)
{
	char type = cmd.data[0];
	//software breakpoint
	if (type == '0') {
		u32 addr = INVALID_PTR;
		if (cmd.data.find(';') != umax) {
			GDB.warning("Received request to set breakpoint with condition, but they are not supported.");
			return send_cmd_ack("E01");
		}
		sscanf_s(cmd.data.c_str(), "0,%x", &addr);
		if (addr == INVALID_PTR) {
			GDB.warning("Can't parse breakpoint request, data: '%s'.", cmd.data);
			return send_cmd_ack("E02");
		}
		ppu_set_breakpoint(addr);
		return send_cmd_ack("OK");
	}
	//other breakpoint types are not supported
	return send_cmd_ack("");
}

bool gdb_thread::cmd_remove_breakpoint(gdb_cmd& cmd)
{
	char type = cmd.data[0];
	//software breakpoint
	if (type == '0') {
		u32 addr = INVALID_PTR;
		sscanf_s(cmd.data.c_str(), "0,%x", &addr);
		if (addr == INVALID_PTR) {
			GDB.warning("Can't parse breakpoint remove request, data: '%s'.", cmd.data);
			return send_cmd_ack("E01");
		}
		ppu_remove_breakpoint(addr);
		return send_cmd_ack("OK");
	}
	//other breakpoint types are not supported
	return send_cmd_ack("");

}

#define PROCESS_CMD(cmds,handler) if (cmd.cmd == cmds) { if (!handler(cmd)) break; else continue; }

gdb_thread::gdb_thread() noexcept
{
#ifdef _WIN32
	WSADATA wsa_data;
	WSAStartup(MAKEWORD(2, 2), &wsa_data);
#endif
}

gdb_thread::~gdb_thread()
{
	if (server_socket != -1)
	{
		closesocket(server_socket);
	}

	if (client_socket != -1)
	{
		closesocket(client_socket);
	}

#ifdef _WIN32
	WSACleanup();
#endif
}

void gdb_thread::operator()()
{
	start_server();

	while (server_socket != -1 && thread_ctrl::state() != thread_state::aborting)
	{
		sockaddr_in client;
		socklen_t client_len = sizeof(client);
		client_socket = static_cast<int>(accept(server_socket, reinterpret_cast<struct sockaddr*>(&client), &client_len));

		if (client_socket == -1)
		{
			if (check_errno_again())
			{
				thread_ctrl::wait_for(5000);
				continue;
			}

			GDB.error("Could not establish new connection.");
			return;
		}
		//stop immediately
		if (Emu.IsRunning()) {
			Emu.Pause();
		}

		{
			char hostbuf[32];
			inet_ntop(client.sin_family, reinterpret_cast<void*>(&client.sin_addr), hostbuf, 32);
			GDB.success("Got connection to GDB debug server from %s:%d.", hostbuf, client.sin_port);

			gdb_cmd cmd;

			while (thread_ctrl::state() != thread_state::aborting)
			{
				if (!read_cmd(cmd))
				{
					break;
				}
				GDB.trace("Command %s with data %s received.", cmd.cmd, cmd.data);
				PROCESS_CMD("!", cmd_extended_mode);
				PROCESS_CMD("?", cmd_reason);
				PROCESS_CMD("qSupported", cmd_supported);
				PROCESS_CMD("qfThreadInfo", cmd_thread_info);
				PROCESS_CMD("qC", cmd_current_thread);
				PROCESS_CMD("p", cmd_read_register);
				PROCESS_CMD("P", cmd_write_register);
				PROCESS_CMD("m", cmd_read_memory);
				PROCESS_CMD("M", cmd_write_memory);
				PROCESS_CMD("g", cmd_read_all_registers);
				PROCESS_CMD("G", cmd_write_all_registers);
				PROCESS_CMD("H", cmd_set_thread_ops);
				PROCESS_CMD("qAttached", cmd_attached_to_what);
				PROCESS_CMD("k", cmd_kill);
				PROCESS_CMD("vCont?", cmd_continue_support);
				PROCESS_CMD("vCont", cmd_vcont);
				PROCESS_CMD("z", cmd_remove_breakpoint);
				PROCESS_CMD("Z", cmd_set_breakpoint);

				GDB.trace("Unsupported command received: '%s'.", cmd.cmd);
				if (!send_cmd_ack("")) {
					break;
				}
			}
		}
	}
}

#undef PROCESS_CMD

void gdb_thread::pause_from(cpu_thread* t)
{
	if (paused)
	{
		return;
	}
	paused = true;
	pausedBy = t->id;
	thread_ctrl::notify(*static_cast<gdb_server*>(this));
}

#ifndef _WIN32
#undef sscanf_s
#endif

#undef HEX_U32
#undef HEX_U64
