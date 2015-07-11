#pragma once

#include "sceNet.h"

union SceNetCtlInfo
{
	char cnf_name[65];
	le_t<u32> device;
	SceNetEtherAddr ether_addr;
	le_t<u32> mtu;
	le_t<u32> link;
	SceNetEtherAddr bssid;
	char ssid[33];
	le_t<u32> wifi_security;
	le_t<u32> rssi_dbm;
	le_t<u32> rssi_percentage;
	le_t<u32> channel;
	le_t<u32> ip_config;
	char dhcp_hostname[256];
	char pppoe_auth_name[128];
	char ip_address[16];
	char netmask[16];
	char default_route[16];
	char primary_dns[16];
	char secondary_dns[16];
	le_t<u32> http_proxy_config;
	char http_proxy_server[256];
	le_t<u32> http_proxy_port;
};

struct SceNetCtlNatInfo
{
	le_t<u32> size;
	le_t<s32> stun_status;
	le_t<s32> nat_type;
	SceNetInAddr mapped_addr;
};

struct SceNetCtlAdhocPeerInfo
{
	vm::lptr<SceNetCtlAdhocPeerInfo> next;
	SceNetInAddr inet_addr;
};

using SceNetCtlCallback = func_def<void(s32 event_type, vm::ptr<void> arg)>;

extern psv_log_base sceNetCtl;
