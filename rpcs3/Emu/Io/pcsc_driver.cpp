#include "stdafx.h"
#include "pcsc_driver.h"
#include "util/logs.hpp"
#include "Utilities/StrUtil.h"

#include <algorithm>
#include <array>
#include <charconv>
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
	constexpr pcsc_dword_t C_STATE_PRESENT = 0x00000020;
	constexpr pcsc_dword_t C_STATUS_ABSENT = 1;
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

		// Product IDs from CLOUD.inf driver and uTrust 370xF User Manual
		constexpr u16 PID_CHICONY_KUS0967  = 0x0967; // USB Smart Card Keyboard
		constexpr u16 PID_SCL3711          = 0x5411; // SCL3711 Contactless USB Dongle
		constexpr u16 PID_STC5_DFU         = 0x5601; // STC5 DFU Adapter
		constexpr u16 PID_UTRUST_5500_SAM  = 0x5610; // uTrust 5500 SAM Reader (MI_00)
		constexpr u16 PID_UTRUST_5500_CL   = 0x5610; // uTrust 5500 Contactless Reader (MI_01)
		constexpr u16 PID_UTRUST_5501_SAM  = 0x5611; // uTrust 5501 SAM Reader (MI_00)
		constexpr u16 PID_UTRUST_5501_CL   = 0x5611; // uTrust 5501 Contactless Reader (MI_01)
		constexpr u16 PID_UTRUST_3720_CL   = 0x5612; // uTrust 3720 Contactless Reader
		constexpr u16 PID_UTRUST_3721_CL   = 0x5613; // uTrust 3721 Contactless Reader (MI_00)
		constexpr u16 PID_UTRUST_5501_CLS  = 0x5614; // uTrust 5501 Contactless Reader
		constexpr u16 PID_STC4_DFU         = 0x5700; // STC4 DFU Adapter
		constexpr u16 PID_STC4_DFU_SEC     = 0x5701; // STC4 DFU Adapter (Secondary)
		constexpr u16 PID_UTRUST_2700F     = 0x5710; // uTrust 2700 F Smart Card Reader
		constexpr u16 PID_UTRUST_2710F     = 0x5711; // uTrust 2710 F Smart Card Reader
		constexpr u16 PID_SCR331_2700F     = 0x5712; // SCR331 CLOUD 2700 F
		constexpr u16 PID_UTRUST_2980F     = 0x5713; // uTrust 2980 F Smart Card Reader
		constexpr u16 PID_IAUTH2           = 0x5714; // iAuthenticate 2.0
		constexpr u16 PID_UTRUST_2700F_FIDO= 0x5715; // uTrust 2700 F FIDO Authenticator (MI_01)
		constexpr u16 PID_UTRUST_4700F_CNT  = 0x5720; // uTrust 4700 F Contact Reader (MI_00)
		constexpr u16 PID_UTRUST_4700F_CL   = 0x5720; // uTrust 4700 F CL Reader (MI_01)
		constexpr u16 PID_UTRUST_4710F_SAM  = 0x5721; // uTrust 4710 F SAM Reader (MI_00)
		constexpr u16 PID_UTRUST_4710F_CL   = 0x5721; // uTrust 4710 F CL Reader (MI_01)
		constexpr u16 PID_CLOUD_4000F_CNT   = 0x5723; // CLOUD 4000 F DTC Contact Reader (MI_00)
		constexpr u16 PID_CLOUD_4000F_CL    = 0x5723; // CLOUD 4000 F DTC CL Reader (MI_01)
		constexpr u16 PID_UTRUST_4701F_CNT  = 0x5724; // uTrust 4701 F Contact Reader (MI_00)
		constexpr u16 PID_UTRUST_4701F_CL   = 0x5724; // uTrust 4701 F CL Reader (MI_01)
		constexpr u16 PID_UTRUST_4711F_SAM  = 0x5725; // uTrust 4711 F SAM Reader (MI_00)
		constexpr u16 PID_UTRUST_4711F_CL   = 0x5725; // uTrust 4711 F CL Reader (MI_01)
		constexpr u16 PID_UTRUST_4701_CT_CL = 0x5726; // uTrust 4701 F CT_CL Reader
		constexpr u16 PID_SMARTOS_CT        = 0x5727; // Identiv SmartOS Contact Reader (MI_00)
		constexpr u16 PID_SMARTOS_CL        = 0x5727; // Identiv SmartOS Contactless Reader (MI_01)
		constexpr u16 PID_SMARTOS_CTC       = 0x5728; // Identiv SmartOS Contact Reader (MI_00)
		constexpr u16 PID_SMARTOS_CLS       = 0x5728; // Identiv SmartOS Contactless Reader (MI_01)
		constexpr u16 PID_CLOUD_2190F       = 0x5750; // CLOUD 2190 F Smart Card Reader (MI_01)
		constexpr u16 PID_UTRUST_3700F_CL   = 0x5790; // uTrust 3700 F CL Reader (CLOUD3700F_BSI_CL)
		constexpr u16 PID_UTRUST_3701F_CL   = 0x5791; // uTrust 3701 F CL Reader (CLOUD3701F_LASCOM_CL)
		constexpr u16 PID_SMARTOS_CLM       = 0x5792; // Identiv SmartOS Contactless Reader
		constexpr u16 PID_UTRUST_2700R      = 0x5810; // uTrust 2700 R Smart Card Reader
		constexpr u16 PID_CLOUD_2900R       = 0x5811; // CLOUD 2900 R Smart Card Reader
		constexpr u16 PID_CLOUD_2910R       = 0x5812; // CLOUD 2910 R Smart Card Reader
		constexpr u16 PID_SCR3310_2700R     = 0x5813; // SCR3310 CLOUD 2700 R
		constexpr u16 PID_SCR3500A          = 0x5814; // SCR3500 A Contact Reader
		constexpr u16 PID_SCR3500B          = 0x5815; // SCR3500 B Contact Reader
		constexpr u16 PID_UTRUST_3512       = 0x5816; // uTrust 3512 SAM slot Token
		constexpr u16 PID_UTRUST_3522_CC    = 0x5817; // uTrust 3522 embd SE Token
		constexpr u16 PID_MAXX_LIGHT2       = 0x5818; // @MAXX Light2 token
		constexpr u16 PID_MAXX_ID1          = 0x5819; // @MAXX ID-1 Smart Card Reader
		constexpr u16 PID_UTRUST_3522_DI    = 0x581A; // uTrust 3522 embd SE RFID Token
		constexpr u16 PID_CLOUD_2910R_TAG   = 0x581B; // CLOUD 2910 R Taglio SC Reader
		constexpr u16 PID_SCR35XX           = 0x581C; // SCR35xx USB Smart Card Reader
		constexpr u16 PID_SCR3500C          = 0x581D; // SCR3500 C Contact Reader
		constexpr u16 PID_MYAUTH2_CTC       = 0x5821; // myAuthenticate 2.0
		constexpr u16 PID_MYAUTH2_CNT       = 0x5822; // myAuthenticate 2.0
		constexpr u16 PID_UTRUST_2700R_C    = 0x5823; // uTrust 2700 R Smart Card Reader
		constexpr u16 PID_UTRUST_2700R_C5   = 0x5825; // uTrust 2700 R Smart Card Reader
		constexpr u16 PID_UTRUST_TOKEN_FLEX = 0x5826; // uTrust Token Flex

		// Checks if the given PID belongs to an Identiv product range (CLOUD.inf)
		inline bool is_identiv_pid(u16 pid)
		{
			return pid == PID_SCL3711 ||
				pid == PID_CLOUD_2190F ||
				(pid >= 0x5601 && pid <= 0x5614) || // uTrust 5500, 5501, 3720, 3721 & DFU
				(pid >= 0x5700 && pid <= 0x5728) || // uTrust 2700, 4700, 4701, 4710, 4711, CLOUD 4000, SmartOS & DFU
				(pid >= 0x5790 && pid <= 0x5792) || // uTrust / CLOUD 3700 F, 3701 F, SmartOS CL
				(pid >= 0x5810 && pid <= 0x5826);   // uTrust 2700 R, 2900 R, 3512, 3522, SCR3500, myAuth, Flex
		}

		// Checks if the given VID/PID pair corresponds to an Identiv reader
		inline bool is_identiv_vid_pid(u16 vid, u16 pid)
		{
			if (vid == VENDOR_ID_IDENTIV)
			{
				// All smartcard devices under USB VID 0x04E6 are Identiv / SCM Microsystems
				return pid == 0 || is_identiv_pid(pid);
			}

			if (vid == VENDOR_ID_CHICONY && pid == PID_CHICONY_KUS0967)
			{
				return true;
			}

			return false;
		}
	} // namespace identiv

	// Helper to parse USB VID and PID from device paths or identifier strings
	inline bool parse_usb_vid_pid(std::string_view str, u16& out_vid, u16& out_pid)
	{
		if (str.empty())
		{
			return false;
		}

		const std::string lower = fmt::to_lower(str);

		// Format 1: Standard Windows / PnP string: VID_04E6&PID_5790 (or VID_04E6, PID_5790)
		auto vid_pos = lower.find("vid_");
		if (vid_pos == std::string::npos)
		{
			vid_pos = lower.find("vid&");
		}
		if (vid_pos == std::string::npos)
		{
			vid_pos = lower.find("vid=");
		}

		auto pid_pos = lower.find("pid_");
		if (pid_pos == std::string::npos)
		{
			pid_pos = lower.find("pid&");
		}
		if (pid_pos == std::string::npos)
		{
			pid_pos = lower.find("pid=");
		}

		if (vid_pos != std::string::npos && pid_pos != std::string::npos)
		{
			vid_pos += 4;
			pid_pos += 4;
			if (vid_pos + 4 <= lower.size() && pid_pos + 4 <= lower.size())
			{
				u16 vid = 0;
				u16 pid = 0;
				const auto [v_ptr, v_ec] = std::from_chars(lower.data() + vid_pos, lower.data() + vid_pos + 4, vid, 16);
				const auto [p_ptr, p_ec] = std::from_chars(lower.data() + pid_pos, lower.data() + pid_pos + 4, pid, 16);
				if (v_ec == std::errc{} && p_ec == std::errc{})
				{
					out_vid = vid;
					out_pid = pid;
					return true;
				}
			}
		}

		// Format 2: Linux / macOS libusb / ccid style: "04e6:5790" or "04e6/5790"
		const auto colon_pos = lower.find("04e6:");
		if (colon_pos != std::string::npos && colon_pos + 9 <= lower.size())
		{
			u16 pid = 0;
			const auto [p_ptr, p_ec] = std::from_chars(lower.data() + colon_pos + 5, lower.data() + colon_pos + 9, pid, 16);
			if (p_ec == std::errc{})
			{
				out_vid = identiv::VENDOR_ID_IDENTIV;
				out_pid = pid;
				return true;
			}
		}

		const auto slash_pos = lower.find("04e6/");
		if (slash_pos != std::string::npos && slash_pos + 9 <= lower.size())
		{
			u16 pid = 0;
			const auto [p_ptr, p_ec] = std::from_chars(lower.data() + slash_pos + 5, lower.data() + slash_pos + 9, pid, 16);
			if (p_ec == std::errc{})
			{
				out_vid = identiv::VENDOR_ID_IDENTIV;
				out_pid = pid;
				return true;
			}
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

		std::vector<u8> buf(512, 0);
		pcsc_dword_t attr_len = static_cast<pcsc_dword_t>(buf.size() - 1);
		pcsc_long_t ret = SCardGetAttrib(card, attr_id, buf.data(), &attr_len);
		if (ret == C_SUCCESS && attr_len > 0)
		{
			const char* str = reinterpret_cast<const char*>(buf.data());
			return std::string(str, strnlen(str, attr_len));
		}

		// Try two-step length query if buffer was insufficient
		attr_len = 0;
		ret = SCardGetAttrib(card, attr_id, nullptr, &attr_len);
		if (ret == C_SUCCESS && attr_len > 0)
		{
			buf.assign(attr_len + 1, 0);
			ret = SCardGetAttrib(card, attr_id, buf.data(), &attr_len);
			if (ret == C_SUCCESS && attr_len > 0)
			{
				const char* str = reinterpret_cast<const char*>(buf.data());
				return std::string(str, strnlen(str, attr_len));
			}
		}

		return {};
	}
} // namespace

struct pcsc_driver::impl
{
	pcsc_context_t m_context = 0;
	pcsc_handle_t m_card = 0;
	pcsc_dword_t m_active_protocol = C_PROTO_UNDEFINED;
	std::string m_reader_name;
	std::recursive_mutex m_mutex;
	bool m_card_info_cached = false;
	pcsc_card_info m_cached_card_info{};
	bool m_reader_configured = false;
	bool m_reader_config_attempted = false;
	u16 m_vendor_id = 0;
	u16 m_product_id = 0;
	bool m_is_identiv = false;
	bool m_identiv_checked = false;

	// Hardware-only Identiv reader detection using three complementary hardware layers
	// (Zero reliance on reader display name / whitelist string comparison)
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

		// Layer 1: Hardware ID detection via PC/SC SCARD_ATTR_DEVICE_SYSTEM_NAME_A
		// On Windows, returns the OS PnP device interface path, e.g.:
		// "\\?\\USB#VID_04E6&PID_5790#<Serial>#{50dd5230-ba8a-11d1-bf5d-0000f805f530}"
		// Directly yields the USB Vendor ID and Product ID without relying on reader name strings.
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
						nfc_log.trace("PC/SC: Identified Identiv hardware via Device System Name (VID: 0x%04X, PID: 0x%04X)", vid, pid);
						m_is_identiv = true;
						m_identiv_checked = true;
						return true;
					}
				}
			}
		}

		// Layer 2: Hardware Escape Query: READER_GET_IFDTYPE (Escape 0x12)
		// uTrust_370xF_User_Manual.pdf Section 6.3.3.3 (Page 51):
		// Reader firmware directly returns its PID in little-endian format (2 bytes).
		// e.g. 0x90 0x57 for CLOUD 3700 F (PID 0x5790); 0x91 0x57 for CLOUD 3701 F (PID 0x5791).
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
					nfc_log.trace("PC/SC: Identified Identiv hardware via READER_GET_IFDTYPE Escape 0x12 (PID: 0x%04X)", pid);
					m_is_identiv = true;
					m_identiv_checked = true;
					return true;
				}
			}
		}

		// Layer 3: Hardware Vendor attribute: SCARD_ATTR_VENDOR_NAME
		// Driver or USB descriptor iManufacturer string (e.g. "Identiv", "SCM")
		if (m_card != 0)
		{
			const std::string vendor_name = get_pcsc_attribute(m_card, SCARD_ATTR_VENDOR_NAME);
			if (!vendor_name.empty())
			{
				const std::string lower_v = fmt::to_lower(vendor_name);
				if (lower_v.find("identiv") != std::string::npos || lower_v.find("scm") != std::string::npos)
				{
					nfc_log.trace("PC/SC: Identified Identiv hardware via SCARD_ATTR_VENDOR_NAME ('%s')", vendor_name);
					m_is_identiv = true;
					m_identiv_checked = true;
					return true;
				}
			}
		}

		if (m_card != 0)
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
			m_reader_config_attempted = false;
			m_is_identiv = false;
			m_identiv_checked = false;
			m_vendor_id = 0;
			m_product_id = 0;
		}
	}
};

pcsc_driver::pcsc_driver()
	: m_impl(std::make_unique<impl>())
{
}

pcsc_driver::~pcsc_driver()
{
	close();
}

pcsc_driver::pcsc_driver(pcsc_driver&&) noexcept = default;
pcsc_driver& pcsc_driver::operator=(pcsc_driver&&) noexcept = default;

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
	std::lock_guard lock(m_impl->m_mutex);

	if (reader_name.empty() || reader_name == "Fake")
	{
		close();
		return false;
	}

	if (m_impl->m_reader_name == reader_name && m_impl->context_valid())
	{
		return true;
	}

	close();

	if (!m_impl->ensure_context())
	{
		nfc_log.error("PC/SC: Failed to establish context");
		return false;
	}

	m_impl->m_reader_name = reader_name;

	// Try connecting to card if present, or open direct connection
	if (!connect_card())
	{
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
		}
	}

	m_impl->m_reader_configured = false;
	m_impl->m_reader_config_attempted = false;
	configure_reader_if_needed();

	nfc_log.success("PC/SC: Successfully opened reader '%s'", reader_name);
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
		// In Direct mode (no card connected), if CCID Escape is blocked by Windows driver
		// (uTrust Manual Section 6.3.1), do not retry repeatedly during is_card_present() polling.
		// Instead, retry once card is connected in Shared mode where PAPDU_ESCAPE_CMD can be used.
		if (m_impl->m_reader_config_attempted && m_impl->m_active_protocol == C_PROTO_UNDEFINED)
		{
			return;
		}

		m_impl->m_reader_config_attempted = true;

		// Configure contactless polling to detect all supported technologies (Type A, Type B, FeliCa 212/424, Topaz)
		// uTrust Manual Section 6.3.4.4 (Page 60-61): Factory default is Type A + Type B (0x02).
		// Without setting the bitmask to include FeliCa (0x20, 0x40), FeliCa cards will not be detected by the reader firmware.
		// Try standard 3-byte command first {0x95, 0xFF, 0xFF} (All technologies per uTrust Manual Section 6.3.4.4),
		// then fall back to standard NFC Forum bitmask 0xE3 (Type A + B + FeliCa 212/424 + Topaz)
		// or 0x63 (Type A + B + FeliCa 212/424), and finally 2-byte simple mode 0x02 (Type A + Type B).
		static const std::vector<u8> try_set_type_cmds[] = {
			{0x95, 0xFF, 0xFF}, // 3-byte All technologies (uTrust Manual Section 6.3.4.4)
			{0x95, 0xFF, 0xE3}, // 3-byte NFC Forum (Type A, Type B, FeliCa 212/424, Topaz)
			{0x95, 0xFF, 0x63}, // 3-byte Mainstream (Type A, Type B, FeliCa 212/424)
			{0x95, 0x02},       // 2-byte Simple mode: Type A + Type B (uTrust Manual Section 6.3.4.4)
		};

		std::vector<u8> resp;
		for (const auto& cmd : try_set_type_cmds)
		{
			if (send_escape(cmd, resp))
			{
				m_impl->m_reader_configured = true;
				break;
			}
		}

		if (!m_impl->m_reader_configured)
		{
			nfc_log.error("PC/SC: Failed to configure contactless polling types for '%s' (uTrust Escape 0x95 failed). If using default Windows CCID driver, ensure EscapeCommandEnable registry key is set to 1.", m_impl->m_reader_name);
		}
	}
	else
	{
		m_impl->m_reader_configured = true;
	}
}

void pcsc_driver::close()
{
	std::lock_guard lock(m_impl->m_mutex);
	disconnect_card(false);
	m_impl->m_card_info_cached = false;
	m_impl->m_cached_card_info = {};
	m_impl->m_reader_configured = false;
	m_impl->m_reader_config_attempted = false;
	m_impl->m_is_identiv = false;
	m_impl->m_identiv_checked = false;
	m_impl->m_vendor_id = 0;
	m_impl->m_product_id = 0;
	m_impl->release_context();
	m_impl->m_reader_name.clear();
}

bool pcsc_driver::is_open() const
{
	if (!m_impl)
	{
		return false;
	}

	std::lock_guard lock(m_impl->m_mutex);
	return !m_impl->m_reader_name.empty() && m_impl->m_reader_name != "Fake";
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
	std::lock_guard lock(m_impl->m_mutex);
	if (m_impl->m_reader_name.empty())
	{
		return false;
	}

	if (!m_impl->ensure_context())
	{
		return false;
	}

	if (!m_impl->m_reader_configured)
	{
		configure_reader_if_needed();
	}

	pcsc_reader_state_t state{};
	state.szReader = m_impl->m_reader_name.c_str();
	state.dwCurrentState = C_STATE_UNAWARE;

	const pcsc_long_t ret = SCardGetStatusChange(m_impl->m_context, 0, &state, 1);
	if (ret == C_SUCCESS)
	{
		const bool present = (state.dwEventState & C_STATE_PRESENT) != 0;
		if (!present)
		{
			if (m_impl->m_active_protocol != C_PROTO_UNDEFINED)
			{
				disconnect_card(false);
			}
			else
			{
				m_impl->m_card_info_cached = false;
				m_impl->m_cached_card_info = {};
			}
		}
		return present;
	}

	// Reader may have been unplugged or USB communication disrupted
	m_impl->m_card_info_cached = false;
	m_impl->m_cached_card_info = {};
	if (m_impl->m_card != 0)
	{
		SCardDisconnect(m_impl->m_card, C_LEAVE_CARD);
		m_impl->m_card = 0;
		m_impl->m_active_protocol = C_PROTO_UNDEFINED;
	}

	// Invalidate stale context on critical errors so next call will re-establish automatically upon reconnection
	if (ret == static_cast<pcsc_long_t>(0x80100003) /* SCARD_E_INVALID_HANDLE */ ||
		ret == static_cast<pcsc_long_t>(0x80100009) /* SCARD_E_UNKNOWN_READER */ ||
		ret == static_cast<pcsc_long_t>(0x80100017) /* SCARD_E_READER_UNAVAILABLE */ ||
		ret == static_cast<pcsc_long_t>(0x8010001D) /* SCARD_E_NO_SERVICE */ ||
		ret == static_cast<pcsc_long_t>(0x8010001E) /* SCARD_E_SERVICE_STOPPED */ ||
		ret == static_cast<pcsc_long_t>(0x8010002F) /* SCARD_E_COMM_DATA_LOST */)
	{
		m_impl->release_context();
		m_impl->m_reader_configured = false;
	}

	return false;
}

bool pcsc_driver::connect_card()
{
	std::lock_guard lock(m_impl->m_mutex);

	if (m_impl->m_reader_name.empty())
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
		if (SCardStatus(m_impl->m_card, rname, &rlen, &state, &proto, atr, &atrlen) == C_SUCCESS &&
			state != C_STATUS_ABSENT && state > 0)
		{
			return true;
		}
		disconnect_card(false);
	}

	// If connected in direct mode, disconnect before connecting in shared mode
	if (m_impl->m_card != 0)
	{
		SCardDisconnect(m_impl->m_card, C_LEAVE_CARD);
		m_impl->m_card = 0;
		m_impl->m_active_protocol = C_PROTO_UNDEFINED;
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
		return true;
	}

	m_impl->m_card = 0;
	m_impl->m_active_protocol = C_PROTO_UNDEFINED;
	return false;
}

void pcsc_driver::disconnect_card(bool reset)
{
	std::lock_guard lock(m_impl->m_mutex);
	m_impl->m_card_info_cached = false;
	m_impl->m_cached_card_info = {};
	if (m_impl->m_card != 0)
	{
		SCardDisconnect(m_impl->m_card, reset ? C_RESET_CARD : C_LEAVE_CARD);
		m_impl->m_card = 0;
		m_impl->m_active_protocol = C_PROTO_UNDEFINED;
	}
}

void pcsc_driver::invalidate_card_cache()
{
	std::lock_guard lock(m_impl->m_mutex);
	m_impl->m_card_info_cached = false;
	m_impl->m_cached_card_info = {};
}

pcsc_card_info pcsc_driver::get_card_info()
{
	std::lock_guard lock(m_impl->m_mutex);

	pcsc_card_info info{};
	if (!is_card_present())
	{
		disconnect_card(false);
		m_impl->m_card_info_cached = false;
		m_impl->m_cached_card_info = {};
		return info;
	}

	if (m_impl->m_card_info_cached && m_impl->m_cached_card_info.present)
	{
		return m_impl->m_cached_card_info;
	}

	info.present = true;

	if (!connect_card())
	{
		return info;
	}

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

	// Query UID using PAPDU_GET_UID (FF CA 00 00 00)
	// uTrust_370xF_User_Manual.pdf Section 6.1.2 / PC/SC Part 3 Section 3.2.2.1.3
	std::vector<u8> get_uid_apdu = {0xFF, 0xCA, 0x00, 0x00, 0x00};
	std::vector<u8> uid_resp;
	if (transmit_apdu(get_uid_apdu, uid_resp) && uid_resp.size() >= 2)
	{
		u16 sw = static_cast<u16>((uid_resp[uid_resp.size() - 2] << 8) | uid_resp.back());

		// ISO 7816-4: If reader/card requests exact Le via SW 6C xx, re-issue with specified length
		if ((sw & 0xFF00) == 0x6C00)
		{
			get_uid_apdu[4] = static_cast<u8>(sw & 0xFF);
			if (transmit_apdu(get_uid_apdu, uid_resp) && uid_resp.size() >= 2)
			{
				sw = static_cast<u16>((uid_resp[uid_resp.size() - 2] << 8) | uid_resp.back());
			}
		}

		if (sw == 0x9000)
		{
			uid_resp.resize(uid_resp.size() - 2);
			info.uid = std::move(uid_resp);
		}
	}

	// Fallback for non-standard readers rejecting Le=0x00 with SW 67 00 (Wrong length)
	if (info.uid.empty())
	{
		for (u8 try_le : {0x04, 0x07, 0x08, 0x0A})
		{
			get_uid_apdu[4] = try_le;
			if (transmit_apdu(get_uid_apdu, uid_resp) && uid_resp.size() >= 2)
			{
				const u16 sw = static_cast<u16>((uid_resp[uid_resp.size() - 2] << 8) | uid_resp.back());
				if (sw == 0x9000)
				{
					uid_resp.resize(uid_resp.size() - 2);
					info.uid = std::move(uid_resp);
					break;
				}
			}
		}
	}

	bool is_type_a = true;

	// Try uTrust Escape 0xDA (CNTLESS_GET_CARD_DETAILS)
	// uTrust_370xF_User_Manual.pdf Section 6.3.4.12 (Page 66-67)
	std::vector<u8> details_in = {0xDA};
	std::vector<u8> details_out;
	if (send_escape(details_in, details_out) && details_out.size() >= 3)
	{
		const u8 card_tech = details_out[0];
		const u8 card_arch = details_out[1];

		// Byte 0: 0x00=Type A, 0x01=Type B, 0x04=FeliCa 212, 0x08=FeliCa 424
		// Byte 1: 0x00=Memory card, 0x01=T-CL card, 0x43=FeliCa
		if (card_tech == 0x04 || card_tech == 0x08 || card_arch == 0x43)
		{
			info.is_felica = true;
			if (details_out.size() >= 11 && info.uid.empty())
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

			const u8 uid_len = details_out[2];
			if (info.uid.empty() && uid_len > 0 && details_out.size() >= 3u + uid_len)
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
			if (card_arch == 0x01 || card_arch == 0x02)
			{
				info.is_iso14443_4 = true;
			}

			const u8 uid_len = details_out[2];
			if (info.uid.empty() && uid_len > 0 && details_out.size() >= 3u + uid_len)
			{
				info.uid.assign(details_out.begin() + 3, details_out.begin() + 3 + uid_len);
			}
		}
	}
	else
	{
		// Fallback for older uTrust / SCM firmware: CNTLESS_GET_CARD_INFO (Escape 0x11)
		// uTrust_370xF_User_Manual.pdf Section 6.3.4.1 (Page 58)
		std::vector<u8> info_in = {0x11};
		std::vector<u8> info_out;
		if (send_escape(info_in, info_out) && info_out.size() >= 3)
		{
			const u8 card_type_info = info_out[2];
			const u8 upper = (card_type_info >> 4) & 0x0F;
			const u8 lower = card_type_info & 0x0F;
			is_type_a = (lower == 0x00);
			if (upper == 0x00)
			{
				info.is_mifare = true;
			}
			else if (upper == 0x01 || upper == 0x02)
			{
				info.is_iso14443_4 = true;
			}
		}
	}

	// Parse ATR if technology is not yet definitively determined
	if (!info.atr.empty() && !info.is_felica)
	{
		bool is_pcsc_storage = false;
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
					if (!info.is_felica)
					{
						info.is_mifare = true;
					}
					break;
				}
				break; // Only process the first RID match
			}
		}

		if (is_pcsc_storage)
		{
			// Storage tokens (MIFARE Classic, Ultralight, FeliCa) are not ISO 14443-4 T=CL cards
			info.is_iso14443_4 = false;
		}
		else if (!info.is_mifare && !info.is_felica)
		{
			if (info.sak_valid)
			{
				if ((info.sak & 0x20) != 0)
				{
					info.is_iso14443_4 = true;
				}
				else if (info.sak == 0x00 && info.uid.size() == 7)
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
				else if (info.sak == 0x18)
				{
					// MIFARE Classic 4K
					info.is_mifare = true;
					info.atqa = 0x0002;
				}
				else if (info.sak == 0x08 || (info.sak & 0x08) != 0)
				{
					// MIFARE Classic 1K / compatible (SAK 0x08, Infineon clone SAK 0x88, Plus SL1 SAK 0x19)
					info.is_mifare = true;
					info.atqa = 0x0004;
				}
			}
			else
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

	// If 8-byte UID is present and not explicitly MIFARE, treat as FeliCa NFCID2
	if (info.uid.size() == 8 && !info.is_mifare)
	{
		info.is_felica = true;
	}

	// Retrieve Historical Bytes for ISO 14443-4 T=CL cards
	if (info.is_iso14443_4)
	{
		// Method 1: uTrust Escape 0x93 (CNTLESS_GET_ATS_ATQB) - uTrust Manual Section 6.3.4.2
		// Returns full ATS for Type A T=CL cards; parse to extract historical bytes only
		// ATS format: [TL] [T0] [TA(1) if T0 bit4] [TB(1) if T0 bit5] [TC(1) if T0 bit6] [Hist. Bytes]
		// (Only applicable for Type A; for Type B, Escape 0x93 returns ATQB instead of ATS)
		if (is_type_a)
		{
			std::vector<u8> ats_in = {0x93};
			std::vector<u8> ats_out;
			if (send_escape(ats_in, ats_out) && ats_out.size() >= 2)
			{
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
				const usz max_ats_len = std::min(ats_out.size(), static_cast<usz>(ats_out[0]));
				if (hb_offset < max_ats_len)
				{
					info.historical_bytes.assign(ats_out.begin() + hb_offset, ats_out.begin() + max_ats_len);
				}
			}
		}

		// Method 2: Standard PC/SC Get Historical Bytes (FF CA 01 00 00) - PC/SC Part 3 Section 3.2.2.1.3
		if (info.historical_bytes.empty())
		{
			std::vector<u8> get_hb = {0xFF, 0xCA, 0x01, 0x00, 0x00};
			std::vector<u8> hb_resp;
			if (transmit_apdu(get_hb, hb_resp) && hb_resp.size() >= 2)
			{
				u16 sw = static_cast<u16>((hb_resp[hb_resp.size() - 2] << 8) | hb_resp.back());

				// ISO 7816-4: If reader/card requests exact Le via SW 6C xx, re-issue with specified length
				if ((sw & 0xFF00) == 0x6C00)
				{
					get_hb[4] = static_cast<u8>(sw & 0xFF);
					if (transmit_apdu(get_hb, hb_resp) && hb_resp.size() >= 2)
					{
						sw = static_cast<u16>((hb_resp[hb_resp.size() - 2] << 8) | hb_resp.back());
					}
				}

				if (sw == 0x9000)
				{
					hb_resp.resize(hb_resp.size() - 2);
					info.historical_bytes = std::move(hb_resp);
				}
			}
		}

		// Method 3: Parse directly from ATR (uTrust Manual Section 5.3.3.2 Page 25 / PC/SC Part 3 Section 3.1.3.2.3)
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

	// If card is FeliCa, retrieve real PMm and System Code
	if (info.is_felica)
	{
		// Method 1: Sony PaSoRi (RC-S380) / standard PC/SC GET DATA for FeliCa PMm (FF CA 01 00 00)
		// Sony SDK for NFC User's Manual (PC/SC Edition) Section 3.2.6
		if (info.pmm.empty())
		{
			std::vector<u8> get_pmm_apdu = {0xFF, 0xCA, 0x01, 0x00, 0x00};
			std::vector<u8> pmm_resp;
			if (transmit_apdu(get_pmm_apdu, pmm_resp) && pmm_resp.size() >= 2)
			{
				u16 sw = static_cast<u16>((pmm_resp[pmm_resp.size() - 2] << 8) | pmm_resp.back());
				if ((sw & 0xFF00) == 0x6C00)
				{
					get_pmm_apdu[4] = static_cast<u8>(sw & 0xFF);
					if (transmit_apdu(get_pmm_apdu, pmm_resp) && pmm_resp.size() >= 2)
					{
						sw = static_cast<u16>((pmm_resp[pmm_resp.size() - 2] << 8) | pmm_resp.back());
					}
				}
				if (sw == 0x9000 && pmm_resp.size() == 10) // 8B PMm + 2B SW
				{
					pmm_resp.resize(8);
					info.pmm = std::move(pmm_resp);
				}
			}
		}

		// Method 2: Pseudo-APDU PAPDU_FELICA_REQC (uTrust Manual Section 6.2.13, Page 41)
		// APDU: FF 40 00 00 04 FF FF 01 00
		// Data: SystemCode(FFFF) + RequestCode(01=Request System Code) + TimeSlot(00=1 slot)
		// Note: Some manual revisions label byte 7 as RFU; firmware treats it as FeliCa Request Code.
		// Returns: 16 bytes NFCID2 (8B IDm + 8B PMm), 2 bytes System Code, SW1, SW2
		std::vector<u8> reqc_apdu = {0xFF, 0x40, 0x00, 0x00, 0x04, 0xFF, 0xFF, 0x01, 0x00};
		std::vector<u8> reqc_resp;
		if (transmit_apdu(reqc_apdu, reqc_resp) && reqc_resp.size() >= 18)
		{
			const u16 sw = static_cast<u16>((reqc_resp[reqc_resp.size() - 2] << 8) | reqc_resp.back());
			if (sw == 0x9000)
			{
				info.uid.assign(reqc_resp.begin(), reqc_resp.begin() + 8);
				info.pmm.assign(reqc_resp.begin() + 8, reqc_resp.begin() + 16);
				if (reqc_resp.size() >= 20)
				{
					info.system_code = static_cast<u16>((reqc_resp[16] << 8) | reqc_resp[17]);
				}
			}
		}

		// Method 3: Fallback to raw FeliCa Polling frame via felica_pass_thru if PMm or System Code still missing
		if (info.pmm.empty() || info.system_code == 0xFFFF)
		{
			const std::vector<u8> poll_cmd = {0x06, 0x00, 0xFF, 0xFF, 0x01, 0x00};
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

	m_impl->m_cached_card_info = info;
	m_impl->m_card_info_cached = true;
	return info;
}

bool pcsc_driver::transmit_apdu(std::span<const u8> apdu, std::vector<u8>& response)
{
	std::lock_guard lock(m_impl->m_mutex);
	response.clear();

	if (apdu.empty() || !connect_card())
	{
		return false;
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

	// Reset card handle and cache only upon fatal communication failure or card removal
	if (ret == static_cast<pcsc_long_t>(0x80100069) /* SCARD_W_REMOVED_CARD */ ||
		ret == static_cast<pcsc_long_t>(0x80100068) /* SCARD_W_RESET_CARD */ ||
		ret == static_cast<pcsc_long_t>(0x80100003) /* SCARD_E_INVALID_HANDLE */ ||
		ret == static_cast<pcsc_long_t>(0x80100009) /* SCARD_E_UNKNOWN_READER */ ||
		ret == static_cast<pcsc_long_t>(0x80100017) /* SCARD_E_READER_UNAVAILABLE */ ||
		ret == static_cast<pcsc_long_t>(0x8010001D) /* SCARD_E_NO_SERVICE */ ||
		ret == static_cast<pcsc_long_t>(0x8010001E) /* SCARD_E_SERVICE_STOPPED */ ||
		ret == static_cast<pcsc_long_t>(0x8010002F) /* SCARD_E_COMM_DATA_LOST */)
	{
		disconnect_card(false);
		if (ret != static_cast<pcsc_long_t>(0x80100069) && ret != static_cast<pcsc_long_t>(0x80100068))
		{
			m_impl->release_context();
		}
	}

	return false;
}

bool pcsc_driver::control(u32 control_code, std::span<const u8> in_data, std::vector<u8>& out_data)
{
	std::lock_guard lock(m_impl->m_mutex);
	out_data.clear();
	if (!is_open())
	{
		return false;
	}

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

	// Reset stale card handle and cache upon fatal communication failure (do not discard handle on unsupported IOCTL)
	if (ret == static_cast<pcsc_long_t>(0x80100069) /* SCARD_W_REMOVED_CARD */ ||
		ret == static_cast<pcsc_long_t>(0x80100068) /* SCARD_W_RESET_CARD */ ||
		ret == static_cast<pcsc_long_t>(0x80100003) /* SCARD_E_INVALID_HANDLE */ ||
		ret == static_cast<pcsc_long_t>(0x80100009) /* SCARD_E_UNKNOWN_READER */ ||
		ret == static_cast<pcsc_long_t>(0x80100017) /* SCARD_E_READER_UNAVAILABLE */ ||
		ret == static_cast<pcsc_long_t>(0x8010001D) /* SCARD_E_NO_SERVICE */ ||
		ret == static_cast<pcsc_long_t>(0x8010001E) /* SCARD_E_SERVICE_STOPPED */ ||
		ret == static_cast<pcsc_long_t>(0x8010002F) /* SCARD_E_COMM_DATA_LOST */)
	{
		disconnect_card(false);
		if (ret != static_cast<pcsc_long_t>(0x80100069) && ret != static_cast<pcsc_long_t>(0x80100068))
		{
			m_impl->release_context();
		}
		else
		{
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
	}

	return false;
}

bool pcsc_driver::send_escape(std::span<const u8> escape_data, std::vector<u8>& response)
{
	response.clear();
	if (escape_data.empty())
	{
		return false;
	}

	// Method 1: SCardControl via CCID_ESCAPE_CTL (uTrust Manual Section 6.3.1)
	if (control(CCID_ESCAPE_CTL, escape_data, response))
	{
		return true;
	}

#if defined(_WIN32)
	// On Windows, try IOCTL_SMARTCARD_VENDOR_IFD_EXCHANGE if standard CCID escape (3500) fails or is blocked
	constexpr pcsc_dword_t CCID_VENDOR_CTL = SCARD_CTL_CODE(2048);
	if (control(CCID_VENDOR_CTL, escape_data, response))
	{
		return true;
	}
#else
	// On Linux / macOS pcsc-lite, some CCID drivers map escape to SCARD_CTL_CODE(1)
	constexpr pcsc_dword_t CCID_ESCAPE_CTL_ALT = (0x42000000 + 1);
	if (control(CCID_ESCAPE_CTL_ALT, escape_data, response))
	{
		return true;
	}
#endif

	// Method 2: Fallback to vendor-specific PAPDU_ESCAPE_CMD (uTrust Manual Section 6.1.3)
	// Command APDU: FF CC 00 00 Lc <escape_data>
	// Used when CCID Escape IOCTL is blocked by the OS driver
	if (escape_data.size() <= 255)
	{
		std::vector<u8> apdu;
		if (escape_data.size() >= 5 && escape_data[0] == 0xFF && escape_data[1] == 0x70)
		{
			// Note 2: Commands defined using READER_GENERIC_ESCAPE (FF 70 04 E6 Lc ...) have ISO 7816 APDU format
			// and are transmitted directly without prepending FF CC 00 00
			apdu.assign(escape_data.begin(), escape_data.end());
		}
		else
		{
			apdu = {0xFF, 0xCC, 0x00, 0x00, static_cast<u8>(escape_data.size())};
			apdu.insert(apdu.end(), escape_data.begin(), escape_data.end());
		}

		std::vector<u8> rapdu;
		if (transmit_apdu(apdu, rapdu) && rapdu.size() >= 2)
		{
			const u8 sw1 = rapdu[rapdu.size() - 2];
			const u8 sw2 = rapdu.back();
			if (sw1 == 0x90 && sw2 == 0x00)
			{
				rapdu.resize(rapdu.size() - 2);
				response = std::move(rapdu);
				return true;
			}
		}
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

	const bool has_len = (felica_cmd[0] == felica_cmd.size() && felica_cmd.size() >= 2);
	const usz base = has_len ? 1 : 0;
	const u8 fcmd = felica_cmd[base];

	// Method 1: Identiv CNTLESS_FELICA_PASS_THRU (Escape Code 0xF3)
	// uTrust_370xF_User_Manual.pdf Section 6.3.4.15 (Page 69)
	// Input:  [0xF3] [FeliCa command bytes (DataOut)]
	// Output: [FeliCa response bytes (DataIn)]
	std::vector<u8> escape_cmd;
	escape_cmd.reserve(1 + felica_cmd.size());
	escape_cmd.push_back(0xF3); // CNTLESS_FELICA_PASS_THRU
	escape_cmd.insert(escape_cmd.end(), felica_cmd.begin(), felica_cmd.end());

	std::vector<u8> escape_resp;

	auto is_valid_felica = [&](std::vector<u8>& resp) -> bool
	{
		if (resp.size() >= 2 && resp[1] == static_cast<u8>(fcmd + 1))
		{
			// Byte 0 is length byte, Byte 1 is Response Code. Truncate driver padding if any.
			if (resp.size() >= resp[0] && resp[0] >= 2)
			{
				resp.resize(resp[0]);
			}
		}
		else if (!resp.empty() && resp[0] == static_cast<u8>(fcmd + 1))
		{
			// Reader firmware stripped length byte; prepend length byte
			resp.insert(resp.begin(), static_cast<u8>(resp.size() + 1));
		}

		// Validate that the response conforms to standard FeliCa framing:
		// Byte 0 is length (must match buffer size), Byte 1 is Response Code (must match Command Code + 1)
		return resp.size() >= 2 && resp[0] == resp.size() && resp[1] == static_cast<u8>(fcmd + 1);
	};

	bool valid_resp = send_escape(escape_cmd, escape_resp) && is_valid_felica(escape_resp);

	// Fallback 1: If primary format failed to obtain a valid FeliCa response, try alternative framing format
	// (uTrust manual: "At least 1 byte of command is required"; toggle between with and without length byte)
	if (!valid_resp)
	{
		std::vector<u8> alt_cmd = {0xF3};
		if (has_len && felica_cmd.size() > 1)
		{
			alt_cmd.insert(alt_cmd.end(), felica_cmd.begin() + 1, felica_cmd.end());
		}
		else if (!has_len)
		{
			alt_cmd.push_back(static_cast<u8>(felica_cmd.size() + 1));
			alt_cmd.insert(alt_cmd.end(), felica_cmd.begin(), felica_cmd.end());
		}

		valid_resp = send_escape(alt_cmd, escape_resp) && is_valid_felica(escape_resp);
	}

	if (valid_resp)
	{
		felica_resp = std::move(escape_resp);
		return true;
	}

	// Method 2: Fallback to Pseudo-APDU based on FeliCa command opcode
	// E.g. PAPDU_FELICA_REQC (FF 40 00 00 04 ...) for Polling (cmd 0x00)
	if (felica_cmd.size() >= base + 1)
	{
		if (fcmd == 0x00 && felica_cmd.size() >= base + 3) // Polling (System Code requires at least 2 bytes)
		{
			const u8 sys_msb = felica_cmd[base + 1];
			const u8 sys_lsb = felica_cmd[base + 2];
			// JIS X 6319-4 Section 8.2.3: Defaults are 0x00 for Request Code and Time Slot Number if omitted
			const u8 req_code = (felica_cmd.size() >= base + 4) ? felica_cmd[base + 3] : 0x00;
			const u8 tsn = (felica_cmd.size() >= base + 5) ? felica_cmd[base + 4] : 0x00;

			std::vector<u8> apdu = {0xFF, 0x40, 0x00, 0x00, 0x04, sys_msb, sys_lsb, req_code, tsn};
			std::vector<u8> rapdu;
			if (transmit_apdu(apdu, rapdu) && rapdu.size() >= 18) // At least 16B payload (IDm + PMm) + 2B SW
			{
				const u16 sw = static_cast<u16>((rapdu[rapdu.size() - 2] << 8) | rapdu.back());
				if (sw == 0x9000)
				{
					// Synthesize standard FeliCa polling response:
					// Length byte + RespCode (0x01) + NFCID2 (8B) + PMm (8B) [+ 2B SystemCode if returned]
					const usz data_len = rapdu.size() - 2;
					felica_resp.resize(2 + data_len);
					felica_resp[0] = static_cast<u8>(2 + data_len);
					felica_resp[1] = 0x01;
					std::memcpy(felica_resp.data() + 2, rapdu.data(), data_len);
					return true;
				}
			}
		}
		else if (fcmd == 0x02 && felica_cmd.size() >= base + 10) // Request Service (PAPDU_FELICA_REQ_SERVICE, Section 6.2.14)
		{
			// FeliCa raw ReqService: [Len: 1B] [0x02] [IDm: 8B] [NumServices: 1B] [ServiceList: 2*N]
			const u8 num_services = felica_cmd[base + 9];
			const usz srv_offset = base + 10;
			const usz srv_len = num_services * 2;
			if (felica_cmd.size() >= srv_offset + srv_len && srv_len <= 255)
			{
				std::vector<u8> apdu = {0xFF, 0x42, num_services, 0x00, static_cast<u8>(srv_len)};
				apdu.insert(apdu.end(), felica_cmd.begin() + srv_offset, felica_cmd.begin() + srv_offset + srv_len);

				std::vector<u8> rapdu;
				if (transmit_apdu(apdu, rapdu) && rapdu.size() >= 11) // 8B IDm + 1B NumServices + 2B SW (JIS X 6319-4 Section 8.2.4 / uTrust Section 6.2.14)
				{
					const u16 sw = static_cast<u16>((rapdu[rapdu.size() - 2] << 8) | rapdu.back());
					if (sw == 0x9000)
					{
						// rapdu data: 8B IDm + 1B NumServices/Areas (n) + 2*n B Service versions
						const usz data_len = rapdu.size() - 2;
						felica_resp.resize(2 + data_len);
						felica_resp[0] = static_cast<u8>(2 + data_len);
						felica_resp[1] = 0x03; // ReqService response code
						std::memcpy(felica_resp.data() + 2, rapdu.data(), data_len);
						return true;
					}
				}
			}
		}
		else if (fcmd == 0x04 && felica_cmd.size() >= base + 9) // Request Response (PAPDU_FELICA_REQ_RESPONSE, Section 6.2.15)
		{
			// FeliCa raw ReqResponse: [Len: 1B] [0x04] [IDm: 8B]
			std::vector<u8> apdu = {0xFF, 0x44, 0x00, 0x00, 0x00};
			std::vector<u8> rapdu;
			if (transmit_apdu(apdu, rapdu) && rapdu.size() >= 11) // 8B IDm + 1B Mode + 2B SW
			{
				const u16 sw = static_cast<u16>((rapdu[rapdu.size() - 2] << 8) | rapdu.back());
				if (sw == 0x9000)
				{
					const usz data_len = rapdu.size() - 2;
					felica_resp.resize(2 + data_len);
					felica_resp[0] = static_cast<u8>(2 + data_len);
					felica_resp[1] = 0x05; // ReqResponse response code
					std::memcpy(felica_resp.data() + 2, rapdu.data(), data_len);
					return true;
				}
			}
		}
		else if (fcmd == 0x06 && felica_cmd.size() >= base + 11) // Read Without Encryption
		{
			// Data: [Len] [0x06] [IDm: 8B] [NumServices: 1B] [ServiceList: 2*N] [NumBlocks: 1B] [BlockList: 2~3*M]
			const u8 num_services = felica_cmd[base + 9];
			const usz srv_offset = base + 10;
			const usz srv_len = num_services * 2;
			if (felica_cmd.size() > srv_offset + srv_len)
			{
				const u8 num_blocks = felica_cmd[srv_offset + srv_len];
				const usz blk_offset = srv_offset + srv_len + 1;
				// Calculate actual block list byte length (FeliCa elements may be 2 or 3 bytes based on bit 7)
				usz blk_len = 0;
				{
					usz scan = blk_offset;
					for (u8 b = 0; b < num_blocks && scan < felica_cmd.size(); ++b)
					{
						// JIS X 6319-4 Section 8.2.7: Bit 7 = 1 is 2-byte element, Bit 7 = 0 is 3-byte element
						const usz elem_sz = (felica_cmd[scan] & 0x80) ? 2 : 3;
						blk_len += elem_sz;
						scan += elem_sz;
					}
				}
				if (felica_cmd.size() >= blk_offset + blk_len)
				{
					// Identiv Section 6.2.16: FF 46 P1(NumServices) P2(NumBlocks) P3(2*(P1+P2)) Data(ServiceList, BlockList)
					// uTrust PAPDU assumes all block list elements are 2 bytes; skip if 3-byte elements are present
					const usz payload_len = srv_len + blk_len;
					if (payload_len <= 255 && blk_len == static_cast<usz>(num_blocks) * 2)
					{
						std::vector<u8> apdu = {0xFF, 0x46, num_services, num_blocks, static_cast<u8>(payload_len)};
						apdu.insert(apdu.end(), felica_cmd.begin() + srv_offset, felica_cmd.begin() + srv_offset + srv_len);
						apdu.insert(apdu.end(), felica_cmd.begin() + blk_offset, felica_cmd.begin() + blk_offset + blk_len);

						std::vector<u8> rapdu;
						if (transmit_apdu(apdu, rapdu) && rapdu.size() >= 12) // 8B IDm + 1B SF1 + 1B SF2 + 2B SW (JIS X 6319-4 Section 8.2 / uTrust Section 6.2.16)
						{
							const u16 sw = static_cast<u16>((rapdu[rapdu.size() - 2] << 8) | rapdu.back());
							if (sw == 0x9000)
							{
								rapdu.resize(rapdu.size() - 2);
								felica_resp.clear();
								felica_resp.push_back(static_cast<u8>(rapdu.size() + 2));
								felica_resp.push_back(0x07); // Read response code
								felica_resp.insert(felica_resp.end(), rapdu.begin(), rapdu.end());
								return true;
							}
						}
					}
				}
			}
		}
		else if (fcmd == 0x08 && felica_cmd.size() >= base + 11) // Write Without Encryption (PAPDU_FELICA_WRITE_BLK, Section 6.2.17)
		{
			// FeliCa raw Write: [Len: 1B] [0x08] [IDm: 8B] [NumServices: 1B] [ServiceList: 2*N] [NumBlocks: 1B] [BlockList: 2~3*M] [BlockData: 16*M]
			const u8 num_services = felica_cmd[base + 9];
			const usz srv_offset = base + 10;
			const usz srv_len = num_services * 2;
			if (felica_cmd.size() > srv_offset + srv_len)
			{
				const u8 num_blocks = felica_cmd[srv_offset + srv_len];
				const usz blk_offset = srv_offset + srv_len + 1;
				// Calculate actual block list byte length (FeliCa elements may be 2 or 3 bytes based on bit 7)
				usz blk_len = 0;
				{
					usz scan = blk_offset;
					for (u8 b = 0; b < num_blocks && scan < felica_cmd.size(); ++b)
					{
						// JIS X 6319-4 Section 8.2.7: Bit 7 = 1 is 2-byte element, Bit 7 = 0 is 3-byte element
						const usz elem_sz = (felica_cmd[scan] & 0x80) ? 2 : 3;
						blk_len += elem_sz;
						scan += elem_sz;
					}
				}
				const usz data_offset = blk_offset + blk_len;
				const usz data_len = num_blocks * 16;
				if (felica_cmd.size() >= data_offset + data_len)
				{
					// Identiv Section 6.2.17: FF 48 P1(NumServices) P2(NumBlocks) P3(2*(P1+P2) + 16*P2) Data(ServiceList, BlockList, BlockData)
					// uTrust PAPDU assumes all block list elements are 2 bytes; skip if 3-byte elements are present
					const usz payload_len = srv_len + blk_len + data_len;
					if (payload_len <= 255 && blk_len == static_cast<usz>(num_blocks) * 2)
					{
						std::vector<u8> apdu = {0xFF, 0x48, num_services, num_blocks, static_cast<u8>(payload_len)};
						apdu.insert(apdu.end(), felica_cmd.begin() + srv_offset, felica_cmd.begin() + srv_offset + srv_len);
						apdu.insert(apdu.end(), felica_cmd.begin() + blk_offset, felica_cmd.begin() + blk_offset + blk_len);
						apdu.insert(apdu.end(), felica_cmd.begin() + data_offset, felica_cmd.begin() + data_offset + data_len);

						std::vector<u8> rapdu;
						if (transmit_apdu(apdu, rapdu) && rapdu.size() >= 12) // 8B IDm + 1B SF1 + 1B SF2 + 2B SW (JIS X 6319-4 Section 8.2 / uTrust Section 6.2.17)
						{
							const u16 sw = static_cast<u16>((rapdu[rapdu.size() - 2] << 8) | rapdu.back());
							if (sw == 0x9000)
							{
								// Synthesize FeliCa Write response: [Len=12 (0x0C)] [RespCode=0x09] [IDm: 8B] [SF1: 1B] [SF2: 1B]
								felica_resp.resize(12);
								felica_resp[0] = 0x0C;
								felica_resp[1] = 0x09;
								std::memcpy(felica_resp.data() + 2, rapdu.data(), 10);
								return true;
							}
						}
					}
				}
			}
		}
		else if (fcmd == 0x0C && felica_cmd.size() >= base + 9) // Request System Code (PAPDU_FELICA_SYS_CODE, Section 6.2.18)
		{
			// FeliCa raw ReqSystemCode: [Len: 1B = 0x0A] [0x0C] [IDm: 8B]
			std::vector<u8> apdu = {0xFF, 0x4A, 0x00, 0x00, 0x00};
			std::vector<u8> rapdu;
			if (transmit_apdu(apdu, rapdu) && rapdu.size() >= 11) // 8B IDm + 1B NumSysCodes + 2*n B SysCodes + 2B SW
			{
				const u16 sw = static_cast<u16>((rapdu[rapdu.size() - 2] << 8) | rapdu.back());
				if (sw == 0x9000)
				{
					const usz data_len = rapdu.size() - 2;
					felica_resp.resize(2 + data_len);
					felica_resp[0] = static_cast<u8>(2 + data_len);
					felica_resp[1] = 0x0D; // ReqSystemCode response code
					std::memcpy(felica_resp.data() + 2, rapdu.data(), data_len);
					return true;
				}
			}
		}
	}

	// Assemble RF frame with mandatory JIS X 6319-4 Section 6.2 length byte prefix if input lacks it
	std::vector<u8> rf_frame_buf;
	if (!has_len)
	{
		rf_frame_buf.reserve(1 + felica_cmd.size());
		rf_frame_buf.push_back(static_cast<u8>(felica_cmd.size() + 1));
		rf_frame_buf.insert(rf_frame_buf.end(), felica_cmd.begin(), felica_cmd.end());
	}
	const std::span<const u8> rf_cmd = has_len ? felica_cmd : std::span<const u8>(rf_frame_buf);

	// Method 3: PC/SC Part 3 Supplemental Document Transparent Exchange (FF C2 00 01)
	// Standard transceive object: Tag 0x95 [Len] [Data]. Response: Tag 0x97 [Len] [ICC Response Data] + Tag 0x96 [Len] [Status] + 90 00
	if (rf_cmd.size() <= 252)
	{
		// ISO/IEC 7816-4 Section 5.2.2 BER-TLV length encoding:
		// Length <= 127 uses 1 byte; length > 127 uses 0x81 followed by 1 byte length
		std::vector<u8> trans_apdu;
		if (rf_cmd.size() <= 127)
		{
			trans_apdu = {0xFF, 0xC2, 0x00, 0x01, static_cast<u8>(rf_cmd.size() + 2), 0x95, static_cast<u8>(rf_cmd.size())};
		}
		else
		{
			trans_apdu = {0xFF, 0xC2, 0x00, 0x01, static_cast<u8>(rf_cmd.size() + 3), 0x95, 0x81, static_cast<u8>(rf_cmd.size())};
		}
		trans_apdu.insert(trans_apdu.end(), rf_cmd.begin(), rf_cmd.end());
		trans_apdu.push_back(0x00); // Le = 0x00 (PC/SC Part 3 Supplement 2 Section 2.1 / ISO 7816-4 Case 4)

		std::vector<u8> rapdu;
		bool trans_ok = transmit_apdu(trans_apdu, rapdu);
		if (!trans_ok || (rapdu.size() == 2 && rapdu[0] == 0x67 && rapdu[1] == 0x00))
		{
			// Fallback: retry without Le if non-standard driver rejects Case 4 Le with SW 67 00
			trans_apdu.pop_back();
			trans_ok = transmit_apdu(trans_apdu, rapdu);
		}

		if (trans_ok && rapdu.size() >= 4)
		{
			const u16 sw = static_cast<u16>((rapdu[rapdu.size() - 2] << 8) | rapdu.back());
			if (sw == 0x9000)
			{
				rapdu.resize(rapdu.size() - 2);
				// Parse BER-TLV (ISO/IEC 7816-4 Section 5.2.2 / PC/SC Part 3 Supplement 2 Section 2.2):
				// Prioritize Tag 0x97 (PC/SC standard ICC Response Data Object)
				// Fall back to non-status Tag 0x96 (> 2 bytes) only for non-standard vendor implementations
				usz tlv_offset = 0;
				std::vector<u8> cand;
				while (tlv_offset + 2 <= rapdu.size())
				{
					const u8 tag = rapdu[tlv_offset];
					usz header_len = 2;
					usz len = rapdu[tlv_offset + 1];

					if (len == 0x81)
					{
						if (tlv_offset + 3 > rapdu.size())
						{
							break;
						}
						header_len = 3;
						len = rapdu[tlv_offset + 2];
					}
					else if (len == 0x82)
					{
						if (tlv_offset + 4 > rapdu.size())
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

					if (tlv_offset + header_len + len <= rapdu.size() && len >= 2)
					{
						if (tag == 0x97) // Standard ICC Response Data Object
						{
							cand.assign(rapdu.begin() + tlv_offset + header_len, rapdu.begin() + tlv_offset + header_len + len);
							break;
						}
						else if (tag == 0x96 && len > 2 && cand.empty()) // Non-standard vendor payload (excluding 2-byte Response Status)
						{
							cand.assign(rapdu.begin() + tlv_offset + header_len, rapdu.begin() + tlv_offset + header_len + len);
						}
					}
					tlv_offset += header_len + len;
				}

				if (!cand.empty())
				{
					// Validate FeliCa Response Code (fcmd + 1) per JIS X 6319-4 Section 8.1 Table 12
					if (cand.size() >= 2 && cand[0] == cand.size() && cand[1] == static_cast<u8>(fcmd + 1))
					{
						felica_resp = std::move(cand);
						return true;
					}
					else if (cand.size() >= 1 && cand[0] == static_cast<u8>(fcmd + 1))
					{
						felica_resp.clear();
						felica_resp.push_back(static_cast<u8>(cand.size() + 1));
						felica_resp.insert(felica_resp.end(), cand.begin(), cand.end());
						return true;
					}
				}
			}
		}
	}

	// Method 4: Generic PC/SC Direct Transmit APDU (FF 00 00 00 Lc <rf_cmd>)
	// Supported by standard PC/SC contactless readers (e.g. Sony PaSoRi RC-S380, ACS ACR122U)
	if (rf_cmd.size() <= 255)
	{
		std::vector<u8> direct_apdu = {0xFF, 0x00, 0x00, 0x00, static_cast<u8>(rf_cmd.size())};
		direct_apdu.insert(direct_apdu.end(), rf_cmd.begin(), rf_cmd.end());

		std::vector<u8> rapdu;
		if (transmit_apdu(direct_apdu, rapdu) && rapdu.size() >= 2)
		{
			const u16 sw = static_cast<u16>((rapdu[rapdu.size() - 2] << 8) | rapdu.back());
			if (sw == 0x9000 && rapdu.size() >= 4) // At least 2B payload + 2B SW
			{
				rapdu.resize(rapdu.size() - 2);
				// Validate FeliCa Response Code (fcmd + 1) per JIS X 6319-4 Section 8.1 Table 12
				if (rapdu.size() >= 2 && rapdu[0] == rapdu.size() && rapdu[1] == static_cast<u8>(fcmd + 1))
				{
					felica_resp = std::move(rapdu);
					return true;
				}
				else if (rapdu.size() >= 1 && rapdu[0] == static_cast<u8>(fcmd + 1))
				{
					felica_resp.clear();
					felica_resp.push_back(static_cast<u8>(rapdu.size() + 1));
					felica_resp.insert(felica_resp.end(), rapdu.begin(), rapdu.end());
					return true;
				}
			}
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

	const u8 key_type = key_a ? 0x60 : 0x61;

	// Check if reader is Identiv uTrust / CLOUD 370x F / 470x F
	const bool is_utrust = m_impl->is_identiv(this);

	// Step 1: Load Keys APDU (PAPDU_MIFARE_LOAD_KEYS, Section 6.2.3, Page 32-34)
	// PC/SC Part 3: FF 82 <Key Structure> <Key Number> 06 <6 bytes key>
	// P1 = 0x00: Card key, plain transmission, volatile memory
	// P2 = key_slot: On standard PC/SC readers, slot 0x00 is used.
	// On uTrust 370xF, Section 6.2.3 shows P2 = key_type (0x60 for Key A, 0x61 for Key B).
	u8 load_slot = is_utrust ? key_type : 0x00;
	std::vector<u8> load_key_apdu = {0xFF, 0x82, 0x00, load_slot, 0x06};
	load_key_apdu.insert(load_key_apdu.end(), key.begin(), key.end());

	std::vector<u8> rapdu;
	bool load_ok = transmit_apdu(load_key_apdu, rapdu) && rapdu.size() >= 2 &&
	               ((rapdu[rapdu.size() - 2] << 8) | rapdu.back()) == 0x9000;

	if (!load_ok)
	{
		// Try alternate slot: 0x00 if uTrust preferred slot failed, or key_type if 0x00 failed
		load_slot = (load_slot == 0x00) ? key_type : 0x00;
		load_key_apdu[3] = load_slot;
		load_ok = transmit_apdu(load_key_apdu, rapdu) && rapdu.size() >= 2 &&
		          ((rapdu[rapdu.size() - 2] << 8) | rapdu.back()) == 0x9000;
	}

	if (!load_ok)
	{
		return false;
	}

	// Step 2: Authenticate APDU (PAPDU_MIFARE_AUTHENTICATE, Section 6.2.4, Page 34)
	// PC/SC Part 3: FF 86 00 00 05 [B0:Version(01)] [B1:BlockMSB] [B2:BlockLSB] [B3:KeyType] [B4:KeyNumber]
	// PC/SC Part 3 specifies B4 must match the loaded key slot.
	// uTrust manual Section 6.2.4 explicitly specifies B4 shall be set to 0x01.
	const u8 primary_auth_slot = is_utrust ? 0x01 : load_slot;
	std::vector<u8> auth_apdu = {0xFF, 0x86, 0x00, 0x00, 0x05, 0x01, 0x00, block, key_type, primary_auth_slot};
	if (transmit_apdu(auth_apdu, rapdu) && rapdu.size() >= 2)
	{
		const u16 sw = static_cast<u16>((rapdu[rapdu.size() - 2] << 8) | rapdu.back());
		if (sw == 0x9000)
		{
			return true;
		}
	}

	// Fallback 1: Match load_slot
	if (auth_apdu[9] != load_slot)
	{
		auth_apdu[9] = load_slot;
		if (transmit_apdu(auth_apdu, rapdu) && rapdu.size() >= 2)
		{
			const u16 sw = static_cast<u16>((rapdu[rapdu.size() - 2] << 8) | rapdu.back());
			if (sw == 0x9000)
			{
				return true;
			}
		}
	}

	// Fallback 2: Slot 0x01 (uTrust standard slot)
	if (auth_apdu[9] != 0x01)
	{
		auth_apdu[9] = 0x01;
		if (transmit_apdu(auth_apdu, rapdu) && rapdu.size() >= 2)
		{
			const u16 sw = static_cast<u16>((rapdu[rapdu.size() - 2] << 8) | rapdu.back());
			if (sw == 0x9000)
			{
				return true;
			}
		}
	}

	// Fallback 3: key_type (0x60 / 0x61) for readers that index slots by key type
	if (auth_apdu[9] != key_type)
	{
		auth_apdu[9] = key_type;
		if (transmit_apdu(auth_apdu, rapdu) && rapdu.size() >= 2)
		{
			const u16 sw = static_cast<u16>((rapdu[rapdu.size() - 2] << 8) | rapdu.back());
			if (sw == 0x9000)
			{
				return true;
			}
		}
	}

	// Fallback 4: Slot 0x00 (PC/SC Part 3 standard key slot)
	if (auth_apdu[9] != 0x00)
	{
		auth_apdu[9] = 0x00;
		if (transmit_apdu(auth_apdu, rapdu) && rapdu.size() >= 2)
		{
			const u16 sw = static_cast<u16>((rapdu[rapdu.size() - 2] << 8) | rapdu.back());
			if (sw == 0x9000)
			{
				return true;
			}
		}
	}

	// Format B: Obsolete authenticate format FF 88 (not in uTrust manual; legacy reader support)
	std::vector<u8> auth_apdu_b = {0xFF, 0x88, 0x00, block, key_type, load_slot};
	if (transmit_apdu(auth_apdu_b, rapdu) && rapdu.size() >= 2)
	{
		const u16 sw = static_cast<u16>((rapdu[rapdu.size() - 2] << 8) | rapdu.back());
		if (sw == 0x9000)
		{
			return true;
		}
	}

	// Fallback for legacy readers indexing loaded keys at slot 0x00
	if (load_slot != 0x00)
	{
		auth_apdu_b[5] = 0x00;
		if (transmit_apdu(auth_apdu_b, rapdu) && rapdu.size() >= 2)
		{
			const u16 sw = static_cast<u16>((rapdu[rapdu.size() - 2] << 8) | rapdu.back());
			if (sw == 0x9000)
			{
				return true;
			}
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

	// PAPDU_MIFARE_READ_BINARY: FF B0 AddrMSB(00) AddrLSB(block) Le(10)
	// uTrust_370xF_User_Manual.pdf Section 6.2.1 (Page 30)
	std::vector<u8> read_apdu = {0xFF, 0xB0, 0x00, block, 0x10};
	std::vector<u8> rapdu;
	bool read_ok = transmit_apdu(read_apdu, rapdu) && rapdu.size() >= 2 &&
	               ((rapdu[rapdu.size() - 2] << 8) | rapdu.back()) == 0x9000;

	if (!read_ok)
	{
		// Fallback 1 for readers that reject Le=0x10 on 4-byte cards (MIFARE Ultralight/NTAG): try Le=0x04
		read_apdu[4] = 0x04;
		read_ok = transmit_apdu(read_apdu, rapdu) && rapdu.size() >= 2 &&
		          ((rapdu[rapdu.size() - 2] << 8) | rapdu.back()) == 0x9000;
	}

	if (!read_ok)
	{
		// Fallback 2 for readers expecting wildcard/full block Le=0x00 (PC/SC Part 3 Section 3.2.2.1.8)
		read_apdu[4] = 0x00;
		read_ok = transmit_apdu(read_apdu, rapdu) && rapdu.size() >= 2 &&
		          ((rapdu[rapdu.size() - 2] << 8) | rapdu.back()) == 0x9000;
	}

	if (read_ok)
	{
		rapdu.resize(rapdu.size() - 2);
		block_data = std::move(rapdu);

		// uTrust 370xF manual Section 6.2.1 explicitly states:
		// "Regardless of the value given in Le, this command will always return the entire block content:
		//  16 bytes for MIFARE Classic, 4 bytes for MIFARE UL and UL C"
		// The standard MIFARE Ultralight read command (0x30) expects 16 bytes (4 pages of 4 bytes).
		// If only 4 bytes were returned, fetch the subsequent 3 blocks to assemble the full 16-byte buffer.
		if (block_data.size() == 4)
		{
			const u8 successful_le = read_apdu[4];
			for (u8 next_block = 1; next_block < 4; ++next_block)
			{
				if (static_cast<u16>(block) + next_block > 255)
					break;

				const std::vector<u8> next_apdu = {0xFF, 0xB0, 0x00, static_cast<u8>(block + next_block), successful_le};
				std::vector<u8> next_rapdu;
				if (transmit_apdu(next_apdu, next_rapdu) && next_rapdu.size() >= 6)
				{
					const u16 next_sw = (next_rapdu[next_rapdu.size() - 2] << 8) | next_rapdu.back();
					if (next_sw == 0x9000)
					{
						block_data.insert(block_data.end(), next_rapdu.begin(), next_rapdu.begin() + 4);
						continue;
					}
				}
				break;
			}

			// If reading near card boundary yields less than 16 bytes, pad with zeroes to satisfy 16B Mifare frame
			if (block_data.size() < 16)
			{
				block_data.resize(16, 0x00);
			}
		}

		return true;
	}

	return false;
}

bool pcsc_driver::mifare_write(u8 block, std::span<const u8> block_data)
{
	if (!is_open() || block_data.empty() || block_data.size() > 255)
	{
		return false;
	}

	// PAPDU_MIFARE_UPDATE_BINARY: FF D6 AddrMSB(00) AddrLSB(block) Lc <data>
	// uTrust_370xF_User_Manual.pdf Section 6.2.2 (Page 31)
	std::vector<u8> write_apdu = {0xFF, 0xD6, 0x00, block, static_cast<u8>(block_data.size())};
	write_apdu.insert(write_apdu.end(), block_data.begin(), block_data.end());

	std::vector<u8> rapdu;
	if (transmit_apdu(write_apdu, rapdu) && rapdu.size() >= 2)
	{
		const u16 sw = (rapdu[rapdu.size() - 2] << 8) | rapdu.back();
		if (sw == 0x9000)
		{
			return true;
		}
	}

	// Per uTrust Section 6.2.2 and NXP MF0ICU1 Section 8.5.3 (COMPATIBILITY WRITE):
	// MIFARE Ultralight blocks (pages) are 4 bytes. If a 16-byte write was rejected by the
	// reader firmware with SW 67 00 (Length incorrect), fall back to writing only the first
	// 4 bytes to the addressed block (the remaining 12 bytes are discarded per MF0ICU1 Section 8.5.3).
	// Never write across subsequent blocks to prevent overwriting Lock bytes or OTP.
	if (block_data.size() == 16)
	{
		std::vector<u8> page_apdu = {0xFF, 0xD6, 0x00, block, 0x04};
		page_apdu.insert(page_apdu.end(), block_data.begin(), block_data.begin() + 4);
		std::vector<u8> page_rapdu;
		if (transmit_apdu(page_apdu, page_rapdu) && page_rapdu.size() >= 2)
		{
			const u16 page_sw = (page_rapdu[page_rapdu.size() - 2] << 8) | page_rapdu.back();
			if (page_sw == 0x9000)
			{
				return true;
			}
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

	// PAPDU_ISO14443_PART3_PASS_THRU: FF EF DiscardCRC(01) 00 Lc <Data>
	// uTrust_370xF_User_Manual.pdf Section 6.2.11 (Page 40)
	// P1 = 0x01 instructs reader to discard CRC bytes from response (standard PN532 InDataExchange payload)
	std::vector<u8> apdu = {0xFF, 0xEF, 0x01, 0x00, static_cast<u8>(data.size())};
	apdu.insert(apdu.end(), data.begin(), data.end());

	std::vector<u8> rapdu;
	if (transmit_apdu(apdu, rapdu) && rapdu.size() >= 2)
	{
		const u16 sw = static_cast<u16>((rapdu[rapdu.size() - 2] << 8) | rapdu.back());
		if (sw == 0x9000)
		{
			rapdu.resize(rapdu.size() - 2);
			response = std::move(rapdu);
			return true;
		}
	}

	// Fallback with P1 = 0x00 (Transmit CRC) if P1 = 0x01 is rejected by firmware
	apdu[2] = 0x00;
	if (transmit_apdu(apdu, rapdu) && rapdu.size() >= 2)
	{
		const u16 sw = static_cast<u16>((rapdu[rapdu.size() - 2] << 8) | rapdu.back());
		if (sw == 0x9000)
		{
			rapdu.resize(rapdu.size() - 2);
			// Strip 2 CRC bytes if present so caller receives clean payload
			if (rapdu.size() >= 2)
			{
				rapdu.resize(rapdu.size() - 2);
			}
			response = std::move(rapdu);
			return true;
		}
	}

	// Fallback 2: PC/SC Part 3 Supplement 2 Section 2.1 Transparent Exchange (FF C2 00 01)
	if (data.size() <= 252)
	{
		std::vector<u8> trans_apdu;
		if (data.size() <= 127)
		{
			trans_apdu = {0xFF, 0xC2, 0x00, 0x01, static_cast<u8>(data.size() + 2), 0x95, static_cast<u8>(data.size())};
		}
		else
		{
			trans_apdu = {0xFF, 0xC2, 0x00, 0x01, static_cast<u8>(data.size() + 3), 0x95, 0x81, static_cast<u8>(data.size())};
		}
		trans_apdu.insert(trans_apdu.end(), data.begin(), data.end());
		trans_apdu.push_back(0x00); // Le = 0x00 (PC/SC Part 3 Supplement 2 Section 2.1 / ISO 7816-4 Case 4)

		bool trans_ok = transmit_apdu(trans_apdu, rapdu);
		if (!trans_ok || (rapdu.size() == 2 && rapdu[0] == 0x67 && rapdu[1] == 0x00))
		{
			trans_apdu.pop_back();
			trans_ok = transmit_apdu(trans_apdu, rapdu);
		}

		if (trans_ok && rapdu.size() >= 4)
		{
			const u16 sw = static_cast<u16>((rapdu[rapdu.size() - 2] << 8) | rapdu.back());
			if (sw == 0x9000)
			{
				rapdu.resize(rapdu.size() - 2);
				usz tlv_offset = 0;
				while (tlv_offset + 2 <= rapdu.size())
				{
					const u8 tag = rapdu[tlv_offset];
					usz header_len = 2;
					usz len = rapdu[tlv_offset + 1];
					if (len == 0x81)
					{
						if (tlv_offset + 3 > rapdu.size())
						{
							break;
						}
						header_len = 3;
						len = rapdu[tlv_offset + 2];
					}
					if (tlv_offset + header_len + len <= rapdu.size() && tag == 0x97 && len > 0)
					{
						response.assign(rapdu.begin() + tlv_offset + header_len, rapdu.begin() + tlv_offset + header_len + len);
						return true;
					}
					tlv_offset += header_len + len;
				}
			}
		}
	}

	// Fallback 3 for generic PC/SC readers: Direct Transmit APDU (FF 00 00 00 Lc <Data>)
	if (data.size() <= 255)
	{
		std::vector<u8> direct_apdu = {0xFF, 0x00, 0x00, 0x00, static_cast<u8>(data.size())};
		direct_apdu.insert(direct_apdu.end(), data.begin(), data.end());
		if (transmit_apdu(direct_apdu, rapdu) && rapdu.size() >= 2)
		{
			const u16 sw = static_cast<u16>((rapdu[rapdu.size() - 2] << 8) | rapdu.back());
			if (sw == 0x9000)
			{
				rapdu.resize(rapdu.size() - 2);
				response = std::move(rapdu);
				return true;
			}
		}
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

	const bool is_utrust = m_impl->is_identiv(this);

	if (is_utrust)
	{
		// Method 1 for Identiv: PAPDU_TCL_PASS_THRU (FF FE 00 00 Lc <Data>)
		// uTrust_370xF_User_Manual.pdf Section 6.2.10 (Page 39)
		// Note: "The card itself will send SW1, SW2."
		// Lc (P3) is 1 byte, applicable when data payload <= 255 bytes
		if (data.size() <= 255)
		{
			std::vector<u8> apdu = {0xFF, 0xFE, 0x00, 0x00, static_cast<u8>(data.size())};
			apdu.insert(apdu.end(), data.begin(), data.end());

			std::vector<u8> rapdu;
			if (transmit_apdu(apdu, rapdu) && rapdu.size() >= 2)
			{
				const u16 sw = static_cast<u16>((rapdu[rapdu.size() - 2] << 8) | rapdu.back());
				// Filter reader rejection status words (uTrust Manual Section 7.1) so unsupported FF FE falls back to direct T=1
				if (sw != 0x6D00 && sw != 0x6E00 && sw != 0x6A81 && sw != 0x6F00 && sw != 0x6800 && sw != 0x6B00)
				{
					response = std::move(rapdu);
					return true;
				}
			}
		}

		// Fallback for Identiv in direct T=1 connection (or standard short/extended APDUs > 255 bytes): direct APDU pass-through without wrapper
		std::vector<u8> rapdu;
		if (transmit_apdu(data, rapdu) && rapdu.size() >= 2)
		{
			response = std::move(rapdu);
			return true;
		}
	}
	else
	{
		// Method 1 for generic PC/SC readers: Standard PC/SC Part 7 direct APDU pass-through without wrapper
		std::vector<u8> rapdu;
		if (transmit_apdu(data, rapdu) && rapdu.size() >= 2)
		{
			response = std::move(rapdu);
			return true;
		}

		// Fallback: try PAPDU_TCL_PASS_THRU in case of OEM firmware derivative
		if (data.size() <= 255)
		{
			std::vector<u8> apdu = {0xFF, 0xFE, 0x00, 0x00, static_cast<u8>(data.size())};
			apdu.insert(apdu.end(), data.begin(), data.end());
			if (transmit_apdu(apdu, rapdu) && rapdu.size() >= 2)
			{
				const u16 sw = static_cast<u16>((rapdu[rapdu.size() - 2] << 8) | rapdu.back());
				// Filter reader rejection status words (uTrust Section 7.1)
				if (sw != 0x6D00 && sw != 0x6E00 && sw != 0x6A81 && sw != 0x6F00 && sw != 0x6800 && sw != 0x6B00)
				{
					response = std::move(rapdu);
					return true;
				}
			}
		}
	}

	return false;
}

bool pcsc_driver::part4_switch(bool to_tcl)
{
	if (!is_open())
	{
		return false;
	}

	// PAPDU_ISO14443_PART4_PART3_SWITCH: FF F8 P1 00 00
	// uTrust_370xF_User_Manual.pdf Section 6.2.12 (Page 40)
	// P1 = 0x00: MIFARE mode to TCL mode; P1 = 0x01: TCL mode to MIFARE mode
	const std::vector<u8> apdu = {0xFF, 0xF8, static_cast<u8>(to_tcl ? 0x00 : 0x01), 0x00, 0x00};
	std::vector<u8> rapdu;
	if (transmit_apdu(apdu, rapdu) && rapdu.size() >= 2)
	{
		const u16 sw = static_cast<u16>((rapdu[rapdu.size() - 2] << 8) | rapdu.back());
		return sw == 0x9000;
	}
	return false;
}
