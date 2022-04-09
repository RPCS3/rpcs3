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
sys_net_error get_last_error(bool is_blocking, int native_error = 0);
sys_net_sockaddr native_addr_to_sys_net_addr(const ::sockaddr_storage& native_addr);
::sockaddr_in sys_net_addr_to_native_addr(const sys_net_sockaddr& sn_addr);
void network_clear_queue(ppu_thread& ppu);

#ifdef _WIN32
void windows_poll(pollfd* fds, unsigned long nfds, int timeout, bool* connecting);
#endif
