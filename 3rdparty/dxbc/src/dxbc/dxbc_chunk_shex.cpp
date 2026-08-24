#include "dxbc_chunk_shex.h"

namespace dxvk {
  
  DxbcShex::DxbcShex(DxbcReader reader) {
    // The shader version and type are stored in a 32-bit unit,
    // where the first byte contains the major and minor version
    // numbers, and the high word contains the program type.
    reader.skip(2);
    auto pType    = reader.readEnum<DxbcProgramType>();
    m_programInfo = DxbcProgramInfo(pType);
    
    // Read the actual shader code as an array of DWORDs.
    auto codeLength = reader.readu32() - 2;
    m_code.resize(codeLength);
    reader.read(m_code.data(), codeLength * sizeof(uint32_t));
  }
  
  
  DxbcShex::~DxbcShex() {
    
  }
  
}
