#pragma once

#include <array>
#include <utility>
#include <vector>

#include "../spirv/spirv_module.h"

#include "dxbc_analysis.h"
#include "dxbc_chunk_isgn.h"
#include "dxbc_decoder.h"
#include "dxbc_defs.h"
#include "dxbc_modinfo.h"
#include "dxbc_names.h"
#include "dxbc_util.h"

namespace dxvk {
  
  /**
   * \brief Vector type
   * 
   * Convenience struct that stores a scalar
   * type and a component count. The compiler
   * can use this to generate SPIR-V types.
   */
  struct DxbcVectorType {
    DxbcScalarType    ctype;
    uint32_t          ccount;
  };
  
  
  /**
   * \brief Array type
   * 
   * Convenience struct that stores a scalar type, a
   * component count and an array size. An array of
   * length 0 will be evaluated to a vector type. The
   * compiler can use this to generate SPIR-V types.
   */
  struct DxbcArrayType {
    DxbcScalarType    ctype;
    uint32_t          ccount;
    uint32_t          alength;
  };
  
  
  /**
   * \brief Register info
   * 
   * Stores the array type of a register and
   * its storage class. The compiler can use
   * this to generate SPIR-V pointer types.
   */
  struct DxbcRegisterInfo {
    DxbcArrayType     type;
    spv::StorageClass sclass;
  };
  
  
  /**
   * \brief Register value
   * 
   * Stores a vector type and a SPIR-V ID that
   * represents an intermediate value. This is
   * used to track the type of such values.
   */
  struct DxbcRegisterValue {
    DxbcVectorType    type;
    uint32_t          id;
  };
  
  
  /**
   * \brief Register pointer
   * 
   * Stores a vector type and a SPIR-V ID that
   * represents a pointer to such a vector. This
   * can be used to load registers conveniently.
   */
  struct DxbcRegisterPointer {
    DxbcVectorType    type;
    uint32_t          id;
  };
  
  
  struct DxbcXreg {
    uint32_t ccount  = 0;
    uint32_t alength = 0;
    uint32_t varId   = 0;
  };
  
  
  struct DxbcGreg {
    DxbcResourceType type  = DxbcResourceType::Raw;
    uint32_t elementStride = 0;
    uint32_t elementCount  = 0;
    uint32_t varId         = 0;
  };
  
  
  /**
   * \brief Specialization constant properties
   * 
   * Stores the name, data type and initial
   * value of a specialization constant.
   */
  struct DxbcSpecConstant {
    DxbcScalarType  ctype;
    uint32_t        ccount;
    uint32_t        value;
    const char*     name;
  };
  
  
  /**
   * \brief Helper struct for conditional execution
   * 
   * Stores a set of labels required to implement either
   * an if-then block or an if-then-else block. This is
   * not used to implement control flow instructions.
   */
  struct DxbcConditional {
    uint32_t labelIf   = 0;
    uint32_t labelElse = 0;
    uint32_t labelEnd  = 0;
  };


  struct DxbcXfbVar {
    uint32_t    varId     = 0;
    uint32_t    streamId  = 0;
    uint32_t    outputId  = 0;
    DxbcRegMask srcMask   = 0;
    DxbcRegMask dstMask   = 0;
    uint32_t    location  = 0;
    uint32_t    component = 0;
  };
  

  struct DxbcIndexRange {
    DxbcOperandType type;
    uint32_t start;
    uint32_t length;
  };

  
  /**
   * \brief Vertex shader-specific structure
   */
  struct DxbcCompilerVsPart {
    uint32_t functionId = 0;
    
    uint32_t builtinVertexId     = 0;
    uint32_t builtinInstanceId   = 0;
    uint32_t builtinBaseVertex   = 0;
    uint32_t builtinBaseInstance = 0;
  };
  
  
  /**
   * \brief Geometry shader-specific structure
   */
  struct DxbcCompilerGsPart {
    DxbcPrimitive         inputPrimitive      = DxbcPrimitive::Undefined;
    DxbcPrimitiveTopology outputTopology      = DxbcPrimitiveTopology::Undefined;
    uint32_t              outputVertexCount   = 0;
    uint32_t              functionId          = 0;
    
    uint32_t builtinLayer         = 0;
    uint32_t builtinViewportId    = 0;
    uint32_t builtinInvocationId  = 0;
    uint32_t invocationCount      = 0;

    bool needsOutputSetup = false;
  };


  /**
   * \brief Pixel shader-specific structure
   */
  struct DxbcCompilerPsPart {
    uint32_t functionId = 0;
    
    uint32_t builtinFragCoord     = 0;
    uint32_t builtinDepth         = 0;
    uint32_t builtinStencilRef    = 0;
    uint32_t builtinIsFrontFace   = 0;
    uint32_t builtinSampleId      = 0;
    uint32_t builtinSampleMaskIn  = 0;
    uint32_t builtinSampleMaskOut = 0;
    uint32_t builtinLayer         = 0;
    uint32_t builtinViewportId    = 0;
    uint32_t builtinInnerCoverageId = 0;
    
    uint32_t pushConstantId       = 0;
  };
  
  
  /**
   * \brief Compute shader-specific structure
   */
  struct DxbcCompilerCsPart {
    uint32_t functionId = 0;

    uint32_t workgroupSizeX               = 0;
    uint32_t workgroupSizeY               = 0;
    uint32_t workgroupSizeZ               = 0;
    
    uint32_t builtinGlobalInvocationId    = 0;
    uint32_t builtinLocalInvocationId     = 0;
    uint32_t builtinLocalInvocationIndex  = 0;
    uint32_t builtinWorkgroupId           = 0;
  };
  
  
  /**
   * \brief Hull shader fork/join phase
   * 
   * Defines a function and built-in variables
   * for a single fork or join phase sub-program.
   */
  struct DxbcCompilerHsForkJoinPhase {
    uint32_t functionId         = 0;
    uint32_t instanceCount      = 1;
    
    uint32_t instanceId         = 0;
    uint32_t instanceIdPtr      = 0;
  };
  
  
  /**
   * \brief Hull shader control point phase
   * 
   * Defines the function for the control
   * point phase program of a hull shader.
   */
  struct DxbcCompilerHsControlPointPhase {
    uint32_t functionId             = 0;
  };
  
  
  /**
   * \brief Hull shader phase
   * 
   * Used to identify the current
   * phase and function ID.
   */
  enum class DxbcCompilerHsPhase : uint32_t {
    None,         ///< No active phase
    Decl,         ///< \c hs_decls
    ControlPoint, ///< \c hs_control_point_phase
    Fork,         ///< \c hs_fork_phase
    Join,         ///< \c hs_join_phase
  };
  
  
  /**
   * \brief Hull shader-specific structure
   */
  struct DxbcCompilerHsPart {
    DxbcCompilerHsPhase currPhaseType   = DxbcCompilerHsPhase::None;
    size_t              currPhaseId     = 0;
    
    float maxTessFactor = 64.0f;
    
    uint32_t vertexCountIn   = 0;
    uint32_t vertexCountOut  = 0;
    
    uint32_t builtinInvocationId   = 0;
    uint32_t builtinTessLevelOuter = 0;
    uint32_t builtinTessLevelInner = 0;
    
    uint32_t outputPerPatch        = 0;
    uint32_t outputPerVertex       = 0;
    
    uint32_t invocationBlockBegin  = 0;
    uint32_t invocationBlockEnd    = 0;

    uint32_t outputPerPatchMask    = 0;
    
    DxbcCompilerHsControlPointPhase          cpPhase;
    std::vector<DxbcCompilerHsForkJoinPhase> forkPhases;
    std::vector<DxbcCompilerHsForkJoinPhase> joinPhases;
  };
  
  
  /**
   * \brief Domain shader-specific structure
   */
  struct DxbcCompilerDsPart {
    uint32_t functionId            = 0;
    
    uint32_t builtinTessCoord      = 0;
    uint32_t builtinTessLevelOuter = 0;
    uint32_t builtinTessLevelInner = 0;
    
    uint32_t vertexCountIn         = 0;
    
    uint32_t inputPerPatch         = 0;
    uint32_t inputPerVertex        = 0;
  };
  
  
  enum class DxbcCfgBlockType : uint32_t {
    If, Loop, Switch,
  };
  
  
  struct DxbcCfgBlockIf {
    uint32_t ztestId;
    uint32_t labelIf;
    uint32_t labelElse;
    uint32_t labelEnd;
    size_t   headerPtr;
  };
  
  
  struct DxbcCfgBlockLoop {
    uint32_t labelHeader;
    uint32_t labelBegin;
    uint32_t labelContinue;
    uint32_t labelBreak;
  };
  
  
  struct DxbcSwitchLabel {
    SpirvSwitchCaseLabel desc;
    DxbcSwitchLabel*     next;
  };
  
  
  struct DxbcCfgBlockSwitch {
    size_t            insertPtr;
    uint32_t          selectorId;
    uint32_t          labelBreak;
    uint32_t          labelCase;
    uint32_t          labelDefault;
    DxbcSwitchLabel*  labelCases;
  };
  
  
  struct DxbcCfgBlock {
    DxbcCfgBlockType type;
    
    union {
      DxbcCfgBlockIf     b_if;
      DxbcCfgBlockLoop   b_loop;
      DxbcCfgBlockSwitch b_switch;
    };
  };
  
  
  struct DxbcBufferInfo {
    DxbcImageInfo image;
    DxbcScalarType stype;
    DxbcResourceType type;
    uint32_t typeId;
    uint32_t varId;
    uint32_t stride;
    uint32_t coherence;
    bool isSsbo;
  };
  

  /**
   * \brief DXBC to SPIR-V shader compiler
   * 
   * Processes instructions from a DXBC shader and creates
   * a DXVK shader object, which contains the SPIR-V module
   * and information about the shader resource bindings.
   */
  class DxbcCompiler {
    
  public:
    
    DxbcCompiler(
      const std::string&        fileName,
      const DxbcModuleInfo&     moduleInfo,
      const DxbcProgramInfo&    programInfo,
      const Rc<DxbcIsgn>&       isgn,
      const Rc<DxbcIsgn>&       osgn,
      const Rc<DxbcIsgn>&       psgn,
      const DxbcAnalysisInfo&   analysis);
    ~DxbcCompiler();
    
    /**
     * \brief Processes a single instruction
     * \param [in] ins The instruction
     */
    void processInstruction(
      const DxbcShaderInstruction&  ins);
    
    /**
     * \brief Emits transform feedback passthrough
     * 
     * Writes all captured input variables to the
     * corresponding xfb outputs, and sets up the
     * geometry shader for point-to-point mode.
     */
    void processXfbPassthrough();
    
    /**
     * \brief Finalizes the shader
     * \returns The final shader object
     */
    SpirvCodeBuffer finalize();

    /**
     * \brief Extracts immediate constant buffer data
     *
     * Only defined if the ICB needs to be backed by a
     * uniform buffer.
     * \returns Immediate constant buffer data
     */
    std::vector<uint32_t> getIcbData() const {
      return std::move(m_icbData);
    }
    
  private:
    
    DxbcModuleInfo      m_moduleInfo;
    DxbcProgramInfo     m_programInfo;
    SpirvModule         m_module;
    
    Rc<DxbcIsgn>        m_isgn;
    Rc<DxbcIsgn>        m_osgn;
    Rc<DxbcIsgn>        m_psgn;
    
    const DxbcAnalysisInfo* m_analysis;
    
    ///////////////////////////////////////////////////////
    // Resource slot description for the shader. This will
    // be used to map D3D11 bindings to DXVK bindings.
    std::vector<DxvkBindingInfo> m_bindings;
    
    ////////////////////////////////////////////////
    // Temporary r# vector registers with immediate
    // indexing, and x# vector array registers.
    std::vector<uint32_t> m_rRegs;
    std::vector<DxbcXreg> m_xRegs;
    
    /////////////////////////////////////////////
    // Thread group shared memory (g#) registers
    std::vector<DxbcGreg> m_gRegs;
    
    ///////////////////////////////////////////////////////////
    // v# registers as defined by the shader. The type of each
    // of these inputs is either float4 or an array of float4.
    std::array<
      DxbcRegisterPointer,
      DxbcMaxInterfaceRegs>     m_vRegs;
    std::vector<DxbcSvMapping>  m_vMappings;
    
    //////////////////////////////////////////////////////////
    // o# registers as defined by the shader. In the fragment
    // shader stage, these registers are typed by the signature,
    // in all other stages, they are float4 registers or arrays.
    std::array<
      DxbcRegisterPointer,
      DxbcMaxInterfaceRegs>     m_oRegs;
    std::vector<DxbcSvMapping>  m_oMappings;

    /////////////////////////////////////////////
    // xfb output registers for geometry shaders
    std::vector<DxbcXfbVar> m_xfbVars;
    
    /////////////////////////////////////////////
    // Dynamically indexed input and output regs
    std::vector<DxbcIndexRange> m_indexRanges = { };

    //////////////////////////////////////////////////////
    // Shader resource variables. These provide access to
    // constant buffers, samplers, textures, and UAVs.
    std::array<DxbcConstantBuffer,  16> m_constantBuffers;
    std::array<DxbcSampler,         16> m_samplers;
    std::array<DxbcShaderResource, 128> m_textures;
    std::array<DxbcUav,             64> m_uavs;

    bool m_hasGloballyCoherentUav = false;
    bool m_hasRasterizerOrderedUav = false;

    ///////////////////////////////////////////////
    // Control flow information. Stores labels for
    // currently active if-else blocks and loops.
    std::vector<DxbcCfgBlock> m_controlFlowBlocks;

    bool m_topLevelIsUniform = true;

    uint64_t m_uavRdMask = 0u;
    uint64_t m_uavWrMask = 0u;

    //////////////////////////////////////////////
    // Function state tracking. Required in order
    // to properly end functions in some cases.
    bool m_insideFunction = false;

    ///////////////////////////////////////////////////////////
    // Array of input values. Since v# registers are indexable
    // in DXBC, we need to copy them into an array first.
    uint32_t m_vArrayLength   = 0;
    uint32_t m_vArrayLengthId = 0;

    uint32_t m_vArray = 0;

    ////////////////////////////////////////////////////
    // Per-vertex input and output blocks. Depending on
    // the shader stage, these may be declared as arrays.
    uint32_t m_positionIn     = 0;
    uint32_t m_positionOut    = 0;

    uint32_t m_clipDistances  = 0;
    uint32_t m_cullDistances  = 0;
    
    uint32_t m_primitiveIdIn  = 0;
    uint32_t m_primitiveIdOut = 0;

    //////////////////////////////////////////////////
    // Immediate constant buffer. If defined, this is
    // an array of four-component uint32 vectors.
    uint32_t          m_icbArray = 0;
    std::vector<uint32_t> m_icbData;

    uint32_t          m_icbComponents = 0u;
    uint32_t          m_icbSize = 0u;
    
    ///////////////////////////////////////////////////
    // Sample pos array. If defined, this iis an array
    // of 32 four-component float vectors.
    uint32_t m_samplePositions = 0;
    
    ////////////////////////////////////////////
    // Struct type used for UAV counter buffers
    uint32_t m_uavCtrStructType  = 0;
    uint32_t m_uavCtrPointerType = 0;
    
    ////////////////////////////////
    // Function IDs for subroutines
    std::unordered_map<uint32_t, uint32_t> m_subroutines;
    
    ///////////////////////////////////////////////////
    // Entry point description - we'll need to declare
    // the function ID and all input/output variables.
    uint32_t              m_entryPointId = 0;
    bool                  m_hasRawAccessChains = false;
    
    ////////////////////////////////////////////
    // Inter-stage shader interface slots. Also
    // covers vertex input and fragment output.
    uint32_t m_inputMask = 0u;
    uint32_t m_outputMask = 0u;

    ///////////////////////////////////
    // Shader-specific data structures
    DxbcCompilerVsPart m_vs;
    DxbcCompilerHsPart m_hs;
    DxbcCompilerDsPart m_ds;
    DxbcCompilerGsPart m_gs;
    DxbcCompilerPsPart m_ps;
    DxbcCompilerCsPart m_cs;

    //////////////////////
    // Global state stuff
    bool m_precise = true;

    DxbcOpcode m_lastOp = DxbcOpcode::Nop;
    DxbcOpcode m_currOp = DxbcOpcode::Nop;

    VkPrimitiveTopology m_inputTopology = VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
    VkPrimitiveTopology m_outputTopology = VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;

    /////////////////////////////////////////////////////
    // Shader interface and metadata declaration methods
    void emitDcl(
      const DxbcShaderInstruction&  ins);
    
    void emitDclGlobalFlags(
      const DxbcShaderInstruction&  ins);
    
    void emitDclIndexRange(
      const DxbcShaderInstruction&  ins);

    void emitDclTemps(
      const DxbcShaderInstruction&  ins);
    
    void emitDclIndexableTemp(
      const DxbcShaderInstruction&  ins);
    
    void emitDclInterfaceReg(
      const DxbcShaderInstruction&  ins);
    
    void emitDclInput(
            uint32_t                regIdx,
            uint32_t                regDim,
            DxbcRegMask             regMask,
            DxbcSystemValue         sv,
            DxbcInterpolationMode   im);
    
    void emitDclOutput(
            uint32_t                regIdx,
            uint32_t                regDim,
            DxbcRegMask             regMask,
            DxbcSystemValue         sv,
            DxbcInterpolationMode   im);
    
    void emitDclConstantBuffer(
      const DxbcShaderInstruction&  ins);
    
    void emitDclConstantBufferVar(
            uint32_t                regIdx,
            uint32_t                numConstants,
            uint32_t                numComponents,
      const char*                   name);
    
    void emitDclSampler(
      const DxbcShaderInstruction&  ins);
    
    void emitDclStream(
      const DxbcShaderInstruction&  ins);
    
    void emitDclResourceTyped(
      const DxbcShaderInstruction&  ins);
    
    void emitDclResourceRawStructured(
      const DxbcShaderInstruction&  ins);
    
    void emitDclThreadGroupSharedMemory(
      const DxbcShaderInstruction&  ins);
    
    void emitDclGsInputPrimitive(
      const DxbcShaderInstruction&  ins);
    
    void emitDclGsOutputTopology(
      const DxbcShaderInstruction&  ins);
    
    void emitDclMaxOutputVertexCount(
      const DxbcShaderInstruction&  ins);
    
    void emitDclInputControlPointCount(
      const DxbcShaderInstruction&  ins);
    
    void emitDclOutputControlPointCount(
      const DxbcShaderInstruction&  ins);
    
    void emitDclHsMaxTessFactor(
      const DxbcShaderInstruction&  ins);
    
    void emitDclTessDomain(
      const DxbcShaderInstruction&  ins);
    
    void emitDclTessPartitioning(
      const DxbcShaderInstruction&  ins);
    
    void emitDclTessOutputPrimitive(
      const DxbcShaderInstruction&  ins);
    
    void emitDclThreadGroup(
      const DxbcShaderInstruction&  ins);
    
    void emitDclGsInstanceCount(
      const DxbcShaderInstruction&  ins);
    
    uint32_t emitDclUavCounter(
            uint32_t                regId);
    
    ////////////////////////
    // Custom data handlers
    void emitDclImmediateConstantBuffer(
      const DxbcShaderInstruction&  ins);
    
    void emitDclImmediateConstantBufferBaked(
            uint32_t                dwordCount,
      const uint32_t*               dwordArray,
            uint32_t                componentCount);
    
    void emitDclImmediateConstantBufferUbo(
            uint32_t                dwordCount,
      const uint32_t*               dwordArray,
            uint32_t                componentCount);
    
    void emitCustomData(
      const DxbcShaderInstruction&  ins);
    
    //////////////////////////////
    // Instruction class handlers
    void emitVectorAlu(
      const DxbcShaderInstruction&  ins);
    
    void emitVectorCmov(
      const DxbcShaderInstruction&  ins);
    
    void emitVectorCmp(
      const DxbcShaderInstruction&  ins);
    
    void emitVectorDeriv(
      const DxbcShaderInstruction&  ins);
    
    void emitVectorDot(
      const DxbcShaderInstruction&  ins);
    
    void emitVectorIdiv(
      const DxbcShaderInstruction&  ins);
    
    void emitVectorImul(
      const DxbcShaderInstruction&  ins);
    
    void emitVectorMsad(
      const DxbcShaderInstruction&  ins);
    
    void emitVectorShift(
      const DxbcShaderInstruction&  ins);
    
    void emitVectorSinCos(
      const DxbcShaderInstruction&  ins);
    
    void emitGeometryEmit(
      const DxbcShaderInstruction&  ins);
    
    void emitAtomic(
      const DxbcShaderInstruction&  ins);
    
    void emitAtomicCounter(
      const DxbcShaderInstruction&  ins);
    
    void emitBarrier(
      const DxbcShaderInstruction&  ins);
    
    void emitBitExtract(
      const DxbcShaderInstruction&  ins);
    
    void emitBitInsert(
      const DxbcShaderInstruction&  ins);
    
    void emitBitScan(
      const DxbcShaderInstruction&  ins);
    
    void emitBufferQuery(
      const DxbcShaderInstruction&  ins);
    
    void emitBufferLoad(
      const DxbcShaderInstruction&  ins);
    
    void emitBufferStore(
      const DxbcShaderInstruction&  ins);
    
    void emitConvertFloat16(
      const DxbcShaderInstruction&  ins);
    
    void emitConvertFloat64(
      const DxbcShaderInstruction&  ins);
    
    void emitHullShaderPhase(
      const DxbcShaderInstruction&  ins);
    
    void emitHullShaderInstCnt(
      const DxbcShaderInstruction&  ins);
    
    void emitInterpolate(
      const DxbcShaderInstruction&  ins);
    
    void emitSparseCheckAccess(
      const DxbcShaderInstruction&  ins);
    
    void emitTextureQuery(
      const DxbcShaderInstruction&  ins);
    
    void emitTextureQueryLod(
      const DxbcShaderInstruction&  ins);
    
    void emitTextureQueryMs(
      const DxbcShaderInstruction&  ins);
    
    void emitTextureQueryMsPos(
      const DxbcShaderInstruction&  ins);
    
    void emitTextureFetch(
      const DxbcShaderInstruction&  ins);
    
    void emitTextureGather(
      const DxbcShaderInstruction&  ins);
    
    void emitTextureSample(
      const DxbcShaderInstruction&  ins);
    
    void emitTypedUavLoad(
      const DxbcShaderInstruction&  ins);
    
    void emitTypedUavStore(
      const DxbcShaderInstruction&  ins);
    
    /////////////////////////////////////
    // Control flow instruction handlers
    void emitControlFlowIf(
      const DxbcShaderInstruction&  ins);
    
    void emitControlFlowElse(
      const DxbcShaderInstruction&  ins);
    
    void emitControlFlowEndIf(
      const DxbcShaderInstruction&  ins);
    
    void emitControlFlowSwitch(
      const DxbcShaderInstruction&  ins);
    
    void emitControlFlowCase(
      const DxbcShaderInstruction&  ins);
    
    void emitControlFlowDefault(
      const DxbcShaderInstruction&  ins);
    
    void emitControlFlowEndSwitch(
      const DxbcShaderInstruction&  ins);
    
    void emitControlFlowLoop(
      const DxbcShaderInstruction&  ins);
    
    void emitControlFlowEndLoop(
      const DxbcShaderInstruction&  ins);
    
    void emitControlFlowBreak(
      const DxbcShaderInstruction&  ins);
    
    void emitControlFlowBreakc(
      const DxbcShaderInstruction&  ins);
    
    void emitControlFlowRet(
      const DxbcShaderInstruction&  ins);

    void emitControlFlowRetc(
      const DxbcShaderInstruction&  ins);
    
    void emitControlFlowDiscard(
      const DxbcShaderInstruction&  ins);
    
    void emitControlFlowLabel(
      const DxbcShaderInstruction&  ins);

    void emitControlFlowCall(
      const DxbcShaderInstruction&  ins);
    
    void emitControlFlowCallc(
      const DxbcShaderInstruction&  ins);
    
    void emitControlFlow(
      const DxbcShaderInstruction&  ins);
    
    ////////////////////////////////////////////////
    // Constant building methods. These are used to
    // generate constant vectors that store the same
    // value in each component.
    DxbcRegisterValue emitBuildConstVecf32(
            float                   x,
            float                   y,
            float                   z,
            float                   w,
      const DxbcRegMask&            writeMask);
    
    DxbcRegisterValue emitBuildConstVecu32(
            uint32_t                x,
            uint32_t                y,
            uint32_t                z,
            uint32_t                w,
      const DxbcRegMask&            writeMask);
    
    DxbcRegisterValue emitBuildConstVeci32(
            int32_t                 x,
            int32_t                 y,
            int32_t                 z,
            int32_t                 w,
      const DxbcRegMask&            writeMask);
    
    DxbcRegisterValue emitBuildConstVecf64(
            double                  xy,
            double                  zw,
      const DxbcRegMask&            writeMask);

    DxbcRegisterValue emitBuildVector(
            DxbcRegisterValue       scalar,
            uint32_t                count);
    
    DxbcRegisterValue emitBuildZeroVector(
            DxbcVectorType          type);

    /////////////////////////////////////////
    // Generic register manipulation methods
    DxbcRegisterValue emitRegisterBitcast(
            DxbcRegisterValue       srcValue,
            DxbcScalarType          dstType);
    
    DxbcRegisterValue emitRegisterSwizzle(
            DxbcRegisterValue       value,
            DxbcRegSwizzle          swizzle,
            DxbcRegMask             writeMask);
    
    DxbcRegisterValue emitRegisterExtract(
            DxbcRegisterValue       value,
            DxbcRegMask             mask);
    
    DxbcRegisterValue emitRegisterInsert(
            DxbcRegisterValue       dstValue,
            DxbcRegisterValue       srcValue,
            DxbcRegMask             srcMask);
    
    DxbcRegisterValue emitRegisterConcat(
            DxbcRegisterValue       value1,
            DxbcRegisterValue       value2);
    
    DxbcRegisterValue emitRegisterExtend(
            DxbcRegisterValue       value,
            uint32_t                size);
    
    DxbcRegisterValue emitRegisterAbsolute(
            DxbcRegisterValue       value);
    
    DxbcRegisterValue emitRegisterNegate(
            DxbcRegisterValue       value);
    
    DxbcRegisterValue emitRegisterZeroTest(
            DxbcRegisterValue       value,
            DxbcZeroTest            test);
    
    DxbcRegisterValue emitRegisterMaskBits(
            DxbcRegisterValue       value,
            uint32_t                mask);
    
    DxbcRegisterValue emitSrcOperandModifiers(
            DxbcRegisterValue       value,
            DxbcRegModifiers        modifiers);
    
    DxbcRegisterValue emitDstOperandModifiers(
            DxbcRegisterValue       value,
            DxbcOpModifiers         modifiers);
    
    ///////////////////////////
    // Sparse feedback methods
    uint32_t emitExtractSparseTexel(
            uint32_t          texelTypeId,
            uint32_t          resultId);

    void emitStoreSparseFeedback(
      const DxbcRegister&     feedbackRegister,
            uint32_t          resultId);

    ////////////////////////////////
    // Pointer manipulation methods
    DxbcRegisterPointer emitArrayAccess(
            DxbcRegisterPointer     pointer,
            spv::StorageClass       sclass,
            uint32_t                index);

    ///////////////////////////////////////
    // Image register manipulation methods
    uint32_t emitLoadSampledImage(
      const DxbcShaderResource&     textureResource,
      const DxbcSampler&            samplerResource,
            bool                    isDepthCompare);
    
    ////////////////////////
    // Address load methods
    DxbcRegisterPointer emitGetTempPtr(
      const DxbcRegister&           operand);
    
    DxbcRegisterPointer emitGetIndexableTempPtr(
      const DxbcRegister&           operand);
    
    DxbcRegisterPointer emitGetInputPtr(
      const DxbcRegister&           operand);
    
    DxbcRegisterPointer emitGetOutputPtr(
      const DxbcRegister&           operand);
    
    DxbcRegisterPointer emitGetConstBufPtr(
      const DxbcRegister&           operand);
    
    DxbcRegisterPointer emitGetImmConstBufPtr(
      const DxbcRegister&           operand);
    
    DxbcRegisterPointer emitGetOperandPtr(
      const DxbcRegister&           operand);
    
    DxbcRegisterPointer emitGetAtomicPointer(
      const DxbcRegister&           operand,
      const DxbcRegister&           address);
    
    //////////////////////////
    // Resource query methods
    DxbcRegisterValue emitQueryBufferSize(
      const DxbcRegister&           resource);
    
    DxbcRegisterValue emitQueryTexelBufferSize(
      const DxbcRegister&           resource);
    
    DxbcRegisterValue emitQueryTextureLods(
      const DxbcRegister&           resource);
    
    DxbcRegisterValue emitQueryTextureSamples(
      const DxbcRegister&           resource);
    
    DxbcRegisterValue emitQueryTextureSize(
      const DxbcRegister&           resource,
            DxbcRegisterValue       lod);
    
    ////////////////////////////////////
    // Buffer index calculation methods
    DxbcRegisterValue emitCalcBufferIndexStructured(
            DxbcRegisterValue       structId,
            DxbcRegisterValue       structOffset,
            uint32_t                structStride);
    
    DxbcRegisterValue emitCalcBufferIndexRaw(
            DxbcRegisterValue       byteOffset);
    
    DxbcRegisterValue emitCalcTexCoord(
            DxbcRegisterValue       coordVector,
      const DxbcImageInfo&          imageInfo);
    
    DxbcRegisterValue emitLoadTexCoord(
      const DxbcRegister&           coordReg,
      const DxbcImageInfo&          imageInfo);
    
    //////////////////////////////
    // Operand load/store methods
    DxbcRegisterValue emitIndexLoad(
            DxbcRegIndex            index);
    
    DxbcRegisterValue emitValueLoad(
            DxbcRegisterPointer     ptr);
    
    void emitValueStore(
            DxbcRegisterPointer     ptr,
            DxbcRegisterValue       value,
            DxbcRegMask             writeMask);
    
    DxbcRegisterValue emitRegisterLoadRaw(
      const DxbcRegister&           reg);
    
    DxbcRegisterValue emitConstantBufferLoad(
      const DxbcRegister&           reg,
            DxbcRegMask             writeMask);
    
    DxbcRegisterValue emitRegisterLoad(
      const DxbcRegister&           reg,
            DxbcRegMask             writeMask);
    
    void emitRegisterStore(
      const DxbcRegister&           reg,
            DxbcRegisterValue       value);
    
    ////////////////////////////
    // Input/output preparation
    void emitInputSetup();
    void emitInputSetup(uint32_t vertexCount);
    
    void emitOutputSetup();
    void emitOutputDepthClamp();
    
    void emitInitWorkgroupMemory();

    //////////////////////////////////////////
    // System value load methods (per shader)
    DxbcRegisterValue emitVsSystemValueLoad(
            DxbcSystemValue         sv,
            DxbcRegMask             mask);
    
    DxbcRegisterValue emitGsSystemValueLoad(
            DxbcSystemValue         sv,
            DxbcRegMask             mask,
            uint32_t                vertexId);
    
    DxbcRegisterValue emitPsSystemValueLoad(
            DxbcSystemValue         sv,
            DxbcRegMask             mask);
    
    ///////////////////////////////////////////
    // System value store methods (per shader)
    void emitVsSystemValueStore(
            DxbcSystemValue         sv,
            DxbcRegMask             mask,
      const DxbcRegisterValue&      value);
    
    void emitHsSystemValueStore(
            DxbcSystemValue         sv,
            DxbcRegMask             mask,
      const DxbcRegisterValue&      value);
    
    void emitDsSystemValueStore(
            DxbcSystemValue         sv,
            DxbcRegMask             mask,
      const DxbcRegisterValue&      value);
    
    void emitGsSystemValueStore(
            DxbcSystemValue         sv,
            DxbcRegMask             mask,
      const DxbcRegisterValue&      value);
    
    void emitPsSystemValueStore(
            DxbcSystemValue         sv,
            DxbcRegMask             mask,
      const DxbcRegisterValue&      value);
    
    ///////////////////////////////
    // Special system value stores
    void emitClipCullStore(
            DxbcSystemValue         sv,
            uint32_t                dstArray);
    
    void emitClipCullLoad(
            DxbcSystemValue         sv,
            uint32_t                srcArray);

    void emitPointSizeStore();

    //////////////////////////////////////
    // Common function definition methods
    void emitInit();
    
    void emitFunctionBegin(
            uint32_t                entryPoint,
            uint32_t                returnType,
            uint32_t                funcType);
    
    void emitFunctionEnd();
    
    void emitFunctionLabel();
    
    void emitMainFunctionBegin();
    
    /////////////////////////////////
    // Shader initialization methods
    void emitVsInit();
    void emitHsInit();
    void emitDsInit();
    void emitGsInit();
    void emitPsInit();
    void emitCsInit();
    
    ///////////////////////////////
    // Shader finalization methods
    void emitVsFinalize();
    void emitHsFinalize();
    void emitDsFinalize();
    void emitGsFinalize();
    void emitPsFinalize();
    void emitCsFinalize();

    ///////////////////////
    // Xfb related methods
    void emitXfbOutputDeclarations();

    void emitXfbOutputSetup(
            uint32_t                          streamId,
            bool                              passthrough);
    
    ///////////////////////////////
    // Hull shader phase methods
    void emitHsControlPointPhase(
      const DxbcCompilerHsControlPointPhase&  phase);
    
    void emitHsForkJoinPhase(
      const DxbcCompilerHsForkJoinPhase&      phase);
    
    void emitHsPhaseBarrier();
    
    void emitHsInvocationBlockBegin(
            uint32_t                          count);
    
    void emitHsInvocationBlockEnd();

    void emitHsOutputSetup();
    
    uint32_t emitTessInterfacePerPatch(
            spv::StorageClass                 storageClass);
    
    uint32_t emitTessInterfacePerVertex(
            spv::StorageClass                 storageClass,
            uint32_t                          vertexCount);
    
    //////////////
    // Misc stuff
    void emitDclInputArray(
            uint32_t          vertexCount);
    
    uint32_t emitDclClipCullDistanceArray(
            uint32_t          length,
            spv::BuiltIn      builtIn,
            spv::StorageClass storageClass);
    
    DxbcCompilerHsControlPointPhase emitNewHullShaderControlPointPhase();
    
    DxbcCompilerHsControlPointPhase emitNewHullShaderPassthroughPhase();
    
    DxbcCompilerHsForkJoinPhase emitNewHullShaderForkJoinPhase();
    
    uint32_t emitSamplePosArray();
    
    void emitFloatControl();

    ///////////////////////////////
    // Variable definition methods
    uint32_t emitNewVariable(
      const DxbcRegisterInfo& info);
    
    uint32_t emitNewBuiltinVariable(
      const DxbcRegisterInfo& info,
            spv::BuiltIn      builtIn,
      const char*             name);
    
    uint32_t emitBuiltinTessLevelOuter(
            spv::StorageClass storageClass);
    
    uint32_t emitBuiltinTessLevelInner(
            spv::StorageClass storageClass);

    uint32_t emitPushConstants();

    ////////////////
    // Misc methods
    DxbcCfgBlock* cfgFindBlock(
      const std::initializer_list<DxbcCfgBlockType>& types);
    
    DxbcBufferInfo getBufferInfo(
      const DxbcRegister& reg);
    
    uint32_t getTexSizeDim(
      const DxbcImageInfo& imageType) const;
    
    uint32_t getTexLayerDim(
      const DxbcImageInfo& imageType) const;
    
    uint32_t getTexCoordDim(
      const DxbcImageInfo& imageType) const;
    
    DxbcRegMask getTexCoordMask(
      const DxbcImageInfo& imageType) const;
    
    DxbcVectorType getInputRegType(
            uint32_t regIdx) const;
    
    DxbcVectorType getOutputRegType(
            uint32_t regIdx) const;
    
    DxbcImageInfo getResourceType(
            DxbcResourceDim   resourceType,
            bool              isUav) const;
    
    spv::ImageFormat getScalarImageFormat(
            DxbcScalarType type) const;
    
    bool isDoubleType(
            DxbcScalarType type) const;
    
    DxbcRegisterPointer getIndexableTempPtr(
      const DxbcRegister&           operand,
            DxbcRegisterValue       vectorId);

    bool caseBlockIsFallthrough() const;

    uint32_t getUavCoherence(
            uint32_t                registerId,
            DxbcUavFlags            flags);
    
    bool ignoreInputSystemValue(
            DxbcSystemValue         sv) const;

    void emitUavBarrier(
            uint64_t                readMask,
            uint64_t                writeMask);

    ///////////////////////////
    // Type definition methods
    uint32_t getScalarTypeId(
            DxbcScalarType type);
    
    uint32_t getVectorTypeId(
      const DxbcVectorType& type);
    
    uint32_t getArrayTypeId(
      const DxbcArrayType& type);
    
    uint32_t getPointerTypeId(
      const DxbcRegisterInfo& type);

    uint32_t getSparseResultTypeId(
            uint32_t baseType);

    uint32_t getFunctionId(
            uint32_t          functionNr);
    
    DxbcCompilerHsForkJoinPhase* getCurrentHsForkJoinPhase();
    
  };
  
}
