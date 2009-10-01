#pragma once

template<int from, int to>
int Convert(char *dest, const char* psrc, int lsn);

template<>
int Convert<2048,2048>(char *dest, const char* psrc, int lsn)
{
	memcpy(dest,psrc,2048);
	return 0;
}

template<>
int Convert<2352,2352>(char *dest, const char* psrc, int lsn)
{
	memcpy(dest,psrc,2352);
	return 0;
}

template<>
int Convert<2352,2048>(char *dest, const char* psrc, int lsn)
{
	memcpy(dest,psrc+16,2048);
	return 0;
}

template<>
int Convert<2048,2352>(char *dest, const char* psrc, int lsn)
{
	int m = lsn / (75*60);
	int s = (lsn/75) % 60;
	int f = lsn%75;
	unsigned char header[16] = {
		0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,   m,   s,   f,0x02
	};

	memcpy(dest,header,16);
	memcpy(dest+16,psrc,2048);
	return 0;
}
