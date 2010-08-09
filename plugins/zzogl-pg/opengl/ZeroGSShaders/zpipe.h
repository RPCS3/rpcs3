 /* ZeroGS KOSMOS
  *
  * Zerofrog's ZeroGS KOSMOS (c)2005-2008
  *
  * Zerofrog forgot to write any copyright notice after release the plugin into GPLv2
  * If someone can contact him successfully to clarify this matter that would be great.
  */

#ifndef zpipe_h
#define zpipe_h

int def(char *src, char *dst, int bytes_to_compress, int *bytes_after_compressed) ;
int inf(char *src, char *dst, int bytes_to_decompress, int maximum_after_decompress, int* outbytes);

#endif
