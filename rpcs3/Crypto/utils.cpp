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

void xor_(unsigned char *dest, unsigned char *src1, unsigned char *src2, int size)
{
	int i;
	for(i = 0; i < size; i++)
	{
		dest[i] = src1[i] ^ src2[i];
	}
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

// Crypto functions (AES128-CBC, AES128-ECB, SHA1-HMAC and AES-CMAC).
void aescbc128_decrypt(unsigned char *key, unsigned char *iv, unsigned char *in, unsigned char *out, int len)
{
	aes_context ctx;
	aes_setkey_dec(&ctx, key, 128);
	aes_crypt_cbc(&ctx, AES_DECRYPT, len, iv, in, out);

	// Reset the IV.
	memset(iv, 0, 0x10);
}

void aesecb128_encrypt(unsigned char *key, unsigned char *in, unsigned char *out)
{
	aes_context ctx;
	aes_setkey_enc(&ctx, key, 128);
	aes_crypt_ecb(&ctx, AES_ENCRYPT, in, out);
}

bool hmac_hash_compare(unsigned char *key, int key_len, unsigned char *in, int in_len, unsigned char *hash)
{
	unsigned char *out = new unsigned char[key_len];

	sha1_hmac(key, key_len, in, in_len, out);

	for (int i = 0; i < 0x10; i++)
	{
		if (out[i] != hash[i])
		{
			delete[] out;
			return false;
		}
	}

	delete[] out;

	return true;
}

bool cmac_hash_compare(unsigned char *key, int key_len, unsigned char *in, int in_len, unsigned char *hash)
{
	unsigned char *out = new unsigned char[key_len];

	aes_context ctx;
	aes_setkey_enc(&ctx, key, 128);
	aes_cmac(&ctx, in_len, in, out);

	for (int i = 0; i < key_len; i++)
	{
		if (out[i] != hash[i])
		{
			delete[] out;
			return false;
		}
	}

	delete[] out;

	return true;
}

// Reverse-engineered custom Lempel–Ziv–Markov based compression (unknown variant of LZRC).
int lz_decompress(unsigned char *out, unsigned char *in, unsigned int size)
{
	char *tmp = new char[3272];
	char *p;
	char *p2;
	char *sub;
	char *sub2;
	char *sub3;
	int offset;
	int index;
	int index2;
	int unk;

	int flag;
	int flag2;
	unsigned int c;
	int cc;
	int sp;
	unsigned int sc;
	int scc;
	char st;
	char t;
	unsigned int n_size;
	unsigned int r_size;
	signed int f_size;
	signed int b_size;
	signed int diff;
	signed int diff_pad;

	bool adjust;
	int pos;
	int end;
	int n_end;
	signed int end_size;
	int chunk_size;
	char pad;
	unsigned int remainder;
	int result;

	adjust = true;
	offset = 0;
	index = 0;
	remainder = -1;
	end = (int)((char *)out + size);
	pos = (int)in;
	pad = *in;
	chunk_size = (*(in + 1) << 24) | (*(in + 2) << 16) | (*(in + 3) << 8) | *(in + 4);

	if (*in >= 0) // Check if we have a valid starting byte.
	{
		memset(tmp, 128, 0xCA8u);
		end_size = 0;
		while (1)
		{
			while (1)
			{
				p = &tmp[offset];
				c = (unsigned char)tmp[offset + 2920];

				if (!(remainder >> 24))
				{
					int add = *(unsigned char *)(pos + 5);
					remainder <<= 8;
					++pos;
					chunk_size = (chunk_size << 8) + add;
				}

				cc = c - (c >> 3);
				r_size = c * (remainder >> 8);
				f_size = (unsigned int)chunk_size < r_size;

				if ((unsigned int)chunk_size < r_size)
					break;

				remainder -= r_size;
				chunk_size -= r_size;
				p[2920] = cc;
				offset = (offset - 1) & ((u64)~(offset - 1) >> 32);

				if (out == (void *)end)
					return -1;

				sub = &tmp[255 * ((((((unsigned char)out & 7) << 8) | index & 0xFFFFF8FFu) >> pad) & 7)];
				index = 1;

				do
				{
					sp = (int)&sub[index];
					sc = (unsigned char)sub[index - 1];

					if (!(remainder >> 24))
					{
						int add = *(unsigned char *)(pos++ + 5);
						remainder <<= 8;
						chunk_size = (chunk_size << 8) + add;
					}

					index *= 2;
					n_size = sc * (remainder >> 8);
					scc = sc - (sc >> 3);
					st = scc;

					if ((unsigned int)chunk_size < n_size)
					{
						remainder = n_size;
						++index;
						st = scc + 31;
					}
					else
					{
						remainder -= n_size;
						chunk_size -= n_size;
					}
					*(unsigned char *)(sp - 1) = st;
				}
				while (index <= 255);

				out += 1;
				++end_size;
				*(out - 1) = index;
			}

			remainder = c * (remainder >> 8);
			p[2920] = cc + 31;
			index = -1;

			while (1)
			{
				c = (unsigned char)p[2928];

				if (!(r_size >> 24))
				{
					int add = *(unsigned char *)(pos++ + 5);
					remainder = r_size << 8;
					chunk_size = (chunk_size << 8) + add;
				}

				p += 8;
				r_size = c * (remainder >> 8);
				cc = c - (c >> 3);

				if ((unsigned int)chunk_size >= r_size)
					break;

				remainder = r_size;
				p[2920] = cc + 31;
				++index;

				if (index == 6)
				{
					adjust = false;
					break;
				}
					
			}

			if (adjust)
			{
				remainder -= r_size;
				chunk_size -= r_size;
				p[2920] = cc;
			}
			adjust = true;

			p2 = &tmp[index];
			if (index >= 0)
			{
				sub3 = &tmp[offset & 7 | 8 * (((unsigned int)out << index) & 3) | 32 * index];
				flag = index - 3;
				c = (unsigned char)sub3[2984];

				if (!(remainder >> 24))
				{
					int add = *(unsigned char *)(pos++ + 5);
					remainder <<= 8;
					chunk_size = (chunk_size << 8) + add;
				}

				n_size = c * (remainder >> 8);
				cc = c - (c >> 3);
				t = cc;
				index2 = 2;

				if ((unsigned int)chunk_size >= n_size)
				{
					remainder -= n_size;
					chunk_size -= n_size;
				}
				else
				{
					remainder = n_size;
					index2 = 3;
					t = cc + 31;
				}

				if (flag < 0)
				{
					sub3[2984] = t;
				}
				else
				{
					if (flag <= 0)
					{
						sub3[2984] = t;
					}
					else
					{
						c = (unsigned char)t;

						if (!(remainder >> 24))
						{
							int add = *(unsigned char *)(pos++ + 5);
							remainder <<= 8;
							chunk_size = (chunk_size << 8) + add;
						}
						index2 *= 2;
						n_size = c * (remainder >> 8);
						cc = c - (c >> 3);
						t = cc;

						if ((unsigned int)chunk_size >= n_size)
						{
							remainder -= n_size;
							chunk_size -= n_size;
						}
						else
						{
							remainder = n_size;
							++index2;
							t = cc + 31;
						}
						sub3[2984] = t;

						if (flag != 1)
						{
							if (!(remainder >> 24))
							{
								int add = *(unsigned char *)(pos + 5);
								remainder <<= 8;
								++pos;
								chunk_size = (chunk_size << 8) + add;
							}
							do
							{
								remainder >>= 1;
								index2 = ((unsigned int)chunk_size < remainder) + 2 * index2;

								if ((unsigned int)chunk_size >= remainder)
									chunk_size -= remainder;

								--flag;
							}
							while (flag != 1);
						}
					}
					c = (unsigned char)sub3[3008];

					if (!(remainder >> 24))
					{
						int add = *(unsigned char *)(pos + 5);
						remainder <<= 8;
						++pos;
						chunk_size = (chunk_size << 8) + add;
					}
					index2 *= 2;
					n_size = c * (remainder >> 8);
					cc = c - (c >> 3);
					t = cc;

					if ((unsigned int)chunk_size >= n_size)
					{
						remainder -= n_size;
						chunk_size -= n_size;
					}
					else
					{
						remainder = n_size;
						++index2;
						t = cc + 31;
					}
					sub3[3008] = t;
				}
				if (index > 0)
				{
					c = (unsigned char)sub3[2992];

					if (!(remainder >> 24))
					{
						int add = *(unsigned char *)(pos++ + 5);
						remainder <<= 8;
						chunk_size = (chunk_size << 8) + add;
					}

					index2 *= 2;
					n_size = c * (remainder >> 8);
					cc = c - (c >> 3);
					t = cc;

					if ((unsigned int)chunk_size >= n_size)
					{
						remainder -= n_size;
						chunk_size -= n_size;
					}
					else
					{
						remainder = n_size;
						++index2;
						t = cc + 31;
					}
					sub3[2992] = t;

					if (index != 1)
					{
						c = (unsigned char)sub3[3000];

						if (!(remainder >> 24))
						{
							int add = *(unsigned char *)(pos + 5);
							remainder <<= 8;
							++pos;
							chunk_size = (chunk_size << 8) + add;
						}

						index2 *= 2;
						n_size = c * (remainder >> 8);
						cc = c - (c >> 3);
						t = cc;

						if ((unsigned int)chunk_size >= n_size)
						{
							remainder -= n_size;
							chunk_size -= n_size;
						}
						else
						{
							remainder = n_size;
							++index2;
							t = cc + 31;
						}
						sub3[3000] = t;
					}
				}
				f_size = index2;

				if (index2 == 255)
					break;
			}
			index = 8;
			b_size = 352;

			if (f_size <= 2)
			{
				p2 += 248;
				b_size = 64;
			}
			do
			{
				unk = (int)&p2[index];

				if (!(remainder >> 24))
				{
					int add = *(unsigned char *)(pos++ + 5);
					remainder <<= 8;
					chunk_size = (chunk_size << 8) + add;
				}

				c = *(unsigned char *)(unk + 2033);
				index *= 2;
				n_size = c * (remainder >> 8);
				cc = c - (c >> 3);
				t = cc;

				if ((unsigned int)chunk_size < n_size)
				{
					remainder = n_size;
					t = cc + 31;
					index += 8;
				}
				else
				{
					remainder -= n_size;
					chunk_size -= n_size;
				}
				*(unsigned char *)(unk + 2033) = t;
				diff = index - b_size;
			}
			while ((index - b_size) < 0);

			if (index != b_size)
			{
				diff_pad = diff >> 3;
				flag = diff_pad - 1;
				flag2 = diff_pad - 4;
				sub2 = &tmp[32 * (diff_pad - 1)];
				c = (unsigned char)sub2[2344];

				if (!(remainder >> 24))
				{
					int add = *(unsigned char *)(pos + 5);
					remainder <<= 8;
					++pos;
					chunk_size = (chunk_size << 8) + add;
				}

				n_size = c * (remainder >> 8);
				cc = c - (c >> 3);
				t = cc;
				index2 = 2;

				if ((unsigned int)chunk_size >= n_size)
				{
					remainder -= n_size;
					chunk_size -= n_size;
				}
				else
				{
					remainder = n_size;
					index2 = 3;
					t = cc + 31;
				}

				if (flag2 < 0)
				{
					sub2[2344] = t;
				}
				else
				{
					if (flag2 <= 0)
					{
						sub2[2344] = t;
					}
					else
					{
						c = (unsigned char)t;

						if (!(remainder >> 24))
						{
							int add = *(unsigned char *)(pos++ + 5);
							remainder <<= 8;
							chunk_size = (chunk_size << 8) + add;
						}

						index2 *= 2;
						n_size = c * (remainder >> 8);
						cc = c - (c >> 3);
						t = cc;

						if ((unsigned int)chunk_size >= n_size)
						{
							remainder -= n_size;
							chunk_size -= n_size;
						}
						else
						{
							remainder = n_size;
							++index2;
							t = cc + 31;
						}
						sub2[2344] = t;

						if (flag2 != 1)
						{
							if (!(remainder >> 24))
							{
								int add = *(unsigned char *)(pos + 5);
								remainder <<= 8;
								++pos;
								chunk_size = (chunk_size << 8) + add;
							}
							do
							{
								remainder >>= 1;
								index2 = ((unsigned int)chunk_size < remainder) + 2 * index2;

								if ((unsigned int)chunk_size >= remainder)
									chunk_size -= remainder;

								--flag2;
							}
							while (flag2 != 1);
						}
					}
					c = (unsigned char)sub2[2368];

					if (!(remainder >> 24))
					{
						int add = *(unsigned char *)(pos + 5);
						remainder <<= 8;
						++pos;
						chunk_size = (chunk_size << 8) + add;
					}

					index2 *= 2;
					n_size = c * (remainder >> 8);
					cc = c - (c >> 3);
					t = cc;

					if ((unsigned int)chunk_size >= n_size)
					{
						remainder -= n_size;
						chunk_size -= n_size;
					}
					else
					{
						remainder = n_size;
						++index2;
						t = cc + 31;
					}
					sub2[2368] = t;
				}
				if (flag > 0)
				{
					c = (unsigned char)sub2[2352];
					if (!(remainder >> 24))
					{
						int add = *(unsigned char *)(pos++ + 5);
						remainder <<= 8;
						chunk_size = (chunk_size << 8) + add;
					}
					index2 *= 2;
					n_size = c * (remainder >> 8);
					cc = c - (c >> 3);
					t = cc;
					if ((unsigned int)chunk_size >= n_size)
					{
						remainder -= n_size;
						chunk_size -= n_size;
					}
					else
					{
						remainder = n_size;
						++index2;
						t = cc + 31;
					}
					sub2[2352] = t;
					if (flag != 1)
					{
						c = (unsigned char)sub2[2360];
						if (!(remainder >> 24))
						{
							int add = *(unsigned char *)(pos + 5);
							remainder <<= 8;
							++pos;
							chunk_size = (chunk_size << 8) + add;
						}
						index2 *= 2;
						n_size = c * (remainder >> 8);
						cc = c - (c >> 3);
						t = cc;

						if ((unsigned int)chunk_size >= n_size)
						{
							remainder -= n_size;
							chunk_size -= n_size;
						}
						else
						{
							remainder = n_size;
							++index2;
							t = cc + 31;
						}
						sub2[2360] = t;
					}
				}
				diff = index2 - 1;
			}

			if (end_size <= diff)
				return -1;

			index = *(out - diff - 1);
			n_end = (int)(out + f_size);
			offset = (((unsigned char)f_size + (unsigned char)out) & 1) + 6;

			if ((unsigned int)(out + f_size) >= (unsigned int)end)
				return -1;

			do
			{
				out += 1;
				++end_size;
				*(out - 1) = index;
				index = *(out - diff - 1);
			}
			while (out != (void *)n_end);

			out += 1;
			++end_size;
			*((unsigned char *)out - 1) = index;
		}
		result = end_size;
	}
	else // Starting byte is invalid.
	{
		result = -1;
		if (chunk_size <= (int)size)
		{
			memcpy(out, (const void *)(in + 5), chunk_size);
			result = chunk_size;
		}
	}
	delete[] tmp;

	return result;
}
