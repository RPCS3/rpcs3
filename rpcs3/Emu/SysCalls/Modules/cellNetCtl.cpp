#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/SysCalls/Modules.h"
#include "Utilities/Log.h"

#include "rpcs3/Ini.h"
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

extern Module cellNetCtl;

s32 cellNetCtlInit()
{
	cellNetCtl.Warning("cellNetCtlInit()");

	return CELL_OK;
}

s32 cellNetCtlTerm()
{
	cellNetCtl.Warning("cellNetCtlTerm()");

	return CELL_OK;
}

s32 cellNetCtlGetState(vm::ptr<u32> state)
{
	cellNetCtl.Log("cellNetCtlGetState(state=*0x%x)", state);

	if (Ini.NETStatus.GetValue() == 0)
	{
		*state = CELL_NET_CTL_STATE_IPObtained;
	}
	else if (Ini.NETStatus.GetValue() == 1)
	{
		*state = CELL_NET_CTL_STATE_IPObtaining;
	}
	else if (Ini.NETStatus.GetValue() == 2)
	{
		*state = CELL_NET_CTL_STATE_Connecting;
	}
	else
	{
		*state = CELL_NET_CTL_STATE_Disconnected;
	}

	return CELL_OK;
}

s32 cellNetCtlAddHandler(vm::ptr<cellNetCtlHandler> handler, vm::ptr<void> arg, vm::ptr<s32> hid)
{
	cellNetCtl.Todo("cellNetCtlAddHandler(handler=*0x%x, arg=*0x%x, hid=*0x%x)", handler, arg, hid);

	return CELL_OK;
}

s32 cellNetCtlDelHandler(s32 hid)
{
	cellNetCtl.Todo("cellNetCtlDelHandler(hid=0x%x)", hid);

	return CELL_OK;
}

s32 cellNetCtlGetInfo(s32 code, vm::ptr<CellNetCtlInfo> info)
{
	cellNetCtl.Todo("cellNetCtlGetInfo(code=0x%x (%s), info=*0x%x)", code, InfoCodeToName(code), info);

	if (code == CELL_NET_CTL_INFO_MTU)
	{
#ifdef _WIN32
		PIP_ADAPTER_ADDRESSES pAddresses;
		DWORD ret;
		ULONG outBufLen = sizeof(PIP_ADAPTER_ADDRESSES);

		pAddresses = (IP_ADAPTER_ADDRESSES*)vm::alloc(outBufLen, vm::main);

		if (pAddresses == nullptr)
		{
			cellNetCtl.Error("cellNetCtlGetInfo(INFO_MTU): pAddresses memory allocation failed.");
		}
		else
		{
			ret = GetAdaptersAddresses(AF_INET, GAA_FLAG_INCLUDE_PREFIX, nullptr, pAddresses, &outBufLen);

			if (ret == ERROR_BUFFER_OVERFLOW)
			{
				free(pAddresses);
			}
		}

		if (ret == NO_ERROR)
		{
			PIP_ADAPTER_ADDRESSES pCurrAddresses = pAddresses;

			for (int c = 0; c < Ini.NETInterface.GetValue(); c++)
			{
				pCurrAddresses = pCurrAddresses->Next;
			}

			info->mtu = pCurrAddresses->Mtu;
		}
		else
		{
			cellNetCtl.Error("cellNetCtlGetInfo(INFO_MTU): Call to GetAdaptersAddresses failed.");
			info->mtu = 1490; // Seems to be the default value on Windows, generally.
		}
#else
		struct ifaddrs *ifaddr, *ifa;
		int family, s, n;

		if (getifaddrs(&ifaddr) == -1)
		{
			LOG_ERROR(HLE, "Call to getifaddrs returned negative.");
		}

		for (ifa = ifaddr, n = 0; ifa != nullptr; ifa = ifa->ifa_next, n++)
		{
			if (ifa->ifa_addr == nullptr)
			{
				continue;
			}

			if (n < Ini.NETInterface.GetValue())
			{
				continue;
			}

			family = ifa->ifa_addr->sa_family;

			if (family == AF_INET)
			{
				s32 fd, status;

				fd = open("/proc/net/dev", O_RDONLY);
				struct ifreq freq;

				if (ioctl(fd, SIOCGIFMTU, &freq) == -1)
				{
					cellNetCtl.Error("cellNetCtlGetInfo(INFO_MTU): Call to ioctl failed.");
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
	else if (code == CELL_NET_CTL_INFO_IP_ADDRESS)
	{
#ifdef _WIN32
		PIP_ADAPTER_INFO pAdapterInfo;
		pAdapterInfo = (IP_ADAPTER_INFO*)vm::alloc(sizeof(IP_ADAPTER_INFO), vm::main);
		ULONG buflen = sizeof(IP_ADAPTER_INFO);

		if (GetAdaptersInfo(pAdapterInfo, &buflen) == ERROR_BUFFER_OVERFLOW)
		{
			free(pAdapterInfo);
		}

		if (GetAdaptersInfo(pAdapterInfo, &buflen) == NO_ERROR)
		{
			PIP_ADAPTER_INFO pAdapter = pAdapterInfo;

			for (int c = 0; c < Ini.NETInterface.GetValue(); c++)
			{
				pAdapter = pAdapter->Next;
			}

			strcpy_trunc(info->ip_address, pAdapter->IpAddressList.IpAddress.String);
		}
		else
		{
			cellNetCtl.Error("cellNetCtlGetInfo(IP_ADDRESS): Call to GetAdaptersInfo failed.");
			// 0.0.0.0 seems to be the default address when no ethernet cables are connected to the PS3
			strcpy_trunc(info->ip_address, "0.0.0.0");
		}
#else
		struct ifaddrs *ifaddr, *ifa;
		int family, s, n;

		if (getifaddrs(&ifaddr) == -1)
		{
			LOG_ERROR(HLE, "Call to getifaddrs returned negative.");
		}

		for (ifa = ifaddr, n = 0; ifa != nullptr; ifa = ifa->ifa_next, n++)
		{
			if (ifa->ifa_addr == nullptr)
			{
				continue;
			}

			if (n < Ini.NETInterface.GetValue())
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
		PIP_ADAPTER_INFO pAdapterInfo;
		pAdapterInfo = (IP_ADAPTER_INFO*)vm::alloc(sizeof(IP_ADAPTER_INFO), vm::main);
		ULONG buflen = sizeof(IP_ADAPTER_INFO);

		if (GetAdaptersInfo(pAdapterInfo, &buflen) == ERROR_BUFFER_OVERFLOW)
		{
			free(pAdapterInfo);
		}

		if (GetAdaptersInfo(pAdapterInfo, &buflen) == NO_ERROR)
		{
			PIP_ADAPTER_INFO pAdapter = pAdapterInfo;

			for (int c = 0; c < Ini.NETInterface.GetValue(); c++)
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
			cellNetCtl.Error("cellNetCtlGetInfo(INFO_NETMASK): Call to GetAdaptersInfo failed.");
			// TODO: Is the default netmask default?
			info->netmask[0] = 255;
			info->netmask[1] = 255;
			info->netmask[2] = 255;
			info->netmask[3] = 0;
		}
#else
		struct ifaddrs *ifaddr, *ifa;
		int family, s, n;
		char host[NI_MAXHOST];

		if (getifaddrs(&ifaddr) == -1)
		{
			LOG_ERROR(HLE, "Call to getifaddrs returned negative.");
		}

		for (ifa = ifaddr, n = 0; ifa != nullptr; ifa = ifa->ifa_next, n++)
		{
			if (ifa->ifa_addr == nullptr)
			{
				continue;
			}

			if (n < Ini.NETInterface.GetValue())
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
	cellNetCtl.Warning("cellNetCtlNetStartDialogLoadAsync(param=*0x%x)", param);

	// TODO: Actually sign into PSN or an emulated network similar to PSN
	sysutilSendSystemCommand(CELL_SYSUTIL_NET_CTL_NETSTART_FINISHED, 0);

	return CELL_OK;
}

s32 cellNetCtlNetStartDialogAbortAsync()
{
	cellNetCtl.Todo("cellNetCtlNetStartDialogAbortAsync()");

	return CELL_OK;
}

s32 cellNetCtlNetStartDialogUnloadAsync(vm::ptr<CellNetCtlNetStartDialogResult> result)
{
	cellNetCtl.Warning("cellNetCtlNetStartDialogUnloadAsync(result=*0x%x)", result);

	sysutilSendSystemCommand(CELL_SYSUTIL_NET_CTL_NETSTART_UNLOADED, 0);

	return CELL_OK;
}

s32 cellNetCtlGetNatInfo(vm::ptr<CellNetCtlNatInfo> natInfo)
{
	cellNetCtl.Todo("cellNetCtlGetNatInfo(natInfo=*0x%x)", natInfo);

	if (natInfo->size == 0)
	{
		cellNetCtl.Error("cellNetCtlGetNatInfo : CELL_NET_CTL_ERROR_INVALID_SIZE");
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


Module cellNetCtl("cellNetCtl", []()
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
