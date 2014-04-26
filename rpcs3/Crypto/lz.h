#include <string.h>

int decode_range(unsigned int *range, unsigned int *code, unsigned char **src);
int decode_bit(unsigned int *range, unsigned int *code, int *index, unsigned char **src, unsigned char *c);
int decode_number(unsigned char *ptr, int index, int *bit_flag, unsigned int *range, unsigned int *code, unsigned char **src);
int decode_word(unsigned char *ptr, int index, int *bit_flag, unsigned int *range, unsigned int *code, unsigned char **src);
int decompress(unsigned char *out, unsigned char *in, unsigned int size);
