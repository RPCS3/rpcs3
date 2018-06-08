#include "stdafx.h"
#ifdef WITH_GDB_DEBUGGER

#include "GDBDebugServer.h"
#include "Log.h"
#include <algorithm>
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/CPU/CPUThread.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/RawSPUThread.h"
#include "Emu/Cell/SPUThread.h"

#ifndef _WIN32
#include "fcntl.h"
#endif

extern void ppu_set_breakpoint(u32 addr);
extern void ppu_remove_breakpoint(u32 addr);

logs::channel gdbDebugServer("gdbDebugServer");

int sock_init(void)
{
#ifdef _WIN32
	WSADATA wsa_data;
	return WSAStartup(MAKEWORD(1, 1), &wsa_data);
#else
	return 0;
#endif
}

int sock_quit(void)
{
#ifdef _WIN32
	return WSACleanup();
#else
	return 0;
#endif
}

#ifndef _WIN32
int closesocket(socket_t s) {
	return close(s);
}
const int SOCKET_ERROR = -1;
const socket_t INVALID_SOCKET = -1;
#define sscanf_s sscanf
#define HEX_U32 "x"
#define HEX_U64 "lx"
#else
#define HEX_U32 "lx"
#define HEX_U64 "llx"
#endif

bool check_errno_again() {
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

void GDBDebugServer::start_server()
{
	server_socket = socket(AF_INET, SOCK_STREAM, 0);

	if (server_socket == INVALID_SOCKET) {
		gdbDebugServer.error("Error creating server socket");
		return;
	}

#ifdef WIN32
	{
		int mode = 1;
		ioctlsocket(server_socket, FIONBIO, (u_long FAR *)&mode);
	}
#else
	fcntl(server_socket, F_SETFL, fcntl(server_socket, F_GETFL) | O_NONBLOCK);
#endif

	int err;

	sockaddr_in server_saddr;
	server_saddr.sin_family = AF_INET;
	int port = g_cfg.misc.gdb_server_port;
	server_saddr.sin_port = htons(port);
	server_saddr.sin_addr.s_addr = htonl(INADDR_ANY);

	err = bind(server_socket, (struct sockaddr *) &server_saddr, sizeof(server_saddr));
	if (err == SOCKET_ERROR) {
		gdbDebugServer.error("Error binding to port %d", port);
		return;
	}

	err = listen(server_socket, 1);
	if (err == SOCKET_ERROR) {
		gdbDebugServer.error("Error listening on port %d", port);
		return;
	}

	gdbDebugServer.success("GDB Debug Server listening on port %d", port);
}

int GDBDebugServer::read(void * buf, int cnt)
{
	while (!stop) {
		int result = recv(client_socket, reinterpret_cast<char*>(buf), cnt, 0);

		if (result == SOCKET_ERROR) {
			if (check_errno_again()) {
				thread_ctrl::wait_for(50);
				continue;
			}

			gdbDebugServer.error("Error during socket read");
			fmt::throw_exception("Error during socket read" HERE);
		}
		return result;
	}
	return 0;
}

char GDBDebugServer::read_char()
{
	char result;
	int cnt = read(&result, 1);
	if (!cnt) {
		fmt::throw_exception("Tried to read char, but no data was available" HERE);
	}
	return result;
}

u8 GDBDebugServer::read_hexbyte()
{
	std::string s = "";
	s += read_char();
	s += read_char();
	return hex_to_u8(s);
}

void GDBDebugServer::try_read_cmd(gdb_cmd & out_cmd)
{
	char c = read_char();
	//interrupt
	if (UNLIKELY(c == 0x03)) {
		out_cmd.cmd = '\x03';
		out_cmd.data = "";
		out_cmd.checksum = 0;
		return;
	}
	if (UNLIKELY(c != '$')) {
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
	if (out_cmd.checksum != checksum) {
		throw wrong_checksum_exception("Wrong checksum for packet" HERE);
	}
}

bool GDBDebugServer::read_cmd(gdb_cmd & out_cmd)
{
	while (true) {
		try {
			try_read_cmd(out_cmd);
			ack(true);
			return true;
		}
		catch (wrong_checksum_exception) {
			ack(false);
		}
		catch (std::runtime_error e) {
			gdbDebugServer.error(e.what());
			return false;
		}
	}
}

void GDBDebugServer::send(const char * buf, int cnt)
{
	gdbDebugServer.trace("Sending %s (%d bytes)", buf, cnt);
	while (!stop) {
		int res = ::send(client_socket, buf, cnt, 0);
		if (res == SOCKET_ERROR) {
			if (check_errno_again()) {
				thread_ctrl::wait_for(50);
				continue;
			}
			gdbDebugServer.error("Failed sending %d bytes", cnt);
			return;
		}
		return;
	}
}

void GDBDebugServer::send_char(char c)
{
	send(&c, 1);
}

void GDBDebugServer::ack(bool accepted)
{
	send_char(accepted ? '+' : '-');
}

void GDBDebugServer::send_cmd(const std::string & cmd)
{
	u8 checksum = 0;
	std::string buf;
	buf.reserve(cmd.length() + 4);
	buf += "$";
	for (int i = 0; i < cmd.length(); ++i) {
		checksum = (checksum + append_encoded_char(cmd[i], buf)) % 256;
	}
	buf += "#";
	buf += to_hexbyte(checksum);
	send(buf.c_str(), static_cast<int>(buf.length()));
}

bool GDBDebugServer::send_cmd_ack(const std::string & cmd)
{
	while (true) {
		send_cmd(cmd);
		char c = read_char();
		if (LIKELY(c == '+')) {
			return true;
		}
		if (UNLIKELY(c != '-')) {
			gdbDebugServer.error("Wrong acknowledge character received %c", c);
			return false;
		}
		gdbDebugServer.warning("Client rejected our cmd");
	}
}

u8 GDBDebugServer::append_encoded_char(char c, std::string & str)
{
	u8 checksum = 0;
	if (UNLIKELY((c == '#') || (c == '$') || (c == '}'))) {
		str += '}';
		c ^= 0x20;
		checksum = '}';
	}
	checksum = (checksum + reinterpret_cast<u8&>(c)) % 256;
	str += c;
	return checksum;
}

std::string GDBDebugServer::to_hexbyte(u8 i)
{
	std::string result = "00";
	u8 i1 = i & 0xF;
	u8 i2 = i >> 4;
	result[0] = i2 > 9 ? 'a' + i2 - 10 : '0' + i2;
	result[1] = i1 > 9 ? 'a' + i1 - 10 : '0' + i1;
	return result;
}

bool GDBDebugServer::select_thread(u64 id)
{
	//in case we have none at all
	selected_thread.reset();
	const auto on_select = [&](u32, cpu_thread& cpu)
	{
		return (id == ALL_THREADS) || (id == ANY_THREAD) || (cpu.id == id);
	};
	if (auto ppu = idm::select<ppu_thread>(on_select)) {
		selected_thread = ppu.ptr;
		return true;
	}
	gdbDebugServer.warning("Unable to select thread! Is the emulator running?");
	return false;
}

std::string GDBDebugServer::get_reg(std::shared_ptr<ppu_thread> thread, u32 rid)
{
	std::string result;
	//ids from gdb/features/rs6000/powerpc-64.c
	//pc
	switch (rid) {
	case 64:
		return u64_to_padded_hex(thread->cia);
	//msr?
	case 65:
		return std::string(16, 'x');
	case 66:
		return u32_to_padded_hex(thread->cr_pack());
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
			? u64_to_padded_hex(reinterpret_cast<u64&>(thread->fpr[rid - 32])) //fpr
			: u64_to_padded_hex(thread->gpr[rid]); //gpr
	}
}

bool GDBDebugServer::set_reg(std::shared_ptr<ppu_thread> thread, u32 rid, std::string value)
{
	switch (rid) {
	case 64:
		thread->cia = static_cast<u32>(hex_to_u64(value));
		return true;
		//msr?
	case 65:
		return true;
	case 66:
		thread->cr_unpack(hex_to_u32(value));
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
			thread->fpr[rid - 32] = reinterpret_cast<f64&>(val);
		} else {
			thread->gpr[rid] = hex_to_u64(value);
		}
		return true;
	}
}

u32 GDBDebugServer::get_reg_size(std::shared_ptr<ppu_thread> thread, u32 rid)
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

bool GDBDebugServer::send_reason()
{
	return send_cmd_ack("S05");
}

void GDBDebugServer::wait_with_interrupts() {
	char c;
	while (!paused) {
		int result = recv(client_socket, &c, 1, 0);

		if (result == SOCKET_ERROR) {
			if (check_errno_again()) {
				thread_ctrl::wait_for(50);
				continue;
			}

			gdbDebugServer.error("Error during socket read");
			fmt::throw_exception("Error during socket read" HERE);
		} else if (c == 0x03) {
			paused = true;
		}
	}
}

bool GDBDebugServer::cmd_extended_mode(gdb_cmd & cmd)
{
	return send_cmd_ack("OK");
}

bool GDBDebugServer::cmd_reason(gdb_cmd & cmd)
{
	return send_reason();
}

bool GDBDebugServer::cmd_supported(gdb_cmd & cmd)
{
	return send_cmd_ack("PacketSize=1200");
}

bool GDBDebugServer::cmd_thread_info(gdb_cmd & cmd)
{
	std::string result = "";
	const auto on_select = [&](u32, cpu_thread& cpu)
	{
		if (result.length()) {
			result += ",";
		}
		result += u64_to_padded_hex(static_cast<u64>(cpu.id));
	};
	idm::select<ppu_thread>(on_select);
	//idm::select<RawSPUThread>(on_select);
	//idm::select<SPUThread>(on_select);

	//todo: this may exceed max command length
	result = "m" + result + "l";

	return send_cmd_ack(result);;
}

bool GDBDebugServer::cmd_current_thread(gdb_cmd & cmd)
{
	return send_cmd_ack(selected_thread.expired() ? "" : ("QC" + u64_to_padded_hex(selected_thread.lock()->id)));
}

bool GDBDebugServer::cmd_read_register(gdb_cmd & cmd)
{
	if (!select_thread(general_ops_thread_id)) {
		return send_cmd_ack("E02");
	}
	auto th = selected_thread.lock();
	if (th->id_type() == 1) {
		auto ppu = std::static_pointer_cast<ppu_thread>(th);
		u32 rid = hex_to_u32(cmd.data);
		std::string result = get_reg(ppu, rid);
		if (!result.length()) {
			gdbDebugServer.warning("Wrong register id %d", rid);
			return send_cmd_ack("E01");
		}
		return send_cmd_ack(result);
	}
	gdbDebugServer.warning("Unimplemented thread type %d", th->id_type());
	return send_cmd_ack("");
}

bool GDBDebugServer::cmd_write_register(gdb_cmd & cmd)
{
	if (!select_thread(general_ops_thread_id)) {
		return send_cmd_ack("E02");
	}
	auto th = selected_thread.lock();
	if (th->id_type() == 1) {
		auto ppu = std::static_pointer_cast<ppu_thread>(th);
		size_t eq_pos = cmd.data.find('=');
		if (eq_pos == std::string::npos) {
			gdbDebugServer.warning("Wrong write_register cmd data %s", cmd.data.c_str());
			return send_cmd_ack("E02");
		}
		u32 rid = hex_to_u32(cmd.data.substr(0, eq_pos));
		std::string value = cmd.data.substr(eq_pos + 1);
		if (!set_reg(ppu, rid, value)) {
			gdbDebugServer.warning("Wrong register id %d", rid);
			return send_cmd_ack("E01");
		}
		return send_cmd_ack("OK");
	}
	gdbDebugServer.warning("Unimplemented thread type %d", th->id_type());
	return send_cmd_ack("");
}

bool GDBDebugServer::cmd_read_memory(gdb_cmd & cmd)
{
	size_t s = cmd.data.find(',');
	u32 addr = hex_to_u32(cmd.data.substr(0, s));
	u32 len = hex_to_u32(cmd.data.substr(s + 1));
	std::string result;
	result.reserve(len * 2);
	for (u32 i = 0; i < len; ++i) {
		if (vm::check_addr(addr, 1, vm::page_allocated | vm::page_readable)) {
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

bool GDBDebugServer::cmd_write_memory(gdb_cmd & cmd)
{
	size_t s = cmd.data.find(',');
	size_t s2 = cmd.data.find(':');
	if ((s == std::string::npos) || (s2 == std::string::npos)) {
		gdbDebugServer.warning("Malformed write memory request received: %s", cmd.data.c_str());
		return send_cmd_ack("E01");
	}
	u32 addr = hex_to_u32(cmd.data.substr(0, s));
	u32 len = hex_to_u32(cmd.data.substr(s + 1, s2 - s - 1));
	const char* data_ptr = (cmd.data.c_str()) + s2 + 1;
	for (u32 i = 0; i < len; ++i) {
		if (vm::check_addr(addr + i, 1, vm::page_allocated | vm::page_writable)) {
			u8 val;
			int res = sscanf_s(data_ptr, "%02hhX", &val);
			if (!res) {
				gdbDebugServer.warning("Couldn't read u8 from string %s", data_ptr);
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

bool GDBDebugServer::cmd_read_all_registers(gdb_cmd & cmd)
{
	std::string result;
	select_thread(general_ops_thread_id);

	auto th = selected_thread.lock();
	if (th->id_type() == 1) {
		auto ppu = std::static_pointer_cast<ppu_thread>(th);
		//68 64-bit registers, and 3 32-bit
		result.reserve(68*16 + 3*8);
		for (int i = 0; i < 71; ++i) {
			result += get_reg(ppu, i);
		}
		return send_cmd_ack(result);
	}
	gdbDebugServer.warning("Unimplemented thread type %d", th->id_type());
	return send_cmd_ack("");
}

bool GDBDebugServer::cmd_write_all_registers(gdb_cmd & cmd)
{
	select_thread(general_ops_thread_id);
	auto th = selected_thread.lock();
	if (th->id_type() == 1) {
		auto ppu = std::static_pointer_cast<ppu_thread>(th);
		int ptr = 0;
		for (int i = 0; i < 71; ++i) {
			int sz = get_reg_size(ppu, i);
			set_reg(ppu, i, cmd.data.substr(ptr, sz * 2));
			ptr += sz * 2;
		}
		return send_cmd_ack("OK");
	}
	gdbDebugServer.warning("Unimplemented thread type %d", th->id_type());
	return send_cmd_ack("E01");
}

bool GDBDebugServer::cmd_set_thread_ops(gdb_cmd & cmd)
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
	gdbDebugServer.warning("Client asked to use thread %llx for %s, but no matching thread was found", id, type == 'c' ? "continue ops" : "general ops");
	return send_cmd_ack("E01");
}

bool GDBDebugServer::cmd_attached_to_what(gdb_cmd & cmd)
{
	//creating processes from client is not available yet
	return send_cmd_ack("1");
}

bool GDBDebugServer::cmd_kill(gdb_cmd & cmd)
{
	Emu.Stop();
	return true;
}

bool GDBDebugServer::cmd_continue_support(gdb_cmd & cmd)
{
	return send_cmd_ack("vCont;c;s;C;S");
}

bool GDBDebugServer::cmd_vcont(gdb_cmd & cmd)
{
	//todo: handle multiple actions and thread ids
	this->from_breakpoint = false;
	if (cmd.data[1] == 'c' || cmd.data[1] == 's') {
		select_thread(continue_ops_thread_id);
		auto ppu = std::static_pointer_cast<ppu_thread>(selected_thread.lock());
		paused = false;
		if (cmd.data[1] == 's') {
			ppu->state += cpu_flag::dbg_step;
		}
		ppu->state -= cpu_flag::dbg_pause;
		//special case if app didn't start yet (only loaded)
		if (!Emu.IsPaused() && !Emu.IsRunning()) {
			Emu.Run();
		}
		if (Emu.IsPaused()) {
			Emu.Resume();
		} else {
			ppu->notify();
		}
		wait_with_interrupts();
		//we are in all-stop mode
		Emu.Pause();
		select_thread(pausedBy);
		// we have to remove dbg_pause from thread that paused execution, otherwise
		// it will be paused forever (Emu.Resume only removes dbg_global_pause)
		ppu = std::static_pointer_cast<ppu_thread>(selected_thread.lock());
		ppu->state -= cpu_flag::dbg_pause;
		return send_reason();
	}
	return send_cmd_ack("");
}

static const u32 INVALID_PTR = 0xffffffff;

bool GDBDebugServer::cmd_set_breakpoint(gdb_cmd & cmd)
{
	char type = cmd.data[0];
	//software breakpoint
	if (type == '0') {
		u32 addr = INVALID_PTR;
		if (cmd.data.find(';') != std::string::npos) {
			gdbDebugServer.warning("Received request to set breakpoint with condition, but they are not supported");
			return send_cmd_ack("E01");
		}
		sscanf_s(cmd.data.c_str(), "0,%x", &addr);
		if (addr == INVALID_PTR) {
			gdbDebugServer.warning("Can't parse breakpoint request, data: %s", cmd.data.c_str());
			return send_cmd_ack("E02");
		}
		ppu_set_breakpoint(addr);
		return send_cmd_ack("OK");
	}
	//other breakpoint types are not supported
	return send_cmd_ack("");
}

bool GDBDebugServer::cmd_remove_breakpoint(gdb_cmd & cmd)
{
	char type = cmd.data[0];
	//software breakpoint
	if (type == '0') {
		u32 addr = INVALID_PTR;
		sscanf_s(cmd.data.c_str(), "0,%x", &addr);
		if (addr == INVALID_PTR) {
			gdbDebugServer.warning("Can't parse breakpoint remove request, data: %s", cmd.data.c_str());
			return send_cmd_ack("E01");
		}
		ppu_remove_breakpoint(addr);
		return send_cmd_ack("OK");
	}
	//other breakpoint types are not supported
	return send_cmd_ack("");

}

#define PROCESS_CMD(cmds,handler) if (cmd.cmd == cmds) { if (!handler(cmd)) break; else continue; }

void GDBDebugServer::on_task()
{
	sock_init();

	start_server();

	while (!stop) {
		sockaddr_in client;
		socklen_t client_len = sizeof(client);
		client_socket = accept(server_socket, (struct sockaddr *) &client, &client_len);

		if (client_socket == INVALID_SOCKET) {
			if (check_errno_again()) {
				thread_ctrl::wait_for(50);
				continue;
			}

			gdbDebugServer.error("Could not establish new connection\n");
			return;
		}
		//stop immediately
		if (Emu.IsRunning()) {
			Emu.Pause();
		}

		try {
			char hostbuf[32];
			inet_ntop(client.sin_family, reinterpret_cast<void*>(&client.sin_addr), hostbuf, 32);
			gdbDebugServer.success("Got connection to GDB debug server from %s:%d", hostbuf, client.sin_port);

			gdb_cmd cmd;

			while (!stop) {
				if (!read_cmd(cmd)) {
					break;
				}
				gdbDebugServer.trace("Command %s with data %s received", cmd.cmd.c_str(), cmd.data.c_str());
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

				gdbDebugServer.trace("Unsupported command received %s", cmd.cmd.c_str());
				if (!send_cmd_ack("")) {
					break;
				}
			}
		}
		catch (std::runtime_error& e)
		{
			if (client_socket) {
				closesocket(client_socket);
				client_socket = 0;
			}
			gdbDebugServer.error(e.what());
		}
	}
}

#undef PROCESS_CMD

void GDBDebugServer::on_exit()
{
	if (server_socket) {
		closesocket(server_socket);
	}
	if (client_socket) {
		closesocket(client_socket);
	}
	sock_quit();
}

std::string GDBDebugServer::get_name() const
{
	return "GDBDebugger";
}

void GDBDebugServer::on_stop()
{
	this->stop = true;
	//just in case we are waiting for breakpoint
	this->notify();
	named_thread::on_stop();
}

void GDBDebugServer::pause_from(cpu_thread* t) {
	if (paused) {
		return;
	}
	paused = true;
	pausedBy = t->id;
	notify();
}

u32 g_gdb_debugger_id = 0;

#ifndef _WIN32
#undef sscanf_s
#endif

#undef HEX_U32
#undef HEX_U64

#endif
