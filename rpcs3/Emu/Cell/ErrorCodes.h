#pragma once

#include "util/types.hpp"
#include "Utilities/StrFmt.h"

namespace logs
{
	struct message;
}

// Error code type (return type), implements error reporting.
class error_code
{
	s32 value;

	// Common constructor
	template <typename ET> requires requires (ET v) { static_cast<s32>(v); }
	error_code(const logs::message* ch, const ET& value) noexcept
		: value(error_report(static_cast<s32>(value), ch, " : %s", fmt::type_info_v<ET>, fmt_args_t<ET>{fmt_unveil<ET>::get(value)}))
	{
	}

	// Error constructor (2 args)
	template <typename ET, typename T> requires requires (ET v) { static_cast<s32>(v); }
	error_code(const logs::message* ch, const ET& value, const T& arg) noexcept
		: value(error_report(static_cast<s32>(value), ch, " : %s, %s", fmt::type_info_v<ET, T>, fmt_args_t<ET, T>{fmt_unveil<ET>::get(value), fmt_unveil<T>::get(arg)}))
	{
	}

	// Formatting constructor (error, format string, variadic list)
	template <typename ET, typename... Args> requires requires (ET v) { static_cast<s32>(v); }
	error_code(const logs::message* ch, const ET& value, const const_str& fmt, const Args&... args) noexcept
		: value(error_report(static_cast<s32>(value), ch, fmt, fmt::type_info_v<Args...>, fmt_args_t<Args...>{fmt_unveil<Args>::get(args)...}))
	{
	}

public:
	// Implementation must be provided independently
	static s32 error_report(s32 result, const logs::message* channel, const char* fmt, const fmt_type_info* sup, const u64* args);

	error_code() = default;

	// Constructor without channel
	template <typename... Args> requires (sizeof...(Args) > 0 && !(std::is_same_v<std::decay_t<std::remove_pointer_t<Args>>, logs::message> || ...))
	error_code(const Args&... args) noexcept
		: error_code(std::add_pointer_t<const logs::message>{}, args...)
	{
	}

	// Constructor with channel
	template <typename... Args>
	error_code(const logs::message& ch, const Args&... args) noexcept
		: error_code(std::addressof(ch), args...)
	{
	}

	operator s32() const
	{
		return value;
	}

	ENABLE_BITWISE_SERIALIZATION;
};

enum CellNotAnError : s32
{
	CELL_OK     = 0,
	CELL_CANCEL = 1,
};

// Constructor specialization that doesn't trigger reporting
template <>
constexpr FORCE_INLINE error_code::error_code(const CellNotAnError& value) noexcept
	: value(value)
{
}

// Helper function for error_code
template <typename T>
constexpr FORCE_INLINE CellNotAnError not_an_error(const T& value)
{
	return static_cast<CellNotAnError>(static_cast<s32>(value));
}

template <typename T>
struct ppu_gpr_cast_impl;

template <>
struct ppu_gpr_cast_impl<error_code>
{
	static inline u64 to(const error_code& code)
	{
		return code;
	}

	static inline error_code from(const u64 reg)
	{
		return not_an_error(reg);
	}
};

enum CellError : u32
{
	CELL_EAGAIN        = 0x80010001, // The resource is temporarily unavailable
	CELL_EINVAL        = 0x80010002, // An invalid argument value is specified
	CELL_ENOSYS        = 0x80010003, // The feature is not yet implemented
	CELL_ENOMEM        = 0x80010004, // Memory allocation failure
	CELL_ESRCH         = 0x80010005, // The resource with the specified identifier does not exist
	CELL_ENOENT        = 0x80010006, // The file does not exist
	CELL_ENOEXEC       = 0x80010007, // The file is in unrecognized format
	CELL_EDEADLK       = 0x80010008, // Resource deadlock is avoided
	CELL_EPERM         = 0x80010009, // The operation is not permitted
	CELL_EBUSY         = 0x8001000A, // The device or resource is busy
	CELL_ETIMEDOUT     = 0x8001000B, // The operation is timed out
	CELL_EABORT        = 0x8001000C, // The operation is aborted
	CELL_EFAULT        = 0x8001000D, // Invalid memory access
	CELL_ENOCHILD      = 0x8001000E, // Process has no child(s)
	CELL_ESTAT         = 0x8001000F, // State of the target thread is invalid
	CELL_EALIGN        = 0x80010010, // Alignment is invalid.
	CELL_EKRESOURCE    = 0x80010011, // Shortage of the kernel resources
	CELL_EISDIR        = 0x80010012, // The file is a directory
	CELL_ECANCELED     = 0x80010013, // Operation canceled
	CELL_EEXIST        = 0x80010014, // Entry already exists
	CELL_EISCONN       = 0x80010015, // Port is already connected
	CELL_ENOTCONN      = 0x80010016, // Port is not connected
	CELL_EAUTHFAIL     = 0x80010017, // Program authentication fail
	CELL_ENOTMSELF     = 0x80010018, // The file is not a MSELF
	CELL_ESYSVER       = 0x80010019, // System version error
	CELL_EAUTHFATAL    = 0x8001001A, // Fatal system error
	CELL_EDOM          = 0x8001001B, // Math domain violation
	CELL_ERANGE        = 0x8001001C, // Math range violation
	CELL_EILSEQ        = 0x8001001D, // Illegal multi-byte sequence in input
	CELL_EFPOS         = 0x8001001E, // File position error
	CELL_EINTR         = 0x8001001F, // Syscall was interrupted
	CELL_EFBIG         = 0x80010020, // File too large
	CELL_EMLINK        = 0x80010021, // Too many links
	CELL_ENFILE        = 0x80010022, // File table overflow
	CELL_ENOSPC        = 0x80010023, // No space left on device
	CELL_ENOTTY        = 0x80010024, // Not a TTY
	CELL_EPIPE         = 0x80010025, // Broken pipe
	CELL_EROFS         = 0x80010026, // Read-only filesystem (write fail)
	CELL_ESPIPE        = 0x80010027, // Illegal seek (e.g. seek on pipe)
	CELL_E2BIG         = 0x80010028, // Arg list too long
	CELL_EACCES        = 0x80010029, // Access violation
	CELL_EBADF         = 0x8001002A, // Invalid file descriptor
	CELL_EIO           = 0x8001002B, // Filesystem mounting failed (actually IO error...EIO)
	CELL_EMFILE        = 0x8001002C, // Too many files open
	CELL_ENODEV        = 0x8001002D, // No device
	CELL_ENOTDIR       = 0x8001002E, // Not a directory
	CELL_ENXIO         = 0x8001002F, // No such device or IO
	CELL_EXDEV         = 0x80010030, // Cross-device link error
	CELL_EBADMSG       = 0x80010031, // Bad Message
	CELL_EINPROGRESS   = 0x80010032, // In progress
	CELL_EMSGSIZE      = 0x80010033, // Message size error
	CELL_ENAMETOOLONG  = 0x80010034, // Name too long
	CELL_ENOLCK        = 0x80010035, // No lock
	CELL_ENOTEMPTY     = 0x80010036, // Not empty
	CELL_ENOTSUP       = 0x80010037, // Not supported
	CELL_EFSSPECIFIC   = 0x80010038, // File-system specific error
	CELL_EOVERFLOW     = 0x80010039, // Overflow occured
	CELL_ENOTMOUNTED   = 0x8001003A, // Filesystem not mounted
	CELL_ENOTSDATA     = 0x8001003B, // Not SData
	CELL_ESDKVER       = 0x8001003C, // Incorrect version in sys_load_param
	CELL_ENOLICDISC    = 0x8001003D, // Pointer is null. Similar than 0x8001003E but with some PARAM.SFO parameter (TITLE_ID?) embedded.
	CELL_ENOLICENT     = 0x8001003E, // Pointer is null
};
