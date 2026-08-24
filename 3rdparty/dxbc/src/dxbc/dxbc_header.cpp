#include "dxbc_header.h"

namespace dxvk {
  
  DxbcHeader::DxbcHeader(DxbcReader& reader) {
    // FourCC at the start of the file, must be 'DXBC'
    DxbcTag fourcc = reader.readTag();
    
    if (fourcc != "DXBC")
      throw DxvkError("DxbcHeader::DxbcHeader: Invalid fourcc, expected 'DXBC'");
    
    // Stuff we don't actually need to store
    reader.skip(4 * sizeof(uint32_t)); // Check sum
    reader.skip(1 * sizeof(uint32_t)); // Constant 1
    reader.skip(1 * sizeof(uint32_t)); // Bytecode length
    
    // Number of chunks in the file
    uint32_t chunkCount = reader.readu32();
    
    // Chunk offsets are stored immediately after
    for (uint32_t i = 0; i < chunkCount; i++)
      m_chunkOffsets.push_back(reader.readu32());
  }
  
  
  DxbcHeader::~DxbcHeader() {
    
  }
  
}