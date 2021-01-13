#pragma once

// Copyright (C) 2014       Hykem <hykem@hotmail.com>
// Licensed under the terms of the GNU GPL, version 3
// http://www.gnu.org/licenses/gpl-3.0.txt

#include <string.h>
#include <stdio.h>

int ecdsa_set_curve(const unsigned char *p, const unsigned char *a, const unsigned char *b, const unsigned char *N, const unsigned char *Gx, const unsigned char *Gy);
void ecdsa_set_pub(const unsigned char *Q);
void ecdsa_set_priv(const unsigned char *k);
int ecdsa_verify(unsigned char *hash, unsigned char *R, unsigned char *S);
