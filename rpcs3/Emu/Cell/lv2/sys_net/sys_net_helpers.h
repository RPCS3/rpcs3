#pragma once

#ifdef _WIN32
#include <winsock2.h>
#include <WS2tcpip.h>
#else
#ifdef __clang__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif
#include <sys/socket.h>
#include <netinet/in.h>
#ifdef __clang__
#pragma GCC diagnostic pop
#endif
#endif

#include "Emu/Cell/lv2/sys_net.h"

int get_native_error();
sys_net_error convert_error(bool is_blocking, int native_error, bool is_connecting = false);
sys_net_error get_last_error(bool is_blocking, bool is_connecting = false);
sys_net_sockaddr native_addr_to_sys_net_addr(const ::sockaddr_storage& native_addr);
::sockaddr_in sys_net_addr_to_native_addr(const sys_net_sockaddr& sn_addr);
bool is_ip_public_address(const ::sockaddr_in& addr);
u32 network_clear_queue(ppu_thread& ppu);
void clear_ppu_to_awake(ppu_thread& ppu);
be_t<u32> resolve_binding_ip();

#ifdef _WIN32
void windows_poll(std::vector<pollfd>& fds, unsigned long nfds, int timeout, std::vector<bool>& connecting);
#endif
