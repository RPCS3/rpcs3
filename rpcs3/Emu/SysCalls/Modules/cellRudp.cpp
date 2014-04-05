#include "stdafx.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/SC_FUNC.h"

void cellRudp_init();
Module cellRudp(0x0057, cellRudp_init);

// Return Codes
enum
{
	CELL_RUDP_SUCCESS                            = 0,
	CELL_RUDP_ERROR_NOT_INITIALIZED              = 0x80770001,
	CELL_RUDP_ERROR_ALREADY_INITIALIZED          = 0x80770002,
	CELL_RUDP_ERROR_INVALID_CONTEXT_ID           = 0x80770003,
	CELL_RUDP_ERROR_INVALID_ARGUMENT             = 0x80770004,
	CELL_RUDP_ERROR_INVALID_OPTION               = 0x80770005,
	CELL_RUDP_ERROR_INVALID_MUXMODE              = 0x80770006,
	CELL_RUDP_ERROR_MEMORY                       = 0x80770007,
	CELL_RUDP_ERROR_INTERNAL                     = 0x80770008,
	CELL_RUDP_ERROR_CONN_RESET                   = 0x80770009,
	CELL_RUDP_ERROR_CONN_REFUSED                 = 0x8077000a,
	CELL_RUDP_ERROR_CONN_TIMEOUT                 = 0x8077000b,
	CELL_RUDP_ERROR_CONN_VERSION_MISMATCH        = 0x8077000c,
	CELL_RUDP_ERROR_CONN_TRANSPORT_TYPE_MISMATCH = 0x8077000d,
	CELL_RUDP_ERROR_QUALITY_LEVEL_MISMATCH       = 0x8077000e,
	CELL_RUDP_ERROR_THREAD                       = 0x8077000f,
	CELL_RUDP_ERROR_THREAD_IN_USE                = 0x80770010,
	CELL_RUDP_ERROR_NOT_ACCEPTABLE               = 0x80770011,
	CELL_RUDP_ERROR_MSG_TOO_LARGE                = 0x80770012,
	CELL_RUDP_ERROR_NOT_BOUND                    = 0x80770013,
	CELL_RUDP_ERROR_CANCELLED                    = 0x80770014,
	CELL_RUDP_ERROR_INVALID_VPORT                = 0x80770015,
	CELL_RUDP_ERROR_WOULDBLOCK                   = 0x80770016,
	CELL_RUDP_ERROR_VPORT_IN_USE                 = 0x80770017,
	CELL_RUDP_ERROR_VPORT_EXHAUSTED              = 0x80770018,
	CELL_RUDP_ERROR_INVALID_SOCKET               = 0x80770019,
	CELL_RUDP_ERROR_BUFFER_TOO_SMALL             = 0x8077001a,
	CELL_RUDP_ERROR_MSG_MALFORMED                = 0x8077001b,
	CELL_RUDP_ERROR_ADDR_IN_USE                  = 0x8077001c,
	CELL_RUDP_ERROR_ALREADY_BOUND                = 0x8077001d,
	CELL_RUDP_ERROR_ALREADY_EXISTS               = 0x8077001e,
	CELL_RUDP_ERROR_INVALID_POLL_ID              = 0x8077001f,
	CELL_RUDP_ERROR_TOO_MANY_CONTEXTS            = 0x80770020,
	CELL_RUDP_ERROR_IN_PROGRESS                  = 0x80770021,
	CELL_RUDP_ERROR_NO_EVENT_HANDLER             = 0x80770022,
	CELL_RUDP_ERROR_PAYLOAD_TOO_LARGE            = 0x80770023,
	CELL_RUDP_ERROR_END_OF_DATA                  = 0x80770024,
	CELL_RUDP_ERROR_ALREADY_ESTABLISHED          = 0x80770025,
	CELL_RUDP_ERROR_KEEP_ALIVE_FAILURE           = 0x80770026,
};

int cellRudpInit()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

int cellRudpEnd()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

int cellRudpEnableInternalIOThread()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

int cellRudpSetEventHandler()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

int cellRudpSetMaxSegmentSize()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

int cellRudpGetMaxSegmentSize()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

int cellRudpCreateContext()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

int cellRudpSetOption()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

int cellRudpGetOption()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

int cellRudpGetContextStatus()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

int cellRudpGetStatus()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

int cellRudpGetLocalInfo()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

int cellRudpGetRemoteInfo()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

int cellRudpBind()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

int cellRudpInitiate()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

int cellRudpActivate()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

int cellRudpTerminate()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

int cellRudpRead()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

int cellRudpWrite()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

int cellRudpGetSizeReadable()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

int cellRudpGetSizeWritable()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

int cellRudpFlush()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

int cellRudpPollCreate()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

int cellRudpPollDestroy()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

int cellRudpPollControl()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

int cellRudpPollWait()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

int cellRudpNetReceived()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

int cellRudpProcessEvents()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

void cellRudp_init()
{
	cellRudp.AddFunc(0x63f63545, cellRudpInit);
	cellRudp.AddFunc(0xb6bcb4a1, cellRudpEnd);
	cellRudp.AddFunc(0x6c0cff03, cellRudpEnableInternalIOThread);
	cellRudp.AddFunc(0x7ed95e60, cellRudpSetEventHandler);
	cellRudp.AddFunc(0x54f81789, cellRudpSetMaxSegmentSize);
	cellRudp.AddFunc(0xfbf7e9e4, cellRudpGetMaxSegmentSize);

	cellRudp.AddFunc(0x7dadc739, cellRudpCreateContext);
	cellRudp.AddFunc(0x384ba777, cellRudpSetOption);
	cellRudp.AddFunc(0xff9d259c, cellRudpGetOption);

	cellRudp.AddFunc(0x74bfad12, cellRudpGetContextStatus);
	cellRudp.AddFunc(0xcd1a3f23, cellRudpGetStatus);
	cellRudp.AddFunc(0xd666931f, cellRudpGetLocalInfo);
	cellRudp.AddFunc(0x576831ae, cellRudpGetRemoteInfo);

	cellRudp.AddFunc(0xee41e16a, cellRudpBind);
	cellRudp.AddFunc(0xc407844f, cellRudpInitiate);
	cellRudp.AddFunc(0xc1ad7ced, cellRudpActivate);
	cellRudp.AddFunc(0x48d3eeac, cellRudpTerminate);

	cellRudp.AddFunc(0x92e4d899, cellRudpRead);
	cellRudp.AddFunc(0x48c001b0, cellRudpWrite);
	cellRudp.AddFunc(0x2cde989f, cellRudpGetSizeReadable);
	cellRudp.AddFunc(0xa86b28e3, cellRudpGetSizeWritable);
	cellRudp.AddFunc(0xa70737da, cellRudpFlush);

	cellRudp.AddFunc(0x6bc587e9, cellRudpPollCreate);
	cellRudp.AddFunc(0x8ac398f1, cellRudpPollDestroy);
	cellRudp.AddFunc(0xa3db855c, cellRudpPollControl);
	cellRudp.AddFunc(0xd8310700, cellRudpPollWait);
	//cellRudp.AddFunc(, cellRudpPollCancel);

	cellRudp.AddFunc(0x6ee04954, cellRudpNetReceived);
	cellRudp.AddFunc(0xfade48b2, cellRudpProcessEvents);
}