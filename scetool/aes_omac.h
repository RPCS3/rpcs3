#ifndef _AES_OMAC_H_
#define _AES_OMAC_H_

#include "types.h"

#define AES_OMAC1_DIGEST_SIZE 0x10

void aes_omac1(scetool::u8 *digest, scetool::u8 *input, scetool::u32 length, scetool::u8 *key, scetool::u32 keybits);

#endif
