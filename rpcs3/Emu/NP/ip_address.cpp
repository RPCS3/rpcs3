#include "stdafx.h"
#include "Emu/system_config.h"
#include "ip_address.h"
#include "Utilities/StrFmt.h"
#include "Emu/IdManager.h"
#include "util/endian.hpp"
#include "util/types.hpp"
#include "Emu/NP/rpcn_config.h"
#include "Emu/Cell/lv2/sys_net/sys_net_helpers.h"
#include <algorithm>

#ifndef _WIN32
#include <unistd.h>
#include <fcntl.h>
#endif

LOG_CHANNEL(IPv6_log, "IPv6_layer");

namespace np
{
	bool ip_address_translator::is_ipv6(u32 addr_be)
	{
		const u32 addr = std::bit_cast<u32, be_t<u32>>(addr_be);
		return (addr >= translation_range_begin && addr <= translation_range_end);
	}

	u32 ip_address_translator::register_ipv6(const std::array<u8, 16>& ipv6_addr)
	{
		if (ipv6_addr[10] == 0xFF && ipv6_addr[11] == 0xFF &&
			std::all_of(ipv6_addr.begin(), ipv6_addr.begin() + 10, [](u8 val)
				{
					return val == 0;
				}))
		{
			// IPv4 over IPv6
			u32 ip_addr{};
			std::memcpy(&ip_addr, &ipv6_addr[12], sizeof(u32));
			return ip_addr;
		}

		std::lock_guard lock(mutex);

		const auto it = ipv6_to_ipv4.find(ipv6_addr);

		if (it == ipv6_to_ipv4.end())
		{
			ensure(cur_addr != translation_range_end);
			const u32 translation_ip = cur_addr++;
			const u32 translation_ip_be = std::bit_cast<u32, be_t<u32>>(translation_ip);
			ipv6_to_ipv4.emplace(ipv6_addr, translation_ip_be);
			ipv4_to_ipv6.push_back(ipv6_addr);
			return translation_ip_be;
		}

		return it->second;
	}

	sockaddr_in6 ip_address_translator::get_ipv6_sockaddr(u32 addr_be, u16 port_be)
	{
		const u32 addr_native = std::bit_cast<u32, be_t<u32>>(addr_be);
		const auto ipv6_addr = ::at32(ipv4_to_ipv6, addr_native - translation_range_begin);

		sockaddr_in6 sockaddr_ipv6{.sin6_family = AF_INET6, .sin6_port = port_be};
		std::memcpy(&sockaddr_ipv6.sin6_addr, ipv6_addr.data(), ipv6_addr.size());

		return sockaddr_ipv6;
	}

	u32 register_ip(const flatbuffers::Vector<std::uint8_t>* vec)
	{
		if (vec->size() == 4)
		{
			const u32 ip = static_cast<u32>(vec->Get(0)) << 24 | static_cast<u32>(vec->Get(1)) << 16 |
			               static_cast<u32>(vec->Get(2)) << 8 | static_cast<u32>(vec->Get(3));

			u32 result_ip = std::bit_cast<u32, be_t<u32>>(ip);

			return result_ip;
		}
		else if (vec->size() == 16)
		{
			std::array<u8, 16> ipv6_addr{};
			std::memcpy(ipv6_addr.data(), vec->Data(), 16);

			auto& translator = g_fxo->get<np::ip_address_translator>();
			return translator.register_ipv6(ipv6_addr);
		}
		else
		{
			fmt::throw_exception("Received ip address with size = %d", vec->size());
		}
	}

	struct ipv6_support_state
	{
		ipv6_support_state() = default;
		ipv6_support_state(const ipv6_support_state&) = delete;
		ipv6_support_state& operator=(const ipv6_support_state&) = delete;

		atomic_t<IPV6_SUPPORT> status = IPV6_SUPPORT::IPV6_UNKNOWN;
	};

	// The conditions for IPv6 to be enabled are, in order:
	// -IPv6 is not disabled in config
	// -Internet config is Connected
	// -PSN config is RPCN
	// -Bind IP is not set
	// -RPCN host has an IPv6
	// -Can connect to ipv6.google.com:413
	bool is_ipv6_supported(std::optional<IPV6_SUPPORT> force_state)
	{
		auto& ipv6_support = g_fxo->get<ipv6_support_state>();

		if (force_state)
			ipv6_support.status = *force_state;

		if (ipv6_support.status != IPV6_SUPPORT::IPV6_UNKNOWN)
			return ipv6_support.status == IPV6_SUPPORT::IPV6_SUPPORTED;

		static shared_mutex mtx;
		std::lock_guard lock(mtx);

		if (ipv6_support.status != IPV6_SUPPORT::IPV6_UNKNOWN)
			return ipv6_support.status == IPV6_SUPPORT::IPV6_SUPPORTED;

		auto notice_and_disable = [&](std::string_view reason) -> bool
		{
			IPv6_log.notice("is_ipv6_supported(): disabled cause: %s", reason);
			ipv6_support.status = IPV6_SUPPORT::IPV6_UNSUPPORTED;
			return false;
		};

		// IPv6 feature is only used by RPCN
		if (!g_cfg_rpcn.get_ipv6_support())
			return notice_and_disable("force-disabled through config");

		if (g_cfg.net.net_active != np_internet_status::enabled || g_cfg.net.psn_status != np_psn_status::psn_rpcn)
			return notice_and_disable("RPCN is disabled");

		if (resolve_binding_ip())
			return notice_and_disable("Bind IP is used");

		addrinfo* addr_info{};
		socket_type socket_ipv6{};
		const addrinfo hints{.ai_family = AF_INET6};

		auto cleanup = [&]()
		{
			if (socket_ipv6)
				close_socket(socket_ipv6);

			if (addr_info)
				freeaddrinfo(addr_info);
		};

		auto error_and_disable = [&](std::string_view message) -> bool
		{
			IPv6_log.error("is_ipv6_supported(): disabled cause: %s", message);
			ipv6_support.status = IPV6_SUPPORT::IPV6_UNSUPPORTED;
			cleanup();
			return false;
		};

		auto get_ipv6 = [](const addrinfo* addr_info) -> const addrinfo*
		{
			const addrinfo* found = addr_info;

			while (found != nullptr)
			{
				if (found->ai_family == AF_INET6)
					break;

				found = found->ai_next;
			}

			return found;
		};

		// Check if RPCN has an IPv6 address
		const auto parsed_host = parse_rpcn_host(g_cfg_rpcn.get_host());

		if (!parsed_host)
			return error_and_disable("failed to parse RPCN host");

		const auto [hostname, _] = *parsed_host;

		if (getaddrinfo(hostname.c_str(), nullptr, &hints, &addr_info))
			return error_and_disable("failed to resolve RPCN host");

		if (!get_ipv6(addr_info))
			return error_and_disable("RPCN host doesn't support IPv6");

		freeaddrinfo(addr_info);
		addr_info = {};

		// We try to connect to ipv6.google.com:8080
		if (getaddrinfo("ipv6.google.com", nullptr, &hints, &addr_info))
			return error_and_disable("failed to resolve ipv6.google.com");

		const auto* google_ipv6_addr = get_ipv6(addr_info);

		if (!google_ipv6_addr)
			return error_and_disable("failed to find IPv6 for ipv6.google.com");

		socket_type socket_or_err = ::socket(AF_INET6, SOCK_STREAM, 0);

#ifdef _WIN32
		if (socket_or_err == INVALID_SOCKET)
#else
		if (socket_or_err == -1)
#endif
			return error_and_disable("Failed to create IPv6 socket!");

		socket_ipv6 = socket_or_err;
		sockaddr_in6 ipv6_addr = *reinterpret_cast<const sockaddr_in6*>(google_ipv6_addr->ai_addr);
		ipv6_addr.sin6_port = std::bit_cast<u16, be_t<u16>>(443);

		if (::connect(socket_ipv6, reinterpret_cast<const sockaddr*>(&ipv6_addr), sizeof(ipv6_addr)) != 0)
			return error_and_disable("Failed to connect to ipv6.google.com");

		cleanup();

		IPv6_log.success("Successfully tested IPv6 support!");
		ipv6_support.status = IPV6_SUPPORT::IPV6_SUPPORTED;
		return true;
	}

	s32 sendto_possibly_ipv6(socket_type native_socket, const char* data, u32 size, const sockaddr_in* addr, int native_flags)
	{
		if (is_ipv6_supported())
		{
			sockaddr_in6 addr_ipv6{};

			if (np::ip_address_translator::is_ipv6(addr->sin_addr.s_addr))
			{
				auto& translator = g_fxo->get<np::ip_address_translator>();
				addr_ipv6 = translator.get_ipv6_sockaddr(addr->sin_addr.s_addr, addr->sin_port);
			}
			else
			{
				addr_ipv6 = sockaddr_to_sockaddr6(*addr);
			}

			return ::sendto(native_socket, data, size, native_flags, reinterpret_cast<const sockaddr*>(&addr_ipv6), sizeof(sockaddr_in6));
		}

		return ::sendto(native_socket, data, size, native_flags, reinterpret_cast<const sockaddr*>(addr), sizeof(sockaddr_in));
	}

	sockaddr_in6 sockaddr_to_sockaddr6(const sockaddr_in& addr)
	{
		sockaddr_in6 addr_ipv6{.sin6_family = AF_INET6, .sin6_port = addr.sin_port};
		std::array<u8, 16> ipv6{};

		// IPv4 over IPv6
		ipv6[10] = 0xFF;
		ipv6[11] = 0xFF;
		std::memcpy(ipv6.data() + 12, &addr.sin_addr.s_addr, 4);
		std::memcpy(&addr_ipv6.sin6_addr, ipv6.data(), ipv6.size());
		return addr_ipv6;
	}

	sockaddr_in sockaddr6_to_sockaddr(const sockaddr_in6& addr)
	{
		sockaddr_in addr_ipv4{.sin_family = AF_INET, .sin_port = addr.sin6_port};

		std::array<u8, 16> ipv6{};
		std::memcpy(ipv6.data(), &addr.sin6_addr, 16);
		auto& translator = g_fxo->get<np::ip_address_translator>();
		addr_ipv4.sin_addr.s_addr = translator.register_ipv6(ipv6);

		return addr_ipv4;
	}

	void close_socket(socket_type socket)
	{
		if (socket)
		{
#ifdef _WIN32
			::closesocket(socket);
#else
			::close(socket);
#endif
		}
	}

	void set_socket_non_blocking(socket_type socket)
	{
		if (socket)
		{
#ifdef _WIN32
			u_long _true = 1;
			::ioctlsocket(socket, FIONBIO, &_true);
#else
			::fcntl(socket, F_SETFL, ::fcntl(socket, F_GETFL, 0) | O_NONBLOCK);
#endif
		}
	}
} // namespace np
