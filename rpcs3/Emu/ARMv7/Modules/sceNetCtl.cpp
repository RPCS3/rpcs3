#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceNet.h"

extern psv_log_base sceNetCtl;

union SceNetCtlInfo
{
	char cnf_name[65];
	u32 device;
	SceNetEtherAddr ether_addr;
	u32 mtu;
	u32 link;
	SceNetEtherAddr bssid;
	char ssid[33];
	u32 wifi_security;
	u32 rssi_dbm;
	u32 rssi_percentage;
	u32 channel;
	u32 ip_config;
	char dhcp_hostname[256];
	char pppoe_auth_name[128];
	char ip_address[16];
	char netmask[16];
	char default_route[16];
	char primary_dns[16];
	char secondary_dns[16];
	u32 http_proxy_config;
	char http_proxy_server[256];
	u32 http_proxy_port;
};

struct SceNetCtlNatInfo
{
	u32 size;
	s32 stun_status;
	s32 nat_type;
	SceNetInAddr mapped_addr;
};

struct SceNetCtlAdhocPeerInfo
{
	vm::psv::ptr<SceNetCtlAdhocPeerInfo> next;
	SceNetInAddr inet_addr;
};

typedef vm::psv::ptr<void(s32 event_type, vm::psv::ptr<void> arg)> SceNetCtlCallback;

s32 sceNetCtlInit()
{
	throw __FUNCTION__;
}

void sceNetCtlTerm()
{
	throw __FUNCTION__;
}

s32 sceNetCtlCheckCallback()
{
	throw __FUNCTION__;
}

s32 sceNetCtlInetGetResult(s32 eventType, vm::psv::ptr<s32> errorCode)
{
	throw __FUNCTION__;
}

s32 sceNetCtlAdhocGetResult(s32 eventType, vm::psv::ptr<s32> errorCode)
{
	throw __FUNCTION__;
}

s32 sceNetCtlInetGetInfo(s32 code, vm::psv::ptr<SceNetCtlInfo> info)
{
	throw __FUNCTION__;
}

s32 sceNetCtlInetGetState(vm::psv::ptr<s32> state)
{
	throw __FUNCTION__;
}

s32 sceNetCtlGetNatInfo(vm::psv::ptr<SceNetCtlNatInfo> natinfo)
{
	throw __FUNCTION__;
}

s32 sceNetCtlInetRegisterCallback(SceNetCtlCallback func, vm::psv::ptr<void> arg, vm::psv::ptr<s32> cid)
{
	throw __FUNCTION__;
}

s32 sceNetCtlInetUnregisterCallback(s32 cid)
{
	throw __FUNCTION__;
}

s32 sceNetCtlAdhocRegisterCallback(SceNetCtlCallback func, vm::psv::ptr<void> arg, vm::psv::ptr<s32> cid)
{
	throw __FUNCTION__;
}

s32 sceNetCtlAdhocUnregisterCallback(s32 cid)
{
	throw __FUNCTION__;
}

s32 sceNetCtlAdhocGetState(vm::psv::ptr<s32> state)
{
	throw __FUNCTION__;
}

s32 sceNetCtlAdhocDisconnect()
{
	throw __FUNCTION__;
}

s32 sceNetCtlAdhocGetPeerList(vm::psv::ptr<u32> buflen, vm::psv::ptr<void> buf)
{
	throw __FUNCTION__;
}

s32 sceNetCtlAdhocGetInAddr(vm::psv::ptr<SceNetInAddr> inaddr)
{
	throw __FUNCTION__;
}

#define REG_FUNC(nid, name) reg_psv_func<name>(nid, &sceNetCtl, #name, name)

psv_log_base sceNetCtl("SceNetCtl", []()
{
	sceNetCtl.on_load = nullptr;
	sceNetCtl.on_unload = nullptr;
	sceNetCtl.on_stop = nullptr;

	REG_FUNC(0x495CA1DB, sceNetCtlInit);
	REG_FUNC(0xCD188648, sceNetCtlTerm);
	REG_FUNC(0xDFFC3ED4, sceNetCtlCheckCallback);
	REG_FUNC(0x6B20EC02, sceNetCtlInetGetResult);
	REG_FUNC(0x7AE0ED19, sceNetCtlAdhocGetResult);
	REG_FUNC(0xB26D07F3, sceNetCtlInetGetInfo);
	REG_FUNC(0x6D26AC68, sceNetCtlInetGetState);
	REG_FUNC(0xEAEE6185, sceNetCtlInetRegisterCallback);
	REG_FUNC(0xD0C3BF3F, sceNetCtlInetUnregisterCallback);
	REG_FUNC(0x4DDD6149, sceNetCtlGetNatInfo);
	REG_FUNC(0x0961A561, sceNetCtlAdhocGetState);
	REG_FUNC(0xFFA9D594, sceNetCtlAdhocRegisterCallback);
	REG_FUNC(0xA4471E10, sceNetCtlAdhocUnregisterCallback);
	REG_FUNC(0xED43B79A, sceNetCtlAdhocDisconnect);
	REG_FUNC(0x77586C59, sceNetCtlAdhocGetPeerList);
	REG_FUNC(0x7118C99D, sceNetCtlAdhocGetInAddr);
});
