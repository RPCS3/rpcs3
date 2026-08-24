#include <cstring>

#include "dxbc_reader.h"

namespace dxvk {
  
  DxbcTag DxbcReader::readTag() {
    DxbcTag tag;
    this->read(&tag, 4);
    return tag;
  }
  
  
  std::string DxbcReader::readString() {
    std::string result;
    
    while (m_data[m_pos] != '\0')
      result.push_back(m_data[m_pos++]);
    
    m_pos++;
    return result;
  }
  
  
  void DxbcReader::read(void* dst, size_t n) {
    if (m_pos + n > m_size)
      throw DxvkError("DxbcReader::read: Unexpected end of file");
    std::memcpy(dst, m_data + m_pos, n);
    m_pos += n;
  }
  
  
  void DxbcReader::skip(size_t n) {
    if (m_pos + n > m_size)
      throw DxvkError("DxbcReader::skip: Unexpected end of file");
    m_pos += n;
  }
  
  
  DxbcReader DxbcReader::clone(size_t pos) const {
    if (pos > m_size)
      throw DxvkError("DxbcReader::clone: Invalid offset");
    return DxbcReader(m_data + pos, m_size - pos);
  }
  
  
  DxbcReader DxbcReader::resize(size_t size) const {
    if (size > m_size)
      throw DxvkError("DxbcReader::resize: Invalid size");
    return DxbcReader(m_data, size, m_pos);
  }
  
  
  void DxbcReader::store(std::ostream&& stream) const {
    stream.write(m_data, m_size);
  }
  
}