/*********************************************************************
 * Copyright (C) 2003 Tord Lindstrom (pukko@home.se)
 * This file is subject to the terms and conditions of the PS2Link License.
 * See the file LICENSE in the main directory of this distribution for more
 * details.
 */

#ifndef _EXCEPTION_H_
#define _EXCEPTION_H_

void installExceptionHandlers(void);
void iopException(int cause, int badvaddr, int status, int epc, u32 *regs, int repc, char* name);

#endif
