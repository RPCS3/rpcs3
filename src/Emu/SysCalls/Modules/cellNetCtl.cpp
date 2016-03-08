#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/state.h"
#include "Emu/SysCalls/Modules.h"

#include "cellSysutil.h"
#include "cellNetCtl.h"

#ifdef _WIN32
#include <winsock2.h>
#include <windows.h>
#include <iphlpapi.h>

#pragma comment(lib, "iphlpapi.lib")
#else
#include <unistd.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <fcntl.h>
#endif

extern Module<> cellNetCtl;

s32 cellNetCtlInit()
{
	cellNetCtl.warning("cellNetCtlInit()");

	return CELL_OK;
}

s32 cellNetCtlTerm()
{
	cellNetCtl.warning("cellNetCtlTerm()");

	return CELL_OK;
}

s32 cellNetCtlGetState(vm::ptr<u32> state)
{
	cellNetCtl.trace("cellNetCtlGetState(state=*0x%x)", state);

	switch (rpcs3::config.misc.net.status.value())
	{
	case misc_net_status::ip_obtained: *state = CELL_NET_CTL_STATE_IPObtained; break;
	case misc_net_status::obtaining_ip: *state = CELL_NET_CTL_STATE_IPObtaining; break;
	case misc_net_status::connecting: *state = CELL_NET_CTL_STATE_Connecting; break;
	case misc_net_status::disconnected: *state = CELL_NET_CTL_STATE_Disconnected; break;

	default: *state = CELL_NET_CTL_STATE_Disconnected; break;
	}

	return CELL_OK;
}

s32 cellNetCtlAddHandler(vm::ptr<cellNetCtlHandler> handler, vm::ptr<void> arg, vm::ptr<s32> hid)
{
	cellNetCtl.todo("cellNetCtlAddHandler(handler=*0x%x, arg=*0x%x, hid=*0x%x)", handler, arg, hid);

	return CELL_OK;
}

s32 cellNetCtlDelHandler(s32 hid)
{
	cellNetCtl.todo("cellNetCtlDelHandler(hid=0x%x)", hid);

	return CELL_OK;
}

s32 cellNetCtlGetInfo(s32 code, vm::ptr<CellNetCtlInfo> info)
{
	cellNetCtl.todo("cellNetCtlGetInfo(code=0x%x (%s), info=*0x%x)", code, InfoCodeToName(code), info);

	if (code == CELL_NET_CTL_INFO_MTU)
	{
#ifdef _WIN32
		ULONG bufLen = sizeof(PIP_ADAPTER_ADDRESSES) + 1;
		PIP_ADAPTER_ADDRESSES pAddresses = (PIP_ADAPTER_ADDRESSES)malloc(bufLen);
		DWORD ret;

		ret = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, nullptr, pAddresses, &bufLen);

		if (ret == ERROR_BUFFER_OVERFLOW)
		{
			cellNetCtl.error("cellNetCtlGetInfo(INFO_MTU): GetAdaptersAddresses buffer overflow.");
			free(pAddresses);
			pAddresses = (PIP_ADAPTER_ADDRESSES)malloc(bufLen);

			if (pAddresses == nullptr)
			{
				cellNetCtl.error("cellNetCtlGetInfo(INFO_MTU): Unable to allocate memory for pAddresses.");
				return CELL_NET_CTL_ERROR_NET_CABLE_NOT_CONNECTED;
			}
		}
		
		ret = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, nullptr, pAddresses, &bufLen);

		if (ret == NO_ERROR)
		{
			PIP_ADAPTER_ADDRESSES pCurrAddresses = pAddresses;

			for (int c = 0; c < rpcs3::config.misc.net._interface.value(); c++)
			{
				pCurrAddresses = pCurrAddresses->Next;
			}

			info->mtu = pCurrAddresses->Mtu;
		}
		else
		{
			cellNetCtl.error("cellNetCtlGetInfo(INFO_MTU): Call to GetAdaptersAddresses failed. (%d)", ret);
			info->mtu = 1500; // Seems to be the default value on Windows 10.
		}

		free(pAddresses);
#else
		struct ifaddrs *ifaddr, *ifa;
		s32 family, n;

		if (getifaddrs(&ifaddr) == -1)
		{
			cellNetCtl.error("cellNetCtlGetInfo(INFO_MTU): Call to getifaddrs returned negative.");
		}

		for (ifa = ifaddr, n = 0; ifa != nullptr; ifa = ifa->ifa_next, n++)
		{
			if (ifa->ifa_addr == nullptr)
			{
				continue;
			}

			if (n < rpcs3::config.misc.net._interface.value())
			{
				continue;
			}

			family = ifa->ifa_addr->sa_family;

			if (family == AF_INET)
			{
				u32 fd = open("/proc/net/dev", O_RDONLY);
				struct ifreq freq;

				if (ioctl(fd, SIOCGIFMTU, &freq) == -1)
				{
					cellNetCtl.error("cellNetCtlGetInfo(INFO_MTU): Call to ioctl failed.");
				}
				else
				{
					info->mtu = (u32)freq.ifr_mtu;
				}

				close(fd);
			}
		}

		freeifaddrs(ifaddr);
#endif
	}
	else if (code == CELL_NET_CTL_INFO_LINK)
	{
		if (rpcs3::config.misc.net.status.value() == misc_net_status::ip_obtained)
		{
			info->link = CELL_NET_CTL_LINK_CONNECTED;
		}
		else
		{
			info->link = CELL_NET_CTL_LINK_DISCONNECTED;
		}
	}
	else if (code == CELL_NET_CTL_INFO_IP_ADDRESS)
	{
		// 0.0.0.0 seems to be the default address when no ethernet cables are connected to the PS3
		strcpy_trunc(info->ip_address, "0.0.0.0");

#ifdef _WIN32
		ULONG bufLen = sizeof(IP_ADAPTER_INFO);
		PIP_ADAPTER_INFO pAdapterInfo = (IP_ADAPTER_INFO*)malloc(bufLen);
		DWORD ret;

		ret = GetAdaptersInfo(pAdapterInfo, &bufLen);
		
		if (ret == ERROR_BUFFER_OVERFLOW)
		{
			cellNetCtl.error("cellNetCtlGetInfo(IP_ADDRESS): GetAdaptersInfo buffer overflow.");
			free(pAdapterInfo);
			pAdapterInfo = (IP_ADAPTER_INFO*)malloc(bufLen);

			if (pAdapterInfo == nullptr)
			{
				cellNetCtl.error("cellNetCtlGetInfo(IP_ADDRESS): Unable to allocate memory for pAddresses.");
				return CELL_NET_CTL_ERROR_NET_CABLE_NOT_CONNECTED;
			}
		}
		
		ret = GetAdaptersInfo(pAdapterInfo, &bufLen);

		if (ret == NO_ERROR)
		{
			PIP_ADAPTER_INFO pAdapter = pAdapterInfo;

			for (int c = 0; c < rpcs3::config.misc.net._interface.value(); c++)
			{
				pAdapter = pAdapter->Next;
			}

			strcpy_trunc(info->ip_address, pAdapter->IpAddressList.IpAddress.String);
		}
		else
		{
			cellNetCtl.error("cellNetCtlGetInfo(IP_ADDRESS): Call to GetAdaptersInfo failed. (%d)", ret);
		}

		free(pAdapterInfo);
#else
		struct ifaddrs *ifaddr, *ifa;
		s32 family, n;

		if (getifaddrs(&ifaddr) == -1)
		{
			cellNetCtl.error("cellNetCtlGetInfo(IP_ADDRESS): Call to getifaddrs returned negative.");
		}

		for (ifa = ifaddr, n = 0; ifa != nullptr; ifa = ifa->ifa_next, n++)
		{
			if (ifa->ifa_addr == nullptr)
			{
				continue;
			}

			if (n < rpcs3::config.misc.net._interface.value())
			{
				continue;
			}

			family = ifa->ifa_addr->sa_family;

			if (family == AF_INET)
			{
				strcpy_trunc(info->ip_address, ifa->ifa_addr->sa_data);
			}
		}

		freeifaddrs(ifaddr);
#endif
	}
	else if (code == CELL_NET_CTL_INFO_NETMASK)
	{
#ifdef _WIN32
		ULONG bufLen = sizeof(IP_ADAPTER_INFO) + 1;
		PIP_ADAPTER_INFO pAdapterInfo = (PIP_ADAPTER_INFO)malloc(bufLen);
		DWORD ret;

		ret = GetAdaptersInfo(pAdapterInfo, &bufLen);

		if (ret == ERROR_BUFFER_OVERFLOW)
		{
			cellNetCtl.error("cellNetCtlGetInfo(INFO_NETMASK): GetAdaptersInfo buffer overflow.");
			free(pAdapterInfo);
			pAdapterInfo = (IP_ADAPTER_INFO*)malloc(bufLen);

			if (pAdapterInfo == nullptr)
			{
				cellNetCtl.error("cellNetCtlGetInfo(INFO_NETMASK): Unable to allocate memory for pAddresses.");
				return CELL_NET_CTL_ERROR_NET_CABLE_NOT_CONNECTED;
			}
		}
		
		ret = GetAdaptersInfo(pAdapterInfo, &bufLen);

		if (ret == NO_ERROR)
		{
			PIP_ADAPTER_INFO pAdapter = pAdapterInfo;

			for (int c = 0; c < rpcs3::config.misc.net._interface.value(); c++)
			{
				pAdapter = pAdapter->Next;
			}

			for (int c = 0; c < 4; c++)
			{
				info->netmask[c] = (s8)pAdapter->IpAddressList.IpMask.String;
			}
		}
		else
		{
			cellNetCtl.error("cellNetCtlGetInfo(INFO_NETMASK): Call to GetAdaptersInfo failed. (%d)", ret);
			// TODO: Is this the default netmask?
			info->netmask[0] = 255;
			info->netmask[1] = 255;
			info->netmask[2] = 255;
			info->netmask[3] = 0;
		}

		free(pAdapterInfo);
#else
		struct ifaddrs *ifaddr, *ifa;
		s32 family, n;

		if (getifaddrs(&ifaddr) == -1)
		{
			cellNetCtl.error("cellNetCtlGetInfo(INFO_NETMASK): Call to getifaddrs returned negative.");
		}

		for (ifa = ifaddr, n = 0; ifa != nullptr; ifa = ifa->ifa_next, n++)
		{
			if (ifa->ifa_addr == nullptr)
			{
				continue;
			}

			if (n < rpcs3::config.misc.net._interface.value())
			{
				continue;
			}

			family = ifa->ifa_addr->sa_family;

			if (family == AF_INET)
			{
				strcpy_trunc(info->ip_address, ifa->ifa_netmask->sa_data);
			}
	}

		freeifaddrs(ifaddr);
#endif
	}

	return CELL_OK;
}

s32 cellNetCtlNetStartDialogLoadAsync(vm::ptr<CellNetCtlNetStartDialogParam> param)
{
	cellNetCtl.error("cellNetCtlNetStartDialogLoadAsync(param=*0x%x)", param);

	// TODO: Actually sign into PSN or an emulated network similar to PSN (ESN)
	// TODO: Properly open the dialog prompt for sign in
	sysutilSendSystemCommand(CELL_SYSUTIL_NET_CTL_NETSTART_LOADED, 0);
	sysutilSendSystemCommand(CELL_SYSUTIL_NET_CTL_NETSTART_FINISHED, 0);

	return CELL_OK;
}

s32 cellNetCtlNetStartDialogAbortAsync()
{
	cellNetCtl.error("cellNetCtlNetStartDialogAbortAsync()");

	return CELL_OK;
}

s32 cellNetCtlNetStartDialogUnloadAsync(vm::ptr<CellNetCtlNetStartDialogResult> result)
{
	cellNetCtl.warning("cellNetCtlNetStartDialogUnloadAsync(result=*0x%x)", result);

	result->result = CELL_NET_CTL_ERROR_DIALOG_CANCELED;
	sysutilSendSystemCommand(CELL_SYSUTIL_NET_CTL_NETSTART_UNLOADED, 0);

	return CELL_OK;
}

s32 cellNetCtlGetNatInfo(vm::ptr<CellNetCtlNatInfo> natInfo)
{
	cellNetCtl.todo("cellNetCtlGetNatInfo(natInfo=*0x%x)", natInfo);

	if (natInfo->size == 0)
	{
		cellNetCtl.error("cellNetCtlGetNatInfo : CELL_NET_CTL_ERROR_INVALID_SIZE");
		return CELL_NET_CTL_ERROR_INVALID_SIZE;
	}
	
	return CELL_OK;
}

s32 cellGameUpdateInit()
{
	throw EXCEPTION("");
}

s32 cellGameUpdateTerm()
{
	throw EXCEPTION("");
}


s32 cellGameUpdateCheckStartAsync()
{
	throw EXCEPTION("");
}

s32 cellGameUpdateCheckFinishAsync()
{
	throw EXCEPTION("");
}

s32 cellGameUpdateCheckStartWithoutDialogAsync()
{
	throw EXCEPTION("");
}

s32 cellGameUpdateCheckAbort()
{
	throw EXCEPTION("");
}

s32 cellGameUpdateCheckStartAsyncEx()
{
	throw EXCEPTION("");
}

s32 cellGameUpdateCheckFinishAsyncEx()
{
	throw EXCEPTION("");
}

s32 cellGameUpdateCheckStartWithoutDialogAsyncEx()
{
	throw EXCEPTION("");
}


Module<> cellNetCtl("cellNetCtl", []()
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
