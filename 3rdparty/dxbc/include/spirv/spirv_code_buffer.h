#pragma once

#include <iostream>
#include <utility>
#include <vector>

#include "spirv_instruction.h"

namespace dxvk {
  
  /**
   * \brief SPIR-V code buffer
   * 
   * Helper class for generating SPIR-V shaders.
   * Stores arbitrary SPIR-V instructions in a
   * format that can be read by Vulkan drivers.
   */
  class SpirvCodeBuffer {
    
  public:
    
    SpirvCodeBuffer();
    explicit SpirvCodeBuffer(uint32_t size);
    SpirvCodeBuffer(const SpirvCodeBuffer &) = default;
    SpirvCodeBuffer(SpirvCodeBuffer &&) = default;
    SpirvCodeBuffer(uint32_t size, const uint32_t* data);
    SpirvCodeBuffer(std::istream& stream);
    
    template<size_t N>
    SpirvCodeBuffer(const uint32_t (&data)[N])
    : SpirvCodeBuffer(N, data) { }
    
    ~SpirvCodeBuffer();

    SpirvCodeBuffer &operator=(const SpirvCodeBuffer &) = default;
    SpirvCodeBuffer &operator=(SpirvCodeBuffer &&) = default;
    
    /**
     * \brief Code data
     * \returns Code data
     */
    const uint32_t* data() const { return m_code.data(); }
          uint32_t* data()       { return m_code.data(); }
    
    /**
     * \brief Code size, in dwords
     * \returns Code size, in dwords
     */
    uint32_t dwords() const {
      return m_code.size();
    }
    
    /**
     * \brief Code size, in bytes
     * \returns Code size, in bytes
     */
    size_t size() const {
      return m_code.size() * sizeof(uint32_t);
    }
    
    /**
     * \brief Begin instruction iterator
     * 
     * Points to the first instruction in the instruction
     * block. The header, if any, will be skipped over.
     * \returns Instruction iterator
     */
    SpirvInstructionIterator begin() {
      return SpirvInstructionIterator(
        m_code.data(), 0, m_code.size());
    }
    
    /**
     * \brief End instruction iterator
     * 
     * Points to the end of the instruction block.
     * \returns Instruction iterator
     */
    SpirvInstructionIterator end() {
      return SpirvInstructionIterator(nullptr, 0, 0);
    }
    
    /**
     * \brief Allocates a new ID
     *
     * Returns a new valid ID and increments the
     * maximum ID count stored in the header.
     * \returns The new SPIR-V ID
     */
    uint32_t allocId();
    
    /**
     * \brief Appends an instruction
     *
     * Slightly faster than individually adding words.
     * \param [in] ins Instruction
     */
    void append(const SpirvInstruction& ins);

    /**
     * \brief Merges two code buffers
     * 
     * This is useful to generate declarations or
     * the SPIR-V header at the same time as the
     * code when doing so in advance is impossible.
     * \param [in] other Code buffer to append
     */
    void append(const SpirvCodeBuffer& other);
    
    /**
     * \brief Appends an 32-bit word to the buffer
     * \param [in] word The word to append
     */
    void putWord(uint32_t word);
    
    /**
     * \brief Appends an instruction word to the buffer
     * 
     * Adds a single word containing both the word count
     * and the op code number for a single instruction.
     * \param [in] opCode Operand code
     * \param [in] wordCount Number of words
     */
    void putIns(spv::Op opCode, uint16_t wordCount);

    /**
     * \brief Appends a 32-bit integer to the buffer
     * \param [in] value The number to add
     */
    void putInt32(uint32_t word);
    
    /**
     * \brief Appends a 64-bit integer to the buffer
     * 
     * A 64-bit integer will take up two 32-bit words.
     * \param [in] value 64-bit value to add
     */
    void putInt64(uint64_t value);
    
    /**
     * \brief Appends a 32-bit float to the buffer
     * \param [in] value The number to add
     */
    void putFloat32(float value);
    
    /**
     * \brief Appends a 64-bit float to the buffer
     * \param [in] value The number to add
     */
    void putFloat64(double value);
    
    /**
     * \brief Appends a literal string to the buffer
     * \param [in] str String to append to the buffer
     */
    void putStr(const char* str);
    
    /**
     * \brief Adds the header to the buffer
     *
     * \param [in] version SPIR-V version
     * \param [in] boundIds Number of bound IDs
     */
    void putHeader(uint32_t version, uint32_t boundIds);

    /**
     * \brief Erases given number of dwords
     *
     * Removes data from the code buffer, starting
     * at the current insertion offset.
     * \param [in] size Number of words to remove
     */
    void erase(size_t size);
    
    /**
     * \brief Computes length of a literal string
     * 
     * \param [in] str The string to check
     * \returns Number of words consumed by a string
     */
    uint32_t strLen(const char* str);
    
    /**
     * \brief Stores the SPIR-V module to a stream
     * 
     * The ability to save modules to a file
     * exists mostly for debugging purposes.
     * \param [in] stream Output stream
     */
    void store(std::ostream& stream) const;
    
    /**
     * \brief Retrieves current insertion pointer
     * 
     * Sometimes it may be necessay to insert code into the
     * middle of the stream rather than appending it. This
     * retrieves the current function pointer. Note that the
     * pointer will become invalid if any code is inserted
     * before the current pointer location.
     * \returns Current instruction pointr
     */
    size_t getInsertionPtr() const {
      return m_ptr;
    }
    
    /**
     * \brief Sets insertion pointer to a specific value
     * 
     * Sets the insertion pointer to a value that was
     * previously retrieved by \ref getInsertionPtr.
     * \returns Current instruction pointr
     */
    void beginInsertion(size_t ptr) {
      m_ptr = ptr;
    }
    
    /**
     * \brief Sets insertion pointer to the end
     * 
     * After this call, new instructions will be
     * appended to the stream. In other words,
     * this will restore default behaviour.
     * \returns Previous instruction pointer
     */
    size_t endInsertion() {
      return std::exchange(m_ptr, m_code.size());
    }
    
  private:
    
    std::vector<uint32_t> m_code;
    size_t m_ptr = 0;
    
  };
  
}
