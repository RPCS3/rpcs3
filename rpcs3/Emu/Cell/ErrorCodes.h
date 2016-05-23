#pragma once

#define ERROR_CODE(code) static_cast<s32>(code)

enum CellOk : s32
{
	CELL_OK = 0,
};

enum CellError : s32
{
	CELL_EAGAIN        = ERROR_CODE(0x80010001), // The resource is temporarily unavailable
	CELL_EINVAL        = ERROR_CODE(0x80010002), // An invalid argument value is specified
	CELL_ENOSYS        = ERROR_CODE(0x80010003), // The feature is not yet implemented
	CELL_ENOMEM        = ERROR_CODE(0x80010004), // Memory allocation failure
	CELL_ESRCH         = ERROR_CODE(0x80010005), // The resource with the specified identifier does not exist
	CELL_ENOENT        = ERROR_CODE(0x80010006), // The file does not exist
	CELL_ENOEXEC       = ERROR_CODE(0x80010007), // The file is in unrecognized format
	CELL_EDEADLK       = ERROR_CODE(0x80010008), // Resource deadlock is avoided
	CELL_EPERM         = ERROR_CODE(0x80010009), // The operation is not permitted
	CELL_EBUSY         = ERROR_CODE(0x8001000A), // The device or resource is busy
	CELL_ETIMEDOUT     = ERROR_CODE(0x8001000B), // The operation is timed out
	CELL_EABORT        = ERROR_CODE(0x8001000C), // The operation is aborted
	CELL_EFAULT        = ERROR_CODE(0x8001000D), // Invalid memory access
	CELL_ESTAT         = ERROR_CODE(0x8001000F), // State of the target thread is invalid
	CELL_EALIGN        = ERROR_CODE(0x80010010), // Alignment is invalid.
	CELL_EKRESOURCE    = ERROR_CODE(0x80010011), // Shortage of the kernel resources
	CELL_EISDIR        = ERROR_CODE(0x80010012), // The file is a directory
	CELL_ECANCELED     = ERROR_CODE(0x80010013), // Operation canceled
	CELL_EEXIST        = ERROR_CODE(0x80010014), // Entry already exists
	CELL_EISCONN       = ERROR_CODE(0x80010015), // Port is already connected
	CELL_ENOTCONN      = ERROR_CODE(0x80010016), // Port is not connected
	CELL_EAUTHFAIL     = ERROR_CODE(0x80010017), // Program authentication fail
	CELL_ENOTMSELF     = ERROR_CODE(0x80010018), // The file is not a MSELF
	CELL_ESYSVER       = ERROR_CODE(0x80010019), // System version error
	CELL_EAUTHFATAL    = ERROR_CODE(0x8001001A), // Fatal system error
	CELL_EDOM          = ERROR_CODE(0x8001001B),
	CELL_ERANGE        = ERROR_CODE(0x8001001C),
	CELL_EILSEQ        = ERROR_CODE(0x8001001D),
	CELL_EFPOS         = ERROR_CODE(0x8001001E),
	CELL_EINTR         = ERROR_CODE(0x8001001F),
	CELL_EFBIG         = ERROR_CODE(0x80010020),
	CELL_EMLINK        = ERROR_CODE(0x80010021),
	CELL_ENFILE        = ERROR_CODE(0x80010022),
	CELL_ENOSPC        = ERROR_CODE(0x80010023),
	CELL_ENOTTY        = ERROR_CODE(0x80010024),
	CELL_EPIPE         = ERROR_CODE(0x80010025),
	CELL_EROFS         = ERROR_CODE(0x80010026),
	CELL_ESPIPE        = ERROR_CODE(0x80010027),
	CELL_E2BIG         = ERROR_CODE(0x80010028),
	CELL_EACCES        = ERROR_CODE(0x80010029),
	CELL_EBADF         = ERROR_CODE(0x8001002A),
	CELL_EIO           = ERROR_CODE(0x8001002B),
	CELL_EMFILE        = ERROR_CODE(0x8001002C),
	CELL_ENODEV        = ERROR_CODE(0x8001002D),
	CELL_ENOTDIR       = ERROR_CODE(0x8001002E),
	CELL_ENXIO         = ERROR_CODE(0x8001002F),
	CELL_EXDEV         = ERROR_CODE(0x80010030),
	CELL_EBADMSG       = ERROR_CODE(0x80010031),
	CELL_EINPROGRESS   = ERROR_CODE(0x80010032),
	CELL_EMSGSIZE      = ERROR_CODE(0x80010033),
	CELL_ENAMETOOLONG  = ERROR_CODE(0x80010034),
	CELL_ENOLCK        = ERROR_CODE(0x80010035),
	CELL_ENOTEMPTY     = ERROR_CODE(0x80010036),
	CELL_ENOTSUP       = ERROR_CODE(0x80010037),
	CELL_EFSSPECIFIC   = ERROR_CODE(0x80010038),
	CELL_EOVERFLOW     = ERROR_CODE(0x80010039),
	CELL_ENOTMOUNTED   = ERROR_CODE(0x8001003A),
	CELL_ENOTSDATA     = ERROR_CODE(0x8001003B),
};

// Special return type signaling on errors
struct ppu_error_code
{
	s32 value;

	// Print error message, error code is returned
	static s32 report(s32 error, const char* text);

	// Must be specialized for specific tag type T
	template<typename T>
	static const char* print(T code)
	{
		return nullptr;
	}

	template<typename T>
	s32 error_check(T code)
	{
		if (const auto text = print(code))
		{
			return report(code, text);
		}

		return code;
	}

	ppu_error_code() = default;

	// General error check
	template<typename T, typename = std::enable_if_t<std::is_enum<T>::value>>
	ppu_error_code(T value)
		: value(error_check(value))
	{
	}

	// Force error reporting with a message specified
	ppu_error_code(s32 value, const char* text)
		: value(report(value, text))
	{
	}

	// Helper
	enum class not_an_error : s32 {};

	// Silence any error
	constexpr ppu_error_code(not_an_error value)
		: value(static_cast<s32>(value))
	{
	}

	// Conversion
	constexpr operator s32() const
	{
		return value;
	}
};

// Helper macro for silencing possible error checks on returning ppu_error_code values
#define NOT_AN_ERROR(...) static_cast<ppu_error_code::not_an_error>(__VA_ARGS__)

template<typename T, typename>
struct ppu_gpr_cast_impl;

template<>
struct ppu_gpr_cast_impl<ppu_error_code, void>
{
	static inline u64 to(const ppu_error_code& code)
	{
		return code;
	}

	static inline ppu_error_code from(const u64 reg)
	{
		return NOT_AN_ERROR(reg);
	}
};

template<>
inline const char* ppu_error_code::print(CellError error)
{
	switch (error)
	{
		STR_CASE(CELL_EAGAIN);
		STR_CASE(CELL_EINVAL);
		STR_CASE(CELL_ENOSYS);
		STR_CASE(CELL_ENOMEM);
		STR_CASE(CELL_ESRCH);
		STR_CASE(CELL_ENOENT);
		STR_CASE(CELL_ENOEXEC);
		STR_CASE(CELL_EDEADLK);
		STR_CASE(CELL_EPERM);
		STR_CASE(CELL_EBUSY);
		STR_CASE(CELL_ETIMEDOUT);
		STR_CASE(CELL_EABORT);
		STR_CASE(CELL_EFAULT);
		STR_CASE(CELL_ESTAT);
		STR_CASE(CELL_EALIGN);
		STR_CASE(CELL_EKRESOURCE);
		STR_CASE(CELL_EISDIR);
		STR_CASE(CELL_ECANCELED);
		STR_CASE(CELL_EEXIST);
		STR_CASE(CELL_EISCONN);
		STR_CASE(CELL_ENOTCONN);
		STR_CASE(CELL_EAUTHFAIL);
		STR_CASE(CELL_ENOTMSELF);
		STR_CASE(CELL_ESYSVER);
		STR_CASE(CELL_EAUTHFATAL);
		STR_CASE(CELL_EDOM);
		STR_CASE(CELL_ERANGE);
		STR_CASE(CELL_EILSEQ);
		STR_CASE(CELL_EFPOS);
		STR_CASE(CELL_EINTR);
		STR_CASE(CELL_EFBIG);
		STR_CASE(CELL_EMLINK);
		STR_CASE(CELL_ENFILE);
		STR_CASE(CELL_ENOSPC);
		STR_CASE(CELL_ENOTTY);
		STR_CASE(CELL_EPIPE);
		STR_CASE(CELL_EROFS);
		STR_CASE(CELL_ESPIPE);
		STR_CASE(CELL_E2BIG);
		STR_CASE(CELL_EACCES);
		STR_CASE(CELL_EBADF);
		STR_CASE(CELL_EIO);
		STR_CASE(CELL_EMFILE);
		STR_CASE(CELL_ENODEV);
		STR_CASE(CELL_ENOTDIR);
		STR_CASE(CELL_ENXIO);
		STR_CASE(CELL_EXDEV);
		STR_CASE(CELL_EBADMSG);
		STR_CASE(CELL_EINPROGRESS);
		STR_CASE(CELL_EMSGSIZE);
		STR_CASE(CELL_ENAMETOOLONG);
		STR_CASE(CELL_ENOLCK);
		STR_CASE(CELL_ENOTEMPTY);
		STR_CASE(CELL_ENOTSUP);
		STR_CASE(CELL_EFSSPECIFIC);
		STR_CASE(CELL_EOVERFLOW);
		STR_CASE(CELL_ENOTMOUNTED);
		STR_CASE(CELL_ENOTSDATA);
	}

	return nullptr;
}
