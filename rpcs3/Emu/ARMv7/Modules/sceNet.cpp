#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceNet;

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceNet, #name, name)

psv_log_base sceNet("SceNet", []()
{
	sceNet.on_load = nullptr;
	sceNet.on_unload = nullptr;
	sceNet.on_stop = nullptr;

	//REG_FUNC(0xD62EF218, sceNetSetDnsInfo);
	//REG_FUNC(0xFEC1166D, sceNetClearDnsCache);
	//REG_FUNC(0xAFF9FA4D, sceNetDumpCreate);
	//REG_FUNC(0x04042925, sceNetDumpRead);
	//REG_FUNC(0x82DDCF63, sceNetDumpDestroy);
	//REG_FUNC(0x3B24E75F, sceNetDumpAbort);
	//REG_FUNC(0xF9D102AE, sceNetEpollCreate);
	//REG_FUNC(0x4C8764AC, sceNetEpollControl);
	//REG_FUNC(0x45CE337D, sceNetEpollWait);
	//REG_FUNC(0x92D3E767, sceNetEpollWaitCB);
	//REG_FUNC(0x7915CAF3, sceNetEpollDestroy);
	//REG_FUNC(0x93FCC4E8, sceNetEpollAbort);
	//REG_FUNC(0xE37F34AA, sceNetErrnoLoc);
	//REG_FUNC(0xEEC6D75F, sceNetEtherStrton);
	//REG_FUNC(0x84334EB2, sceNetEtherNtostr);
	//REG_FUNC(0x06C05518, sceNetGetMacAddress);
	//REG_FUNC(0x98839B74, sceNetInetNtop);
	//REG_FUNC(0xD5EEB048, sceNetInetPton);
	//REG_FUNC(0x12C19209, sceNetHtonll);
	//REG_FUNC(0x4C30B03C, sceNetHtonl);
	//REG_FUNC(0x9FA3207B, sceNetHtons);
	//REG_FUNC(0xFB3336A6, sceNetNtohll);
	//REG_FUNC(0xD2EAA645, sceNetNtohl);
	//REG_FUNC(0x07845128, sceNetNtohs);
	//REG_FUNC(0xEB03E265, sceNetInit);
	//REG_FUNC(0xEA3CC286, sceNetTerm);
	//REG_FUNC(0x658B903B, sceNetShowIfconfig);
	//REG_FUNC(0x6AB3B74B, sceNetShowRoute);
	//REG_FUNC(0x338EDC2E, sceNetShowNetstat);
	//REG_FUNC(0x561DFD03, sceNetEmulationSet);
	//REG_FUNC(0xAE3F4AC6, sceNetEmulationGet);
	//REG_FUNC(0x6DA29319, sceNetResolverCreate);
	//REG_FUNC(0x1EB11857, sceNetResolverStartNtoa);
	//REG_FUNC(0x0424AE26, sceNetResolverStartAton);
	//REG_FUNC(0x874EF500, sceNetResolverGetError);
	//REG_FUNC(0x3559F098, sceNetResolverDestroy);
	//REG_FUNC(0x38EBBD57, sceNetResolverAbort);
	//REG_FUNC(0xF084FCE3, sceNetSocket);
	//REG_FUNC(0x1ADF9BB1, sceNetAccept);
	//REG_FUNC(0x1296A94B, sceNetBind);
	//REG_FUNC(0x11E5B6F6, sceNetConnect);
	//REG_FUNC(0x2348D353, sceNetGetpeername);
	//REG_FUNC(0x1C66A6DB, sceNetGetsockname);
	//REG_FUNC(0xBA652062, sceNetGetsockopt);
	//REG_FUNC(0x7A8DA094, sceNetListen);
	//REG_FUNC(0x023643B7, sceNetRecv);
	//REG_FUNC(0xB226138B, sceNetRecvfrom);
	//REG_FUNC(0xDE94C6FE, sceNetRecvmsg);
	//REG_FUNC(0xE3DD8CD9, sceNetSend);
	//REG_FUNC(0x52DB31D5, sceNetSendto);
	//REG_FUNC(0x99C579AE, sceNetSendmsg);
	//REG_FUNC(0x065505CA, sceNetSetsockopt);
	//REG_FUNC(0x69E50BB5, sceNetShutdown);
	//REG_FUNC(0x29822B4D, sceNetSocketClose);
	//REG_FUNC(0x891C1B9B, sceNetSocketAbort);
	//REG_FUNC(0xB1AF6840, sceNetGetSockInfo);
	//REG_FUNC(0x138CF1D6, sceNetGetSockIdInfo);
	//REG_FUNC(0xA86F8FE5, sceNetGetStatisticsInfo);
});
