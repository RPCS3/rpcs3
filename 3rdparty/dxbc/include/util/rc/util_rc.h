#pragma once

#include <atomic>

#include "../util_likely.h"

namespace dxvk {
  
  /**
   * \brief Reference-counted object
   */
  class RcObject {
    
  public:
    
    /**
     * \brief Increments reference count
     * \returns New reference count
     */
    force_inline uint32_t incRef() {
      return ++m_refCount;
    }
    
    /**
     * \brief Decrements reference count
     * \returns New reference count
     */
    force_inline uint32_t decRef() {
      return --m_refCount;
    }
    
  private:
    
    std::atomic<uint32_t> m_refCount = { 0u };
    
  };
  
}