// Copyright (C) 2014       Hykem <hykem@hotmail.com>
// Licensed under the terms of the GNU GPL, version 2.0 or later versions.
// http://www.gnu.org/licenses/gpl-2.0.txt

#include <string.h>
#include <vector>
#include "lz.h"

void decode_range(unsigned int *range, unsigned int *code, unsigned char **src)
{
	if (!((*range) >> 24))
	{
		(*range) <<= 8;
		*code = ((*code) << 8) + (*src)++[5];
	}
}

int decode_bit(unsigned int *range, unsigned int *code, int *index, unsigned char **src, unsigned char *c)
{
	decode_range(range, code, src);

	unsigned int val = ((*range) >> 8) * (*c);

	*c -= ((*c) >> 3);
	if (index) (*index) <<= 1;

	if (*code < val)
	{
		*range = val;
		*c += 31;
		if (index) (*index)++;
		return 1;
	}
	else
	{
		*code -= val;
		*range -= val;
		return 0;
	}
}

int decode_number(unsigned char *ptr, int index, int *bit_flag, unsigned int *range, unsigned int *code, unsigned char **src)
{
	int i = 1;

	if (index >= 3)
	{
		decode_bit(range, code, &i, src, ptr + 0x18);
		if (index >= 4)
		{
			decode_bit(range, code, &i, src, ptr + 0x18);
			if (index >= 5)
			{
				decode_range(range, code, src);
				for (; index >= 5; index--)
				{
					i <<= 1;
					(*range) >>= 1;
					if (*code < *range)
						i++;
					else
						(*code) -= *range;
				}
			}
		}
	}

	*bit_flag = decode_bit(range, code, &i, src, ptr);

	if (index >= 1)
	{
		decode_bit(range, code, &i, src, ptr + 0x8);
		if (index >= 2)
		{
			decode_bit(range, code, &i, src, ptr + 0x10);
		}
	}

	return i;
}

int decode_word(unsigned char *ptr, int index, int *bit_flag, unsigned int *range, unsigned int *code, unsigned char **src)
{
	int i = 1;
	index /= 8;

	if (index >= 3)
	{
		decode_bit(range, code, &i, src, ptr + 4);
		if (index >= 4)
		{
			decode_bit(range, code, &i, src, ptr + 4);
			if (index >= 5)
			{
				decode_range(range, code, src);
				for (; index >= 5; index--)
				{
					i <<= 1;
					(*range) >>= 1;
					if (*code < *range)
						i++;
					else
						(*code) -= *range;
				}
			}
		}
	}

	*bit_flag = decode_bit(range, code, &i, src, ptr);

	if (index >= 1)
	{
		decode_bit(range, code, &i, src, ptr + 1);
		if (index >= 2)
		{
			decode_bit(range, code, &i, src, ptr + 2);
		}
	}

	return i;
}

int decompress(unsigned char *out, unsigned char *in, unsigned int size)
{
	int result;

	int offset = 0;
	int bit_flag = 0;
	int data_length = 0;
	int data_offset = 0;

	unsigned char *tmp_sect1, *tmp_sect2, *tmp_sect3;
	const unsigned char *buf_start, *buf_end;
	unsigned char prev = 0;

	unsigned char *start = out;
	const unsigned char *end = (out + size);
	unsigned char head = in[0];

	unsigned int range = 0xFFFFFFFF;
	unsigned int code = (in[1] << 24) | (in[2] << 16) | (in[3] << 8) | in[4];

	if (head > 0x80) // Check if we have a valid starting byte.
	{
		// The dictionary header is invalid, the data is not compressed.
		result = -1;
		if (code <= size)
		{
			memcpy(out, in + 5, code);
			result = static_cast<int>(start - out);
		}
	}
	else
	{
		// Set up a temporary buffer (sliding window).
		std::vector<unsigned char> tmp_vec(0xCC8);
		unsigned char* tmp = tmp_vec.data();
		memset(tmp, 0x80, 0xCA8);
		while (true)
		{
			// Start reading at 0xB68.
			tmp_sect1 = tmp + offset + 0xB68;
			if (!decode_bit(&range, &code, 0, &in, tmp_sect1))  // Raw char.
			{
				// Adjust offset and check for stream end.
				if (offset > 0) offset--;
				if (start == end) return static_cast<int>(start - out);

				// Locate first section.
				int sect = ((((((static_cast<int>(start - out)) & 7) << 8) + prev) >> head) & 7) * 0xFF - 1;
				tmp_sect1 = tmp + sect;
				int index = 1;

				// Read, decode and write back.
				do
				{
					decode_bit(&range, &code, &index, &in, tmp_sect1 + index);
				} while ((index >> 8) == 0);

				// Save index.
				*start++ = index;
			}
			else  // Compressed char stream.
			{
				int index = -1;

				// Identify the data length bit field.
				do
				{
					tmp_sect1 += 8;
					bit_flag = decode_bit(&range, &code, 0, &in, tmp_sect1);
					index += bit_flag;
				} while ((bit_flag != 0) && (index < 6));

				// Default block size is 0x160.
				int b_size = 0x160;
				tmp_sect2 = tmp + index + 0x7F1;

				// If the data length was found, parse it as a number.
				if ((index >= 0) || (bit_flag != 0))
				{
					// Locate next section.
					int sect = (index << 5) | ((((static_cast<int>(start - out)) << index) & 3) << 3) | (offset & 7);
					tmp_sect1 = tmp + 0xBA8 + sect;

					// Decode the data length (8 bit fields).
					data_length = decode_number(tmp_sect1, index, &bit_flag, &range, &code, &in);
					if (data_length == 0xFF) return static_cast<int>(start - out);  // End of stream.
				}
				else
				{
					// Assume one byte of advance.
					data_length = 1;
				}

				// If we got valid parameters, seek to find data offset.
				if ((data_length <= 2))
				{
					tmp_sect2 += 0xF8;
					b_size = 0x40;  // Block size is now 0x40.
				}

				int diff = 0;
				int shift = 1;

				// Identify the data offset bit field.
				do
				{
					diff = (shift << 4) - b_size;
					bit_flag = decode_bit(&range, &code, &shift, &in, tmp_sect2 + (shift << 3));
				} while (diff < 0);

				// If the data offset was found, parse it as a number.
				if ((diff > 0) || (bit_flag != 0))
				{
					// Adjust diff if needed.
					if (bit_flag == 0) diff -= 8;

					// Locate section.
					tmp_sect3 = tmp + 0x928 + diff;

					// Decode the data offset (1 bit fields).
					data_offset = decode_word(tmp_sect3, diff, &bit_flag, &range, &code, &in);
				}
				else
				{
					// Assume one byte of advance.
					data_offset = 1;
				}

				// Set buffer start/end.
				buf_start = start - data_offset;
				buf_end = start + data_length + 1;

				// Underflow.
				if (buf_start < out)
				{
					return -1;
				}

				// Overflow.
				if (buf_end > end)
				{
					return -1;
				}

				// Update offset.
				offset = (((static_cast<int>(buf_end - out)) + 1) & 1) + 6;

				// Copy data.
				do
				{
					*start++ = *buf_start++;
				} while (start < buf_end);

			}
			prev = *(start - 1);
		}
		result = static_cast<int>(start - out);
	}
	return result;
}
