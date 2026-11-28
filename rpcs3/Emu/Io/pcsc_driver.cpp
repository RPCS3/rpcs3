#include "stdafx.h"
#include "pcsc_driver.h"
#include "util/logs.hpp"
#include "Utilities/StrUtil.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <charconv>
#include <chrono>
#include <cstring>
#include <mutex>

LOG_CHANNEL(nfc_log, "NFC");

#if defined(_WIN32)

#include <windows.h>
#include <winscard.h>

#if defined(_MSC_VER)
#pragma comment(lib, "winscard.lib")
#endif

#ifndef SCARD_CTL_CODE
#define SCARD_CTL_CODE(code) CTL_CODE(FILE_DEVICE_SMARTCARD, (code), METHOD_BUFFERED, FILE_ANY_ACCESS)
#endif

// When UNICODE is defined in RPCS3, standard SCard functions map to wide-char (W).
// Redirect them to ANSI (A) so all call sites can use clean, non-suffixed SCard* names across all platforms.
#undef SCardListReaders
#undef SCardConnect
#undef SCardStatus
#undef SCardGetStatusChange
#define SCardListReaders SCardListReadersA
#define SCardConnect SCardConnectA
#define SCardStatus SCardStatusA
#define SCardGetStatusChange SCardGetStatusChangeA

namespace
{
	using pcsc_context_t = SCARDCONTEXT;
	using pcsc_handle_t = SCARDHANDLE;
	using pcsc_dword_t = DWORD;
	using pcsc_long_t = LONG;
	using pcsc_reader_state_t = SCARD_READERSTATEA;
	using pcsc_io_request_t = SCARD_IO_REQUEST;

	constexpr pcsc_dword_t C_SCOPE_USER = SCARD_SCOPE_USER;
	constexpr pcsc_dword_t C_SCOPE_SYSTEM = SCARD_SCOPE_SYSTEM;
	constexpr pcsc_dword_t C_SHARE_SHARED = SCARD_SHARE_SHARED;
	constexpr pcsc_dword_t C_SHARE_DIRECT = SCARD_SHARE_DIRECT;
	constexpr pcsc_dword_t C_PROTO_UNDEFINED = SCARD_PROTOCOL_UNDEFINED;
	constexpr pcsc_dword_t C_PROTO_T0 = SCARD_PROTOCOL_T0;
	constexpr pcsc_dword_t C_PROTO_T1 = SCARD_PROTOCOL_T1;
	constexpr pcsc_dword_t C_LEAVE_CARD = SCARD_LEAVE_CARD;
	constexpr pcsc_dword_t C_RESET_CARD = SCARD_RESET_CARD;
	constexpr pcsc_dword_t C_UNPOWER_CARD = SCARD_UNPOWER_CARD;
	constexpr pcsc_dword_t C_STATE_UNAWARE = SCARD_STATE_UNAWARE;
	constexpr pcsc_dword_t C_STATE_UNKNOWN = SCARD_STATE_UNKNOWN;
	constexpr pcsc_dword_t C_STATE_UNAVAILABLE = SCARD_STATE_UNAVAILABLE;
	constexpr pcsc_dword_t C_STATE_PRESENT = SCARD_STATE_PRESENT;
	constexpr pcsc_dword_t C_STATUS_ABSENT = SCARD_ABSENT;
	constexpr pcsc_long_t C_SUCCESS = SCARD_S_SUCCESS;
	constexpr pcsc_long_t C_NO_READERS = static_cast<LONG>(0x8010002E);
	constexpr pcsc_dword_t CCID_ESCAPE_CTL = SCARD_CTL_CODE(3500);

	inline const SCARD_IO_REQUEST* get_pci(pcsc_dword_t proto)
	{
		return (proto == SCARD_PROTOCOL_T0) ? SCARD_PCI_T0 : SCARD_PCI_T1;
	}
} // namespace

#elif defined(__APPLE__)

#include <PCSC/winscard.h>
#include <PCSC/wintypes.h>

namespace
{
	using pcsc_context_t = SCARDCONTEXT;
	using pcsc_handle_t = SCARDHANDLE;
	using pcsc_dword_t = DWORD;
	using pcsc_long_t = LONG;
	using pcsc_reader_state_t = SCARD_READERSTATE;
	using pcsc_io_request_t = SCARD_IO_REQUEST;

	constexpr pcsc_dword_t C_SCOPE_USER = SCARD_SCOPE_USER;
	constexpr pcsc_dword_t C_SCOPE_SYSTEM = SCARD_SCOPE_SYSTEM;
	constexpr pcsc_dword_t C_SHARE_SHARED = SCARD_SHARE_SHARED;
	constexpr pcsc_dword_t C_SHARE_DIRECT = SCARD_SHARE_DIRECT;
	constexpr pcsc_dword_t C_PROTO_UNDEFINED = SCARD_PROTOCOL_UNDEFINED;
	constexpr pcsc_dword_t C_PROTO_T0 = SCARD_PROTOCOL_T0;
	constexpr pcsc_dword_t C_PROTO_T1 = SCARD_PROTOCOL_T1;
	constexpr pcsc_dword_t C_LEAVE_CARD = SCARD_LEAVE_CARD;
	constexpr pcsc_dword_t C_RESET_CARD = SCARD_RESET_CARD;
	constexpr pcsc_dword_t C_UNPOWER_CARD = SCARD_UNPOWER_CARD;
	constexpr pcsc_dword_t C_STATE_UNAWARE = SCARD_STATE_UNAWARE;
	constexpr pcsc_dword_t C_STATE_UNKNOWN = SCARD_STATE_UNKNOWN;
	constexpr pcsc_dword_t C_STATE_UNAVAILABLE = SCARD_STATE_UNAVAILABLE;
	constexpr pcsc_dword_t C_STATE_PRESENT = SCARD_STATE_PRESENT;
	constexpr pcsc_dword_t C_STATUS_ABSENT = SCARD_ABSENT;
	constexpr pcsc_long_t C_SUCCESS = SCARD_S_SUCCESS;
	constexpr pcsc_long_t C_NO_READERS = static_cast<LONG>(0x8010002E);
	constexpr pcsc_dword_t CCID_ESCAPE_CTL = (0x42000000 + 3500);

	inline const SCARD_IO_REQUEST* get_pci(pcsc_dword_t proto)
	{
		return (proto == SCARD_PROTOCOL_T0) ? SCARD_PCI_T0 : SCARD_PCI_T1;
	}
} // namespace

#else // Linux / BSD: dynamically load libpcsclite

#include <dlfcn.h>

namespace
{
	using pcsc_context_t = std::intptr_t;
	using pcsc_handle_t = std::intptr_t;
	using pcsc_dword_t = ulong;
	using pcsc_long_t = long;

	struct pcsc_io_request_t
	{
		pcsc_dword_t dwProtocol;
		pcsc_dword_t cbPciLength;
	};

	struct pcsc_reader_state_t
	{
		const char* szReader;
		void* pvUserData;
		pcsc_dword_t dwCurrentState;
		pcsc_dword_t dwEventState;
		pcsc_dword_t cbAtr;
		u8 rgbAtr[33];
	};

	constexpr pcsc_dword_t C_SCOPE_USER = 0;
	constexpr pcsc_dword_t C_SCOPE_SYSTEM = 2;
	constexpr pcsc_dword_t C_SHARE_SHARED = 2;
	constexpr pcsc_dword_t C_SHARE_DIRECT = 3;
	constexpr pcsc_dword_t C_PROTO_UNDEFINED = 0;
	constexpr pcsc_dword_t C_PROTO_T0 = 1;
	constexpr pcsc_dword_t C_PROTO_T1 = 2;
	constexpr pcsc_dword_t C_LEAVE_CARD = 0;
	constexpr pcsc_dword_t C_RESET_CARD = 1;
	constexpr pcsc_dword_t C_UNPOWER_CARD = 2;
	constexpr pcsc_dword_t C_STATE_UNAWARE = 0;
	constexpr pcsc_dword_t C_STATE_UNKNOWN = 0x00000004;
	constexpr pcsc_dword_t C_STATE_UNAVAILABLE = 0x00000008;
	constexpr pcsc_dword_t C_STATE_PRESENT = 0x00000020;
	constexpr pcsc_dword_t C_STATUS_ABSENT = 2; // pcsc-lite SCARD_ABSENT bitmask (Windows uses enum value 1 via winscard.h)
	constexpr pcsc_long_t C_SUCCESS = 0;
	constexpr pcsc_long_t C_NO_READERS = static_cast<pcsc_long_t>(0x8010002E);
	constexpr pcsc_long_t C_NO_SERVICE = static_cast<pcsc_long_t>(0x8010001D);
	constexpr pcsc_dword_t CCID_ESCAPE_CTL = (0x42000000 + 3500);

	// Dynamic API loader for libpcsclite on Linux / BSD systems.
	// Loaded lazily to avoid static initialization / destruction overhead or crashes when PCSC is not installed.
	struct pcsc_dyn_api
	{
		void* handle = nullptr;
		pcsc_long_t (*establish_context)(pcsc_dword_t, const void*, const void*, pcsc_context_t*) = nullptr;
		pcsc_long_t (*release_context)(pcsc_context_t) = nullptr;
		pcsc_long_t (*list_readers)(pcsc_context_t, const char*, char*, pcsc_dword_t*) = nullptr;
		pcsc_long_t (*connect)(pcsc_context_t, const char*, pcsc_dword_t, pcsc_dword_t, pcsc_handle_t*, pcsc_dword_t*) = nullptr;
		pcsc_long_t (*reconnect)(pcsc_handle_t, pcsc_dword_t, pcsc_dword_t, pcsc_dword_t, pcsc_dword_t*) = nullptr;
		pcsc_long_t (*disconnect)(pcsc_handle_t, pcsc_dword_t) = nullptr;
		pcsc_long_t (*status)(pcsc_handle_t, char*, pcsc_dword_t*, pcsc_dword_t*, pcsc_dword_t*, u8*, pcsc_dword_t*) = nullptr;
		pcsc_long_t (*get_status_change)(pcsc_context_t, pcsc_dword_t, pcsc_reader_state_t*, pcsc_dword_t) = nullptr;
		pcsc_long_t (*control)(pcsc_handle_t, pcsc_dword_t, const void*, pcsc_dword_t, void*, pcsc_dword_t, pcsc_dword_t*) = nullptr;
		pcsc_long_t (*transmit)(pcsc_handle_t, const pcsc_io_request_t*, const u8*, pcsc_dword_t, pcsc_io_request_t*, u8*, pcsc_dword_t*) = nullptr;
		pcsc_long_t (*get_attrib)(pcsc_handle_t, pcsc_dword_t, u8*, pcsc_dword_t*) = nullptr;
		pcsc_io_request_t* pci_t0 = nullptr;
		pcsc_io_request_t* pci_t1 = nullptr;

		std::once_flag init_flag;
		bool init_success = false;

		~pcsc_dyn_api()
		{
			// dlclose(nullptr) causes SIGSEGV on glibc; only close if a library was actually loaded
			if (handle)
			{
				dlclose(handle);
				handle = nullptr;
			}
		}

		template <typename T>
		T get_sym(const char* name) const
		{
			return reinterpret_cast<T>(dlsym(handle, name));
		}

		bool init()
		{
			std::call_once(init_flag, [this]()
			{
				handle = dlopen("libpcsclite.so.1", RTLD_LAZY);
				if (!handle)
				{
					handle = dlopen("libpcsclite.so", RTLD_LAZY);
				}
				if (!handle)
				{
					init_success = false;
					return;
				}

				establish_context = get_sym<decltype(establish_context)>("SCardEstablishContext");
				release_context   = get_sym<decltype(release_context)>("SCardReleaseContext");
				list_readers      = get_sym<decltype(list_readers)>("SCardListReaders");
				connect           = get_sym<decltype(connect)>("SCardConnect");
				reconnect         = get_sym<decltype(reconnect)>("SCardReconnect");
				disconnect        = get_sym<decltype(disconnect)>("SCardDisconnect");
				status            = get_sym<decltype(status)>("SCardStatus");
				get_status_change = get_sym<decltype(get_status_change)>("SCardGetStatusChange");
				control           = get_sym<decltype(control)>("SCardControl");
				transmit          = get_sym<decltype(transmit)>("SCardTransmit");
				get_attrib        = get_sym<decltype(get_attrib)>("SCardGetAttrib");
				pci_t0            = get_sym<pcsc_io_request_t*>("g_rgSCardT0Pci");
				pci_t1            = get_sym<pcsc_io_request_t*>("g_rgSCardT1Pci");

				init_success = establish_context && release_context && list_readers && connect && disconnect && transmit;
			});

			return init_success;
		}
	};

	inline pcsc_dyn_api& get_pcsc_dyn()
	{
		static pcsc_dyn_api s_instance;
		return s_instance;
	}

	inline pcsc_long_t SCardEstablishContext(pcsc_dword_t scope, const void* r1, const void* r2, pcsc_context_t* ctx)
	{
		auto& api = get_pcsc_dyn();
		if (!api.init() || !api.establish_context)
		{
			return C_NO_SERVICE;
		}

		return api.establish_context(scope, r1, r2, ctx);
	}

	inline pcsc_long_t SCardReleaseContext(pcsc_context_t ctx)
	{
		auto& api = get_pcsc_dyn();
		if (!api.init() || !api.release_context)
		{
			return C_NO_SERVICE;
		}

		return api.release_context(ctx);
	}

	inline pcsc_long_t SCardListReaders(pcsc_context_t ctx, const char* grp, char* readers, pcsc_dword_t* len)
	{
		auto& api = get_pcsc_dyn();
		if (!api.init() || !api.list_readers)
		{
			return C_NO_SERVICE;
		}

		return api.list_readers(ctx, grp, readers, len);
	}

	inline pcsc_long_t SCardConnect(pcsc_context_t ctx, const char* reader, pcsc_dword_t share, pcsc_dword_t pref, pcsc_handle_t* card, pcsc_dword_t* active)
	{
		auto& api = get_pcsc_dyn();
		if (!api.init() || !api.connect)
		{
			return C_NO_SERVICE;
		}

		return api.connect(ctx, reader, share, pref, card, active);
	}

	inline pcsc_long_t SCardReconnect(pcsc_handle_t card, pcsc_dword_t share, pcsc_dword_t pref, pcsc_dword_t init, pcsc_dword_t* active)
	{
		auto& api = get_pcsc_dyn();
		if (!api.init() || !api.reconnect)
		{
			return C_NO_SERVICE;
		}

		return api.reconnect(card, share, pref, init, active);
	}

	inline pcsc_long_t SCardDisconnect(pcsc_handle_t card, pcsc_dword_t disp)
	{
		auto& api = get_pcsc_dyn();
		if (!api.init() || !api.disconnect)
		{
			return C_NO_SERVICE;
		}

		return api.disconnect(card, disp);
	}

	inline pcsc_long_t SCardStatus(pcsc_handle_t card, char* reader, pcsc_dword_t* rlen, pcsc_dword_t* state, pcsc_dword_t* proto, u8* atr, pcsc_dword_t* atrlen)
	{
		auto& api = get_pcsc_dyn();
		if (!api.init() || !api.status)
		{
			return C_NO_SERVICE;
		}

		return api.status(card, reader, rlen, state, proto, atr, atrlen);
	}

	inline pcsc_long_t SCardGetStatusChange(pcsc_context_t ctx, pcsc_dword_t timeout, pcsc_reader_state_t* states, pcsc_dword_t count)
	{
		auto& api = get_pcsc_dyn();
		if (!api.init() || !api.get_status_change)
		{
			return C_NO_SERVICE;
		}

		return api.get_status_change(ctx, timeout, states, count);
	}

	inline pcsc_long_t SCardControl(pcsc_handle_t card, pcsc_dword_t code, const void* in_buf, pcsc_dword_t in_len, void* out_buf, pcsc_dword_t out_len, pcsc_dword_t* ret_len)
	{
		auto& api = get_pcsc_dyn();
		if (!api.init() || !api.control)
		{
			return C_NO_SERVICE;
		}

		return api.control(card, code, in_buf, in_len, out_buf, out_len, ret_len);
	}

	inline pcsc_long_t SCardTransmit(pcsc_handle_t card, const pcsc_io_request_t* pci, const u8* s_buf, pcsc_dword_t s_len, pcsc_io_request_t* r_pci, u8* r_buf, pcsc_dword_t* r_len)
	{
		auto& api = get_pcsc_dyn();
		if (!api.init() || !api.transmit)
		{
			return C_NO_SERVICE;
		}

		return api.transmit(card, pci, s_buf, s_len, r_pci, r_buf, r_len);
	}

	inline pcsc_long_t SCardGetAttrib(pcsc_handle_t card, pcsc_dword_t attr_id, u8* attr, pcsc_dword_t* attr_len)
	{
		auto& api = get_pcsc_dyn();
		if (!api.init() || !api.get_attrib)
		{
			return C_NO_SERVICE;
		}

		return api.get_attrib(card, attr_id, attr, attr_len);
	}

	inline const pcsc_io_request_t* get_pci(pcsc_dword_t proto)
	{
		auto& api = get_pcsc_dyn();
		api.init();
		static const pcsc_io_request_t fallback_t0 = {C_PROTO_T0, static_cast<pcsc_dword_t>(sizeof(pcsc_io_request_t))};
		static const pcsc_io_request_t fallback_t1 = {C_PROTO_T1, static_cast<pcsc_dword_t>(sizeof(pcsc_io_request_t))};
		if (proto == C_PROTO_T0)
		{
			return api.pci_t0 ? api.pci_t0 : &fallback_t0;
		}
		return api.pci_t1 ? api.pci_t1 : &fallback_t1;
	}
} // namespace

#endif

#ifndef SCARD_ATTR_VALUE
#define SCARD_ATTR_VALUE(Class, Tag) ((static_cast<pcsc_dword_t>(Class) << 16) | static_cast<pcsc_dword_t>(Tag))
#endif

#ifndef SCARD_CLASS_VENDOR_INFO
#define SCARD_CLASS_VENDOR_INFO 1
#endif
#ifndef SCARD_CLASS_SYSTEM
#define SCARD_CLASS_SYSTEM 0x7fff
#endif

#ifndef SCARD_ATTR_VENDOR_NAME
#define SCARD_ATTR_VENDOR_NAME SCARD_ATTR_VALUE(SCARD_CLASS_VENDOR_INFO, 0x0100)
#endif
#ifndef SCARD_ATTR_VENDOR_IFD_TYPE
#define SCARD_ATTR_VENDOR_IFD_TYPE SCARD_ATTR_VALUE(SCARD_CLASS_VENDOR_INFO, 0x0101)
#endif
#ifndef SCARD_ATTR_DEVICE_SYSTEM_NAME_A
#define SCARD_ATTR_DEVICE_SYSTEM_NAME_A SCARD_ATTR_VALUE(SCARD_CLASS_SYSTEM, 0x0004)
#endif

namespace
{
	namespace identiv
	{
		// USB Vendor IDs (CLOUD.inf and uTrust 370xF User Manual Section 4.2.3.2)
		constexpr u16 VENDOR_ID_IDENTIV = 0x04E6; // Identiv / SCM Microsystems, Inc.
		constexpr u16 VENDOR_ID_CHICONY = 0x04F2; // Chicony (OEM keyboard with Identiv chip)

		constexpr u16 PID_CHICONY_KUS0967 = 0x0967;
		constexpr u16 PID_SCL3711         = 0x5411;
		constexpr u16 PID_CLOUD_2190F     = 0x5750;

		inline bool is_identiv_pid(u16 pid)
		{
			return pid == PID_SCL3711 ||
				pid == PID_CLOUD_2190F ||
				(pid >= 0x5601 && pid <= 0x5614) || // uTrust 5500, 5501, 3720, 3721 & DFU
				(pid >= 0x5700 && pid <= 0x5728) || // uTrust 2700, 4700, 4701, 4710, 4711, CLOUD 4000, SmartOS & DFU
				(pid >= 0x5790 && pid <= 0x5792) || // uTrust / CLOUD 3700 F, 3701 F, SmartOS CL
				(pid >= 0x5810 && pid <= 0x5826);   // uTrust 2700 R, 2900 R, 3512, 3522, SCR3500, myAuth, Flex
		}

		inline bool is_identiv_vid_pid(u16 vid, u16 pid)
		{
			if (vid == VENDOR_ID_IDENTIV)
			{
				// All smartcard devices under USB VID 0x04E6 are Identiv / SCM Microsystems
				return pid == 0 || is_identiv_pid(pid);
			}

			return vid == VENDOR_ID_CHICONY && pid == PID_CHICONY_KUS0967;
		}
	} // namespace identiv

	inline bool parse_usb_vid_pid(std::string_view str, u16& out_vid, u16& out_pid)
	{
		if (str.empty())
		{
			return false;
		}

		const std::string lower = fmt::to_lower(str);

		const auto find_prefix_end = [&](std::initializer_list<std::string_view> prefixes) -> usz
		{
			for (const std::string_view prefix : prefixes)
			{
				if (const usz pos = lower.find(prefix); pos != std::string::npos)
				{
					return pos + prefix.size();
				}
			}
			return std::string::npos;
		};

		const auto parse_hex4 = [&](usz pos, u16& val) -> bool
		{
			if (pos == std::string::npos || pos + 4 > lower.size())
			{
				return false;
			}
			const auto [ptr, ec] = std::from_chars(lower.data() + pos, lower.data() + pos + 4, val, 16);
			return ec == std::errc{};
		};

		// Standard Windows / PnP format: VID_04E6&PID_5790 (or vid& / vid=)
		u16 vid = 0;
		u16 pid = 0;
		if (parse_hex4(find_prefix_end({"vid_", "vid&", "vid="}), vid) &&
			parse_hex4(find_prefix_end({"pid_", "pid&", "pid="}), pid))
		{
			out_vid = vid;
			out_pid = pid;
			return true;
		}

		// Linux / macOS libusb / CCID style: "04e6:5790" or "04e6/5790"
		if (parse_hex4(find_prefix_end({"04e6:", "04e6/"}), pid))
		{
			out_vid = identiv::VENDOR_ID_IDENTIV;
			out_pid = pid;
			return true;
		}

		return false;
	}

	// Helper to query a PC/SC string attribute from a connected reader
	inline std::string get_pcsc_attribute(pcsc_handle_t card, pcsc_dword_t attr_id)
	{
		if (card == 0)
		{
			return {};
		}

		char stack_buf[512]{};
		pcsc_dword_t attr_len = static_cast<pcsc_dword_t>(sizeof(stack_buf) - 1);
		pcsc_long_t ret = SCardGetAttrib(card, attr_id, reinterpret_cast<u8*>(stack_buf), &attr_len);
		if (ret == C_SUCCESS && attr_len > 0)
		{
			return std::string(stack_buf, strnlen(stack_buf, attr_len));
		}

		// Try two-step length query directly into return string if buffer was insufficient
		attr_len = 0;
		ret = SCardGetAttrib(card, attr_id, nullptr, &attr_len);
		if (ret == C_SUCCESS && attr_len > 0)
		{
			std::string result(attr_len + 1, '\0');
			ret = SCardGetAttrib(card, attr_id, reinterpret_cast<u8*>(result.data()), &attr_len);
			if (ret == C_SUCCESS && attr_len > 0)
			{
				result.resize(strnlen(result.data(), attr_len));
				return result;
			}
		}

		return {};
	}

	inline std::span<const u8> build_short_apdu(std::array<u8, 261>& buf, u8 cla, u8 ins, u8 p1, u8 p2, std::span<const u8> data)
	{
		buf[0] = cla;
		buf[1] = ins;
		buf[2] = p1;
		buf[3] = p2;
		buf[4] = static_cast<u8>(data.size());
		if (!data.empty())
		{
			std::memcpy(buf.data() + 5, data.data(), data.size());
		}
		return {buf.data(), 5 + data.size()};
	}

	inline std::span<const u8> build_transparent_apdu(std::array<u8, 264>& buf, std::span<const u8> payload, bool with_le)
	{
		usz pos = 0;
		buf[pos++] = 0xFF;
		buf[pos++] = 0xC2;
		buf[pos++] = 0x00;
		buf[pos++] = 0x01;
		if (payload.size() <= 127)
		{
			buf[pos++] = static_cast<u8>(payload.size() + 2);
			buf[pos++] = 0x95;
			buf[pos++] = static_cast<u8>(payload.size());
		}
		else
		{
			buf[pos++] = static_cast<u8>(payload.size() + 3);
			buf[pos++] = 0x95;
			buf[pos++] = 0x81;
			buf[pos++] = static_cast<u8>(payload.size());
		}
		std::memcpy(buf.data() + pos, payload.data(), payload.size());
		pos += payload.size();
		if (with_le)
		{
			buf[pos++] = 0x00; // Le = 0x00 (ISO 7816-4 Case 4)
		}
		return {buf.data(), pos};
	}

	inline u16 extract_sw(std::span<const u8> rapdu)
	{
		return rapdu.size() >= 2 ? static_cast<u16>((rapdu[rapdu.size() - 2] << 8) | rapdu.back()) : 0;
	}

	// ISO 7816-4 Case 2 short APDU exchange with automatic SW 6C xx exact-Le retry
	inline u16 transmit_case2_apdu(pcsc_driver& driver, std::array<u8, 5>& apdu, std::vector<u8>& resp)
	{
		if (!driver.transmit_apdu(apdu, resp) || resp.size() < 2)
		{
			return 0;
		}
		u16 sw = extract_sw(resp);
		if ((sw & 0xFF00) == 0x6C00)
		{
			apdu[4] = static_cast<u8>(sw & 0xFF);
			if (!driver.transmit_apdu(apdu, resp) || resp.size() < 2)
			{
				return 0;
			}
			sw = extract_sw(resp);
		}
		return sw;
	}

	// PC/SC Part 3 Supp. 2 Transparent Exchange (FF C2 00 01), retrying without Le on SW 67 00
	inline bool transmit_transparent_apdu(pcsc_driver& driver, std::span<const u8> payload, std::vector<u8>& rapdu, usz& out_off, usz& out_len, bool allow_tag96)
	{
		out_off = 0;
		out_len = 0;
		if (payload.size() > 252)
		{
			return false;
		}

		std::array<u8, 264> trans_buf{};
		bool ok = driver.transmit_apdu(build_transparent_apdu(trans_buf, payload, true), rapdu);
		if (!ok || (rapdu.size() == 2 && extract_sw(rapdu) == 0x6700))
		{
			ok = driver.transmit_apdu(build_transparent_apdu(trans_buf, payload, false), rapdu);
		}
		if (!ok || rapdu.size() < 4 || extract_sw(rapdu) != 0x9000)
		{
			return false;
		}

		const usz tlv_end = rapdu.size() - 2;
		const usz min_97_len = allow_tag96 ? 2u : 1u;
		usz tlv_offset = 0;
		while (tlv_offset + 2 <= tlv_end)
		{
			const u8 tag = rapdu[tlv_offset];
			usz header_len = 2;
			usz len = rapdu[tlv_offset + 1];
			if (len == 0x81)
			{
				if (tlv_offset + 3 > tlv_end)
				{
					break;
				}
				header_len = 3;
				len = rapdu[tlv_offset + 2];
			}
			else if (len == 0x82)
			{
				if (tlv_offset + 4 > tlv_end)
				{
					break;
				}
				header_len = 4;
				len = (static_cast<usz>(rapdu[tlv_offset + 2]) << 8) | rapdu[tlv_offset + 3];
			}
			else if (len > 0x82)
			{
				break;
			}

			if (tlv_offset + header_len + len <= tlv_end)
			{
				if (tag == 0x97 && len >= min_97_len)
				{
					out_off = tlv_offset + header_len;
					out_len = len;
					return true;
				}
				if (allow_tag96 && tag == 0x96 && len > 2 && out_len == 0)
				{
					out_off = tlv_offset + header_len;
					out_len = len;
				}
			}
			tlv_offset += header_len + len;
		}

		return out_len > 0;
	}

	constexpr bool is_reader_unavailable_error(pcsc_long_t ret)
	{
		return ret == static_cast<pcsc_long_t>(0x80100009) /* SCARD_E_UNKNOWN_READER */ ||
			ret == static_cast<pcsc_long_t>(0x80100017) /* SCARD_E_READER_UNAVAILABLE */ ||
			ret == static_cast<pcsc_long_t>(0x8010001D) /* SCARD_E_NO_SERVICE */ ||
			ret == static_cast<pcsc_long_t>(0x8010001E) /* SCARD_E_SERVICE_STOPPED */ ||
			ret == C_NO_READERS /* SCARD_E_NO_READERS_AVAILABLE (0x8010002E) */;
	}

	constexpr bool is_recoverable_comm_error(pcsc_long_t ret)
	{
		return is_reader_unavailable_error(ret) ||
			ret == static_cast<pcsc_long_t>(0x80100003) /* SCARD_E_INVALID_HANDLE */ ||
			ret == static_cast<pcsc_long_t>(0x8010002F) /* SCARD_E_COMM_DATA_LOST */;
	}

	constexpr bool is_status_absent(pcsc_dword_t state)
	{
#if defined(_WIN32)
		return state == 0 || state == C_STATUS_ABSENT;
#else
		// pcsc-lite reports SCardStatus state as a bitmask (SCARD_UNKNOWN=0x01, SCARD_ABSENT=0x02)
		return state <= 0x0001 || (state & C_STATUS_ABSENT) != 0;
#endif
	}
} // namespace

struct pcsc_driver::impl
{
	pcsc_context_t m_context = 0;
	pcsc_handle_t m_card = 0;
	pcsc_dword_t m_active_protocol = C_PROTO_UNDEFINED;
	std::string m_reader_name;
	std::recursive_mutex m_mutex;
	std::atomic<bool> m_is_open{false};
	bool m_reader_disconnected = false;
	std::chrono::steady_clock::time_point m_last_reconnect_attempt{};
	bool m_card_info_cached = false;
	pcsc_card_info m_cached_card_info{};
	pcsc_dword_t m_last_event_state = 0;
	pcsc_dword_t m_cached_event_state = 0;
	std::vector<u8> m_last_event_atr;
	bool m_in_part3_mode = false;
	bool m_reader_configured = false;
	bool m_reader_config_attempted_direct = false;
	bool m_reader_config_attempted_shared = false;
	bool m_ioctl_escape_blocked = false;
	u16 m_vendor_id = 0;
	u16 m_product_id = 0;
	bool m_is_identiv = false;
	bool m_identiv_checked = false;

	struct failed_auth_entry
	{
		std::array<u8, 16> uid{};
		u8 uid_len = 0;
		u8 sector = 0;
		u8 key_type = 0;
		std::array<u8, 6> key{};
	};
	static constexpr usz max_failed_auth_cache = 64;
	std::array<failed_auth_entry, max_failed_auth_cache> m_failed_auth_cache{};
	usz m_failed_auth_count = 0;

	bool is_auth_failed_cached(std::span<const u8> uid, u8 sector, u8 key_type, std::span<const u8, 6> key) const
	{
		if (uid.empty() || uid.size() > 16)
		{
			return false;
		}
		for (usz i = 0; i < m_failed_auth_count; ++i)
		{
			const auto& entry = m_failed_auth_cache[i];
			if (entry.sector == sector && entry.key_type == key_type &&
				entry.uid_len == uid.size() && std::memcmp(entry.uid.data(), uid.data(), uid.size()) == 0 &&
				std::memcmp(entry.key.data(), key.data(), 6) == 0)
			{
				return true;
			}
		}
		return false;
	}

	void record_failed_auth(std::span<const u8> uid, u8 sector, u8 key_type, std::span<const u8, 6> key)
	{
		if (uid.empty() || uid.size() > 16 || is_auth_failed_cached(uid, sector, key_type, key))
		{
			return;
		}
		if (m_failed_auth_count >= max_failed_auth_cache)
		{
			std::move(m_failed_auth_cache.begin() + 1, m_failed_auth_cache.begin() + max_failed_auth_cache, m_failed_auth_cache.begin());
			--m_failed_auth_count;
		}
		auto& entry = m_failed_auth_cache[m_failed_auth_count++];
		std::memcpy(entry.uid.data(), uid.data(), uid.size());
		entry.uid_len = static_cast<u8>(uid.size());
		entry.sector = sector;
		entry.key_type = key_type;
		std::memcpy(entry.key.data(), key.data(), 6);
	}

	void invalidate_failed_auth(std::span<const u8> uid)
	{
		if (uid.empty() || uid.size() > 16)
		{
			m_failed_auth_count = 0;
			return;
		}
		usz write_idx = 0;
		for (usz i = 0; i < m_failed_auth_count; ++i)
		{
			const auto& entry = m_failed_auth_cache[i];
			const bool match = (entry.uid_len == uid.size() && std::memcmp(entry.uid.data(), uid.data(), uid.size()) == 0);
			if (!match)
			{
				if (write_idx != i)
				{
					m_failed_auth_cache[write_idx] = entry;
				}
				++write_idx;
			}
		}
		m_failed_auth_count = write_idx;
	}

	void clear_failed_auth_cache()
	{
		m_failed_auth_count = 0;
	}

	// Hardware-only Identiv reader detection (checks local attributes before Escape 0x12 I/O)
	bool is_identiv(pcsc_driver* driver = nullptr)
	{
		if (m_identiv_checked)
		{
			return m_is_identiv;
		}

		if (m_reader_name.empty())
		{
			return false;
		}

		// Layer 1: USB VID/PID via PC/SC SCARD_ATTR_DEVICE_SYSTEM_NAME_A
		if (m_card != 0)
		{
			const std::string sys_name = get_pcsc_attribute(m_card, SCARD_ATTR_DEVICE_SYSTEM_NAME_A);
			if (!sys_name.empty())
			{
				u16 vid = 0;
				u16 pid = 0;
				if (parse_usb_vid_pid(sys_name, vid, pid))
				{
					m_vendor_id = vid;
					m_product_id = pid;
					if (identiv::is_identiv_vid_pid(vid, pid))
					{
						m_is_identiv = true;
						m_identiv_checked = true;
						return true;
					}
				}
			}
		}

		// Layer 2: Hardware Escape Query: READER_GET_IFDTYPE (Escape 0x12, uTrust Manual Section 6.3.3.3)
		if (driver && m_card != 0)
		{
			const std::array<u8, 1> ifd_cmd{0x12};
			std::vector<u8> ifd_resp;
			if (driver->send_escape(ifd_cmd, ifd_resp) && ifd_resp.size() >= 2)
			{
				const u16 pid = static_cast<u16>(ifd_resp[0] | (static_cast<u16>(ifd_resp[1]) << 8));
				if (identiv::is_identiv_pid(pid))
				{
					m_vendor_id = identiv::VENDOR_ID_IDENTIV;
					m_product_id = pid;
					m_is_identiv = true;
					m_identiv_checked = true;
					return true;
				}
			}
		}

		if (m_card != 0 && (m_active_protocol == C_PROTO_T0 || m_active_protocol == C_PROTO_T1))
		{
			m_identiv_checked = true;
		}

		return false;
	}

	bool context_valid() const
	{
		return m_context != 0;
	}

	bool ensure_context()
	{
		if (context_valid())
		{
			return true;
		}

		pcsc_long_t ret = SCardEstablishContext(C_SCOPE_USER, nullptr, nullptr, &m_context);
		if (ret != C_SUCCESS)
		{
			ret = SCardEstablishContext(C_SCOPE_SYSTEM, nullptr, nullptr, &m_context);
		}

		if (ret != C_SUCCESS)
		{
			m_context = 0;
			return false;
		}
		return true;
	}

	void release_context()
	{
		if (m_context != 0)
		{
			SCardReleaseContext(m_context);
			m_context = 0;
			m_reader_configured = false;
			m_reader_config_attempted_direct = false;
			m_reader_config_attempted_shared = false;
			m_is_identiv = false;
			m_identiv_checked = false;
			m_vendor_id = 0;
			m_product_id = 0;
		}
	}
};

namespace
{
	static std::mutex s_claimed_readers_mutex;
	static std::unordered_map<std::string, const void*> s_claimed_readers;
}

pcsc_driver::pcsc_driver()
	: m_impl(std::make_unique<impl>())
{
}

pcsc_driver::~pcsc_driver()
{
	close();
}

pcsc_driver::pcsc_driver(pcsc_driver&&) noexcept = default;

pcsc_driver& pcsc_driver::operator=(pcsc_driver&& other) noexcept
{
	if (this != &other)
	{
		close();
		m_impl = std::move(other.m_impl);
	}
	return *this;
}

std::vector<std::string> pcsc_driver::list_readers()
{
	std::vector<std::string> readers;

	pcsc_context_t ctx = 0;
	pcsc_long_t ret = SCardEstablishContext(C_SCOPE_USER, nullptr, nullptr, &ctx);
	if (ret != C_SUCCESS)
	{
		ret = SCardEstablishContext(C_SCOPE_SYSTEM, nullptr, nullptr, &ctx);
	}
	if (ret != C_SUCCESS || ctx == 0)
	{
		return readers;
	}

	pcsc_dword_t cch_readers = 0;
	ret = SCardListReaders(ctx, nullptr, nullptr, &cch_readers);
	if (ret == C_SUCCESS && cch_readers > 0)
	{
		std::vector<char> buffer(cch_readers);
		ret = SCardListReaders(ctx, nullptr, buffer.data(), &cch_readers);
		if (ret == C_SUCCESS)
		{
			const char* ptr = buffer.data();
			const char* const end = buffer.data() + cch_readers;
			while (ptr < end && *ptr != '\0')
			{
				const usz len = strnlen(ptr, end - ptr);
				if (len > 0)
				{
					readers.emplace_back(ptr, len);
				}
				ptr += len + 1;
			}
		}
	}

	SCardReleaseContext(ctx);
	return readers;
}

bool pcsc_driver::open(std::string_view reader_name)
{
	if (!m_impl)
	{
		return false;
	}

	std::lock_guard lock(m_impl->m_mutex);

	if (reader_name.empty() || reader_name == fake_reader_name)
	{
		close();
		return false;
	}

	if (m_impl->m_reader_name == reader_name && is_open() && (m_impl->context_valid() || m_impl->m_reader_disconnected))
	{
		return true;
	}

	// Release any prior reader claim by this instance before atomically claiming the new reader
	close();

	{
		std::lock_guard claim_lock(s_claimed_readers_mutex);
		const auto [it, inserted] = s_claimed_readers.try_emplace(std::string(reader_name), m_impl.get());
		if (!inserted && it->second != m_impl.get())
		{
			nfc_log.error("PC/SC: Reader '%s' is already claimed; falling back to '%s'",
				reader_name, fake_reader_name);
			return false;
		}
	}

	m_impl->m_reader_name = reader_name;

	const auto enter_hotplug_wait = [&]()
	{
		disconnect_card(false);
		m_impl->release_context();
		m_impl->m_last_event_state = 0;
		m_impl->m_reader_disconnected = true;
		m_impl->m_last_reconnect_attempt = std::chrono::steady_clock::now();
		m_impl->m_is_open.store(true, std::memory_order_release);
		nfc_log.error("PC/SC: Reader '%s' is not connected; waiting for hotplug", reader_name);
		return true;
	};

	if (!m_impl->ensure_context())
	{
		return enter_hotplug_wait();
	}

	// Probe reader availability before attempting card/direct connection
	pcsc_reader_state_t probe_state{};
	probe_state.szReader = m_impl->m_reader_name.c_str();
	probe_state.dwCurrentState = C_STATE_UNAWARE;
	const pcsc_long_t probe_ret = SCardGetStatusChange(m_impl->m_context, 0, &probe_state, 1);
	if (probe_ret != C_SUCCESS || (probe_state.dwEventState & (C_STATE_UNKNOWN | C_STATE_UNAVAILABLE)) != 0)
	{
		if (probe_ret == C_SUCCESS || is_recoverable_comm_error(probe_ret))
		{
			return enter_hotplug_wait();
		}

		nfc_log.error("PC/SC: Failed to query reader '%s' (0x%08X)", reader_name, static_cast<u32>(probe_ret));
		close();
		return false;
	}

	m_impl->m_last_event_state = probe_state.dwEventState;
	const bool card_present = (probe_state.dwEventState & C_STATE_PRESENT) != 0;

	// Try connecting to card if present, or open direct connection for reader configuration
	if (!card_present || !connect_card())
	{
		if (m_impl->m_reader_disconnected)
		{
			m_impl->m_is_open.store(true, std::memory_order_release);
			return true;
		}

		// Try direct connection for reader-level escape commands
		pcsc_dword_t proto = C_PROTO_UNDEFINED;
		const pcsc_long_t ret = SCardConnect(
			m_impl->m_context,
			m_impl->m_reader_name.c_str(),
			C_SHARE_DIRECT,
			C_PROTO_UNDEFINED,
			&m_impl->m_card,
			&proto);

		if (ret == C_SUCCESS)
		{
			m_impl->m_active_protocol = proto;
		}
		else
		{
			m_impl->m_card = 0;
			if (is_recoverable_comm_error(ret))
			{
				return enter_hotplug_wait();
			}
		}
	}

	m_impl->m_reader_configured = false;
	m_impl->m_reader_config_attempted_direct = false;
	m_impl->m_reader_config_attempted_shared = false;
	m_impl->m_is_open.store(true, std::memory_order_release);
	configure_reader_if_needed();

	if (m_impl->m_reader_disconnected)
	{
		return true;
	}
	if (!m_impl->context_valid())
	{
		return enter_hotplug_wait();
	}

	nfc_log.success("PC/SC: Opened reader '%s'", reader_name);
	return true;
}

void pcsc_driver::configure_reader_if_needed()
{
	if (m_impl->m_reader_configured || m_impl->m_reader_name.empty())
	{
		return;
	}

	// Check if reader is Identiv uTrust / CLOUD 370x F / 470x F
	if (m_impl->is_identiv(this))
	{
		const bool is_shared = (m_impl->m_active_protocol == C_PROTO_T0 || m_impl->m_active_protocol == C_PROTO_T1);
		if (is_shared)
		{
			if (m_impl->m_reader_config_attempted_shared)
			{
				return;
			}
			m_impl->m_reader_config_attempted_shared = true;
		}
		else
		{
			if (m_impl->m_reader_config_attempted_direct)
			{
				return;
			}
			m_impl->m_reader_config_attempted_direct = true;
		}

		// Enable FeliCa polling in addition to default Type A + Type B (uTrust Manual Section 6.3.4.4)
		static constexpr std::array<u8, 3> cmd_all{0x95, 0xFF, 0xFF};
		static constexpr std::array<u8, 3> cmd_nfc_forum{0x95, 0xFF, 0xE3};
		static constexpr std::array<u8, 3> cmd_mainstream{0x95, 0xFF, 0x63};
		static constexpr std::array<u8, 2> cmd_simple{0x95, 0x02};
		const std::span<const u8> try_set_type_cmds[] = {cmd_all, cmd_nfc_forum, cmd_mainstream, cmd_simple};

		std::vector<u8> resp;
		for (const auto& cmd : try_set_type_cmds)
		{
			if (send_escape(cmd, resp))
			{
				m_impl->m_reader_configured = true;
				break;
			}
		}

		if (!m_impl->m_reader_configured && is_shared)
		{
			nfc_log.error("PC/SC: Failed to configure FeliCa polling for '%s' (Escape 0x95); ensure EscapeCommandEnable=1 if using Windows CCID driver",
				m_impl->m_reader_name);
		}
	}
	else if (m_impl->m_active_protocol == C_PROTO_T0 || m_impl->m_active_protocol == C_PROTO_T1)
	{
		m_impl->m_reader_configured = true;
	}
}

void pcsc_driver::mark_reader_disconnected(s64 ret)
{
	if (!m_impl)
	{
		return;
	}

	std::lock_guard lock(m_impl->m_mutex);
	m_impl->m_last_event_state = 0;
	disconnect_card(false);
	m_impl->release_context();

	if (!m_impl->m_reader_disconnected)
	{
		m_impl->m_reader_disconnected = true;
		nfc_log.error("PC/SC: Reader '%s' disconnected (0x%08X); waiting for reconnect",
			m_impl->m_reader_name, static_cast<u32>(ret));
	}

	m_impl->m_last_reconnect_attempt = std::chrono::steady_clock::now();
}

void pcsc_driver::close()
{
	if (!m_impl)
	{
		return;
	}

	m_impl->m_is_open.store(false, std::memory_order_release);
	std::lock_guard lock(m_impl->m_mutex);
	if (!m_impl->m_reader_name.empty())
	{
		std::lock_guard claim_lock(s_claimed_readers_mutex);
		const auto it = s_claimed_readers.find(m_impl->m_reader_name);
		if (it != s_claimed_readers.end() && it->second == m_impl.get())
		{
			s_claimed_readers.erase(it);
		}
	}
	disconnect_card(false);
	m_impl->m_reader_disconnected = false;
	m_impl->m_last_reconnect_attempt = {};
	m_impl->m_last_event_state = 0;
	m_impl->m_reader_configured = false;
	m_impl->m_reader_config_attempted_direct = false;
	m_impl->m_reader_config_attempted_shared = false;
	m_impl->m_ioctl_escape_blocked = false;
	m_impl->m_is_identiv = false;
	m_impl->m_identiv_checked = false;
	m_impl->m_vendor_id = 0;
	m_impl->m_product_id = 0;
	m_impl->clear_failed_auth_cache();
	m_impl->release_context();
	m_impl->m_reader_name.clear();
}

bool pcsc_driver::is_open() const
{
	return m_impl && m_impl->m_is_open.load(std::memory_order_acquire);
}

std::string pcsc_driver::get_reader_name() const
{
	if (!m_impl)
	{
		return {};
	}

	std::lock_guard lock(m_impl->m_mutex);
	return m_impl->m_reader_name;
}

bool pcsc_driver::is_card_present()
{
	if (!m_impl)
	{
		return false;
	}

	std::lock_guard lock(m_impl->m_mutex);
	if (!is_open() || m_impl->m_reader_name.empty())
	{
		return false;
	}

	if (m_impl->m_reader_disconnected)
	{
		const auto now = std::chrono::steady_clock::now();
		if (now - m_impl->m_last_reconnect_attempt < std::chrono::milliseconds(500))
		{
			return false;
		}
		m_impl->m_last_reconnect_attempt = now;

		if (!m_impl->ensure_context())
		{
			return false;
		}

		pcsc_reader_state_t probe_state{};
		probe_state.szReader = m_impl->m_reader_name.c_str();
		probe_state.dwCurrentState = C_STATE_UNAWARE;
		const pcsc_long_t probe_ret = SCardGetStatusChange(m_impl->m_context, 0, &probe_state, 1);
		if (probe_ret != C_SUCCESS || (probe_state.dwEventState & (C_STATE_UNKNOWN | C_STATE_UNAVAILABLE)) != 0)
		{
			disconnect_card(false);
			m_impl->release_context();
			return false;
		}

		m_impl->m_reader_disconnected = false;
		m_impl->m_reader_configured = false;
		m_impl->m_reader_config_attempted_direct = false;
		m_impl->m_reader_config_attempted_shared = false;
		nfc_log.success("PC/SC: Reconnected to reader '%s'", m_impl->m_reader_name);
	}

	if (!m_impl->ensure_context())
	{
		mark_reader_disconnected(static_cast<s64>(0x8010001D));
		return false;
	}

	pcsc_reader_state_t state{};
	state.szReader = m_impl->m_reader_name.c_str();
	state.dwCurrentState = C_STATE_UNAWARE;

	const pcsc_long_t ret = SCardGetStatusChange(m_impl->m_context, 0, &state, 1);

	if (ret == C_SUCCESS)
	{
		if ((state.dwEventState & (C_STATE_UNKNOWN | C_STATE_UNAVAILABLE)) != 0)
		{
			mark_reader_disconnected(static_cast<s64>(0x80100017));
			return false;
		}

		m_impl->m_last_event_state = state.dwEventState;
		const usz atr_len = std::min<usz>(state.cbAtr, sizeof(state.rgbAtr));
		if (atr_len > 0)
		{
			m_impl->m_last_event_atr.assign(state.rgbAtr, state.rgbAtr + atr_len);
		}
		else
		{
			m_impl->m_last_event_atr.clear();
		}

		const bool present = (state.dwEventState & C_STATE_PRESENT) != 0;
		if (!present)
		{
			if (m_impl->m_active_protocol != C_PROTO_UNDEFINED)
			{
				disconnect_card(false);
			}
			else
			{
				invalidate_card_cache();
			}

			if (!m_impl->m_reader_configured && !m_impl->m_reader_config_attempted_direct && m_impl->m_card == 0)
			{
				pcsc_dword_t proto = C_PROTO_UNDEFINED;
				const pcsc_long_t dir_ret = SCardConnect(
					m_impl->m_context,
					m_impl->m_reader_name.c_str(),
					C_SHARE_DIRECT,
					C_PROTO_UNDEFINED,
					&m_impl->m_card,
					&proto);
				if (dir_ret == C_SUCCESS)
				{
					m_impl->m_active_protocol = proto;
					configure_reader_if_needed();
					m_impl->m_reader_config_attempted_direct = true;
					// Do not retain an idle DIRECT handle if escape IOCTLs failed; T1 SHARED will configure on first card
					if (!m_impl->m_reader_configured && m_impl->m_card != 0 && m_impl->m_active_protocol == C_PROTO_UNDEFINED)
					{
						SCardDisconnect(m_impl->m_card, C_LEAVE_CARD);
						m_impl->m_card = 0;
					}
				}
				else if (is_recoverable_comm_error(dir_ret))
				{
					mark_reader_disconnected(dir_ret);
					return false;
				}
				else
				{
					m_impl->m_reader_config_attempted_direct = true;
				}
			}
		}
		else if (m_impl->m_card_info_cached)
		{
			// Invalidate cached card info immediately if the OS event counter or ATR indicates a quick card swap
			const bool counter_changed = (m_impl->m_cached_event_state & 0xFFFF0000u) != 0 &&
				(state.dwEventState & 0xFFFF0000u) != (m_impl->m_cached_event_state & 0xFFFF0000u);
			const bool atr_changed = !m_impl->m_last_event_atr.empty() &&
				!m_impl->m_cached_card_info.atr.empty() &&
				m_impl->m_last_event_atr != m_impl->m_cached_card_info.atr;
			if (counter_changed || atr_changed)
			{
				disconnect_card(false);
			}
		}
		return present;
	}

	// Reader may have been unplugged or USB communication disrupted
	m_impl->m_last_event_state = 0;
	disconnect_card(false);

	if (is_recoverable_comm_error(ret))
	{
		mark_reader_disconnected(ret);
	}

	return false;
}

bool pcsc_driver::connect_card()
{
	if (!m_impl)
	{
		return false;
	}

	std::lock_guard lock(m_impl->m_mutex);

	if (m_impl->m_reader_name.empty() || m_impl->m_reader_disconnected)
	{
		return false;
	}

	if (!m_impl->ensure_context())
	{
		return false;
	}

	// If already connected with active protocol T0 or T1, verify it's still alive
	if (m_impl->m_card != 0 && (m_impl->m_active_protocol == C_PROTO_T0 || m_impl->m_active_protocol == C_PROTO_T1))
	{
		char rname[128]{};
		pcsc_dword_t rlen = sizeof(rname);
		pcsc_dword_t state = 0;
		pcsc_dword_t proto = 0;
		u8 atr[36]{};
		pcsc_dword_t atrlen = sizeof(atr);
		const pcsc_long_t status_ret = SCardStatus(m_impl->m_card, rname, &rlen, &state, &proto, atr, &atrlen);
		if (status_ret == C_SUCCESS && !is_status_absent(state))
		{
			const bool atr_changed = atrlen > 0 && !m_impl->m_cached_card_info.atr.empty() &&
				(atrlen != m_impl->m_cached_card_info.atr.size() ||
				 std::memcmp(atr, m_impl->m_cached_card_info.atr.data(), atrlen) != 0);
			if (!atr_changed)
			{
				if (proto == C_PROTO_T0 || proto == C_PROTO_T1)
				{
					m_impl->m_active_protocol = proto;
				}
				return true;
			}
		}
		disconnect_card(false);
	}

	// If connected in direct mode, disconnect before connecting in shared mode
	if (m_impl->m_card != 0)
	{
		SCardDisconnect(m_impl->m_card, C_LEAVE_CARD);
		m_impl->m_card = 0;
		m_impl->m_active_protocol = C_PROTO_UNDEFINED;
		m_impl->m_in_part3_mode = false;
	}

	pcsc_dword_t active = C_PROTO_UNDEFINED;
	const pcsc_long_t ret = SCardConnect(
		m_impl->m_context,
		m_impl->m_reader_name.c_str(),
		C_SHARE_SHARED,
		C_PROTO_T0 | C_PROTO_T1,
		&m_impl->m_card,
		&active);

	if (ret == C_SUCCESS)
	{
		m_impl->m_active_protocol = active;
		m_impl->m_in_part3_mode = false;
		return true;
	}

	m_impl->m_card = 0;
	m_impl->m_active_protocol = C_PROTO_UNDEFINED;
	m_impl->m_in_part3_mode = false;
	if (is_recoverable_comm_error(ret))
	{
		mark_reader_disconnected(ret);
	}
	return false;
}

void pcsc_driver::disconnect_card(bool reset)
{
	if (!m_impl)
	{
		return;
	}

	std::lock_guard lock(m_impl->m_mutex);
	invalidate_card_cache();
	m_impl->m_in_part3_mode = false;
	if (m_impl->m_card != 0)
	{
		SCardDisconnect(m_impl->m_card, reset ? C_RESET_CARD : C_LEAVE_CARD);
		m_impl->m_card = 0;
		m_impl->m_active_protocol = C_PROTO_UNDEFINED;
	}
}

void pcsc_driver::release_target(bool reset_auth)
{
	if (!m_impl)
	{
		return;
	}

	std::lock_guard lock(m_impl->m_mutex);
	if (m_impl->m_card == 0)
	{
		m_impl->m_in_part3_mode = false;
		invalidate_card_cache();
		return;
	}

	if (reset_auth)
	{
		disconnect_card(true);
		return;
	}

	if (m_impl->m_in_part3_mode)
	{
		if (!part4_switch(true))
		{
			disconnect_card(true);
		}
	}
}

void pcsc_driver::invalidate_card_cache()
{
	if (!m_impl)
	{
		return;
	}

	std::lock_guard lock(m_impl->m_mutex);
	m_impl->m_card_info_cached = false;
	m_impl->m_cached_card_info = {};
	m_impl->m_cached_event_state = 0;
	m_impl->m_last_event_atr.clear();
}

pcsc_card_info pcsc_driver::get_card_info(bool verify_presence)
{
	if (!is_open())
	{
		return {};
	}

	std::lock_guard lock(m_impl->m_mutex);

	const bool has_live_shared_handle = m_impl->m_card != 0 &&
		(m_impl->m_active_protocol == C_PROTO_T0 || m_impl->m_active_protocol == C_PROTO_T1);

	// Fast path for active data exchanges immediately following InListPassiveTarget / InAutoPoll:
	// avoids redundant SCardGetStatusChange/SCardStatus IPC stalls while a new card settles in OS PC/SC.
	if (!verify_presence && !m_impl->m_reader_disconnected && has_live_shared_handle &&
		m_impl->m_card_info_cached && m_impl->m_cached_card_info.present)
	{
		return m_impl->m_cached_card_info;
	}

	pcsc_card_info info{};
	if (!is_card_present())
	{
		disconnect_card(false);
		return info;
	}

	if (m_impl->m_card_info_cached && m_impl->m_cached_card_info.present)
	{
		const bool event_counter_unchanged = has_live_shared_handle &&
			(m_impl->m_last_event_state & 0xFFFF0000u) != 0 &&
			(m_impl->m_last_event_state & 0xFFFF0000u) == (m_impl->m_cached_event_state & 0xFFFF0000u);
		if (event_counter_unchanged)
		{
			return m_impl->m_cached_card_info;
		}

		// Fallback check on backends without upper-word event counters: verify handle via SCardStatus
		if (connect_card() && m_impl->m_card_info_cached && m_impl->m_cached_card_info.present)
		{
			return m_impl->m_cached_card_info;
		}
	}

	if (!connect_card())
	{
		return info;
	}

	info.present = true;
	m_impl->m_in_part3_mode = false;

	// If reader configuration previously failed in direct mode (e.g. SCardControl blocked by OS driver),
	// retry now in shared mode where PAPDU_ESCAPE_CMD (FF CC 00 00 ...) can succeed via SCardTransmit
	if (!m_impl->m_reader_configured)
	{
		configure_reader_if_needed();
	}

	// Query Card ATR
	char reader_buf[128]{};
	pcsc_dword_t rlen = sizeof(reader_buf);
	pcsc_dword_t state = 0;
	pcsc_dword_t proto = 0;
	u8 atr_buf[36]{};
	pcsc_dword_t atr_len = sizeof(atr_buf);

	if (SCardStatus(m_impl->m_card, reader_buf, &rlen, &state, &proto, atr_buf, &atr_len) == C_SUCCESS && atr_len > 0)
	{
		info.atr.assign(atr_buf, atr_buf + atr_len);
	}

	bool is_type_a = true;
	const bool is_utrust = m_impl->is_identiv(this);

	// On Identiv readers, query Escape 0xDA FIRST because it returns UID + Tech + Arch + SAK in a single roundtrip.
	// Avoids an extra FF CA 00 00 00 APDU roundtrip unless 0xDA fails or omits the UID.
	if (is_utrust)
	{
		// Try uTrust Escape 0xDA (CNTLESS_GET_CARD_DETAILS)
		// uTrust_370xF_User_Manual.pdf Section 6.3.4.12 (Page 66-67)
		static constexpr std::array<u8, 1> details_in = {0xDA};
		std::vector<u8> details_out;
		bool got_details = send_escape(details_in, details_out);
		if (!got_details && !m_impl->m_reader_disconnected && !m_impl->context_valid())
		{
			// Context reset on recoverable comm error (e.g. 0x80100017); re-configure polling and retry 0xDA once
			configure_reader_if_needed();
			got_details = send_escape(details_in, details_out);
		}

		if (got_details && details_out.size() >= 3)
		{
			const u8 card_tech = details_out[0];
			const u8 card_arch = details_out[1];
			const u8 uid_len = details_out[2];

			// Byte 0: 0x00=Type A, 0x01=Type B, 0x04=FeliCa 212, 0x08=FeliCa 424
			// Byte 1: 0x00=Memory card, 0x01=T-CL card, 0x02=Dual interface, 0x43=FeliCa
			if (card_tech == 0x04 || card_tech == 0x08 || card_arch == 0x43)
			{
				info.is_felica = true;
				info.felica_speed = (card_tech == 0x08) ? 2 : 1;
				// uTrust_370xF_User_Manual.pdf Section 6.3.4.12 (Page 67): B2 = 0x08 for FeliCa NFCID2
				if (uid_len == 8 && details_out.size() >= 11)
				{
					info.uid.assign(details_out.begin() + 3, details_out.begin() + 11);
				}
			}
			else if (card_tech == 0x00) // Type A
			{
				is_type_a = true;
				if (card_arch == 0x00)
				{
					info.is_mifare = true;
				}
				else if (card_arch == 0x01 || card_arch == 0x02)
				{
					info.is_iso14443_4 = true;
				}

				if (uid_len >= 4 && uid_len <= 10 && details_out.size() >= 3u + uid_len)
				{
					info.uid.assign(details_out.begin() + 3, details_out.begin() + 3 + uid_len);
				}

				if (details_out.size() >= 20)
				{
					info.sak = details_out[19];
					info.sak_valid = true;
				}
			}
			else if (card_tech == 0x01) // Type B
			{
				is_type_a = false;
				info.is_type_b = true;
				if (card_arch == 0x01 || card_arch == 0x02)
				{
					info.is_iso14443_4 = true;
				}

				if (uid_len >= 4 && uid_len <= 10 && details_out.size() >= 3u + uid_len)
				{
					info.uid.assign(details_out.begin() + 3, details_out.begin() + 3 + uid_len);
				}
			}
		}
		else if (!m_impl->m_reader_disconnected)
		{
			// Fallback for older uTrust / SCM firmware: CNTLESS_GET_CARD_INFO (Escape 0x11)
			// uTrust_370xF_User_Manual.pdf Section 6.3.4.1 (Page 58)
			static constexpr std::array<u8, 1> info_in = {0x11};
			std::vector<u8> info_out;
			if (send_escape(info_in, info_out) && info_out.size() >= 3)
			{
				const u8 card_type_info = info_out[2];
				const u8 upper = (card_type_info >> 4) & 0x0F;
				const u8 lower = card_type_info & 0x0F;
				is_type_a = (lower == 0x00);
				if (lower == 0x01)
				{
					info.is_type_b = true;
				}
				if (upper == 0x00)
				{
					info.is_mifare = true;
				}
				else if (upper == 0x01 || upper == 0x02)
				{
					info.is_iso14443_4 = true;
				}
			}

			if (m_impl->m_reader_disconnected || m_impl->m_card == 0)
			{
				return {};
			}
		}
		else
		{
			return {};
		}
	}

	// Query UID using PAPDU_GET_UID (FF CA 00 00 00) if not already obtained from Identiv Escape 0xDA
	if (info.uid.empty())
	{
		std::array<u8, 5> get_uid_apdu = {0xFF, 0xCA, 0x00, 0x00, 0x00};
		std::vector<u8> uid_resp;
		const u16 last_sw = transmit_case2_apdu(*this, get_uid_apdu, uid_resp);
		if (last_sw == 0x9000)
		{
			uid_resp.resize(uid_resp.size() - 2);
			info.uid = std::move(uid_resp);
		}
		else if (last_sw == 0x6700)
		{
			// Fallback for readers rejecting Le=0x00 with SW 67 00 (Wrong length)
			for (u8 try_le : {0x04, 0x07, 0x08, 0x0A})
			{
				get_uid_apdu[4] = try_le;
				if (transmit_apdu(get_uid_apdu, uid_resp) && extract_sw(uid_resp) == 0x9000)
				{
					uid_resp.resize(uid_resp.size() - 2);
					info.uid = std::move(uid_resp);
					break;
				}
			}
		}
	}

	// Parse ATR if technology is not yet definitively determined
	bool is_pcsc_storage = false;
	if (!info.atr.empty() && !info.is_felica)
	{
		// Contactless Storage Token ATR (PC/SC Part 3 Section 3.1.3.2.3 / uTrust Manual Section 5.3.3.1)
		// Standard ATR format: 3B 8x 80 01 [Historical Bytes...] [TCK]
		// Storage tokens contain COMPACT-TLV with tag 0x4F (AID) and PC/SC RID A0 00 00 03 06
		if (info.atr.size() >= 4 && info.atr[0] == 0x3B && (info.atr[1] & 0xF0) == 0x80 && info.atr[2] == 0x80 && info.atr[3] == 0x01)
		{
			constexpr u8 pcsc_rid[] = {0xA0, 0x00, 0x00, 0x03, 0x06};
			for (usz i = 4; i + sizeof(pcsc_rid) + 3 <= info.atr.size(); ++i)
			{
				if (std::memcmp(&info.atr[i], pcsc_rid, sizeof(pcsc_rid)) != 0)
				{
					continue;
				}

				is_pcsc_storage = true;
				const u8 standard = info.atr[i + sizeof(pcsc_rid)];
				if (standard == 0x11) // FeliCa standard (PC/SC Part 3 Section 3.1.3.2.3)
				{
					info.is_felica = true;
				}
				else if (standard >= 0x04 && standard <= 0x09) // ISO 14443 Type B standard (PC/SC Part 3 Supplemental Table 2)
				{
					is_type_a = false;
					info.is_type_b = true;
				}
				else if (standard >= 0x01 && standard <= 0x03) // ISO 14443 Type A standard (Parts 1, 2, 3)
				{
					is_type_a = true;
				}

				const usz nn_offset = i + sizeof(pcsc_rid) + 1;
				if (nn_offset + 1 >= info.atr.size())
				{
					break;
				}

				const u16 card_name = static_cast<u16>((info.atr[nn_offset] << 8) | info.atr[nn_offset + 1]);

				switch (card_name)
				{
				case 0x0001: // MIFARE Classic 1K
					info.is_mifare = true;
					info.sak = 0x08;
					info.atqa = 0x0004;
					info.sak_valid = true;
					break;
				case 0x0002: // MIFARE Classic 4K
					info.is_mifare = true;
					info.sak = 0x18;
					info.atqa = 0x0002;
					info.sak_valid = true;
					break;
				case 0x0003: // MIFARE Ultralight
				case 0x003A: // MIFARE Ultralight C (PC/SC Part 3 Section 3.1.3.2.3)
					info.is_mifare = true;
					info.sak = 0x00;
					info.atqa = 0x0044;
					info.sak_valid = true;
					break;
				case 0x0026: // MIFARE Mini
					info.is_mifare = true;
					info.sak = 0x09;
					info.atqa = 0x0004;
					info.sak_valid = true;
					break;
				case 0x003B: // FeliCa
					info.is_felica = true;
					break;
				default:
					if (!info.is_felica && is_type_a)
					{
						info.is_mifare = true;
					}
					break;
				}
				break; // Only process the first RID match
			}
		}
	}

	if (is_pcsc_storage)
	{
		// Storage tokens (MIFARE Classic, Ultralight, FeliCa) are not ISO 14443-4 T=CL cards
		info.is_iso14443_4 = false;
	}
	else if (!info.is_mifare && !info.is_felica)
	{
		if (info.sak_valid && !info.is_type_b)
		{
			if ((info.sak & 0x20) != 0)
			{
				info.is_iso14443_4 = true;
			}
			if (info.sak == 0x00 && info.uid.size() == 7)
			{
				// MIFARE Ultralight / NTAG (SAK 0x00, 7-byte UID)
				info.is_mifare = true;
				info.atqa = 0x0044;
			}
			else if (info.sak == 0x09)
			{
				// MIFARE Mini (SAK 0x09)
				info.is_mifare = true;
				info.atqa = 0x0004;
			}
			else if (info.sak == 0x18 || info.sak == 0x38)
			{
				// MIFARE Classic 4K / dual-interface 4K + T=CL (SAK 0x18 / 0x38)
				info.is_mifare = true;
				info.atqa = 0x0002;
			}
			else if ((info.sak & 0x08) != 0)
			{
				// MIFARE Classic 1K / compatible / dual-interface Classic + T=CL (SAK 0x08, 0x28, 0x88, 0x19)
				info.is_mifare = true;
				info.atqa = 0x0004;
			}
		}
		else if (!info.atr.empty())
		{
			// Non-storage card ATR (no PC/SC storage RID A0 00 00 03 06).
			// Per PC/SC Part 3 Section 3.1.3.2 and uTrust Manual Section 5.3.3.2,
			// an activated non-storage card with T=1 / ATS mapping is an ISO 14443-4 T=CL card.
			info.is_iso14443_4 = true;
			if (is_type_a)
			{
				info.sak = 0x20;
				info.atqa = 0x0344;
				info.sak_valid = true;
			}
			else
			{
				info.sak_valid = false;
			}
		}
	}

	// Ensure ATQA and SAK are properly populated for MIFARE cards
	if (info.is_mifare)
	{
		if (!info.sak_valid)
		{
			if (info.uid.size() == 7)
			{
				info.sak = 0x00;
				info.atqa = 0x0044;
			}
			else
			{
				info.sak = 0x08;
				info.atqa = 0x0004;
			}
			info.sak_valid = true;
		}
		else if (info.atqa == 0x0000)
		{
			// Deduce ATQA from validated SAK per NXP AN10833 Table 4 / ISO 14443-3 §6.3.2
			if (info.sak == 0x18)
			{
				info.atqa = 0x0002; // MIFARE Classic 4K
			}
			else if (info.sak == 0x00)
			{
				info.atqa = 0x0044; // MIFARE Ultralight / NTAG
			}
			else
			{
				info.atqa = 0x0004; // MIFARE Classic 1K / Mini
			}
		}
	}

	// If 8-byte UID is present and not explicitly MIFARE or Type B, treat as FeliCa NFCID2
	if (info.uid.size() == 8 && !info.is_mifare && !info.is_type_b)
	{
		info.is_felica = true;
	}

	// Retrieve Historical Bytes / ATS for ISO 14443-4 T=CL cards
	if (info.is_iso14443_4)
	{
		// Method 1: uTrust Escape 0x93 (CNTLESS_GET_ATS_ATQB) - uTrust Manual Section 6.3.4.2
		// Returns full ATS for Type A T=CL cards; preserve raw ATS and extract historical bytes
		if (is_utrust && is_type_a)
		{
			static constexpr std::array<u8, 1> ats_in = {0x93};
			std::vector<u8> ats_out;
			if (send_escape(ats_in, ats_out) && ats_out.size() >= 2 && ats_out[0] >= 2 && ats_out[0] <= ats_out.size())
			{
				const usz max_ats_len = static_cast<usz>(ats_out[0]);
				info.ats.assign(ats_out.begin(), ats_out.begin() + max_ats_len);

				const u8 t0 = ats_out[1];
				usz hb_offset = 2;
				if (t0 & 0x10)
				{
					hb_offset++; // TA(1) present
				}
				if (t0 & 0x20)
				{
					hb_offset++; // TB(1) present
				}
				if (t0 & 0x40)
				{
					hb_offset++; // TC(1) present
				}
				if (hb_offset < max_ats_len)
				{
					info.historical_bytes.assign(ats_out.begin() + hb_offset, ats_out.begin() + max_ats_len);
				}
			}
		}

		// Method 2: Standard PC/SC Get Historical Bytes (FF CA 01 00 00)
		if (info.historical_bytes.empty())
		{
			std::array<u8, 5> get_hb = {0xFF, 0xCA, 0x01, 0x00, 0x00};
			std::vector<u8> hb_resp;
			if (transmit_case2_apdu(*this, get_hb, hb_resp) == 0x9000)
			{
				hb_resp.resize(hb_resp.size() - 2);
				if (std::any_of(hb_resp.begin(), hb_resp.end(), [](u8 b) { return b != 0; }))
				{
					info.historical_bytes = std::move(hb_resp);
				}
			}
		}

		// Method 3: Parse directly from ATR (uTrust Manual Section 5.3.3.2 / PC/SC Part 3 Section 3.1.3.2.3)
		if (info.historical_bytes.empty() && info.atr.size() >= 4 &&
			info.atr[0] == 0x3B && (info.atr[1] & 0xF0) == 0x80 && info.atr[2] == 0x80 && info.atr[3] == 0x01)
		{
			const u8 num_hb = info.atr[1] & 0x0F;
			if (num_hb > 0 && info.atr.size() >= 4u + num_hb)
			{
				info.historical_bytes.assign(info.atr.begin() + 4, info.atr.begin() + 4 + num_hb);
			}
		}
	}

	if (info.is_felica)
	{
		// Method 1: Standard PC/SC GET DATA for FeliCa PMm (FF CA 01 00 00)
		// Skip on Identiv uTrust where FF CA 01 00 00 always returns SW 6A 81 (unsupported)
		if (!is_utrust && info.pmm.empty())
		{
			std::array<u8, 5> get_pmm_apdu = {0xFF, 0xCA, 0x01, 0x00, 0x00};
			std::vector<u8> pmm_resp;
			if (transmit_case2_apdu(*this, get_pmm_apdu, pmm_resp) == 0x9000 && pmm_resp.size() == 10)
			{
				pmm_resp.resize(8);
				info.pmm = std::move(pmm_resp);
			}
		}

		// Method 2: Pseudo-APDU PAPDU_FELICA_REQC (FF 40 00 00 04 FF FF 01 00)
		static constexpr std::array<u8, 9> reqc_apdu = {0xFF, 0x40, 0x00, 0x00, 0x04, 0xFF, 0xFF, 0x01, 0x00};
		std::vector<u8> reqc_resp;
		if (transmit_apdu(reqc_apdu, reqc_resp) && reqc_resp.size() >= 18 && extract_sw(reqc_resp) == 0x9000)
		{
			info.uid.assign(reqc_resp.begin(), reqc_resp.begin() + 8);
			info.pmm.assign(reqc_resp.begin() + 8, reqc_resp.begin() + 16);
			if (reqc_resp.size() >= 20)
			{
				info.system_code = static_cast<u16>((reqc_resp[16] << 8) | reqc_resp[17]);
			}
		}

		// Method 3: Fallback to raw FeliCa Polling frame via felica_pass_thru if PMm or System Code still missing
		if (info.pmm.empty() || info.system_code == 0xFFFF)
		{
			static constexpr std::array<u8, 6> poll_cmd = {0x06, 0x00, 0xFF, 0xFF, 0x01, 0x00};
			std::vector<u8> poll_resp;
			if (felica_pass_thru(poll_cmd, poll_resp) && poll_resp.size() >= 18 && poll_resp[1] == 0x01)
			{
				info.uid.assign(poll_resp.begin() + 2, poll_resp.begin() + 10);
				info.pmm.assign(poll_resp.begin() + 10, poll_resp.begin() + 18);
				if (poll_resp.size() >= 20)
				{
					info.system_code = static_cast<u16>((poll_resp[18] << 8) | poll_resp[19]);
				}
			}
		}
	}

	// Per PN532 / uTrust 370xF semantics (UM0701-02 §7.3.5 / §7.3.13, uTrust §6.1.2 / §6.3.4.12),
	// a target cannot be selected without retrieving a valid UID / PUPI / NFCID2t from the card.
	const usz min_uid_size = info.is_felica ? 8 : 4;
	if (m_impl->m_reader_disconnected || m_impl->m_card == 0 || info.uid.size() < min_uid_size)
	{
		disconnect_card(false);
		info = {};
		return info;
	}

	m_impl->m_cached_card_info = info;
	m_impl->m_cached_event_state = m_impl->m_last_event_state;
	m_impl->m_in_part3_mode = false;
	m_impl->m_card_info_cached = true;
	return info;
}

bool pcsc_driver::transmit_apdu(std::span<const u8> apdu, std::vector<u8>& response)
{
	response.clear();
	if (!m_impl)
	{
		return false;
	}

	std::lock_guard lock(m_impl->m_mutex);

	if (apdu.empty())
	{
		return false;
	}

	if (m_impl->m_card == 0 || (m_impl->m_active_protocol != C_PROTO_T0 && m_impl->m_active_protocol != C_PROTO_T1))
	{
		if (!connect_card())
		{
			return false;
		}
	}

	const auto* pci = get_pci(m_impl->m_active_protocol);
	if (!pci)
	{
		return false;
	}

	u8 recv_buf[2048]{};
	pcsc_dword_t recv_len = sizeof(recv_buf);

	const pcsc_long_t ret = SCardTransmit(
		m_impl->m_card,
		pci,
		apdu.data(),
		static_cast<pcsc_dword_t>(apdu.size()),
		nullptr,
		recv_buf,
		&recv_len);

	if (ret == C_SUCCESS && recv_len > 0)
	{
		response.assign(recv_buf, recv_buf + recv_len);
		return true;
	}

	// Reset card handle upon card removal, abort, or unresponsive card, or reset both handle and context on recoverable reader/comm errors
	if (ret == static_cast<pcsc_long_t>(0x80100069) /* SCARD_W_REMOVED_CARD */ ||
		ret == static_cast<pcsc_long_t>(0x80100068) /* SCARD_W_RESET_CARD */ ||
		ret == static_cast<pcsc_long_t>(0x80100066) /* SCARD_W_UNRESPONSIVE_CARD */ ||
		ret == static_cast<pcsc_long_t>(0x80100067) /* SCARD_W_UNPOWERED_CARD */ ||
		ret == static_cast<pcsc_long_t>(0x80100002) /* SCARD_E_CANCELLED */ ||
		ret == static_cast<pcsc_long_t>(0x000003E3) /* ERROR_OPERATION_ABORTED */)
	{
		disconnect_card(false);
	}
	else if (is_recoverable_comm_error(ret))
	{
		disconnect_card(false);
		m_impl->release_context();
	}

	return false;
}

bool pcsc_driver::control(u32 control_code, std::span<const u8> in_data, std::vector<u8>& out_data)
{
	out_data.clear();
	if (!m_impl || !is_open())
	{
		return false;
	}

	std::lock_guard lock(m_impl->m_mutex);

	// SCardControl works on card handle in shared mode or direct mode
	if (m_impl->m_card == 0)
	{
		if (!connect_card())
		{
			// Try direct connect
			pcsc_dword_t proto = C_PROTO_UNDEFINED;
			if (SCardConnect(m_impl->m_context, m_impl->m_reader_name.c_str(), C_SHARE_DIRECT, C_PROTO_UNDEFINED, &m_impl->m_card, &proto) == C_SUCCESS)
			{
				m_impl->m_active_protocol = proto;
			}
			else
			{
				m_impl->m_card = 0;
				m_impl->m_active_protocol = C_PROTO_UNDEFINED;
			}
		}
	}

	if (m_impl->m_card == 0)
	{
		return false;
	}

	u8 recv_buf[512]{};
	pcsc_dword_t bytes_ret = 0;

	const pcsc_long_t ret = SCardControl(
		m_impl->m_card,
		control_code,
		in_data.data(),
		static_cast<pcsc_dword_t>(in_data.size()),
		recv_buf,
		sizeof(recv_buf),
		&bytes_ret);

	if (ret == C_SUCCESS)
	{
		if (bytes_ret > 0)
		{
			out_data.assign(recv_buf, recv_buf + bytes_ret);
		}
		return true;
	}

	// Reset stale card handle upon fatal communication failure or card removal (do not discard handle on unsupported IOCTL)
	if (is_recoverable_comm_error(ret))
	{
		disconnect_card(false);
		m_impl->release_context();
	}
	else if (ret == static_cast<pcsc_long_t>(0x80100069) /* SCARD_W_REMOVED_CARD */ ||
		ret == static_cast<pcsc_long_t>(0x80100068) /* SCARD_W_RESET_CARD */)
	{
		disconnect_card(false);

		// If card was removed or reset, reconnect in DIRECT mode and retry escape command
		pcsc_dword_t proto = C_PROTO_UNDEFINED;
		if (SCardConnect(m_impl->m_context, m_impl->m_reader_name.c_str(), C_SHARE_DIRECT, C_PROTO_UNDEFINED, &m_impl->m_card, &proto) == C_SUCCESS)
		{
			m_impl->m_active_protocol = proto;
			bytes_ret = 0;
			const pcsc_long_t retry_ret = SCardControl(
				m_impl->m_card,
				control_code,
				in_data.data(),
				static_cast<pcsc_dword_t>(in_data.size()),
				recv_buf,
				sizeof(recv_buf),
				&bytes_ret);

			if (retry_ret == C_SUCCESS)
			{
				if (bytes_ret > 0)
				{
					out_data.assign(recv_buf, recv_buf + bytes_ret);
				}
				return true;
			}
		}
	}

	return false;
}

bool pcsc_driver::send_escape(std::span<const u8> escape_data, std::vector<u8>& response)
{
	response.clear();
	if (!m_impl || escape_data.empty())
	{
		return false;
	}

	std::lock_guard lock(m_impl->m_mutex);

	bool is_shared = (m_impl->m_active_protocol == C_PROTO_T0 || m_impl->m_active_protocol == C_PROTO_T1);
	if (m_impl->m_ioctl_escape_blocked && !is_shared && m_impl->m_card == 0)
	{
		// Avoid retrying blocked SCardControl IOCTLs after a handle/context reset when a shared card connection works
		is_shared = connect_card();
	}
	const bool skip_ioctl = m_impl->m_ioctl_escape_blocked && is_shared;

	if (!skip_ioctl)
	{
		// Method 1: SCardControl via CCID_ESCAPE_CTL (uTrust Manual Section 6.3.1)
		if (control(CCID_ESCAPE_CTL, escape_data, response))
		{
			return true;
		}

#if defined(_WIN32)
		// Identiv uTrust only defines IOCTL_CCID_ESCAPE (3500); skip 2048 to avoid 65 ms driver timeouts
		if (!m_impl->m_is_identiv)
		{
			constexpr pcsc_dword_t CCID_VENDOR_CTL = SCARD_CTL_CODE(2048);
			if (control(CCID_VENDOR_CTL, escape_data, response))
			{
				return true;
			}
		}
#else
		// On Linux / macOS pcsc-lite, some CCID drivers map escape to SCARD_CTL_CODE(1)
		constexpr pcsc_dword_t CCID_ESCAPE_CTL_ALT = (0x42000000 + 1);
		if (control(CCID_ESCAPE_CTL_ALT, escape_data, response))
		{
			return true;
		}
#endif
	}

	// Method 2: Fallback to vendor-specific PAPDU_ESCAPE_CMD (uTrust Manual Section 6.1.3)
	// Command APDU: FF CC 00 00 Lc <escape_data>
	// Used when CCID Escape IOCTL is blocked by the OS driver
	if (escape_data.size() <= 255)
	{
		std::array<u8, 261> apdu_buf{};
		std::span<const u8> apdu = escape_data;
		if (!(escape_data.size() >= 5 && escape_data[0] == 0xFF && escape_data[1] == 0x70))
		{
			apdu = build_short_apdu(apdu_buf, 0xFF, 0xCC, 0x00, 0x00, escape_data);
		}

		if (transmit_apdu(apdu, response) && response.size() >= 2)
		{
			const u8 sw1 = response[response.size() - 2];
			const u8 sw2 = response.back();
			if (sw1 == 0x90 && sw2 == 0x00)
			{
				if (!skip_ioctl && !m_impl->m_ioctl_escape_blocked)
				{
					m_impl->m_ioctl_escape_blocked = true;
				}
				response.resize(response.size() - 2);
				return true;
			}
		}
		response.clear();
	}

	return false;
}

bool pcsc_driver::felica_pass_thru(std::span<const u8> felica_cmd, std::vector<u8>& felica_resp)
{
	felica_resp.clear();
	if (!is_open() || felica_cmd.empty())
	{
		return false;
	}

	std::lock_guard lock(m_impl->m_mutex);

	const bool has_len = (felica_cmd[0] == felica_cmd.size() && felica_cmd.size() >= 2);
	const usz base = has_len ? 1 : 0;
	const u8 fcmd = felica_cmd[base];
	const bool is_utrust = m_impl->is_identiv(this);

	auto is_valid_felica = [&](std::vector<u8>& resp) -> bool
	{
		if (resp.size() >= 2 && resp[0] >= 2 && resp[0] <= resp.size() && resp[1] == static_cast<u8>(fcmd + 1))
		{
			// Byte 0 is length byte, Byte 1 is Response Code. Truncate driver padding if any.
			resp.resize(resp[0]);
		}
		else if (!resp.empty() && resp[0] == static_cast<u8>(fcmd + 1) && resp.size() + 1 <= 255)
		{
			// Reader firmware stripped length byte; prepend length byte
			resp.insert(resp.begin(), static_cast<u8>(resp.size() + 1));
		}

		// Validate standard FeliCa framing: Byte 0 == size, Byte 1 == Command Code + 1
		return resp.size() >= 2 && resp[0] == resp.size() && resp[1] == static_cast<u8>(fcmd + 1);
	};

	// Method 1: Identiv CNTLESS_FELICA_PASS_THRU (Escape Code 0xF3)
	// Firmware prepends the JIS X 6319-4 length byte itself; sending one to uTrust causes a ~5.4s RF timeout.
	std::array<u8, 264> cmd_buf{};
	const usz raw_len = felica_cmd.size() - base;
	if (raw_len <= 255)
	{
		cmd_buf[0] = 0xF3;
		std::memcpy(cmd_buf.data() + 1, felica_cmd.data() + base, raw_len);
		if (send_escape(std::span<const u8>(cmd_buf.data(), 1 + raw_len), felica_resp) && is_valid_felica(felica_resp))
		{
			return true;
		}
	}

	// Fallback 1: Only for non-Identiv OEM readers whose 0xF3 expects explicit length prefix
	if (!is_utrust && felica_cmd.size() <= (has_len ? 255u : 254u))
	{
		usz alt_len = 0;
		cmd_buf[alt_len++] = 0xF3;
		if (!has_len)
		{
			cmd_buf[alt_len++] = static_cast<u8>(felica_cmd.size() + 1);
		}
		std::memcpy(cmd_buf.data() + alt_len, felica_cmd.data(), felica_cmd.size());
		alt_len += felica_cmd.size();
		if (send_escape(std::span<const u8>(cmd_buf.data(), alt_len), felica_resp) && is_valid_felica(felica_resp))
		{
			return true;
		}
	}

	felica_resp.clear();
	std::vector<u8> rapdu;

	// Transmit a FeliCa pseudo-APDU in cmd_buf and retry with Le=0x00 / SW 6C xx if required by OS driver
	const auto transmit_felica_apdu = [&](usz apdu_len) -> bool
	{
		if (transmit_apdu(std::span<const u8>(cmd_buf.data(), apdu_len), rapdu) && rapdu.size() >= 2)
		{
			const u16 sw = extract_sw(rapdu);
			if (sw == 0x9000)
			{
				return true;
			}
			if ((sw == 0x6700 || (sw & 0xFF00) == 0x6C00) && apdu_len < cmd_buf.size())
			{
				const u8 corrected_le = (sw & 0xFF00) == 0x6C00 ? static_cast<u8>(sw & 0xFF) : 0x00;
				usz retry_len = apdu_len;
				if (retry_len == 5)
				{
					// Replace existing Le on Case 2 short APDUs instead of turning them into invalid 6-byte headers
					cmd_buf[4] = corrected_le;
				}
				else
				{
					cmd_buf[retry_len++] = corrected_le;
				}
				transmit_apdu(std::span<const u8>(cmd_buf.data(), retry_len), rapdu);
			}
			return true;
		}
		return false;
	};

	const auto wrap_felica_response = [&](u8 resp_code, std::span<const u8> body)
	{
		felica_resp.resize(2 + body.size());
		felica_resp[0] = static_cast<u8>(2 + body.size());
		felica_resp[1] = resp_code;
		if (!body.empty())
		{
			std::memcpy(felica_resp.data() + 2, body.data(), body.size());
		}
	};

	const auto finish_pseudo_apdu = [&](usz apdu_len, usz min_resp_len, u8 resp_code, usz fixed_copy_len = 0) -> bool
	{
		if (transmit_felica_apdu(apdu_len) && rapdu.size() >= min_resp_len && extract_sw(rapdu) == 0x9000)
		{
			const usz body_len = fixed_copy_len ? fixed_copy_len : (rapdu.size() - 2);
			wrap_felica_response(resp_code, std::span<const u8>(rapdu.data(), body_len));
			return true;
		}
		return false;
	};

	const auto calc_block_list_len = [&](usz blk_offset, u8 num_blocks) -> usz
	{
		usz blk_len = 0;
		usz scan = blk_offset;
		for (u8 b = 0; b < num_blocks && scan < felica_cmd.size(); ++b)
		{
			// JIS X 6319-4 Section 8.2.7: Bit 7 = 1 is 2-byte element, Bit 7 = 0 is 3-byte element
			const usz elem_sz = (felica_cmd[scan] & 0x80) ? 2 : 3;
			blk_len += elem_sz;
			scan += elem_sz;
		}
		return blk_len;
	};

	// Method 2: Fallback to Pseudo-APDU based on FeliCa command opcode
	if (felica_cmd.size() >= base + 1)
	{
		if (fcmd == 0x00 && felica_cmd.size() >= base + 3) // Polling
		{
			const u8 req_code = (felica_cmd.size() >= base + 4) ? felica_cmd[base + 3] : 0x00;
			const u8 tsn = (felica_cmd.size() >= base + 5) ? felica_cmd[base + 4] : 0x00;
			cmd_buf[0] = 0xFF;
			cmd_buf[1] = 0x40;
			cmd_buf[2] = 0x00;
			cmd_buf[3] = 0x00;
			cmd_buf[4] = 0x04;
			cmd_buf[5] = felica_cmd[base + 1];
			cmd_buf[6] = felica_cmd[base + 2];
			cmd_buf[7] = req_code;
			cmd_buf[8] = tsn;
			if (finish_pseudo_apdu(9, 18, 0x01))
			{
				return true;
			}
		}
		else if (fcmd == 0x02 && felica_cmd.size() >= base + 10) // Request Service
		{
			const u8 num_services = felica_cmd[base + 9];
			const usz srv_offset = base + 10;
			const usz srv_len = num_services * 2;
			if (felica_cmd.size() >= srv_offset + srv_len && srv_len <= 255)
			{
				cmd_buf[0] = 0xFF;
				cmd_buf[1] = 0x42;
				cmd_buf[2] = num_services;
				cmd_buf[3] = 0x00;
				cmd_buf[4] = static_cast<u8>(srv_len);
				std::memcpy(cmd_buf.data() + 5, felica_cmd.data() + srv_offset, srv_len);
				if (finish_pseudo_apdu(5 + srv_len, 11, 0x03))
				{
					return true;
				}
			}
		}
		else if ((fcmd == 0x04 || fcmd == 0x0C) && felica_cmd.size() >= base + 9) // Request Response / System Code
		{
			cmd_buf[0] = 0xFF;
			cmd_buf[1] = (fcmd == 0x04) ? 0x44 : 0x4A;
			cmd_buf[2] = 0x00;
			cmd_buf[3] = 0x00;
			cmd_buf[4] = 0x00;
			if (finish_pseudo_apdu(5, 11, static_cast<u8>(fcmd + 1)))
			{
				return true;
			}
		}
		else if ((fcmd == 0x06 || fcmd == 0x08) && felica_cmd.size() >= base + 11) // Read / Write Without Encryption
		{
			const u8 num_services = felica_cmd[base + 9];
			const usz srv_offset = base + 10;
			const usz srv_len = num_services * 2;
			if (felica_cmd.size() > srv_offset + srv_len)
			{
				const u8 num_blocks = felica_cmd[srv_offset + srv_len];
				const usz blk_offset = srv_offset + srv_len + 1;
				const usz blk_len = calc_block_list_len(blk_offset, num_blocks);
				const usz data_offset = blk_offset + blk_len;
				const usz data_len = (fcmd == 0x08) ? static_cast<usz>(num_blocks) * 16 : 0;
				const usz payload_len = srv_len + blk_len + data_len;
				if (felica_cmd.size() >= data_offset + data_len && payload_len <= 255 && blk_len == static_cast<usz>(num_blocks) * 2)
				{
					cmd_buf[0] = 0xFF;
					cmd_buf[1] = (fcmd == 0x06) ? 0x46 : 0x48;
					cmd_buf[2] = num_services;
					cmd_buf[3] = num_blocks;
					cmd_buf[4] = static_cast<u8>(payload_len);
					std::memcpy(cmd_buf.data() + 5, felica_cmd.data() + srv_offset, srv_len);
					std::memcpy(cmd_buf.data() + 5 + srv_len, felica_cmd.data() + blk_offset, blk_len);
					if (data_len > 0)
					{
						std::memcpy(cmd_buf.data() + 5 + srv_len + blk_len, felica_cmd.data() + data_offset, data_len);
					}
					if (finish_pseudo_apdu(5 + payload_len, 12, static_cast<u8>(fcmd + 1), (fcmd == 0x08) ? 10 : 0))
					{
						return true;
					}
				}
			}
		}
	}

	// Assemble RF frame with mandatory JIS X 6319-4 Section 6.2 length byte prefix if input lacks it
	std::array<u8, 256> rf_frame_buf{};
	std::span<const u8> rf_cmd = felica_cmd;
	if (!has_len)
	{
		if (felica_cmd.size() >= 255)
		{
			return false;
		}
		rf_frame_buf[0] = static_cast<u8>(felica_cmd.size() + 1);
		std::memcpy(rf_frame_buf.data() + 1, felica_cmd.data(), felica_cmd.size());
		rf_cmd = {rf_frame_buf.data(), 1 + felica_cmd.size()};
	}

	const auto finalize_felica_slice = [&](std::span<const u8> slice) -> bool
	{
		if (slice.size() >= 2 && slice[0] == slice.size() && slice[1] == static_cast<u8>(fcmd + 1))
		{
			felica_resp.assign(slice.begin(), slice.end());
			return true;
		}
		if (!slice.empty() && slice[0] == static_cast<u8>(fcmd + 1) && slice.size() + 1 <= 255)
		{
			felica_resp.resize(slice.size() + 1);
			felica_resp[0] = static_cast<u8>(slice.size() + 1);
			std::memcpy(felica_resp.data() + 1, slice.data(), slice.size());
			return true;
		}
		return false;
	};

	// Method 3: PC/SC Part 3 Supplemental Document Transparent Exchange (FF C2 00 01)
	{
		usz cand_off = 0;
		usz cand_len = 0;
		if (transmit_transparent_apdu(*this, rf_cmd, rapdu, cand_off, cand_len, true) &&
			finalize_felica_slice(std::span<const u8>(rapdu.data() + cand_off, cand_len)))
		{
			return true;
		}
	}

	// Method 4: Generic PC/SC Direct Transmit APDU (FF 00 00 00 Lc <rf_cmd>)
	if (rf_cmd.size() <= 255)
	{
		std::array<u8, 261> direct_buf{};
		const auto direct_apdu = build_short_apdu(direct_buf, 0xFF, 0x00, 0x00, 0x00, rf_cmd);
		if (transmit_apdu(direct_apdu, rapdu) && rapdu.size() >= 4 && extract_sw(rapdu) == 0x9000 &&
			finalize_felica_slice(std::span<const u8>(rapdu.data(), rapdu.size() - 2)))
		{
			return true;
		}
	}

	return false;
}

bool pcsc_driver::mifare_authenticate(u8 block, bool key_a, std::span<const u8, 6> key)
{
	if (!is_open())
	{
		return false;
	}

	std::lock_guard lock(m_impl->m_mutex);

	const u8 key_type = key_a ? 0x60 : 0x61;
	const u8 sector = (block < 128) ? static_cast<u8>(block / 4) : static_cast<u8>(32 + (block - 128) / 16);

	if (m_impl->is_auth_failed_cached(m_impl->m_cached_card_info.uid, sector, key_type, key))
	{
		return false;
	}

	const auto record_rejected_key = [&]()
	{
		m_impl->record_failed_auth(m_impl->m_cached_card_info.uid, sector, key_type, key);
	};

	const bool is_utrust = m_impl->is_identiv(this);

	// For dual-interface cards (e.g. SAK 0x28), ensure reader switches from T=CL mode to MIFARE mode (uTrust Section 6.2.12)
	if (is_utrust && m_impl->m_cached_card_info.is_iso14443_4 && m_impl->m_cached_card_info.is_mifare && !m_impl->m_in_part3_mode)
	{
		part4_switch(false);
	}

	// Step 1: Load Keys APDU (uTrust Section 6.2.3 uses P2=key_type; standard PC/SC uses slot 0x00)
	u8 load_slot = is_utrust ? key_type : 0x00;
	std::array<u8, 11> load_key_apdu = {0xFF, 0x82, 0x00, load_slot, 0x06, key[0], key[1], key[2], key[3], key[4], key[5]};

	std::vector<u8> rapdu;
	bool load_ok = transmit_apdu(load_key_apdu, rapdu) && extract_sw(rapdu) == 0x9000;
	if (!load_ok)
	{
		load_slot = (load_slot == 0x00) ? key_type : 0x00;
		load_key_apdu[3] = load_slot;
		load_ok = transmit_apdu(load_key_apdu, rapdu) && extract_sw(rapdu) == 0x9000;
	}
	if (!load_ok)
	{
		return false;
	}

	const auto is_auth_rejected_sw = [](u16 sw) -> bool
	{
		return sw == 0x6300 || sw == 0x6982 || sw == 0x6983 || sw == 0x6985 || (sw & 0xFFF0) == 0x63C0;
	};

	// Step 2: Authenticate APDU (uTrust Section 6.2.4 specifies KeyNumber B4=0x01; PC/SC Part 3 uses loaded slot)
	const u8 primary_auth_slot = is_utrust ? 0x01 : load_slot;
	std::array<u8, 10> auth_apdu = {0xFF, 0x86, 0x00, 0x00, 0x05, 0x01, 0x00, block, key_type, primary_auth_slot};
	std::array<u8, 5> tried_slots{};
	usz tried_count = 0;
	for (const u8 slot : {primary_auth_slot, load_slot, u8{0x01}, key_type, u8{0x00}})
	{
		if (std::find(tried_slots.begin(), tried_slots.begin() + tried_count, slot) != tried_slots.begin() + tried_count)
		{
			continue;
		}
		tried_slots[tried_count++] = slot;
		auth_apdu[9] = slot;
		if (!transmit_apdu(auth_apdu, rapdu) || rapdu.size() < 2)
		{
			return false;
		}
		const u16 sw = extract_sw(rapdu);
		if (sw == 0x9000)
		{
			return true;
		}
		if (is_auth_rejected_sw(sw))
		{
			// Card rejected the key; do not waste RF time retrying alternate key slots or legacy FF 88
			record_rejected_key();
			return false;
		}
	}

	// Obsolete authenticate format FF 88 for legacy readers
	for (const u8 slot : {load_slot, u8{0x00}})
	{
		const std::array<u8, 6> auth_apdu_b = {0xFF, 0x88, 0x00, block, key_type, slot};
		if (transmit_apdu(auth_apdu_b, rapdu) && extract_sw(rapdu) == 0x9000)
		{
			return true;
		}
		if (slot == 0x00)
		{
			break;
		}
	}

	return false;
}

bool pcsc_driver::mifare_read(u8 block, std::vector<u8>& block_data)
{
	block_data.clear();
	if (!is_open())
	{
		return false;
	}

	std::lock_guard lock(m_impl->m_mutex);

	std::array<u8, 5> read_apdu = {0xFF, 0xB0, 0x00, block, 0x10};
	std::vector<u8> rapdu;
	bool read_ok = false;

	for (u8 try_le : {u8{0x10}, u8{0x04}, u8{0x00}})
	{
		read_apdu[4] = try_le;
		const u16 last_sw = transmit_case2_apdu(*this, read_apdu, rapdu);
		if (last_sw == 0)
		{
			break;
		}
		if (last_sw == 0x9000)
		{
			read_ok = true;
			break;
		}
		if (last_sw != 0x6700 && (last_sw & 0xFF00) != 0x6C00 && last_sw != 0x6282)
		{
			// Auth/card error rather than wrong Le; don't waste RF retries on alternate lengths
			break;
		}
	}

	if (!read_ok)
	{
		return false;
	}

	rapdu.resize(rapdu.size() - 2);
	block_data = std::move(rapdu);
	rapdu.reserve(32); // Preserve scratch vector after move so Ultralight multi-page assembly avoids reallocations

	// Assemble 16-byte buffer (4 pages) for 4-byte MIFARE Ultralight / UL C blocks
	if (block_data.size() == 4)
	{
		const u8 successful_le = read_apdu[4];
		bool rolled_over = false;
		for (u8 next_block = 1; next_block < 4; ++next_block)
		{
			u16 target_page = rolled_over
				? ((static_cast<u16>(block) + next_block) % 16)
				: (static_cast<u16>(block) + next_block);
			if (target_page > 255)
			{
				break;
			}

			auto read_page = [&](u16 page) -> bool
			{
				const std::array<u8, 5> next_apdu = {0xFF, 0xB0, 0x00, static_cast<u8>(page), successful_le};
				if (transmit_apdu(next_apdu, rapdu) && rapdu.size() >= 6 && extract_sw(rapdu) == 0x9000)
				{
					block_data.insert(block_data.end(), rapdu.begin(), rapdu.begin() + 4);
					return true;
				}
				return false;
			};

			if (read_page(target_page))
			{
				continue;
			}

			// Standard 16-page MIFARE Ultralight (MF0ICU1) rolls over from page 15 back to page 0
			if (!rolled_over && block < 16 && target_page >= 16)
			{
				rolled_over = true;
				if (read_page(target_page % 16))
				{
					continue;
				}
			}
			break;
		}

		// Pad boundary reads shorter than 16 bytes with zeroes to satisfy 16B MIFARE frame
		if (block_data.size() < 16)
		{
			block_data.resize(16, 0x00);
		}
	}

	return true;
}

bool pcsc_driver::mifare_write(u8 block, std::span<const u8> block_data)
{
	if (!is_open() || block_data.empty() || block_data.size() > 255)
	{
		return false;
	}

	std::lock_guard lock(m_impl->m_mutex);

	std::array<u8, 261> write_buf{};
	const auto write_apdu = build_short_apdu(write_buf, 0xFF, 0xD6, 0x00, block, block_data);

	std::vector<u8> rapdu;
	if (transmit_apdu(write_apdu, rapdu) && extract_sw(rapdu) == 0x9000)
	{
		m_impl->invalidate_failed_auth(m_impl->m_cached_card_info.uid);
		return true;
	}

	// Ultralight fallback (4-byte page COMPATIBILITY WRITE per MF0ICU1 Section 8.5.3)
	if (block_data.size() == 16)
	{
		const std::array<u8, 9> page_apdu = {0xFF, 0xD6, 0x00, block, 0x04, block_data[0], block_data[1], block_data[2], block_data[3]};
		if (transmit_apdu(page_apdu, rapdu) && extract_sw(rapdu) == 0x9000)
		{
			m_impl->invalidate_failed_auth(m_impl->m_cached_card_info.uid);
			return true;
		}
	}

	return false;
}

bool pcsc_driver::part3_pass_thru(std::span<const u8> data, std::vector<u8>& response)
{
	response.clear();
	if (!is_open() || data.empty() || data.size() > 255)
	{
		return false;
	}

	std::lock_guard lock(m_impl->m_mutex);

	// PAPDU_ISO14443_PART3_PASS_THRU: FF EF P1(01=DiscardCRC) 00 Lc <Data>
	std::array<u8, 261> apdu_buf{};
	const auto apdu = build_short_apdu(apdu_buf, 0xFF, 0xEF, 0x01, 0x00, data);

	std::vector<u8> rapdu;
	if (transmit_apdu(apdu, rapdu) && rapdu.size() >= 2)
	{
		u16 sw = extract_sw(rapdu);
		if (sw == 0x6700 || (sw & 0xFF00) == 0x6C00)
		{
			apdu_buf[5 + data.size()] = (sw & 0xFF00) == 0x6C00 ? static_cast<u8>(sw & 0xFF) : 0x00;
			if (transmit_apdu(std::span<const u8>(apdu_buf.data(), 6 + data.size()), rapdu) && rapdu.size() >= 2)
			{
				sw = extract_sw(rapdu);
			}
		}
		if (sw == 0x9000)
		{
			rapdu.resize(rapdu.size() - 2);
			response = std::move(rapdu);
			return true;
		}
	}

	// Fallback with P1 = 0x00 (Transmit CRC) if P1 = 0x01 is rejected by firmware
	apdu_buf[2] = 0x00;
	if (transmit_apdu(apdu, rapdu) && extract_sw(rapdu) == 0x9000)
	{
		rapdu.resize(rapdu.size() - 2);
		if (rapdu.size() >= 2)
		{
			rapdu.resize(rapdu.size() - 2);
		}
		response = std::move(rapdu);
		return true;
	}

	// Fallback 2: PC/SC Part 3 Supp. 2 Transparent Exchange (FF C2 00 01)
	{
		usz resp_off = 0;
		usz resp_len = 0;
		if (transmit_transparent_apdu(*this, data, rapdu, resp_off, resp_len, false))
		{
			response.assign(rapdu.begin() + resp_off, rapdu.begin() + resp_off + resp_len);
			return true;
		}
	}

	// Fallback 3: Generic Direct Transmit APDU (FF 00 00 00 Lc <Data>)
	const auto direct_apdu = build_short_apdu(apdu_buf, 0xFF, 0x00, 0x00, 0x00, data);
	if (transmit_apdu(direct_apdu, rapdu) && extract_sw(rapdu) == 0x9000)
	{
		rapdu.resize(rapdu.size() - 2);
		response = std::move(rapdu);
		return true;
	}

	return false;
}

bool pcsc_driver::tcl_pass_thru(std::span<const u8> data, std::vector<u8>& response)
{
	response.clear();
	if (!is_open() || data.empty())
	{
		return false;
	}

	std::lock_guard lock(m_impl->m_mutex);

	const bool is_utrust = m_impl->is_identiv(this);
	std::vector<u8> rapdu;

	const auto try_wrapped_tcl = [&]() -> bool
	{
		if (data.size() > 255)
		{
			return false;
		}
		std::array<u8, 261> apdu_buf{};
		const auto apdu = build_short_apdu(apdu_buf, 0xFF, 0xFE, 0x00, 0x00, data);
		if (transmit_apdu(apdu, rapdu) && rapdu.size() >= 2)
		{
			const u16 sw = extract_sw(rapdu);
			// Filter reader rejection status words (uTrust Manual Section 7.1)
			if (sw != 0x6D00 && sw != 0x6E00 && sw != 0x6A81 && sw != 0x6F00 && sw != 0x6800 && sw != 0x6B00)
			{
				response = std::move(rapdu);
				return true;
			}
		}
		return false;
	};

	const auto try_direct_tcl = [&]() -> bool
	{
		if (data.size() >= 4 && transmit_apdu(data, rapdu) && rapdu.size() >= 2)
		{
			response = std::move(rapdu);
			return true;
		}
		return false;
	};

	if (is_utrust)
	{
		if (m_impl->m_in_part3_mode)
		{
			part4_switch(true);
		}
		return try_wrapped_tcl() || try_direct_tcl();
	}

	return try_direct_tcl() || try_wrapped_tcl();
}

bool pcsc_driver::part4_switch(bool to_tcl)
{
	if (!is_open())
	{
		return false;
	}

	std::lock_guard lock(m_impl->m_mutex);

	// PAPDU_ISO14443_PART4_PART3_SWITCH (uTrust Manual Section 6.2.12): FF F8 P1 00 00
	const std::array<u8, 5> apdu = {0xFF, 0xF8, static_cast<u8>(to_tcl ? 0x00 : 0x01), 0x00, 0x00};
	std::vector<u8> rapdu;
	if (transmit_apdu(apdu, rapdu) && extract_sw(rapdu) == 0x9000)
	{
		m_impl->m_in_part3_mode = !to_tcl;
		return true;
	}
	return false;
}
