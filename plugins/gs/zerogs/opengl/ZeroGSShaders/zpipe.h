#ifndef zpipe_h
#define zpipe_h

int def(char *src, char *dst, int bytes_to_compress, int *bytes_after_compressed) ;
int inf(char *src, char *dst, int bytes_to_decompress, int maximum_after_decompress, int* outbytes);

#endif
