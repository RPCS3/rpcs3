#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/ARMv7/PSVFuncList.h"

#include "sceFios.h"

s32 sceFiosInitialize(vm::cptr<SceFiosParams> pParameters)
{
	throw EXCEPTION("");
}

void sceFiosTerminate()
{
	throw EXCEPTION("");
}

bool sceFiosIsInitialized(vm::ptr<SceFiosParams> pOutParameters)
{
	throw EXCEPTION("");
}

void sceFiosUpdateParameters(vm::cptr<SceFiosParams> pParameters)
{
	throw EXCEPTION("");
}

void sceFiosSetGlobalDefaultOpAttr(vm::cptr<SceFiosOpAttr> pAttr)
{
	throw EXCEPTION("");
}

bool sceFiosGetGlobalDefaultOpAttr(vm::ptr<SceFiosOpAttr> pOutAttr)
{
	throw EXCEPTION("");
}

void sceFiosSetThreadDefaultOpAttr(vm::cptr<SceFiosOpAttr> pAttr)
{
	throw EXCEPTION("");
}

bool sceFiosGetThreadDefaultOpAttr(vm::ptr<SceFiosOpAttr> pOutAttr)
{
	throw EXCEPTION("");
}

void sceFiosGetDefaultOpAttr(vm::ptr<SceFiosOpAttr> pOutAttr)
{
	throw EXCEPTION("");
}

void sceFiosSuspend()
{
	throw EXCEPTION("");
}

u32 sceFiosGetSuspendCount()
{
	throw EXCEPTION("");
}

bool sceFiosIsSuspended()
{
	throw EXCEPTION("");
}

void sceFiosResume()
{
	throw EXCEPTION("");
}

void sceFiosShutdownAndCancelOps()
{
	throw EXCEPTION("");
}

void sceFiosCancelAllOps()
{
	throw EXCEPTION("");
}

void sceFiosCloseAllFiles()
{
	throw EXCEPTION("");
}

bool sceFiosIsIdle()
{
	throw EXCEPTION("");
}

u32 sceFiosGetAllFHs(vm::ptr<s32> pOutArray, u32 arraySize)
{
	throw EXCEPTION("");
}

u32 sceFiosGetAllDHs(vm::ptr<s32> pOutArray, u32 arraySize)
{
	throw EXCEPTION("");
}

u32 sceFiosGetAllOps(vm::ptr<s32> pOutArray, u32 arraySize)
{
	throw EXCEPTION("");
}

bool sceFiosIsValidHandle(s32 h)
{
	throw EXCEPTION("");
}

s32 sceFiosPathcmp(vm::cptr<char> pA, vm::cptr<char> pB)
{
	throw EXCEPTION("");
}

s32 sceFiosPathncmp(vm::cptr<char> pA, vm::cptr<char> pB, u32 n)
{
	throw EXCEPTION("");
}

s32 sceFiosPrintf(vm::cptr<char> pFormat) // va_args...
{
	throw EXCEPTION("");
}

s32 sceFiosVprintf(vm::cptr<char> pFormat) // va_list
{
	throw EXCEPTION("");
}

s32 sceFiosFileExists(vm::cptr<SceFiosOpAttr> pAttr, vm::cptr<char> pPath, vm::ptr<bool> pOutExists)
{
	throw EXCEPTION("");
}

bool sceFiosFileExistsSync(vm::cptr<SceFiosOpAttr> pAttr, vm::cptr<char> pPath)
{
	throw EXCEPTION("");
}

s32 sceFiosFileGetSize(vm::cptr<SceFiosOpAttr> pAttr, vm::cptr<char> pPath, vm::ptr<s64> pOutSize)
{
	throw EXCEPTION("");
}

s64 sceFiosFileGetSizeSync(vm::cptr<SceFiosOpAttr> pAttr, vm::cptr<char> pPath)
{
	throw EXCEPTION("");
}

s32 sceFiosFileDelete(vm::cptr<SceFiosOpAttr> pAttr, vm::cptr<char> pPath)
{
	throw EXCEPTION("");
}

s32 sceFiosFileDeleteSync(vm::cptr<SceFiosOpAttr> pAttr, vm::cptr<char> pPath)
{
	throw EXCEPTION("");
}

s32 sceFiosDirectoryExists(vm::cptr<SceFiosOpAttr> pAttr, vm::cptr<char> pPath, vm::ptr<bool> pOutExists)
{
	throw EXCEPTION("");
}

bool sceFiosDirectoryExistsSync(vm::cptr<SceFiosOpAttr> pAttr, vm::cptr<char> pPath)
{
	throw EXCEPTION("");
}

s32 sceFiosDirectoryCreate(vm::cptr<SceFiosOpAttr> pAttr, vm::cptr<char> pPath)
{
	throw EXCEPTION("");
}

s32 sceFiosDirectoryCreateSync(vm::cptr<SceFiosOpAttr> pAttr, vm::cptr<char> pPath)
{
	throw EXCEPTION("");
}

s32 sceFiosDirectoryDelete(vm::cptr<SceFiosOpAttr> pAttr, vm::cptr<char> pPath)
{
	throw EXCEPTION("");
}

s32 sceFiosDirectoryDeleteSync(vm::cptr<SceFiosOpAttr> pAttr, vm::cptr<char> pPath)
{
	throw EXCEPTION("");
}

s32 sceFiosExists(vm::cptr<SceFiosOpAttr> pAttr, vm::cptr<char> pPath, vm::ptr<bool> pOutExists)
{
	throw EXCEPTION("");
}

bool sceFiosExistsSync(vm::cptr<SceFiosOpAttr> pAttr, vm::cptr<char> pPath)
{
	throw EXCEPTION("");
}

s32 sceFiosStat(vm::cptr<SceFiosOpAttr> pAttr, vm::cptr<char> pPath, vm::ptr<SceFiosStat> pOutStatus)
{
	throw EXCEPTION("");
}

s32 sceFiosStatSync(vm::cptr<SceFiosOpAttr> pAttr, vm::cptr<char> pPath, vm::ptr<SceFiosStat> pOutStatus)
{
	throw EXCEPTION("");
}

s32 sceFiosDelete(vm::cptr<SceFiosOpAttr> pAttr, vm::cptr<char> pPath)
{
	throw EXCEPTION("");
}

s32 sceFiosDeleteSync(vm::cptr<SceFiosOpAttr> pAttr, vm::cptr<char> pPath)
{
	throw EXCEPTION("");
}

s32 sceFiosResolve(vm::cptr<SceFiosOpAttr> pAttr, vm::cptr<SceFiosTuple> pInTuple, vm::ptr<SceFiosTuple> pOutTuple)
{
	throw EXCEPTION("");
}

s32 sceFiosResolveSync(vm::cptr<SceFiosOpAttr> pAttr, vm::cptr<SceFiosTuple> pInTuple, vm::ptr<SceFiosTuple> pOutTuple)
{
	throw EXCEPTION("");
}

s32 sceFiosRename(vm::cptr<SceFiosOpAttr> pAttr, vm::cptr<char> pOldPath, vm::cptr<char> pNewPath)
{
	throw EXCEPTION("");
}

s32 sceFiosRenameSync(vm::cptr<SceFiosOpAttr> pAttr, vm::cptr<char> pOldPath, vm::cptr<char> pNewPath)
{
	throw EXCEPTION("");
}

s32 sceFiosFileRead(vm::cptr<SceFiosOpAttr> pAttr, vm::cptr<char> pPath, vm::ptr<void> pBuf, s64 length, s64 offset)
{
	throw EXCEPTION("");
}

s64 sceFiosFileReadSync(vm::cptr<SceFiosOpAttr> pAttr, vm::cptr<char> pPath, vm::ptr<void> pBuf, s64 length, s64 offset)
{
	throw EXCEPTION("");
}

s32 sceFiosFileWrite(vm::cptr<SceFiosOpAttr> pAttr, vm::cptr<char> pPath, vm::cptr<void> pBuf, s64 length, s64 offset)
{
	throw EXCEPTION("");
}

s64 sceFiosFileWriteSync(vm::cptr<SceFiosOpAttr> pAttr, vm::cptr<char> pPath, vm::cptr<void> pBuf, s64 length, s64 offset)
{
	throw EXCEPTION("");
}

s32 sceFiosFileTruncate(vm::cptr<SceFiosOpAttr> pAttr, vm::cptr<char> pPath, s64 length)
{
	throw EXCEPTION("");
}

s32 sceFiosFileTruncateSync(vm::cptr<SceFiosOpAttr> pAttr, vm::cptr<char> pPath, s64 length)
{
	throw EXCEPTION("");
}

s32 sceFiosFHOpen(vm::cptr<SceFiosOpAttr> pAttr, vm::ptr<s32> pOutFH, vm::cptr<char> pPath, vm::cptr<SceFiosOpenParams> pOpenParams)
{
	throw EXCEPTION("");
}

s32 sceFiosFHOpenSync(vm::cptr<SceFiosOpAttr> pAttr, vm::ptr<s32> pOutFH, vm::cptr<char> pPath, vm::cptr<SceFiosOpenParams> pOpenParams)
{
	throw EXCEPTION("");
}

s32 sceFiosFHStat(vm::cptr<SceFiosOpAttr> pAttr, s32 fh, vm::ptr<SceFiosStat> pOutStatus)
{
	throw EXCEPTION("");
}

s32 sceFiosFHStatSync(vm::cptr<SceFiosOpAttr> pAttr, s32 fh, vm::ptr<SceFiosStat> pOutStatus)
{
	throw EXCEPTION("");
}

s32 sceFiosFHTruncate(vm::cptr<SceFiosOpAttr> pAttr, s32 fh, s64 length)
{
	throw EXCEPTION("");
}

s32 sceFiosFHTruncateSync(vm::cptr<SceFiosOpAttr> pAttr, s32 fh, s64 length)
{
	throw EXCEPTION("");
}

s32 sceFiosFHSync(vm::cptr<SceFiosOpAttr> pAttr, s32 fh)
{
	throw EXCEPTION("");
}

s32 sceFiosFHSyncSync(vm::cptr<SceFiosOpAttr> pAttr, s32 fh)
{
	throw EXCEPTION("");
}

s32 sceFiosFHRead(vm::cptr<SceFiosOpAttr> pAttr, s32 fh, vm::ptr<void> pBuf, s64 length)
{
	throw EXCEPTION("");
}

s64 sceFiosFHReadSync(vm::cptr<SceFiosOpAttr> pAttr, s32 fh, vm::ptr<void> pBuf, s64 length)
{
	throw EXCEPTION("");
}

s32 sceFiosFHWrite(vm::cptr<SceFiosOpAttr> pAttr, s32 fh, vm::cptr<void> pBuf, s64 length)
{
	throw EXCEPTION("");
}

s64 sceFiosFHWriteSync(vm::cptr<SceFiosOpAttr> pAttr, s32 fh, vm::cptr<void> pBuf, s64 length)
{
	throw EXCEPTION("");
}

s32 sceFiosFHReadv(vm::cptr<SceFiosOpAttr> pAttr, s32 fh, vm::cptr<SceFiosBuffer> iov, s32 iovcnt)
{
	throw EXCEPTION("");
}

s64 sceFiosFHReadvSync(vm::cptr<SceFiosOpAttr> pAttr, s32 fh, vm::cptr<SceFiosBuffer> iov, s32 iovcnt)
{
	throw EXCEPTION("");
}

s32 sceFiosFHWritev(vm::cptr<SceFiosOpAttr> pAttr, s32 fh, vm::cptr<SceFiosBuffer> iov, s32 iovcnt)
{
	throw EXCEPTION("");
}

s64 sceFiosFHWritevSync(vm::cptr<SceFiosOpAttr> pAttr, s32 fh, vm::cptr<SceFiosBuffer> iov, s32 iovcnt)
{
	throw EXCEPTION("");
}

s32 sceFiosFHPread(vm::cptr<SceFiosOpAttr> pAttr, s32 fh, vm::ptr<void> pBuf, s64 length, s64 offset)
{
	throw EXCEPTION("");
}

s64 sceFiosFHPreadSync(vm::cptr<SceFiosOpAttr> pAttr, s32 fh, vm::ptr<void> pBuf, s64 length, s64 offset)
{
	throw EXCEPTION("");
}

s32 sceFiosFHPwrite(vm::cptr<SceFiosOpAttr> pAttr, s32 fh, vm::cptr<void> pBuf, s64 length, s64 offset)
{
	throw EXCEPTION("");
}

s64 sceFiosFHPwriteSync(vm::cptr<SceFiosOpAttr> pAttr, s32 fh, vm::cptr<void> pBuf, s64 length, s64 offset)
{
	throw EXCEPTION("");
}

s32 sceFiosFHPreadv(vm::cptr<SceFiosOpAttr> pAttr, s32 fh, vm::cptr<SceFiosBuffer> iov, s32 iovcnt, s64 offset)
{
	throw EXCEPTION("");
}

s64 sceFiosFHPreadvSync(vm::cptr<SceFiosOpAttr> pAttr, s32 fh, vm::cptr<SceFiosBuffer> iov, s32 iovcnt, s64 offset)
{
	throw EXCEPTION("");
}

s32 sceFiosFHPwritev(vm::cptr<SceFiosOpAttr> pAttr, s32 fh, vm::cptr<SceFiosBuffer> iov, s32 iovcnt, s64 offset)
{
	throw EXCEPTION("");
}

s64 sceFiosFHPwritevSync(vm::cptr<SceFiosOpAttr> pAttr, s32 fh, vm::cptr<SceFiosBuffer> iov, s32 iovcnt, s64 offset)
{
	throw EXCEPTION("");
}

s32 sceFiosFHClose(vm::cptr<SceFiosOpAttr> pAttr, s32 fh)
{
	throw EXCEPTION("");
}

s32 sceFiosFHCloseSync(vm::cptr<SceFiosOpAttr> pAttr, s32 fh)
{
	throw EXCEPTION("");
}

s64 sceFiosFHSeek(s32 fh, s64 offset, SceFiosWhence whence)
{
	throw EXCEPTION("");
}

s64 sceFiosFHTell(s32 fh)
{
	throw EXCEPTION("");
}

vm::cptr<char> sceFiosFHGetPath(s32 fh)
{
	throw EXCEPTION("");
}

s64 sceFiosFHGetSize(s32 fh)
{
	throw EXCEPTION("");
}

vm::ptr<SceFiosOpenParams> sceFiosFHGetOpenParams(s32 fh)
{
	throw EXCEPTION("");
}

s32 sceFiosDHOpen(vm::cptr<SceFiosOpAttr> pAttr, vm::ptr<s32> pOutDH, vm::cptr<char> pPath, SceFiosBuffer buf)
{
	throw EXCEPTION("");
}

s32 sceFiosDHOpenSync(vm::cptr<SceFiosOpAttr> pAttr, vm::ptr<s32> pOutDH, vm::cptr<char> pPath, SceFiosBuffer buf)
{
	throw EXCEPTION("");
}

s32 sceFiosDHRead(vm::cptr<SceFiosOpAttr> pAttr, s32 dh, vm::ptr<SceFiosDirEntry> pOutEntry)
{
	throw EXCEPTION("");
}

s32 sceFiosDHReadSync(vm::cptr<SceFiosOpAttr> pAttr, s32 dh, vm::ptr<SceFiosDirEntry> pOutEntry)
{
	throw EXCEPTION("");
}

s32 sceFiosDHClose(vm::cptr<SceFiosOpAttr> pAttr, s32 dh)
{
	throw EXCEPTION("");
}

s32 sceFiosDHCloseSync(vm::cptr<SceFiosOpAttr> pAttr, s32 dh)
{
	throw EXCEPTION("");
}

vm::cptr<char> sceFiosDHGetPath(s32 dh)
{
	throw EXCEPTION("");
}

bool sceFiosOpIsDone(s32 op)
{
	throw EXCEPTION("");
}

s32 sceFiosOpWait(s32 op)
{
	throw EXCEPTION("");
}

s32 sceFiosOpWaitUntil(s32 op, s64 deadline)
{
	throw EXCEPTION("");
}

void sceFiosOpDelete(s32 op)
{
	throw EXCEPTION("");
}

s32 sceFiosOpSyncWait(s32 op)
{
	throw EXCEPTION("");
}

s64 sceFiosOpSyncWaitForIO(s32 op)
{
	throw EXCEPTION("");
}

s32 sceFiosOpGetError(s32 op)
{
	throw EXCEPTION("");
}

void sceFiosOpCancel(s32 op)
{
	throw EXCEPTION("");
}

bool sceFiosOpIsCancelled(s32 op)
{
	throw EXCEPTION("");
}

vm::cptr<SceFiosOpAttr> sceFiosOpGetAttr(s32 op)
{
	throw EXCEPTION("");
}

vm::cptr<char> sceFiosOpGetPath(s32 op)
{
	throw EXCEPTION("");
}

vm::ptr<void> sceFiosOpGetBuffer(s32 op)
{
	throw EXCEPTION("");
}

s64 sceFiosOpGetOffset(s32 op)
{
	throw EXCEPTION("");
}

s64 sceFiosOpGetRequestCount(s32 op)
{
	throw EXCEPTION("");
}

s64 sceFiosOpGetActualCount(s32 op)
{
	throw EXCEPTION("");
}

void sceFiosOpReschedule(s32 op, s64 newDeadline)
{
	throw EXCEPTION("");
}

s64 sceFiosTimeGetCurrent()
{
	throw EXCEPTION("");
}

s64 sceFiosTimeIntervalToNanoseconds(s64 interval)
{
	throw EXCEPTION("");
}

s64 sceFiosTimeIntervalFromNanoseconds(s64 ns)
{
	throw EXCEPTION("");
}

u64 sceFiosDateGetCurrent()
{
	throw EXCEPTION("");
}

u64 sceFiosDateFromComponents(vm::cptr<tm> pComponents)
{
	throw EXCEPTION("");
}

vm::ptr<tm> sceFiosDateToComponents(u64 date, vm::ptr<tm> pOutComponents)
{
	throw EXCEPTION("");
}

u64 sceFiosDateFromSceDateTime(vm::cptr<SceDateTime> pSceDateTime)
{
	throw EXCEPTION("");
}

vm::ptr<SceDateTime> sceFiosDateToSceDateTime(u64 date, vm::ptr<SceDateTime> pSceDateTime)
{
	throw EXCEPTION("");
}

s32 sceFiosOverlayAdd(vm::cptr<SceFiosOverlay> pOverlay, vm::ptr<s32> pOutID)
{
	throw EXCEPTION("");
}

s32 sceFiosOverlayRemove(s32 id)
{
	throw EXCEPTION("");
}

s32 sceFiosOverlayGetInfo(s32 id, vm::ptr<SceFiosOverlay> pOutOverlay)
{
	throw EXCEPTION("");
}

s32 sceFiosOverlayModify(s32 id, vm::cptr<SceFiosOverlay> pNewValue)
{
	throw EXCEPTION("");
}

s32 sceFiosOverlayGetList(vm::ptr<s32> pOutIDs, u32 maxIDs, vm::ptr<u32> pActualIDs)
{
	throw EXCEPTION("");
}

s32 sceFiosOverlayResolveSync(s32 resolveFlag, vm::cptr<char> pInPath, vm::ptr<char> pOutPath, u32 maxPath)
{
	throw EXCEPTION("");
}

s32 sceFiosArchiveGetMountBufferSize(vm::cptr<SceFiosOpAttr> pAttr, vm::cptr<char> pArchivePath, vm::cptr<SceFiosOpenParams> pOpenParams)
{
	throw EXCEPTION("");
}

s64 sceFiosArchiveGetMountBufferSizeSync(vm::cptr<SceFiosOpAttr> pAttr, vm::cptr<char> pArchivePath, vm::cptr<SceFiosOpenParams> pOpenParams)
{
	throw EXCEPTION("");
}

s32 sceFiosArchiveMount(vm::cptr<SceFiosOpAttr> pAttr, vm::ptr<s32> pOutFH, vm::cptr<char> pArchivePath, vm::cptr<char> pMountPoint, SceFiosBuffer mountBuffer, vm::cptr<SceFiosOpenParams> pOpenParams)
{
	throw EXCEPTION("");
}

s32 sceFiosArchiveMountSync(vm::cptr<SceFiosOpAttr> pAttr, vm::ptr<s32> pOutFH, vm::cptr<char> pArchivePath, vm::cptr<char> pMountPoint, SceFiosBuffer mountBuffer, vm::cptr<SceFiosOpenParams> pOpenParams)
{
	throw EXCEPTION("");
}

s32 sceFiosArchiveUnmount(vm::cptr<SceFiosOpAttr> pAttr, s32 fh)
{
	throw EXCEPTION("");
}

s32 sceFiosArchiveUnmountSync(vm::cptr<SceFiosOpAttr> pAttr, s32 fh)
{
	throw EXCEPTION("");
}

vm::ptr<char> sceFiosDebugDumpError(s32 err, vm::ptr<char> pBuffer, u32 bufferSize)
{
	throw EXCEPTION("");
}

vm::ptr<char> sceFiosDebugDumpOp(s32 op, vm::ptr<char> pBuffer, u32 bufferSize)
{
	throw EXCEPTION("");
}

vm::ptr<char> sceFiosDebugDumpFH(s32 fh, vm::ptr<char> pBuffer, u32 bufferSize)
{
	throw EXCEPTION("");
}

vm::ptr<char> sceFiosDebugDumpDH(s32 dh, vm::ptr<char> pBuffer, u32 bufferSize)
{
	throw EXCEPTION("");
}

vm::ptr<char> sceFiosDebugDumpDate(u64 date, vm::ptr<char> pBuffer, u32 bufferSize)
{
	throw EXCEPTION("");
}

s32 sceFiosIOFilterAdd(s32 index, vm::ptr<SceFiosIOFilterCallback> pFilterCallback, vm::ptr<void> pFilterContext)
{
	throw EXCEPTION("");
}

s32 sceFiosIOFilterGetInfo(s32 index, vm::pptr<SceFiosIOFilterCallback> pOutFilterCallback, vm::pptr<void> pOutFilterContext)
{
	throw EXCEPTION("");
}

s32 sceFiosIOFilterRemove(s32 index)
{
	throw EXCEPTION("");
}

void sceFiosIOFilterPsarcDearchiver()
{
	throw EXCEPTION("");
}

#define REG_FUNC(nid, name) reg_psv_func(nid, &sceFios, #name, name)

psv_log_base sceFios("SceFios2", []()
{
	sceFios.on_load = nullptr;
	sceFios.on_unload = nullptr;
	sceFios.on_stop = nullptr;
	sceFios.on_error = nullptr;

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
