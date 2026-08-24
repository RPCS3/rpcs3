#pragma once

#include <array>

#include "dxbc_common.h"
#include "dxbc_decoder.h"
#include "dxbc_defs.h"
#include "dxbc_enums.h"
#include "dxbc_names.h"

namespace dxvk {
  
  constexpr size_t DxbcMaxRegIndexDim = 3;
  
  struct DxbcRegister;
  
  /**
   * \brief Source operand modifiers
   * 
   * These are applied after loading
   * an operand register.
   */
  enum class DxbcRegModifier : uint32_t {
    Neg = 0,
    Abs = 1,
  };
  
  using DxbcRegModifiers = Flags<DxbcRegModifier>;
  
  
  /**
   * \brief Constant buffer binding
   * 
   * Stores information required to
   * access a constant buffer.
   */
  struct DxbcConstantBuffer {
    uint32_t varId  = 0;
    uint32_t size   = 0;
  };
  
  /**
   * \brief Sampler binding
   * 
   * Stores a sampler variable that can be
   * used together with a texture resource.
   */
  struct DxbcSampler {
    uint32_t varId  = 0;
    uint32_t typeId = 0;
  };
  
  
  /**
   * \brief Image type information
   */
  struct DxbcImageInfo {
    spv::Dim        dim     = spv::Dim1D;
    uint32_t        array   = 0;
    uint32_t        ms      = 0;
    uint32_t        sampled = 0;
    VkImageViewType vtype   = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
  };
  
  
  /**
   * \brief Shader resource binding
   * 
   * Stores a resource variable
   * and associated type IDs.
   */
  struct DxbcShaderResource {
    DxbcResourceType  type          = DxbcResourceType::Typed;
    DxbcImageInfo     imageInfo;
    uint32_t          varId         = 0;
    uint32_t          specId        = 0;
    DxbcScalarType    sampledType   = DxbcScalarType::Float32;
    uint32_t          sampledTypeId = 0;
    uint32_t          imageTypeId   = 0;
    uint32_t          colorTypeId   = 0;
    uint32_t          depthTypeId   = 0;
    uint32_t          structStride  = 0;
    bool              isRawSsbo     = false;
  };
  
  
  /**
   * \brief Unordered access binding
   * 
   * Stores a resource variable that is provided
   * by a UAV, as well as associated type IDs.
   */
  struct DxbcUav {
    DxbcResourceType  type          = DxbcResourceType::Typed;
    DxbcImageInfo     imageInfo;
    uint32_t          varId         = 0;
    uint32_t          ctrId         = 0;
    uint32_t          specId        = 0;
    DxbcScalarType    sampledType   = DxbcScalarType::Float32;
    uint32_t          sampledTypeId = 0;
    uint32_t          imageTypeId   = 0;
    uint32_t          structStride  = 0;
    uint32_t          coherence     = 0;
    bool              isRawSsbo     = false;
  };
  
  
  /**
   * \brief Component swizzle
   * 
   * Maps vector components to
   * other vector components.
   */
  class DxbcRegSwizzle {
    
  public:
    
    DxbcRegSwizzle() { }
    DxbcRegSwizzle(uint32_t x, uint32_t y, uint32_t z, uint32_t w)
    : m_mask((x << 0) | (y << 2) | (z << 4) | (w << 6)) { }
    
    uint32_t operator [] (uint32_t id) const {
      return (m_mask >> (id + id)) & 0x3;
    }
    
    bool operator == (const DxbcRegSwizzle& other) const { return m_mask == other.m_mask; }
    bool operator != (const DxbcRegSwizzle& other) const { return m_mask != other.m_mask; }
    
  private:
    
    uint8_t m_mask = 0;
    
  };
  
  
  /**
   * \brief Component mask
   * 
   * Enables access to certain
   * subset of vector components.
   */
  class DxbcRegMask {
    
  public:
    
    DxbcRegMask() { }
    DxbcRegMask(uint32_t mask) : m_mask(mask) { }
    DxbcRegMask(bool x, bool y, bool z, bool w)
    : m_mask((x ? 0x1 : 0) | (y ? 0x2 : 0)
           | (z ? 0x4 : 0) | (w ? 0x8 : 0)) { }
    
    uint32_t raw() const {
      return m_mask;
    }

    bool operator [] (uint32_t id) const {
      return (m_mask >> id) & 1;
    }
    
    uint32_t popCount() const {
      const uint8_t n[16] = { 0, 1, 1, 2, 1, 2, 2, 3,
                              1, 2, 2, 3, 2, 3, 3, 4 };
      return n[m_mask & 0xF];
    }
    
    uint32_t firstSet() const {
      const uint8_t n[16] = { 4, 0, 1, 0, 2, 0, 1, 0,
                              3, 0, 1, 0, 2, 0, 1, 0 };
      return n[m_mask & 0xF];
    }
    
    uint32_t minComponents() const {
      const uint8_t n[16] = { 0, 1, 2, 2, 3, 3, 3, 3,
                              4, 4, 4, 4, 4, 4, 4, 4 };
      return n[m_mask & 0xF];
    }
    
    bool operator == (const DxbcRegMask& other) const { return m_mask == other.m_mask; }
    bool operator != (const DxbcRegMask& other) const { return m_mask != other.m_mask; }
    
    DxbcRegMask& operator |= (const DxbcRegMask& other) {
      m_mask |= other.m_mask;
      return *this;
    }

    static DxbcRegMask firstN(uint32_t n) {
      return DxbcRegMask(n >= 1, n >= 2, n >= 3, n >= 4);
    }
    
    static DxbcRegMask select(uint32_t n) {
      return DxbcRegMask(n == 0, n == 1, n == 2, n == 3);
    }

    std::string maskString() const {
      std::string out = "";
      out += (m_mask & 0x1) ? "x" : "";
      out += (m_mask & 0x2) ? "y" : "";
      out += (m_mask & 0x4) ? "z" : "";
      out += (m_mask & 0x8) ? "w" : "";
      return out;
    }

    explicit operator bool () const {
      return m_mask != 0;
    }
    
  private:
    
    uint8_t m_mask = 0;
    
  };
  
  
  /**
   * \brief System value mapping
   * 
   * Maps a system value to a given set of
   * components of an input or output register.
   */
  struct DxbcSvMapping {
    uint32_t        regId;
    DxbcRegMask     regMask;
    DxbcSystemValue sv;
  };
  
  
  struct DxbcRegIndex {
    DxbcRegister*     relReg;
    int32_t           offset;
  };
  
  
  /**
   * \brief Instruction operand
   */
  struct DxbcRegister {
    DxbcOperandType       type;
    DxbcScalarType        dataType;
    DxbcComponentCount    componentCount;
    
    uint32_t              idxDim;
    DxbcRegIndex          idx[DxbcMaxRegIndexDim];
    
    DxbcRegMask           mask;
    DxbcRegSwizzle        swizzle;
    DxbcRegModifiers      modifiers;
    
    union {
      uint32_t            u32_4[4];
      uint32_t            u32_1;
    } imm;
  };
  
  
  /**
   * \brief Instruction result modifiers
   * 
   * Modifiers that are applied
   * to all destination operands.
   */
  struct DxbcOpModifiers {
    bool saturate;
    bool precise;
  };
  
  
  /**
   * \brief Opcode controls
   * 
   * Instruction-specific controls. Usually,
   * only one of the members will be valid.
   */
  class DxbcShaderOpcodeControls {
    
  public:
    
    DxbcShaderOpcodeControls()
    : m_bits(0) { }
    
    DxbcShaderOpcodeControls(uint32_t bits)
    : m_bits(bits) { }
    
    DxbcInstructionReturnType returnType() const {
      return DxbcInstructionReturnType(bit::extract(m_bits, 11, 11));
    }
    
    DxbcGlobalFlags globalFlags() const {
      return DxbcGlobalFlags(bit::extract(m_bits, 11, 14));
    }
    
    DxbcZeroTest zeroTest() const {
      return DxbcZeroTest(bit::extract(m_bits, 18, 18));
    }
    
    DxbcSyncFlags syncFlags() const {
      return DxbcSyncFlags(bit::extract(m_bits, 11, 14));
    }
    
    DxbcResourceDim resourceDim() const {
      return DxbcResourceDim(bit::extract(m_bits, 11, 15));
    }
    
    DxbcResinfoType resinfoType() const {
      return DxbcResinfoType(bit::extract(m_bits, 11, 12));
    }
    
    DxbcInterpolationMode interpolation() const {
      return DxbcInterpolationMode(bit::extract(m_bits, 11, 14));
    }
    
    DxbcSamplerMode samplerMode() const {
      return DxbcSamplerMode(bit::extract(m_bits, 11, 14));
    }
    
    DxbcPrimitiveTopology primitiveTopology() const {
      return DxbcPrimitiveTopology(bit::extract(m_bits, 11, 17));
    }
    
    DxbcPrimitive primitive() const {
      return DxbcPrimitive(bit::extract(m_bits, 11, 16));
    }
    
    DxbcTessDomain tessDomain() const {
      return DxbcTessDomain(bit::extract(m_bits, 11, 12));
    }
    
    DxbcTessOutputPrimitive tessOutputPrimitive() const {
      return DxbcTessOutputPrimitive(bit::extract(m_bits, 11, 13));
    }
    
    DxbcTessPartitioning tessPartitioning() const {
      return DxbcTessPartitioning(bit::extract(m_bits, 11, 13));
    }
    
    DxbcUavFlags uavFlags() const {
      return DxbcUavFlags(bit::extract(m_bits, 16, 17));
    }

    DxbcConstantBufferAccessType accessType() const {
      return DxbcConstantBufferAccessType(bit::extract(m_bits, 11, 11));
    }
    
    uint32_t controlPointCount() const {
      return bit::extract(m_bits, 11, 16);
    }
    
    bool precise() const {
      return bit::extract(m_bits, 19, 22) != 0;
    }
    
  private:
    
    uint32_t m_bits;
    
  };
  
  
  /**
   * \brief Sample controls
   * 
   * Constant texel offset with
   * values raning from -8 to 7.
   */
  struct DxbcShaderSampleControls {
    int u, v, w;
  };
  
  
  /**
   * \brief Immediate value
   * 
   * Immediate argument represented either
   * as a 32-bit or 64-bit unsigned integer,
   * or a 32-bit or 32-bit floating point number.
   */
  union DxbcImmediate {
    float    f32;
    double   f64;
    uint32_t u32;
    uint64_t u64;
  };
  
  
  /**
   * \brief Shader instruction
   * 
   * Note that this structure may store pointer to
   * external structures, such as the original code
   * buffer. This is safe to use if and only if:
   * - The \ref DxbcDecodeContext that created it
   *   still exists and was not moved
   * - The code buffer that was being decoded
   *   still exists and was not moved.
   */
  struct DxbcShaderInstruction {
    DxbcOpcode                op;
    DxbcInstClass             opClass;
    DxbcOpModifiers           modifiers;
    DxbcShaderOpcodeControls  controls;
    DxbcShaderSampleControls  sampleControls;
    
    uint32_t                  dstCount;
    uint32_t                  srcCount;
    uint32_t                  immCount;
    
    const DxbcRegister*       dst;
    const DxbcRegister*       src;
    const DxbcImmediate*      imm;
    
    DxbcCustomDataClass       customDataType;
    uint32_t                  customDataSize;
    const uint32_t*           customData;
  };
  
  
  /**
   * \brief DXBC code slice
   * 
   * Convenient pointer pair that allows
   * reading the code word stream safely.
   */
  class DxbcCodeSlice {
    
  public:
    
    DxbcCodeSlice(
      const uint32_t* ptr,
      const uint32_t* end)
    : m_ptr(ptr), m_end(end) { }
    
    const uint32_t* ptrAt(uint32_t id) const;
    
    uint32_t at(uint32_t id) const;
    uint32_t read();
    
    DxbcCodeSlice take(uint32_t n) const;
    DxbcCodeSlice skip(uint32_t n) const;
    
    bool atEnd() const {
      return m_ptr == m_end;
    }
    
  private:
    
    const uint32_t* m_ptr = nullptr;
    const uint32_t* m_end = nullptr;
    
  };
  
  
  /**
   * \brief Decode context
   * 
   * Stores data that is required to decode a single
   * instruction. This data is not persistent, so it
   * should be forwarded to the compiler right away.
   */
  class DxbcDecodeContext {
    
  public:
    
    /**
     * \brief Retrieves current instruction
     * 
     * This is only valid after a call to \ref decode.
     * \returns Reference to last decoded instruction
     */
    const DxbcShaderInstruction& getInstruction() const {
      return m_instruction;
    }
    
    /**
     * \brief Decodes an instruction
     * 
     * This also advances the given code slice by the
     * number of dwords consumed by the instruction.
     * \param [in] code Code slice
     */
    void decodeInstruction(DxbcCodeSlice& code);
    
  private:
    
    DxbcShaderInstruction m_instruction;
    
    std::array<DxbcRegister,  8> m_dstOperands;
    std::array<DxbcRegister,  8> m_srcOperands;
    std::array<DxbcImmediate, 4> m_immOperands;
    std::array<DxbcRegister, 12> m_indices;
    
    // Index into the indices array. Used when decoding
    // instruction operands with relative indexing.
    uint32_t m_indexId = 0;
    
    void decodeCustomData(DxbcCodeSlice code);
    void decodeOperation(DxbcCodeSlice code);
    
    void decodeComponentSelection(DxbcRegister& reg, uint32_t token);
    void decodeOperandExtensions(DxbcCodeSlice& code, DxbcRegister& reg, uint32_t token);
    void decodeOperandImmediates(DxbcCodeSlice& code, DxbcRegister& reg);
    void decodeOperandIndex(DxbcCodeSlice& code, DxbcRegister& reg, uint32_t token);
    
    void decodeRegister(DxbcCodeSlice& code, DxbcRegister& reg, DxbcScalarType type);
    void decodeImm32(DxbcCodeSlice& code, DxbcImmediate& imm, DxbcScalarType type);
    
    void decodeOperand(DxbcCodeSlice& code, const DxbcInstOperandFormat& format);
    
  };
  
}
