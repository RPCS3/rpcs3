#include "stdafx.h"
#if 0

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
	REG_FUNC(cellRudp, cellRudpInit);
	REG_FUNC(cellRudp, cellRudpEnd);
	REG_FUNC(cellRudp, cellRudpEnableInternalIOThread);
	REG_FUNC(cellRudp, cellRudpSetEventHandler);
	REG_FUNC(cellRudp, cellRudpSetMaxSegmentSize);
	REG_FUNC(cellRudp, cellRudpGetMaxSegmentSize);

	REG_FUNC(cellRudp, cellRudpCreateContext);
	REG_FUNC(cellRudp, cellRudpSetOption);
	REG_FUNC(cellRudp, cellRudpGetOption);

	REG_FUNC(cellRudp, cellRudpGetContextStatus);
	REG_FUNC(cellRudp, cellRudpGetStatus);
	REG_FUNC(cellRudp, cellRudpGetLocalInfo);
	REG_FUNC(cellRudp, cellRudpGetRemoteInfo);

	REG_FUNC(cellRudp, cellRudpBind);
	REG_FUNC(cellRudp, cellRudpInitiate);
	REG_FUNC(cellRudp, cellRudpActivate);
	REG_FUNC(cellRudp, cellRudpTerminate);

	REG_FUNC(cellRudp, cellRudpRead);
	REG_FUNC(cellRudp, cellRudpWrite);
	REG_FUNC(cellRudp, cellRudpGetSizeReadable);
	REG_FUNC(cellRudp, cellRudpGetSizeWritable);
	REG_FUNC(cellRudp, cellRudpFlush);

	REG_FUNC(cellRudp, cellRudpPollCreate);
	REG_FUNC(cellRudp, cellRudpPollDestroy);
	REG_FUNC(cellRudp, cellRudpPollControl);
	REG_FUNC(cellRudp, cellRudpPollWait);
	//cellRudp.AddFunc(, cellRudpPollCancel);

	REG_FUNC(cellRudp, cellRudpNetReceived);
	REG_FUNC(cellRudp, cellRudpProcessEvents);
}
#endif
