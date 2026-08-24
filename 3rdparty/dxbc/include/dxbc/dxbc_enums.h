#pragma once

#include "dxbc_include.h"

namespace dxvk {
  
  /**
   * \brief Instruction code listing
   */
  enum class DxbcOpcode : uint32_t {
    Add                                  = 0,
    And                                  = 1,
    Break                                = 2,
    Breakc                               = 3,
    Call                                 = 4,
    Callc                                = 5,
    Case                                 = 6,
    Continue                             = 7,
    Continuec                            = 8,
    Cut                                  = 9,
    Default                              = 10,
    DerivRtx                             = 11,
    DerivRty                             = 12,
    Discard                              = 13,
    Div                                  = 14,
    Dp2                                  = 15,
    Dp3                                  = 16,
    Dp4                                  = 17,
    Else                                 = 18,
    Emit                                 = 19,
    EmitThenCut                          = 20,
    EndIf                                = 21,
    EndLoop                              = 22,
    EndSwitch                            = 23,
    Eq                                   = 24,
    Exp                                  = 25,
    Frc                                  = 26,
    FtoI                                 = 27,
    FtoU                                 = 28,
    Ge                                   = 29,
    IAdd                                 = 30,
    If                                   = 31,
    IEq                                  = 32,
    IGe                                  = 33,
    ILt                                  = 34,
    IMad                                 = 35,
    IMax                                 = 36,
    IMin                                 = 37,
    IMul                                 = 38,
    INe                                  = 39,
    INeg                                 = 40,
    IShl                                 = 41,
    IShr                                 = 42,
    ItoF                                 = 43,
    Label                                = 44,
    Ld                                   = 45,
    LdMs                                 = 46,
    Log                                  = 47,
    Loop                                 = 48,
    Lt                                   = 49,
    Mad                                  = 50,
    Min                                  = 51,
    Max                                  = 52,
    CustomData                           = 53,
    Mov                                  = 54,
    Movc                                 = 55,
    Mul                                  = 56,
    Ne                                   = 57,
    Nop                                  = 58,
    Not                                  = 59,
    Or                                   = 60,
    ResInfo                              = 61,
    Ret                                  = 62,
    Retc                                 = 63,
    RoundNe                              = 64,
    RoundNi                              = 65,
    RoundPi                              = 66,
    RoundZ                               = 67,
    Rsq                                  = 68,
    Sample                               = 69,
    SampleC                              = 70,
    SampleClz                            = 71,
    SampleL                              = 72,
    SampleD                              = 73,
    SampleB                              = 74,
    Sqrt                                 = 75,
    Switch                               = 76,
    SinCos                               = 77,
    UDiv                                 = 78,
    ULt                                  = 79,
    UGe                                  = 80,
    UMul                                 = 81,
    UMad                                 = 82,
    UMax                                 = 83,
    UMin                                 = 84,
    UShr                                 = 85,
    UtoF                                 = 86,
    Xor                                  = 87,
    DclResource                          = 88,
    DclConstantBuffer                    = 89,
    DclSampler                           = 90,
    DclIndexRange                        = 91,
    DclGsOutputPrimitiveTopology         = 92,
    DclGsInputPrimitive                  = 93,
    DclMaxOutputVertexCount              = 94,
    DclInput                             = 95,
    DclInputSgv                          = 96,
    DclInputSiv                          = 97,
    DclInputPs                           = 98,
    DclInputPsSgv                        = 99,
    DclInputPsSiv                        = 100,
    DclOutput                            = 101,
    DclOutputSgv                         = 102,
    DclOutputSiv                         = 103,
    DclTemps                             = 104,
    DclIndexableTemp                     = 105,
    DclGlobalFlags                       = 106,
    Reserved0                            = 107,
    Lod                                  = 108,
    Gather4                              = 109,
    SamplePos                            = 110,
    SampleInfo                           = 111,
    Reserved1                            = 112,
    HsDecls                              = 113,
    HsControlPointPhase                  = 114,
    HsForkPhase                          = 115,
    HsJoinPhase                          = 116,
    EmitStream                           = 117,
    CutStream                            = 118,
    EmitThenCutStream                    = 119,
    InterfaceCall                        = 120,
    BufInfo                              = 121,
    DerivRtxCoarse                       = 122,
    DerivRtxFine                         = 123,
    DerivRtyCoarse                       = 124,
    DerivRtyFine                         = 125,
    Gather4C                             = 126,
    Gather4Po                            = 127,
    Gather4PoC                           = 128,
    Rcp                                  = 129,
    F32toF16                             = 130,
    F16toF32                             = 131,
    UAddc                                = 132,
    USubb                                = 133,
    CountBits                            = 134,
    FirstBitHi                           = 135,
    FirstBitLo                           = 136,
    FirstBitShi                          = 137,
    UBfe                                 = 138,
    IBfe                                 = 139,
    Bfi                                  = 140,
    BfRev                                = 141,
    Swapc                                = 142,
    DclStream                            = 143,
    DclFunctionBody                      = 144,
    DclFunctionTable                     = 145,
    DclInterface                         = 146,
    DclInputControlPointCount            = 147,
    DclOutputControlPointCount           = 148,
    DclTessDomain                        = 149,
    DclTessPartitioning                  = 150,
    DclTessOutputPrimitive               = 151,
    DclHsMaxTessFactor                   = 152,
    DclHsForkPhaseInstanceCount          = 153,
    DclHsJoinPhaseInstanceCount          = 154,
    DclThreadGroup                       = 155,
    DclUavTyped                          = 156,
    DclUavRaw                            = 157,
    DclUavStructured                     = 158,
    DclThreadGroupSharedMemoryRaw        = 159,
    DclThreadGroupSharedMemoryStructured = 160,
    DclResourceRaw                       = 161,
    DclResourceStructured                = 162,
    LdUavTyped                           = 163,
    StoreUavTyped                        = 164,
    LdRaw                                = 165,
    StoreRaw                             = 166,
    LdStructured                         = 167,
    StoreStructured                      = 168,
    AtomicAnd                            = 169,
    AtomicOr                             = 170,
    AtomicXor                            = 171,
    AtomicCmpStore                       = 172,
    AtomicIAdd                           = 173,
    AtomicIMax                           = 174,
    AtomicIMin                           = 175,
    AtomicUMax                           = 176,
    AtomicUMin                           = 177,
    ImmAtomicAlloc                       = 178,
    ImmAtomicConsume                     = 179,
    ImmAtomicIAdd                        = 180,
    ImmAtomicAnd                         = 181,
    ImmAtomicOr                          = 182,
    ImmAtomicXor                         = 183,
    ImmAtomicExch                        = 184,
    ImmAtomicCmpExch                     = 185,
    ImmAtomicIMax                        = 186,
    ImmAtomicIMin                        = 187,
    ImmAtomicUMax                        = 188,
    ImmAtomicUMin                        = 189,
    Sync                                 = 190,
    DAdd                                 = 191,
    DMax                                 = 192,
    DMin                                 = 193,
    DMul                                 = 194,
    DEq                                  = 195,
    DGe                                  = 196,
    DLt                                  = 197,
    DNe                                  = 198,
    DMov                                 = 199,
    DMovc                                = 200,
    DtoF                                 = 201,
    FtoD                                 = 202,
    EvalSnapped                          = 203,
    EvalSampleIndex                      = 204,
    EvalCentroid                         = 205,
    DclGsInstanceCount                   = 206,
    Abort                                = 207,
    DebugBreak                           = 208,
    ReservedBegin11_1                    = 209,
    DDiv                                 = 210,
    DFma                                 = 211,
    DRcp                                 = 212,
    Msad                                 = 213,
    DtoI                                 = 214,
    DtoU                                 = 215,
    ItoD                                 = 216,
    UtoD                                 = 217,
    ReservedBegin11_2                    = 218,
    Gather4S                             = 219,
    Gather4CS                            = 220,
    Gather4PoS                           = 221,
    Gather4PoCS                          = 222,
    LdS                                  = 223,
    LdMsS                                = 224,
    LdUavTypedS                          = 225,
    LdRawS                               = 226,
    LdStructuredS                        = 227,
    SampleLS                             = 228,
    SampleClzS                           = 229,
    SampleClampS                         = 230,
    SampleBClampS                        = 231,
    SampleDClampS                        = 232,
    SampleCClampS                        = 233,
    CheckAccessFullyMapped               = 234,
  };
  
  
  /**
   * \brief Extended opcode
   */
  enum class DxbcExtOpcode : uint32_t {
    Empty                                = 0,
    SampleControls                       = 1,
    ResourceDim                          = 2,
    ResourceReturnType                   = 3,
  };
  
  
  /**
   * \brief Operand type
   * 
   * Selects the 'register file' from which
   * to retrieve an operand's value.
   */
  enum class DxbcOperandType : uint32_t {
    Temp                    = 0,
    Input                   = 1,
    Output                  = 2,
    IndexableTemp           = 3,
    Imm32                   = 4,
    Imm64                   = 5,
    Sampler                 = 6,
    Resource                = 7,
    ConstantBuffer          = 8,
    ImmediateConstantBuffer = 9,
    Label                   = 10,
    InputPrimitiveId        = 11,
    OutputDepth             = 12,
    Null                    = 13,
    Rasterizer              = 14,
    OutputCoverageMask      = 15,
    Stream                  = 16,
    FunctionBody            = 17,
    FunctionTable           = 18,
    Interface               = 19,
    FunctionInput           = 20,
    FunctionOutput          = 21,
    OutputControlPointId    = 22,
    InputForkInstanceId     = 23,
    InputJoinInstanceId     = 24,
    InputControlPoint       = 25,
    OutputControlPoint      = 26,
    InputPatchConstant      = 27,
    InputDomainPoint        = 28,
    ThisPointer             = 29,
    UnorderedAccessView     = 30,
    ThreadGroupSharedMemory = 31,
    InputThreadId           = 32,
    InputThreadGroupId      = 33,
    InputThreadIdInGroup    = 34,
    InputCoverageMask       = 35,
    InputThreadIndexInGroup = 36,
    InputGsInstanceId       = 37,
    OutputDepthGe           = 38,
    OutputDepthLe           = 39,
    CycleCounter            = 40,
    OutputStencilRef        = 41,
    InputInnerCoverage      = 42,
  };
  
  
  /**
   * \brief Number of components
   * 
   * Used by operands to determine whether the
   * operand has one, four or zero components.
   */
  enum class DxbcComponentCount : uint32_t {
    Component0 = 0,
    Component1 = 1,
    Component4 = 2,
  };
  
  
  /**
   * \brief Component selection mode
   * 
   * When an operand has four components, the
   * component selection mode deterines which
   * components are used for the operation.
   */
  enum class DxbcRegMode : uint32_t {
    Mask    = 0,
    Swizzle = 1,
    Select1 = 2,
  };
  
  
  /**
   * \brief Index representation
   * 
   * Determines how an operand
   * register index is stored.
   */
  enum class DxbcOperandIndexRepresentation : uint32_t {
    Imm32             = 0,
    Imm64             = 1,
    Relative          = 2,
    Imm32Relative     = 3,
    Imm64Relative     = 4,
  };
  
  
  /**
   * \brief Extended operand type
   */
  enum class DxbcOperandExt : uint32_t {
    OperandModifier   = 1,
  };
  
  
  /**
   * \brief Resource dimension
   * The type of a resource.
   */
  enum class DxbcResourceDim : uint32_t {
    Unknown           = 0,
    Buffer            = 1,
    Texture1D         = 2,
    Texture2D         = 3,
    Texture2DMs       = 4,
    Texture3D         = 5,
    TextureCube       = 6,
    Texture1DArr      = 7,
    Texture2DArr      = 8,
    Texture2DMsArr    = 9,
    TextureCubeArr    = 10,
    RawBuffer         = 11,
    StructuredBuffer  = 12,
  };
  
  
  /**
   * \brief Resource return type
   * Data type for resource read ops.
   */
  enum class DxbcResourceReturnType : uint32_t {
    Unorm             = 1,
    Snorm             = 2,
    Sint              = 3,
    Uint              = 4,
    Float             = 5,
    Mixed             = 6,  /// ?
    Double            = 7,
    Continued         = 8,  /// ?
    Unused            = 9,  /// ?
  };
  
  
  /**
   * \brief Register component type
   * Data type of a register component.
   */
  enum class DxbcRegisterComponentType : uint32_t {
    Unknown           = 0,
    Uint32            = 1,
    Sint32            = 2,
    Float32           = 3,
  };
  
  
  /**
   * \brief Instruction return type
   */
  enum class DxbcInstructionReturnType : uint32_t {
    Float             = 0,
    Uint              = 1,
  };
  
  
  enum class DxbcSystemValue : uint32_t {
    None                          = 0,
    Position                      = 1,
    ClipDistance                  = 2,
    CullDistance                  = 3,
    RenderTargetId                = 4,
    ViewportId                    = 5,
    VertexId                      = 6,
    PrimitiveId                   = 7,
    InstanceId                    = 8,
    IsFrontFace                   = 9,
    SampleIndex                   = 10,
    FinalQuadUeq0EdgeTessFactor   = 11,
    FinalQuadVeq0EdgeTessFactor   = 12,
    FinalQuadUeq1EdgeTessFactor   = 13,
    FinalQuadVeq1EdgeTessFactor   = 14,
    FinalQuadUInsideTessFactor    = 15,
    FinalQuadVInsideTessFactor    = 16,
    FinalTriUeq0EdgeTessFactor    = 17,
    FinalTriVeq0EdgeTessFactor    = 18,
    FinalTriWeq0EdgeTessFactor    = 19,
    FinalTriInsideTessFactor      = 20,
    FinalLineDetailTessFactor     = 21,
    FinalLineDensityTessFactor    = 22,
    Target                        = 64,
    Depth                         = 65,
    Coverage                      = 66,
    DepthGe                       = 67,
    DepthLe                       = 68
  };
  
  
  enum class DxbcInterpolationMode : uint32_t {
    Undefined                   = 0,
    Constant                    = 1,
    Linear                      = 2,
    LinearCentroid              = 3,
    LinearNoPerspective         = 4,
    LinearNoPerspectiveCentroid = 5,
    LinearSample                = 6,
    LinearNoPerspectiveSample   = 7,
  };
  
  
  enum class DxbcGlobalFlag : uint32_t {
    RefactoringAllowed    = 0,
    DoublePrecision       = 1,
    EarlyFragmentTests    = 2,
    RawStructuredBuffers  = 3,
  };
  
  using DxbcGlobalFlags = Flags<DxbcGlobalFlag>;
  
  enum class DxbcZeroTest : uint32_t {
    TestZ   = 0,
    TestNz  = 1,
  };
  
  enum class DxbcResinfoType : uint32_t {
    Float     = 0,
    RcpFloat  = 1,
    Uint      = 2,
  };
  
  enum class DxbcSyncFlag : uint32_t {
    ThreadsInGroup                = 0,
    ThreadGroupSharedMemory       = 1,
    UavMemoryGroup                = 2,
    UavMemoryGlobal               = 3,
  };
  
  using DxbcSyncFlags = Flags<DxbcSyncFlag>;
  
  
  /**
   * \brief Geometry shader input primitive
   */
  enum class DxbcPrimitive : uint32_t {
    Undefined         =  0,
    Point             =  1,
    Line              =  2,
    Triangle          =  3,
    LineAdj           =  6,
    TriangleAdj       =  7,
    Patch1            =  8,
    Patch2            =  9,
    Patch3            = 10,
    Patch4            = 11,
    Patch5            = 12,
    Patch6            = 13,
    Patch7            = 14,
    Patch8            = 15,
    Patch9            = 16,
    Patch10           = 17,
    Patch11           = 18,
    Patch12           = 19,
    Patch13           = 20,
    Patch14           = 21,
    Patch15           = 22,
    Patch16           = 23,
    Patch17           = 24,
    Patch18           = 25,
    Patch19           = 26,
    Patch20           = 27,
    Patch21           = 28,
    Patch22           = 29,
    Patch23           = 30,
    Patch24           = 31,
    Patch25           = 32,
    Patch26           = 33,
    Patch27           = 34,
    Patch28           = 35,
    Patch29           = 36,
    Patch30           = 37,
    Patch31           = 38,
    Patch32           = 39,
  };
  
  
  /**
   * \brief Geometry shader output topology
   */
  enum class DxbcPrimitiveTopology : uint32_t {
    Undefined         = 0,
    PointList         = 1,
    LineList          = 2,
    LineStrip         = 3,
    TriangleList      = 4,
    TriangleStrip     = 5,
    LineListAdj       = 10,
    LineStripAdj      = 11,
    TriangleListAdj   = 12,
    TriangleStripAdj  = 13,
  };
  
  
  /**
   * \brief Sampler operation mode
   */
  enum class DxbcSamplerMode : uint32_t {
    Default           = 0,
    Comparison        = 1,
    Mono              = 2,
  };
  
  
  /**
   * \brief Scalar value type
   * 
   * Enumerates possible register component
   * types. Scalar types are represented as
   * a one-component vector type.
   */
  enum class DxbcScalarType : uint32_t {
    Uint32    = 0,
    Uint64    = 1,
    Sint32    = 2,
    Sint64    = 3,
    Float32   = 4,
    Float64   = 5,
    Bool      = 6,
  };
  
  
  /**
   * \brief Tessellator domain
   */
  enum class DxbcTessDomain : uint32_t {
    Undefined     = 0,
    Isolines      = 1,
    Triangles     = 2,
    Quads         = 3,
  };
  
  /**
   * \brief Tessellator partitioning
   */
  enum class DxbcTessPartitioning : uint32_t {
    Undefined     = 0,
    Integer       = 1,
    Pow2          = 2,
    FractOdd      = 3,
    FractEven     = 4,
  };
  
  /**
   * \brief UAV definition flags
   */
  enum class DxbcUavFlag : uint32_t {
    GloballyCoherent = 0,
    RasterizerOrdered = 1,
  };
  
  using DxbcUavFlags = Flags<DxbcUavFlag>;
  
  /**
   * \brief Tessellator output primitive
   */
  enum class DxbcTessOutputPrimitive : uint32_t {
    Undefined     = 0,
    Point         = 1,
    Line          = 2,
    TriangleCw    = 3,
    TriangleCcw   = 4,
  };
  
  /**
   * \brief Custom data class
   * 
   * Stores which type of custom data is
   * referenced by the instruction.
   */
  enum class DxbcCustomDataClass : uint32_t {
    Comment       = 0,
    DebugInfo     = 1,
    Opaque        = 2,
    ImmConstBuf   = 3,
  };
  
  
  enum class DxbcResourceType : uint32_t {
    Typed      = 0,
    Raw        = 1,
    Structured = 2,
  };


  enum class DxbcConstantBufferAccessType : uint32_t {
    StaticallyIndexed = 0,
    DynamicallyIndexed = 1,
  };
  
}