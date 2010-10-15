/* The file is based of zpipe.c + minor rename and minor adaptation
 *
 * zpipe.c: example of proper use of zlib's inflate() and deflate()
 * Not copyrighted -- provided to the public domain
 * Version 1.4  11 December 2005  Mark Adler 
 */

// zpipe.cpp : Defines the entry point for the console application.
//

#include <iostream>

#include <stdio.h>
#include <string.h>
#include <assert.h>

//#define ZLIB_WINAPI
#include <zlib.h>

int def(char *src, char *dst, int bytes_to_compress, int *bytes_after_compressed) ;
int inf(char *src, char *dst, int bytes_to_decompress, int maximum_after_decompress) ;

int def(char *src, char *dst, int bytes_to_compress, int *bytes_after_compressed)
{
	z_stream strm;

	int ret;//, flush;
	unsigned have;

	/* allocate deflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	ret = deflateInit(&strm, Z_DEFAULT_COMPRESSION) ;
	if (ret != Z_OK)
		return ret;

	/* compress */
	strm.avail_in = bytes_to_compress ;
	strm.avail_out = bytes_to_compress ;
	strm.next_in = (Bytef *)src ;
	strm.next_out = (Bytef *)dst ;

	ret = deflate(&strm, Z_FINISH) ;
	have = bytes_to_compress - strm.avail_out ;
	*bytes_after_compressed = have ;

	assert(ret == Z_STREAM_END);		/* stream will be complete */

	/* clean up and return */
	(void)deflateEnd(&strm);
	return Z_OK;
}

int inf(char *src, char *dst, int bytes_to_decompress, int maximum_after_decompress, int* outbytes)
{
	z_stream strm;

	int ret;
	//unsigned have;

	/* allocate inflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	ret = inflateInit(&strm);
	if (ret != Z_OK)
		return ret;

	/* decompress */
	strm.avail_in = bytes_to_decompress ;
	strm.next_in = (Bytef *)src ;
	strm.next_out = (Bytef *)dst ;
	strm.avail_out = maximum_after_decompress ;

	ret = inflate(&strm, Z_NO_FLUSH) ;
	assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
	switch (ret) {
	case Z_NEED_DICT:
	  ret = Z_DATA_ERROR;	 /* and fall through */
	case Z_DATA_ERROR:
	case Z_MEM_ERROR:
	  (void)inflateEnd(&strm);
	  return ret;
	}

	assert(strm.avail_in == 0);	 /* all input will be used */

	if( outbytes != NULL )
		*outbytes = strm.total_out;

	/* clean up and return */
	(void)inflateEnd(&strm);
	return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

/* report a zlib or i/o error */
void zerr(int ret)
{
	fputs("zpipe: ", stderr);
	switch (ret) {
	case Z_ERRNO:
		if (ferror(stdin))
			fputs("error reading stdin\n", stderr);
		if (ferror(stdout))
			fputs("error writing stdout\n", stderr);
		break;
	case Z_STREAM_ERROR:
		fputs("invalid compression level\n", stderr);
		break;
	case Z_DATA_ERROR:
		fputs("invalid or incomplete deflate data\n", stderr);
		break;
	case Z_MEM_ERROR:
		fputs("out of memory\n", stderr);
		break;
	case Z_VERSION_ERROR:
		fputs("zlib version mismatch!\n", stderr);
	}
}
