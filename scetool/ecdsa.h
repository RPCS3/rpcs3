#ifndef _ECDSA_H_
#define _ECDSA_H_

int ecdsa_set_curve(scetool::u32 type);
void ecdsa_set_pub(scetool::u8 *Q);
void ecdsa_set_priv(scetool::u8 *k);
int ecdsa_verify(scetool::u8 *hash, scetool::u8 *R, scetool::u8 *S);
void ecdsa_sign(scetool::u8 *hash, scetool::u8 *R, scetool::u8 *S);

#endif
