#include "stdafx.h"
#include "Emu/Cell/PPUModule.h"

#include "cellDaisy.h"

LOG_CHANNEL(cellDaisy);

template <>
void fmt_class_string<CellDaisyError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](CellDaisyError value)
	{
		switch (value)
		{
			STR_CASE(CELL_DAISY_ERROR_NO_BEGIN);
			STR_CASE(CELL_DAISY_ERROR_INVALID_PORT_ATTACH);
			STR_CASE(CELL_DAISY_ERROR_NOT_IMPLEMENTED);
			STR_CASE(CELL_DAISY_ERROR_AGAIN);
			STR_CASE(CELL_DAISY_ERROR_INVAL);
			STR_CASE(CELL_DAISY_ERROR_PERM);
			STR_CASE(CELL_DAISY_ERROR_BUSY);
			STR_CASE(CELL_DAISY_ERROR_STAT);
		}

		return unknown;
	});
}

using LFQueue2 = struct CellDaisyLFQueue2;
using Lock = struct CellDaisyLock;
using ScatterGatherInterlock = struct CellDaisyScatterGatherInterlock;
using AtomicInterlock = volatile struct CellDaisyAtomicInterlock;

error_code cellDaisyLFQueue2GetPopPointer(vm::ptr<LFQueue2> queue, vm::ptr<s32> pPointer, u32 isBlocking)
{
	cellDaisy.todo("cellDaisyLFQueue2GetPopPointer(queue=*0x%x, pPointer=*0x%x, isBlocking=%d)", queue, pPointer, isBlocking);
	return CELL_OK;
}

error_code cellDaisyLFQueue2CompletePopPointer(vm::ptr<LFQueue2> queue, s32 pointer, vm::ptr<s32(vm::ptr<void>, u32)> fpSendSignal, u32 isQueueFull)
{
	cellDaisy.todo("cellDaisyLFQueue2CompletePopPointer(queue=*0x%x, pointer=0x%x, fpSendSignal=*0x%x, isQueueFull=%d)", queue, pointer, fpSendSignal, isQueueFull);
	return CELL_OK;
}

void cellDaisyLFQueue2PushOpen(vm::ptr<LFQueue2> queue)
{
	cellDaisy.todo("cellDaisyLFQueue2PushOpen(queue=*0x%x)", queue);
}

error_code cellDaisyLFQueue2PushClose(vm::ptr<LFQueue2> queue, vm::ptr<s32(vm::ptr<void>, u32)> fpSendSignal)
{
	cellDaisy.todo("cellDaisyLFQueue2PushClose(queue=*0x%x, fpSendSignal=*0x%x)", queue, fpSendSignal);
	return CELL_OK;
}

void cellDaisyLFQueue2PopOpen(vm::ptr<LFQueue2> queue)
{
	cellDaisy.todo("cellDaisyLFQueue2PopOpen(queue=*0x%x)", queue);
}

error_code cellDaisyLFQueue2PopClose(vm::ptr<LFQueue2> queue, vm::ptr<s32(vm::ptr<void>, u32)> fpSendSignal)
{
	cellDaisy.todo("cellDaisyLFQueue2PopClose(queue=*0x%x, fpSendSignal=*0x%x)", queue, fpSendSignal);
	return CELL_OK;
}

error_code cellDaisyLFQueue2HasUnfinishedConsumer(vm::ptr<LFQueue2> queue, u32 isCancelled)
{
	cellDaisy.todo("cellDaisyLFQueue2HasUnfinishedConsumer(queue=*0x%x, isCancelled=%d)", queue, isCancelled);
	return CELL_OK;
}

error_code cellDaisy_snprintf(vm::ptr<char> buffer, u32 count, vm::cptr<char> fmt, ppu_va_args_t fmt_args)
{
	cellDaisy.todo("cellDaisy_snprintf(buffer=*0x%x, count=%d, fmt=*0x%x, fmt_args=%d)", buffer, count, fmt, fmt_args.count);
	return CELL_OK;
}

error_code cellDaisyLock_initialize(vm::ptr<Lock> _this, u32 depth)
{
	cellDaisy.todo("cellDaisyLock_initialize(_this=*0x%x, depth=%d)", _this, depth);
	return CELL_OK;
}

error_code cellDaisyLock_getNextHeadPointer(vm::ptr<Lock> _this)
{
	cellDaisy.todo("cellDaisyLock_getNextHeadPointer(_this=*0x%x)", _this);
	return CELL_OK;
}

error_code cellDaisyLock_getNextTailPointer(vm::ptr<Lock> _this)
{
	cellDaisy.todo("cellDaisyLock_getNextTailPointer(_this=*0x%x)", _this);
	return CELL_OK;
}

error_code cellDaisyLock_completeConsume(vm::ptr<Lock> _this, u32 pointer)
{
	cellDaisy.todo("cellDaisyLock_completeConsume(_this=*0x%x, pointer=0x%x)", _this, pointer);
	return CELL_OK;
}

error_code cellDaisyLock_completeProduce(vm::ptr<Lock> _this, u32 pointer)
{
	cellDaisy.todo("cellDaisyLock_completeProduce(_this=*0x%x, pointer=0x%x)", _this, pointer);
	return CELL_OK;
}

error_code cellDaisyLock_pushOpen(vm::ptr<Lock> _this)
{
	cellDaisy.todo("cellDaisyLock_pushOpen(_this=*0x%x)", _this);
	return CELL_OK;
}

error_code cellDaisyLock_pushClose(vm::ptr<Lock> _this)
{
	cellDaisy.todo("cellDaisyLock_pushClose(_this=*0x%x)", _this);
	return CELL_OK;
}

error_code cellDaisyLock_popOpen(vm::ptr<Lock> _this)
{
	cellDaisy.todo("cellDaisyLock_popOpen(_this=*0x%x)", _this);
	return CELL_OK;
}

error_code cellDaisyLock_popClose(vm::ptr<Lock> _this)
{
	cellDaisy.todo("cellDaisyLock_popClose(_this=*0x%x)", _this);
	return CELL_OK;
}

void cellDaisyScatterGatherInterlock_1(vm::ptr<ScatterGatherInterlock> _this, vm::ptr<AtomicInterlock> ea, u32 size, vm::ptr<void> eaSignal, vm::ptr<s32(vm::ptr<void>, u32)> fpSendSignal)
{
	cellDaisy.todo("cellDaisyScatterGatherInterlock_1(_this=*0x%x, ea=*0x%x, size=%d, eaSignal=*0x%x, fpSendSignal=*0x%x)", _this, ea, size, eaSignal, fpSendSignal);
}

void cellDaisyScatterGatherInterlock_2(vm::ptr<ScatterGatherInterlock> _this, u32 size, vm::ptr<u32> ids, u32 numSpus, u8 spup)
{
	cellDaisy.todo("cellDaisyScatterGatherInterlock_2(_this=*0x%x, size=%d, ids=*0x%x, numSpus=%d, spup=%d)", _this, size, ids, numSpus, spup);
}

void cellDaisyScatterGatherInterlock_9tor(vm::ptr<ScatterGatherInterlock> _this)
{
	cellDaisy.todo("cellDaisyScatterGatherInterlock_9tor(_this=*0x%x)", _this);
}

error_code cellDaisyScatterGatherInterlock_probe(vm::ptr<ScatterGatherInterlock> _this, u32 isBlocking)
{
	cellDaisy.todo("cellDaisyScatterGatherInterlock_probe(_this=*0x%x, isBlocking=%d)", _this, isBlocking);
	return CELL_OK;
}

error_code cellDaisyScatterGatherInterlock_release(vm::ptr<ScatterGatherInterlock> _this)
{
	cellDaisy.todo("cellDaisyScatterGatherInterlock_release(_this=*0x%x)", _this);
	return CELL_OK;
}

void cellDaisyScatterGatherInterlock_proceedSequenceNumber(vm::ptr<ScatterGatherInterlock> _this)
{
	cellDaisy.todo("cellDaisyScatterGatherInterlock_proceedSequenceNumber(_this=*0x%x)", _this);
}


DECLARE(ppu_module_manager::cellDaisy)("cellDaisy", []()
{
	REG_FNID(cellDaisy, "_ZN4cell5Daisy17LFQueue2PushCloseEPNS0_8LFQueue2EPFiPvjE", cellDaisyLFQueue2PushClose);
	REG_FNID(cellDaisy, "_QN4cell5Daisy17LFQueue2PushCloseEPNS0_8LFQueue2EPFiPvjE", cellDaisyLFQueue2PushClose);
	REG_FNID(cellDaisy, "_ZN4cell5Daisy21LFQueue2GetPopPointerEPNS0_8LFQueue2EPij", cellDaisyLFQueue2GetPopPointer);
	REG_FNID(cellDaisy, "_QN4cell5Daisy21LFQueue2GetPopPointerEPNS0_8LFQueue2EPij", cellDaisyLFQueue2GetPopPointer);
	REG_FNID(cellDaisy, "_ZN4cell5Daisy26LFQueue2CompletePopPointerEPNS0_8LFQueue2EiPFiPvjEj", cellDaisyLFQueue2CompletePopPointer);
	REG_FNID(cellDaisy, "_QN4cell5Daisy26LFQueue2CompletePopPointerEPNS0_8LFQueue2EiPFiPvjEj", cellDaisyLFQueue2CompletePopPointer);
	REG_FNID(cellDaisy, "_ZN4cell5Daisy29LFQueue2HasUnfinishedConsumerEPNS0_8LFQueue2Ej", cellDaisyLFQueue2HasUnfinishedConsumer);
	REG_FNID(cellDaisy, "_QN4cell5Daisy29LFQueue2HasUnfinishedConsumerEPNS0_8LFQueue2Ej", cellDaisyLFQueue2HasUnfinishedConsumer);
	REG_FNID(cellDaisy, "_ZN4cell5Daisy16LFQueue2PushOpenEPNS0_8LFQueue2E", cellDaisyLFQueue2PushOpen);
	REG_FNID(cellDaisy, "_QN4cell5Daisy16LFQueue2PushOpenEPNS0_8LFQueue2E", cellDaisyLFQueue2PushOpen);
	REG_FNID(cellDaisy, "_ZN4cell5Daisy16LFQueue2PopCloseEPNS0_8LFQueue2EPFiPvjE", cellDaisyLFQueue2PopClose);
	REG_FNID(cellDaisy, "_QN4cell5Daisy16LFQueue2PopCloseEPNS0_8LFQueue2EPFiPvjE", cellDaisyLFQueue2PopClose);
	REG_FNID(cellDaisy, "_ZN4cell5Daisy15LFQueue2PopOpenEPNS0_8LFQueue2E", cellDaisyLFQueue2PopOpen);
	REG_FNID(cellDaisy, "_QN4cell5Daisy15LFQueue2PopOpenEPNS0_8LFQueue2E", cellDaisyLFQueue2PopOpen);
	REG_FNID(cellDaisy, "_ZN4cell5Daisy9_snprintfEPcjPKcz", cellDaisy_snprintf);
	REG_FNID(cellDaisy, "_QN4cell5Daisy9_snprintfEPcjPKcz", cellDaisy_snprintf);

	REG_FNID(cellDaisy, "_ZN4cell5Daisy4Lock7popOpenEv", cellDaisyLock_popOpen);
	REG_FNID(cellDaisy, "_QN4cell5Daisy4Lock7popOpenEv", cellDaisyLock_popOpen);
	REG_FNID(cellDaisy, "_ZN4cell5Daisy4Lock18getNextHeadPointerEv", cellDaisyLock_getNextHeadPointer);
	REG_FNID(cellDaisy, "_QN4cell5Daisy4Lock18getNextHeadPointerEv", cellDaisyLock_getNextHeadPointer);
	REG_FNID(cellDaisy, "_ZN4cell5Daisy4Lock10initializeEj", cellDaisyLock_initialize);
	REG_FNID(cellDaisy, "_QN4cell5Daisy4Lock10initializeEj", cellDaisyLock_initialize);
	REG_FNID(cellDaisy, "_ZN4cell5Daisy4Lock15completeProduceEj", cellDaisyLock_completeProduce);
	REG_FNID(cellDaisy, "_QN4cell5Daisy4Lock15completeProduceEj", cellDaisyLock_completeProduce);
	REG_FNID(cellDaisy, "_ZN4cell5Daisy4Lock8popCloseEv", cellDaisyLock_popClose);
	REG_FNID(cellDaisy, "_QN4cell5Daisy4Lock8popCloseEv", cellDaisyLock_popClose);
	REG_FNID(cellDaisy, "_ZN4cell5Daisy4Lock18getNextTailPointerEv", cellDaisyLock_getNextTailPointer);
	REG_FNID(cellDaisy, "_QN4cell5Daisy4Lock18getNextTailPointerEv", cellDaisyLock_getNextTailPointer);
	REG_FNID(cellDaisy, "_ZN4cell5Daisy4Lock8pushOpenEv", cellDaisyLock_pushOpen);
	REG_FNID(cellDaisy, "_QN4cell5Daisy4Lock8pushOpenEv", cellDaisyLock_pushOpen);
	REG_FNID(cellDaisy, "_ZN4cell5Daisy4Lock9pushCloseEv", cellDaisyLock_pushClose);
	REG_FNID(cellDaisy, "_QN4cell5Daisy4Lock9pushCloseEv", cellDaisyLock_pushClose);
	REG_FNID(cellDaisy, "_ZN4cell5Daisy4Lock15completeConsumeEj", cellDaisyLock_completeConsume);
	REG_FNID(cellDaisy, "_QN4cell5Daisy4Lock15completeConsumeEj", cellDaisyLock_completeConsume);

	REG_FNID(cellDaisy, "_ZN4cell5Daisy22ScatterGatherInterlockC1EPVNS0_16_AtomicInterlockEjPjjh", cellDaisyScatterGatherInterlock_1);
	REG_FNID(cellDaisy, "_QN4cell5Daisy22ScatterGatherInterlockC1EPVNS0_16_AtomicInterlockEjPjjh", cellDaisyScatterGatherInterlock_1);
	REG_FNID(cellDaisy, "_ZN4cell5Daisy22ScatterGatherInterlockC2EPVNS0_16_AtomicInterlockEjPjjh", cellDaisyScatterGatherInterlock_1);
	REG_FNID(cellDaisy, "_QN4cell5Daisy22ScatterGatherInterlockC2EPVNS0_16_AtomicInterlockEjPjjh", cellDaisyScatterGatherInterlock_1);
	REG_FNID(cellDaisy, "_ZN4cell5Daisy22ScatterGatherInterlockC1EPVNS0_16_AtomicInterlockEjPvPFiS5_jE", cellDaisyScatterGatherInterlock_2);
	REG_FNID(cellDaisy, "_QN4cell5Daisy22ScatterGatherInterlockC1EPVNS0_16_AtomicInterlockEjPvPFiS5_jE", cellDaisyScatterGatherInterlock_2);
	REG_FNID(cellDaisy, "_QN4cell5Daisy22ScatterGatherInterlockC2EPVNS0_16_AtomicInterlockEjPvPFiS5_jE", cellDaisyScatterGatherInterlock_2);
	REG_FNID(cellDaisy, "_ZN4cell5Daisy22ScatterGatherInterlockC2EPVNS0_16_AtomicInterlockEjPvPFiS5_jE", cellDaisyScatterGatherInterlock_2);
	REG_FNID(cellDaisy, "_ZN4cell5Daisy22ScatterGatherInterlockD2Ev", cellDaisyScatterGatherInterlock_9tor);
	REG_FNID(cellDaisy, "_QN4cell5Daisy22ScatterGatherInterlockD2Ev", cellDaisyScatterGatherInterlock_9tor);
	REG_FNID(cellDaisy, "_ZN4cell5Daisy22ScatterGatherInterlockD1Ev", cellDaisyScatterGatherInterlock_9tor);
	REG_FNID(cellDaisy, "_QN4cell5Daisy22ScatterGatherInterlockD1Ev", cellDaisyScatterGatherInterlock_9tor);
	REG_FNID(cellDaisy, "_ZN4cell5Daisy22ScatterGatherInterlock7releaseEv", cellDaisyScatterGatherInterlock_release);
	REG_FNID(cellDaisy, "_QN4cell5Daisy22ScatterGatherInterlock7releaseEv", cellDaisyScatterGatherInterlock_release);
	REG_FNID(cellDaisy, "_ZN4cell5Daisy22ScatterGatherInterlock21proceedSequenceNumberEv", cellDaisyScatterGatherInterlock_proceedSequenceNumber);
	REG_FNID(cellDaisy, "_QN4cell5Daisy22ScatterGatherInterlock21proceedSequenceNumberEv", cellDaisyScatterGatherInterlock_proceedSequenceNumber);
	REG_FNID(cellDaisy, "_ZN4cell5Daisy22ScatterGatherInterlock5probeEj", cellDaisyScatterGatherInterlock_probe);
	REG_FNID(cellDaisy, "_QN4cell5Daisy22ScatterGatherInterlock5probeEj", cellDaisyScatterGatherInterlock_probe);
});
