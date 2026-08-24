#include "dxbc_defs.h"

namespace dxvk {
  
  const std::array<DxbcInstFormat, 235> g_instructionFormats = {{
    /* Add                                  */
    { 3, DxbcInstClass::VectorAlu, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* And                                  */
    { 3, DxbcInstClass::VectorAlu, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* Break                                */
    { 0, DxbcInstClass::ControlFlow },
    /* Breakc                               */
    { 1, DxbcInstClass::ControlFlow, {
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* Call                                 */
    { 1, DxbcInstClass::ControlFlow, {
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* Callc                                */
    { 2, DxbcInstClass::ControlFlow, {
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* Case                                 */
    { 1, DxbcInstClass::ControlFlow, {
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* Continue                             */
    { 0, DxbcInstClass::ControlFlow },
    /* Continuec                            */
    { 1, DxbcInstClass::ControlFlow, {
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* Cut                                  */
    { 0, DxbcInstClass::GeometryEmit },
    /* Default                              */
    { 0, DxbcInstClass::ControlFlow },
    /* DerivRtx                             */
    { 2, DxbcInstClass::VectorDeriv, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* DerivRty                             */
    { 2, DxbcInstClass::VectorDeriv, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* Discard                              */
    { 1, DxbcInstClass::ControlFlow, {
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* Div                                  */
    { 3, DxbcInstClass::VectorAlu, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* Dp2                                  */
    { 3, DxbcInstClass::VectorDot, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* Dp3                                  */
    { 3, DxbcInstClass::VectorDot, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* Dp4                                  */
    { 3, DxbcInstClass::VectorDot, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* Else                                 */
    { 0, DxbcInstClass::ControlFlow },
    /* Emit                                 */
    { 0, DxbcInstClass::GeometryEmit },
    /* EmitThenCut                          */
    { 0, DxbcInstClass::GeometryEmit },
    /* EndIf                                */
    { 0, DxbcInstClass::ControlFlow },
    /* EndLoop                              */
    { 0, DxbcInstClass::ControlFlow },
    /* EndSwitch                            */
    { 0, DxbcInstClass::ControlFlow },
    /* Eq                                   */
    { 3, DxbcInstClass::VectorCmp, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* Exp                                  */
    { 2, DxbcInstClass::VectorAlu, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* Frc                                  */
    { 2, DxbcInstClass::VectorAlu, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* FtoI                                 */
    { 2, DxbcInstClass::VectorAlu, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Sint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* FtoU                                 */
    { 2, DxbcInstClass::VectorAlu, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* Ge                                   */
    { 3, DxbcInstClass::VectorCmp, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* IAdd                                 */
    { 3, DxbcInstClass::VectorAlu, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
    } },
    /* If                                   */
    { 1, DxbcInstClass::ControlFlow, {
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* IEq                                  */
    { 3, DxbcInstClass::VectorCmp, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
    } },
    /* IGe                                  */
    { 3, DxbcInstClass::VectorCmp, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
    } },
    /* ILt                                  */
    { 3, DxbcInstClass::VectorCmp, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
    } },
    /* IMad                                 */
    { 4, DxbcInstClass::VectorAlu, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
    } },
    /* IMax                                 */
    { 3, DxbcInstClass::VectorAlu, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
    } },
    /* IMin                                 */
    { 3, DxbcInstClass::VectorAlu, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
    } },
    /* IMul                                 */
    { 4, DxbcInstClass::VectorImul, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::DstReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
    } },
    /* INe                                  */
    { 3, DxbcInstClass::VectorCmp, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
    } },
    /* INeg                                 */
    { 2, DxbcInstClass::VectorAlu, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
    } },
    /* IShl                                 */
    { 3, DxbcInstClass::VectorShift, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* IShr                                 */
    { 3, DxbcInstClass::VectorShift, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* ItoF                                 */
    { 2, DxbcInstClass::VectorAlu, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32  },
    } },
    /* Label                                */
    { 1, DxbcInstClass::ControlFlow, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
    } },
    /* Ld                                   */
    { 3, DxbcInstClass::TextureFetch, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* LdMs                                 */
    { 4, DxbcInstClass::TextureFetch, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32  },
    } },
    /* Log                                  */
    { 2, DxbcInstClass::VectorAlu, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* Loop                                 */
    { 0, DxbcInstClass::ControlFlow },
    /* Lt                                   */
    { 3, DxbcInstClass::VectorCmp, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* Mad                                  */
    { 4, DxbcInstClass::VectorAlu, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* Min                                  */
    { 3, DxbcInstClass::VectorAlu, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* Max                                  */
    { 3, DxbcInstClass::VectorAlu, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* CustomData                           */
    { 0, DxbcInstClass::CustomData },
    /* Mov                                  */
    { 2, DxbcInstClass::VectorAlu, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* Movc                                 */
    { 4, DxbcInstClass::VectorCmov, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* Mul                                  */
    { 3, DxbcInstClass::VectorAlu, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* Ne                                   */
    { 3, DxbcInstClass::VectorCmp, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* Nop                                  */
    { 0, DxbcInstClass::NoOperation },
    /* Not                                  */
    { 2, DxbcInstClass::VectorAlu, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* Or                                   */
    { 3, DxbcInstClass::VectorAlu, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* ResInfo                              */
    { 3, DxbcInstClass::TextureQuery, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* Ret                                  */
    { 0, DxbcInstClass::ControlFlow },
    /* Retc                                 */
    { 1, DxbcInstClass::ControlFlow, {
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* RoundNe                              */
    { 2, DxbcInstClass::VectorAlu, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* RoundNi                              */
    { 2, DxbcInstClass::VectorAlu, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* RoundPi                              */
    { 2, DxbcInstClass::VectorAlu, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* RoundZ                               */
    { 2, DxbcInstClass::VectorAlu, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* Rsq                                  */
    { 2, DxbcInstClass::VectorAlu, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* Sample                               */
    { 4, DxbcInstClass::TextureSample, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* SampleC                              */
    { 5, DxbcInstClass::TextureSample, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* SampleClz                            */
    { 5, DxbcInstClass::TextureSample, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* SampleL                              */
    { 5, DxbcInstClass::TextureSample, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* SampleD                              */
    { 6, DxbcInstClass::TextureSample, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* SampleB                              */
    { 5, DxbcInstClass::TextureSample, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* Sqrt                                 */
    { 2, DxbcInstClass::VectorAlu, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* Switch                               */
    { 1, DxbcInstClass::ControlFlow, {
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* SinCos                               */
    { 3, DxbcInstClass::VectorSinCos, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* UDiv                                 */
    { 4, DxbcInstClass::VectorIdiv, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* ULt                                  */
    { 3, DxbcInstClass::VectorCmp, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* UGe                                  */
    { 3, DxbcInstClass::VectorCmp, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* UMul                                 */
    { 4, DxbcInstClass::VectorImul, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* UMad                                 */
    { 4, DxbcInstClass::VectorAlu, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* UMax                                 */
    { 3, DxbcInstClass::VectorAlu, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* UMin                                 */
    { 3, DxbcInstClass::VectorAlu, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* UShr                                 */
    { 3, DxbcInstClass::VectorShift, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* UtoF                                 */
    { 2, DxbcInstClass::VectorAlu, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32  },
    } },
    /* Xor                                  */
    { 3, DxbcInstClass::VectorAlu, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* DclResource                          */
    { 2, DxbcInstClass::Declaration, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::Imm32,  DxbcScalarType::Uint32  },
    } },
    /* DclConstantBuffer                    */
    { 1, DxbcInstClass::Declaration, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
    } },
    /* DclSampler                           */
    { 1, DxbcInstClass::Declaration, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
    } },
    /* DclIndexRange                        */
    { 2, DxbcInstClass::Declaration, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::Imm32,  DxbcScalarType::Uint32  },
    } },
    /* DclGsOutputPrimitiveTopology         */
    { 0, DxbcInstClass::Declaration },
    /* DclGsInputPrimitive                  */
    { 0, DxbcInstClass::Declaration },
    /* DclMaxOutputVertexCount              */
    { 1, DxbcInstClass::Declaration, {
      { DxbcOperandKind::Imm32,  DxbcScalarType::Uint32 },
    } },
    /* DclInput                             */
    { 1, DxbcInstClass::Declaration, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
    } },
    /* DclInputSgv                          */
    { 2, DxbcInstClass::Declaration, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::Imm32,  DxbcScalarType::Uint32  },
    } },
    /* DclInputSiv                          */
    { 2, DxbcInstClass::Declaration, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::Imm32,  DxbcScalarType::Uint32  },
    } },
    /* DclInputPs                           */
    { 1, DxbcInstClass::Declaration, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
    } },
    /* DclInputPsSgv                        */
    { 2, DxbcInstClass::Declaration, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::Imm32,  DxbcScalarType::Uint32  },
    } },
    /* DclInputPsSiv                        */
    { 2, DxbcInstClass::Declaration, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::Imm32,  DxbcScalarType::Uint32  },
    } },
    /* DclOutput                            */
    { 1, DxbcInstClass::Declaration, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
    } },
    /* DclOutputSgv                         */
    { 2, DxbcInstClass::Declaration, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::Imm32,  DxbcScalarType::Uint32  },
    } },
    /* DclOutputSiv                         */
    { 2, DxbcInstClass::Declaration, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::Imm32,  DxbcScalarType::Uint32  },
    } },
    /* DclTemps                             */
    { 1, DxbcInstClass::Declaration, {
      { DxbcOperandKind::Imm32, DxbcScalarType::Uint32 },
    } },
    /* DclIndexableTemp                     */
    { 3, DxbcInstClass::Declaration, {
      { DxbcOperandKind::Imm32, DxbcScalarType::Uint32 },
      { DxbcOperandKind::Imm32, DxbcScalarType::Uint32 },
      { DxbcOperandKind::Imm32, DxbcScalarType::Uint32 },
    } },
    /* DclGlobalFlags                       */
    { 0, DxbcInstClass::Declaration },
    /* Reserved0                            */
    { 0, DxbcInstClass::Undefined },
    /* Lod                                  */
    { 4, DxbcInstClass::TextureQueryLod, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* Gather4                              */
    { 4, DxbcInstClass::TextureGather, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* SamplePos                            */
    { 3, DxbcInstClass::TextureQueryMsPos, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32  },
    } },
    /* SampleInfo                           */
    { 2, DxbcInstClass::TextureQueryMs, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* Reserved1                            */
    { },
    /* HsDecls                              */
    { 0, DxbcInstClass::HullShaderPhase },
    /* HsControlPointPhase                  */
    { 0, DxbcInstClass::HullShaderPhase },
    /* HsForkPhase                          */
    { 0, DxbcInstClass::HullShaderPhase },
    /* HsJoinPhase                          */
    { 0, DxbcInstClass::HullShaderPhase },
    /* EmitStream                           */
    { 1, DxbcInstClass::GeometryEmit, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32  },
    } },
    /* CutStream                            */
    { 1, DxbcInstClass::GeometryEmit, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32  },
    } },
    /* EmitThenCutStream                    */
    { 1, DxbcInstClass::GeometryEmit, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32  },
    } },
    /* InterfaceCall                        */
    { },
    /* BufInfo                              */
    { 2, DxbcInstClass::BufferQuery, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32  },
    } },
    /* DerivRtxCoarse                       */
    { 2, DxbcInstClass::VectorDeriv, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* DerivRtxFine                         */
    { 2, DxbcInstClass::VectorDeriv, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* DerivRtyCoarse                       */
    { 2, DxbcInstClass::VectorDeriv, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* DerivRtyFine                         */
    { 2, DxbcInstClass::VectorDeriv, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* Gather4C                             */
    { 5, DxbcInstClass::TextureGather, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* Gather4Po                            */
    { 5, DxbcInstClass::TextureGather, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* Gather4PoC                           */
    { 6, DxbcInstClass::TextureGather, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* Rcp                                  */
    { 2, DxbcInstClass::VectorAlu, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* F32toF16                             */
    { 2, DxbcInstClass::ConvertFloat16, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* F16toF32                             */
    { 2, DxbcInstClass::ConvertFloat16, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32  },
    } },
    /* UAddc                                */
    { },
    /* USubb                                */
    { },
    /* CountBits                            */
    { 2, DxbcInstClass::VectorAlu, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* FirstBitHi                           */
    { 2, DxbcInstClass::BitScan, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* FirstBitLo                           */
    { 2, DxbcInstClass::BitScan, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* FirstBitShi                          */
    { 2, DxbcInstClass::BitScan, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* UBfe                                 */
    { 4, DxbcInstClass::BitExtract, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* IBfe                                 */
    { 4, DxbcInstClass::BitExtract, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
    } },
    /* Bfi                                  */
    { 5, DxbcInstClass::BitInsert, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* BfRev                                */
    { 2, DxbcInstClass::VectorAlu, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* Swapc                                */
    { 5, DxbcInstClass::VectorCmov, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* DclStream                            */
    { 1, DxbcInstClass::Declaration, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32  },
    } },
    /* DclFunctionBody                      */
    { },
    /* DclFunctionTable                     */
    { },
    /* DclInterface                         */
    { },
    /* DclInputControlPointCount            */
    { 0, DxbcInstClass::Declaration },
    /* DclOutputControlPointCount           */
    { 0, DxbcInstClass::Declaration },
    /* DclTessDomain                        */
    { 0, DxbcInstClass::Declaration },
    /* DclTessPartitioning                  */
    { 0, DxbcInstClass::Declaration },
    /* DclTessOutputPrimitive               */
    { 0, DxbcInstClass::Declaration },
    /* DclHsMaxTessFactor                   */
    { 1, DxbcInstClass::Declaration, {
      { DxbcOperandKind::Imm32, DxbcScalarType::Float32 },
    } },
    /* DclHsForkPhaseInstanceCount          */
    { 1, DxbcInstClass::HullShaderInstCnt, {
      { DxbcOperandKind::Imm32, DxbcScalarType::Uint32 },
    } },
    /* DclHsJoinPhaseInstanceCount          */
    { 1, DxbcInstClass::HullShaderInstCnt, {
      { DxbcOperandKind::Imm32, DxbcScalarType::Uint32 },
    } },
    /* DclThreadGroup                       */
    { 3, DxbcInstClass::Declaration, {
      { DxbcOperandKind::Imm32, DxbcScalarType::Uint32 },
      { DxbcOperandKind::Imm32, DxbcScalarType::Uint32 },
      { DxbcOperandKind::Imm32, DxbcScalarType::Uint32 },
    } },
    /* DclUavTyped                          */
    { 2, DxbcInstClass::Declaration, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::Imm32,  DxbcScalarType::Uint32  },
    } },
    /* DclUavRaw                            */
    { 1, DxbcInstClass::Declaration, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
    } },
    /* DclUavStructured                     */
    { 2, DxbcInstClass::Declaration, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::Imm32,  DxbcScalarType::Uint32  },
    } },
    /* DclThreadGroupSharedMemoryRaw        */
    { 2, DxbcInstClass::Declaration, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::Imm32,  DxbcScalarType::Uint32  },
    } },
    /* DclThreadGroupSharedMemoryStructured */
    { 3, DxbcInstClass::Declaration, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::Imm32,  DxbcScalarType::Uint32  },
      { DxbcOperandKind::Imm32,  DxbcScalarType::Uint32  },
    } },
    /* DclResourceRaw                       */
    { 1, DxbcInstClass::Declaration, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
    } },
    /* DclResourceStructured                */
    { 2, DxbcInstClass::Declaration, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::Imm32,  DxbcScalarType::Uint32  },
    } },
    /* LdUavTyped                           */
    { 3, DxbcInstClass::TypedUavLoad, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32  },
    } },
    /* StoreUavTyped                        */
    { 3, DxbcInstClass::TypedUavStore, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* LdRaw                                */
    { 3, DxbcInstClass::BufferLoad, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* StoreRaw                             */
    { 3, DxbcInstClass::BufferStore, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* LdStructured                         */
    { 4, DxbcInstClass::BufferLoad, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* StoreStructured                      */
    { 4, DxbcInstClass::BufferStore, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* AtomicAnd                            */
    { 3, DxbcInstClass::Atomic, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* AtomicOr                             */
    { 3, DxbcInstClass::Atomic, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* AtomicXor                            */
    { 3, DxbcInstClass::Atomic, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* AtomicCmpStore                       */
    { 4, DxbcInstClass::Atomic, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* AtomicIAdd                           */
    { 3, DxbcInstClass::Atomic, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* AtomicIMax                           */
    { 3, DxbcInstClass::Atomic, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
    } },
    /* AtomicIMin                           */
    { 3, DxbcInstClass::Atomic, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
    } },
    /* AtomicUMax                           */
    { 3, DxbcInstClass::Atomic, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* AtomicUMin                           */
    { 3, DxbcInstClass::Atomic, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* ImmAtomicAlloc                       */
    { 2, DxbcInstClass::AtomicCounter, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
    } },
    /* ImmAtomicConsume                     */
    { 2, DxbcInstClass::AtomicCounter, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
    } },
    /* ImmAtomicIAdd                        */
    { 4, DxbcInstClass::Atomic, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* ImmAtomicAnd                         */
    { 4, DxbcInstClass::Atomic, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* ImmAtomicOr                          */
    { 4, DxbcInstClass::Atomic, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* ImmAtomicXor                         */
    { 4, DxbcInstClass::Atomic, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* ImmAtomicExch                        */
    { 4, DxbcInstClass::Atomic, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* ImmAtomicCmpExch                     */
    { 5, DxbcInstClass::Atomic, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* ImmAtomicIMax                        */
    { 4, DxbcInstClass::Atomic, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::DstReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
    } },
    /* ImmAtomicIMin                        */
    { 4, DxbcInstClass::Atomic, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::DstReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
    } },
    /* ImmAtomicUMax                        */
    { 4, DxbcInstClass::Atomic, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* ImmAtomicUMin                        */
    { 4, DxbcInstClass::Atomic, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* Sync                                 */
    { 0, DxbcInstClass::Barrier },
    /* DAdd                                 */
    { 3, DxbcInstClass::VectorAlu, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float64 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float64 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float64 },
    } },
    /* DMax                                 */
    { 3, DxbcInstClass::VectorAlu, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float64 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float64 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float64 },
    } },
    /* DMin                                 */
    { 3, DxbcInstClass::VectorAlu, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float64 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float64 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float64 },
    } },
    /* DMul                                 */
    { 3, DxbcInstClass::VectorAlu, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float64 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float64 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float64 },
    } },
    /* DEq                                  */
    { 3, DxbcInstClass::VectorCmp, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float64 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float64 },
    } },
    /* DGe                                  */
    { 3, DxbcInstClass::VectorCmp, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float64 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float64 },
    } },
    /* DLt                                  */
    { 3, DxbcInstClass::VectorCmp, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float64 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float64 },
    } },
    /* DNe                                  */
    { 3, DxbcInstClass::VectorCmp, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float64 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float64 },
    } },
    /* DMov                                 */
    { 2, DxbcInstClass::VectorAlu, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float64 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float64 },
    } },
    /* DMovc                                */
    { 4, DxbcInstClass::VectorCmov, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float64 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float64 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float64 },
    } },
    /* DtoF                                 */
    { 2, DxbcInstClass::ConvertFloat64, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float64 },
    } },
    /* FtoD                                 */
    { 2, DxbcInstClass::ConvertFloat64, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float64 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* EvalSnapped                          */
    { 3, DxbcInstClass::Interpolate, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32  },
    } },
    /* EvalSampleIndex                      */
    { 3, DxbcInstClass::Interpolate, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32  },
    } },
    /* EvalCentroid                         */
    { 2, DxbcInstClass::Interpolate, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* DclGsInstanceCount                   */
    { 1, DxbcInstClass::Declaration, {
      { DxbcOperandKind::Imm32, DxbcScalarType::Uint32 },
    } },
    /* Abort                                */
    { },
    /* DebugBreak                           */
    { },
    /* ReservedBegin11_1                    */
    { },
    /* DDiv                                 */
    { 3, DxbcInstClass::VectorAlu, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float64 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float64 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float64 },
    } },
    /* DFma                                 */
    { 4, DxbcInstClass::VectorAlu, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float64 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float64 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float64 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float64 },
    } },
    /* DRcp                                 */
    { 2, DxbcInstClass::VectorAlu, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float64 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float64 },
    } },
    /* Msad                                 */
    { 4, DxbcInstClass::VectorMsad, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* DtoI                                 */
    { 2, DxbcInstClass::ConvertFloat64, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Sint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float64 },
    } },
    /* DtoU                                 */
    { 2, DxbcInstClass::ConvertFloat64, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float64 },
    } },
    /* ItoD                                 */
    { 2, DxbcInstClass::ConvertFloat64, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float64 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32  },
    } },
    /* UtoD                                 */
    { 2, DxbcInstClass::ConvertFloat64, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float64 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32  },
    } },
    /* ReservedBegin11_2                    */
    { },
    /* Gather4S                             */
    { 5, DxbcInstClass::TextureGather, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* Gather4CS                            */
    { 6, DxbcInstClass::TextureGather, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* Gather4PoS                           */
    { 6, DxbcInstClass::TextureGather, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* Gather4PoCS                          */
    { 7, DxbcInstClass::TextureGather, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* LdS                                  */
    { 4, DxbcInstClass::TextureFetch, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* LdMsS                                */
    { 5, DxbcInstClass::TextureFetch, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32  },
    } },
    /* LdUavTypedS                          */
    { 4, DxbcInstClass::TypedUavLoad, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32  },
    } },
    /* LdRawS                               */
    { 4, DxbcInstClass::BufferLoad, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* LdStructuredS                        */
    { 5, DxbcInstClass::BufferLoad, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Sint32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32 },
    } },
    /* SampleLS                             */
    { 6, DxbcInstClass::TextureSample, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* SampleClzS                           */
    { 6, DxbcInstClass::TextureSample, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* SampleClampS                         */
    { 6, DxbcInstClass::TextureSample, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* SampleBClampS                        */
    { 7, DxbcInstClass::TextureSample, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* SampleDClampS                        */
    { 8, DxbcInstClass::TextureSample, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* SampleCClampS                        */
    { 7, DxbcInstClass::TextureSample, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Float32 },
    } },
    /* CheckAccessFullyMapped               */
    { 2, DxbcInstClass::SparseCheckAccess, {
      { DxbcOperandKind::DstReg, DxbcScalarType::Uint32  },
      { DxbcOperandKind::SrcReg, DxbcScalarType::Uint32  },
    } },
  }};
  
  
  DxbcInstFormat dxbcInstructionFormat(DxbcOpcode opcode) {
    const uint32_t idx = static_cast<uint32_t>(opcode);
    
    return (idx < g_instructionFormats.size())
      ? g_instructionFormats.at(idx)
      : DxbcInstFormat();
  }
  
}