#pragma once

#include "dxbc_enums.h"

namespace dxvk {
  
  constexpr size_t DxbcMaxInterfaceRegs = 32;
  constexpr size_t DxbcMaxOperandCount  = 8;
  
  /**
   * \brief Operand kind
   * 
   * In the instruction format definition, this specified
   * whether an operand uses an actual operand token, or
   * whether it is stored as an immediate value.
   */
  enum class DxbcOperandKind {
    DstReg, ///< Destination register
    SrcReg, ///< Source register
    Imm32,  ///< Constant number
  };
  
  /**
   * \brief Instruction class
   * 
   * Instructions with a similar format are grouped into
   * instruction classes in order to make implementing
   * new instructions easier.
   */
  enum class DxbcInstClass {
    Declaration,        ///< Interface or resource declaration
    CustomData,         ///< Immediate constant buffer
    ControlFlow,        ///< Control flow instructions
    GeometryEmit,       ///< Special geometry shader instructions
    Atomic,             ///< Atomic operations
    AtomicCounter,      ///< Atomic counter operations
    Barrier,            ///< Execution or memory barrier
    BitExtract,         ///< Bit field extract operations
    BitInsert,          ///< Bit field insert operations
    BitScan,            ///< Bit scan operations
    BufferQuery,        ///< Buffer query instruction
    BufferLoad,         ///< Structured or raw buffer load
    BufferStore,        ///< Structured or raw buffer store
    ConvertFloat16,     ///< 16-bit float packing/unpacking
    ConvertFloat64,     ///< 64-bit float conversion
    HullShaderPhase,    ///< Hull shader phase declaration
    HullShaderInstCnt,  ///< Hull shader phase instance count
    Interpolate,        ///< Input attribute interpolation
    NoOperation,        ///< The most useful instruction class
    SparseCheckAccess,  ///< Verifies sparse resource access
    TextureQuery,       ///< Texture query instruction
    TextureQueryLod,    ///< Texture LOD query instruction
    TextureQueryMs,     ///< Multisample texture query
    TextureQueryMsPos,  ///< Sample position query
    TextureFetch,       ///< Texture fetch instruction
    TextureGather,      ///< Texture gather instruction
    TextureSample,      ///< Texture sampling instruction
    TypedUavLoad,       ///< Typed UAV load
    TypedUavStore,      ///< Typed UAV store
    VectorAlu,          ///< Component-wise vector instructions
    VectorCmov,         ///< Component-wise conditional move
    VectorCmp,          ///< Component-wise vector comparison
    VectorDeriv,        ///< Vector derivatives
    VectorDot,          ///< Dot product instruction
    VectorIdiv,         ///< Component-wise integer division
    VectorImul,         ///< Component-wise integer multiplication
    VectorMsad,         ///< Component-wise sum of absolute difference
    VectorShift,        ///< Bit shift operations on vectors
    VectorSinCos,       ///< Sine and Cosine instruction
    Undefined,          ///< Instruction code not defined
  };
  
  /**
   * \brief Instruction operand format
   * 
   * Stores the kind and the expected data type
   * of an operand. Used when parsing instructions.
   */
  struct DxbcInstOperandFormat {
    DxbcOperandKind kind;
    DxbcScalarType  type;
  };
  
  /**
   * \brief Instruction format
   * 
   * Defines the instruction class as well as
   * the format of the insttruction operands.
   */
  struct DxbcInstFormat {
    uint32_t              operandCount      = 0;
    DxbcInstClass         instructionClass  = DxbcInstClass::Undefined;
    DxbcInstOperandFormat operands[DxbcMaxOperandCount];
  };
  
  /**
   * \brief Retrieves instruction format info
   * 
   * \param [in] opcode The opcode to retrieve
   * \returns Instruction format info
   */
  DxbcInstFormat dxbcInstructionFormat(DxbcOpcode opcode);
  
}