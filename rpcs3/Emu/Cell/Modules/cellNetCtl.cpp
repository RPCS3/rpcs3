#include "stdafx.h"
#include "Emu/system_config.h"
#include "Emu/Cell/PPUModule.h"
#include "Emu/IdManager.h"

#include "cellGame.h"
#include "cellSysutil.h"
#include "cellNetCtl.h"

#include "Utilities/StrUtil.h"

#include "Emu/NP/np_handler.h"
#include "Emu/NP/np_helpers.h"

LOG_CHANNEL(cellNetCtl);

template <>
void fmt_class_string<CellNetCtlError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_NET_CTL_ERROR_NOT_INITIALIZED);
			STR_CASE(CELL_NET_CTL_ERROR_NOT_TERMINATED);
			STR_CASE(CELL_NET_CTL_ERROR_HANDLER_MAX);
			STR_CASE(CELL_NET_CTL_ERROR_ID_NOT_FOUND);
			STR_CASE(CELL_NET_CTL_ERROR_INVALID_ID);
			STR_CASE(CELL_NET_CTL_ERROR_INVALID_CODE);
			STR_CASE(CELL_NET_CTL_ERROR_INVALID_ADDR);
			STR_CASE(CELL_NET_CTL_ERROR_NOT_CONNECTED);
			STR_CASE(CELL_NET_CTL_ERROR_NOT_AVAIL);
			STR_CASE(CELL_NET_CTL_ERROR_INVALID_TYPE);
			STR_CASE(CELL_NET_CTL_ERROR_INVALID_SIZE);
			STR_CASE(CELL_NET_CTL_ERROR_NET_DISABLED);
			STR_CASE(CELL_NET_CTL_ERROR_NET_NOT_CONNECTED);
			STR_CASE(CELL_NET_CTL_ERROR_NP_NO_ACCOUNT);
			STR_CASE(CELL_NET_CTL_ERROR_NP_RESERVED1);
			STR_CASE(CELL_NET_CTL_ERROR_NP_RESERVED2);
			STR_CASE(CELL_NET_CTL_ERROR_NET_CABLE_NOT_CONNECTED);
			STR_CASE(CELL_NET_CTL_ERROR_DIALOG_CANCELED);
			STR_CASE(CELL_NET_CTL_ERROR_DIALOG_ABORTED);

			STR_CASE(CELL_NET_CTL_ERROR_WLAN_DEAUTHED);
			STR_CASE(CELL_NET_CTL_ERROR_WLAN_KEYINFO_EXCHNAGE_TIMEOUT);
			STR_CASE(CELL_NET_CTL_ERROR_WLAN_ASSOC_FAILED);
			STR_CASE(CELL_NET_CTL_ERROR_WLAN_AP_DISAPPEARED);
			STR_CASE(CELL_NET_CTL_ERROR_PPPOE_SESSION_INIT);
			STR_CASE(CELL_NET_CTL_ERROR_PPPOE_SESSION_NO_PADO);
			STR_CASE(CELL_NET_CTL_ERROR_PPPOE_SESSION_NO_PADS);
			STR_CASE(CELL_NET_CTL_ERROR_PPPOE_SESSION_GET_PADT);
			STR_CASE(CELL_NET_CTL_ERROR_PPPOE_SESSION_SERVICE_NAME);
			STR_CASE(CELL_NET_CTL_ERROR_PPPOE_SESSION_AC_SYSTEM);
			STR_CASE(CELL_NET_CTL_ERROR_PPPOE_SESSION_GENERIC);
			STR_CASE(CELL_NET_CTL_ERROR_PPPOE_STATUS_AUTH);
			STR_CASE(CELL_NET_CTL_ERROR_PPPOE_STATUS_NETWORK);
			STR_CASE(CELL_NET_CTL_ERROR_PPPOE_STATUS_TERMINATE);
			STR_CASE(CELL_NET_CTL_ERROR_DHCP_LEASE_TIME);

			STR_CASE(CELL_GAMEUPDATE_ERROR_NOT_INITIALIZED);
			STR_CASE(CELL_GAMEUPDATE_ERROR_ALREADY_INITIALIZED);
			STR_CASE(CELL_GAMEUPDATE_ERROR_INVALID_ADDR);
			STR_CASE(CELL_GAMEUPDATE_ERROR_INVALID_SIZE);
			STR_CASE(CELL_GAMEUPDATE_ERROR_INVALID_MEMORY_CONTAINER);
			STR_CASE(CELL_GAMEUPDATE_ERROR_INSUFFICIENT_MEMORY_CONTAINER);
			STR_CASE(CELL_GAMEUPDATE_ERROR_BUSY);
			STR_CASE(CELL_GAMEUPDATE_ERROR_NOT_START);
			STR_CASE(CELL_GAMEUPDATE_ERROR_LOAD_FAILED);
		}

		return unknown;
	});
}

template <>
void fmt_class_string<CellNetCtlState>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](CellNetCtlState value)
	{
		switch (value)
		{
		case CELL_NET_CTL_STATE_Disconnected: return "Disconnected";
		case CELL_NET_CTL_STATE_Connecting: return "Connecting";
		case CELL_NET_CTL_STATE_IPObtaining: return "Obtaining IP";
		case CELL_NET_CTL_STATE_IPObtained: return "IP Obtained";
		}

		return unknown;
	});
}

struct CellGameUpdateResult
{
	be_t<s32> status; // CellGameUpdateResultStatus
	be_t<s32> error_code;
	char app_ver[CELL_GAME_SYSP_APP_VER_SIZE];
	char padding[2];
};

struct CellGameUpdateParam
{
	be_t<u32> size;
	be_t<u32> cid;
};

using CellGameUpdateCallback = void(s32 status, s32 error_code, vm::ptr<void> userdata);
using CellGameUpdateCallbackEx = void(vm::ptr<CellGameUpdateResult> result, vm::ptr<void> userdata);

error_code cellNetCtlInit()
{
	cellNetCtl.warning("cellNetCtlInit()");

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (nph.is_netctl_init)
	{
		return CELL_NET_CTL_ERROR_NOT_TERMINATED;
	}

	nph.is_netctl_init = true;

	return CELL_OK;
}

void cellNetCtlTerm()
{
	cellNetCtl.warning("cellNetCtlTerm()");

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	nph.is_netctl_init = false;
}

error_code cellNetCtlGetState(vm::ptr<s32> state)
{
	cellNetCtl.trace("cellNetCtlGetState(state=*0x%x)", state);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_netctl_init)
	{
		return CELL_NET_CTL_ERROR_NOT_INITIALIZED;
	}

	if (!state)
	{
		return CELL_NET_CTL_ERROR_INVALID_ADDR;
	}

	*state = nph.get_net_status();

	return CELL_OK;
}

error_code cellNetCtlAddHandler(vm::ptr<cellNetCtlHandler> handler, vm::ptr<void> arg, vm::ptr<s32> hid)
{
	cellNetCtl.todo("cellNetCtlAddHandler(handler=*0x%x, arg=*0x%x, hid=*0x%x)", handler, arg, hid);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_netctl_init)
	{
		return CELL_NET_CTL_ERROR_NOT_INITIALIZED;
	}

	if (!hid)
	{
		return CELL_NET_CTL_ERROR_INVALID_ADDR;
	}

	return CELL_OK;
}

error_code cellNetCtlDelHandler(s32 hid)
{
	cellNetCtl.todo("cellNetCtlDelHandler(hid=0x%x)", hid);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_netctl_init)
	{
		return CELL_NET_CTL_ERROR_NOT_INITIALIZED;
	}

	if (hid > 3)
	{
		return CELL_NET_CTL_ERROR_INVALID_ID;
	}

	return CELL_OK;
}

error_code cellNetCtlGetInfo(s32 code, vm::ptr<CellNetCtlInfo> info)
{
	cellNetCtl.warning("cellNetCtlGetInfo(code=0x%x (%s), info=*0x%x)", code, InfoCodeToName(code), info);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_netctl_init)
	{
		return CELL_NET_CTL_ERROR_NOT_INITIALIZED;
	}

	if (!info)
	{
		return CELL_NET_CTL_ERROR_INVALID_ADDR;
	}

	if (code == CELL_NET_CTL_INFO_ETHER_ADDR)
	{
		memcpy(info->ether_addr.data, nph.get_ether_addr().data(), 6);
		return CELL_OK;
	}

	if (nph.get_net_status() == CELL_NET_CTL_STATE_Disconnected)
	{
		return CELL_NET_CTL_ERROR_NOT_CONNECTED;
	}

	switch (code)
	{
	case CELL_NET_CTL_INFO_DEVICE: info->device = CELL_NET_CTL_DEVICE_WIRED; break;
	case CELL_NET_CTL_INFO_MTU: info->mtu = 1500; break;
	case CELL_NET_CTL_INFO_LINK: info->link = CELL_NET_CTL_LINK_CONNECTED; break;
	case CELL_NET_CTL_INFO_LINK_TYPE: info->link_type = CELL_NET_CTL_LINK_TYPE_100BASE_FULL; break;
	// case CELL_NET_CTL_INFO_BSSID: break;
	// case CELL_NET_CTL_INFO_SSID: break;
	// case CELL_NET_CTL_INFO_WLAN_SECURITY: break;
	// case CELL_NET_CTL_INFO_8021X_TYPE: break;
	// case CELL_NET_CTL_INFO_8021X_AUTH_NAME: break;
	case CELL_NET_CTL_INFO_RSSI: info->rssi = 100; break; // wireless: value ranges from 0-100 indicating wireless connection strength
	case CELL_NET_CTL_INFO_CHANNEL: info->channel = 1; break; // wireless: channel used to connect to the AP?
	case CELL_NET_CTL_INFO_IP_CONFIG: info->ip_config = CELL_NET_CTL_IP_STATIC; break;
	case CELL_NET_CTL_INFO_DHCP_HOSTNAME: strcpy_trunc(info->dhcp_hostname, nph.get_hostname()); break;
	// case CELL_NET_CTL_INFO_PPPOE_AUTH_NAME: break;
	case CELL_NET_CTL_INFO_IP_ADDRESS: strcpy_trunc(info->ip_address, np::ip_to_string(nph.get_local_ip_addr())); break; // verified on HW
	case CELL_NET_CTL_INFO_NETMASK: strcpy_trunc(info->netmask, "255.255.255.0"); break;
	case CELL_NET_CTL_INFO_DEFAULT_ROUTE: strcpy_trunc(info->default_route, "192.168.1.1"); break;
	case CELL_NET_CTL_INFO_PRIMARY_DNS: strcpy_trunc(info->primary_dns, np::ip_to_string(nph.get_dns_ip())); break;
	case CELL_NET_CTL_INFO_SECONDARY_DNS: strcpy_trunc(info->secondary_dns, np::ip_to_string(nph.get_dns_ip())); break;
	case CELL_NET_CTL_INFO_HTTP_PROXY_CONFIG: info->http_proxy_config = 0; break;
	// case CELL_NET_CTL_INFO_HTTP_PROXY_SERVER: break;
	// case CELL_NET_CTL_INFO_HTTP_PROXY_PORT: break;
	case CELL_NET_CTL_INFO_UPNP_CONFIG: info->upnp_config = (nph.get_upnp_status() == SCE_NP_SIGNALING_NETINFO_UPNP_STATUS_VALID) ? CELL_NET_CTL_UPNP_ON : CELL_NET_CTL_UPNP_OFF; break;
	// case CELL_NET_CTL_INFO_RESERVED1: break;
	// case CELL_NET_CTL_INFO_RESERVED2: break;
	default: cellNetCtl.error("Unsupported request: %s", InfoCodeToName(code)); break;
	}

	return CELL_OK;
}

struct netstart_hack
{
	static constexpr std::string_view thread_name = "NetStart Hack";

	void operator()(int) const
	{
		thread_ctrl::wait_for(500'000);

		sysutil_send_system_cmd(CELL_SYSUTIL_NET_CTL_NETSTART_LOADED, 0);
		sysutil_send_system_cmd(CELL_SYSUTIL_NET_CTL_NETSTART_FINISHED, 0);
	}
};

error_code cellNetCtlNetStartDialogLoadAsync(vm::cptr<CellNetCtlNetStartDialogParam> param)
{
	cellNetCtl.warning("cellNetCtlNetStartDialogLoadAsync(param=*0x%x)", param);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_netctl_init)
	{
		return CELL_NET_CTL_ERROR_NOT_INITIALIZED;
	}

	if (!param)
	{
		return CELL_NET_CTL_ERROR_INVALID_ADDR;
	}

	if (param->type >= CELL_NET_CTL_NETSTART_TYPE_MAX)
	{
		return CELL_NET_CTL_ERROR_INVALID_TYPE;
	}

	if (param->size != 12u)
	{
		return CELL_NET_CTL_ERROR_INVALID_SIZE;
	}

	// This is a hack for Diva F 2nd that registers the sysutil callback after calling this function.
	g_fxo->get<named_thread<netstart_hack>>()(0);

	return CELL_OK;
}

error_code cellNetCtlNetStartDialogAbortAsync()
{
	cellNetCtl.error("cellNetCtlNetStartDialogAbortAsync()");

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_netctl_init)
	{
		return CELL_NET_CTL_ERROR_NOT_INITIALIZED;
	}

	return CELL_OK;
}

error_code cellNetCtlNetStartDialogUnloadAsync(vm::ptr<CellNetCtlNetStartDialogResult> result)
{
	cellNetCtl.warning("cellNetCtlNetStartDialogUnloadAsync(result=*0x%x)", result);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_netctl_init)
	{
		return CELL_NET_CTL_ERROR_NOT_INITIALIZED;
	}

	if (!result)
	{
		return CELL_NET_CTL_ERROR_INVALID_ADDR;
	}

	if (result->size != 8u)
	{
		return CELL_NET_CTL_ERROR_INVALID_SIZE;
	}

	result->result = nph.get_net_status() == CELL_NET_CTL_STATE_IPObtained ? 0 : CELL_NET_CTL_ERROR_DIALOG_CANCELED;

	sysutil_send_system_cmd(CELL_SYSUTIL_NET_CTL_NETSTART_UNLOADED, 0);
	return CELL_OK;
}

error_code cellNetCtlGetNatInfo(vm::ptr<CellNetCtlNatInfo> natInfo)
{
	cellNetCtl.warning("cellNetCtlGetNatInfo(natInfo=*0x%x)", natInfo);

	auto& nph = g_fxo->get<named_thread<np::np_handler>>();

	if (!nph.is_netctl_init)
	{
		return CELL_NET_CTL_ERROR_NOT_INITIALIZED;
	}

	if (!natInfo)
	{
		return CELL_NET_CTL_ERROR_INVALID_ADDR;
	}

	if (natInfo->size != 16u && natInfo->size != 20u)
	{
		return CELL_NET_CTL_ERROR_INVALID_SIZE;
	}

	natInfo->nat_type = CELL_NET_CTL_NATINFO_NAT_TYPE_2;
	natInfo->stun_status = CELL_NET_CTL_NATINFO_STUN_OK;
	natInfo->upnp_status = nph.get_upnp_status();

	return CELL_OK;
}

error_code cellNetCtlAddHandlerGameInt()
{
	cellNetCtl.todo("cellNetCtlAddHandlerGameInt()");
	return CELL_OK;
}

error_code cellNetCtlConnectGameInt()
{
	cellNetCtl.todo("cellNetCtlConnectGameInt()");
	return CELL_OK;
}

error_code cellNetCtlDelHandlerGameInt()
{
	cellNetCtl.todo("cellNetCtlDelHandlerGameInt()");
	return CELL_OK;
}

error_code cellNetCtlDisconnectGameInt()
{
	cellNetCtl.todo("cellNetCtlDisconnectGameInt()");
	return CELL_OK;
}

error_code cellNetCtlGetInfoGameInt()
{
	cellNetCtl.todo("cellNetCtlGetInfoGameInt()");
	return CELL_OK;
}

error_code cellNetCtlGetScanInfoGameInt()
{
	cellNetCtl.todo("cellNetCtlGetScanInfoGameInt()");
	return CELL_OK;
}

error_code cellNetCtlGetStateGameInt()
{
	cellNetCtl.todo("cellNetCtlGetStateGameInt()");
	return CELL_OK;
}

error_code cellNetCtlScanGameInt()
{
	cellNetCtl.todo("cellNetCtlScanGameInt()");
	return CELL_OK;
}

error_code cellGameUpdateInit()
{
	cellNetCtl.todo("cellGameUpdateInit()");
	return CELL_OK;
}

error_code cellGameUpdateTerm()
{
	cellNetCtl.todo("cellGameUpdateTerm()");
	return CELL_OK;
}

error_code cellGameUpdateCheckStartAsync(vm::cptr<CellGameUpdateParam> param, vm::ptr<CellGameUpdateCallback> cb_func, vm::ptr<void> userdata)
{
	cellNetCtl.todo("cellGameUpdateCheckStartAsync(param=*0x%x, cb_func=*0x%x, userdata=*0x%x)", param, cb_func, userdata);
	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		cb_func(ppu, CELL_GAMEUPDATE_RESULT_STATUS_NO_UPDATE, CELL_OK, userdata);
		return CELL_OK;
	});
	return CELL_OK;
}

error_code cellGameUpdateCheckFinishAsync(vm::ptr<CellGameUpdateCallback> cb_func, vm::ptr<void> userdata)
{
	cellNetCtl.todo("cellGameUpdateCheckFinishAsync(cb_func=*0x%x, userdata=*0x%x)", cb_func, userdata);
	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		cb_func(ppu, CELL_GAMEUPDATE_RESULT_STATUS_FINISHED, CELL_OK, userdata);
		return CELL_OK;
	});
	return CELL_OK;
}

error_code cellGameUpdateCheckStartWithoutDialogAsync(vm::ptr<CellGameUpdateCallback> cb_func, vm::ptr<void> userdata)
{
	cellNetCtl.todo("cellGameUpdateCheckStartWithoutDialogAsync(cb_func=*0x%x, userdata=*0x%x)", cb_func, userdata);
	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		cb_func(ppu, CELL_GAMEUPDATE_RESULT_STATUS_NO_UPDATE, CELL_OK, userdata);
		return CELL_OK;
	});
	return CELL_OK;
}

error_code cellGameUpdateCheckAbort()
{
	cellNetCtl.todo("cellGameUpdateCheckAbort()");
	return CELL_OK;
}

error_code cellGameUpdateCheckStartAsyncEx(vm::cptr<CellGameUpdateParam> param, vm::ptr<CellGameUpdateCallbackEx> cb_func, vm::ptr<void> userdata)
{
	cellNetCtl.todo("cellGameUpdateCheckStartAsyncEx(param=*0x%x, cb_func=*0x%x, userdata=*0x%x)", param, cb_func, userdata);
	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		cb_func(ppu, vm::make_var(CellGameUpdateResult{CELL_GAMEUPDATE_RESULT_STATUS_NO_UPDATE, CELL_OK}), userdata);
		return CELL_OK;
	});
	return CELL_OK;
}

error_code cellGameUpdateCheckFinishAsyncEx(vm::ptr<CellGameUpdateCallbackEx> cb_func, vm::ptr<void> userdata)
{
	cellNetCtl.todo("cellGameUpdateCheckFinishAsyncEx(cb_func=*0x%x, userdata=*0x%x)", cb_func, userdata);
	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		cb_func(ppu, vm::make_var(CellGameUpdateResult{CELL_GAMEUPDATE_RESULT_STATUS_FINISHED, CELL_OK}), userdata);
		return CELL_OK;
	});
	return CELL_OK;
}

error_code cellGameUpdateCheckStartWithoutDialogAsyncEx(vm::ptr<CellGameUpdateCallbackEx> cb_func, vm::ptr<void> userdata)
{
	cellNetCtl.todo("cellGameUpdateCheckStartWithoutDialogAsyncEx(cb_func=*0x%x, userdata=*0x%x)", cb_func, userdata);
	sysutil_register_cb([=](ppu_thread& ppu) -> s32
	{
		cb_func(ppu, vm::make_var(CellGameUpdateResult{CELL_GAMEUPDATE_RESULT_STATUS_NO_UPDATE, CELL_OK}), userdata);
		return CELL_OK;
	});
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellNetCtl)("cellNetCtl", []()
{
	REG_FUNC(cellNetCtl, cellNetCtlInit);
	REG_FUNC(cellNetCtl, cellNetCtlTerm);

	REG_FUNC(cellNetCtl, cellNetCtlGetState);
	REG_FUNC(cellNetCtl, cellNetCtlAddHandler);
	REG_FUNC(cellNetCtl, cellNetCtlDelHandler);

	REG_FUNC(cellNetCtl, cellNetCtlGetInfo);

	REG_FUNC(cellNetCtl, cellNetCtlNetStartDialogLoadAsync);
	REG_FUNC(cellNetCtl, cellNetCtlNetStartDialogAbortAsync);
	REG_FUNC(cellNetCtl, cellNetCtlNetStartDialogUnloadAsync);

	REG_FUNC(cellNetCtl, cellNetCtlGetNatInfo);

	REG_FUNC(cellNetCtl, cellNetCtlAddHandlerGameInt);
	REG_FUNC(cellNetCtl, cellNetCtlConnectGameInt);
	REG_FUNC(cellNetCtl, cellNetCtlDelHandlerGameInt);
	REG_FUNC(cellNetCtl, cellNetCtlDisconnectGameInt);
	REG_FUNC(cellNetCtl, cellNetCtlGetInfoGameInt);
	REG_FUNC(cellNetCtl, cellNetCtlGetScanInfoGameInt);
	REG_FUNC(cellNetCtl, cellNetCtlGetStateGameInt);
	REG_FUNC(cellNetCtl, cellNetCtlScanGameInt);

	REG_FUNC(cellNetCtl, cellGameUpdateInit);
	REG_FUNC(cellNetCtl, cellGameUpdateTerm);

	REG_FUNC(cellNetCtl, cellGameUpdateCheckStartAsync);
	REG_FUNC(cellNetCtl, cellGameUpdateCheckFinishAsync);
	REG_FUNC(cellNetCtl, cellGameUpdateCheckStartWithoutDialogAsync);
	REG_FUNC(cellNetCtl, cellGameUpdateCheckAbort);
	REG_FUNC(cellNetCtl, cellGameUpdateCheckStartAsyncEx);
	REG_FUNC(cellNetCtl, cellGameUpdateCheckFinishAsyncEx);
	REG_FUNC(cellNetCtl, cellGameUpdateCheckStartWithoutDialogAsyncEx);
});
