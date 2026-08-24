#pragma once

#include "dxbc_common.h"
#include "dxbc_decoder.h"
#include "dxbc_reader.h"

namespace dxvk {
  
  /**
   * \brief Shader code chunk
   * 
   * Stores the DXBC shader code itself, as well
   * as some meta info about the shader, i.e. what
   * type of shader this is.
   */
  class DxbcShex : public RcObject {
    
  public:
    
    DxbcShex(DxbcReader reader);
    ~DxbcShex();
    
    DxbcProgramInfo programInfo() const {
      return m_programInfo;
    }
    
    DxbcCodeSlice slice() const {
      return DxbcCodeSlice(m_code.data(),
        m_code.data() + m_code.size());
    }
    
  private:
    
    DxbcProgramInfo       m_programInfo;
    std::vector<uint32_t> m_code;
    
  };
  
}
