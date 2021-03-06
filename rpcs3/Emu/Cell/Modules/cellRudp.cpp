#include "stdafx.h"
#include "Emu/IdManager.h"
#include "Emu/Cell/PPUModule.h"

#include "cellRudp.h"

LOG_CHANNEL(cellRudp);

template <>
void fmt_class_string<CellRudpError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
			STR_CASE(CELL_RUDP_ERROR_NOT_INITIALIZED);
			STR_CASE(CELL_RUDP_ERROR_ALREADY_INITIALIZED);
			STR_CASE(CELL_RUDP_ERROR_INVALID_CONTEXT_ID);
			STR_CASE(CELL_RUDP_ERROR_INVALID_ARGUMENT);
			STR_CASE(CELL_RUDP_ERROR_INVALID_OPTION);
			STR_CASE(CELL_RUDP_ERROR_INVALID_MUXMODE);
			STR_CASE(CELL_RUDP_ERROR_MEMORY);
			STR_CASE(CELL_RUDP_ERROR_INTERNAL);
			STR_CASE(CELL_RUDP_ERROR_CONN_RESET);
			STR_CASE(CELL_RUDP_ERROR_CONN_REFUSED);
			STR_CASE(CELL_RUDP_ERROR_CONN_TIMEOUT);
			STR_CASE(CELL_RUDP_ERROR_CONN_VERSION_MISMATCH);
			STR_CASE(CELL_RUDP_ERROR_CONN_TRANSPORT_TYPE_MISMATCH);
			STR_CASE(CELL_RUDP_ERROR_QUALITY_LEVEL_MISMATCH);
			STR_CASE(CELL_RUDP_ERROR_THREAD);
			STR_CASE(CELL_RUDP_ERROR_THREAD_IN_USE);
			STR_CASE(CELL_RUDP_ERROR_NOT_ACCEPTABLE);
			STR_CASE(CELL_RUDP_ERROR_MSG_TOO_LARGE);
			STR_CASE(CELL_RUDP_ERROR_NOT_BOUND);
			STR_CASE(CELL_RUDP_ERROR_CANCELLED);
			STR_CASE(CELL_RUDP_ERROR_INVALID_VPORT);
			STR_CASE(CELL_RUDP_ERROR_WOULDBLOCK);
			STR_CASE(CELL_RUDP_ERROR_VPORT_IN_USE);
			STR_CASE(CELL_RUDP_ERROR_VPORT_EXHAUSTED);
			STR_CASE(CELL_RUDP_ERROR_INVALID_SOCKET);
			STR_CASE(CELL_RUDP_ERROR_BUFFER_TOO_SMALL);
			STR_CASE(CELL_RUDP_ERROR_MSG_MALFORMED);
			STR_CASE(CELL_RUDP_ERROR_ADDR_IN_USE);
			STR_CASE(CELL_RUDP_ERROR_ALREADY_BOUND);
			STR_CASE(CELL_RUDP_ERROR_ALREADY_EXISTS);
			STR_CASE(CELL_RUDP_ERROR_INVALID_POLL_ID);
			STR_CASE(CELL_RUDP_ERROR_TOO_MANY_CONTEXTS);
			STR_CASE(CELL_RUDP_ERROR_IN_PROGRESS);
			STR_CASE(CELL_RUDP_ERROR_NO_EVENT_HANDLER);
			STR_CASE(CELL_RUDP_ERROR_PAYLOAD_TOO_LARGE);
			STR_CASE(CELL_RUDP_ERROR_END_OF_DATA);
			STR_CASE(CELL_RUDP_ERROR_ALREADY_ESTABLISHED);
			STR_CASE(CELL_RUDP_ERROR_KEEP_ALIVE_FAILURE);
		}

		return unknown;
	});
}

struct rudp_info
{
	// allocator functions
	std::function<vm::ptr<void>(ppu_thread& ppu, u32 size)> malloc;
	std::function<void(ppu_thread& ppu, vm::ptr<void> ptr)> free;

	// event handler function
	vm::ptr<CellRudpEventHandler> handler = vm::null;
	vm::ptr<void> handler_arg{};

	shared_mutex mutex;
};

error_code cellRudpInit(vm::ptr<CellRudpAllocator> allocator)
{
	cellRudp.warning("cellRudpInit(allocator=*0x%x)", allocator);

	auto& rudp = g_fxo->get<rudp_info>();

	if (rudp.malloc)
	{
		return CELL_RUDP_ERROR_ALREADY_INITIALIZED;
	}

	if (allocator)
	{
		rudp.malloc = allocator->app_malloc;
		rudp.free = allocator->app_free;
	}
	else
	{
		rudp.malloc = [](ppu_thread&, u32 size)
		{
			return vm::ptr<void>::make(vm::alloc(size, vm::main));
		};

		rudp.free = [](ppu_thread&, vm::ptr<void> ptr)
		{
			if (!vm::dealloc(ptr.addr(), vm::main))
			{
				fmt::throw_exception("Memory deallocation failed (ptr=0x%x)", ptr);
			}
		};
	}

	return CELL_OK;
}

error_code cellRudpEnd()
{
	cellRudp.warning("cellRudpEnd()");

	auto& rudp = g_fxo->get<rudp_info>();

	if (!rudp.malloc)
	{
		return CELL_RUDP_ERROR_NOT_INITIALIZED;
	}

	rudp.malloc = nullptr;
	rudp.free = nullptr;
	rudp.handler = vm::null;

	return CELL_OK;
}

error_code cellRudpEnableInternalIOThread()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

error_code cellRudpSetEventHandler(vm::ptr<CellRudpEventHandler> handler, vm::ptr<void> arg)
{
	cellRudp.todo("cellRudpSetEventHandler(handler=*0x%x, arg=*0x%x)", handler, arg);

	auto& rudp = g_fxo->get<rudp_info>();

	if (!rudp.malloc)
	{
		return CELL_RUDP_ERROR_NOT_INITIALIZED;
	}

	rudp.handler = handler;
	rudp.handler_arg = arg;

	return CELL_OK;
}

error_code cellRudpSetMaxSegmentSize(u16 mss)
{
	cellRudp.todo("cellRudpSetMaxSegmentSize(mss=%d)", mss);
	return CELL_OK;
}

error_code cellRudpGetMaxSegmentSize()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

error_code cellRudpCreateContext()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

error_code cellRudpSetOption()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

error_code cellRudpGetOption()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

error_code cellRudpGetContextStatus()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

error_code cellRudpGetStatus()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

error_code cellRudpGetLocalInfo()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

error_code cellRudpGetRemoteInfo()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

error_code cellRudpAccept()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

error_code cellRudpBind()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

error_code cellRudpListen()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

error_code cellRudpInitiate()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

error_code cellRudpActivate()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

error_code cellRudpTerminate()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

error_code cellRudpRead()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

error_code cellRudpWrite()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

error_code cellRudpGetSizeReadable()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

error_code cellRudpGetSizeWritable()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

error_code cellRudpFlush()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

error_code cellRudpPollCreate()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

error_code cellRudpPollDestroy()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

error_code cellRudpPollControl()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

error_code cellRudpPollWait()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

error_code cellRudpPollCancel()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

error_code cellRudpNetReceived()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

error_code cellRudpProcessEvents()
{
	UNIMPLEMENTED_FUNC(cellRudp);
	return CELL_OK;
}

DECLARE(ppu_module_manager::cellRudp)("cellRudp", []()
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

	REG_FUNC(cellRudp, cellRudpAccept);
	REG_FUNC(cellRudp, cellRudpBind);
	REG_FUNC(cellRudp, cellRudpListen);
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
	REG_FUNC(cellRudp, cellRudpPollCancel);

	REG_FUNC(cellRudp, cellRudpNetReceived);
	REG_FUNC(cellRudp, cellRudpProcessEvents);
});
