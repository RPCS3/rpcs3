#include "stdafx.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/IPC.h"
#include "Emu/PSP2/ARMv7Module.h"

#include "sceLibKernel.h"

#include "Utilities/StrUtil.h"
#include "Utilities/lockless.h"

#include <algorithm>

logs::channel sceLibKernel("sceLibKernel");

extern u64 get_system_time();

template<>
void fmt_class_string<SceError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
		STR_CASE(SCE_ERROR_ERRNO_EPERM);
		STR_CASE(SCE_ERROR_ERRNO_ENOENT);
		STR_CASE(SCE_ERROR_ERRNO_ESRCH);
		STR_CASE(SCE_ERROR_ERRNO_EINTR);
		STR_CASE(SCE_ERROR_ERRNO_EIO);
		STR_CASE(SCE_ERROR_ERRNO_ENXIO);
		STR_CASE(SCE_ERROR_ERRNO_E2BIG);
		STR_CASE(SCE_ERROR_ERRNO_ENOEXEC);
		STR_CASE(SCE_ERROR_ERRNO_EBADF);
		STR_CASE(SCE_ERROR_ERRNO_ECHILD);
		STR_CASE(SCE_ERROR_ERRNO_EAGAIN);
		STR_CASE(SCE_ERROR_ERRNO_ENOMEM);
		STR_CASE(SCE_ERROR_ERRNO_EACCES);
		STR_CASE(SCE_ERROR_ERRNO_EFAULT);
		STR_CASE(SCE_ERROR_ERRNO_ENOTBLK);
		STR_CASE(SCE_ERROR_ERRNO_EBUSY);
		STR_CASE(SCE_ERROR_ERRNO_EEXIST);
		STR_CASE(SCE_ERROR_ERRNO_EXDEV);
		STR_CASE(SCE_ERROR_ERRNO_ENODEV);
		STR_CASE(SCE_ERROR_ERRNO_ENOTDIR);
		STR_CASE(SCE_ERROR_ERRNO_EISDIR);
		STR_CASE(SCE_ERROR_ERRNO_EINVAL);
		STR_CASE(SCE_ERROR_ERRNO_ENFILE);
		STR_CASE(SCE_ERROR_ERRNO_EMFILE);
		STR_CASE(SCE_ERROR_ERRNO_ENOTTY);
		STR_CASE(SCE_ERROR_ERRNO_ETXTBSY);
		STR_CASE(SCE_ERROR_ERRNO_EFBIG);
		STR_CASE(SCE_ERROR_ERRNO_ENOSPC);
		STR_CASE(SCE_ERROR_ERRNO_ESPIPE);
		STR_CASE(SCE_ERROR_ERRNO_EROFS);
		STR_CASE(SCE_ERROR_ERRNO_EMLINK);
		STR_CASE(SCE_ERROR_ERRNO_EPIPE);
		STR_CASE(SCE_ERROR_ERRNO_EDOM);
		STR_CASE(SCE_ERROR_ERRNO_ERANGE);
		STR_CASE(SCE_ERROR_ERRNO_ENOMSG);
		STR_CASE(SCE_ERROR_ERRNO_EIDRM);
		STR_CASE(SCE_ERROR_ERRNO_ECHRNG);
		STR_CASE(SCE_ERROR_ERRNO_EL2NSYNC);
		STR_CASE(SCE_ERROR_ERRNO_EL3HLT);
		STR_CASE(SCE_ERROR_ERRNO_EL3RST);
		STR_CASE(SCE_ERROR_ERRNO_ELNRNG);
		STR_CASE(SCE_ERROR_ERRNO_EUNATCH);
		STR_CASE(SCE_ERROR_ERRNO_ENOCSI);
		STR_CASE(SCE_ERROR_ERRNO_EL2HLT);
		STR_CASE(SCE_ERROR_ERRNO_EDEADLK);
		STR_CASE(SCE_ERROR_ERRNO_ENOLCK);
		STR_CASE(SCE_ERROR_ERRNO_EFORMAT);
		STR_CASE(SCE_ERROR_ERRNO_EUNSUP);
		STR_CASE(SCE_ERROR_ERRNO_EBADE);
		STR_CASE(SCE_ERROR_ERRNO_EBADR);
		STR_CASE(SCE_ERROR_ERRNO_EXFULL);
		STR_CASE(SCE_ERROR_ERRNO_ENOANO);
		STR_CASE(SCE_ERROR_ERRNO_EBADRQC);
		STR_CASE(SCE_ERROR_ERRNO_EBADSLT);
		STR_CASE(SCE_ERROR_ERRNO_EDEADLOCK);
		STR_CASE(SCE_ERROR_ERRNO_EBFONT);
		STR_CASE(SCE_ERROR_ERRNO_ENOSTR);
		STR_CASE(SCE_ERROR_ERRNO_ENODATA);
		STR_CASE(SCE_ERROR_ERRNO_ETIME);
		STR_CASE(SCE_ERROR_ERRNO_ENOSR);
		STR_CASE(SCE_ERROR_ERRNO_ENONET);
		STR_CASE(SCE_ERROR_ERRNO_ENOPKG);
		STR_CASE(SCE_ERROR_ERRNO_EREMOTE);
		STR_CASE(SCE_ERROR_ERRNO_ENOLINK);
		STR_CASE(SCE_ERROR_ERRNO_EADV);
		STR_CASE(SCE_ERROR_ERRNO_ESRMNT);
		STR_CASE(SCE_ERROR_ERRNO_ECOMM);
		STR_CASE(SCE_ERROR_ERRNO_EPROTO);
		STR_CASE(SCE_ERROR_ERRNO_EMULTIHOP);
		STR_CASE(SCE_ERROR_ERRNO_ELBIN);
		STR_CASE(SCE_ERROR_ERRNO_EDOTDOT);
		STR_CASE(SCE_ERROR_ERRNO_EBADMSG);
		STR_CASE(SCE_ERROR_ERRNO_EFTYPE);
		STR_CASE(SCE_ERROR_ERRNO_ENOTUNIQ);
		STR_CASE(SCE_ERROR_ERRNO_EBADFD);
		STR_CASE(SCE_ERROR_ERRNO_EREMCHG);
		STR_CASE(SCE_ERROR_ERRNO_ELIBACC);
		STR_CASE(SCE_ERROR_ERRNO_ELIBBAD);
		STR_CASE(SCE_ERROR_ERRNO_ELIBSCN);
		STR_CASE(SCE_ERROR_ERRNO_ELIBMAX);
		STR_CASE(SCE_ERROR_ERRNO_ELIBEXEC);
		STR_CASE(SCE_ERROR_ERRNO_ENOSYS);
		STR_CASE(SCE_ERROR_ERRNO_ENMFILE);
		STR_CASE(SCE_ERROR_ERRNO_ENOTEMPTY);
		STR_CASE(SCE_ERROR_ERRNO_ENAMETOOLONG);
		STR_CASE(SCE_ERROR_ERRNO_ELOOP);
		STR_CASE(SCE_ERROR_ERRNO_EOPNOTSUPP);
		STR_CASE(SCE_ERROR_ERRNO_EPFNOSUPPORT);
		STR_CASE(SCE_ERROR_ERRNO_ECONNRESET);
		STR_CASE(SCE_ERROR_ERRNO_ENOBUFS);
		STR_CASE(SCE_ERROR_ERRNO_EAFNOSUPPORT);
		STR_CASE(SCE_ERROR_ERRNO_EPROTOTYPE);
		STR_CASE(SCE_ERROR_ERRNO_ENOTSOCK);
		STR_CASE(SCE_ERROR_ERRNO_ENOPROTOOPT);
		STR_CASE(SCE_ERROR_ERRNO_ESHUTDOWN);
		STR_CASE(SCE_ERROR_ERRNO_ECONNREFUSED);
		STR_CASE(SCE_ERROR_ERRNO_EADDRINUSE);
		STR_CASE(SCE_ERROR_ERRNO_ECONNABORTED);
		STR_CASE(SCE_ERROR_ERRNO_ENETUNREACH);
		STR_CASE(SCE_ERROR_ERRNO_ENETDOWN);
		STR_CASE(SCE_ERROR_ERRNO_ETIMEDOUT);
		STR_CASE(SCE_ERROR_ERRNO_EHOSTDOWN);
		STR_CASE(SCE_ERROR_ERRNO_EHOSTUNREACH);
		STR_CASE(SCE_ERROR_ERRNO_EINPROGRESS);
		STR_CASE(SCE_ERROR_ERRNO_EALREADY);
		STR_CASE(SCE_ERROR_ERRNO_EDESTADDRREQ);
		STR_CASE(SCE_ERROR_ERRNO_EMSGSIZE);
		STR_CASE(SCE_ERROR_ERRNO_EPROTONOSUPPORT);
		STR_CASE(SCE_ERROR_ERRNO_ESOCKTNOSUPPORT);
		STR_CASE(SCE_ERROR_ERRNO_EADDRNOTAVAIL);
		STR_CASE(SCE_ERROR_ERRNO_ENETRESET);
		STR_CASE(SCE_ERROR_ERRNO_EISCONN);
		STR_CASE(SCE_ERROR_ERRNO_ENOTCONN);
		STR_CASE(SCE_ERROR_ERRNO_ETOOMANYREFS);
		STR_CASE(SCE_ERROR_ERRNO_EPROCLIM);
		STR_CASE(SCE_ERROR_ERRNO_EUSERS);
		STR_CASE(SCE_ERROR_ERRNO_EDQUOT);
		STR_CASE(SCE_ERROR_ERRNO_ESTALE);
		STR_CASE(SCE_ERROR_ERRNO_ENOTSUP);
		STR_CASE(SCE_ERROR_ERRNO_ENOMEDIUM);
		STR_CASE(SCE_ERROR_ERRNO_ENOSHARE);
		STR_CASE(SCE_ERROR_ERRNO_ECASECLASH);
		STR_CASE(SCE_ERROR_ERRNO_EILSEQ);
		STR_CASE(SCE_ERROR_ERRNO_EOVERFLOW);
		STR_CASE(SCE_ERROR_ERRNO_ECANCELED);
		STR_CASE(SCE_ERROR_ERRNO_ENOTRECOVERABLE);
		STR_CASE(SCE_ERROR_ERRNO_EOWNERDEAD);
		}

		return unknown;
	});
}

template<>
void fmt_class_string<SceLibKernelError>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](auto error)
	{
		switch (error)
		{
		STR_CASE(SCE_KERNEL_ERROR_ERROR);
		STR_CASE(SCE_KERNEL_ERROR_NOT_IMPLEMENTED);
		STR_CASE(SCE_KERNEL_ERROR_INVALID_ARGUMENT);
		STR_CASE(SCE_KERNEL_ERROR_INVALID_ARGUMENT_SIZE);
		STR_CASE(SCE_KERNEL_ERROR_INVALID_FLAGS);
		STR_CASE(SCE_KERNEL_ERROR_ILLEGAL_SIZE);
		STR_CASE(SCE_KERNEL_ERROR_ILLEGAL_ADDR);
		STR_CASE(SCE_KERNEL_ERROR_UNSUP);
		STR_CASE(SCE_KERNEL_ERROR_ILLEGAL_MODE);
		STR_CASE(SCE_KERNEL_ERROR_ILLEGAL_ALIGNMENT);
		STR_CASE(SCE_KERNEL_ERROR_NOSYS);
		STR_CASE(SCE_KERNEL_ERROR_DEBUG_ERROR);
		STR_CASE(SCE_KERNEL_ERROR_ILLEGAL_DIPSW_NUMBER);
		STR_CASE(SCE_KERNEL_ERROR_CPU_ERROR);
		STR_CASE(SCE_KERNEL_ERROR_MMU_ILLEGAL_L1_TYPE);
		STR_CASE(SCE_KERNEL_ERROR_MMU_L2_INDEX_OVERFLOW);
		STR_CASE(SCE_KERNEL_ERROR_MMU_L2_SIZE_OVERFLOW);
		STR_CASE(SCE_KERNEL_ERROR_INVALID_CPU_AFFINITY);
		STR_CASE(SCE_KERNEL_ERROR_INVALID_MEMORY_ACCESS);
		STR_CASE(SCE_KERNEL_ERROR_INVALID_MEMORY_ACCESS_PERMISSION);
		STR_CASE(SCE_KERNEL_ERROR_VA2PA_FAULT);
		STR_CASE(SCE_KERNEL_ERROR_VA2PA_MAPPED);
		STR_CASE(SCE_KERNEL_ERROR_VALIDATION_CHECK_FAILED);
		STR_CASE(SCE_KERNEL_ERROR_SYSMEM_ERROR);
		STR_CASE(SCE_KERNEL_ERROR_INVALID_PROCESS_CONTEXT);
		STR_CASE(SCE_KERNEL_ERROR_UID_NAME_TOO_LONG);
		STR_CASE(SCE_KERNEL_ERROR_VARANGE_IS_NOT_PHYSICAL_CONTINUOUS);
		STR_CASE(SCE_KERNEL_ERROR_PHYADDR_ERROR);
		STR_CASE(SCE_KERNEL_ERROR_NO_PHYADDR);
		STR_CASE(SCE_KERNEL_ERROR_PHYADDR_USED);
		STR_CASE(SCE_KERNEL_ERROR_PHYADDR_NOT_USED);
		STR_CASE(SCE_KERNEL_ERROR_NO_IOADDR);
		STR_CASE(SCE_KERNEL_ERROR_PHYMEM_ERROR);
		STR_CASE(SCE_KERNEL_ERROR_ILLEGAL_PHYPAGE_STATUS);
		STR_CASE(SCE_KERNEL_ERROR_NO_FREE_PHYSICAL_PAGE);
		STR_CASE(SCE_KERNEL_ERROR_NO_FREE_PHYSICAL_PAGE_UNIT);
		STR_CASE(SCE_KERNEL_ERROR_PHYMEMPART_NOT_EMPTY);
		STR_CASE(SCE_KERNEL_ERROR_NO_PHYMEMPART_LPDDR2);
		STR_CASE(SCE_KERNEL_ERROR_NO_PHYMEMPART_CDRAM);
		STR_CASE(SCE_KERNEL_ERROR_FIXEDHEAP_ERROR);
		STR_CASE(SCE_KERNEL_ERROR_FIXEDHEAP_ILLEGAL_SIZE);
		STR_CASE(SCE_KERNEL_ERROR_FIXEDHEAP_ILLEGAL_INDEX);
		STR_CASE(SCE_KERNEL_ERROR_FIXEDHEAP_INDEX_OVERFLOW);
		STR_CASE(SCE_KERNEL_ERROR_FIXEDHEAP_NO_CHUNK);
		STR_CASE(SCE_KERNEL_ERROR_UID_ERROR);
		STR_CASE(SCE_KERNEL_ERROR_INVALID_UID);
		STR_CASE(SCE_KERNEL_ERROR_SYSMEM_UID_INVALID_ARGUMENT);
		STR_CASE(SCE_KERNEL_ERROR_SYSMEM_INVALID_UID_RANGE);
		STR_CASE(SCE_KERNEL_ERROR_SYSMEM_NO_VALID_UID);
		STR_CASE(SCE_KERNEL_ERROR_SYSMEM_CANNOT_ALLOCATE_UIDENTRY);
		STR_CASE(SCE_KERNEL_ERROR_NOT_PROCESS_UID);
		STR_CASE(SCE_KERNEL_ERROR_NOT_KERNEL_UID);
		STR_CASE(SCE_KERNEL_ERROR_INVALID_UID_CLASS);
		STR_CASE(SCE_KERNEL_ERROR_INVALID_UID_SUBCLASS);
		STR_CASE(SCE_KERNEL_ERROR_UID_CANNOT_FIND_BY_NAME);
		STR_CASE(SCE_KERNEL_ERROR_VIRPAGE_ERROR);
		STR_CASE(SCE_KERNEL_ERROR_ILLEGAL_VIRPAGE_TYPE);
		STR_CASE(SCE_KERNEL_ERROR_BLOCK_ERROR);
		STR_CASE(SCE_KERNEL_ERROR_ILLEGAL_BLOCK_ID);
		STR_CASE(SCE_KERNEL_ERROR_ILLEGAL_BLOCK_TYPE);
		STR_CASE(SCE_KERNEL_ERROR_BLOCK_IN_USE);
		STR_CASE(SCE_KERNEL_ERROR_PARTITION_ERROR);
		STR_CASE(SCE_KERNEL_ERROR_ILLEGAL_PARTITION_ID);
		STR_CASE(SCE_KERNEL_ERROR_ILLEGAL_PARTITION_INDEX);
		STR_CASE(SCE_KERNEL_ERROR_NO_L2PAGETABLE);
		STR_CASE(SCE_KERNEL_ERROR_HEAPLIB_ERROR);
		STR_CASE(SCE_KERNEL_ERROR_ILLEGAL_HEAP_ID);
		STR_CASE(SCE_KERNEL_ERROR_OUT_OF_RANG);
		STR_CASE(SCE_KERNEL_ERROR_HEAPLIB_NOMEM);
		STR_CASE(SCE_KERNEL_ERROR_SYSMEM_ADDRESS_SPACE_ERROR);
		STR_CASE(SCE_KERNEL_ERROR_INVALID_ADDRESS_SPACE_ID);
		STR_CASE(SCE_KERNEL_ERROR_INVALID_PARTITION_INDEX);
		STR_CASE(SCE_KERNEL_ERROR_ADDRESS_SPACE_CANNOT_FIND_PARTITION_BY_ADDR);
		STR_CASE(SCE_KERNEL_ERROR_SYSMEM_MEMBLOCK_ERROR);
		STR_CASE(SCE_KERNEL_ERROR_ILLEGAL_MEMBLOCK_TYPE);
		STR_CASE(SCE_KERNEL_ERROR_ILLEGAL_MEMBLOCK_REMAP_TYPE);
		STR_CASE(SCE_KERNEL_ERROR_NOT_PHY_CONT_MEMBLOCK);
		STR_CASE(SCE_KERNEL_ERROR_ILLEGAL_MEMBLOCK_CODE);
		STR_CASE(SCE_KERNEL_ERROR_ILLEGAL_MEMBLOCK_SIZE);
		STR_CASE(SCE_KERNEL_ERROR_ILLEGAL_USERMAP_SIZE);
		STR_CASE(SCE_KERNEL_ERROR_MEMBLOCK_TYPE_FOR_KERNEL_PROCESS);
		STR_CASE(SCE_KERNEL_ERROR_PROCESS_CANNOT_REMAP_MEMBLOCK);
		STR_CASE(SCE_KERNEL_ERROR_SYSMEM_PHYMEMLOW_ERROR);
		STR_CASE(SCE_KERNEL_ERROR_CANNOT_ALLOC_PHYMEMLOW);
		STR_CASE(SCE_KERNEL_ERROR_UNKNOWN_PHYMEMLOW_TYPE);
		STR_CASE(SCE_KERNEL_ERROR_SYSMEM_BITHEAP_ERROR);
		STR_CASE(SCE_KERNEL_ERROR_CANNOT_ALLOC_BITHEAP);
		STR_CASE(SCE_KERNEL_ERROR_LOADCORE_ERROR);
		STR_CASE(SCE_KERNEL_ERROR_ILLEGAL_ELF_HEADER);
		STR_CASE(SCE_KERNEL_ERROR_ILLEGAL_SELF_HEADER);
		STR_CASE(SCE_KERNEL_ERROR_EXCPMGR_ERROR);
		STR_CASE(SCE_KERNEL_ERROR_ILLEGAL_EXCPCODE);
		STR_CASE(SCE_KERNEL_ERROR_ILLEGAL_EXCPHANDLER);
		STR_CASE(SCE_KERNEL_ERROR_NOTFOUND_EXCPHANDLER);
		STR_CASE(SCE_KERNEL_ERROR_CANNOT_RELEASE_EXCPHANDLER);
		STR_CASE(SCE_KERNEL_ERROR_INTRMGR_ERROR);
		STR_CASE(SCE_KERNEL_ERROR_ILLEGAL_CONTEXT);
		STR_CASE(SCE_KERNEL_ERROR_ILLEGAL_INTRCODE);
		STR_CASE(SCE_KERNEL_ERROR_ILLEGAL_INTRPARAM);
		STR_CASE(SCE_KERNEL_ERROR_ILLEGAL_INTRPRIORITY);
		STR_CASE(SCE_KERNEL_ERROR_ILLEGAL_TARGET_CPU);
		STR_CASE(SCE_KERNEL_ERROR_ILLEGAL_INTRFILTER);
		STR_CASE(SCE_KERNEL_ERROR_ILLEGAL_INTRTYPE);
		STR_CASE(SCE_KERNEL_ERROR_ILLEGAL_HANDLER);
		STR_CASE(SCE_KERNEL_ERROR_FOUND_HANDLER);
		STR_CASE(SCE_KERNEL_ERROR_NOTFOUND_HANDLER);
		STR_CASE(SCE_KERNEL_ERROR_NO_MEMORY);
		STR_CASE(SCE_KERNEL_ERROR_DMACMGR_ERROR);
		STR_CASE(SCE_KERNEL_ERROR_ALREADY_QUEUED);
		STR_CASE(SCE_KERNEL_ERROR_NOT_QUEUED);
		STR_CASE(SCE_KERNEL_ERROR_NOT_SETUP);
		STR_CASE(SCE_KERNEL_ERROR_ON_TRANSFERRING);
		STR_CASE(SCE_KERNEL_ERROR_NOT_INITIALIZED);
		STR_CASE(SCE_KERNEL_ERROR_TRANSFERRED);
		STR_CASE(SCE_KERNEL_ERROR_NOT_UNDER_CONTROL);
		STR_CASE(SCE_KERNEL_ERROR_SYSTIMER_ERROR);
		STR_CASE(SCE_KERNEL_ERROR_NO_FREE_TIMER);
		STR_CASE(SCE_KERNEL_ERROR_TIMER_NOT_ALLOCATED);
		STR_CASE(SCE_KERNEL_ERROR_TIMER_COUNTING);
		STR_CASE(SCE_KERNEL_ERROR_TIMER_STOPPED);
		STR_CASE(SCE_KERNEL_ERROR_THREADMGR_ERROR);
		STR_CASE(SCE_KERNEL_ERROR_DORMANT);
		STR_CASE(SCE_KERNEL_ERROR_NOT_DORMANT);
		STR_CASE(SCE_KERNEL_ERROR_UNKNOWN_THID);
		STR_CASE(SCE_KERNEL_ERROR_CAN_NOT_WAIT);
		STR_CASE(SCE_KERNEL_ERROR_ILLEGAL_THID);
		STR_CASE(SCE_KERNEL_ERROR_THREAD_TERMINATED);
		STR_CASE(SCE_KERNEL_ERROR_DELETED);
		STR_CASE(SCE_KERNEL_ERROR_WAIT_TIMEOUT);
		STR_CASE(SCE_KERNEL_ERROR_NOTIFY_CALLBACK);
		STR_CASE(SCE_KERNEL_ERROR_WAIT_DELETE);
		STR_CASE(SCE_KERNEL_ERROR_ILLEGAL_ATTR);
		STR_CASE(SCE_KERNEL_ERROR_EVF_MULTI);
		STR_CASE(SCE_KERNEL_ERROR_WAIT_CANCEL);
		STR_CASE(SCE_KERNEL_ERROR_EVF_COND);
		STR_CASE(SCE_KERNEL_ERROR_ILLEGAL_COUNT);
		STR_CASE(SCE_KERNEL_ERROR_ILLEGAL_PRIORITY);
		STR_CASE(SCE_KERNEL_ERROR_MUTEX_RECURSIVE);
		STR_CASE(SCE_KERNEL_ERROR_MUTEX_LOCK_OVF);
		STR_CASE(SCE_KERNEL_ERROR_MUTEX_NOT_OWNED);
		STR_CASE(SCE_KERNEL_ERROR_MUTEX_UNLOCK_UDF);
		STR_CASE(SCE_KERNEL_ERROR_MUTEX_FAILED_TO_OWN);
		STR_CASE(SCE_KERNEL_ERROR_FAST_MUTEX_RECURSIVE);
		STR_CASE(SCE_KERNEL_ERROR_FAST_MUTEX_LOCK_OVF);
		STR_CASE(SCE_KERNEL_ERROR_FAST_MUTEX_FAILED_TO_OWN);
		STR_CASE(SCE_KERNEL_ERROR_FAST_MUTEX_NOT_OWNED);
		STR_CASE(SCE_KERNEL_ERROR_FAST_MUTEX_OWNED);
		STR_CASE(SCE_KERNEL_ERROR_ALARM_CAN_NOT_CANCEL);
		STR_CASE(SCE_KERNEL_ERROR_INVALID_OBJECT_TYPE);
		STR_CASE(SCE_KERNEL_ERROR_KERNEL_TLS_FULL);
		STR_CASE(SCE_KERNEL_ERROR_ILLEGAL_KERNEL_TLS_INDEX);
		STR_CASE(SCE_KERNEL_ERROR_KERNEL_TLS_BUSY);
		STR_CASE(SCE_KERNEL_ERROR_DIFFERENT_UID_CLASS);
		STR_CASE(SCE_KERNEL_ERROR_UNKNOWN_UID);
		STR_CASE(SCE_KERNEL_ERROR_SEMA_ZERO);
		STR_CASE(SCE_KERNEL_ERROR_SEMA_OVF);
		STR_CASE(SCE_KERNEL_ERROR_PMON_NOT_THREAD_MODE);
		STR_CASE(SCE_KERNEL_ERROR_PMON_NOT_CPU_MODE);
		STR_CASE(SCE_KERNEL_ERROR_ALREADY_REGISTERED);
		STR_CASE(SCE_KERNEL_ERROR_INVALID_THREAD_ID);
		STR_CASE(SCE_KERNEL_ERROR_ALREADY_DEBUG_SUSPENDED);
		STR_CASE(SCE_KERNEL_ERROR_NOT_DEBUG_SUSPENDED);
		STR_CASE(SCE_KERNEL_ERROR_CAN_NOT_USE_VFP);
		STR_CASE(SCE_KERNEL_ERROR_RUNNING);
		STR_CASE(SCE_KERNEL_ERROR_EVENT_COND);
		STR_CASE(SCE_KERNEL_ERROR_MSG_PIPE_FULL);
		STR_CASE(SCE_KERNEL_ERROR_MSG_PIPE_EMPTY);
		STR_CASE(SCE_KERNEL_ERROR_ALREADY_SENT);
		STR_CASE(SCE_KERNEL_ERROR_CAN_NOT_SUSPEND);
		STR_CASE(SCE_KERNEL_ERROR_FAST_MUTEX_ALREADY_INITIALIZED);
		STR_CASE(SCE_KERNEL_ERROR_FAST_MUTEX_NOT_INITIALIZED);
		STR_CASE(SCE_KERNEL_ERROR_THREAD_STOPPED);
		STR_CASE(SCE_KERNEL_ERROR_THREAD_SUSPENDED);
		STR_CASE(SCE_KERNEL_ERROR_NOT_SUSPENDED);
		STR_CASE(SCE_KERNEL_ERROR_WAIT_DELETE_MUTEX);
		STR_CASE(SCE_KERNEL_ERROR_WAIT_CANCEL_MUTEX);
		STR_CASE(SCE_KERNEL_ERROR_WAIT_DELETE_COND);
		STR_CASE(SCE_KERNEL_ERROR_WAIT_CANCEL_COND);
		STR_CASE(SCE_KERNEL_ERROR_LW_MUTEX_NOT_OWNED);
		STR_CASE(SCE_KERNEL_ERROR_LW_MUTEX_LOCK_OVF);
		STR_CASE(SCE_KERNEL_ERROR_LW_MUTEX_UNLOCK_UDF);
		STR_CASE(SCE_KERNEL_ERROR_LW_MUTEX_RECURSIVE);
		STR_CASE(SCE_KERNEL_ERROR_LW_MUTEX_FAILED_TO_OWN);
		STR_CASE(SCE_KERNEL_ERROR_WAIT_DELETE_LW_MUTEX);
		STR_CASE(SCE_KERNEL_ERROR_ILLEGAL_STACK_SIZE);
		STR_CASE(SCE_KERNEL_ERROR_RW_LOCK_RECURSIVE);
		STR_CASE(SCE_KERNEL_ERROR_RW_LOCK_LOCK_OVF);
		STR_CASE(SCE_KERNEL_ERROR_RW_LOCK_NOT_OWNED);
		STR_CASE(SCE_KERNEL_ERROR_RW_LOCK_UNLOCK_UDF);
		STR_CASE(SCE_KERNEL_ERROR_RW_LOCK_FAILED_TO_LOCK);
		STR_CASE(SCE_KERNEL_ERROR_RW_LOCK_FAILED_TO_UNLOCK);
		STR_CASE(SCE_KERNEL_ERROR_PROCESSMGR_ERROR);
		STR_CASE(SCE_KERNEL_ERROR_INVALID_PID);
		STR_CASE(SCE_KERNEL_ERROR_INVALID_PROCESS_TYPE);
		STR_CASE(SCE_KERNEL_ERROR_PLS_FULL);
		STR_CASE(SCE_KERNEL_ERROR_INVALID_PROCESS_STATUS);
		STR_CASE(SCE_KERNEL_ERROR_INVALID_BUDGET_ID);
		STR_CASE(SCE_KERNEL_ERROR_INVALID_BUDGET_SIZE);
		STR_CASE(SCE_KERNEL_ERROR_CP14_DISABLED);
		STR_CASE(SCE_KERNEL_ERROR_EXCEEDED_MAX_PROCESSES);
		STR_CASE(SCE_KERNEL_ERROR_PROCESS_REMAINING);
		STR_CASE(SCE_KERNEL_ERROR_IOFILEMGR_ERROR);
		STR_CASE(SCE_KERNEL_ERROR_IO_NAME_TOO_LONG);
		STR_CASE(SCE_KERNEL_ERROR_IO_REG_DEV);
		STR_CASE(SCE_KERNEL_ERROR_IO_ALIAS_USED);
		STR_CASE(SCE_KERNEL_ERROR_IO_DEL_DEV);
		STR_CASE(SCE_KERNEL_ERROR_IO_WOULD_BLOCK);
		STR_CASE(SCE_KERNEL_ERROR_MODULEMGR_START_FAILED);
		STR_CASE(SCE_KERNEL_ERROR_MODULEMGR_STOP_FAIL);
		STR_CASE(SCE_KERNEL_ERROR_MODULEMGR_IN_USE);
		STR_CASE(SCE_KERNEL_ERROR_MODULEMGR_NO_LIB);
		STR_CASE(SCE_KERNEL_ERROR_MODULEMGR_SYSCALL_REG);
		STR_CASE(SCE_KERNEL_ERROR_MODULEMGR_NOMEM_LIB);
		STR_CASE(SCE_KERNEL_ERROR_MODULEMGR_NOMEM_STUB);
		STR_CASE(SCE_KERNEL_ERROR_MODULEMGR_NOMEM_SELF);
		STR_CASE(SCE_KERNEL_ERROR_MODULEMGR_NOMEM);
		STR_CASE(SCE_KERNEL_ERROR_MODULEMGR_INVALID_LIB);
		STR_CASE(SCE_KERNEL_ERROR_MODULEMGR_INVALID_STUB);
		STR_CASE(SCE_KERNEL_ERROR_MODULEMGR_NO_FUNC_NID);
		STR_CASE(SCE_KERNEL_ERROR_MODULEMGR_NO_VAR_NID);
		STR_CASE(SCE_KERNEL_ERROR_MODULEMGR_INVALID_TYPE);
		STR_CASE(SCE_KERNEL_ERROR_MODULEMGR_NO_MOD_ENTRY);
		STR_CASE(SCE_KERNEL_ERROR_MODULEMGR_INVALID_PROC_PARAM);
		STR_CASE(SCE_KERNEL_ERROR_MODULEMGR_NO_MODOBJ);
		STR_CASE(SCE_KERNEL_ERROR_MODULEMGR_NO_MOD);
		STR_CASE(SCE_KERNEL_ERROR_MODULEMGR_NO_PROCESS);
		STR_CASE(SCE_KERNEL_ERROR_MODULEMGR_OLD_LIB);
		STR_CASE(SCE_KERNEL_ERROR_MODULEMGR_STARTED);
		STR_CASE(SCE_KERNEL_ERROR_MODULEMGR_NOT_STARTED);
		STR_CASE(SCE_KERNEL_ERROR_MODULEMGR_NOT_STOPPED);
		STR_CASE(SCE_KERNEL_ERROR_MODULEMGR_INVALID_PROCESS_UID);
		STR_CASE(SCE_KERNEL_ERROR_MODULEMGR_CANNOT_EXPORT_LIB_TO_SHARED);
		STR_CASE(SCE_KERNEL_ERROR_MODULEMGR_INVALID_REL_INFO);
		STR_CASE(SCE_KERNEL_ERROR_MODULEMGR_INVALID_REF_INFO);
		STR_CASE(SCE_KERNEL_ERROR_MODULEMGR_ELINK);
		STR_CASE(SCE_KERNEL_ERROR_MODULEMGR_NOENT);
		STR_CASE(SCE_KERNEL_ERROR_MODULEMGR_BUSY);
		STR_CASE(SCE_KERNEL_ERROR_MODULEMGR_NOEXEC);
		STR_CASE(SCE_KERNEL_ERROR_MODULEMGR_NAMETOOLONG);
		STR_CASE(SCE_KERNEL_ERROR_LIBRARYDB_NOENT);
		STR_CASE(SCE_KERNEL_ERROR_LIBRARYDB_NO_LIB);
		STR_CASE(SCE_KERNEL_ERROR_LIBRARYDB_NO_MOD);
		STR_CASE(SCE_KERNEL_ERROR_AUTHFAIL);
		STR_CASE(SCE_KERNEL_ERROR_NO_AUTH);
		}

		return unknown;
	});
}

arm_tls_manager::arm_tls_manager(u32 vaddr, u32 fsize, u32 vsize)
	: vaddr(vaddr)
	, fsize(fsize)
	, vsize(vsize)
	, start(vsize ? vm::alloc(vsize * ::size32(m_map), vm::main) : 0)
{
}

u32 arm_tls_manager::alloc()
{
	if (!vsize)
	{
		return 0;
	}

	for (u32 i = 0; i < m_map.size(); i++)
	{
		if (!m_map[i] && m_map[i].exchange(true) == false)
		{
			const u32 addr = start + i * vsize; // Get TLS address
			std::memcpy(vm::base(addr), vm::base(vaddr), fsize); // Initialize from TLS image
			std::memset(vm::base(addr + fsize), 0, vsize - fsize); // Fill the rest with zeros
			return addr;
		}
	}

	sceLibKernel.error("arm_tls_manager::alloc(): out of TLS memory (max=%zu)", m_map.size());
	return 0;
}

void arm_tls_manager::dealloc(u32 addr)
{
	if (!addr)
	{
		return;
	}

	// Calculate TLS index
	const u32 i = (addr - start) / vsize;

	if (addr < start || i >= m_map.size() || (addr - start) % vsize)
	{
		sceLibKernel.error("arm_tls_manager::free(0x%x): invalid address", addr);
		return;
	}

	if (m_map[i].exchange(false) == false)
	{
		sceLibKernel.error("arm_tls_manager::free(0x%x): deallocation failed", addr);
		return;
	}
}

s32 sceKernelAllocMemBlock(vm::cptr<char> name, s32 type, u32 vsize, vm::ptr<SceKernelAllocMemBlockOpt> pOpt)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelFreeMemBlock(s32 uid)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelGetMemBlockBase(s32 uid, vm::pptr<void> ppBase)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelGetMemBlockInfoByAddr(vm::ptr<void> vbase, vm::ptr<SceKernelMemBlockInfo> pInfo)
{
	fmt::throw_exception("Unimplemented" HERE);
}

error_code sceKernelCreateThread(vm::cptr<char> pName, vm::ptr<SceKernelThreadEntry> entry, s32 initPriority, u32 stackSize, u32 attr, s32 cpuAffinityMask, vm::cptr<SceKernelThreadOptParam> pOptParam)
{
	sceLibKernel.warning("sceKernelCreateThread(pName=%s, entry=*0x%x, initPriority=%d, stackSize=0x%x, attr=0x%x, cpuAffinityMask=0x%x, pOptParam=*0x%x)",
		pName, entry, initPriority, stackSize, attr, cpuAffinityMask, pOptParam);

	const auto thread = idm::make_ptr<ARMv7Thread>(pName.get_ptr(), initPriority, stackSize);

	thread->write_pc(entry.addr(), 0);
	thread->TLS = fxm::get<arm_tls_manager>()->alloc();

	return not_an_error(thread->id);
}

error_code sceKernelStartThread(s32 threadId, u32 argSize, vm::cptr<void> pArgBlock)
{
	sceLibKernel.warning("sceKernelStartThread(threadId=0x%x, argSize=0x%x, pArgBlock=*0x%x)", threadId, argSize, pArgBlock);

	const auto thread = idm::get<ARMv7Thread>(threadId);

	if (!thread)
	{
		return SCE_KERNEL_ERROR_INVALID_UID;
	}

	// thread should be in DORMANT state, but it's not possible to check it correctly atm

	//if (thread->IsAlive())
	//{
	//	return SCE_KERNEL_ERROR_NOT_DORMANT;
	//}

	// push arg block onto the stack
	const u32 pos = (thread->SP -= argSize);
	std::memcpy(vm::base(pos), pArgBlock.get_ptr(), argSize);

	// set SceKernelThreadEntry function arguments
	thread->GPR[0] = argSize;
	thread->GPR[1] = pos;
	thread->run();
	return SCE_OK;
}

error_code sceKernelExitThread(ARMv7Thread& cpu, s32 exitStatus)
{
	sceLibKernel.warning("sceKernelExitThread(exitStatus=0x%x)", exitStatus);

	// Exit status is stored in r0
	cpu.state += cpu_flag::exit;

	return SCE_OK;
}

error_code sceKernelDeleteThread(s32 threadId)
{
	sceLibKernel.warning("sceKernelDeleteThread(threadId=0x%x)", threadId);

	const auto thread = idm::get<ARMv7Thread>(threadId);

	if (!thread)
	{
		return SCE_KERNEL_ERROR_INVALID_UID;
	}

	// thread should be in DORMANT state, but it's not possible to check it correctly atm

	//if (thread->IsAlive())
	//{
	//	return SCE_KERNEL_ERROR_NOT_DORMANT;
	//}

	fxm::get<arm_tls_manager>()->dealloc(thread->TLS);
	idm::remove<ARMv7Thread>(threadId);
	return SCE_OK;
}

error_code sceKernelExitDeleteThread(ARMv7Thread& cpu, s32 exitStatus)
{
	sceLibKernel.warning("sceKernelExitDeleteThread(exitStatus=0x%x)", exitStatus);

	//cpu.state += cpu_flag::stop;

	// Delete current thread; exit status is stored in r0
	fxm::get<arm_tls_manager>()->dealloc(cpu.TLS);
	idm::remove<ARMv7Thread>(cpu.id);

	return SCE_OK;
}

s32 sceKernelChangeThreadCpuAffinityMask(s32 threadId, s32 cpuAffinityMask)
{
	sceLibKernel.todo("sceKernelChangeThreadCpuAffinityMask(threadId=0x%x, cpuAffinityMask=0x%x)", threadId, cpuAffinityMask);

	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelGetThreadCpuAffinityMask(s32 threadId)
{
	sceLibKernel.todo("sceKernelGetThreadCpuAffinityMask(threadId=0x%x)", threadId);

	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelChangeThreadPriority(s32 threadId, s32 priority)
{
	sceLibKernel.todo("sceKernelChangeThreadPriority(threadId=0x%x, priority=%d)", threadId, priority);

	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelGetThreadCurrentPriority()
{
	sceLibKernel.todo("sceKernelGetThreadCurrentPriority()");

	fmt::throw_exception("Unimplemented" HERE);
}

u32 sceKernelGetThreadId(ARMv7Thread& cpu)
{
	sceLibKernel.trace("sceKernelGetThreadId()");

	return cpu.id;
}

s32 sceKernelChangeCurrentThreadAttr(u32 clearAttr, u32 setAttr)
{
	sceLibKernel.todo("sceKernelChangeCurrentThreadAttr()");

	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelGetThreadExitStatus(s32 threadId, vm::ptr<s32> pExitStatus)
{
	sceLibKernel.todo("sceKernelGetThreadExitStatus(threadId=0x%x, pExitStatus=*0x%x)", threadId, pExitStatus);

	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelGetProcessId()
{
	sceLibKernel.todo("sceKernelGetProcessId()");

	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelCheckWaitableStatus()
{
	sceLibKernel.todo("sceKernelCheckWaitableStatus()");

	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelGetThreadInfo(s32 threadId, vm::ptr<SceKernelThreadInfo> pInfo)
{
	sceLibKernel.todo("sceKernelGetThreadInfo(threadId=0x%x, pInfo=*0x%x)", threadId, pInfo);

	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelGetThreadRunStatus(vm::ptr<SceKernelThreadRunStatus> pStatus)
{
	sceLibKernel.todo("sceKernelGetThreadRunStatus(pStatus=*0x%x)", pStatus);

	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelGetSystemInfo(vm::ptr<SceKernelSystemInfo> pInfo)
{
	sceLibKernel.todo("sceKernelGetSystemInfo(pInfo=*0x%x)", pInfo);

	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelChangeThreadVfpException(s32 clearMask, s32 setMask)
{
	sceLibKernel.todo("sceKernelChangeThreadVfpException(clearMask=0x%x, setMask=0x%x)", clearMask, setMask);

	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelGetCurrentThreadVfpException()
{
	sceLibKernel.todo("sceKernelGetCurrentThreadVfpException()");

	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelDelayThread(u32 usec)
{
	sceLibKernel.todo("sceKernelDelayThread()");

	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelDelayThreadCB(u32 usec)
{
	sceLibKernel.todo("sceKernelDelayThreadCB()");

	fmt::throw_exception("Unimplemented" HERE);
}

error_code sceKernelWaitThreadEnd(s32 threadId, vm::ptr<s32> pExitStatus, vm::ptr<u32> pTimeout)
{
	sceLibKernel.warning("sceKernelWaitThreadEnd(threadId=0x%x, pExitStatus=*0x%x, pTimeout=*0x%x)", threadId, pExitStatus, pTimeout);

	const auto thread = idm::get<ARMv7Thread>(threadId);

	if (!thread)
	{
		return SCE_KERNEL_ERROR_INVALID_UID;
	}

	if (pTimeout)
	{
	}

	thread->join();

	if (pExitStatus)
	{
		*pExitStatus = thread->GPR[0];
	}

	return SCE_OK;
}

s32 sceKernelWaitThreadEndCB(s32 threadId, vm::ptr<s32> pExitStatus, vm::ptr<u32> pTimeout)
{
	sceLibKernel.todo("sceKernelWaitThreadEndCB(threadId=0x%x, pExitStatus=*0x%x, pTimeout=*0x%x)", threadId, pExitStatus, pTimeout);

	fmt::throw_exception("Unimplemented" HERE);
}

// Callback functions

s32 sceKernelCreateCallback(vm::cptr<char> pName, u32 attr, vm::ptr<SceKernelCallbackFunction> callbackFunc, vm::ptr<void> pCommon)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelDeleteCallback(s32 callbackId)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelNotifyCallback(s32 callbackId, s32 notifyArg)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelCancelCallback(s32 callbackId)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelGetCallbackCount(s32 callbackId)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelCheckCallback()
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelGetCallbackInfo(s32 callbackId, vm::ptr<SceKernelCallbackInfo> pInfo)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelRegisterCallbackToEvent(s32 eventId, s32 callbackId)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelUnregisterCallbackFromEvent(s32 eventId, s32 callbackId)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelUnregisterCallbackFromEventAll(s32 eventId)
{
	fmt::throw_exception("Unimplemented" HERE);
}

// Event functions

s32 sceKernelWaitEvent(s32 eventId, u32 waitPattern, vm::ptr<u32> pResultPattern, vm::ptr<u64> pUserData, vm::ptr<u32> pTimeout)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelWaitEventCB(s32 eventId, u32 waitPattern, vm::ptr<u32> pResultPattern, vm::ptr<u64> pUserData, vm::ptr<u32> pTimeout)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelPollEvent(s32 eventId, u32 bitPattern, vm::ptr<u32> pResultPattern, vm::ptr<u64> pUserData)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelCancelEvent(s32 eventId, vm::ptr<s32> pNumWaitThreads)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelGetEventInfo(s32 eventId, vm::ptr<SceKernelEventInfo> pInfo)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelWaitMultipleEvents(vm::ptr<SceKernelWaitEvent> pWaitEventList, s32 numEvents, u32 waitMode, vm::ptr<SceKernelResultEvent> pResultEventList, vm::ptr<u32> pTimeout)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelWaitMultipleEventsCB(vm::ptr<SceKernelWaitEvent> pWaitEventList, s32 numEvents, u32 waitMode, vm::ptr<SceKernelResultEvent> pResultEventList, vm::ptr<u32> pTimeout)
{
	fmt::throw_exception("Unimplemented" HERE);
}

struct psp2_event_flag final
{
	static const u32 id_base = 1;
	static const u32 id_step = 2;
	static const u32 id_count = 8192;

	using ipc = ipc_manager<psp2_event_flag, std::string>;

	const std::string name;   // IPC/Debug Name
	atomic_t<u32> ipc_ref{1}; // IPC Ref Count

	const u32 attr;
	const u32 init;

	atomic_t<u32> pattern{0}; // Sync variable
	atomic_t<u32> waiters{0}; // Waiter number or waiter id
	atomic_t<u64> wait_ctr;   // FIFO ordering helper

	psp2_event_flag(std::string&& name, u32 attr, u32 pattern)
		: name(std::move(name))
		, attr(attr)
		, init(pattern)
		, pattern(pattern)
	{
	}

	static inline bool pat_test(u32 current, u32 pattern, u32 mode)
	{
		const u32 or_mask = mode & SCE_KERNEL_EVF_WAITMODE_OR ? pattern : 0;
		const u32 and_mask = mode & SCE_KERNEL_EVF_WAITMODE_OR ? 0 : pattern;

		return (current & or_mask) != 0 && (current & and_mask) == and_mask;
	}

	// Get mask for bitwise AND for bit clear operation
	static inline u32 pat_clear(u32 pattern, u32 mode)
	{
		return
			mode & SCE_KERNEL_EVF_WAITMODE_CLEAR_ALL ? 0 :
			mode & SCE_KERNEL_EVF_WAITMODE_CLEAR_PAT ? ~pattern : ~0;
	}

	// Commands
	enum class task : u32
	{
		null,
		wait,    // Check condition, enqueue waiting thread
		poll,    // Check condition only
		set,     // Set pattern bits and wake up threads
		clear,   // Clear pattern bits (bitwise AND)
		cancel,  // Wake up all threads with SCE_KERNEL_ERROR_WAIT_CANCEL
		destroy, // Wake up all threads with SCE_KERNEL_ERROR_WAIT_DELETE
		timeout, // Dequeue waiting thread (cleanup)
		signal,  // Signal selected thread (aux)
	};

	struct alignas(8) cmd_t
	{
		task type;
		u32 arg;
	};

	// Enqueue a command and try to execute all pending commands. Commands are executed sequentially.
	// Returns true if the command has been completed immediately. Its status is unknown otherwise.
	bool exec(task type, u32 arg)
	{
		// Allocate position in the queue
		const u32 push_pos = m_workload.push_begin();

		// Make the command
		cmd_t cmd{type, arg};

		// Get queue head
		u32 pos = m_workload.peek();

		// Check non-optimistic case
		if (UNLIKELY(pos != push_pos))
		{
			// Try to acquire first command in the queue, *then* write current command
			m_workload[push_pos] = std::exchange(cmd, m_workload[pos].exchange({task::null}));
		}

		while (true)
		{
			switch (cmd.type)
			{
			case task::null:
			{
				// Return immediately if can't process a command. Possible reasons:
				// 1) The command has not yet been written
				// 2) The command has already been acquired
				return push_pos < pos;
			}

			case task::wait:    op_wait(cmd.arg); break;
			case task::poll:    op_poll(cmd.arg); break;
			case task::set:     op_set(cmd.arg); break;
			case task::clear:   pattern &= cmd.arg; break;
			case task::cancel:  op_stop(vm::cast(cmd.arg), SCE_KERNEL_ERROR_WAIT_CANCEL); break;
			case task::destroy: op_stop(vm::cast(cmd.arg), SCE_KERNEL_ERROR_WAIT_DELETE); break;

			case task::timeout:
			{
				// Timeout cleanup
				idm::check<ARMv7Thread>(cmd.arg, [&](ARMv7Thread& cpu)
				{
					if (cpu.owner.compare_and_swap_test(this, nullptr))
					{
						cpu.GPR[0] = SCE_KERNEL_ERROR_WAIT_TIMEOUT;
						cpu.GPR[1] = pattern;
						waiters -= attr & SCE_KERNEL_ATTR_MULTI ? 1 : cpu.id;
					}
				});

				break;
			}

			case task::signal:
			{
				idm::check<ARMv7Thread>(cmd.arg, [](auto& cpu)
				{
					cpu.state += cpu_flag::signal;
					cpu.notify();
				});

				break;
			}

			default: ASSUME(0);
			}

			// Get next position
			pos = m_workload.pop_end();

			// Exit after the cleanup
			if (LIKELY(!pos)) return true;

			// Get next command
			cmd = m_workload[pos].exchange({task::null});
		}
	}

	// Enqueue a command and ensure its completion.
	void sync(ARMv7Thread& cpu, task type, u32 arg)
	{
		if (UNLIKELY(!exec(type, arg)))
		{
			if (!exec(task::signal, cpu.id))
			{
				thread_ctrl::wait([&] { return cpu.state.test_and_reset(cpu_flag::signal); });
			}
			else
			{
				cpu.state -= cpu_flag::signal;
			}
		}
	}

private:
	lf_fifo<atomic_t<cmd_t>, 16> m_workload;

	void op_wait(u32 thread_id)
	{
		idm::check<ARMv7Thread>(thread_id, [=](ARMv7Thread& cpu)
		{
			if (attr & SCE_KERNEL_ATTR_MULTI)
			{
				waiters++;
			}
			else if (!waiters.compare_and_swap_test(0, thread_id))
			{
				cpu.GPR[0] = SCE_KERNEL_ERROR_EVF_MULTI;
				cpu.GPR[1] = pattern;
				cpu.state += cpu_flag::signal;
				cpu.notify();
				return;
			}

			const u32 old_pattern = pattern.atomic_op([&](u32& value) -> u32
			{
				const u32 pat = value;
				if (pat_test(pat, cpu.GPR[1], cpu.GPR[0]))
				{
					value &= pat_clear(cpu.GPR[1], cpu.GPR[0]);
					return pat;
				}

				return 0;
			});

			if (old_pattern)
			{
				waiters -= attr & SCE_KERNEL_ATTR_MULTI ? 1 : cpu.id;
				cpu.GPR[0] = SCE_OK;
				cpu.GPR[1] = old_pattern;
				cpu.state += cpu_flag::signal;
				cpu.notify();
			}
			else
			{
				cpu.owner = this;
				cpu.GPR_D[1] = ++wait_ctr;
			}
		});
	}

	void op_poll(u32 thread_id)
	{
		idm::check<ARMv7Thread>(thread_id, [&](ARMv7Thread& cpu)
		{
			cpu.GPR[1] = pattern.atomic_op([&](u32& value) -> u32
			{
				const u32 pat = value;
				if (pat_test(pat, cpu.GPR[1], cpu.GPR[0]))
				{
					value &= pat_clear(cpu.GPR[1], cpu.GPR[0]);
					return pat;
				}

				return 0;
			});
		});
	}

	void op_set(u32 _pattern)
	{
		pattern |= _pattern;

		if (const u32 _waiters = waiters)
		{
			const u32 new_pattern = pattern;

			std::vector<std::reference_wrapper<ARMv7Thread>> threads;

			// Enumerate appropriate threads
			if (attr & SCE_KERNEL_ATTR_MULTI)
			{
				threads.reserve(_waiters);

				idm::select<ARMv7Thread>([&](u32, ARMv7Thread& cpu)
				{
					if (cpu.owner == this && pat_test(new_pattern, cpu.GPR[1], cpu.GPR[0]))
					{
						threads.emplace_back(cpu);
					}
				});

				// Sort the thread list using appropriate scheduling policy
				std::sort(threads.begin(), threads.end(), [&](const ARMv7Thread& cpu1, const ARMv7Thread& cpu2)
				{
					if (attr & SCE_KERNEL_ATTR_TH_PRIO && cpu1.prio != cpu2.prio)
					{
						return cpu1.prio < cpu2.prio;
					}
					else
					{
						return cpu1.GPR_D[1] < cpu2.GPR_D[1];
					}
				});
			}
			else
			{
				idm::check<ARMv7Thread>(_waiters, [&](ARMv7Thread& cpu)
				{
					if (cpu.owner == this && pat_test(new_pattern, cpu.GPR[1], cpu.GPR[0]))
					{
						threads.emplace_back(cpu);
					}
				});
			}

			// Wake up threads
			for (ARMv7Thread& cpu : threads)
			{
				const u32 old_pattern = pattern.atomic_op([&](u32& value) -> u32
				{
					const u32 pat = value;

					if (pat_test(pat, cpu.GPR[1], cpu.GPR[0]))
					{
						value &= pat_clear(cpu.GPR[1], cpu.GPR[0]);
						return pat;
					}

					return 0;
				});

				if (old_pattern)
				{
					cpu.GPR[0] = SCE_OK;
					cpu.GPR[1] = old_pattern;
					cpu.state += cpu_flag::signal;
					cpu.owner = nullptr;
					waiters -= attr & SCE_KERNEL_ATTR_MULTI ? 1 : cpu.id;
					cpu.notify();
				}
			}
		}
	}

	void op_stop(vm::ptr<u32> ptr, s32 error)
	{
		const u32 _pattern = ptr ? ptr->value() : pattern.load();

		std::vector<std::reference_wrapper<ARMv7Thread>> threads;

		idm::select<ARMv7Thread>([&](u32, ARMv7Thread& cpu)
		{
			if (cpu.owner == this)
			{
				threads.emplace_back(cpu);
			}
		});

		if (ptr)
		{
			*ptr = static_cast<u32>(threads.size());
		}

		for (ARMv7Thread& cpu : threads)
		{
			cpu.GPR[0] = error;
			cpu.GPR[1] = _pattern;
			cpu.state += cpu_flag::signal;
			cpu.owner = nullptr;
			cpu.notify();
		}

		pattern = _pattern;
		waiters = 0;
	}
};

template<> DECLARE(psp2_event_flag::ipc::g_ipc) {};

error_code sceKernelCreateEventFlag(vm::cptr<char> pName, u32 attr, u32 initPattern, vm::cptr<SceKernelEventFlagOptParam> pOptParam)
{
	sceLibKernel.error("sceKernelCreateEventFlag(pName=%s, attr=0x%x, initPattern=0x%x, pOptParam=*0x%x)", pName, attr, initPattern, pOptParam);

	// Create EVF
	auto evf = std::make_shared<psp2_event_flag>(pName.get_ptr(), attr, initPattern);

	// Try to register IPC name, only if not empty string (TODO)
	if (evf->name.empty() || !psp2_event_flag::ipc::add(evf->name, [&] { return evf; }))
	{
		evf->ipc_ref = 0;
	}

	// Register ID
	return not_an_error(idm::import_existing(evf));
}

error_code sceKernelDeleteEventFlag(s32 evfId)
{
	sceLibKernel.error("sceKernelDeleteEventFlag(evfId=0x%x)", evfId);

	const auto evf = idm::withdraw<psp2_event_flag>(evfId);

	if (!evf)
	{
		return SCE_KERNEL_ERROR_INVALID_UID;
	}

	// Unregister IPC name
	if (evf->ipc_ref.fetch_op([](u32& ref) { if (ref) ref--; }))
	{
		psp2_event_flag::ipc::remove(evf->name);
	}

	// Finalize the last reference
	if (!evf->ipc_ref)
	{
		evf->exec(psp2_event_flag::task::destroy, 0);
	}

	return SCE_OK;
}

error_code sceKernelOpenEventFlag(vm::cptr<char> pName)
{
	sceLibKernel.error("sceKernelOpenEventFlag(pName=%s)", pName);

	const auto evf = psp2_event_flag::ipc::get(pName.get_ptr());

	// Try to add IPC reference
	if (!evf || !evf->ipc_ref.fetch_op([](u32& ref) { if (ref) ref++; }))
	{
		return SCE_KERNEL_ERROR_UID_CANNOT_FIND_BY_NAME;
	}

	return not_an_error(idm::import_existing(evf));
}

error_code sceKernelCloseEventFlag(s32 evfId)
{
	sceLibKernel.error("sceKernelCloseEventFlag(evfId=0x%x)", evfId);

	const auto evf = idm::withdraw<psp2_event_flag>(evfId);

	if (!evf || !evf->ipc_ref)
	{
		return SCE_KERNEL_ERROR_INVALID_UID;
	}

	// Remove one IPC reference
	if (!evf->ipc_ref.op_fetch([](u32& ref) { if (ref) ref--; }))
	{
		evf->exec(psp2_event_flag::task::destroy, 0);
	}

	return SCE_OK;
}

error_code sceKernelWaitEventFlag(ARMv7Thread& cpu, s32 evfId, u32 bitPattern, u32 waitMode, vm::ptr<u32> pResultPat, vm::ptr<u32> pTimeout)
{
	sceLibKernel.error("sceKernelWaitEventFlag(evfId=0x%x, bitPattern=0x%x, waitMode=0x%x, pResultPat=*0x%x, pTimeout=*0x%x)", evfId, bitPattern, waitMode, pResultPat, pTimeout);

	const u64 start_time = pTimeout ? get_system_time() : 0;
	const u32 timeout = pTimeout ? pTimeout->value() : 0;

	const auto evf = idm::get<psp2_event_flag>(evfId);

	if (!evf)
	{
		return SCE_KERNEL_ERROR_INVALID_UID;
	}

	// First chance (TODO)
	//const auto state = evf->ctrl.fetch_op([&](psp2_event_flag::ctrl_t& state)
	//{
	//	if (!state.waiters && psp2_event_flag::pat_test(state.pattern, bitPattern, waitMode))
	//	{
	//		state.pattern &= psp2_event_flag::pat_clear(bitPattern, waitMode);
	//	}
	//	else if (evf->attr & SCE_KERNEL_ATTR_MULTI)
	//	{
	//		state.waiters++;
	//	}
	//	else if (!state.waiters)
	//	{
	//		state.waiters = cpu.id;
	//	}
	//});

	//if (evf->waiters && !(evf->attr & SCE_KERNEL_ATTR_MULTI))
	//{
	//	return SCE_KERNEL_ERROR_EVF_MULTI;
	//}

	//if (!state.waiters && psp2_event_flag::pat_test(state.pattern, bitPattern, waitMode))
	//{
	//	if (pResultPat) *pResultPat = state.pattern;
	//	return SCE_OK;
	//}

	// Set register values for external use
	cpu.GPR[0] = waitMode;
	cpu.GPR[1] = bitPattern;

	// Second chance
	if (!evf->exec(psp2_event_flag::task::wait, cpu.id) || !cpu.state.test_and_reset(cpu_flag::signal))
	{
		while (!cpu.state.test_and_reset(cpu_flag::signal))
		{
			if (timeout)
			{
				const u64 passed = get_system_time() - start_time;

				if (passed >= timeout)
				{
					if (!evf->exec(psp2_event_flag::task::timeout, cpu.id))
					{
						if (!evf->exec(psp2_event_flag::task::signal, cpu.id))
						{
							thread_ctrl::wait([&] { return cpu.state.test_and_reset(cpu_flag::signal); });
						}
						else
						{
							cpu.state -= cpu_flag::signal;
						}
					}

					break;
				}

				thread_ctrl::wait_for(timeout - passed);
			}
			else
			{
				thread_ctrl::wait();
			}
		}
	}

	if (cpu.GPR[0] == SCE_KERNEL_ERROR_EVF_MULTI)
	{
		return SCE_KERNEL_ERROR_EVF_MULTI;
	}

	if (pResultPat) *pResultPat = cpu.GPR[1];
	if (pTimeout) *pTimeout = static_cast<u32>(std::max<s64>(0, timeout - (get_system_time() - start_time)));
	return not_an_error(cpu.GPR[0]);
}

error_code sceKernelWaitEventFlagCB(ARMv7Thread& cpu, s32 evfId, u32 bitPattern, u32 waitMode, vm::ptr<u32> pResultPat, vm::ptr<u32> pTimeout)
{
	sceLibKernel.todo("sceKernelWaitEventFlagCB(evfId=0x%x, bitPattern=0x%x, waitMode=0x%x, pResultPat=*0x%x, pTimeout=*0x%x)", evfId, bitPattern, waitMode, pResultPat, pTimeout);

	return sceKernelWaitEventFlag(cpu, evfId, bitPattern, waitMode, pResultPat, pTimeout);
}

error_code sceKernelPollEventFlag(ARMv7Thread& cpu, s32 evfId, u32 bitPattern, u32 waitMode, vm::ptr<u32> pResultPat)
{
	sceLibKernel.error("sceKernelPollEventFlag(evfId=0x%x, bitPattern=0x%x, waitMode=0x%x, pResultPat=*0x%x)", evfId, bitPattern, waitMode, pResultPat);

	const auto evf = idm::get<psp2_event_flag>(evfId);

	if (!evf)
	{
		return SCE_KERNEL_ERROR_INVALID_UID;
	}

	// First chance (TODO)
	//const auto state = evf->ctrl.fetch_op([&](psp2_event_flag::ctrl_t& state)
	//{
	//	if (!state.waiters && psp2_event_flag::pat_test(state.pattern, bitPattern, waitMode))
	//	{
	//		state.pattern &= psp2_event_flag::pat_clear(bitPattern, waitMode);
	//	}
	//});

	if (psp2_event_flag::pat_test(evf->pattern, bitPattern, waitMode))
	{
		cpu.GPR[0] = waitMode;
		cpu.GPR[1] = bitPattern;
		
		evf->sync(cpu, psp2_event_flag::task::poll, cpu.id);

		if (cpu.GPR[1])
		{
			*pResultPat = cpu.GPR[1];
			return SCE_OK;
		}
	}

	return not_an_error(SCE_KERNEL_ERROR_EVENT_COND);
}

error_code sceKernelSetEventFlag(s32 evfId, u32 bitPattern)
{
	sceLibKernel.error("sceKernelSetEventFlag(evfId=0x%x, bitPattern=0x%x)", evfId, bitPattern);

	const auto evf = idm::get<psp2_event_flag>(evfId);

	if (!evf)
	{
		return SCE_KERNEL_ERROR_INVALID_UID;
	}

	if (!evf->exec(psp2_event_flag::task::set, bitPattern))
	{
	}

	return SCE_OK;
}

error_code sceKernelClearEventFlag(s32 evfId, u32 bitPattern)
{
	sceLibKernel.error("sceKernelClearEventFlag(evfId=0x%x, bitPattern=0x%x)", evfId, bitPattern);

	const auto evf = idm::get<psp2_event_flag>(evfId);

	if (!evf)
	{
		return SCE_KERNEL_ERROR_INVALID_UID;
	}

	if (!evf->exec(psp2_event_flag::task::clear, bitPattern))
	{
	}

	return SCE_OK;
}

error_code sceKernelCancelEventFlag(ARMv7Thread& cpu, s32 evfId, u32 setPattern, vm::ptr<s32> pNumWaitThreads)
{
	sceLibKernel.error("sceKernelCancelEventFlag(evfId=0x%x, setPattern=0x%x, pNumWaitThreads=*0x%x)", evfId, setPattern, pNumWaitThreads);

	const auto evf = idm::get<psp2_event_flag>(evfId);

	if (!evf)
	{
		return SCE_KERNEL_ERROR_INVALID_UID;
	}

	*pNumWaitThreads = setPattern;

	evf->sync(cpu, psp2_event_flag::task::cancel, pNumWaitThreads.addr());

	return SCE_OK;
}

error_code sceKernelGetEventFlagInfo(s32 evfId, vm::ptr<SceKernelEventFlagInfo> pInfo)
{
	sceLibKernel.error("sceKernelGetEventFlagInfo(evfId=0x%x, pInfo=*0x%x)", evfId, pInfo);

	const auto evf = idm::get<psp2_event_flag>(evfId);

	if (!evf)
	{
		return SCE_KERNEL_ERROR_INVALID_UID;
	}

	pInfo->size = SIZE_32(SceKernelEventFlagInfo);
	pInfo->evfId = evfId;
	
	strcpy_trunc(pInfo->name, evf->name);

	pInfo->attr = evf->attr;
	pInfo->initPattern = evf->init;
	pInfo->currentPattern = evf->pattern;
	pInfo->numWaitThreads = evf->attr & SCE_KERNEL_ATTR_MULTI ? evf->waiters.load() : evf->waiters != 0;

	return SCE_OK;
}

// Semaphore functions

error_code sceKernelCreateSema(vm::cptr<char> pName, u32 attr, s32 initCount, s32 maxCount, vm::cptr<SceKernelSemaOptParam> pOptParam)
{
	sceLibKernel.error("sceKernelCreateSema(pName=%s, attr=0x%x, initCount=%d, maxCount=%d, pOptParam=*0x%x)", pName, attr, initCount, maxCount, pOptParam);

	return not_an_error(idm::make<psp2_semaphore>(pName.get_ptr(), attr, initCount, maxCount));
}

error_code sceKernelDeleteSema(s32 semaId)
{
	sceLibKernel.error("sceKernelDeleteSema(semaId=0x%x)", semaId);

	const auto sema = idm::withdraw<psp2_semaphore>(semaId);

	if (!sema)
	{
		return SCE_KERNEL_ERROR_INVALID_UID;
	}

	// ...

	return SCE_OK;
}

s32 sceKernelOpenSema(vm::cptr<char> pName)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelCloseSema(s32 semaId)
{
	fmt::throw_exception("Unimplemented" HERE);
}

error_code sceKernelWaitSema(s32 semaId, s32 needCount, vm::ptr<u32> pTimeout)
{
	sceLibKernel.error("sceKernelWaitSema(semaId=0x%x, needCount=%d, pTimeout=*0x%x)", semaId, needCount, pTimeout);

	const auto sema = idm::get<psp2_semaphore>(semaId);

	if (!sema)
	{
		return SCE_KERNEL_ERROR_INVALID_UID;
	}

	sceLibKernel.error("*** name = %s", sema->name);
	Emu.Pause();
	return SCE_OK;
}

s32 sceKernelWaitSemaCB(s32 semaId, s32 needCount, vm::ptr<u32> pTimeout)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelPollSema(s32 semaId, s32 needCount)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelSignalSema(s32 semaId, s32 signalCount)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelCancelSema(s32 semaId, s32 setCount, vm::ptr<s32> pNumWaitThreads)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelGetSemaInfo(s32 semaId, vm::ptr<SceKernelSemaInfo> pInfo)
{
	fmt::throw_exception("Unimplemented" HERE);
}

// Mutex functions

error_code sceKernelCreateMutex(vm::cptr<char> pName, u32 attr, s32 initCount, vm::cptr<SceKernelMutexOptParam> pOptParam)
{
	sceLibKernel.error("sceKernelCreateMutex(pName=%s, attr=0x%x, initCount=%d, pOptParam=*0x%x)", pName, attr, initCount, pOptParam);

	return not_an_error(idm::make<psp2_mutex>(pName.get_ptr(), attr, initCount));
}

error_code sceKernelDeleteMutex(s32 mutexId)
{
	sceLibKernel.error("sceKernelDeleteMutex(mutexId=0x%x)", mutexId);

	const auto mutex = idm::withdraw<psp2_mutex>(mutexId);

	if (!mutex)
	{
		return SCE_KERNEL_ERROR_INVALID_UID;
	}

	// ...

	return SCE_OK;
}

s32 sceKernelOpenMutex(vm::cptr<char> pName)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelCloseMutex(s32 mutexId)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelLockMutex(s32 mutexId, s32 lockCount, vm::ptr<u32> pTimeout)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelLockMutexCB(s32 mutexId, s32 lockCount, vm::ptr<u32> pTimeout)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelTryLockMutex(s32 mutexId, s32 lockCount)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelUnlockMutex(s32 mutexId, s32 unlockCount)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelCancelMutex(s32 mutexId, s32 newCount, vm::ptr<s32> pNumWaitThreads)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelGetMutexInfo(s32 mutexId, vm::ptr<SceKernelMutexInfo> pInfo)
{
	fmt::throw_exception("Unimplemented" HERE);
}

// Lightweight mutex functions

s32 sceKernelCreateLwMutex(vm::ptr<SceKernelLwMutexWork> pWork, vm::cptr<char> pName, u32 attr, s32 initCount, vm::cptr<SceKernelLwMutexOptParam> pOptParam)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelDeleteLwMutex(vm::ptr<SceKernelLwMutexWork> pWork)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelLockLwMutex(vm::ptr<SceKernelLwMutexWork> pWork, s32 lockCount, vm::ptr<u32> pTimeout)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelLockLwMutexCB(vm::ptr<SceKernelLwMutexWork> pWork, s32 lockCount, vm::ptr<u32> pTimeout)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelTryLockLwMutex(vm::ptr<SceKernelLwMutexWork> pWork, s32 lockCount)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelUnlockLwMutex(vm::ptr<SceKernelLwMutexWork> pWork, s32 unlockCount)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelGetLwMutexInfo(vm::ptr<SceKernelLwMutexWork> pWork, vm::ptr<SceKernelLwMutexInfo> pInfo)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelGetLwMutexInfoById(s32 lwMutexId, vm::ptr<SceKernelLwMutexInfo> pInfo)
{
	fmt::throw_exception("Unimplemented" HERE);
}

// Condition variable functions

error_code sceKernelCreateCond(vm::cptr<char> pName, u32 attr, s32 mutexId, vm::cptr<SceKernelCondOptParam> pOptParam)
{
	sceLibKernel.error("sceKernelCreateCond(pName=%s, attr=0x%x, mutexId=0x%x, pOptParam=*0x%x)", pName, attr, mutexId, pOptParam);

	const auto mutex = idm::get<psp2_mutex>(mutexId);

	if (!mutex)
	{
		return SCE_KERNEL_ERROR_INVALID_UID;
	}

	return not_an_error(idm::make<psp2_cond>(pName.get_ptr(), attr, mutex));
}

error_code sceKernelDeleteCond(s32 condId)
{
	sceLibKernel.error("sceKernelDeleteCond(condId=0x%x)", condId);

	const auto cond = idm::withdraw<psp2_cond>(condId);

	if (!cond)
	{
		return SCE_KERNEL_ERROR_INVALID_UID;
	}

	// ...

	return SCE_OK;
}

s32 sceKernelOpenCond(vm::cptr<char> pName)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelCloseCond(s32 condId)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelWaitCond(s32 condId, vm::ptr<u32> pTimeout)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelWaitCondCB(s32 condId, vm::ptr<u32> pTimeout)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelSignalCond(s32 condId)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelSignalCondAll(s32 condId)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelSignalCondTo(s32 condId, s32 threadId)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelGetCondInfo(s32 condId, vm::ptr<SceKernelCondInfo> pInfo)
{
	fmt::throw_exception("Unimplemented" HERE);
}

// Lightweight condition variable functions

s32 sceKernelCreateLwCond(vm::ptr<SceKernelLwCondWork> pWork, vm::cptr<char> pName, u32 attr, vm::ptr<SceKernelLwMutexWork> pLwMutex, vm::cptr<SceKernelLwCondOptParam> pOptParam)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelDeleteLwCond(vm::ptr<SceKernelLwCondWork> pWork)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelWaitLwCond(vm::ptr<SceKernelLwCondWork> pWork, vm::ptr<u32> pTimeout)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelWaitLwCondCB(vm::ptr<SceKernelLwCondWork> pWork, vm::ptr<u32> pTimeout)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelSignalLwCond(vm::ptr<SceKernelLwCondWork> pWork)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelSignalLwCondAll(vm::ptr<SceKernelLwCondWork> pWork)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelSignalLwCondTo(vm::ptr<SceKernelLwCondWork> pWork, s32 threadId)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelGetLwCondInfo(vm::ptr<SceKernelLwCondWork> pWork, vm::ptr<SceKernelLwCondInfo> pInfo)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelGetLwCondInfoById(s32 lwCondId, vm::ptr<SceKernelLwCondInfo> pInfo)
{
	fmt::throw_exception("Unimplemented" HERE);
}

// Time functions

s32 sceKernelGetSystemTime(vm::ptr<SceKernelSysClock> pClock)
{
	fmt::throw_exception("Unimplemented" HERE);
}

u64 sceKernelGetSystemTimeWide()
{
	fmt::throw_exception("Unimplemented" HERE);
}

u32 sceKernelGetSystemTimeLow()
{
	fmt::throw_exception("Unimplemented" HERE);
}

// Timer functions

s32 sceKernelCreateTimer(vm::cptr<char> pName, u32 attr, vm::cptr<SceKernelTimerOptParam> pOptParam)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelDeleteTimer(s32 timerId)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelOpenTimer(vm::cptr<char> pName)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelCloseTimer(s32 timerId)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelStartTimer(s32 timerId)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelStopTimer(s32 timerId)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelGetTimerBase(s32 timerId, vm::ptr<SceKernelSysClock> pBase)
{
	fmt::throw_exception("Unimplemented" HERE);
}

u64 sceKernelGetTimerBaseWide(s32 timerId)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelGetTimerTime(s32 timerId, vm::ptr<SceKernelSysClock> pClock)
{
	fmt::throw_exception("Unimplemented" HERE);
}

u64 sceKernelGetTimerTimeWide(s32 timerId)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelSetTimerTime(s32 timerId, vm::ptr<SceKernelSysClock> pClock)
{
	fmt::throw_exception("Unimplemented" HERE);
}

u64 sceKernelSetTimerTimeWide(s32 timerId, u64 clock)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelSetTimerEvent(s32 timerId, s32 type, vm::ptr<SceKernelSysClock> pInterval, s32 fRepeat)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelCancelTimer(s32 timerId, vm::ptr<s32> pNumWaitThreads)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelGetTimerInfo(s32 timerId, vm::ptr<SceKernelTimerInfo> pInfo)
{
	fmt::throw_exception("Unimplemented" HERE);
}

// Reader/writer lock functions

s32 sceKernelCreateRWLock(vm::cptr<char> pName, u32 attr, vm::cptr<SceKernelRWLockOptParam> pOptParam)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelDeleteRWLock(s32 rwLockId)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelOpenRWLock(vm::cptr<char> pName)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelCloseRWLock(s32 rwLockId)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelLockReadRWLock(s32 rwLockId, vm::ptr<u32> pTimeout)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelLockReadRWLockCB(s32 rwLockId, vm::ptr<u32> pTimeout)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelTryLockReadRWLock(s32 rwLockId)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelUnlockReadRWLock(s32 rwLockId)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelLockWriteRWLock(s32 rwLockId, vm::ptr<u32> pTimeout)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelLockWriteRWLockCB(s32 rwLockId, vm::ptr<u32> pTimeout)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelTryLockWriteRWLock(s32 rwLockId)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelUnlockWriteRWLock(s32 rwLockId)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelCancelRWLock(s32 rwLockId, vm::ptr<s32> pNumReadWaitThreads, vm::ptr<s32> pNumWriteWaitThreads, s32 flag)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceKernelGetRWLockInfo(s32 rwLockId, vm::ptr<SceKernelRWLockInfo> pInfo)
{
	fmt::throw_exception("Unimplemented" HERE);
}

error_code sceKernelGetThreadmgrUIDClass(s32 uid)
{
	sceLibKernel.error("sceKernelGetThreadmgrUIDClass(uid=0x%x)", uid);

	if (idm::check<ARMv7Thread>(uid)) return not_an_error(SCE_KERNEL_THREADMGR_UID_CLASS_THREAD);
	if (idm::check<psp2_semaphore>(uid)) return not_an_error(SCE_KERNEL_THREADMGR_UID_CLASS_SEMA);
	if (idm::check<psp2_event_flag>(uid)) return not_an_error(SCE_KERNEL_THREADMGR_UID_CLASS_EVENT_FLAG);
	if (idm::check<psp2_mutex>(uid)) return not_an_error(SCE_KERNEL_THREADMGR_UID_CLASS_MUTEX);
	if (idm::check<psp2_cond>(uid)) return not_an_error(SCE_KERNEL_THREADMGR_UID_CLASS_COND);

	return SCE_KERNEL_ERROR_INVALID_UID;
}

// IO/File functions

s32 sceIoRemove(vm::cptr<char> filename)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceIoMkdir(vm::cptr<char> dirname, s32 mode)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceIoRmdir(vm::cptr<char> dirname)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceIoRename(vm::cptr<char> oldname, vm::cptr<char> newname)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceIoDevctl(vm::cptr<char> devname, s32 cmd, vm::cptr<void> arg, u32 arglen, vm::ptr<void> bufp, u32 buflen)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceIoSync(vm::cptr<char> devname, s32 flag)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceIoOpen(vm::cptr<char> filename, s32 flag, s32 mode)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceIoClose(s32 fd)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceIoIoctl(s32 fd, s32 cmd, vm::cptr<void> argp, u32 arglen, vm::ptr<void> bufp, u32 buflen)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s64 sceIoLseek(s32 fd, s64 offset, s32 whence)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceIoLseek32(s32 fd, s32 offset, s32 whence)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceIoRead(s32 fd, vm::ptr<void> buf, u32 nbyte)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceIoWrite(s32 fd, vm::cptr<void> buf, u32 nbyte)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceIoPread(s32 fd, vm::ptr<void> buf, u32 nbyte, s64 offset)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceIoPwrite(s32 fd, vm::cptr<void> buf, u32 nbyte, s64 offset)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceIoDopen(vm::cptr<char> dirname)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceIoDclose(s32 fd)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceIoDread(s32 fd, vm::ptr<SceIoDirent> buf)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceIoChstat(vm::cptr<char> name, vm::cptr<SceIoStat> buf, u32 cbit)
{
	fmt::throw_exception("Unimplemented" HERE);
}

s32 sceIoGetstat(vm::cptr<char> name, vm::ptr<SceIoStat> buf)
{
	fmt::throw_exception("Unimplemented" HERE);
}


#define REG_FUNC(nid, name) REG_FNID(SceLibKernel, nid, name)

DECLARE(arm_module_manager::SceLibKernel)("SceLibKernel", []()
{
	// REG_FUNC(???, sceKernelGetEventInfo);

	//REG_FUNC(0x023EAA62, sceKernelPuts);
	//REG_FUNC(0xB0335388, sceClibToupper);
	//REG_FUNC(0x4C5471BC, sceClibTolower);
	//REG_FUNC(0xD8EBBB7E, sceClibLookCtypeTable);
	//REG_FUNC(0x20EC3210, sceClibGetCtypeTable);
	//REG_FUNC(0x407D6153, sceClibMemchr);
	//REG_FUNC(0x9CC2BFDF, sceClibMemcmp);
	//REG_FUNC(0x14E9DBD7, sceClibMemcpy);
	//REG_FUNC(0x736753C8, sceClibMemmove);
	//REG_FUNC(0x632980D7, sceClibMemset);
	//REG_FUNC(0xFA26BC62, sceClibPrintf);
	//REG_FUNC(0x5EA3B6CE, sceClibVprintf);
	//REG_FUNC(0x8CBA03D5, sceClibSnprintf);
	//REG_FUNC(0xFA6BE467, sceClibVsnprintf);
	//REG_FUNC(0xA2FB4D9D, sceClibStrcmp);
	//REG_FUNC(0x70CBC2D5, sceClibStrlcat);
	//REG_FUNC(0x2CDFCD1C, sceClibStrlcpy);
	//REG_FUNC(0xA37E6383, sceClibStrncat);
	//REG_FUNC(0x660D1F6D, sceClibStrncmp);
	//REG_FUNC(0xC458D60A, sceClibStrncpy);
	//REG_FUNC(0xAC595E68, sceClibStrnlen);
	//REG_FUNC(0x614076B7, sceClibStrchr);
	//REG_FUNC(0x6E728AAE, sceClibStrrchr);
	//REG_FUNC(0xE265498B, sceClibStrstr);
	//REG_FUNC(0xB54C0BE4, sceClibStrncasecmp);
	//REG_FUNC(0x2F2C6046, sceClibAbort);
	//REG_FUNC(0x2E581B88, sceClibStrtoll);
	//REG_FUNC(0x3B9E301A, sceClibMspaceCreate);
	//REG_FUNC(0xAE1A21EC, sceClibMspaceDestroy);
	//REG_FUNC(0x86EF7680, sceClibMspaceMalloc);
	//REG_FUNC(0x9C56B4D1, sceClibMspaceFree);
	//REG_FUNC(0x678374AD, sceClibMspaceCalloc);
	//REG_FUNC(0x3C847D57, sceClibMspaceMemalign);
	//REG_FUNC(0x774891D6, sceClibMspaceRealloc);
	//REG_FUNC(0x586AC68D, sceClibMspaceReallocalign);
	//REG_FUNC(0x46A02279, sceClibMspaceMallocUsableSize);
	//REG_FUNC(0x8CC1D38E, sceClibMspaceMallocStats);
	//REG_FUNC(0x738E0322, sceClibMspaceMallocStatsFast);
	//REG_FUNC(0xD1D59701, sceClibMspaceIsHeapEmpty);
	//REG_FUNC(0xE960FDA2, sceKernelAtomicSet8);
	//REG_FUNC(0x450BFECF, sceKernelAtomicSet16);
	//REG_FUNC(0xB69DA09B, sceKernelAtomicSet32);
	//REG_FUNC(0xC8A4339C, sceKernelAtomicSet64);
	//REG_FUNC(0x27A2AAFA, sceKernelAtomicGetAndAdd8);
	//REG_FUNC(0x5674DB0C, sceKernelAtomicGetAndAdd16);
	//REG_FUNC(0x2611CB0B, sceKernelAtomicGetAndAdd32);
	//REG_FUNC(0x63DAF37D, sceKernelAtomicGetAndAdd64);
	//REG_FUNC(0x8F7BD940, sceKernelAtomicAddAndGet8);
	//REG_FUNC(0x495C52EC, sceKernelAtomicAddAndGet16);
	//REG_FUNC(0x2E84A93B, sceKernelAtomicAddAndGet32);
	//REG_FUNC(0xB6CE9B9A, sceKernelAtomicAddAndGet64);
	//REG_FUNC(0xCDF5DF67, sceKernelAtomicGetAndSub8);
	//REG_FUNC(0xAC51979C, sceKernelAtomicGetAndSub16);
	//REG_FUNC(0x115C516F, sceKernelAtomicGetAndSub32);
	//REG_FUNC(0x4AE9C8E6, sceKernelAtomicGetAndSub64);
	//REG_FUNC(0x99E1796E, sceKernelAtomicSubAndGet8);
	//REG_FUNC(0xC26BBBB1, sceKernelAtomicSubAndGet16);
	//REG_FUNC(0x01C9CD92, sceKernelAtomicSubAndGet32);
	//REG_FUNC(0x9BB4A94B, sceKernelAtomicSubAndGet64);
	//REG_FUNC(0x53DCA02B, sceKernelAtomicGetAndAnd8);
	//REG_FUNC(0x7A0CB056, sceKernelAtomicGetAndAnd16);
	//REG_FUNC(0x08266595, sceKernelAtomicGetAndAnd32);
	//REG_FUNC(0x4828BC43, sceKernelAtomicGetAndAnd64);
	//REG_FUNC(0x86B9170F, sceKernelAtomicAndAndGet8);
	//REG_FUNC(0xF9890F7E, sceKernelAtomicAndAndGet16);
	//REG_FUNC(0x6709D30C, sceKernelAtomicAndAndGet32);
	//REG_FUNC(0xAED2B370, sceKernelAtomicAndAndGet64);
	//REG_FUNC(0x107A68DF, sceKernelAtomicGetAndOr8);
	//REG_FUNC(0x31E49E73, sceKernelAtomicGetAndOr16);
	//REG_FUNC(0x984AD276, sceKernelAtomicGetAndOr32);
	//REG_FUNC(0xC39186CD, sceKernelAtomicGetAndOr64);
	//REG_FUNC(0x51693931, sceKernelAtomicOrAndGet8);
	//REG_FUNC(0x8E248EBD, sceKernelAtomicOrAndGet16);
	//REG_FUNC(0xC3B2F7F8, sceKernelAtomicOrAndGet32);
	//REG_FUNC(0x809BBC7D, sceKernelAtomicOrAndGet64);
	//REG_FUNC(0x7350B2DF, sceKernelAtomicGetAndXor8);
	//REG_FUNC(0x6E2D0B9E, sceKernelAtomicGetAndXor16);
	//REG_FUNC(0x38739E2F, sceKernelAtomicGetAndXor32);
	//REG_FUNC(0x6A19BBE9, sceKernelAtomicGetAndXor64);
	//REG_FUNC(0x634AF062, sceKernelAtomicXorAndGet8);
	//REG_FUNC(0x6F524195, sceKernelAtomicXorAndGet16);
	//REG_FUNC(0x46940704, sceKernelAtomicXorAndGet32);
	//REG_FUNC(0xDDC6866E, sceKernelAtomicXorAndGet64);
	//REG_FUNC(0x327DB4C0, sceKernelAtomicCompareAndSet8);
	//REG_FUNC(0xE8C01236, sceKernelAtomicCompareAndSet16);
	//REG_FUNC(0x1124A1D4, sceKernelAtomicCompareAndSet32);
	//REG_FUNC(0x1EBDFCCD, sceKernelAtomicCompareAndSet64);
	//REG_FUNC(0xD7D49E36, sceKernelAtomicClearMask8);
	//REG_FUNC(0x5FE7DFF8, sceKernelAtomicClearMask16);
	//REG_FUNC(0xE3DF0CB3, sceKernelAtomicClearMask32);
	//REG_FUNC(0x953D118A, sceKernelAtomicClearMask64);
	//REG_FUNC(0x7FD94393, sceKernelAtomicAddUnless8);
	//REG_FUNC(0x1CF4AA4B, sceKernelAtomicAddUnless16);
	//REG_FUNC(0x4B33FD3C, sceKernelAtomicAddUnless32);
	//REG_FUNC(0xFFCE7438, sceKernelAtomicAddUnless64);
	//REG_FUNC(0x9DABE6C3, sceKernelAtomicDecIfPositive8);
	//REG_FUNC(0x323718FB, sceKernelAtomicDecIfPositive16);
	//REG_FUNC(0xCA3294F1, sceKernelAtomicDecIfPositive32);
	//REG_FUNC(0x8BE2A007, sceKernelAtomicDecIfPositive64);
	//REG_FUNC(0xBBE82155, sceKernelLoadModule);
	//REG_FUNC(0x2DCC4AFA, sceKernelLoadStartModule);
	//REG_FUNC(0x702425D5, sceKernelStartModule);
	//REG_FUNC(0x3B2CBA09, sceKernelStopModule);
	//REG_FUNC(0x1987920E, sceKernelUnloadModule);
	//REG_FUNC(0x2415F8A4, sceKernelStopUnloadModule);
	//REG_FUNC(0x15E2A45D, sceKernelCallModuleExit);
	//REG_FUNC(0xD11A5103, sceKernelGetModuleInfoByAddr);
	//REG_FUNC(0x4F2D8B15, sceKernelOpenModule);
	//REG_FUNC(0x657FA50E, sceKernelCloseModule);
	//REG_FUNC(0x7595D9AA, sceKernelExitProcess);
	//REG_FUNC(0x4C7AD128, sceKernelPowerLock);
	//REG_FUNC(0xAF8E9C11, sceKernelPowerUnlock);
	//REG_FUNC(0xB295EB61, sceKernelGetTLSAddr);
	REG_FUNC(0x0FB972F9, sceKernelGetThreadId);
	REG_FUNC(0xA37A6057, sceKernelGetCurrentThreadVfpException);
	//REG_FUNC(0x0CA71EA2, sceKernelSendMsgPipe);
	//REG_FUNC(0xA5CA74AC, sceKernelSendMsgPipeCB);
	//REG_FUNC(0xDFC670E0, sceKernelTrySendMsgPipe);
	//REG_FUNC(0x4E81DD5C, sceKernelReceiveMsgPipe);
	//REG_FUNC(0x33AF829B, sceKernelReceiveMsgPipeCB);
	//REG_FUNC(0x5615B006, sceKernelTryReceiveMsgPipe);
	REG_FUNC(0xA7819967, sceKernelLockLwMutex);
	REG_FUNC(0x6F9C4CC1, sceKernelLockLwMutexCB);
	REG_FUNC(0x9EF798C1, sceKernelTryLockLwMutex);
	REG_FUNC(0x499EA781, sceKernelUnlockLwMutex);
	REG_FUNC(0xF7D8F1FC, sceKernelGetLwMutexInfo);
	REG_FUNC(0xDDB395A9, sceKernelWaitThreadEnd);
	REG_FUNC(0xC54941ED, sceKernelWaitThreadEndCB);
	REG_FUNC(0xD5DC26C4, sceKernelGetThreadExitStatus);
	//REG_FUNC(0x4373B548, __sce_aeabi_idiv0);
	//REG_FUNC(0xFB235848, __sce_aeabi_ldiv0);
	REG_FUNC(0xF08DE149, sceKernelStartThread);
	REG_FUNC(0x58DDAC4F, sceKernelDeleteThread);
	REG_FUNC(0x5150577B, sceKernelChangeThreadCpuAffinityMask);
	REG_FUNC(0x8C57AC2A, sceKernelGetThreadCpuAffinityMask);
	REG_FUNC(0xDF7E6EDA, sceKernelChangeThreadPriority);
	//REG_FUNC(0xBCB63B66, sceKernelGetThreadStackFreeSize);
	REG_FUNC(0x8D9C5461, sceKernelGetThreadInfo);
	REG_FUNC(0xD6B01013, sceKernelGetThreadRunStatus);
	REG_FUNC(0xE0241FAA, sceKernelGetSystemInfo);
	REG_FUNC(0xF994FE65, sceKernelGetThreadmgrUIDClass);
	//REG_FUNC(0xB4DE10C7, sceKernelGetActiveCpuMask);
	REG_FUNC(0x2C1321A3, sceKernelChangeThreadVfpException);
	REG_FUNC(0x3849359A, sceKernelCreateCallback);
	REG_FUNC(0x88DD1BC8, sceKernelGetCallbackInfo);
	REG_FUNC(0x464559D3, sceKernelDeleteCallback);
	REG_FUNC(0xBD9C8F2B, sceKernelNotifyCallback);
	REG_FUNC(0x3137A687, sceKernelCancelCallback);
	REG_FUNC(0x76A2EF81, sceKernelGetCallbackCount);
	REG_FUNC(0xD4F75281, sceKernelRegisterCallbackToEvent);
	REG_FUNC(0x8D3940DF, sceKernelUnregisterCallbackFromEvent);
	REG_FUNC(0x2BD1E682, sceKernelUnregisterCallbackFromEventAll);
	REG_FUNC(0x120F03AF, sceKernelWaitEvent);
	REG_FUNC(0xA0490795, sceKernelWaitEventCB);
	REG_FUNC(0x241F3634, sceKernelPollEvent);
	REG_FUNC(0x603AB770, sceKernelCancelEvent);
	REG_FUNC(0x10586418, sceKernelWaitMultipleEvents);
	REG_FUNC(0x4263DBC9, sceKernelWaitMultipleEventsCB);
	REG_FUNC(0x8516D040, sceKernelCreateEventFlag);
	REG_FUNC(0x11FE9B8B, sceKernelDeleteEventFlag);
	REG_FUNC(0xE04EC73A, sceKernelOpenEventFlag);
	REG_FUNC(0x9C0B8285, sceKernelCloseEventFlag);
	REG_FUNC(0x83C0E2AF, sceKernelWaitEventFlag);
	REG_FUNC(0xE737B1DF, sceKernelWaitEventFlagCB);
	REG_FUNC(0x1FBB0FE1, sceKernelPollEventFlag);
	REG_FUNC(0x2A12D9B7, sceKernelCancelEventFlag);
	REG_FUNC(0x8BA4C0C1, sceKernelGetEventFlagInfo);
	REG_FUNC(0x9EF9C0C5, sceKernelSetEventFlag);
	REG_FUNC(0xD018793F, sceKernelClearEventFlag);
	REG_FUNC(0x297AA2AE, sceKernelCreateSema);
	REG_FUNC(0xC08F5BC5, sceKernelDeleteSema);
	REG_FUNC(0xB028AB78, sceKernelOpenSema);
	REG_FUNC(0x817707AB, sceKernelCloseSema);
	REG_FUNC(0x0C7B834B, sceKernelWaitSema);
	REG_FUNC(0x174692B4, sceKernelWaitSemaCB);
	REG_FUNC(0x66D6BF05, sceKernelCancelSema);
	REG_FUNC(0x595D3FA6, sceKernelGetSemaInfo);
	REG_FUNC(0x3012A9C6, sceKernelPollSema);
	REG_FUNC(0x2053A496, sceKernelSignalSema);
	REG_FUNC(0xED53334A, sceKernelCreateMutex);
	REG_FUNC(0x12D11F65, sceKernelDeleteMutex);
	REG_FUNC(0x16B85235, sceKernelOpenMutex);
	REG_FUNC(0x43DDC9CC, sceKernelCloseMutex);
	REG_FUNC(0x1D8D7945, sceKernelLockMutex);
	REG_FUNC(0x2BDAA524, sceKernelLockMutexCB);
	REG_FUNC(0x2144890D, sceKernelCancelMutex);
	REG_FUNC(0x9A6C43CA, sceKernelGetMutexInfo);
	REG_FUNC(0xE5901FF9, sceKernelTryLockMutex);
	REG_FUNC(0x34746309, sceKernelUnlockMutex);
	REG_FUNC(0x50572FDA, sceKernelCreateCond);
	REG_FUNC(0xFD295414, sceKernelDeleteCond);
	REG_FUNC(0xCB2A73A9, sceKernelOpenCond);
	REG_FUNC(0x4FB91A89, sceKernelCloseCond);
	REG_FUNC(0xC88D44AD, sceKernelWaitCond);
	REG_FUNC(0x4CE42CE2, sceKernelWaitCondCB);
	REG_FUNC(0x6864DCE2, sceKernelGetCondInfo);
	REG_FUNC(0x10A4976F, sceKernelSignalCond);
	REG_FUNC(0x2EB86929, sceKernelSignalCondAll);
	REG_FUNC(0x087629E6, sceKernelSignalCondTo);
	//REG_FUNC(0x0A10C1C8, sceKernelCreateMsgPipe);
	//REG_FUNC(0x69F6575D, sceKernelDeleteMsgPipe);
	//REG_FUNC(0x230691DA, sceKernelOpenMsgPipe);
	//REG_FUNC(0x7E5C0C16, sceKernelCloseMsgPipe);
	//REG_FUNC(0x94D506F7, sceKernelSendMsgPipeVector);
	//REG_FUNC(0x9C6F7F79, sceKernelSendMsgPipeVectorCB);
	//REG_FUNC(0x60DB346F, sceKernelTrySendMsgPipeVector);
	//REG_FUNC(0x9F899087, sceKernelReceiveMsgPipeVector);
	//REG_FUNC(0xBE5B3E27, sceKernelReceiveMsgPipeVectorCB);
	//REG_FUNC(0x86ECC0FF, sceKernelTryReceiveMsgPipeVector);
	//REG_FUNC(0xEF14BA37, sceKernelCancelMsgPipe);
	//REG_FUNC(0x4046D16B, sceKernelGetMsgPipeInfo);
	REG_FUNC(0xDA6EC8EF, sceKernelCreateLwMutex);
	REG_FUNC(0x244E76D2, sceKernelDeleteLwMutex);
	REG_FUNC(0x4846613D, sceKernelGetLwMutexInfoById);
	REG_FUNC(0x48C7EAE6, sceKernelCreateLwCond);
	REG_FUNC(0x721F6CB3, sceKernelDeleteLwCond);
	REG_FUNC(0xE1878282, sceKernelWaitLwCond);
	REG_FUNC(0x8FA54B07, sceKernelWaitLwCondCB);
	REG_FUNC(0x3AC63B9A, sceKernelSignalLwCond);
	REG_FUNC(0xE5241A0C, sceKernelSignalLwCondAll);
	REG_FUNC(0xFC1A48EB, sceKernelSignalLwCondTo);
	REG_FUNC(0xE4DF36A0, sceKernelGetLwCondInfo);
	REG_FUNC(0x971F1DE8, sceKernelGetLwCondInfoById);
	REG_FUNC(0x2255B2A5, sceKernelCreateTimer);
	REG_FUNC(0x746F3290, sceKernelDeleteTimer);
	REG_FUNC(0x2F3D35A3, sceKernelOpenTimer);
	REG_FUNC(0x17283DE6, sceKernelCloseTimer);
	REG_FUNC(0x1478249B, sceKernelStartTimer);
	REG_FUNC(0x075B1329, sceKernelStopTimer);
	REG_FUNC(0x1F59E04D, sceKernelGetTimerBase);
	REG_FUNC(0x3223CCD1, sceKernelGetTimerBaseWide);
	REG_FUNC(0x381DC300, sceKernelGetTimerTime);
	REG_FUNC(0x53C5D833, sceKernelGetTimerTimeWide);
	REG_FUNC(0xFFAD717F, sceKernelSetTimerTime);
	REG_FUNC(0xAF67678B, sceKernelSetTimerTimeWide);
	REG_FUNC(0x621D293B, sceKernelSetTimerEvent);
	REG_FUNC(0x9CCF768C, sceKernelCancelTimer);
	REG_FUNC(0x7E35E10A, sceKernelGetTimerInfo);
	REG_FUNC(0x8667951D, sceKernelCreateRWLock);
	REG_FUNC(0x3D750204, sceKernelDeleteRWLock);
	REG_FUNC(0xBA4DAC9A, sceKernelOpenRWLock);
	REG_FUNC(0xA7F94E64, sceKernelCloseRWLock);
	REG_FUNC(0xFA670F0F, sceKernelLockReadRWLock);
	REG_FUNC(0x2D4A62B7, sceKernelLockReadRWLockCB);
	REG_FUNC(0x1B8586C0, sceKernelTryLockReadRWLock);
	REG_FUNC(0x675D10A8, sceKernelUnlockReadRWLock);
	REG_FUNC(0x67A187BB, sceKernelLockWriteRWLock);
	REG_FUNC(0xA4777082, sceKernelLockWriteRWLockCB);
	REG_FUNC(0x597D4607, sceKernelTryLockWriteRWLock);
	REG_FUNC(0xD9369DF2, sceKernelUnlockWriteRWLock);
	REG_FUNC(0x190CA94B, sceKernelCancelRWLock);
	REG_FUNC(0x079A573B, sceKernelGetRWLockInfo);
	REG_FUNC(0x8AF15B5F, sceKernelGetSystemTime);
	//REG_FUNC(0x99B2BF15, sceKernelPMonThreadGetCounter);
	//REG_FUNC(0x7C21C961, sceKernelPMonCpuGetCounter);
	//REG_FUNC(0xADCA94E5, sceKernelWaitSignal);
	//REG_FUNC(0x24460BB3, sceKernelWaitSignalCB);
	//REG_FUNC(0x7BE9C4C8, sceKernelSendSignal);
	REG_FUNC(0xC5C11EE7, sceKernelCreateThread);
	REG_FUNC(0x6C60AC61, sceIoOpen);
	REG_FUNC(0xF5C6F098, sceIoClose);
	REG_FUNC(0x713523E1, sceIoRead);
	REG_FUNC(0x11FED231, sceIoWrite);
	REG_FUNC(0x99BA173E, sceIoLseek);
	REG_FUNC(0x5CC983AC, sceIoLseek32);
	REG_FUNC(0xE20ED0F3, sceIoRemove);
	REG_FUNC(0xF737E369, sceIoRename);
	REG_FUNC(0x9670D39F, sceIoMkdir);
	REG_FUNC(0xE9F91EC8, sceIoRmdir);
	REG_FUNC(0xA9283DD0, sceIoDopen);
	REG_FUNC(0x9DFF9C59, sceIoDclose);
	REG_FUNC(0x9C8B6624, sceIoDread);
	REG_FUNC(0xBCA5B623, sceIoGetstat);
	REG_FUNC(0x29482F7F, sceIoChstat);
	REG_FUNC(0x98ACED6D, sceIoSync);
	REG_FUNC(0x04B30CB2, sceIoDevctl);
	REG_FUNC(0x54ABACFA, sceIoIoctl);
	//REG_FUNC(0x6A7EA9FD, sceIoOpenAsync);
	//REG_FUNC(0x84201C9B, sceIoCloseAsync);
	//REG_FUNC(0x7B3BE857, sceIoReadAsync);
	//REG_FUNC(0x21329B20, sceIoWriteAsync);
	//REG_FUNC(0xCAC5D672, sceIoLseekAsync);
	//REG_FUNC(0x099C54B9, sceIoIoctlAsync);
	//REG_FUNC(0x446A60AC, sceIoRemoveAsync);
	//REG_FUNC(0x73FC184B, sceIoDopenAsync);
	//REG_FUNC(0x4D0597D7, sceIoDcloseAsync);
	//REG_FUNC(0xCE32490D, sceIoDreadAsync);
	//REG_FUNC(0x8E5FCBB1, sceIoMkdirAsync);
	//REG_FUNC(0x9694D00F, sceIoRmdirAsync);
	//REG_FUNC(0xEE9857CD, sceIoRenameAsync);
	//REG_FUNC(0x9739A5E2, sceIoChstatAsync);
	//REG_FUNC(0x82B20B41, sceIoGetstatAsync);
	//REG_FUNC(0x950F78EB, sceIoDevctlAsync);
	//REG_FUNC(0xF7C7FBFE, sceIoSyncAsync);
	//REG_FUNC(0xEC96EA71, sceIoCancel);
	//REG_FUNC(0x857E0C71, sceIoComplete);
	REG_FUNC(0x52315AD7, sceIoPread);
	REG_FUNC(0x8FFFF5A8, sceIoPwrite);
	//REG_FUNC(0xA010141E, sceIoPreadAsync);
	//REG_FUNC(0xED25BEEF, sceIoPwriteAsync);
	//REG_FUNC(0xA792C404, sceIoCompleteMultiple);
	//REG_FUNC(0x894037E8, sceKernelBacktrace);
	//REG_FUNC(0x20E2D4B7, sceKernelPrintBacktrace);
	//REG_FUNC(0x963F4A99, sceSblACMgrIsGameProgram);
	//REG_FUNC(0x261E2C34, sceKernelGetOpenPsId);

	//REG_FUNC(0x4C4672BF, sceKernelGetProcessTime); // !!!
});

DECLARE(arm_module_manager::SceIofilemgr)("SceIofilemgr", []()
{
	REG_FNID(SceIofilemgr, 0x34EFD876, sceIoWrite); // !!!
	REG_FNID(SceIofilemgr, 0xC70B8886, sceIoClose); // !!!
	REG_FNID(SceIofilemgr, 0xFDB32293, sceIoRead); // !!!
});

DECLARE(arm_module_manager::SceModulemgr)("SceModulemgr", []()
{
	//REG_FNID(SceModulemgr, 0x36585DAF, sceKernelGetModuleInfo);
	//REG_FNID(SceModulemgr, 0x2EF2581F, sceKernelGetModuleList);
	//REG_FNID(SceModulemgr, 0xF5798C7C, sceKernelGetModuleIdByAddr);
});

DECLARE(arm_module_manager::SceProcessmgr)("SceProcessmgr", []()
{
	//REG_FNID(SceProcessmgr, 0xCD248267, sceKernelGetCurrentProcess);
	//REG_FNID(SceProcessmgr, 0x2252890C, sceKernelPowerTick);
	//REG_FNID(SceProcessmgr, 0x9E45DA09, sceKernelLibcClock);
	//REG_FNID(SceProcessmgr, 0x0039BE45, sceKernelLibcTime);
	//REG_FNID(SceProcessmgr, 0x4B879059, sceKernelLibcGettimeofday);
	//REG_FNID(SceProcessmgr, 0xC1727F59, sceKernelGetStdin);
	//REG_FNID(SceProcessmgr, 0xE5AA625C, sceKernelGetStdout);
	//REG_FNID(SceProcessmgr, 0xFA5E3ADA, sceKernelGetStderr);
	//REG_FNID(SceProcessmgr, 0xE6E9FCA3, sceKernelGetRemoteProcessTime);
	//REG_FNID(SceProcessmgr, 0xD37A8437, sceKernelGetProcessTime);
	//REG_FNID(SceProcessmgr, 0xF5D0D4C6, sceKernelGetProcessTimeLow);
	//REG_FNID(SceProcessmgr, 0x89DA0967, sceKernelGetProcessTimeWide);
	//REG_FNID(SceProcessmgr, 0x2BE3E066, sceKernelGetProcessParam);
});

DECLARE(arm_module_manager::SceStdio)("SceStdio", []()
{
	//REG_FNID(SceStdio, 0x54237407, sceKernelStdin);
	//REG_FNID(SceStdio, 0x9033E9BD, sceKernelStdout);
	//REG_FNID(SceStdio, 0x35EE7CF5, sceKernelStderr);
});

DECLARE(arm_module_manager::SceSysmem)("SceSysmem", []()
{
	REG_FNID(SceSysmem, 0xB9D5EBDE, sceKernelAllocMemBlock);
	REG_FNID(SceSysmem, 0xA91E15EE, sceKernelFreeMemBlock);
	REG_FNID(SceSysmem, 0xB8EF5818, sceKernelGetMemBlockBase);
	//REG_FNID(SceSysmem, 0x3B29E0F5, sceKernelRemapMemBlock);
	//REG_FNID(SceSysmem, 0xA33B99D1, sceKernelFindMemBlockByAddr);
	REG_FNID(SceSysmem, 0x4010AD65, sceKernelGetMemBlockInfoByAddr);
});

DECLARE(arm_module_manager::SceCpu)("SceCpu", []()
{
	//REG_FNID(SceCpu, 0x2704CFEE, sceKernelCpuId);
});

DECLARE(arm_module_manager::SceDipsw)("SceDipsw", []()
{
	//REG_FNID(SceDipsw, 0x1C783FB2, sceKernelCheckDipsw);
	//REG_FNID(SceDipsw, 0x817053D4, sceKernelSetDipsw);
	//REG_FNID(SceDipsw, 0x800EDCC1, sceKernelClearDipsw);
});

DECLARE(arm_module_manager::SceThreadmgr)("SceThreadmgr", []()
{
	REG_FNID(SceThreadmgr, 0x0C8A38E1, sceKernelExitThread);
	REG_FNID(SceThreadmgr, 0x1D17DECF, sceKernelExitDeleteThread);
	REG_FNID(SceThreadmgr, 0x4B675D05, sceKernelDelayThread);
	REG_FNID(SceThreadmgr, 0x9C0180E1, sceKernelDelayThreadCB);
	REG_FNID(SceThreadmgr, 0x1BBDE3D9, sceKernelDeleteThread); // !!!
	//REG_FNID(SceThreadmgr, 0x001173F8, sceKernelChangeActiveCpuMask);
	REG_FNID(SceThreadmgr, 0x01414F0B, sceKernelGetThreadCurrentPriority);
	REG_FNID(SceThreadmgr, 0x751C9B7A, sceKernelChangeCurrentThreadAttr);
	REG_FNID(SceThreadmgr, 0xD9BD74EB, sceKernelCheckWaitableStatus);
	REG_FNID(SceThreadmgr, 0x9DCB4B7A, sceKernelGetProcessId);
	REG_FNID(SceThreadmgr, 0xB19CF7E9, sceKernelCreateCallback); // !!!
	REG_FNID(SceThreadmgr, 0xD469676B, sceKernelDeleteCallback); // !!!
	REG_FNID(SceThreadmgr, 0xE53E41F6, sceKernelCheckCallback);
	REG_FNID(SceThreadmgr, 0xF4EE4FA9, sceKernelGetSystemTimeWide);
	REG_FNID(SceThreadmgr, 0x47F6DE49, sceKernelGetSystemTimeLow);
	//REG_FNID(SceThreadmgr, 0xC0FAF6A3, sceKernelCreateThreadForUser);
	REG_FNID(SceThreadmgr, 0xF1AE5654, sceKernelGetThreadCpuAffinityMask); // !!!
});

DECLARE(arm_module_manager::SceDebugLed)("SceDebugLed", []()
{
	//REG_FNID(SceDebugLed, 0x78E702D3, sceKernelSetGPO);
});
