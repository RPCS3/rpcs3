#include "PPUInterpreter.h"
ExperimentalPPU::ExperimentalPPU(){Reset();}
void ExperimentalPPU::Reset(){for(unsigned int i=0;i<32;++i)r_[i]=0;}
unsigned int ExperimentalPPU::GetRegister(unsigned int i) const{return i<32?r_[i]:0u;}
bool ExperimentalPPU::Execute(unsigned int op){ if((op>>26)!=14u)return false; unsigned int rt=(op>>21)&31u,ra=(op>>16)&31u; int imm=(short)(op&65535u);r_[rt]=(ra?r_[ra]:0u)+(unsigned int)imm;return true;}
