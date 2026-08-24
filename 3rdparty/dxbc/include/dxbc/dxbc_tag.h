#pragma once

#include "dxbc_include.h"

namespace dxvk {
  
  /**
   * \brief Four-character tag
   * 
   * Used to identify chunks in the
   * compiled DXBC file by name.
   */
  class DxbcTag {
    
  public:
    
    DxbcTag() {
      for (size_t i = 0; i < 4; i++)
        m_chars[i] = '\0';
    }
    
    DxbcTag(const char* tag) {
      for (size_t i = 0; i < 4; i++)
        m_chars[i] = tag[i];
    }
    
    bool operator == (const DxbcTag& other) const {
      bool result = true;
      for (size_t i = 0; i < 4; i++)
        result &= m_chars[i] == other.m_chars[i];
      return result;
    }
    
    bool operator != (const DxbcTag& other) const {
      return !this->operator == (other);
    }
    
    const char* operator & () const { return m_chars; }
          char* operator & ()       { return m_chars; }
    
  private:
    
    char m_chars[4];
    
  };
  
}