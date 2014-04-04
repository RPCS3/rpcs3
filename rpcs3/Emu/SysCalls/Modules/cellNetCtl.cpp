#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void cellNetCtl_init();
Module cellNetCtl(0x0014, cellNetCtl_init);

// Error Codes
enum
{
	CELL_NET_CTL_ERROR_NOT_INITIALIZED               = 0x80130101,
	CELL_NET_CTL_ERROR_NOT_TERMINATED                = 0x80130102,
	CELL_NET_CTL_ERROR_HANDLER_MAX                   = 0x80130103,
	CELL_NET_CTL_ERROR_ID_NOT_FOUND                  = 0x80130104,
	CELL_NET_CTL_ERROR_INVALID_ID                    = 0x80130105,
	CELL_NET_CTL_ERROR_INVALID_CODE                  = 0x80130106,
	CELL_NET_CTL_ERROR_INVALID_ADDR                  = 0x80130107,
	CELL_NET_CTL_ERROR_NOT_CONNECTED                 = 0x80130108,
	CELL_NET_CTL_ERROR_NOT_AVAIL                     = 0x80130109,
	CELL_NET_CTL_ERROR_INVALID_TYPE                  = 0x8013010a,
	CELL_NET_CTL_ERROR_INVALID_SIZE                  = 0x8013010b,
	CELL_NET_CTL_ERROR_NET_DISABLED                  = 0x80130181,
	CELL_NET_CTL_ERROR_NET_NOT_CONNECTED             = 0x80130182,
	CELL_NET_CTL_ERROR_NP_NO_ACCOUNT                 = 0x80130183,
	CELL_NET_CTL_ERROR_NP_RESERVED1                  = 0x80130184,
	CELL_NET_CTL_ERROR_NP_RESERVED2                  = 0x80130185,
	CELL_NET_CTL_ERROR_NET_CABLE_NOT_CONNECTED       = 0x80130186,
	CELL_NET_CTL_ERROR_DIALOG_CANCELED               = 0x80130190,
	CELL_NET_CTL_ERROR_DIALOG_ABORTED                = 0x80130191,

	CELL_NET_CTL_ERROR_WLAN_DEAUTHED                 = 0x80130137,
	CELL_NET_CTL_ERROR_WLAN_KEYINFO_EXCHNAGE_TIMEOUT = 0x8013013d,
	CELL_NET_CTL_ERROR_WLAN_ASSOC_FAILED             = 0x8013013e,
	CELL_NET_CTL_ERROR_WLAN_AP_DISAPPEARED           = 0x8013013f,
	CELL_NET_CTL_ERROR_PPPOE_SESSION_INIT            = 0x80130409,
	CELL_NET_CTL_ERROR_PPPOE_SESSION_NO_PADO         = 0x8013040a,
	CELL_NET_CTL_ERROR_PPPOE_SESSION_NO_PADS         = 0x8013040b,
	CELL_NET_CTL_ERROR_PPPOE_SESSION_GET_PADT        = 0x8013040d,
	CELL_NET_CTL_ERROR_PPPOE_SESSION_SERVICE_NAME    = 0x8013040f,
	CELL_NET_CTL_ERROR_PPPOE_SESSION_AC_SYSTEM       = 0x80130410,
	CELL_NET_CTL_ERROR_PPPOE_SESSION_GENERIC         = 0x80130411,
	CELL_NET_CTL_ERROR_PPPOE_STATUS_AUTH             = 0x80130412,
	CELL_NET_CTL_ERROR_PPPOE_STATUS_NETWORK          = 0x80130413,
	CELL_NET_CTL_ERROR_PPPOE_STATUS_TERMINATE        = 0x80130414,
	CELL_NET_CTL_ERROR_DHCP_LEASE_TIME               = 0x80130504,
};

// Network connection states
enum
{
	CELL_NET_CTL_STATE_Disconnected = 0,
	CELL_NET_CTL_STATE_Connecting   = 1,
	CELL_NET_CTL_STATE_IPObtaining  = 2,
	CELL_NET_CTL_STATE_IPObtained   = 3,
};

int cellNetCtlInit()
{
	UNIMPLEMENTED_FUNC(cellNetCtl);
	return CELL_OK;
}

int cellNetCtlTerm()
{
	UNIMPLEMENTED_FUNC(cellNetCtl);
	return CELL_OK;
}

int cellNetCtlGetState(mem32_t state)
{
	cellNetCtl.Log("cellNetCtlGetState(state_addr=0x%x)", state.GetAddr());

	if (!state.IsGood())
		return CELL_NET_CTL_ERROR_INVALID_ADDR;

	state = CELL_NET_CTL_STATE_Disconnected; // TODO: Allow other states
	return CELL_OK;
}

int cellNetCtlAddHandler()
{
	UNIMPLEMENTED_FUNC(cellNetCtl);
	return CELL_OK;
}

int cellNetCtlDelHandler()
{
	UNIMPLEMENTED_FUNC(cellNetCtl);
	return CELL_OK;
}

int cellNetCtlGetInfo()
{
	UNIMPLEMENTED_FUNC(cellNetCtl);
	return CELL_OK;
}

int cellNetCtlNetStartDialogLoadAsync()
{
	UNIMPLEMENTED_FUNC(cellNetCtl);
	return CELL_OK;
}

int cellNetCtlNetStartDialogAbortAsync()
{
	UNIMPLEMENTED_FUNC(cellNetCtl);
	return CELL_OK;
}

int cellNetCtlNetStartDialogUnloadAsync()
{
	UNIMPLEMENTED_FUNC(cellNetCtl);
	return CELL_OK;
}

int cellNetCtlGetNatInfo()
{
	UNIMPLEMENTED_FUNC(cellNetCtl);
	return CELL_OK;
}

void cellNetCtl_init()
{
	cellNetCtl.AddFunc(0xbd5a59fc, cellNetCtlInit);
	cellNetCtl.AddFunc(0x105ee2cb, cellNetCtlTerm);

	cellNetCtl.AddFunc(0x8b3eba69, cellNetCtlGetState);
	cellNetCtl.AddFunc(0x0ce13c6b, cellNetCtlAddHandler);
	cellNetCtl.AddFunc(0x901815c3, cellNetCtlDelHandler);

	cellNetCtl.AddFunc(0x1e585b5d, cellNetCtlGetInfo);

	cellNetCtl.AddFunc(0x04459230, cellNetCtlNetStartDialogLoadAsync);
	cellNetCtl.AddFunc(0x71d53210, cellNetCtlNetStartDialogAbortAsync);
	cellNetCtl.AddFunc(0x0f1f13d3, cellNetCtlNetStartDialogUnloadAsync);

	cellNetCtl.AddFunc(0x3a12865f, cellNetCtlGetNatInfo);
}