#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

extern psv_log_base sceFios;

typedef s64 SceFiosOffset;
typedef s64 SceFiosSize;
typedef u8 SceFiosOpEvent;
typedef s32 SceFiosHandle;
typedef SceFiosHandle SceFiosOp;
typedef SceFiosHandle SceFiosFH;
typedef SceFiosHandle SceFiosDH;
typedef s64 SceFiosTime;
typedef s8 SceFiosPriority;
typedef SceFiosTime SceFiosTimeInterval;
typedef u64 SceFiosDate;
typedef s32 SceFiosOverlayID;

typedef vm::psv::ptr<s32(vm::psv::ptr<void> pContext, SceFiosOp op, SceFiosOpEvent event, s32 err)> SceFiosOpCallback;
typedef vm::psv::ptr<s32(vm::psv::ptr<const char> fmt, va_list ap)> SceFiosVprintfCallback;
typedef vm::psv::ptr<vm::psv::ptr<void>(vm::psv::ptr<void> dst, vm::psv::ptr<const void> src, u32 len)> SceFiosMemcpyCallback;

enum SceFiosWhence : s32
{
	SCE_FIOS_SEEK_SET = 0,
	SCE_FIOS_SEEK_CUR = 1,
	SCE_FIOS_SEEK_END = 2
};

struct SceFiosBuffer
{
	vm::psv::ptr<void> pPtr;
	u32 length;
};

struct SceFiosOpAttr
{
	SceFiosTime deadline;
	SceFiosOpCallback pCallback;
	vm::psv::ptr<void> pCallbackContext;
	s32 priority : 8;
	u32 opflags : 24;
	u32 userTag;
	vm::psv::ptr<void> userPtr;
	vm::psv::ptr<void> pReserved;
};

struct SceFiosDirEntry
{
	SceFiosOffset fileSize;
	u32 statFlags;
	u16 nameLength;
	u16 fullPathLength;
	u16 offsetToName;
	u16 reserved[3];
	char fullPath[1024];
};

struct SceFiosStat
{
	SceFiosOffset fileSize;
	SceFiosDate accessDate;
	SceFiosDate modificationDate;
	SceFiosDate creationDate;
	u32 statFlags;
	u32 reserved;
	s64 uid;
	s64 gid;
	s64 dev;
	s64 ino;
	s64 mode;
};

struct SceFiosOpenParams
{
	u32 openFlags;
	u32 reserved;
	SceFiosBuffer buffer;
};

struct SceFiosTuple
{
	SceFiosOffset offset;
	SceFiosSize size;
	char path[1024];
};

struct SceFiosParams
{
	u32 initialized : 1;
	u32 paramsSize : 14;
	u32 pathMax : 16;
	u32 profiling;
	SceFiosBuffer opStorage;
	SceFiosBuffer fhStorage;
	SceFiosBuffer dhStorage;
	SceFiosBuffer chunkStorage;
	SceFiosVprintfCallback pVprintf;
	SceFiosMemcpyCallback pMemcpy;
	s32 threadPriority[2];
	s32 threadAffinity[2];
};

struct SceFiosOverlay
{
	u8 type;
	u8 order;
	u8 reserved[10];
	SceFiosOverlayID id;
	char dst[292];
	char src[292];
};

typedef vm::psv::ptr<void()> SceFiosIOFilterCallback;

struct SceFiosPsarcDearchiverContext
{
	u32 sizeOfContext;
	u32 workBufferSize;
	vm::psv::ptr<void> pWorkBuffer;
	s32 reserved[4];
};

s32 sceFiosInitialize(vm::psv::ptr<const SceFiosParams> pParameters)
{
	throw __FUNCTION__;
}

void sceFiosTerminate()
{
	throw __FUNCTION__;
}

bool sceFiosIsInitialized(vm::psv::ptr<SceFiosParams> pOutParameters)
{
	throw __FUNCTION__;
}

void sceFiosUpdateParameters(vm::psv::ptr<const SceFiosParams> pParameters)
{
	throw __FUNCTION__;
}

void sceFiosSetGlobalDefaultOpAttr(vm::psv::ptr<const SceFiosOpAttr> pAttr)
{
	throw __FUNCTION__;
}

bool sceFiosGetGlobalDefaultOpAttr(vm::psv::ptr<SceFiosOpAttr> pOutAttr)
{
	throw __FUNCTION__;
}

void sceFiosSetThreadDefaultOpAttr(vm::psv::ptr<const SceFiosOpAttr> pAttr)
{
	throw __FUNCTION__;
}

bool sceFiosGetThreadDefaultOpAttr(vm::psv::ptr<SceFiosOpAttr> pOutAttr)
{
	throw __FUNCTION__;
}

void sceFiosGetDefaultOpAttr(vm::psv::ptr<SceFiosOpAttr> pOutAttr)
{
	throw __FUNCTION__;
}

void sceFiosSuspend()
{
	throw __FUNCTION__;
}

u32 sceFiosGetSuspendCount()
{
	throw __FUNCTION__;
}

bool sceFiosIsSuspended()
{
	throw __FUNCTION__;
}

void sceFiosResume()
{
	throw __FUNCTION__;
}

void sceFiosShutdownAndCancelOps()
{
	throw __FUNCTION__;
}

void sceFiosCancelAllOps()
{
	throw __FUNCTION__;
}

void sceFiosCloseAllFiles()
{
	throw __FUNCTION__;
}

bool sceFiosIsIdle()
{
	throw __FUNCTION__;
}

u32 sceFiosGetAllFHs(vm::psv::ptr<SceFiosFH> pOutArray, u32 arraySize)
{
	throw __FUNCTION__;
}

u32 sceFiosGetAllDHs(vm::psv::ptr<SceFiosDH> pOutArray, u32 arraySize)
{
	throw __FUNCTION__;
}

u32 sceFiosGetAllOps(vm::psv::ptr<SceFiosOp> pOutArray, u32 arraySize)
{
	throw __FUNCTION__;
}

bool sceFiosIsValidHandle(SceFiosHandle h)
{
	throw __FUNCTION__;
}

s32 sceFiosPathcmp(vm::psv::ptr<const char> pA, vm::psv::ptr<const char> pB)
{
	throw __FUNCTION__;
}

s32 sceFiosPathncmp(vm::psv::ptr<const char> pA, vm::psv::ptr<const char> pB, u32 n)
{
	throw __FUNCTION__;
}

s32 sceFiosPrintf(vm::psv::ptr<const char> pFormat) // va_args...
{
	throw __FUNCTION__;
}

s32 sceFiosVprintf(vm::psv::ptr<const char> pFormat) // va_list
{
	throw __FUNCTION__;
}

SceFiosOp sceFiosFileExists(vm::psv::ptr<const SceFiosOpAttr> pAttr, vm::psv::ptr<const char> pPath, vm::psv::ptr<bool> pOutExists)
{
	throw __FUNCTION__;
}

bool sceFiosFileExistsSync(vm::psv::ptr<const SceFiosOpAttr> pAttr, vm::psv::ptr<const char> pPath)
{
	throw __FUNCTION__;
}

SceFiosOp sceFiosFileGetSize(vm::psv::ptr<const SceFiosOpAttr> pAttr, vm::psv::ptr<const char> pPath, vm::psv::ptr<SceFiosSize> pOutSize)
{
	throw __FUNCTION__;
}

SceFiosSize sceFiosFileGetSizeSync(vm::psv::ptr<const SceFiosOpAttr> pAttr, vm::psv::ptr<const char> pPath)
{
	throw __FUNCTION__;
}

SceFiosOp sceFiosFileDelete(vm::psv::ptr<const SceFiosOpAttr> pAttr, vm::psv::ptr<const char> pPath)
{
	throw __FUNCTION__;
}

s32 sceFiosFileDeleteSync(vm::psv::ptr<const SceFiosOpAttr> pAttr, vm::psv::ptr<const char> pPath)
{
	throw __FUNCTION__;
}

SceFiosOp sceFiosDirectoryExists(vm::psv::ptr<const SceFiosOpAttr> pAttr, vm::psv::ptr<const char> pPath, vm::psv::ptr<bool> pOutExists)
{
	throw __FUNCTION__;
}

bool sceFiosDirectoryExistsSync(vm::psv::ptr<const SceFiosOpAttr> pAttr, vm::psv::ptr<const char> pPath)
{
	throw __FUNCTION__;
}

SceFiosOp sceFiosDirectoryCreate(vm::psv::ptr<const SceFiosOpAttr> pAttr, vm::psv::ptr<const char> pPath)
{
	throw __FUNCTION__;
}

s32 sceFiosDirectoryCreateSync(vm::psv::ptr<const SceFiosOpAttr> pAttr, vm::psv::ptr<const char> pPath)
{
	throw __FUNCTION__;
}

SceFiosOp sceFiosDirectoryDelete(vm::psv::ptr<const SceFiosOpAttr> pAttr, vm::psv::ptr<const char> pPath)
{
	throw __FUNCTION__;
}

s32 sceFiosDirectoryDeleteSync(vm::psv::ptr<const SceFiosOpAttr> pAttr, vm::psv::ptr<const char> pPath)
{
	throw __FUNCTION__;
}

SceFiosOp sceFiosExists(vm::psv::ptr<const SceFiosOpAttr> pAttr, vm::psv::ptr<const char> pPath, vm::psv::ptr<bool> pOutExists)
{
	throw __FUNCTION__;
}

bool sceFiosExistsSync(vm::psv::ptr<const SceFiosOpAttr> pAttr, vm::psv::ptr<const char> pPath)
{
	throw __FUNCTION__;
}

SceFiosOp sceFiosStat(vm::psv::ptr<const SceFiosOpAttr> pAttr, vm::psv::ptr<const char> pPath, vm::psv::ptr<SceFiosStat> pOutStatus)
{
	throw __FUNCTION__;
}

s32 sceFiosStatSync(vm::psv::ptr<const SceFiosOpAttr> pAttr, vm::psv::ptr<const char> pPath, vm::psv::ptr<SceFiosStat> pOutStatus)
{
	throw __FUNCTION__;
}

SceFiosOp sceFiosDelete(vm::psv::ptr<const SceFiosOpAttr> pAttr, vm::psv::ptr<const char> pPath)
{
	throw __FUNCTION__;
}

s32 sceFiosDeleteSync(vm::psv::ptr<const SceFiosOpAttr> pAttr, vm::psv::ptr<const char> pPath)
{
	throw __FUNCTION__;
}

SceFiosOp sceFiosResolve(vm::psv::ptr<const SceFiosOpAttr> pAttr, vm::psv::ptr<const SceFiosTuple> pInTuple, vm::psv::ptr<SceFiosTuple> pOutTuple)
{
	throw __FUNCTION__;
}

s32 sceFiosResolveSync(vm::psv::ptr<const SceFiosOpAttr> pAttr, vm::psv::ptr<const SceFiosTuple> pInTuple, vm::psv::ptr<SceFiosTuple> pOutTuple)
{
	throw __FUNCTION__;
}

SceFiosOp sceFiosRename(vm::psv::ptr<const SceFiosOpAttr> pAttr, vm::psv::ptr<const char> pOldPath, vm::psv::ptr<const char> pNewPath)
{
	throw __FUNCTION__;
}

s32 sceFiosRenameSync(vm::psv::ptr<const SceFiosOpAttr> pAttr, vm::psv::ptr<const char> pOldPath, vm::psv::ptr<const char> pNewPath)
{
	throw __FUNCTION__;
}

SceFiosOp sceFiosFileRead(vm::psv::ptr<const SceFiosOpAttr> pAttr, vm::psv::ptr<const char> pPath, vm::psv::ptr<void> pBuf, SceFiosSize length, SceFiosOffset offset)
{
	throw __FUNCTION__;
}

SceFiosSize sceFiosFileReadSync(vm::psv::ptr<const SceFiosOpAttr> pAttr, vm::psv::ptr<const char> pPath, vm::psv::ptr<void> pBuf, SceFiosSize length, SceFiosOffset offset)
{
	throw __FUNCTION__;
}

SceFiosOp sceFiosFileWrite(vm::psv::ptr<const SceFiosOpAttr> pAttr, vm::psv::ptr<const char> pPath, vm::psv::ptr<const void> pBuf, SceFiosSize length, SceFiosOffset offset)
{
	throw __FUNCTION__;
}

SceFiosSize sceFiosFileWriteSync(vm::psv::ptr<const SceFiosOpAttr> pAttr, vm::psv::ptr<const char> pPath, vm::psv::ptr<const void> pBuf, SceFiosSize length, SceFiosOffset offset)
{
	throw __FUNCTION__;
}

SceFiosOp sceFiosFileTruncate(vm::psv::ptr<const SceFiosOpAttr> pAttr, vm::psv::ptr<const char> pPath, SceFiosSize length)
{
	throw __FUNCTION__;
}

s32 sceFiosFileTruncateSync(vm::psv::ptr<const SceFiosOpAttr> pAttr, vm::psv::ptr<const char> pPath, SceFiosSize length)
{
	throw __FUNCTION__;
}

SceFiosOp sceFiosFHOpen(vm::psv::ptr<const SceFiosOpAttr> pAttr, vm::psv::ptr<SceFiosFH> pOutFH, vm::psv::ptr<const char> pPath, vm::psv::ptr<const SceFiosOpenParams> pOpenParams)
{
	throw __FUNCTION__;
}

s32 sceFiosFHOpenSync(vm::psv::ptr<const SceFiosOpAttr> pAttr, vm::psv::ptr<SceFiosFH> pOutFH, vm::psv::ptr<const char> pPath, vm::psv::ptr<const SceFiosOpenParams> pOpenParams)
{
	throw __FUNCTION__;
}

SceFiosOp sceFiosFHStat(vm::psv::ptr<const SceFiosOpAttr> pAttr, SceFiosFH fh, vm::psv::ptr<SceFiosStat> pOutStatus)
{
	throw __FUNCTION__;
}

s32 sceFiosFHStatSync(vm::psv::ptr<const SceFiosOpAttr> pAttr, SceFiosFH fh, vm::psv::ptr<SceFiosStat> pOutStatus)
{
	throw __FUNCTION__;
}

SceFiosOp sceFiosFHTruncate(vm::psv::ptr<const SceFiosOpAttr> pAttr, SceFiosFH fh, SceFiosSize length)
{
	throw __FUNCTION__;
}

s32 sceFiosFHTruncateSync(vm::psv::ptr<const SceFiosOpAttr> pAttr, SceFiosFH fh, SceFiosSize length)
{
	throw __FUNCTION__;
}

SceFiosOp sceFiosFHSync(vm::psv::ptr<const SceFiosOpAttr> pAttr, SceFiosFH fh)
{
	throw __FUNCTION__;
}

s32 sceFiosFHSyncSync(vm::psv::ptr<const SceFiosOpAttr> pAttr, SceFiosFH fh)
{
	throw __FUNCTION__;
}

SceFiosOp sceFiosFHRead(vm::psv::ptr<const SceFiosOpAttr> pAttr, SceFiosFH fh, vm::psv::ptr<void> pBuf, SceFiosSize length)
{
	throw __FUNCTION__;
}

SceFiosSize sceFiosFHReadSync(vm::psv::ptr<const SceFiosOpAttr> pAttr, SceFiosFH fh, vm::psv::ptr<void> pBuf, SceFiosSize length)
{
	throw __FUNCTION__;
}

SceFiosOp sceFiosFHWrite(vm::psv::ptr<const SceFiosOpAttr> pAttr, SceFiosFH fh, vm::psv::ptr<const void> pBuf, SceFiosSize length)
{
	throw __FUNCTION__;
}

SceFiosSize sceFiosFHWriteSync(vm::psv::ptr<const SceFiosOpAttr> pAttr, SceFiosFH fh, vm::psv::ptr<const void> pBuf, SceFiosSize length)
{
	throw __FUNCTION__;
}

SceFiosOp sceFiosFHReadv(vm::psv::ptr<const SceFiosOpAttr> pAttr, SceFiosFH fh, vm::psv::ptr<const SceFiosBuffer> iov, s32 iovcnt)
{
	throw __FUNCTION__;
}

SceFiosSize sceFiosFHReadvSync(vm::psv::ptr<const SceFiosOpAttr> pAttr, SceFiosFH fh, vm::psv::ptr<const SceFiosBuffer> iov, s32 iovcnt)
{
	throw __FUNCTION__;
}

SceFiosOp sceFiosFHWritev(vm::psv::ptr<const SceFiosOpAttr> pAttr, SceFiosFH fh, vm::psv::ptr<const SceFiosBuffer> iov, s32 iovcnt)
{
	throw __FUNCTION__;
}

SceFiosSize sceFiosFHWritevSync(vm::psv::ptr<const SceFiosOpAttr> pAttr, SceFiosFH fh, vm::psv::ptr<const SceFiosBuffer> iov, s32 iovcnt)
{
	throw __FUNCTION__;
}

SceFiosOp sceFiosFHPread(vm::psv::ptr<const SceFiosOpAttr> pAttr, SceFiosFH fh, vm::psv::ptr<void> pBuf, SceFiosSize length, SceFiosOffset offset)
{
	throw __FUNCTION__;
}

SceFiosSize sceFiosFHPreadSync(vm::psv::ptr<const SceFiosOpAttr> pAttr, SceFiosFH fh, vm::psv::ptr<void> pBuf, SceFiosSize length, SceFiosOffset offset)
{
	throw __FUNCTION__;
}

SceFiosOp sceFiosFHPwrite(vm::psv::ptr<const SceFiosOpAttr> pAttr, SceFiosFH fh, vm::psv::ptr<const void> pBuf, SceFiosSize length, SceFiosOffset offset)
{
	throw __FUNCTION__;
}

SceFiosSize sceFiosFHPwriteSync(vm::psv::ptr<const SceFiosOpAttr> pAttr, SceFiosFH fh, vm::psv::ptr<const void> pBuf, SceFiosSize length, SceFiosOffset offset)
{
	throw __FUNCTION__;
}

SceFiosOp sceFiosFHPreadv(vm::psv::ptr<const SceFiosOpAttr> pAttr, SceFiosFH fh, vm::psv::ptr<const SceFiosBuffer> iov, s32 iovcnt, SceFiosOffset offset)
{
	throw __FUNCTION__;
}

SceFiosSize sceFiosFHPreadvSync(vm::psv::ptr<const SceFiosOpAttr> pAttr, SceFiosFH fh, vm::psv::ptr<const SceFiosBuffer> iov, s32 iovcnt, SceFiosOffset offset)
{
	throw __FUNCTION__;
}

SceFiosOp sceFiosFHPwritev(vm::psv::ptr<const SceFiosOpAttr> pAttr, SceFiosFH fh, vm::psv::ptr<const SceFiosBuffer> iov, s32 iovcnt, SceFiosOffset offset)
{
	throw __FUNCTION__;
}

SceFiosSize sceFiosFHPwritevSync(vm::psv::ptr<const SceFiosOpAttr> pAttr, SceFiosFH fh, vm::psv::ptr<const SceFiosBuffer> iov, s32 iovcnt, SceFiosOffset offset)
{
	throw __FUNCTION__;
}

SceFiosOp sceFiosFHClose(vm::psv::ptr<const SceFiosOpAttr> pAttr, SceFiosFH fh)
{
	throw __FUNCTION__;
}

s32 sceFiosFHCloseSync(vm::psv::ptr<const SceFiosOpAttr> pAttr, SceFiosFH fh)
{
	throw __FUNCTION__;
}

SceFiosOffset sceFiosFHSeek(SceFiosFH fh, SceFiosOffset offset, SceFiosWhence whence)
{
	throw __FUNCTION__;
}

SceFiosOffset sceFiosFHTell(SceFiosFH fh)
{
	throw __FUNCTION__;
}

vm::psv::ptr<const char> sceFiosFHGetPath(SceFiosFH fh)
{
	throw __FUNCTION__;
}

SceFiosSize sceFiosFHGetSize(SceFiosFH fh)
{
	throw __FUNCTION__;
}

vm::psv::ptr<SceFiosOpenParams> sceFiosFHGetOpenParams(SceFiosFH fh)
{
	throw __FUNCTION__;
}

//SceFiosOp sceFiosDHOpen(vm::psv::ptr<const SceFiosOpAttr> pAttr, vm::psv::ptr<SceFiosDH> pOutDH, vm::psv::ptr<const char> pPath, SceFiosBuffer buf)
//{
//	throw __FUNCTION__;
//}
//
//s32 sceFiosDHOpenSync(vm::psv::ptr<const SceFiosOpAttr> pAttr, vm::psv::ptr<SceFiosDH> pOutDH, vm::psv::ptr<const char> pPath, SceFiosBuffer buf)
//{
//	throw __FUNCTION__;
//}

SceFiosOp sceFiosDHRead(vm::psv::ptr<const SceFiosOpAttr> pAttr, SceFiosDH dh, vm::psv::ptr<SceFiosDirEntry> pOutEntry)
{
	throw __FUNCTION__;
}

s32 sceFiosDHReadSync(vm::psv::ptr<const SceFiosOpAttr> pAttr, SceFiosDH dh, vm::psv::ptr<SceFiosDirEntry> pOutEntry)
{
	throw __FUNCTION__;
}

SceFiosOp sceFiosDHClose(vm::psv::ptr<const SceFiosOpAttr> pAttr, SceFiosDH dh)
{
	throw __FUNCTION__;
}

s32 sceFiosDHCloseSync(vm::psv::ptr<const SceFiosOpAttr> pAttr, SceFiosDH dh)
{
	throw __FUNCTION__;
}

vm::psv::ptr<const char> sceFiosDHGetPath(SceFiosDH dh)
{
	throw __FUNCTION__;
}

bool sceFiosOpIsDone(SceFiosOp op)
{
	throw __FUNCTION__;
}

s32 sceFiosOpWait(SceFiosOp op)
{
	throw __FUNCTION__;
}

s32 sceFiosOpWaitUntil(SceFiosOp op, SceFiosTime deadline)
{
	throw __FUNCTION__;
}

void sceFiosOpDelete(SceFiosOp op)
{
	throw __FUNCTION__;
}

s32 sceFiosOpSyncWait(SceFiosOp op)
{
	throw __FUNCTION__;
}

SceFiosSize sceFiosOpSyncWaitForIO(SceFiosOp op)
{
	throw __FUNCTION__;
}

s32 sceFiosOpGetError(SceFiosOp op)
{
	throw __FUNCTION__;
}

void sceFiosOpCancel(SceFiosOp op)
{
	throw __FUNCTION__;
}

bool sceFiosOpIsCancelled(SceFiosOp op)
{
	throw __FUNCTION__;
}

vm::psv::ptr<const SceFiosOpAttr> sceFiosOpGetAttr(SceFiosOp op)
{
	throw __FUNCTION__;
}

vm::psv::ptr<const char> sceFiosOpGetPath(SceFiosOp op)
{
	throw __FUNCTION__;
}

vm::psv::ptr<void> sceFiosOpGetBuffer(SceFiosOp op)
{
	throw __FUNCTION__;
}

SceFiosOffset sceFiosOpGetOffset(SceFiosOp op)
{
	throw __FUNCTION__;
}

SceFiosSize sceFiosOpGetRequestCount(SceFiosOp op)
{
	throw __FUNCTION__;
}

SceFiosSize sceFiosOpGetActualCount(SceFiosOp op)
{
	throw __FUNCTION__;
}

void sceFiosOpReschedule(SceFiosOp op, SceFiosTime newDeadline)
{
	throw __FUNCTION__;
}

SceFiosTime sceFiosTimeGetCurrent()
{
	throw __FUNCTION__;
}

s64 sceFiosTimeIntervalToNanoseconds(SceFiosTimeInterval interval)
{
	throw __FUNCTION__;
}

SceFiosTimeInterval sceFiosTimeIntervalFromNanoseconds(s64 ns)
{
	throw __FUNCTION__;
}

SceFiosDate sceFiosDateGetCurrent()
{
	throw __FUNCTION__;
}

SceFiosDate sceFiosDateFromComponents(vm::psv::ptr<const tm> pComponents)
{
	throw __FUNCTION__;
}

vm::psv::ptr<tm> sceFiosDateToComponents(SceFiosDate date, vm::psv::ptr<tm> pOutComponents)
{
	throw __FUNCTION__;
}

SceFiosDate sceFiosDateFromSceDateTime(vm::psv::ptr<const SceDateTime> pSceDateTime)
{
	throw __FUNCTION__;
}

vm::psv::ptr<SceDateTime> sceFiosDateToSceDateTime(SceFiosDate date, vm::psv::ptr<SceDateTime> pSceDateTime)
{
	throw __FUNCTION__;
}

s32 sceFiosOverlayAdd(vm::psv::ptr<const SceFiosOverlay> pOverlay, vm::psv::ptr<SceFiosOverlayID> pOutID)
{
	throw __FUNCTION__;
}

s32 sceFiosOverlayRemove(SceFiosOverlayID id)
{
	throw __FUNCTION__;
}

s32 sceFiosOverlayGetInfo(SceFiosOverlayID id, vm::psv::ptr<SceFiosOverlay> pOutOverlay)
{
	throw __FUNCTION__;
}

s32 sceFiosOverlayModify(SceFiosOverlayID id, vm::psv::ptr<const SceFiosOverlay> pNewValue)
{
	throw __FUNCTION__;
}

s32 sceFiosOverlayGetList(vm::psv::ptr<SceFiosOverlayID> pOutIDs, u32 maxIDs, vm::psv::ptr<u32> pActualIDs)
{
	throw __FUNCTION__;
}

s32 sceFiosOverlayResolveSync(s32 resolveFlag, vm::psv::ptr<const char> pInPath, vm::psv::ptr<char> pOutPath, u32 maxPath)
{
	throw __FUNCTION__;
}

SceFiosOp sceFiosArchiveGetMountBufferSize(vm::psv::ptr<const SceFiosOpAttr> pAttr, vm::psv::ptr<const char> pArchivePath, vm::psv::ptr<const SceFiosOpenParams> pOpenParams)
{
	throw __FUNCTION__;
}

SceFiosSize sceFiosArchiveGetMountBufferSizeSync(vm::psv::ptr<const SceFiosOpAttr> pAttr, vm::psv::ptr<const char> pArchivePath, vm::psv::ptr<const SceFiosOpenParams> pOpenParams)
{
	throw __FUNCTION__;
}

//SceFiosOp sceFiosArchiveMount(vm::psv::ptr<const SceFiosOpAttr> pAttr, vm::psv::ptr<SceFiosFH> pOutFH, vm::psv::ptr<const char> pArchivePath, vm::psv::ptr<const char> pMountPoint, SceFiosBuffer mountBuffer, vm::psv::ptr<const SceFiosOpenParams> pOpenParams)
//{
//	throw __FUNCTION__;
//}
//
//s32 sceFiosArchiveMountSync(vm::psv::ptr<const SceFiosOpAttr> pAttr, vm::psv::ptr<SceFiosFH> pOutFH, vm::psv::ptr<const char> pArchivePath, vm::psv::ptr<const char> pMountPoint, SceFiosBuffer mountBuffer, vm::psv::ptr<const SceFiosOpenParams> pOpenParams)
//{
//	throw __FUNCTION__;
//}

SceFiosOp sceFiosArchiveUnmount(vm::psv::ptr<const SceFiosOpAttr> pAttr, SceFiosFH fh)
{
	throw __FUNCTION__;
}

s32 sceFiosArchiveUnmountSync(vm::psv::ptr<const SceFiosOpAttr> pAttr, SceFiosFH fh)
{
	throw __FUNCTION__;
}

vm::psv::ptr<char> sceFiosDebugDumpError(s32 err, vm::psv::ptr<char> pBuffer, u32 bufferSize)
{
	throw __FUNCTION__;
}

vm::psv::ptr<char> sceFiosDebugDumpOp(SceFiosOp op, vm::psv::ptr<char> pBuffer, u32 bufferSize)
{
	throw __FUNCTION__;
}

vm::psv::ptr<char> sceFiosDebugDumpFH(SceFiosFH fh, vm::psv::ptr<char> pBuffer, u32 bufferSize)
{
	throw __FUNCTION__;
}

vm::psv::ptr<char> sceFiosDebugDumpDH(SceFiosDH dh, vm::psv::ptr<char> pBuffer, u32 bufferSize)
{
	throw __FUNCTION__;
}

vm::psv::ptr<char> sceFiosDebugDumpDate(SceFiosDate date, vm::psv::ptr<char> pBuffer, u32 bufferSize)
{
	throw __FUNCTION__;
}

s32 sceFiosIOFilterAdd(s32 index, SceFiosIOFilterCallback pFilterCallback, vm::psv::ptr<void> pFilterContext)
{
	throw __FUNCTION__;
}

s32 sceFiosIOFilterGetInfo(s32 index, vm::psv::ptr<SceFiosIOFilterCallback> pOutFilterCallback, vm::psv::ptr<vm::psv::ptr<void>> pOutFilterContext)
{
	throw __FUNCTION__;
}

s32 sceFiosIOFilterRemove(s32 index)
{
	throw __FUNCTION__;
}

void sceFiosIOFilterPsarcDearchiver()
{
	throw __FUNCTION__;
}

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceFios, #name, name)

psv_log_base sceFios("SceFios2", []()
{
	sceFios.on_load = nullptr;
	sceFios.on_unload = nullptr;
	sceFios.on_stop = nullptr;

	REG_FUNC(0x15857180, sceFiosArchiveGetMountBufferSize);
	REG_FUNC(0xDF3352FC, sceFiosArchiveGetMountBufferSizeSync);
	//REG_FUNC(0x92E76BBD, sceFiosArchiveMount);
	//REG_FUNC(0xC4822276, sceFiosArchiveMountSync);
	REG_FUNC(0xFE1E1D28, sceFiosArchiveUnmount);
	REG_FUNC(0xB26DC24D, sceFiosArchiveUnmountSync);
	REG_FUNC(0x1E920B1D, sceFiosCancelAllOps);
	REG_FUNC(0xF85C208B, sceFiosCloseAllFiles);
	REG_FUNC(0xF6CACFC7, sceFiosDHClose);
	REG_FUNC(0x1F3CC428, sceFiosDHCloseSync);
	REG_FUNC(0x2B406DEB, sceFiosDHGetPath);
	//REG_FUNC(0xEA9855BA, sceFiosDHOpen);
	//REG_FUNC(0x34BC3713, sceFiosDHOpenSync);
	REG_FUNC(0x72A0A851, sceFiosDHRead);
	REG_FUNC(0xB7E79CAD, sceFiosDHReadSync);
	REG_FUNC(0x280D284A, sceFiosDateFromComponents);
	REG_FUNC(0x5C593C1E, sceFiosDateGetCurrent);
	REG_FUNC(0x5CFF6EA0, sceFiosDateToComponents);
	REG_FUNC(0x44B9F8EB, sceFiosDebugDumpDH);
	REG_FUNC(0x159B1FA8, sceFiosDebugDumpDate);
	REG_FUNC(0x51E677DF, sceFiosDebugDumpError);
	REG_FUNC(0x5506ACAB, sceFiosDebugDumpFH);
	REG_FUNC(0xE438D4F0, sceFiosDebugDumpOp);
	REG_FUNC(0x764DFA7A, sceFiosDelete);
	REG_FUNC(0xAAC54B44, sceFiosDeleteSync);
	REG_FUNC(0x9198ED8B, sceFiosDirectoryCreate);
	REG_FUNC(0xE037B076, sceFiosDirectoryCreateSync);
	REG_FUNC(0xDA93677C, sceFiosDirectoryDelete);
	REG_FUNC(0xB9573146, sceFiosDirectoryDeleteSync);
	REG_FUNC(0x48D50D97, sceFiosDirectoryExists);
	REG_FUNC(0x726E01BE, sceFiosDirectoryExistsSync);
	REG_FUNC(0x6F12D8A5, sceFiosExists);
	REG_FUNC(0x125EFD34, sceFiosExistsSync);
	REG_FUNC(0xA88EDCA8, sceFiosFHClose);
	REG_FUNC(0x45182328, sceFiosFHCloseSync);
	REG_FUNC(0xC55DB73B, sceFiosFHGetOpenParams);
	REG_FUNC(0x37143AE3, sceFiosFHGetPath);
	REG_FUNC(0xC5C26581, sceFiosFHGetSize);
	REG_FUNC(0xBF699BD4, sceFiosFHOpen);
	REG_FUNC(0xC3E7C3DB, sceFiosFHOpenSync);
	REG_FUNC(0x6A51E688, sceFiosFHPread);
	REG_FUNC(0xE2805059, sceFiosFHPreadSync);
	REG_FUNC(0x7C4E0C42, sceFiosFHPreadv);
	REG_FUNC(0x4D42F95C, sceFiosFHPreadvSync);
	REG_FUNC(0xCF1FAA6F, sceFiosFHPwrite);
	REG_FUNC(0x1E962F57, sceFiosFHPwriteSync);
	REG_FUNC(0xBBC9AFD5, sceFiosFHPwritev);
	REG_FUNC(0x742ADDC4, sceFiosFHPwritevSync);
	REG_FUNC(0xB09AFBDF, sceFiosFHRead);
	REG_FUNC(0x76945919, sceFiosFHReadSync);
	REG_FUNC(0x7DB0AFAF, sceFiosFHReadv);
	REG_FUNC(0x1BC977FA, sceFiosFHReadvSync);
	REG_FUNC(0xA75F3C4A, sceFiosFHSeek);
	REG_FUNC(0xD97C4DF7, sceFiosFHStat);
	REG_FUNC(0xF8BEAC88, sceFiosFHStatSync);
	REG_FUNC(0xE485F35E, sceFiosFHSync);
	REG_FUNC(0xA909CCE3, sceFiosFHSyncSync);
	REG_FUNC(0xD7F33130, sceFiosFHTell);
	REG_FUNC(0x2B39453B, sceFiosFHTruncate);
	REG_FUNC(0xFEF940B7, sceFiosFHTruncateSync);
	REG_FUNC(0xE663138E, sceFiosFHWrite);
	REG_FUNC(0x984024E5, sceFiosFHWriteSync);
	REG_FUNC(0x988DD7FF, sceFiosFHWritev);
	REG_FUNC(0x267E6CE3, sceFiosFHWritevSync);
	REG_FUNC(0xB647278B, sceFiosFileDelete);
	REG_FUNC(0xB5302E30, sceFiosFileDeleteSync);
	REG_FUNC(0x8758E62F, sceFiosFileExists);
	REG_FUNC(0x233B070C, sceFiosFileExistsSync);
	REG_FUNC(0x79D9BB50, sceFiosFileGetSize);
	REG_FUNC(0x789215C3, sceFiosFileGetSizeSync);
	REG_FUNC(0x84080161, sceFiosFileRead);
	REG_FUNC(0x1C488B32, sceFiosFileReadSync);
	REG_FUNC(0xC5513E13, sceFiosFileTruncate);
	REG_FUNC(0x6E1252B8, sceFiosFileTruncateSync);
	REG_FUNC(0x42C278E5, sceFiosFileWrite);
	REG_FUNC(0x132B6DE6, sceFiosFileWriteSync);
	REG_FUNC(0x681184A2, sceFiosGetAllDHs);
	REG_FUNC(0x90AB9195, sceFiosGetAllFHs);
	REG_FUNC(0x8F62832C, sceFiosGetAllOps);
	REG_FUNC(0xC897F6A7, sceFiosGetDefaultOpAttr);
	REG_FUNC(0x30583FCB, sceFiosGetGlobalDefaultOpAttr);
	REG_FUNC(0x156EAFDC, sceFiosGetSuspendCount);
	REG_FUNC(0xD55B8555, sceFiosIOFilterAdd);
	REG_FUNC(0x7C9B14EB, sceFiosIOFilterGetInfo);
	REG_FUNC(0x057252F2, sceFiosIOFilterPsarcDearchiver);
	REG_FUNC(0x22E35018, sceFiosIOFilterRemove);
	REG_FUNC(0x774C2C05, sceFiosInitialize);
	REG_FUNC(0x29104BF3, sceFiosIsIdle);
	REG_FUNC(0xF4F54E09, sceFiosIsInitialized);
	REG_FUNC(0xD2466EA5, sceFiosIsSuspended);
	REG_FUNC(0xB309E327, sceFiosIsValidHandle);
	REG_FUNC(0x3904F205, sceFiosOpCancel);
	REG_FUNC(0xE4EA92FA, sceFiosOpDelete);
	REG_FUNC(0x218A43EE, sceFiosOpGetActualCount);
	REG_FUNC(0xABFEE706, sceFiosOpGetAttr);
	REG_FUNC(0x68C436E4, sceFiosOpGetBuffer);
	REG_FUNC(0xBF099E16, sceFiosOpGetError);
	REG_FUNC(0xF21213B9, sceFiosOpGetOffset);
	REG_FUNC(0x157515CB, sceFiosOpGetPath);
	REG_FUNC(0x9C1084C5, sceFiosOpGetRequestCount);
	REG_FUNC(0x0C81D80E, sceFiosOpIsCancelled);
	REG_FUNC(0x1B9A575E, sceFiosOpIsDone);
	REG_FUNC(0x968CADBD, sceFiosOpReschedule);
	REG_FUNC(0xE6A66C70, sceFiosOpSyncWait);
	REG_FUNC(0x202079F9, sceFiosOpSyncWaitForIO);
	REG_FUNC(0x2AC79DFC, sceFiosOpWait);
	REG_FUNC(0xCC823B47, sceFiosOpWaitUntil);
	REG_FUNC(0x27AE468B, sceFiosOverlayAdd);
	REG_FUNC(0xF4C6B72A, sceFiosOverlayGetInfo);
	REG_FUNC(0x1C0BCAD5, sceFiosOverlayGetList);
	REG_FUNC(0x30F56704, sceFiosOverlayModify);
	REG_FUNC(0xF3C84D0F, sceFiosOverlayRemove);
	REG_FUNC(0x8A243E74, sceFiosOverlayResolveSync);
	REG_FUNC(0x5E75937A, sceFiosPathcmp);
	REG_FUNC(0xCC21C849, sceFiosPathncmp);
	REG_FUNC(0xAF7FAADF, sceFiosPrintf);
	REG_FUNC(0x25E399E5, sceFiosRename);
	REG_FUNC(0x030306F4, sceFiosRenameSync);
	REG_FUNC(0xD0B19C9F, sceFiosResolve);
	REG_FUNC(0x7FF33797, sceFiosResolveSync);
	REG_FUNC(0xBF2D3CC1, sceFiosResume);
	REG_FUNC(0x4E2FD311, sceFiosSetGlobalDefaultOpAttr);
	REG_FUNC(0x5B8D48C4, sceFiosShutdownAndCancelOps);
	REG_FUNC(0xFF04AF72, sceFiosStat);
	REG_FUNC(0xACBAF3E0, sceFiosStatSync);
	REG_FUNC(0x510953DC, sceFiosSuspend);
	REG_FUNC(0x2904B539, sceFiosTerminate);
	REG_FUNC(0xE76C8EC3, sceFiosTimeGetCurrent);
	REG_FUNC(0x35A82737, sceFiosTimeIntervalFromNanoseconds);
	REG_FUNC(0x397BF626, sceFiosTimeIntervalToNanoseconds);
	REG_FUNC(0x1915052A, sceFiosUpdateParameters);
	REG_FUNC(0x5BA4BD6D, sceFiosVprintf);
});
