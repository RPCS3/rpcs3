#pragma once

// Copyright (C) 2014       Hykem <hykem@hotmail.com>
// Licensed under the terms of the GNU GPL, version 2.0 or later versions.
// http://www.gnu.org/licenses/gpl-2.0.txt

#include "util/types.hpp"

void ecdsa_set_curve(const u8* p, const u8* a, const u8* b, const u8* N, const u8* Gx, const u8* Gy);
void ecdsa_set_pub(const u8* Q);
void ecdsa_set_priv(const u8* k);
bool ecdsa_verify(const u8* hash, u8* R, u8* S);
