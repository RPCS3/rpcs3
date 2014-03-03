#include "stdafx.h"
#include "utils.h"

// Endian swap auxiliary functions.
u16 swap16(u16 i)
{
	return ((i & 0xFF00) >>  8) | ((i & 0xFF) << 8);
}

u32 swap32(u32 i)
{
	return ((i & 0xFF000000) >> 24) | ((i & 0xFF0000) >>  8) | ((i & 0xFF00) <<  8) | ((i & 0xFF) << 24);
}

u64 swap64(u64 i)
{
	return ((i & 0x00000000000000ff) << 56) | ((i & 0x000000000000ff00) << 40) |
		((i & 0x0000000000ff0000) << 24) | ((i & 0x00000000ff000000) <<  8) |
		((i & 0x000000ff00000000) >>  8) | ((i & 0x0000ff0000000000) >> 24) |
		((i & 0x00ff000000000000) >> 40) | ((i & 0xff00000000000000) >> 56);
}

// Hex string conversion auxiliary functions.
u64 hex_to_u64(const char* hex_str)
{
	u32 length = strlen(hex_str);
	u64 tmp = 0;
	u64 result = 0;
	char c;

	while (length--)
	{
		c = *hex_str++;
		if((c >= '0') && (c <= '9'))
			tmp = c - '0';
		else if((c >= 'a') && (c <= 'f'))
			tmp = c - 'a' + 10;
		else if((c >= 'A') && (c <= 'F'))
			tmp = c - 'A' + 10;
		else
			tmp = 0;
		result |= (tmp << (length * 4));
	}

	return result;
}

void hex_to_bytes(unsigned char *data, const char *hex_str)
{
	u32 str_length = strlen(hex_str);
	u32 data_length = str_length / 2;
	char tmp_buf[3] = {0, 0, 0};

	// Don't convert if the string length is odd.
	if (!(str_length % 2))
	{
		u8 *out = (u8 *) malloc (str_length * sizeof(u8));
		u8 *pos = out;

		while (str_length--)
		{
			tmp_buf[0] = *hex_str++;
			tmp_buf[1] = *hex_str++;

			*pos++ = (u8)(hex_to_u64(tmp_buf) & 0xFF);
		}

		// Copy back to our array.
		memcpy(data, out, data_length);
	}
}