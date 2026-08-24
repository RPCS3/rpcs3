#pragma once

#include <vector>

#include "dxbc_reader.h"

namespace dxvk {
  
  /**
   * \brief DXBC header
   * 
   * Stores information about the shader file itself
   * and the data chunks stored inside the file.
   */
  class DxbcHeader {
    
  public:
    
    DxbcHeader(DxbcReader& reader);
    ~DxbcHeader();
    
    /**
     * \brief Number of chunks
     * \returns Chunk count
     */
    uint32_t numChunks() const {
      return m_chunkOffsets.size();
    }
    
    /**
     * \brief Chunk offset
     * 
     * Retrieves the offset of a chunk, in
     * bytes, from the start of the file.
     * \param [in] chunkId Chunk index
     * \returns Byte offset of that chunk
     */
    uint32_t chunkOffset(uint32_t chunkId) const {
      return m_chunkOffsets.at(chunkId);
    }
    
  private:
    
    std::vector<uint32_t> m_chunkOffsets;
    
  };
  
}