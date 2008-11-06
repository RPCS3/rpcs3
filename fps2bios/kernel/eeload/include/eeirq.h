#ifndef __EEIRQ_H__
#define __EEIRQ_H__

#include <tamtypes.h>

void CpuException0();
void CpuException();
void CpuException2();
void SyscException();
void _Deci2Call();
void INTCException();
void DMACException();
void TIMERException();
void setINTCHandler(int n, void (*phandler)(int));
void setDMACHandler(int n, void (*phandler)(int));

#endif /* __EEIRQ_H__ */
