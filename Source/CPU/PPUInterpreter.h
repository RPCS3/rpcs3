#ifndef RPCS3XBOX360_PPU_H
#define RPCS3XBOX360_PPU_H
class ExperimentalPPU { public: ExperimentalPPU(); void Reset(); bool Execute(unsigned int instruction); unsigned int GetRegister(unsigned int index) const; private: unsigned int r_[32]; };
#endif
