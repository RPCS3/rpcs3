#include "dxbc_compiler.h"

namespace dxvk {

  constexpr uint32_t Icb_BindingSlotId   = 14;
  constexpr uint32_t Icb_MaxBakedDwords  = 64;
  
  DxbcCompiler::DxbcCompiler(
    const std::string&        fileName,
    const DxbcModuleInfo&     moduleInfo,
    const DxbcProgramInfo&    programInfo,
    const Rc<DxbcIsgn>&       isgn,
    const Rc<DxbcIsgn>&       osgn,
    const Rc<DxbcIsgn>&       psgn,
    const DxbcAnalysisInfo&   analysis)
  : m_moduleInfo (moduleInfo),
    m_programInfo(programInfo),
    m_module     (spvVersion(1, 6)),
    m_isgn       (isgn),
    m_osgn       (osgn),
    m_psgn       (psgn),
    m_analysis   (&analysis) {
    // Declare an entry point ID. We'll need it during the
    // initialization phase where the execution mode is set.
    m_entryPointId = m_module.allocateId();
    
    // Set the shader name so that we recognize it in renderdoc
    m_module.setDebugSource(
      spv::SourceLanguageUnknown, 0,
      m_module.addDebugString(fileName.c_str()),
      nullptr);

    // Set the memory model. This is the same for all shaders.
    m_module.enableCapability(
      spv::CapabilityVulkanMemoryModel);

    m_module.setMemoryModel(
      spv::AddressingModelLogical,
      spv::MemoryModelVulkan);
    
    // Make sure our interface registers are clear
    for (uint32_t i = 0; i < DxbcMaxInterfaceRegs; i++) {
      m_vRegs.at(i) = DxbcRegisterPointer { };
      m_oRegs.at(i) = DxbcRegisterPointer { };
    }
    
    this->emitInit();
  }
  
  
  DxbcCompiler::~DxbcCompiler() {
    
  }
  
  
  void DxbcCompiler::processInstruction(const DxbcShaderInstruction& ins) {
    m_lastOp = m_currOp;
    m_currOp = ins.op;

    switch (ins.opClass) {
      case DxbcInstClass::Declaration:
        return this->emitDcl(ins);
      
      case DxbcInstClass::CustomData:
        return this->emitCustomData(ins);
        
      case DxbcInstClass::Atomic:
        return this->emitAtomic(ins);
        
      case DxbcInstClass::AtomicCounter:
        return this->emitAtomicCounter(ins);
        
      case DxbcInstClass::Barrier:
        return this->emitBarrier(ins);
        
      case DxbcInstClass::BitExtract:
        return this->emitBitExtract(ins);
        
      case DxbcInstClass::BitInsert:
        return this->emitBitInsert(ins);
        
      case DxbcInstClass::BitScan:
        return this->emitBitScan(ins);
        
      case DxbcInstClass::BufferQuery:
        return this->emitBufferQuery(ins);
        
      case DxbcInstClass::BufferLoad:
        return this->emitBufferLoad(ins);
        
      case DxbcInstClass::BufferStore:
        return this->emitBufferStore(ins);
        
      case DxbcInstClass::ConvertFloat16:
        return this->emitConvertFloat16(ins);
        
      case DxbcInstClass::ConvertFloat64:
        return this->emitConvertFloat64(ins);
        
      case DxbcInstClass::ControlFlow:
        return this->emitControlFlow(ins);
        
      case DxbcInstClass::GeometryEmit:
        return this->emitGeometryEmit(ins);
      
      case DxbcInstClass::HullShaderPhase:
        return this->emitHullShaderPhase(ins);
      
      case DxbcInstClass::HullShaderInstCnt:
        return this->emitHullShaderInstCnt(ins);
      
      case DxbcInstClass::Interpolate:
        return this->emitInterpolate(ins);
      
      case DxbcInstClass::NoOperation:
        return;

      case DxbcInstClass::SparseCheckAccess:
        return this->emitSparseCheckAccess(ins);

      case DxbcInstClass::TextureQuery:
        return this->emitTextureQuery(ins);
        
      case DxbcInstClass::TextureQueryLod:
        return this->emitTextureQueryLod(ins);
        
      case DxbcInstClass::TextureQueryMs:
        return this->emitTextureQueryMs(ins);
        
      case DxbcInstClass::TextureQueryMsPos:
        return this->emitTextureQueryMsPos(ins);
        
      case DxbcInstClass::TextureFetch:
        return this->emitTextureFetch(ins);
        
      case DxbcInstClass::TextureGather:
        return this->emitTextureGather(ins);
        
      case DxbcInstClass::TextureSample:
        return this->emitTextureSample(ins);
        
      case DxbcInstClass::TypedUavLoad:
        return this->emitTypedUavLoad(ins);
        
      case DxbcInstClass::TypedUavStore:
        return this->emitTypedUavStore(ins);
        
      case DxbcInstClass::VectorAlu:
        return this->emitVectorAlu(ins);
        
      case DxbcInstClass::VectorCmov:
        return this->emitVectorCmov(ins);
        
      case DxbcInstClass::VectorCmp:
        return this->emitVectorCmp(ins);
        
      case DxbcInstClass::VectorDeriv:
        return this->emitVectorDeriv(ins);
        
      case DxbcInstClass::VectorDot:
        return this->emitVectorDot(ins);
        
      case DxbcInstClass::VectorIdiv:
        return this->emitVectorIdiv(ins);
        
      case DxbcInstClass::VectorImul:
        return this->emitVectorImul(ins);
        
      case DxbcInstClass::VectorMsad:
        return this->emitVectorMsad(ins);
        
      case DxbcInstClass::VectorShift:
        return this->emitVectorShift(ins);
        
      case DxbcInstClass::VectorSinCos:
        return this->emitVectorSinCos(ins);
        
      default:
        Logger::warn(
          str::format("DxbcCompiler: Unhandled opcode class: ",
          ins.op));
    }
  }


  void DxbcCompiler::processXfbPassthrough() {
    m_module.setExecutionMode (m_entryPointId, spv::ExecutionModeInputPoints);
    m_module.setExecutionMode (m_entryPointId, spv::ExecutionModeOutputPoints);
    m_module.setOutputVertices(m_entryPointId, 1);

    for (auto e = m_isgn->begin(); e != m_isgn->end(); e++) {
      emitDclInput(e->registerId, 1,
        e->componentMask, DxbcSystemValue::None,
        DxbcInterpolationMode::Undefined);
    }

    // Figure out which streams to enable
    uint32_t streamMask = 0;

    for (size_t i = 0; i < m_xfbVars.size(); i++)
      streamMask |= 1u << m_xfbVars[i].streamId;
    
    for (uint32_t streamId : bit::BitMask(streamMask)) {
      emitXfbOutputSetup(streamId, true);
      m_module.opEmitVertex(m_module.constu32(streamId));
    }

    // End the main function
    emitFunctionEnd();

    // For pass-through we always assume points
    m_inputTopology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
  }
  
  
  SpirvCodeBuffer DxbcCompiler::finalize() {
    // Depending on the shader type, this will prepare
    // input registers, call various shader functions
    // and write back the output registers.
    switch (m_programInfo.type()) {
      case DxbcProgramType::VertexShader:   this->emitVsFinalize(); break;
      case DxbcProgramType::HullShader:     this->emitHsFinalize(); break;
      case DxbcProgramType::DomainShader:   this->emitDsFinalize(); break;
      case DxbcProgramType::GeometryShader: this->emitGsFinalize(); break;
      case DxbcProgramType::PixelShader:    this->emitPsFinalize(); break;
      case DxbcProgramType::ComputeShader:  this->emitCsFinalize(); break;
      default: throw DxvkError("Invalid shader stage");
    }

    // Emit float control mode if the extension is supported
    this->emitFloatControl();
    
    // Declare the entry point, we now have all the
    // information we need, including the interfaces
    m_module.addEntryPoint(m_entryPointId,
      m_programInfo.executionModel(), "main");
    m_module.setDebugName(m_entryPointId, "main");

    return m_module.compile();
  }
  
  
  void DxbcCompiler::emitDcl(const DxbcShaderInstruction& ins) {
    switch (ins.op) {
      case DxbcOpcode::DclGlobalFlags:
        return this->emitDclGlobalFlags(ins);
        
      case DxbcOpcode::DclIndexRange:
        return this->emitDclIndexRange(ins);
        
      case DxbcOpcode::DclTemps:
        return this->emitDclTemps(ins);
        
      case DxbcOpcode::DclIndexableTemp:
        return this->emitDclIndexableTemp(ins);
        
      case DxbcOpcode::DclInput:
      case DxbcOpcode::DclInputSgv:
      case DxbcOpcode::DclInputSiv:
      case DxbcOpcode::DclInputPs:
      case DxbcOpcode::DclInputPsSgv:
      case DxbcOpcode::DclInputPsSiv:
      case DxbcOpcode::DclOutput:
      case DxbcOpcode::DclOutputSgv:
      case DxbcOpcode::DclOutputSiv:
        return this->emitDclInterfaceReg(ins);
        
      case DxbcOpcode::DclConstantBuffer:
        return this->emitDclConstantBuffer(ins);
        
      case DxbcOpcode::DclSampler:
        return this->emitDclSampler(ins);
      
      case DxbcOpcode::DclStream:
        return this->emitDclStream(ins);
        
      case DxbcOpcode::DclUavTyped:
      case DxbcOpcode::DclResource:
        return this->emitDclResourceTyped(ins);
        
      case DxbcOpcode::DclUavRaw:
      case DxbcOpcode::DclResourceRaw:
      case DxbcOpcode::DclUavStructured:
      case DxbcOpcode::DclResourceStructured:
        return this->emitDclResourceRawStructured(ins);
      
      case DxbcOpcode::DclThreadGroupSharedMemoryRaw:
      case DxbcOpcode::DclThreadGroupSharedMemoryStructured:
        return this->emitDclThreadGroupSharedMemory(ins);
        
      case DxbcOpcode::DclGsInputPrimitive:
        return this->emitDclGsInputPrimitive(ins);
        
      case DxbcOpcode::DclGsOutputPrimitiveTopology:
        return this->emitDclGsOutputTopology(ins);
        
      case DxbcOpcode::DclMaxOutputVertexCount:
        return this->emitDclMaxOutputVertexCount(ins);
        
      case DxbcOpcode::DclInputControlPointCount:
        return this->emitDclInputControlPointCount(ins);
      
      case DxbcOpcode::DclOutputControlPointCount:
        return this->emitDclOutputControlPointCount(ins);
      
      case DxbcOpcode::DclHsMaxTessFactor:
        return this->emitDclHsMaxTessFactor(ins);
        
      case DxbcOpcode::DclTessDomain:
        return this->emitDclTessDomain(ins);
      
      case DxbcOpcode::DclTessPartitioning:
        return this->emitDclTessPartitioning(ins);
      
      case DxbcOpcode::DclTessOutputPrimitive:
        return this->emitDclTessOutputPrimitive(ins);
      
      case DxbcOpcode::DclThreadGroup:
        return this->emitDclThreadGroup(ins);
      
      case DxbcOpcode::DclGsInstanceCount:
        return this->emitDclGsInstanceCount(ins);
      
      default:
        Logger::warn(
          str::format("DxbcCompiler: Unhandled opcode: ",
          ins.op));
    }
  }
  
  
  void DxbcCompiler::emitDclGlobalFlags(const DxbcShaderInstruction& ins) {
    const DxbcGlobalFlags flags = ins.controls.globalFlags();
    
    if (flags.test(DxbcGlobalFlag::RefactoringAllowed))
      m_precise = false;

    if (flags.test(DxbcGlobalFlag::EarlyFragmentTests))
      m_module.setExecutionMode(m_entryPointId, spv::ExecutionModeEarlyFragmentTests);
  }
  
  
  void DxbcCompiler::emitDclIndexRange(const DxbcShaderInstruction& ins) {
    // dcl_index_range has one operand:
    //    (0) Range start, either an input or output register
    //    (1) Range end
    uint32_t index = ins.dst[0].idxDim - 1u;

    DxbcIndexRange range = { };
    range.type = ins.dst[0].type;
    range.start = ins.dst[0].idx[index].offset;
    range.length = ins.imm[0].u32;

    m_indexRanges.push_back(range);
  }


  void DxbcCompiler::emitDclTemps(const DxbcShaderInstruction& ins) {
    // dcl_temps has one operand:
    //    (imm0) Number of temp registers

    // Ignore this and declare temps on demand.
  }
  
  
  void DxbcCompiler::emitDclIndexableTemp(const DxbcShaderInstruction& ins) {
    // dcl_indexable_temps has three operands:
    //    (imm0) Array register index (x#)
    //    (imm1) Number of vectors stored in the array
    //    (imm2) Component count of each individual vector. This is
    //    always 4 in fxc-generated binaries and therefore useless.
    const uint32_t regId = ins.imm[0].u32;

    DxbcRegisterInfo info;
    info.type.ctype   = DxbcScalarType::Float32;
    info.type.ccount  = m_analysis->xRegMasks.at(regId).minComponents();
    info.type.alength = ins.imm[1].u32;
    info.sclass       = spv::StorageClassPrivate;

    if (regId >= m_xRegs.size())
      m_xRegs.resize(regId + 1);
    
    m_xRegs.at(regId).ccount = info.type.ccount;
    m_xRegs.at(regId).alength = info.type.alength;
    m_xRegs.at(regId).varId  = emitNewVariable(info);
    
    m_module.setDebugName(m_xRegs.at(regId).varId,
      str::format("x", regId).c_str());
  }
  
  
  void DxbcCompiler::emitDclInterfaceReg(const DxbcShaderInstruction& ins) {
    switch (ins.dst[0].type) {
      case DxbcOperandType::InputControlPoint:
        if (m_programInfo.type() != DxbcProgramType::HullShader)
          break;
        [[fallthrough]];

      case DxbcOperandType::Input:
      case DxbcOperandType::Output: {
        // dcl_input and dcl_output instructions
        // have the following operands:
        //    (dst0) The register to declare
        //    (imm0) The system value (optional)
        uint32_t regDim = 0;
        uint32_t regIdx = 0;
        
        // In the vertex and fragment shader stage, the
        // operand indices will have the following format:
        //    (0) Register index
        // 
        // In other stages, the input and output registers
        // may be declared as arrays of a fixed size:
        //    (0) Array length
        //    (1) Register index
        if (ins.dst[0].idxDim == 2) {
          regDim = ins.dst[0].idx[0].offset;
          regIdx = ins.dst[0].idx[1].offset;
        } else if (ins.dst[0].idxDim == 1) {
          regIdx = ins.dst[0].idx[0].offset;
        } else {
          Logger::err(str::format(
            "DxbcCompiler: ", ins.op,
            ": Invalid index dimension"));
          return;
        }
        
        // This declaration may map an output register to a system
        // value. If that is the case, the system value type will
        // be stored in the second operand.
        const bool hasSv =
            ins.op == DxbcOpcode::DclInputSgv
         || ins.op == DxbcOpcode::DclInputSiv
         || ins.op == DxbcOpcode::DclInputPsSgv
         || ins.op == DxbcOpcode::DclInputPsSiv
         || ins.op == DxbcOpcode::DclOutputSgv
         || ins.op == DxbcOpcode::DclOutputSiv;
        
        DxbcSystemValue sv = DxbcSystemValue::None;
        
        if (hasSv)
          sv = static_cast<DxbcSystemValue>(ins.imm[0].u32);
        
        // In the pixel shader, inputs are declared with an
        // interpolation mode that is part of the op token.
        const bool hasInterpolationMode =
            ins.op == DxbcOpcode::DclInputPs
         || ins.op == DxbcOpcode::DclInputPsSiv;
        
        DxbcInterpolationMode im = DxbcInterpolationMode::Undefined;
        
        if (hasInterpolationMode)
          im = ins.controls.interpolation();
        
        // Declare the actual input/output variable
        switch (ins.op) {
          case DxbcOpcode::DclInput:
          case DxbcOpcode::DclInputSgv:
          case DxbcOpcode::DclInputSiv:
          case DxbcOpcode::DclInputPs:
          case DxbcOpcode::DclInputPsSgv:
          case DxbcOpcode::DclInputPsSiv:
            this->emitDclInput(regIdx, regDim, ins.dst[0].mask, sv, im);
            break;
          
          case DxbcOpcode::DclOutput:
          case DxbcOpcode::DclOutputSgv:
          case DxbcOpcode::DclOutputSiv:
            this->emitDclOutput(regIdx, regDim, ins.dst[0].mask, sv, im);
            break;
          
          default:
            Logger::err(str::format(
              "DxbcCompiler: Unexpected opcode: ",
              ins.op));
        }
      } break;
  
      case DxbcOperandType::InputThreadId: {
        m_cs.builtinGlobalInvocationId = emitNewBuiltinVariable({
          { DxbcScalarType::Uint32, 3, 0 },
          spv::StorageClassInput },
          spv::BuiltInGlobalInvocationId,
          "vThreadId");
      } break;
  
      case DxbcOperandType::InputThreadGroupId: {
        m_cs.builtinWorkgroupId = emitNewBuiltinVariable({
          { DxbcScalarType::Uint32, 3, 0 },
          spv::StorageClassInput },
          spv::BuiltInWorkgroupId,
          "vThreadGroupId");
      } break;
  
      case DxbcOperandType::InputThreadIdInGroup: {
        m_cs.builtinLocalInvocationId = emitNewBuiltinVariable({
          { DxbcScalarType::Uint32, 3, 0 },
          spv::StorageClassInput },
          spv::BuiltInLocalInvocationId,
          "vThreadIdInGroup");
      } break;
  
      case DxbcOperandType::InputThreadIndexInGroup: {
        m_cs.builtinLocalInvocationIndex = emitNewBuiltinVariable({
          { DxbcScalarType::Uint32, 1, 0 },
          spv::StorageClassInput },
          spv::BuiltInLocalInvocationIndex,
          "vThreadIndexInGroup");
      } break;
      
      case DxbcOperandType::InputCoverageMask: {
        m_ps.builtinSampleMaskIn = emitNewBuiltinVariable({
          { DxbcScalarType::Uint32, 1, 1 },
          spv::StorageClassInput },
          spv::BuiltInSampleMask,
          "vCoverage");
      } break;
      
      case DxbcOperandType::OutputCoverageMask: {
        m_ps.builtinSampleMaskOut = emitNewBuiltinVariable({
          { DxbcScalarType::Uint32, 1, 1 },
          spv::StorageClassOutput },
          spv::BuiltInSampleMask,
          "oMask");
      } break;
      
      case DxbcOperandType::OutputDepth: {
        m_module.setExecutionMode(m_entryPointId,
          spv::ExecutionModeDepthReplacing);
        m_ps.builtinDepth = emitNewBuiltinVariable({
          { DxbcScalarType::Float32, 1, 0 },
          spv::StorageClassOutput },
          spv::BuiltInFragDepth,
          "oDepth");
      } break;
      
      case DxbcOperandType::OutputStencilRef: {
        m_module.enableExtension("SPV_EXT_shader_stencil_export");
        m_module.enableCapability(spv::CapabilityStencilExportEXT);
        m_module.setExecutionMode(m_entryPointId,
          spv::ExecutionModeStencilRefReplacingEXT);
        m_ps.builtinStencilRef = emitNewBuiltinVariable({
          { DxbcScalarType::Sint32, 1, 0 },
          spv::StorageClassOutput },
          spv::BuiltInFragStencilRefEXT,
          "oStencilRef");
      } break;

      case DxbcOperandType::OutputDepthGe: {
        m_module.setExecutionMode(m_entryPointId, spv::ExecutionModeDepthReplacing);
        m_module.setExecutionMode(m_entryPointId, spv::ExecutionModeDepthGreater);
        m_ps.builtinDepth = emitNewBuiltinVariable({
          { DxbcScalarType::Float32, 1, 0 },
          spv::StorageClassOutput },
          spv::BuiltInFragDepth,
          "oDepthGe");
      } break;
      
      case DxbcOperandType::OutputDepthLe: {
        m_module.setExecutionMode(m_entryPointId, spv::ExecutionModeDepthReplacing);
        m_module.setExecutionMode(m_entryPointId, spv::ExecutionModeDepthLess);
        m_ps.builtinDepth = emitNewBuiltinVariable({
          { DxbcScalarType::Float32, 1, 0 },
          spv::StorageClassOutput },
          spv::BuiltInFragDepth,
          "oDepthLe");
      } break;
      
      case DxbcOperandType::InputPrimitiveId: {
        m_primitiveIdIn = emitNewBuiltinVariable({
          { DxbcScalarType::Uint32, 1, 0 },
          spv::StorageClassInput },
          spv::BuiltInPrimitiveId,
          "vPrim");
      } break;
      
      case DxbcOperandType::InputDomainPoint: {
        m_ds.builtinTessCoord = emitNewBuiltinVariable({
          { DxbcScalarType::Float32, 3, 0 },
          spv::StorageClassInput },
          spv::BuiltInTessCoord,
          "vDomain");
      } break;
      
      case DxbcOperandType::InputForkInstanceId:
      case DxbcOperandType::InputJoinInstanceId: {
        auto phase = this->getCurrentHsForkJoinPhase();
        
        phase->instanceIdPtr = m_module.newVar(
          m_module.defPointerType(
            m_module.defIntType(32, 0),
            spv::StorageClassFunction),
          spv::StorageClassFunction);
        
        m_module.opStore(phase->instanceIdPtr, phase->instanceId);
        m_module.setDebugName(phase->instanceIdPtr,
          ins.dst[0].type == DxbcOperandType::InputForkInstanceId
            ? "vForkInstanceId" : "vJoinInstanceId");
      } break;
      
      case DxbcOperandType::OutputControlPointId: {
        // This system value map to the invocation
        // ID, which has been declared already.
      } break;
      
      case DxbcOperandType::InputPatchConstant:
      case DxbcOperandType::OutputControlPoint: {
        // These have been declared as global input and
        // output arrays, so there's nothing left to do.
      } break;
      
      case DxbcOperandType::InputGsInstanceId: {
        m_gs.builtinInvocationId = emitNewBuiltinVariable({
          { DxbcScalarType::Uint32, 1, 0 },
          spv::StorageClassInput },
          spv::BuiltInInvocationId,
          "vInstanceID");
      } break;
      
      case DxbcOperandType::InputInnerCoverage: {
        m_module.enableExtension("SPV_EXT_fragment_fully_covered");
        m_module.enableCapability(spv::CapabilityFragmentFullyCoveredEXT);

        // This is bool in SPIR-V but uint32 in DXBC. A bool value of
        // false must be 0, and bit 1 must be set to represent true.
        uint32_t builtinId = emitNewBuiltinVariable({
          { DxbcScalarType::Bool, 1, 0 },
          spv::StorageClassInput },
          spv::BuiltInFullyCoveredEXT,
          nullptr);

        m_ps.builtinInnerCoverageId = emitNewVariable({
          { DxbcScalarType::Uint32, 1, 0 },
          spv::StorageClassPrivate });

        m_module.setDebugName(m_ps.builtinInnerCoverageId, "vInnerCoverage");

        uint32_t boolTypeId = m_module.defBoolType();
        uint32_t uintTypeId = m_module.defIntType(32, 0);

        m_module.opStore(m_ps.builtinInnerCoverageId,
          m_module.opSelect(uintTypeId,
            m_module.opLoad(boolTypeId, builtinId),
            m_module.constu32(1),
            m_module.constu32(0)));
      } break;

      default:
        Logger::err(str::format(
          "DxbcCompiler: Unsupported operand type declaration: ",
          ins.dst[0].type));
        
    }
  }
  
  
  void DxbcCompiler::emitDclInput(
          uint32_t                regIdx,
          uint32_t                regDim,
          DxbcRegMask             regMask,
          DxbcSystemValue         sv,
          DxbcInterpolationMode   im) {
    // Avoid declaring the same variable multiple times.
    // This may happen when multiple system values are
    // mapped to different parts of the same register.
    if (m_vRegs.at(regIdx).id == 0 && sv == DxbcSystemValue::None) {
      const DxbcVectorType regType = getInputRegType(regIdx);
      
      DxbcRegisterInfo info;
      info.type.ctype   = regType.ctype;
      info.type.ccount  = regType.ccount;
      info.type.alength = regDim;
      info.sclass = spv::StorageClassInput;
      
      const uint32_t varId = emitNewVariable(info);
      
      m_module.decorateLocation(varId, regIdx);
      m_module.setDebugName(varId, str::format("v", regIdx).c_str());
      
      m_vRegs.at(regIdx) = { regType, varId };
      
      // Interpolation mode, used in pixel shaders
      if (im == DxbcInterpolationMode::Constant)
        m_module.decorate(varId, spv::DecorationFlat);
      
      if (im == DxbcInterpolationMode::LinearCentroid
       || im == DxbcInterpolationMode::LinearNoPerspectiveCentroid)
        m_module.decorate(varId, spv::DecorationCentroid);
      
      if (im == DxbcInterpolationMode::LinearNoPerspective
       || im == DxbcInterpolationMode::LinearNoPerspectiveCentroid
       || im == DxbcInterpolationMode::LinearNoPerspectiveSample)
        m_module.decorate(varId, spv::DecorationNoPerspective);
      
      if (im == DxbcInterpolationMode::LinearSample
       || im == DxbcInterpolationMode::LinearNoPerspectiveSample) {
        m_module.enableCapability(spv::CapabilitySampleRateShading);
        m_module.decorate(varId, spv::DecorationSample);
      }

      if (m_moduleInfo.options.forceSampleRateShading) {
        if (im == DxbcInterpolationMode::Linear
         || im == DxbcInterpolationMode::LinearNoPerspective) {
          m_module.enableCapability(spv::CapabilitySampleRateShading);
          m_module.decorate(varId, spv::DecorationSample);
        }
      }

      // Declare the input slot as defined
      m_inputMask |= 1u << regIdx;
      m_vArrayLength = std::max(m_vArrayLength, regIdx + 1);
    } else if (sv != DxbcSystemValue::None) {
      // Add a new system value mapping if needed
      bool skipSv = sv == DxbcSystemValue::ClipDistance
                 || sv == DxbcSystemValue::CullDistance;
      
      if (!skipSv)
        m_vMappings.push_back({ regIdx, regMask, sv });
    }
  }
  
  
  void DxbcCompiler::emitDclOutput(
          uint32_t                regIdx,
          uint32_t                regDim,
          DxbcRegMask             regMask,
          DxbcSystemValue         sv,
          DxbcInterpolationMode   im) {
    // Add a new system value mapping if needed. Clip
    // and cull distances are handled separately.
    if (sv != DxbcSystemValue::None
     && sv != DxbcSystemValue::ClipDistance
     && sv != DxbcSystemValue::CullDistance)
      m_oMappings.push_back({ regIdx, regMask, sv });
    
    if (m_programInfo.type() == DxbcProgramType::HullShader) {
      // Hull shaders don't use standard outputs
      if (getCurrentHsForkJoinPhase() != nullptr)
        m_hs.outputPerPatchMask |= 1 << regIdx;
    } else if (m_oRegs.at(regIdx).id == 0) {
      // Avoid declaring the same variable multiple times.
      // This may happen when multiple system values are
      // mapped to different parts of the same register.
      const DxbcVectorType regType = getOutputRegType(regIdx);
      
      DxbcRegisterInfo info;
      info.type.ctype   = regType.ctype;
      info.type.ccount  = regType.ccount;
      info.type.alength = regDim;
      info.sclass = spv::StorageClassOutput;

      // In xfb mode, we set up the actual
      // output vars when emitting a vertex
      if (m_moduleInfo.xfb != nullptr)
        info.sclass = spv::StorageClassPrivate;
      
      // In geometry shaders, don't duplicate system value outputs
      // to stay within device limits. The pixel shader will read
      // all GS system value outputs as system value inputs.
      if (m_programInfo.type() == DxbcProgramType::GeometryShader && sv != DxbcSystemValue::None)
        info.sclass = spv::StorageClassPrivate;

      const uint32_t varId = this->emitNewVariable(info);
      m_module.setDebugName(varId, str::format("o", regIdx).c_str());
      
      if (info.sclass == spv::StorageClassOutput) {
        m_module.decorateLocation(varId, regIdx);

        // Add index decoration for potential dual-source blending
        if (m_programInfo.type() == DxbcProgramType::PixelShader)
          m_module.decorateIndex(varId, 0);

        // Declare vertex positions in all stages as invariant, even if
        // this is not the last stage, to help with potential Z fighting.
        if (sv == DxbcSystemValue::Position && m_moduleInfo.options.invariantPosition)
          m_module.decorate(varId, spv::DecorationInvariant);
      }
      
      m_oRegs.at(regIdx) = { regType, varId };
      
      // Declare the output slot as defined
      m_outputMask |= 1u << regIdx;
    }
  }
  
  
  void DxbcCompiler::emitDclConstantBuffer(const DxbcShaderInstruction& ins) {
    // dcl_constant_buffer has one operand with two indices:
    //    (0) Constant buffer register ID (cb#)
    //    (1) Number of constants in the buffer
    uint32_t bufferId     = ins.dst[0].idx[0].offset;
    uint32_t elementCount = ins.dst[0].idx[1].offset;

    // With dynamic indexing, games will often index constant buffers
    // out of bounds. Declare an upper bound to stay within spec.
    if (ins.controls.accessType() == DxbcConstantBufferAccessType::DynamicallyIndexed)
      elementCount = 4096;

    this->emitDclConstantBufferVar(bufferId, elementCount, 4u,
      str::format("cb", bufferId).c_str());
  }
  
  
  void DxbcCompiler::emitDclConstantBufferVar(
          uint32_t                regIdx,
          uint32_t                numConstants,
          uint32_t                numComponents,
    const char*                   name) {
    // Uniform buffer data is stored as a fixed-size array
    // of 4x32-bit vectors. SPIR-V requires explicit strides.
    const uint32_t arrayType = m_module.defArrayTypeUnique(
      getVectorTypeId({ DxbcScalarType::Float32, numComponents }),
      m_module.constu32(numConstants));
    m_module.decorateArrayStride(arrayType, sizeof(uint32_t) * numComponents);
    
    // SPIR-V requires us to put that array into a
    // struct and decorate that struct as a block.
    const uint32_t structType = m_module.defStructTypeUnique(1, &arrayType);
    
    m_module.decorate(structType, spv::DecorationBlock);
    m_module.memberDecorateOffset(structType, 0, 0);
    
    m_module.setDebugName        (structType, str::format(name, "_t").c_str());
    m_module.setDebugMemberName  (structType, 0, "m");
    
    // Variable that we'll use to access the buffer
    const uint32_t varId = m_module.newVar(
      m_module.defPointerType(structType, spv::StorageClassUniform),
      spv::StorageClassUniform);
    
    m_module.setDebugName(varId, name);
    
    // Compute the DXVK binding slot index for the buffer.
    // D3D11 needs to bind the actual buffers to this slot.
    uint32_t bindingId = computeConstantBufferBinding(
      m_programInfo.type(), regIdx);
    
    m_module.decorateDescriptorSet(varId, 0);
    m_module.decorateBinding(varId, bindingId);

    DxbcConstantBuffer buf;
    buf.varId  = varId;
    buf.size   = numConstants;
    m_constantBuffers.at(regIdx) = buf;
    
    // Store descriptor info for the shader interface
    DxvkBindingInfo binding = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER };
    binding.viewType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
    binding.access = VK_ACCESS_UNIFORM_READ_BIT;
    binding.resourceBinding = bindingId;
    binding.uboSet = true;
    m_bindings.push_back(binding);
  }


  void DxbcCompiler::emitDclSampler(const DxbcShaderInstruction& ins) {
    // dclSampler takes one operand:
    //    (dst0) The sampler register to declare
    const uint32_t samplerId = ins.dst[0].idx[0].offset;
    
    // The sampler type is opaque, but we still have to
    // define a pointer and a variable in oder to use it
    const uint32_t samplerType = m_module.defSamplerType();
    const uint32_t samplerPtrType = m_module.defPointerType(
      samplerType, spv::StorageClassUniformConstant);
    
    // Define the sampler variable
    const uint32_t varId = m_module.newVar(samplerPtrType,
      spv::StorageClassUniformConstant);
    m_module.setDebugName(varId,
      str::format("s", samplerId).c_str());
    
    m_samplers.at(samplerId).varId  = varId;
    m_samplers.at(samplerId).typeId = samplerType;
    
    // Compute binding slot index for the sampler
    uint32_t bindingId = computeSamplerBinding(
      m_programInfo.type(), samplerId);
    
    m_module.decorateDescriptorSet(varId, 0);
    m_module.decorateBinding(varId, bindingId);
    
    // Store descriptor info for the shader interface
    DxvkBindingInfo binding = { VK_DESCRIPTOR_TYPE_SAMPLER };
    binding.viewType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
    binding.resourceBinding = bindingId;
    m_bindings.push_back(binding);
  }
  
  
  void DxbcCompiler::emitDclStream(const DxbcShaderInstruction& ins) {
    if (ins.dst[0].idx[0].offset != 0 && m_moduleInfo.xfb == nullptr)
      Logger::err("Dxbc: Multiple streams not supported");
  }
  
  
  void DxbcCompiler::emitDclResourceTyped(const DxbcShaderInstruction& ins) {
    // dclResource takes two operands:
    //    (dst0) The resource register ID
    //    (imm0) The resource return type
    const uint32_t registerId = ins.dst[0].idx[0].offset;
    
    // We also handle unordered access views here
    const bool isUav = ins.op == DxbcOpcode::DclUavTyped;
    
    if (isUav) {
      if (m_moduleInfo.options.supportsTypedUavLoadR32)
        m_module.enableCapability(spv::CapabilityStorageImageReadWithoutFormat);
      m_module.enableCapability(spv::CapabilityStorageImageWriteWithoutFormat);
    }
    
    // Defines the type of the resource (texture2D, ...)
    const DxbcResourceDim resourceType = ins.controls.resourceDim();
    
    // Defines the type of a read operation. DXBC has the ability
    // to define four different types whereas SPIR-V only allows
    // one, but in practice this should not be much of a problem.
    auto xType = static_cast<DxbcResourceReturnType>(
      bit::extract(ins.imm[0].u32, 0, 3));
    auto yType = static_cast<DxbcResourceReturnType>(
      bit::extract(ins.imm[0].u32, 4, 7));
    auto zType = static_cast<DxbcResourceReturnType>(
      bit::extract(ins.imm[0].u32, 8, 11));
    auto wType = static_cast<DxbcResourceReturnType>(
      bit::extract(ins.imm[0].u32, 12, 15));
    
    if ((xType != yType) || (xType != zType) || (xType != wType))
      Logger::warn("DxbcCompiler: dcl_resource: Ignoring resource return types");
    
    // Declare the actual sampled type
    const DxbcScalarType sampledType = [xType] {
      switch (xType) {
        // FIXME is this correct? There's no documentation about it
        case DxbcResourceReturnType::Mixed: return DxbcScalarType::Uint32;
        // FIXME do we have to manually clamp writes to SNORM/UNORM resources?
        case DxbcResourceReturnType::Snorm: return DxbcScalarType::Float32;
        case DxbcResourceReturnType::Unorm: return DxbcScalarType::Float32;
        case DxbcResourceReturnType::Float: return DxbcScalarType::Float32;
        case DxbcResourceReturnType::Sint:  return DxbcScalarType::Sint32;
        case DxbcResourceReturnType::Uint:  return DxbcScalarType::Uint32;
        default: throw DxvkError(str::format("DxbcCompiler: Invalid sampled type: ", xType));
      }
    }();
    
    // Declare the resource type
    const uint32_t sampledTypeId = getScalarTypeId(sampledType);
    const DxbcImageInfo typeInfo = getResourceType(resourceType, isUav);    
    
    // Declare additional capabilities if necessary
    switch (resourceType) {
      case DxbcResourceDim::Buffer:
        m_module.enableCapability(isUav
          ? spv::CapabilityImageBuffer
          : spv::CapabilitySampledBuffer);
        break;
      
      case DxbcResourceDim::Texture1D:
      case DxbcResourceDim::Texture1DArr:
        m_module.enableCapability(isUav
          ? spv::CapabilityImage1D
          : spv::CapabilitySampled1D);
        break;
      
      case DxbcResourceDim::TextureCubeArr:
        m_module.enableCapability(
          spv::CapabilitySampledCubeArray);
        break;
      
      default:
        // No additional capabilities required
        break;
    }
    
    // If the read-without-format capability is not set and this
    // image is access via a typed load, or if atomic operations
    // are used,, we must define the image format explicitly.
    spv::ImageFormat imageFormat = spv::ImageFormatUnknown;
    
    if (isUav) {
      if ((m_analysis->uavInfos[registerId].accessAtomicOp)
       || (m_analysis->uavInfos[registerId].accessTypedLoad
        && !m_moduleInfo.options.supportsTypedUavLoadR32))
        imageFormat = getScalarImageFormat(sampledType);
    }
    
    // We do not know whether the image is going to be used as
    // a color image or a depth image yet, but we can pick the
    // correct type when creating a sampled image object.
    const uint32_t imageTypeId = m_module.defImageType(sampledTypeId,
      typeInfo.dim, 0, typeInfo.array, typeInfo.ms, typeInfo.sampled,
      imageFormat);
    
    // We'll declare the texture variable with the color type
    // and decide which one to use when the texture is sampled.
    const uint32_t resourcePtrType = m_module.defPointerType(
      imageTypeId, spv::StorageClassUniformConstant);
    
    const uint32_t varId = m_module.newVar(resourcePtrType,
      spv::StorageClassUniformConstant);
    
    m_module.setDebugName(varId,
      str::format(isUav ? "u" : "t", registerId).c_str());
    
    // Compute the DXVK binding slot index for the resource.
    // D3D11 needs to bind the actual resource to this slot.
    uint32_t bindingId = isUav
      ? computeUavBinding(m_programInfo.type(), registerId)
      : computeSrvBinding(m_programInfo.type(), registerId);
    
    m_module.decorateDescriptorSet(varId, 0);
    m_module.decorateBinding(varId, bindingId);
    
    // Declare a specialization constant which will
    // store whether or not the resource is bound.
    if (isUav) {
      DxbcUav uav;
      uav.type          = DxbcResourceType::Typed;
      uav.imageInfo     = typeInfo;
      uav.varId         = varId;
      uav.ctrId         = 0;
      uav.sampledType   = sampledType;
      uav.sampledTypeId = sampledTypeId;
      uav.imageTypeId   = imageTypeId;
      uav.structStride  = 0;
      uav.coherence     = getUavCoherence(registerId, ins.controls.uavFlags());
      uav.isRawSsbo     = false;
      m_uavs.at(registerId) = uav;
    } else {
      DxbcShaderResource res;
      res.type          = DxbcResourceType::Typed;
      res.imageInfo     = typeInfo;
      res.varId         = varId;
      res.sampledType   = sampledType;
      res.sampledTypeId = sampledTypeId;
      res.imageTypeId   = imageTypeId;
      res.colorTypeId   = imageTypeId;
      res.depthTypeId   = 0;
      res.structStride  = 0;
      res.isRawSsbo     = false;
      
      if ((sampledType == DxbcScalarType::Float32)
       && (resourceType == DxbcResourceDim::Texture1D
        || resourceType == DxbcResourceDim::Texture1DArr
        || resourceType == DxbcResourceDim::Texture2D
        || resourceType == DxbcResourceDim::Texture2DArr
        || resourceType == DxbcResourceDim::TextureCube
        || resourceType == DxbcResourceDim::TextureCubeArr)) {
        res.depthTypeId = m_module.defImageType(sampledTypeId,
          typeInfo.dim, 1, typeInfo.array, typeInfo.ms, typeInfo.sampled,
          spv::ImageFormatUnknown);
      }
      
      m_textures.at(registerId) = res;
    }
    
    // Store descriptor info for the shader interface
    DxvkBindingInfo binding = { };
    binding.viewType = typeInfo.vtype;
    binding.resourceBinding = bindingId;
    binding.isMultisampled = typeInfo.ms;

    if (isUav) {
      binding.descriptorType = resourceType == DxbcResourceDim::Buffer
        ? VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER
        : VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
      binding.access = m_analysis->uavInfos[registerId].accessFlags;

      if (!m_analysis->uavInfos[registerId].nonInvariantAccess)
        binding.accessOp = m_analysis->uavInfos[registerId].accessOp;

      if (!(binding.access & VK_ACCESS_SHADER_WRITE_BIT))
        m_module.decorate(varId, spv::DecorationNonWritable);
      if (!(binding.access & VK_ACCESS_SHADER_READ_BIT))
        m_module.decorate(varId, spv::DecorationNonReadable);
    } else {
      binding.descriptorType = resourceType == DxbcResourceDim::Buffer
        ? VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER
        : VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
      binding.access = VK_ACCESS_SHADER_READ_BIT;
    }

    m_bindings.push_back(binding);
  }
  
  
  void DxbcCompiler::emitDclResourceRawStructured(const DxbcShaderInstruction& ins) {
    // dcl_resource_raw and dcl_uav_raw take one argument:
    //    (dst0) The resource register ID
    // dcl_resource_structured and dcl_uav_structured take two arguments:
    //    (dst0) The resource register ID
    //    (imm0) Structure stride, in bytes
    const uint32_t registerId = ins.dst[0].idx[0].offset;
    
    const bool isUav = ins.op == DxbcOpcode::DclUavRaw
                    || ins.op == DxbcOpcode::DclUavStructured;
    
    const bool isStructured = ins.op == DxbcOpcode::DclUavStructured
                           || ins.op == DxbcOpcode::DclResourceStructured;
    
    const DxbcScalarType sampledType = DxbcScalarType::Uint32;
    const uint32_t sampledTypeId = getScalarTypeId(sampledType);
    
    const DxbcImageInfo typeInfo = { spv::DimBuffer, 0, 0, isUav ? 2u : 1u, VK_IMAGE_VIEW_TYPE_MAX_ENUM };
    
    // Declare the resource type
    uint32_t resTypeId = 0;
    uint32_t varId     = 0;
    
    // Write back resource info
    DxbcResourceType resType = isStructured
      ? DxbcResourceType::Structured
      : DxbcResourceType::Raw;
    
    uint32_t resStride = isStructured
      ? ins.imm[0].u32
      : 0;
    
    uint32_t resAlign = isStructured
      ? (resStride & -resStride)
      : 16;
    
    // Compute the DXVK binding slot index for the resource.
    uint32_t bindingId = isUav
      ? computeUavBinding(m_programInfo.type(), registerId)
      : computeSrvBinding(m_programInfo.type(), registerId);
    
    // Test whether we should use a raw SSBO for this resource
    bool hasSparseFeedback = isUav
      ? m_analysis->uavInfos[registerId].sparseFeedback
      : m_analysis->srvInfos[registerId].sparseFeedback;

    bool useRawSsbo = m_moduleInfo.options.minSsboAlignment <= resAlign && !hasSparseFeedback;
    
    if (useRawSsbo) {
      uint32_t elemType   = getScalarTypeId(DxbcScalarType::Uint32);
      uint32_t arrayType  = m_module.defRuntimeArrayTypeUnique(elemType);
      uint32_t structType = m_module.defStructTypeUnique(1, &arrayType);
      uint32_t ptrType    = m_module.defPointerType(structType, spv::StorageClassStorageBuffer);

      resTypeId = m_module.defPointerType(elemType, spv::StorageClassStorageBuffer);
      varId     = m_module.newVar(ptrType, spv::StorageClassStorageBuffer);
      
      m_module.decorateArrayStride(arrayType, sizeof(uint32_t));
      m_module.decorate(structType, spv::DecorationBlock);
      m_module.memberDecorateOffset(structType, 0, 0);

      m_module.setDebugName(structType,
        str::format(isUav ? "u" : "t", registerId, "_t").c_str());
      m_module.setDebugMemberName(structType, 0, "m");
    } else {
      // Structured and raw buffers are represented as
      // texel buffers consisting of 32-bit integers.
      m_module.enableCapability(isUav
        ? spv::CapabilityImageBuffer
        : spv::CapabilitySampledBuffer);
      
      resTypeId = m_module.defImageType(sampledTypeId,
        typeInfo.dim, 0, typeInfo.array, typeInfo.ms, typeInfo.sampled,
        spv::ImageFormatR32ui);
      
      varId = m_module.newVar(
        m_module.defPointerType(resTypeId, spv::StorageClassUniformConstant),
        spv::StorageClassUniformConstant);
    }

    m_module.setDebugName(varId,
      str::format(isUav ? "u" : "t", registerId).c_str());
    
    m_module.decorateDescriptorSet(varId, 0);
    m_module.decorateBinding(varId, bindingId);
    
    if (isUav) {
      DxbcUav uav;
      uav.type          = resType;
      uav.imageInfo     = typeInfo;
      uav.varId         = varId;
      uav.ctrId         = 0;
      uav.sampledType   = sampledType;
      uav.sampledTypeId = sampledTypeId;
      uav.imageTypeId   = resTypeId;
      uav.structStride  = resStride;
      uav.coherence     = getUavCoherence(registerId, ins.controls.uavFlags());
      uav.isRawSsbo     = useRawSsbo;
      m_uavs.at(registerId) = uav;
    } else {
      DxbcShaderResource res;
      res.type          = resType;
      res.imageInfo     = typeInfo;
      res.varId         = varId;
      res.sampledType   = sampledType;
      res.sampledTypeId = sampledTypeId;
      res.imageTypeId   = resTypeId;
      res.colorTypeId   = resTypeId;
      res.depthTypeId   = 0;
      res.structStride  = resStride;
      res.isRawSsbo     = useRawSsbo;
      m_textures.at(registerId) = res;
    }
    
    // Store descriptor info for the shader interface
    DxvkBindingInfo binding = { };
    binding.descriptorType = useRawSsbo
      ? VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
      : (isUav ? VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER : VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER);
    binding.viewType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
    binding.resourceBinding = bindingId;
    binding.access = VK_ACCESS_SHADER_READ_BIT;

    if (isUav) {
      binding.access = m_analysis->uavInfos[registerId].accessFlags;

      if (!m_analysis->uavInfos[registerId].nonInvariantAccess)
        binding.accessOp = m_analysis->uavInfos[registerId].accessOp;
    }

    if (useRawSsbo || isUav) {
      if (!(binding.access & VK_ACCESS_SHADER_WRITE_BIT))
        m_module.decorate(varId, spv::DecorationNonWritable);
      if (!(binding.access & VK_ACCESS_SHADER_READ_BIT))
        m_module.decorate(varId, spv::DecorationNonReadable);
    }

    m_bindings.push_back(binding);

    // If supported, we'll be using raw access chains to access this
    if (!m_hasRawAccessChains && m_moduleInfo.options.supportsRawAccessChains) {
      m_module.enableExtension("SPV_NV_raw_access_chains");
      m_module.enableCapability(spv::CapabilityRawAccessChainsNV);

      m_hasRawAccessChains = true;
    }
  }
  
  
  void DxbcCompiler::emitDclThreadGroupSharedMemory(const DxbcShaderInstruction& ins) {
    // dcl_tgsm_raw takes two arguments:
    //    (dst0) The resource register ID
    //    (imm0) Block size, in bytes
    // dcl_tgsm_structured takes three arguments:
    //    (dst0) The resource register ID
    //    (imm0) Structure stride, in bytes
    //    (imm1) Structure count
    const bool isStructured = ins.op == DxbcOpcode::DclThreadGroupSharedMemoryStructured;
    
    const uint32_t regId = ins.dst[0].idx[0].offset;
    
    if (regId >= m_gRegs.size())
      m_gRegs.resize(regId + 1);
    
    const uint32_t elementStride = isStructured ? ins.imm[0].u32 : 0;
    const uint32_t elementCount  = isStructured ? ins.imm[1].u32 : ins.imm[0].u32;
    
    DxbcRegisterInfo varInfo;
    varInfo.type.ctype   = DxbcScalarType::Uint32;
    varInfo.type.ccount  = 1;
    varInfo.type.alength = isStructured
      ? elementCount * elementStride / 4
      : elementCount / 4;
    varInfo.sclass = spv::StorageClassWorkgroup;
    
    m_gRegs[regId].type = isStructured
      ? DxbcResourceType::Structured
      : DxbcResourceType::Raw;
    m_gRegs[regId].elementStride = elementStride;
    m_gRegs[regId].elementCount  = elementCount;
    m_gRegs[regId].varId = emitNewVariable(varInfo);
    
    m_module.setDebugName(m_gRegs[regId].varId,
      str::format("g", regId).c_str());
  }
  
  
  void DxbcCompiler::emitDclGsInputPrimitive(const DxbcShaderInstruction& ins) {
    // The input primitive type is stored within in the
    // control bits of the opcode token. In SPIR-V, we
    // have to define an execution mode.
    const auto mode = [&] {
      switch (ins.controls.primitive()) {
        case DxbcPrimitive::Point:       return std::make_pair(VK_PRIMITIVE_TOPOLOGY_POINT_LIST,                   spv::ExecutionModeInputPoints);
        case DxbcPrimitive::Line:        return std::make_pair(VK_PRIMITIVE_TOPOLOGY_LINE_LIST,                    spv::ExecutionModeInputLines);
        case DxbcPrimitive::Triangle:    return std::make_pair(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,                spv::ExecutionModeTriangles);
        case DxbcPrimitive::LineAdj:     return std::make_pair(VK_PRIMITIVE_TOPOLOGY_LINE_LIST_WITH_ADJACENCY,     spv::ExecutionModeInputLinesAdjacency);
        case DxbcPrimitive::TriangleAdj: return std::make_pair(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST_WITH_ADJACENCY, spv::ExecutionModeInputTrianglesAdjacency);
        default: throw DxvkError("DxbcCompiler: Unsupported primitive type");
      }
    }();

    m_gs.inputPrimitive = ins.controls.primitive();
    m_module.setExecutionMode(m_entryPointId, mode.second);
    m_inputTopology = mode.first;
    
    emitDclInputArray(primitiveVertexCount(m_gs.inputPrimitive));
  }
  
  
  void DxbcCompiler::emitDclGsOutputTopology(const DxbcShaderInstruction& ins) {
    // The input primitive topology is stored within in the
    // control bits of the opcode token. In SPIR-V, we have
    // to define an execution mode.
    auto mode = [&] {
      switch (ins.controls.primitiveTopology()) {
        case DxbcPrimitiveTopology::PointList:     return std::make_pair(VK_PRIMITIVE_TOPOLOGY_POINT_LIST,    spv::ExecutionModeOutputPoints);
        case DxbcPrimitiveTopology::LineStrip:     return std::make_pair(VK_PRIMITIVE_TOPOLOGY_LINE_LIST,     spv::ExecutionModeOutputLineStrip);
        case DxbcPrimitiveTopology::TriangleStrip: return std::make_pair(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, spv::ExecutionModeOutputTriangleStrip);
        default: throw DxvkError("DxbcCompiler: Unsupported primitive topology");
      }
    }();
    
    m_outputTopology = mode.first;
    m_module.setExecutionMode(m_entryPointId, mode.second);
  }
  
  
  void DxbcCompiler::emitDclMaxOutputVertexCount(const DxbcShaderInstruction& ins) {
    // dcl_max_output_vertex_count has one operand:
    //    (imm0) The maximum number of vertices
    m_gs.outputVertexCount = ins.imm[0].u32;
    
    m_module.setOutputVertices(m_entryPointId, m_gs.outputVertexCount);
  }
  
  
  void DxbcCompiler::emitDclInputControlPointCount(const DxbcShaderInstruction& ins) {
    // dcl_input_control_points has the control point
    // count embedded within the opcode token.
    if (m_programInfo.type() == DxbcProgramType::HullShader) {
      m_hs.vertexCountIn = ins.controls.controlPointCount();
      
      emitDclInputArray(m_hs.vertexCountIn);    
    } else {
      m_ds.vertexCountIn = ins.controls.controlPointCount();
      
      m_ds.inputPerPatch  = emitTessInterfacePerPatch (spv::StorageClassInput);
      m_ds.inputPerVertex = emitTessInterfacePerVertex(spv::StorageClassInput, m_ds.vertexCountIn);
    }
  }
  
  
  void DxbcCompiler::emitDclOutputControlPointCount(const DxbcShaderInstruction& ins) {
    // dcl_output_control_points has the control point
    // count embedded within the opcode token.
    m_hs.vertexCountOut = ins.controls.controlPointCount();
    
    m_hs.outputPerPatch  = emitTessInterfacePerPatch(spv::StorageClassPrivate);
    m_hs.outputPerVertex = emitTessInterfacePerVertex(spv::StorageClassOutput, m_hs.vertexCountOut);
    
    m_module.setOutputVertices(m_entryPointId, m_hs.vertexCountOut);
  }
  
  
  void DxbcCompiler::emitDclHsMaxTessFactor(const DxbcShaderInstruction& ins) {
    m_hs.maxTessFactor = ins.imm[0].f32;
  }
  
  
  void DxbcCompiler::emitDclTessDomain(const DxbcShaderInstruction& ins) {
    auto mode = [&] {
      switch (ins.controls.tessDomain()) {
        case DxbcTessDomain::Isolines:  return std::make_pair(VK_PRIMITIVE_TOPOLOGY_LINE_LIST,     spv::ExecutionModeIsolines);
        case DxbcTessDomain::Triangles: return std::make_pair(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, spv::ExecutionModeTriangles);
        case DxbcTessDomain::Quads:     return std::make_pair(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, spv::ExecutionModeQuads);
        default: throw DxvkError("Dxbc: Invalid tess domain");
      }
    }();
    
    m_outputTopology = mode.first;
    m_module.setExecutionMode(m_entryPointId, mode.second);
  }
  
  
  void DxbcCompiler::emitDclTessPartitioning(const DxbcShaderInstruction& ins) {
    const spv::ExecutionMode executionMode = [&] {
      switch (ins.controls.tessPartitioning()) {
        case DxbcTessPartitioning::Pow2:
        case DxbcTessPartitioning::Integer:   return spv::ExecutionModeSpacingEqual;
        case DxbcTessPartitioning::FractOdd:  return spv::ExecutionModeSpacingFractionalOdd;
        case DxbcTessPartitioning::FractEven: return spv::ExecutionModeSpacingFractionalEven;
        default: throw DxvkError("Dxbc: Invalid tess partitioning");
      }
    }();
    
    m_module.setExecutionMode(m_entryPointId, executionMode);
  }
  
  
  void DxbcCompiler::emitDclTessOutputPrimitive(const DxbcShaderInstruction& ins) {
    switch (ins.controls.tessOutputPrimitive()) {
      case DxbcTessOutputPrimitive::Point:
        m_module.setExecutionMode(m_entryPointId, spv::ExecutionModePointMode);
        break;
        
      case DxbcTessOutputPrimitive::Line:
        break;
        
      case DxbcTessOutputPrimitive::TriangleCw:
        m_module.setExecutionMode(m_entryPointId, spv::ExecutionModeVertexOrderCw);
        break;
        
      case DxbcTessOutputPrimitive::TriangleCcw:
        m_module.setExecutionMode(m_entryPointId, spv::ExecutionModeVertexOrderCcw);
        break;
      
      default:
        throw DxvkError("Dxbc: Invalid tess output primitive");
    }
  }
  
  
  void DxbcCompiler::emitDclThreadGroup(const DxbcShaderInstruction& ins) {
    // dcl_thread_group has three operands:
    //    (imm0) Number of threads in X dimension
    //    (imm1) Number of threads in Y dimension
    //    (imm2) Number of threads in Z dimension
    m_cs.workgroupSizeX = ins.imm[0].u32;
    m_cs.workgroupSizeY = ins.imm[1].u32;
    m_cs.workgroupSizeZ = ins.imm[2].u32;

    m_module.setLocalSize(m_entryPointId,
      ins.imm[0].u32, ins.imm[1].u32, ins.imm[2].u32);
  }
  
  
  void DxbcCompiler::emitDclGsInstanceCount(const DxbcShaderInstruction& ins) {
    // dcl_gs_instance_count has one operand:
    //    (imm0) Number of geometry shader invocations
    m_module.setInvocations(m_entryPointId, ins.imm[0].u32);
    m_gs.invocationCount = ins.imm[0].u32;
  }
  
  
  uint32_t DxbcCompiler::emitDclUavCounter(uint32_t regId) {
    // Declare a structure type which holds the UAV counter
    if (m_uavCtrStructType == 0) {
      const uint32_t t_u32    = m_module.defIntType(32, 0);
      const uint32_t t_struct = m_module.defStructTypeUnique(1, &t_u32);
      
      m_module.decorate(t_struct, spv::DecorationBlock);
      m_module.memberDecorateOffset(t_struct, 0, 0);
      
      m_module.setDebugName      (t_struct, "uav_meta");
      m_module.setDebugMemberName(t_struct, 0, "ctr");
      
      m_uavCtrStructType  = t_struct;
      m_uavCtrPointerType = m_module.defPointerType(
        t_struct, spv::StorageClassStorageBuffer);
    }
    
    // Declare the buffer variable
    const uint32_t varId = m_module.newVar(
      m_uavCtrPointerType, spv::StorageClassStorageBuffer);
    
    m_module.setDebugName(varId,
      str::format("u", regId, "_meta").c_str());
    
    uint32_t bindingId = computeUavCounterBinding(
      m_programInfo.type(), regId);
    
    m_module.decorateDescriptorSet(varId, 0);
    m_module.decorateBinding(varId, bindingId);
    
    // Declare the storage buffer binding
    DxvkBindingInfo binding = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER };
    binding.resourceBinding = bindingId;
    binding.viewType = VK_IMAGE_VIEW_TYPE_MAX_ENUM;
    binding.access = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    m_bindings.push_back(binding);

    return varId;
  }
  
  
  void DxbcCompiler::emitDclImmediateConstantBuffer(const DxbcShaderInstruction& ins) {
    if (m_icbArray)
      throw DxvkError("DxbcCompiler: Immediate constant buffer already declared");
    
    if ((ins.customDataSize & 0x3) != 0)
      throw DxvkError("DxbcCompiler: Immediate constant buffer size not a multiple of four DWORDs");

    // A lot of the time we'll be dealing with a scalar or vec2
    // array here, there's no reason to emit all those zeroes.
    uint32_t componentCount = 1u;

    for (uint32_t i = 0; i < ins.customDataSize; i += 4u) {
      for (uint32_t c = componentCount; c < 4u; c++) {
        if (ins.customData[i + c])
          componentCount = c + 1u;
      }

      if (componentCount == 4u)
        break;
    }

    uint32_t vectorCount = (ins.customDataSize / 4u);
    uint32_t dwordCount = vectorCount * componentCount;

    if (dwordCount <= Icb_MaxBakedDwords) {
      this->emitDclImmediateConstantBufferBaked(
        ins.customDataSize, ins.customData, componentCount);
    } else {
      this->emitDclImmediateConstantBufferUbo(
        ins.customDataSize, ins.customData, componentCount);
    }
  }


  void DxbcCompiler::emitDclImmediateConstantBufferBaked(
          uint32_t                dwordCount,
    const uint32_t*               dwordArray,
          uint32_t                componentCount) {
    // Declare individual vector constants as 4x32-bit vectors
    small_vector<uint32_t, Icb_MaxBakedDwords> vectorIds;
    
    DxbcVectorType vecType;
    vecType.ctype  = DxbcScalarType::Uint32;
    vecType.ccount = componentCount;
    
    uint32_t vectorTypeId = getVectorTypeId(vecType);
    
    for (uint32_t i = 0; i < dwordCount; i += 4u) {
      std::array<uint32_t, 4> scalarIds = { };

      for (uint32_t c = 0; c < componentCount; c++)
        scalarIds[c] = m_module.constu32(dwordArray[i + c]);

      uint32_t id = scalarIds[0];

      if (componentCount > 1u)
        id = m_module.constComposite(vectorTypeId, componentCount, scalarIds.data());

      vectorIds.push_back(id);
    }

    // Pad array with one entry of zeroes so that we can
    // handle out-of-bounds accesses more conveniently.
    vectorIds.push_back(emitBuildZeroVector(vecType).id);

    // Declare the array that contains all the vectors
    DxbcArrayType arrInfo;
    arrInfo.ctype   = DxbcScalarType::Uint32;
    arrInfo.ccount  = componentCount;
    arrInfo.alength = vectorIds.size();

    uint32_t arrayTypeId = getArrayTypeId(arrInfo);
    uint32_t arrayId = m_module.constComposite(
      arrayTypeId, vectorIds.size(), vectorIds.data());

    // Declare the variable that will hold the constant
    // data and initialize it with the constant array.
    uint32_t pointerTypeId = m_module.defPointerType(
      arrayTypeId, spv::StorageClassPrivate);

    m_icbArray = m_module.newVarInit(
      pointerTypeId, spv::StorageClassPrivate,
      arrayId);

    m_module.setDebugName(m_icbArray, "icb");
    m_module.decorate(m_icbArray, spv::DecorationNonWritable);

    m_icbComponents = componentCount;
    m_icbSize = dwordCount / 4u;
  }
  
  
  void DxbcCompiler::emitDclImmediateConstantBufferUbo(
          uint32_t                dwordCount,
    const uint32_t*               dwordArray,
          uint32_t                componentCount) {
    uint32_t vectorCount = dwordCount / 4u;

    // Tightly pack vec2 or scalar arrays if possible. Don't bother with
    // vec3 since we'd rather have properly vectorized loads in that case.
    if (m_moduleInfo.options.supportsTightIcbPacking && componentCount <= 2u)
      m_icbComponents = componentCount;
    else
      m_icbComponents = 4u;

    // Immediate constant buffer can be read out of bounds, declare
    // it with the maximum possible size and rely on robustness.
    this->emitDclConstantBufferVar(Icb_BindingSlotId, 4096u, m_icbComponents, "icb");

    m_icbData.reserve(vectorCount * componentCount);

    for (uint32_t i = 0; i < dwordCount; i += 4u) {
      for (uint32_t c = 0; c < m_icbComponents; c++)
        m_icbData.push_back(dwordArray[i + c]);
    }

    m_icbSize = vectorCount;
  }


  void DxbcCompiler::emitCustomData(const DxbcShaderInstruction& ins) {
    switch (ins.customDataType) {
      case DxbcCustomDataClass::ImmConstBuf:
        return emitDclImmediateConstantBuffer(ins);
      
      default:
        Logger::warn(str::format(
          "DxbcCompiler: Unsupported custom data block: ",
          ins.customDataType));
    }
  }
  
  
  void DxbcCompiler::emitVectorAlu(const DxbcShaderInstruction& ins) {
    std::array<DxbcRegisterValue, DxbcMaxOperandCount> src;
    
    for (uint32_t i = 0; i < ins.srcCount; i++)
      src.at(i) = emitRegisterLoad(ins.src[i], ins.dst[0].mask);
    
    DxbcRegisterValue dst;
    dst.type.ctype  = ins.dst[0].dataType;
    dst.type.ccount = ins.dst[0].mask.popCount();

    if (isDoubleType(ins.dst[0].dataType))
      dst.type.ccount /= 2;
    
    const uint32_t typeId = getVectorTypeId(dst.type);
    
    switch (ins.op) {
      /////////////////////
      // Move instructions
      case DxbcOpcode::Mov:
      case DxbcOpcode::DMov:
        dst.id = src.at(0).id;
        break;
        
      /////////////////////////////////////
      // ALU operations on float32 numbers
      case DxbcOpcode::Add:
      case DxbcOpcode::DAdd:
        dst.id = m_module.opFAdd(typeId,
          src.at(0).id, src.at(1).id);
        break;
        
      case DxbcOpcode::Div:
      case DxbcOpcode::DDiv:
        dst.id = m_module.opFDiv(typeId,
          src.at(0).id, src.at(1).id);
        break;
      
      case DxbcOpcode::Exp:
        dst.id = m_module.opExp2(
          typeId, src.at(0).id);
        break;
      
      case DxbcOpcode::Frc:
        dst.id = m_module.opFract(
          typeId, src.at(0).id);
        break;
      
      case DxbcOpcode::Log:
        dst.id = m_module.opLog2(
          typeId, src.at(0).id);
        break;
      
      case DxbcOpcode::Mad:
      case DxbcOpcode::DFma:
        if (ins.controls.precise()) {
          // FXC only emits precise mad if the shader explicitly uses
          // the HLSL mad()/fma() intrinsics, let's preserve that.
          dst.id = m_module.opFFma(typeId,
            src.at(0).id, src.at(1).id, src.at(2).id);
        } else {
          dst.id = m_module.opFMul(typeId, src.at(0).id, src.at(1).id);
          dst.id = m_module.opFAdd(typeId, dst.id, src.at(2).id);
        }
        break;
      
      case DxbcOpcode::Max:
      case DxbcOpcode::DMax:
        dst.id = m_module.opNMax(typeId,
          src.at(0).id, src.at(1).id);
        break;
      
      case DxbcOpcode::Min:
      case DxbcOpcode::DMin:
        dst.id = m_module.opNMin(typeId,
          src.at(0).id, src.at(1).id);
        break;
      
      case DxbcOpcode::Mul:
      case DxbcOpcode::DMul:
        dst.id = m_module.opFMul(typeId,
          src.at(0).id, src.at(1).id);
        break;
      
      case DxbcOpcode::Rcp:
        dst.id = m_module.opFDiv(typeId,
          emitBuildConstVecf32(
            1.0f, 1.0f, 1.0f, 1.0f,
            ins.dst[0].mask).id,
          src.at(0).id);
        break;
      
      case DxbcOpcode::DRcp:
        dst.id = m_module.opFDiv(typeId,
          emitBuildConstVecf64(1.0, 1.0,
            ins.dst[0].mask).id,
          src.at(0).id);
        break;
      
      case DxbcOpcode::RoundNe:
        dst.id = m_module.opRoundEven(
          typeId, src.at(0).id);
        break;
      
      case DxbcOpcode::RoundNi:
        dst.id = m_module.opFloor(
          typeId, src.at(0).id);
        break;
      
      case DxbcOpcode::RoundPi:
        dst.id = m_module.opCeil(
          typeId, src.at(0).id);
        break;
      
      case DxbcOpcode::RoundZ:
        dst.id = m_module.opTrunc(
          typeId, src.at(0).id);
        break;
      
      case DxbcOpcode::Rsq:
        dst.id = m_module.opInverseSqrt(
          typeId, src.at(0).id);
        break;
      
      case DxbcOpcode::Sqrt:
        dst.id = m_module.opSqrt(
          typeId, src.at(0).id);
        break;
      
      /////////////////////////////////////
      // ALU operations on signed integers
      case DxbcOpcode::IAdd:
        dst.id = m_module.opIAdd(typeId,
          src.at(0).id, src.at(1).id);
        break;
      
      case DxbcOpcode::IMad:
      case DxbcOpcode::UMad:
        dst.id = m_module.opIAdd(typeId,
          m_module.opIMul(typeId,
            src.at(0).id, src.at(1).id),
          src.at(2).id);
        break;
      
      case DxbcOpcode::IMax:
        dst.id = m_module.opSMax(typeId,
          src.at(0).id, src.at(1).id);
        break;
        
      case DxbcOpcode::IMin:
        dst.id = m_module.opSMin(typeId,
          src.at(0).id, src.at(1).id);
        break;
        
      case DxbcOpcode::INeg:
        dst.id = m_module.opSNegate(
          typeId, src.at(0).id);
        break;
      
      ///////////////////////////////////////
      // ALU operations on unsigned integers
      case DxbcOpcode::UMax:
        dst.id = m_module.opUMax(typeId,
          src.at(0).id, src.at(1).id);
        break;
        
      case DxbcOpcode::UMin:
        dst.id = m_module.opUMin(typeId,
          src.at(0).id, src.at(1).id);
        break;
      
      ///////////////////////////////////////
      // Bit operations on unsigned integers
      case DxbcOpcode::And:
        dst.id = m_module.opBitwiseAnd(typeId,
          src.at(0).id, src.at(1).id);
        break;
        
      case DxbcOpcode::Not:
        dst.id = m_module.opNot(
          typeId, src.at(0).id);
        break;
        
      case DxbcOpcode::Or:
        dst.id = m_module.opBitwiseOr(typeId,
          src.at(0).id, src.at(1).id);
        break;
      
      case DxbcOpcode::Xor:
        dst.id = m_module.opBitwiseXor(typeId,
          src.at(0).id, src.at(1).id);
        break;
      
      case DxbcOpcode::CountBits:
        dst.id = m_module.opBitCount(
          typeId, src.at(0).id);
        break;
      
      case DxbcOpcode::BfRev:
        dst.id = m_module.opBitReverse(
          typeId, src.at(0).id);
        break;
      
      ///////////////////////////
      // Conversion instructions
      case DxbcOpcode::ItoF:
        dst.id = m_module.opConvertStoF(
          typeId, src.at(0).id);
        break;
      
      case DxbcOpcode::UtoF:
        dst.id = m_module.opConvertUtoF(
          typeId, src.at(0).id);
        break;
      
      case DxbcOpcode::FtoI:
        dst.id = m_module.opConvertFtoS(
          typeId, src.at(0).id);
        break;
      
      case DxbcOpcode::FtoU:
        dst.id = m_module.opConvertFtoU(
          typeId, src.at(0).id);
        break;
      
      default:
        Logger::warn(str::format(
          "DxbcCompiler: Unhandled instruction: ",
          ins.op));
        return;
    }
    
    if (ins.controls.precise() || m_precise)
      m_module.decorate(dst.id, spv::DecorationNoContraction);
    
    // Store computed value
    dst = emitDstOperandModifiers(dst, ins.modifiers);
    emitRegisterStore(ins.dst[0], dst);
  }
  
  
  void DxbcCompiler::emitVectorCmov(const DxbcShaderInstruction& ins) {
    // movc and swapc have the following operands:
    //    (dst0) The first destination register
    //    (dst1) The second destination register (swapc only)
    //    (src0) The condition vector
    //    (src1) Vector to select from if the condition is not 0
    //    (src2) Vector to select from if the condition is 0
    DxbcRegMask condMask = ins.dst[0].mask;

    if (ins.dst[0].dataType == DxbcScalarType::Float64) {
      condMask = DxbcRegMask(
        condMask[0] && condMask[1],
        condMask[2] && condMask[3],
        false, false);
    }
    
    const DxbcRegisterValue condition   = emitRegisterLoad(ins.src[0], condMask);
    const DxbcRegisterValue selectTrue  = emitRegisterLoad(ins.src[1], ins.dst[0].mask);
    const DxbcRegisterValue selectFalse = emitRegisterLoad(ins.src[2], ins.dst[0].mask);
    
    uint32_t componentCount = condMask.popCount();
    
    // We'll compare against a vector of zeroes to generate a
    // boolean vector, which in turn will be used by OpSelect
    uint32_t zeroType = m_module.defIntType(32, 0);
    uint32_t boolType = m_module.defBoolType();
    
    uint32_t zero = m_module.constu32(0);
    
    if (componentCount > 1) {
      zeroType = m_module.defVectorType(zeroType, componentCount);
      boolType = m_module.defVectorType(boolType, componentCount);
      
      const std::array<uint32_t, 4> zeroVec = { zero, zero, zero, zero };
      zero = m_module.constComposite(zeroType, componentCount, zeroVec.data());
    }
    
    // In case of swapc, the second destination operand receives
    // the output that a cmov instruction would normally get
    const uint32_t trueIndex = ins.op == DxbcOpcode::Swapc ? 1 : 0;
    
    for (uint32_t i = 0; i < ins.dstCount; i++) {
      DxbcRegisterValue result;
      result.type.ctype  = ins.dst[i].dataType;
      result.type.ccount = componentCount;
      result.id = m_module.opSelect(
        getVectorTypeId(result.type),
        m_module.opINotEqual(boolType, condition.id, zero),
        i == trueIndex ? selectTrue.id : selectFalse.id,
        i != trueIndex ? selectTrue.id : selectFalse.id);
      
      result = emitDstOperandModifiers(result, ins.modifiers);
      emitRegisterStore(ins.dst[i], result);
    }
  }
  
  void DxbcCompiler::emitVectorCmp(const DxbcShaderInstruction& ins) {
    // Compare instructions have three operands:
    //    (dst0) The destination register
    //    (src0) The first vector to compare
    //    (src1) The second vector to compare
    uint32_t componentCount = ins.dst[0].mask.popCount();

    // For 64-bit operations, we'll return a 32-bit
    // vector, so we have to adjust the read mask
    DxbcRegMask srcMask = ins.dst[0].mask;

    if (isDoubleType(ins.src[0].dataType)) {
      srcMask = DxbcRegMask(
        componentCount > 0, componentCount > 0,
        componentCount > 1, componentCount > 1);
    }

    const std::array<DxbcRegisterValue, 2> src = {
      emitRegisterLoad(ins.src[0], srcMask),
      emitRegisterLoad(ins.src[1], srcMask),
    };
    
    // Condition, which is a boolean vector used
    // to select between the ~0u and 0u vectors.
    uint32_t condition     = 0;
    uint32_t conditionType = m_module.defBoolType();
    
    if (componentCount > 1)
      conditionType = m_module.defVectorType(conditionType, componentCount);
    
    bool invert = false;

    switch (ins.op) {
      case DxbcOpcode::Ne:
      case DxbcOpcode::DNe:
        invert = true;
        [[fallthrough]];

      case DxbcOpcode::Eq:
      case DxbcOpcode::DEq:
        condition = m_module.opFOrdEqual(
          conditionType, src.at(0).id, src.at(1).id);
        break;
      
      case DxbcOpcode::Ge:
      case DxbcOpcode::DGe:
        condition = m_module.opFOrdGreaterThanEqual(
          conditionType, src.at(0).id, src.at(1).id);
        break;
      
      case DxbcOpcode::Lt:
      case DxbcOpcode::DLt:
        condition = m_module.opFOrdLessThan(
          conditionType, src.at(0).id, src.at(1).id);
        break;
      
      case DxbcOpcode::IEq:
        condition = m_module.opIEqual(
          conditionType, src.at(0).id, src.at(1).id);
        break;
      
      case DxbcOpcode::IGe:
        condition = m_module.opSGreaterThanEqual(
          conditionType, src.at(0).id, src.at(1).id);
        break;
      
      case DxbcOpcode::ILt:
        condition = m_module.opSLessThan(
          conditionType, src.at(0).id, src.at(1).id);
        break;
      
      case DxbcOpcode::INe:
        condition = m_module.opINotEqual(
          conditionType, src.at(0).id, src.at(1).id);
        break;
      
      case DxbcOpcode::UGe:
        condition = m_module.opUGreaterThanEqual(
          conditionType, src.at(0).id, src.at(1).id);
        break;
      
      case DxbcOpcode::ULt:
        condition = m_module.opULessThan(
          conditionType, src.at(0).id, src.at(1).id);
        break;
      
      default:
        Logger::warn(str::format(
          "DxbcCompiler: Unhandled instruction: ",
          ins.op));
        return;
    }
    
    // Generate constant vectors for selection
    uint32_t sFalse = m_module.constu32( 0u);
    uint32_t sTrue  = m_module.constu32(~0u);
    
    DxbcRegisterValue result;
    result.type.ctype  = DxbcScalarType::Uint32;
    result.type.ccount = componentCount;
    
    const uint32_t typeId = getVectorTypeId(result.type);
    
    if (componentCount > 1) {
      const std::array<uint32_t, 4> vFalse = { sFalse, sFalse, sFalse, sFalse };
      const std::array<uint32_t, 4> vTrue  = { sTrue,  sTrue,  sTrue,  sTrue  };
      
      sFalse = m_module.constComposite(typeId, componentCount, vFalse.data());
      sTrue  = m_module.constComposite(typeId, componentCount, vTrue .data());
    }
    
    if (invert)
      std::swap(sFalse, sTrue);

    // Perform component-wise mask selection
    // based on the condition evaluated above.
    result.id = m_module.opSelect(
      typeId, condition, sTrue, sFalse);
    
    emitRegisterStore(ins.dst[0], result);
  }
  
  
  void DxbcCompiler::emitVectorDeriv(const DxbcShaderInstruction& ins) {
    // Derivative instructions have two operands:
    //    (dst0) Destination register for the derivative
    //    (src0) The operand to compute the derivative of
    DxbcRegisterValue value = emitRegisterLoad(ins.src[0], ins.dst[0].mask);
    const uint32_t typeId = getVectorTypeId(value.type);
    
    switch (ins.op) {
      case DxbcOpcode::DerivRtx:
        value.id = m_module.opDpdx(typeId, value.id);
        break;
        
      case DxbcOpcode::DerivRty:
        value.id = m_module.opDpdy(typeId, value.id);
        break;
        
      case DxbcOpcode::DerivRtxCoarse:
        value.id = m_module.opDpdxCoarse(typeId, value.id);
        break;
        
      case DxbcOpcode::DerivRtyCoarse:
        value.id = m_module.opDpdyCoarse(typeId, value.id);
        break;
        
      case DxbcOpcode::DerivRtxFine:
        value.id = m_module.opDpdxFine(typeId, value.id);
        break;
        
      case DxbcOpcode::DerivRtyFine:
        value.id = m_module.opDpdyFine(typeId, value.id);
        break;
      
      default:
        Logger::warn(str::format(
          "DxbcCompiler: Unhandled instruction: ",
          ins.op));
        return;
    }
    
    value = emitDstOperandModifiers(value, ins.modifiers);
    emitRegisterStore(ins.dst[0], value);
  }
  
  
  void DxbcCompiler::emitVectorDot(const DxbcShaderInstruction& ins) {
    const DxbcRegMask srcMask(true,
      ins.op >= DxbcOpcode::Dp2,
      ins.op >= DxbcOpcode::Dp3,
      ins.op >= DxbcOpcode::Dp4);
    
    const std::array<DxbcRegisterValue, 2> src = {
      emitRegisterLoad(ins.src[0], srcMask),
      emitRegisterLoad(ins.src[1], srcMask),
    };
    
    DxbcRegisterValue dst;
    dst.type.ctype  = ins.dst[0].dataType;
    dst.type.ccount = 1;
    dst.id = 0;

    uint32_t componentType = getVectorTypeId(dst.type);
    uint32_t componentCount = srcMask.popCount();

    for (uint32_t i = 0; i < componentCount; i++) {
      if (dst.id) {
        dst.id = m_module.opFFma(componentType,
          m_module.opCompositeExtract(componentType, src.at(0).id, 1, &i),
          m_module.opCompositeExtract(componentType, src.at(1).id, 1, &i),
          dst.id);
      } else {
        dst.id = m_module.opFMul(componentType,
          m_module.opCompositeExtract(componentType, src.at(0).id, 1, &i),
          m_module.opCompositeExtract(componentType, src.at(1).id, 1, &i));
      }

      // Unconditionally mark as precise since the exact order of operation
      // matters for some games, even if the instruction itself is not marked
      // as precise.
      m_module.decorate(dst.id, spv::DecorationNoContraction);
    }

    dst = emitDstOperandModifiers(dst, ins.modifiers);
    emitRegisterStore(ins.dst[0], dst);
  }
  
  
  void DxbcCompiler::emitVectorIdiv(const DxbcShaderInstruction& ins) {
    // udiv has four operands:
    //    (dst0) Quotient destination register
    //    (dst1) Remainder destination register
    //    (src0) The first vector to compare
    //    (src1) The second vector to compare
    if (ins.dst[0].type == DxbcOperandType::Null
     && ins.dst[1].type == DxbcOperandType::Null)
      return;
    
    // FIXME support this if applications require it
    if (ins.dst[0].type != DxbcOperandType::Null
     && ins.dst[1].type != DxbcOperandType::Null
     && ins.dst[0].mask != ins.dst[1].mask) {
      Logger::warn("DxbcCompiler: Idiv with different destination masks not supported");
      return;
    }
    
    // Load source operands as integers with the
    // mask of one non-NULL destination operand
    const DxbcRegMask srcMask =
      ins.dst[0].type != DxbcOperandType::Null
        ? ins.dst[0].mask
        : ins.dst[1].mask;
    
    const std::array<DxbcRegisterValue, 2> src = {
      emitRegisterLoad(ins.src[0], srcMask),
      emitRegisterLoad(ins.src[1], srcMask),
    };
    
    // Division by zero will return 0xffffffff for both results
    auto bvecId = getVectorTypeId({ DxbcScalarType::Bool, srcMask.popCount() });

    DxbcRegisterValue const0  = emitBuildConstVecu32( 0u,  0u,  0u,  0u, srcMask);
    DxbcRegisterValue constff = emitBuildConstVecu32(~0u, ~0u, ~0u, ~0u, srcMask);

    uint32_t cmpValue = m_module.opINotEqual(bvecId, src.at(1).id, const0.id);

    // Compute results only if the destination
    // operands are not NULL.
    if (ins.dst[0].type != DxbcOperandType::Null) {
      DxbcRegisterValue quotient;
      quotient.type.ctype  = ins.dst[0].dataType;
      quotient.type.ccount = ins.dst[0].mask.popCount();
      
      quotient.id = m_module.opUDiv(
        getVectorTypeId(quotient.type),
        src.at(0).id, src.at(1).id);

      quotient.id = m_module.opSelect(
        getVectorTypeId(quotient.type),
        cmpValue, quotient.id, constff.id);
      
      quotient = emitDstOperandModifiers(quotient, ins.modifiers);
      emitRegisterStore(ins.dst[0], quotient);
    }
    
    if (ins.dst[1].type != DxbcOperandType::Null) {
      DxbcRegisterValue remainder;
      remainder.type.ctype  = ins.dst[1].dataType;
      remainder.type.ccount = ins.dst[1].mask.popCount();
      
      remainder.id = m_module.opUMod(
        getVectorTypeId(remainder.type),
        src.at(0).id, src.at(1).id);

      remainder.id = m_module.opSelect(
        getVectorTypeId(remainder.type),
        cmpValue, remainder.id, constff.id);
      
      remainder = emitDstOperandModifiers(remainder, ins.modifiers);
      emitRegisterStore(ins.dst[1], remainder);
    }
  }
  
  
  void DxbcCompiler::emitVectorImul(const DxbcShaderInstruction& ins) {
    // imul and umul have four operands:
    //    (dst0) High destination register
    //    (dst1) Low destination register
    //    (src0) The first vector to compare
    //    (src1) The second vector to compare
    if (ins.dst[0].type == DxbcOperandType::Null) {
      if (ins.dst[1].type == DxbcOperandType::Null)
        return;
      
      // If dst0 is NULL, this instruction behaves just
      // like any other three-operand ALU instruction
      const std::array<DxbcRegisterValue, 2> src = {
        emitRegisterLoad(ins.src[0], ins.dst[1].mask),
        emitRegisterLoad(ins.src[1], ins.dst[1].mask),
      };
      
      DxbcRegisterValue result;
      result.type.ctype  = ins.dst[1].dataType;
      result.type.ccount = ins.dst[1].mask.popCount();
      result.id = m_module.opIMul(
        getVectorTypeId(result.type),
        src.at(0).id, src.at(1).id);
      
      result = emitDstOperandModifiers(result, ins.modifiers);
      emitRegisterStore(ins.dst[1], result);
    } else {
      // TODO implement this
      Logger::warn("DxbcCompiler: Extended Imul not yet supported");
    }
  }
  
  
  void DxbcCompiler::emitVectorMsad(const DxbcShaderInstruction& ins) {
    // msad has four operands:
    //    (dst0) Destination
    //    (src0) Reference (packed uint8)
    //    (src1) Source (packed uint8)
    //    (src2) Accumulator
    DxbcRegisterValue refReg = emitRegisterLoad(ins.src[0], ins.dst[0].mask);
    DxbcRegisterValue srcReg = emitRegisterLoad(ins.src[1], ins.dst[0].mask);
    DxbcRegisterValue result = emitRegisterLoad(ins.src[2], ins.dst[0].mask);
    
    auto typeId = getVectorTypeId(result.type);
    auto bvecId = getVectorTypeId({ DxbcScalarType::Bool, result.type.ccount });

    for (uint32_t i = 0; i < 4; i++) {
      auto shift = m_module.constu32(8 * i);
      auto count = m_module.constu32(8);

      auto ref = m_module.opBitFieldUExtract(typeId, refReg.id, shift, count);
      auto src = m_module.opBitFieldUExtract(typeId, srcReg.id, shift, count);

      auto zero = emitBuildConstVecu32(0, 0, 0, 0, ins.dst[0].mask);
      auto mask = m_module.opINotEqual(bvecId, ref, zero.id);

      auto diff = m_module.opSAbs(typeId, m_module.opISub(typeId, ref, src));
      result.id = m_module.opSelect(typeId, mask, m_module.opIAdd(typeId, result.id, diff), result.id);
    }

    result = emitDstOperandModifiers(result, ins.modifiers);
    emitRegisterStore(ins.dst[0], result);
  }


  void DxbcCompiler::emitVectorShift(const DxbcShaderInstruction& ins) {
    // Shift operations have three operands:
    //    (dst0) The destination register
    //    (src0) The register to shift
    //    (src1) The shift amount (scalar)
    DxbcRegisterValue shiftReg = emitRegisterLoad(ins.src[0], ins.dst[0].mask);
    DxbcRegisterValue countReg = emitRegisterLoad(ins.src[1], ins.dst[0].mask);

    if (ins.src[1].type != DxbcOperandType::Imm32)
      countReg = emitRegisterMaskBits(countReg, 0x1F);
    
    if (countReg.type.ccount == 1)
      countReg = emitRegisterExtend(countReg, shiftReg.type.ccount);
    
    DxbcRegisterValue result;
    result.type.ctype  = ins.dst[0].dataType;
    result.type.ccount = ins.dst[0].mask.popCount();
    
    switch (ins.op) {
      case DxbcOpcode::IShl:
        result.id = m_module.opShiftLeftLogical(
          getVectorTypeId(result.type),
          shiftReg.id, countReg.id);
        break;
      
      case DxbcOpcode::IShr:
        result.id = m_module.opShiftRightArithmetic(
          getVectorTypeId(result.type),
          shiftReg.id, countReg.id);
        break;
      
      case DxbcOpcode::UShr:
        result.id = m_module.opShiftRightLogical(
          getVectorTypeId(result.type),
          shiftReg.id, countReg.id);
        break;
      
      default:
        Logger::warn(str::format(
          "DxbcCompiler: Unhandled instruction: ",
          ins.op));
        return;
    }
    
    result = emitDstOperandModifiers(result, ins.modifiers);
    emitRegisterStore(ins.dst[0], result);
  }
  
    
  void DxbcCompiler::emitVectorSinCos(const DxbcShaderInstruction& ins) {
    // sincos has three operands:
    //    (dst0) Destination register for sin(x)
    //    (dst1) Destination register for cos(x)
    //    (src0) Source operand x
    
    // Load source operand as 32-bit float vector.
    const DxbcRegisterValue srcValue = emitRegisterLoad(
      ins.src[0], DxbcRegMask(true, true, true, true));

    uint32_t typeId = getScalarTypeId(srcValue.type.ctype);

    DxbcRegisterValue sinVector = { };
    sinVector.type.ctype = DxbcScalarType::Float32;

    DxbcRegisterValue cosVector = { };
    cosVector.type.ctype = DxbcScalarType::Float32;

    // Only compute sincos for enabled components
    std::array<uint32_t, 4> sinIds = { };
    std::array<uint32_t, 4> cosIds = { };

    for (uint32_t i = 0; i < 4; i++) {
      const uint32_t sinIndex = 0u;
      const uint32_t cosIndex = 1u;

      if (ins.dst[0].mask[i] || ins.dst[1].mask[i]) {
        uint32_t sincosId = m_module.opSinCos(m_module.opCompositeExtract(typeId, srcValue.id, 1u, &i), !m_moduleInfo.options.sincosEmulation);

        if (ins.dst[0].type != DxbcOperandType::Null && ins.dst[0].mask[i])
          sinIds[sinVector.type.ccount++] = m_module.opCompositeExtract(typeId, sincosId, 1u, &sinIndex);

        if (ins.dst[1].type != DxbcOperandType::Null && ins.dst[1].mask[i])
          cosIds[cosVector.type.ccount++] = m_module.opCompositeExtract(typeId, sincosId, 1u, &cosIndex);
      }
    }

    if (sinVector.type.ccount) {
      sinVector.id = sinVector.type.ccount > 1u
        ? m_module.opCompositeConstruct(getVectorTypeId(sinVector.type), sinVector.type.ccount, sinIds.data())
        : sinIds[0];

      emitRegisterStore(ins.dst[0], sinVector);
    }

    if (cosVector.type.ccount) {
      cosVector.id = cosVector.type.ccount > 1u
        ? m_module.opCompositeConstruct(getVectorTypeId(cosVector.type), cosVector.type.ccount, cosIds.data())
        : cosIds[0];

      emitRegisterStore(ins.dst[1], cosVector);
    }
  }
  
  
  void DxbcCompiler::emitGeometryEmit(const DxbcShaderInstruction& ins) {
    // In xfb mode we might have multiple streams, so
    // we have to figure out which stream to write to
    uint32_t streamId  = 0;
    uint32_t streamVar = 0;

    if (m_moduleInfo.xfb != nullptr) {
      streamId  = ins.dstCount > 0 ? ins.dst[0].idx[0].offset : 0;
      streamVar = m_module.constu32(streamId);
    }

    // Checking the negation is easier for EmitThenCut/EmitThenCutStream
    bool doEmit = ins.op != DxbcOpcode::Cut && ins.op != DxbcOpcode::CutStream;
    bool doCut = ins.op != DxbcOpcode::Emit && ins.op != DxbcOpcode::EmitStream;

    if (doEmit) {
      if (m_gs.needsOutputSetup)
        emitOutputSetup();
      emitClipCullStore(DxbcSystemValue::ClipDistance, m_clipDistances);
      emitClipCullStore(DxbcSystemValue::CullDistance, m_cullDistances);
      emitXfbOutputSetup(streamId, false);
      m_module.opEmitVertex(streamVar);
    }

    if (doCut)
      m_module.opEndPrimitive(streamVar);
  }
  
  
  void DxbcCompiler::emitAtomic(const DxbcShaderInstruction& ins) {
    // atomic_* operations have the following operands:
    //    (dst0) Destination u# or g# register
    //    (src0) Index into the texture or buffer
    //    (src1) The source value for the operation
    //    (src2) Second source operand (optional)
    // imm_atomic_* operations have the following operands:
    //    (dst0) Register that receives the result
    //    (dst1) Destination u# or g# register
    //    (srcX) As above
    const DxbcBufferInfo bufferInfo = getBufferInfo(ins.dst[ins.dstCount - 1]);
    
    bool isImm = ins.dstCount == 2;
    bool isUav = ins.dst[ins.dstCount - 1].type == DxbcOperandType::UnorderedAccessView;
    bool isSsbo = bufferInfo.isSsbo;

    // Retrieve destination pointer for the atomic operation>
    const DxbcRegisterPointer pointer = emitGetAtomicPointer(
      ins.dst[ins.dstCount - 1], ins.src[0]);
    
    // Load source values
    std::array<DxbcRegisterValue, 2> src;
    
    for (uint32_t i = 1; i < ins.srcCount; i++) {
      src[i - 1] = emitRegisterBitcast(
        emitRegisterLoad(ins.src[i], DxbcRegMask(true, false, false, false)),
        pointer.type.ctype);
    }
    
    // Define memory scope and semantics based on the operands
    uint32_t scope     = 0;
    uint32_t semantics = 0;
    
    if (isUav) {
      scope     = spv::ScopeQueueFamily;
      semantics = spv::MemorySemanticsAcquireReleaseMask;

      semantics |= isSsbo
                ? spv::MemorySemanticsUniformMemoryMask
                : spv::MemorySemanticsImageMemoryMask;
    } else {
      scope     = spv::ScopeWorkgroup;
      semantics = spv::MemorySemanticsWorkgroupMemoryMask
                | spv::MemorySemanticsAcquireReleaseMask;
    }

    const uint32_t scopeId     = m_module.constu32(scope);
    const uint32_t semanticsId = m_module.constu32(semantics);
    
    // Perform the atomic operation on the given pointer
    DxbcRegisterValue value;
    value.type = pointer.type;
    value.id   = 0;
    
    // The result type, which is a scalar integer
    const uint32_t typeId = getVectorTypeId(value.type);
    
    switch (ins.op) {
      case DxbcOpcode::AtomicCmpStore:
      case DxbcOpcode::ImmAtomicCmpExch:
        value.id = m_module.opAtomicCompareExchange(
          typeId, pointer.id, scopeId, semanticsId,
          m_module.constu32(spv::MemorySemanticsMaskNone),
          src[1].id, src[0].id);
        break;
      
      case DxbcOpcode::ImmAtomicExch:
        value.id = m_module.opAtomicExchange(typeId,
          pointer.id, scopeId, semanticsId,
          src[0].id);
        break;
      
      case DxbcOpcode::AtomicIAdd:
      case DxbcOpcode::ImmAtomicIAdd:
        value.id = m_module.opAtomicIAdd(typeId,
          pointer.id, scopeId, semanticsId,
          src[0].id);
        break;
      
      case DxbcOpcode::AtomicAnd:
      case DxbcOpcode::ImmAtomicAnd:
        value.id = m_module.opAtomicAnd(typeId,
          pointer.id, scopeId, semanticsId,
          src[0].id);
        break;
      
      case DxbcOpcode::AtomicOr:
      case DxbcOpcode::ImmAtomicOr:
        value.id = m_module.opAtomicOr(typeId,
          pointer.id, scopeId, semanticsId,
          src[0].id);
        break;
      
      case DxbcOpcode::AtomicXor:
      case DxbcOpcode::ImmAtomicXor:
        value.id = m_module.opAtomicXor(typeId,
          pointer.id, scopeId, semanticsId,
          src[0].id);
        break;
      
      case DxbcOpcode::AtomicIMin:
      case DxbcOpcode::ImmAtomicIMin:
        value.id = m_module.opAtomicSMin(typeId,
          pointer.id, scopeId, semanticsId,
          src[0].id);
        break;
      
      case DxbcOpcode::AtomicIMax:
      case DxbcOpcode::ImmAtomicIMax:
        value.id = m_module.opAtomicSMax(typeId,
          pointer.id, scopeId, semanticsId,
          src[0].id);
        break;
      
      case DxbcOpcode::AtomicUMin:
      case DxbcOpcode::ImmAtomicUMin:
        value.id = m_module.opAtomicUMin(typeId,
          pointer.id, scopeId, semanticsId,
          src[0].id);
        break;
      
      case DxbcOpcode::AtomicUMax:
      case DxbcOpcode::ImmAtomicUMax:
        value.id = m_module.opAtomicUMax(typeId,
          pointer.id, scopeId, semanticsId,
          src[0].id);
        break;
      
      default:
        Logger::warn(str::format(
          "DxbcCompiler: Unhandled instruction: ",
          ins.op));
        return;
    }
    
    // Write back the result to the destination
    // register if this is an imm_atomic_* opcode.
    if (isImm)
      emitRegisterStore(ins.dst[0], value);
  }
  
  
  void DxbcCompiler::emitAtomicCounter(const DxbcShaderInstruction& ins) {
    // imm_atomic_alloc and imm_atomic_consume have the following operands:
    //    (dst0) The register that will hold the old counter value
    //    (dst1) The UAV whose counter is going to be modified
    const uint32_t registerId = ins.dst[1].idx[0].offset;
    
    if (m_uavs.at(registerId).ctrId == 0)
      m_uavs.at(registerId).ctrId = emitDclUavCounter(registerId);
    
    // Get a pointer to the atomic counter in question
    DxbcRegisterInfo ptrType;
    ptrType.type.ctype   = DxbcScalarType::Uint32;
    ptrType.type.ccount  = 1;
    ptrType.type.alength = 0;
    ptrType.sclass = spv::StorageClassStorageBuffer;
    
    uint32_t zeroId = m_module.consti32(0);
    uint32_t ptrId  = m_module.opAccessChain(
      getPointerTypeId(ptrType),
      m_uavs.at(registerId).ctrId,
      1, &zeroId);
    
    // Define memory scope and semantics based on the operands
    uint32_t scope     = spv::ScopeQueueFamily;
    uint32_t semantics = spv::MemorySemanticsUniformMemoryMask
                       | spv::MemorySemanticsAcquireReleaseMask;
    
    uint32_t scopeId     = m_module.constu32(scope);
    uint32_t semanticsId = m_module.constu32(semantics);
    
    // Compute the result value
    DxbcRegisterValue value;
    value.type.ctype  = DxbcScalarType::Uint32;
    value.type.ccount = 1;
    
    uint32_t typeId = getVectorTypeId(value.type);
    
    switch (ins.op) {
      case DxbcOpcode::ImmAtomicAlloc:
        value.id = m_module.opAtomicIAdd(typeId, ptrId,
          scopeId, semanticsId, m_module.constu32(1));
        break;
        
      case DxbcOpcode::ImmAtomicConsume:
        value.id = m_module.opAtomicISub(typeId, ptrId,
          scopeId, semanticsId, m_module.constu32(1));
        value.id = m_module.opISub(typeId, value.id,
          m_module.constu32(1));
        break;
      
      default:
        Logger::warn(str::format(
          "DxbcCompiler: Unhandled instruction: ",
          ins.op));
        return;
    }

    // Store the result
    emitRegisterStore(ins.dst[0], value);
  }
  
  
  void DxbcCompiler::emitBarrier(const DxbcShaderInstruction& ins) {
    // sync takes no operands. Instead, the synchronization
    // scope is defined by the operand control bits.
    const DxbcSyncFlags flags = ins.controls.syncFlags();
    
    uint32_t executionScope   = spv::ScopeInvocation;
    uint32_t memoryScope      = spv::ScopeInvocation;
    uint32_t memorySemantics  = 0;
    
    if (flags.test(DxbcSyncFlag::ThreadsInGroup))
      executionScope   = spv::ScopeWorkgroup;
    
    if (flags.test(DxbcSyncFlag::ThreadGroupSharedMemory)) {
      memoryScope      = spv::ScopeWorkgroup;
      memorySemantics |= spv::MemorySemanticsWorkgroupMemoryMask
                      |  spv::MemorySemanticsAcquireReleaseMask
                      |  spv::MemorySemanticsMakeAvailableMask
                      |  spv::MemorySemanticsMakeVisibleMask;
    }
    
    if (flags.test(DxbcSyncFlag::UavMemoryGroup)) {
      memoryScope      = spv::ScopeWorkgroup;
      memorySemantics |= spv::MemorySemanticsImageMemoryMask
                      |  spv::MemorySemanticsUniformMemoryMask
                      |  spv::MemorySemanticsAcquireReleaseMask
                      |  spv::MemorySemanticsMakeAvailableMask
                      |  spv::MemorySemanticsMakeVisibleMask;
    }
    
    if (flags.test(DxbcSyncFlag::UavMemoryGlobal)) {
      memoryScope      = spv::ScopeQueueFamily;

      if (m_programInfo.type() == DxbcProgramType::ComputeShader && !m_hasGloballyCoherentUav)
        memoryScope    = spv::ScopeWorkgroup;

      memorySemantics |= spv::MemorySemanticsImageMemoryMask
                      |  spv::MemorySemanticsUniformMemoryMask
                      |  spv::MemorySemanticsAcquireReleaseMask
                      |  spv::MemorySemanticsMakeAvailableMask
                      |  spv::MemorySemanticsMakeVisibleMask;
    }
    
    if (executionScope != spv::ScopeInvocation) {
      m_module.opControlBarrier(
        m_module.constu32(executionScope),
        m_module.constu32(memoryScope),
        m_module.constu32(memorySemantics));
    } else if (memoryScope != spv::ScopeInvocation) {
      m_module.opMemoryBarrier(
        m_module.constu32(memoryScope),
        m_module.constu32(memorySemantics));
    } else {
      Logger::warn("DxbcCompiler: sync instruction has no effect");
    }
  }
  
  
  void DxbcCompiler::emitBitExtract(const DxbcShaderInstruction& ins) {
    // ibfe and ubfe take the following arguments:
    //    (dst0) The destination register
    //    (src0) Number of bits to extact
    //    (src1) Offset of the bits to extract
    //    (src2) Register to extract bits from
    const bool isSigned = ins.op == DxbcOpcode::IBfe;
    
    DxbcRegisterValue bitCnt = emitRegisterLoad(ins.src[0], ins.dst[0].mask);
    DxbcRegisterValue bitOfs = emitRegisterLoad(ins.src[1], ins.dst[0].mask);

    if (ins.src[0].type != DxbcOperandType::Imm32)
      bitCnt = emitRegisterMaskBits(bitCnt, 0x1F);
    
    if (ins.src[1].type != DxbcOperandType::Imm32)
      bitOfs = emitRegisterMaskBits(bitOfs, 0x1F);
    
    const DxbcRegisterValue src = emitRegisterLoad(ins.src[2], ins.dst[0].mask);
    
    const uint32_t componentCount  = src.type.ccount;
    std::array<uint32_t, 4> componentIds = {{ 0, 0, 0, 0 }};
    
    for (uint32_t i = 0; i < componentCount; i++) {
      const DxbcRegisterValue currBitCnt = emitRegisterExtract(bitCnt, DxbcRegMask::select(i));
      const DxbcRegisterValue currBitOfs = emitRegisterExtract(bitOfs, DxbcRegMask::select(i));
      const DxbcRegisterValue currSrc    = emitRegisterExtract(src,    DxbcRegMask::select(i));
      
      const uint32_t typeId = getVectorTypeId(currSrc.type);
      
      componentIds[i] = isSigned
        ? m_module.opBitFieldSExtract(typeId, currSrc.id, currBitOfs.id, currBitCnt.id)
        : m_module.opBitFieldUExtract(typeId, currSrc.id, currBitOfs.id, currBitCnt.id);
    }
    
    DxbcRegisterValue result;
    result.type = src.type;
    result.id   = componentCount > 1
      ? m_module.opCompositeConstruct(
          getVectorTypeId(result.type),
          componentCount, componentIds.data())
      : componentIds[0];
    emitRegisterStore(ins.dst[0], result);
  }
  
  
  void DxbcCompiler::emitBitInsert(const DxbcShaderInstruction& ins) {
    // ibfe and ubfe take the following arguments:
    //    (dst0) The destination register
    //    (src0) Number of bits to extact
    //    (src1) Offset of the bits to extract
    //    (src2) Register to take bits from
    //    (src3) Register to replace bits in
    DxbcRegisterValue bitCnt = emitRegisterLoad(ins.src[0], ins.dst[0].mask);
    DxbcRegisterValue bitOfs = emitRegisterLoad(ins.src[1], ins.dst[0].mask);
    
    if (ins.src[0].type != DxbcOperandType::Imm32)
      bitCnt = emitRegisterMaskBits(bitCnt, 0x1F);
    
    if (ins.src[1].type != DxbcOperandType::Imm32)
      bitOfs = emitRegisterMaskBits(bitOfs, 0x1F);
    
    const DxbcRegisterValue insert = emitRegisterLoad(ins.src[2], ins.dst[0].mask);
    const DxbcRegisterValue base   = emitRegisterLoad(ins.src[3], ins.dst[0].mask);
    
    const uint32_t componentCount  = base.type.ccount;
    std::array<uint32_t, 4> componentIds = {{ 0, 0, 0, 0 }};
    
    for (uint32_t i = 0; i < componentCount; i++) {
      const DxbcRegisterValue currBitCnt = emitRegisterExtract(bitCnt, DxbcRegMask::select(i));
      const DxbcRegisterValue currBitOfs = emitRegisterExtract(bitOfs, DxbcRegMask::select(i));
      const DxbcRegisterValue currInsert = emitRegisterExtract(insert, DxbcRegMask::select(i));
      const DxbcRegisterValue currBase   = emitRegisterExtract(base,   DxbcRegMask::select(i));
      
      componentIds[i] = m_module.opBitFieldInsert(
        getVectorTypeId(currBase.type),
        currBase.id,   currInsert.id,
        currBitOfs.id, currBitCnt.id);
    }
    
    DxbcRegisterValue result;
    result.type = base.type;
    result.id   = componentCount > 1
      ? m_module.opCompositeConstruct(
          getVectorTypeId(result.type),
          componentCount, componentIds.data())
      : componentIds[0];
    emitRegisterStore(ins.dst[0], result);
  }
  
  
  void DxbcCompiler::emitBitScan(const DxbcShaderInstruction& ins) {
    // firstbit(lo|hi|shi) have two operands:
    //    (dst0) The destination operant
    //    (src0) Source operand to scan
    DxbcRegisterValue src = emitRegisterLoad(ins.src[0], ins.dst[0].mask);
    
    DxbcRegisterValue dst;
    dst.type.ctype  = ins.dst[0].dataType;
    dst.type.ccount = ins.dst[0].mask.popCount();
    
    // Result type, should be an unsigned integer
    const uint32_t typeId = getVectorTypeId(dst.type);
    
    switch (ins.op) {
      case DxbcOpcode::FirstBitLo:  dst.id = m_module.opFindILsb(typeId, src.id); break;
      case DxbcOpcode::FirstBitHi:  dst.id = m_module.opFindUMsb(typeId, src.id); break;
      case DxbcOpcode::FirstBitShi: dst.id = m_module.opFindSMsb(typeId, src.id); break;
      default: Logger::warn(str::format("DxbcCompiler: Unhandled instruction: ", ins.op)); return;
    }
    
    // The 'Hi' variants are counted from the MSB in DXBC
    // rather than the LSB, so we have to invert the number
    if (ins.op == DxbcOpcode::FirstBitHi || ins.op == DxbcOpcode::FirstBitShi) {
      uint32_t boolTypeId = m_module.defBoolType();

      if (dst.type.ccount > 1)
        boolTypeId = m_module.defVectorType(boolTypeId, dst.type.ccount);

      DxbcRegisterValue const31 = emitBuildConstVecu32(31u, 31u, 31u, 31u, ins.dst[0].mask);
      DxbcRegisterValue constff = emitBuildConstVecu32(~0u, ~0u, ~0u, ~0u, ins.dst[0].mask);

      dst.id = m_module.opSelect(typeId,
        m_module.opINotEqual(boolTypeId, dst.id, constff.id),
        m_module.opISub(typeId, const31.id, dst.id),
        constff.id);
    }
    
    // No modifiers are supported
    emitRegisterStore(ins.dst[0], dst);
  }
  
  
  void DxbcCompiler::emitBufferQuery(const DxbcShaderInstruction& ins) {
    // bufinfo takes two arguments
    //    (dst0) The destination register
    //    (src0) The buffer register to query
    const DxbcBufferInfo bufferInfo = getBufferInfo(ins.src[0]);
    bool isSsbo = bufferInfo.isSsbo;
    
    // We'll store this as a scalar unsigned integer
    DxbcRegisterValue result = isSsbo
      ? emitQueryBufferSize(ins.src[0])
      : emitQueryTexelBufferSize(ins.src[0]);
    
    uint32_t typeId = getVectorTypeId(result.type);
    
    // Adjust returned size if this is a raw or structured
    // buffer, as emitQueryTexelBufferSize only returns the
    // number of typed elements in the buffer.
    if (bufferInfo.type == DxbcResourceType::Raw) {
      result.id = m_module.opIMul(typeId,
        result.id, m_module.constu32(4));
    } else if (bufferInfo.type == DxbcResourceType::Structured) {
      result.id = m_module.opUDiv(typeId, result.id,
        m_module.constu32(bufferInfo.stride / 4));
    }

    // Store the result. The scalar will be extended to a
    // vector if the write mask consists of more than one
    // component, which is the desired behaviour.
    emitRegisterStore(ins.dst[0], result);
  }
  
  
  void DxbcCompiler::emitBufferLoad(const DxbcShaderInstruction& ins) {
    // ld_raw takes three arguments:
    //    (dst0) Destination register
    //    (src0) Byte offset
    //    (src1) Source register
    // ld_structured takes four arguments:
    //    (dst0) Destination register
    //    (src0) Structure index
    //    (src1) Byte offset
    //    (src2) Source register
    const bool isStructured = ins.op == DxbcOpcode::LdStructured
                           || ins.op == DxbcOpcode::LdStructuredS;
    
    // Source register. The exact way we access
    // the data depends on the register type.
    const DxbcRegister& dstReg = ins.dst[0];
    const DxbcRegister& srcReg = isStructured ? ins.src[2] : ins.src[1];

    if (dstReg.type == DxbcOperandType::UnorderedAccessView)
      emitUavBarrier(uint64_t(1u) << srcReg.idx[0].offset, 0u);

    // Retrieve common info about the buffer
    const DxbcBufferInfo bufferInfo = getBufferInfo(srcReg);

    // Shared memory is the only type of buffer that
    // is not accessed through a texel buffer view
    bool isTgsm = srcReg.type == DxbcOperandType::ThreadGroupSharedMemory;
    bool isSsbo = bufferInfo.isSsbo;

    // Common types and IDs used while loading the data
    uint32_t bufferId = isTgsm || isSsbo ? 0 : m_module.opLoad(bufferInfo.typeId, bufferInfo.varId);

    uint32_t vectorTypeId = getVectorTypeId({ DxbcScalarType::Uint32, 4 });
    uint32_t scalarTypeId = getVectorTypeId({ DxbcScalarType::Uint32, 1 });

    // Since all data is represented as a sequence of 32-bit
    // integers, we have to load each component individually.
    std::array<uint32_t, 4> ccomps = { 0, 0, 0, 0 };
    std::array<uint32_t, 4> scomps = { 0, 0, 0, 0 };
    uint32_t                scount = 0;

    // The sparse feedback ID will be non-zero for sparse
    // instructions on input. We need to reset it to 0.
    SpirvMemoryOperands memoryOperands;
    SpirvImageOperands imageOperands;
    imageOperands.sparse = ins.dstCount == 2;

    uint32_t coherence = bufferInfo.coherence;

    if (isTgsm && m_moduleInfo.options.forceVolatileTgsmAccess) {
      memoryOperands.flags |= spv::MemoryAccessVolatileMask;
      coherence = spv::ScopeWorkgroup;
    }

    if (coherence) {
      memoryOperands.flags |= spv::MemoryAccessNonPrivatePointerMask;

      if (coherence != spv::ScopeInvocation) {
        memoryOperands.flags |= spv::MemoryAccessMakePointerVisibleMask;
        memoryOperands.makeVisible = m_module.constu32(coherence);

        imageOperands.flags = spv::ImageOperandsNonPrivateTexelMask
                            | spv::ImageOperandsMakeTexelVisibleMask;
        imageOperands.makeVisible = m_module.constu32(coherence);
      }
    }

    uint32_t sparseFeedbackId = 0;

    bool useRawAccessChains = m_hasRawAccessChains && isSsbo && !imageOperands.sparse;

    DxbcRegisterValue index = emitRegisterLoad(ins.src[0], DxbcRegMask(true, false, false, false));
    DxbcRegisterValue offset = index;

    if (isStructured)
      offset = emitRegisterLoad(ins.src[1], DxbcRegMask(true, false, false, false));

    DxbcRegisterValue elementIndex = { };

    uint32_t baseAlignment = sizeof(uint32_t);

    if (useRawAccessChains) {
      memoryOperands.flags |= spv::MemoryAccessAlignedMask;

      if (isStructured && ins.src[1].type == DxbcOperandType::Imm32) {
        baseAlignment = bufferInfo.stride | ins.src[1].imm.u32_1;
        baseAlignment = baseAlignment & -baseAlignment;
        baseAlignment = std::min(baseAlignment, uint32_t(m_moduleInfo.options.minSsboAlignment));
      }
    } else {
      elementIndex = isStructured
        ? emitCalcBufferIndexStructured(index, offset, bufferInfo.stride)
        : emitCalcBufferIndexRaw(offset);
    }

    uint32_t readMask = 0u;

    for (uint32_t i = 0; i < 4; i++) {
      if (dstReg.mask[i])
        readMask |= 1u << srcReg.swizzle[i];
    }

    while (readMask) {
      uint32_t sindex = bit::tzcnt(readMask);
      uint32_t scount = bit::tzcnt(~(readMask >> sindex));
      uint32_t zero = 0;

      if (useRawAccessChains) {
        uint32_t alignment = baseAlignment;
        uint32_t offsetId = offset.id;

        if (sindex) {
          offsetId = m_module.opIAdd(scalarTypeId,
            offsetId, m_module.constu32(sizeof(uint32_t) * sindex));
          alignment |= sizeof(uint32_t) * sindex;
        }

        DxbcRegisterInfo storeInfo;
        storeInfo.type.ctype = DxbcScalarType::Uint32;
        storeInfo.type.ccount = scount;
        storeInfo.type.alength = 0;
        storeInfo.sclass = spv::StorageClassStorageBuffer;

        uint32_t loadTypeId = getArrayTypeId(storeInfo.type);
        uint32_t ptrTypeId = getPointerTypeId(storeInfo);

        uint32_t accessChain = isStructured
          ? m_module.opRawAccessChain(ptrTypeId, bufferInfo.varId,
              m_module.constu32(bufferInfo.stride), index.id, offsetId,
              spv::RawAccessChainOperandsRobustnessPerElementNVMask)
          : m_module.opRawAccessChain(ptrTypeId, bufferInfo.varId,
              m_module.constu32(0), m_module.constu32(0), offsetId,
              spv::RawAccessChainOperandsRobustnessPerComponentNVMask);

        memoryOperands.alignment = alignment & -alignment;

        uint32_t vectorId = m_module.opLoad(loadTypeId, accessChain, memoryOperands);

        for (uint32_t i = 0; i < scount; i++) {
          ccomps[sindex + i] = vectorId;

          if (scount > 1) {
            ccomps[sindex + i] = m_module.opCompositeExtract(
              scalarTypeId, vectorId, 1, &i);
          }
        }

        readMask &= ~(((1u << scount) - 1u) << sindex);
      } else {
        uint32_t elementIndexAdjusted = m_module.opIAdd(
          getVectorTypeId(elementIndex.type), elementIndex.id,
          m_module.consti32(sindex));

        if (isTgsm) {
          ccomps[sindex] = m_module.opLoad(scalarTypeId,
            m_module.opAccessChain(bufferInfo.typeId,
              bufferInfo.varId, 1, &elementIndexAdjusted),
            memoryOperands);
        } else if (isSsbo) {
          uint32_t indices[2] = { m_module.constu32(0), elementIndexAdjusted };
          ccomps[sindex] = m_module.opLoad(scalarTypeId,
            m_module.opAccessChain(bufferInfo.typeId,
              bufferInfo.varId, 2, indices),
            memoryOperands);
        } else {
          uint32_t resultTypeId = vectorTypeId;
          uint32_t resultId = 0;

          if (imageOperands.sparse)
            resultTypeId = getSparseResultTypeId(vectorTypeId);

          if (srcReg.type == DxbcOperandType::Resource) {
            resultId = m_module.opImageFetch(resultTypeId,
              bufferId, elementIndexAdjusted, imageOperands);
          } else if (srcReg.type == DxbcOperandType::UnorderedAccessView) {
            resultId = m_module.opImageRead(resultTypeId,
              bufferId, elementIndexAdjusted, imageOperands);
          } else {
            throw DxvkError("DxbcCompiler: Invalid operand type for strucured/raw load");
          }

          // Only read sparse feedback once. This may be somewhat inaccurate
          // for reads that straddle pages, but we can't easily emulate this.
          if (imageOperands.sparse) {
            imageOperands.sparse = false;
            sparseFeedbackId = resultId;

            resultId = emitExtractSparseTexel(vectorTypeId, resultId);
          }

          ccomps[sindex] = m_module.opCompositeExtract(scalarTypeId, resultId, 1, &zero);
        }

        readMask &= readMask - 1;
      }
    }

    for (uint32_t i = 0; i < 4; i++) {
      uint32_t sindex = srcReg.swizzle[i];

      if (dstReg.mask[i])
        scomps[scount++] = ccomps[sindex];
    }

    DxbcRegisterValue result =  { };
    result.type.ctype  = DxbcScalarType::Uint32;
    result.type.ccount = scount;
    result.id = scomps[0];

    if (scount > 1) {
      result.id = m_module.opCompositeConstruct(
        getVectorTypeId(result.type),
        scount, scomps.data());
    }

    emitRegisterStore(dstReg, result);

    if (sparseFeedbackId)
      emitStoreSparseFeedback(ins.dst[1], sparseFeedbackId);
  }
  
  
  void DxbcCompiler::emitBufferStore(const DxbcShaderInstruction& ins) {
    // store_raw takes three arguments:
    //    (dst0) Destination register
    //    (src0) Byte offset
    //    (src1) Source register
    // store_structured takes four arguments:
    //    (dst0) Destination register
    //    (src0) Structure index
    //    (src1) Byte offset
    //    (src2) Source register
    const bool isStructured = ins.op == DxbcOpcode::StoreStructured;
    
    // Source register. The exact way we access
    // the data depends on the register type.
    const DxbcRegister& dstReg = ins.dst[0];
    const DxbcRegister& srcReg = isStructured ? ins.src[2] : ins.src[1];

    if (dstReg.type == DxbcOperandType::UnorderedAccessView)
      emitUavBarrier(0u, uint64_t(1u) << dstReg.idx[0].offset);

    DxbcRegisterValue value = emitRegisterLoad(srcReg, dstReg.mask);
    value = emitRegisterBitcast(value, DxbcScalarType::Uint32);

    // Retrieve common info about the buffer
    const DxbcBufferInfo bufferInfo = getBufferInfo(dstReg);

    // Thread Group Shared Memory is not accessed through a texel buffer view
    bool isTgsm = dstReg.type == DxbcOperandType::ThreadGroupSharedMemory;
    bool isSsbo = bufferInfo.isSsbo;

    uint32_t bufferId = isTgsm || isSsbo ? 0 : m_module.opLoad(bufferInfo.typeId, bufferInfo.varId);

    uint32_t scalarTypeId = getVectorTypeId({ DxbcScalarType::Uint32, 1 });
    uint32_t vectorTypeId = getVectorTypeId({ DxbcScalarType::Uint32, 4 });

    // Set memory operands according to resource properties
    SpirvMemoryOperands memoryOperands;
    SpirvImageOperands imageOperands;

    uint32_t coherence = bufferInfo.coherence;

    if (isTgsm && m_moduleInfo.options.forceVolatileTgsmAccess) {
      memoryOperands.flags |= spv::MemoryAccessVolatileMask;
      coherence = spv::ScopeWorkgroup;
    }

    if (coherence) {
      memoryOperands.flags |= spv::MemoryAccessNonPrivatePointerMask;

      if (coherence != spv::ScopeInvocation) {
        memoryOperands.flags |= spv::MemoryAccessMakePointerAvailableMask;
        memoryOperands.makeAvailable = m_module.constu32(coherence);

        imageOperands.flags = spv::ImageOperandsNonPrivateTexelMask
                            | spv::ImageOperandsMakeTexelAvailableMask;
        imageOperands.makeAvailable = m_module.constu32(coherence);
      }
    }

    // Compute flat element index as necessary
    bool useRawAccessChains = isSsbo && m_hasRawAccessChains;

    DxbcRegisterValue index = emitRegisterLoad(ins.src[0], DxbcRegMask(true, false, false, false));
    DxbcRegisterValue offset = index;

    if (isStructured)
      offset = emitRegisterLoad(ins.src[1], DxbcRegMask(true, false, false, false));

    DxbcRegisterValue elementIndex = { };

    uint32_t baseAlignment = sizeof(uint32_t);

    if (useRawAccessChains) {
      memoryOperands.flags |= spv::MemoryAccessAlignedMask;

      if (isStructured && ins.src[1].type == DxbcOperandType::Imm32) {
        baseAlignment = bufferInfo.stride | ins.src[1].imm.u32_1;
        baseAlignment = baseAlignment & -baseAlignment;
        baseAlignment = std::min(baseAlignment, uint32_t(m_moduleInfo.options.minSsboAlignment));
      }
    } else {
      elementIndex = isStructured
        ? emitCalcBufferIndexStructured(index, offset, bufferInfo.stride)
        : emitCalcBufferIndexRaw(offset);
    }

    uint32_t writeMask = dstReg.mask.raw();

    while (writeMask) {
      uint32_t sindex = bit::tzcnt(writeMask);
      uint32_t scount = bit::tzcnt(~(writeMask >> sindex));

      if (useRawAccessChains) {
        uint32_t alignment = baseAlignment;
        uint32_t offsetId = offset.id;

        if (sindex) {
          offsetId = m_module.opIAdd(scalarTypeId,
            offsetId, m_module.constu32(sizeof(uint32_t) * sindex));
          alignment = alignment | (sizeof(uint32_t) * sindex);
        }

        DxbcRegisterInfo storeInfo;
        storeInfo.type.ctype = DxbcScalarType::Uint32;
        storeInfo.type.ccount = scount;
        storeInfo.type.alength = 0;
        storeInfo.sclass = spv::StorageClassStorageBuffer;

        uint32_t storeTypeId = getArrayTypeId(storeInfo.type);
        uint32_t ptrTypeId = getPointerTypeId(storeInfo);

        uint32_t accessChain = isStructured
          ? m_module.opRawAccessChain(ptrTypeId, bufferInfo.varId,
              m_module.constu32(bufferInfo.stride), index.id, offsetId,
              spv::RawAccessChainOperandsRobustnessPerElementNVMask)
          : m_module.opRawAccessChain(ptrTypeId, bufferInfo.varId,
              m_module.constu32(0), m_module.constu32(0), offsetId,
              spv::RawAccessChainOperandsRobustnessPerComponentNVMask);

        uint32_t valueId = value.id;

        if (scount < value.type.ccount) {
          if (scount == 1) {
            valueId = m_module.opCompositeExtract(storeTypeId, value.id, 1, &sindex);
          } else {
            std::array<uint32_t, 4> indices = { sindex, sindex + 1u, sindex + 2u, sindex + 3u };
            valueId = m_module.opVectorShuffle(storeTypeId, value.id, value.id, scount, indices.data());
          }
        }

        memoryOperands.alignment = alignment & -alignment;
        m_module.opStore(accessChain, valueId, memoryOperands);

        writeMask &= ~(((1u << scount) - 1u) << sindex);
      } else {
        uint32_t srcComponentId = value.type.ccount > 1
          ? m_module.opCompositeExtract(scalarTypeId,
              value.id, 1, &sindex)
          : value.id;

        uint32_t elementIndexAdjusted = sindex != 0
          ? m_module.opIAdd(getVectorTypeId(elementIndex.type),
              elementIndex.id, m_module.consti32(sindex))
          : elementIndex.id;

        if (isTgsm) {
          m_module.opStore(
            m_module.opAccessChain(bufferInfo.typeId,
              bufferInfo.varId, 1, &elementIndexAdjusted),
            srcComponentId, memoryOperands);
        } else if (isSsbo) {
          uint32_t indices[2] = { m_module.constu32(0), elementIndexAdjusted };
          m_module.opStore(
            m_module.opAccessChain(bufferInfo.typeId,
              bufferInfo.varId, 2, indices),
            srcComponentId, memoryOperands);
        } else if (dstReg.type == DxbcOperandType::UnorderedAccessView) {
          const std::array<uint32_t, 4> srcVectorIds = {
            srcComponentId, srcComponentId,
            srcComponentId, srcComponentId,
          };

          m_module.opImageWrite(
            bufferId, elementIndexAdjusted,
            m_module.opCompositeConstruct(vectorTypeId,
              4, srcVectorIds.data()),
            imageOperands);
        } else {
          throw DxvkError("DxbcCompiler: Invalid operand type for strucured/raw store");
        }

        writeMask &= writeMask - 1u;
      }
    }
  }
  
  
  void DxbcCompiler::emitConvertFloat16(const DxbcShaderInstruction& ins) {
    // f32tof16 takes two operands:
    //    (dst0) Destination register as a uint32 vector
    //    (src0) Source register as a float32 vector
    // f16tof32 takes two operands:
    //    (dst0) Destination register as a float32 vector
    //    (src0) Source register as a uint32 vector
    const DxbcRegisterValue src = emitRegisterLoad(ins.src[0], ins.dst[0].mask);
    
    // We handle both packing and unpacking here
    const bool isPack = ins.op == DxbcOpcode::F32toF16;
    
    // The conversion instructions do not map very well to the
    // SPIR-V pack instructions, which operate on 2D vectors.
    std::array<uint32_t, 4> scalarIds  = {{ 0, 0, 0, 0 }};
    
    const uint32_t componentCount = src.type.ccount;
    
    // These types are used in both pack and unpack operations
    const uint32_t t_u32   = getVectorTypeId({ DxbcScalarType::Uint32,  1 });
    const uint32_t t_f32   = getVectorTypeId({ DxbcScalarType::Float32, 1 });
    const uint32_t t_f32v2 = getVectorTypeId({ DxbcScalarType::Float32, 2 });
    
    // Constant zero-bit pattern, used for packing
    const uint32_t zerof32 = isPack ? m_module.constf32(0.0f) : 0;
    
    for (uint32_t i = 0; i < componentCount; i++) {
      const DxbcRegisterValue componentValue
        = emitRegisterExtract(src, DxbcRegMask::select(i));
      
      if (isPack) {  // f32tof16
        const std::array<uint32_t, 2> packIds =
          {{ componentValue.id, zerof32 }};
        
        scalarIds[i] = m_module.opPackHalf2x16(t_u32,
          m_module.opCompositeConstruct(t_f32v2, packIds.size(), packIds.data()));
      } else {  // f16tof32
        const uint32_t zeroIndex = 0;
        
        scalarIds[i] = m_module.opCompositeExtract(t_f32,
          m_module.opUnpackHalf2x16(t_f32v2, componentValue.id),
          1, &zeroIndex);
      }
    }

    DxbcRegisterValue result;
    result.type.ctype  = ins.dst[0].dataType;
    result.type.ccount = componentCount;

    uint32_t typeId = getVectorTypeId(result.type);
    result.id = componentCount > 1
      ? m_module.opCompositeConstruct(typeId,
          componentCount, scalarIds.data())
      : scalarIds[0];

    if (isPack) {
      // Some drivers return infinity if the input value is above a certain
      // threshold, but D3D wants us to return infinity only if the input is
      // actually infinite. Fix this up to return the maximum representable
      // 16-bit floating point number instead, but preserve input infinity.
      uint32_t t_bvec = getVectorTypeId({ DxbcScalarType::Bool, componentCount });
      uint32_t f16Infinity = m_module.constuReplicant(0x7C00, componentCount);
      uint32_t f16Unsigned = m_module.constuReplicant(0x7FFF, componentCount);

      uint32_t isInputInf = m_module.opIsInf(t_bvec, src.id);
      uint32_t isValueInf = m_module.opIEqual(t_bvec, f16Infinity,
        m_module.opBitwiseAnd(typeId, result.id, f16Unsigned));

      result.id = m_module.opSelect(getVectorTypeId(result.type),
        m_module.opLogicalAnd(t_bvec, isValueInf, m_module.opLogicalNot(t_bvec, isInputInf)),
        m_module.opISub(typeId, result.id, m_module.constuReplicant(1, componentCount)),
        result.id);
    }

    // Store result in the destination register
    emitRegisterStore(ins.dst[0], result);
  }


  void DxbcCompiler::emitConvertFloat64(const DxbcShaderInstruction& ins) {
    // ftod and dtof take the following operands:
    //  (dst0) Destination operand
    //  (src0) Number to convert
    uint32_t dstBits = ins.dst[0].mask.popCount();

    DxbcRegMask srcMask = isDoubleType(ins.dst[0].dataType)
      ? DxbcRegMask(dstBits >= 2, dstBits >= 4, false, false)
      : DxbcRegMask(dstBits >= 1, dstBits >= 1, dstBits >= 2, dstBits >= 2);

    // Perform actual conversion, destination modifiers are not applied
    DxbcRegisterValue val = emitRegisterLoad(ins.src[0], srcMask);

    DxbcRegisterValue result;
    result.type.ctype  = ins.dst[0].dataType;
    result.type.ccount = val.type.ccount;

    switch (ins.op) {
      case DxbcOpcode::DtoF:
      case DxbcOpcode::FtoD:
        result.id = m_module.opFConvert(
          getVectorTypeId(result.type), val.id);
        break;

      case DxbcOpcode::DtoI:
        result.id = m_module.opConvertFtoS(
          getVectorTypeId(result.type), val.id);
        break;

      case DxbcOpcode::DtoU:
        result.id = m_module.opConvertFtoU(
          getVectorTypeId(result.type), val.id);
        break;

      case DxbcOpcode::ItoD:
        result.id = m_module.opConvertStoF(
          getVectorTypeId(result.type), val.id);
        break;
      
      case DxbcOpcode::UtoD:
        result.id = m_module.opConvertUtoF(
          getVectorTypeId(result.type), val.id);
        break;
      
      default:
        Logger::warn(str::format("DxbcCompiler: Unhandled instruction: ", ins.op));
        return;
    }
    
    emitRegisterStore(ins.dst[0], result);
  }
  
  
  void DxbcCompiler::emitHullShaderInstCnt(const DxbcShaderInstruction& ins) {
    this->getCurrentHsForkJoinPhase()->instanceCount = ins.imm[0].u32;
  }
  
  
  void DxbcCompiler::emitHullShaderPhase(const DxbcShaderInstruction& ins) {
    switch (ins.op) {
      case DxbcOpcode::HsDecls: {
        if (m_hs.currPhaseType != DxbcCompilerHsPhase::None)
          Logger::err("DXBC: HsDecls not the first phase in hull shader");
        
        m_hs.currPhaseType = DxbcCompilerHsPhase::Decl;
      } break;
        
      case DxbcOpcode::HsControlPointPhase: {
        m_hs.cpPhase = this->emitNewHullShaderControlPointPhase();
        
        m_hs.currPhaseType = DxbcCompilerHsPhase::ControlPoint;
        m_hs.currPhaseId   = 0;
        
        m_module.setDebugName(m_hs.cpPhase.functionId, "hs_control_point");
      } break;
        
      case DxbcOpcode::HsForkPhase: {
        auto phase = this->emitNewHullShaderForkJoinPhase();
        m_hs.forkPhases.push_back(phase);
        
        m_hs.currPhaseType = DxbcCompilerHsPhase::Fork;
        m_hs.currPhaseId   = m_hs.forkPhases.size() - 1;
        
        m_module.setDebugName(phase.functionId,
          str::format("hs_fork_", m_hs.currPhaseId).c_str());
      } break;
        
      case DxbcOpcode::HsJoinPhase: {
        auto phase = this->emitNewHullShaderForkJoinPhase();
        m_hs.joinPhases.push_back(phase);
        
        m_hs.currPhaseType = DxbcCompilerHsPhase::Join;
        m_hs.currPhaseId   = m_hs.joinPhases.size() - 1;
        
        m_module.setDebugName(phase.functionId,
          str::format("hs_join_", m_hs.currPhaseId).c_str());
      } break;
        
      default:
        Logger::warn(str::format(
          "DxbcCompiler: Unhandled instruction: ",
          ins.op));
    }
  }
  
  
  void DxbcCompiler::emitInterpolate(const DxbcShaderInstruction& ins) {
    m_module.enableCapability(spv::CapabilityInterpolationFunction);

    // The SPIR-V instructions operate on input variable pointers,
    // which are all declared as four-component float vectors.
    uint32_t registerId = ins.src[0].idx[0].offset;
    
    DxbcRegisterValue result;
    result.type = getInputRegType(registerId);
    
    switch (ins.op) {
      case DxbcOpcode::EvalCentroid: {
        result.id = m_module.opInterpolateAtCentroid(
          getVectorTypeId(result.type),
          m_vRegs.at(registerId).id);
      } break;
      
      case DxbcOpcode::EvalSampleIndex: {
        const DxbcRegisterValue sampleIndex = emitRegisterLoad(
          ins.src[1], DxbcRegMask(true, false, false, false));
        
        result.id = m_module.opInterpolateAtSample(
          getVectorTypeId(result.type),
          m_vRegs.at(registerId).id,
          sampleIndex.id);
      } break;
      
      case DxbcOpcode::EvalSnapped: {
        // The offset is encoded as a 4-bit fixed point value
        DxbcRegisterValue offset = emitRegisterLoad(
          ins.src[1], DxbcRegMask(true, true, false, false));
        offset.id = m_module.opBitFieldSExtract(
          getVectorTypeId(offset.type), offset.id,
          m_module.consti32(0), m_module.consti32(4));

        offset.type.ctype = DxbcScalarType::Float32;
        offset.id = m_module.opConvertStoF(
          getVectorTypeId(offset.type), offset.id);

        offset.id = m_module.opFMul(
          getVectorTypeId(offset.type), offset.id,
          m_module.constvec2f32(1.0f / 16.0f, 1.0f / 16.0f));

        result.id = m_module.opInterpolateAtOffset(
          getVectorTypeId(result.type),
          m_vRegs.at(registerId).id,
          offset.id);
      } break;
      
      default:
        Logger::warn(str::format(
          "DxbcCompiler: Unhandled instruction: ",
          ins.op));
        return;
    }
    
    result = emitRegisterSwizzle(result,
      ins.src[0].swizzle, ins.dst[0].mask);
    emitRegisterStore(ins.dst[0], result);
  }
  
  
  void DxbcCompiler::emitSparseCheckAccess(
    const DxbcShaderInstruction&  ins) {
    // check_access_mapped has two operands:
    //    (dst0) The destination register
    //    (src0) The residency code
    m_module.enableCapability(spv::CapabilitySparseResidency);

    DxbcRegisterValue srcValue = emitRegisterLoad(ins.src[0], ins.dst[0].mask);

    uint32_t boolId = m_module.opImageSparseTexelsResident(
      m_module.defBoolType(), srcValue.id);

    DxbcRegisterValue dstValue;
    dstValue.type = { DxbcScalarType::Uint32, 1 };
    dstValue.id = m_module.opSelect(getScalarTypeId(DxbcScalarType::Uint32),
      boolId, m_module.constu32(~0u), m_module.constu32(0));

    emitRegisterStore(ins.dst[0], dstValue);
  }


  void DxbcCompiler::emitTextureQuery(const DxbcShaderInstruction& ins) {
    // resinfo has three operands:
    //    (dst0) The destination register
    //    (src0) Resource LOD to query
    //    (src1) Resource to query
    const DxbcBufferInfo resourceInfo = getBufferInfo(ins.src[1]);
    const DxbcResinfoType resinfoType = ins.controls.resinfoType();
    
    // Read the exact LOD for the image query
    const DxbcRegisterValue mipLod = emitRegisterLoad(
      ins.src[0], DxbcRegMask(true, false, false, false));
    
    const DxbcScalarType returnType = resinfoType == DxbcResinfoType::Uint
      ? DxbcScalarType::Uint32 : DxbcScalarType::Float32;
    
    // Query the size of the selected mip level, as well as the
    // total number of mip levels. We will have to combine the
    // result into a four-component vector later.
    DxbcRegisterValue imageSize   = emitQueryTextureSize(ins.src[1], mipLod);
    DxbcRegisterValue imageLevels = emitQueryTextureLods(ins.src[1]);

    // If the mip level is out of bounds, D3D requires us to return
    // zero before applying modifiers, whereas SPIR-V is undefined,
    // so we need to fix it up manually here.
    imageSize.id = m_module.opSelect(getVectorTypeId(imageSize.type),
      m_module.opULessThan(m_module.defBoolType(), mipLod.id, imageLevels.id),
      imageSize.id, emitBuildZeroVector(imageSize.type).id);

    // Convert intermediates to the requested type
    if (returnType == DxbcScalarType::Float32) {
      imageSize.type.ctype = DxbcScalarType::Float32;
      imageSize.id = m_module.opConvertUtoF(
        getVectorTypeId(imageSize.type),
        imageSize.id);
      
      imageLevels.type.ctype = DxbcScalarType::Float32;
      imageLevels.id = m_module.opConvertUtoF(
        getVectorTypeId(imageLevels.type),
        imageLevels.id);
    }
    
    // If the selected return type is rcpFloat, we need
    // to compute the reciprocal of the image dimensions,
    // but not the array size, so we need to separate it.
    const uint32_t imageCoordDim = imageSize.type.ccount;
    
    DxbcRegisterValue imageLayers;
    imageLayers.type = imageSize.type;
    imageLayers.id   = 0;
    
    if (resinfoType == DxbcResinfoType::RcpFloat && resourceInfo.image.array) {
      imageLayers = emitRegisterExtract(imageSize, DxbcRegMask::select(imageCoordDim - 1));
      imageSize   = emitRegisterExtract(imageSize, DxbcRegMask::firstN(imageCoordDim - 1));
    }
    
    if (resinfoType == DxbcResinfoType::RcpFloat) {
      imageSize.id = m_module.opFDiv(
        getVectorTypeId(imageSize.type),
        emitBuildConstVecf32(1.0f, 1.0f, 1.0f, 1.0f,
          DxbcRegMask::firstN(imageSize.type.ccount)).id,
        imageSize.id);
    }
    
    // Concatenate result vectors and scalars to form a
    // 4D vector. Unused components will be set to zero.
    std::array<uint32_t, 4> vectorIds = { imageSize.id, 0, 0, 0 };
    uint32_t numVectorIds = 1;
    
    if (imageLayers.id != 0)
      vectorIds[numVectorIds++] = imageLayers.id;
    
    if (imageCoordDim < 3) {
      const uint32_t zero = returnType == DxbcScalarType::Uint32
        ? m_module.constu32(0)
        : m_module.constf32(0.0f);
      
      for (uint32_t i = imageCoordDim; i < 3; i++)
        vectorIds[numVectorIds++] = zero;
    }
    
    vectorIds[numVectorIds++] = imageLevels.id;
    
    // Create the actual result vector
    DxbcRegisterValue result;
    result.type.ctype  = returnType;
    result.type.ccount = 4;
    result.id = m_module.opCompositeConstruct(
      getVectorTypeId(result.type),
      numVectorIds, vectorIds.data());
    
    // Swizzle components using the resource swizzle
    // and the destination operand's write mask
    result = emitRegisterSwizzle(result,
      ins.src[1].swizzle, ins.dst[0].mask);
    emitRegisterStore(ins.dst[0], result);
  }
  
  
  void DxbcCompiler::emitTextureQueryLod(const DxbcShaderInstruction& ins) {
    // All sample instructions have at least these operands:
    //    (dst0) The destination register
    //    (src0) Texture coordinates
    //    (src1) The texture itself
    //    (src2) The sampler object
    const DxbcRegister& texCoordReg = ins.src[0];
    const DxbcRegister& textureReg  = ins.src[1];
    const DxbcRegister& samplerReg  = ins.src[2];
    
    // Texture and sampler register IDs
    const auto& texture = m_textures.at(textureReg.idx[0].offset);
    const auto& sampler = m_samplers.at(samplerReg.idx[0].offset);
    
    // Load texture coordinates
    const DxbcRegisterValue coord = emitRegisterLoad(texCoordReg,
      DxbcRegMask::firstN(getTexLayerDim(texture.imageInfo)));
    
    // Query the LOD. The result is a two-dimensional float32
    // vector containing the mip level and virtual LOD numbers.
    const uint32_t sampledImageId = emitLoadSampledImage(texture, sampler, false);
    const uint32_t queriedLodId = m_module.opImageQueryLod(
      getVectorTypeId({ DxbcScalarType::Float32, 2 }),
      sampledImageId, coord.id);
    
    // Build the result array vector by filling up
    // the remaining two components with zeroes.
    const uint32_t zero = m_module.constf32(0.0f);
    const std::array<uint32_t, 3> resultIds
      = {{ queriedLodId, zero, zero }};
    
    DxbcRegisterValue result;
    result.type = DxbcVectorType { DxbcScalarType::Float32, 4 };
    result.id   = m_module.opCompositeConstruct(
      getVectorTypeId(result.type),
      resultIds.size(), resultIds.data());
    
    result = emitRegisterSwizzle(result, ins.src[1].swizzle, ins.dst[0].mask);
    emitRegisterStore(ins.dst[0], result);
  }
  
  
  void DxbcCompiler::emitTextureQueryMs(const DxbcShaderInstruction& ins) {
    // sampleinfo has two operands:
    //    (dst0) The destination register
    //    (src0) Resource to query
    DxbcRegisterValue sampleCount = emitQueryTextureSamples(ins.src[0]);
    
    if (ins.controls.returnType() != DxbcInstructionReturnType::Uint) {
      sampleCount.type = { DxbcScalarType::Float32, 1 };
      sampleCount.id = m_module.opConvertUtoF(
        getVectorTypeId(sampleCount.type),
        sampleCount.id);
    }
    
    emitRegisterStore(ins.dst[0], sampleCount);
  }
  
  
  void DxbcCompiler::emitTextureQueryMsPos(const DxbcShaderInstruction& ins) {
    // samplepos has three operands:
    //    (dst0) The destination register
    //    (src0) Resource to query 
    //    (src1) Sample index
    if (m_samplePositions == 0)
      m_samplePositions = emitSamplePosArray();
    
    // The lookup index is qual to the sample count plus the
    // sample index, or 0 if the resource cannot be queried.
    DxbcRegisterValue sampleCount = emitQueryTextureSamples(ins.src[0]);
    DxbcRegisterValue sampleIndex = emitRegisterLoad(
      ins.src[1], DxbcRegMask(true, false, false, false));
    
    uint32_t lookupIndex = m_module.opIAdd(
      getVectorTypeId(sampleCount.type),
      sampleCount.id, sampleIndex.id);
    
    // Validate the parameters
    uint32_t sampleCountValid = m_module.opULessThanEqual(
      m_module.defBoolType(),
      sampleCount.id,
      m_module.constu32(16));
    
    uint32_t sampleIndexValid = m_module.opULessThan(
      m_module.defBoolType(),
      sampleIndex.id,
      sampleCount.id);
    
    // If the lookup cannot be performed, set the lookup
    // index to zero, which will return a zero vector.
    lookupIndex = m_module.opSelect(
      getVectorTypeId(sampleCount.type),
      m_module.opLogicalAnd(
        m_module.defBoolType(),
        sampleCountValid,
        sampleIndexValid),
      lookupIndex,
      m_module.constu32(0));
    
    // Load sample pos vector and write the masked
    // components to the destination register.
    DxbcRegisterPointer samplePos;
    samplePos.type.ctype  = DxbcScalarType::Float32;
    samplePos.type.ccount = 2;
    samplePos.id = m_module.opAccessChain(
      m_module.defPointerType(
        getVectorTypeId(samplePos.type),
        spv::StorageClassPrivate),
      m_samplePositions, 1, &lookupIndex);
    
    // Expand to vec4 by appending zeroes
    DxbcRegisterValue result = emitValueLoad(samplePos);

    DxbcRegisterValue zero;
    zero.type.ctype  = DxbcScalarType::Float32;
    zero.type.ccount = 2;
    zero.id = m_module.constvec2f32(0.0f, 0.0f);

    result = emitRegisterConcat(result, zero);
    
    emitRegisterStore(ins.dst[0],
      emitRegisterSwizzle(result,
        ins.src[0].swizzle,
        ins.dst[0].mask));
  }
  
  
  void DxbcCompiler::emitTextureFetch(const DxbcShaderInstruction& ins) {
    // ld has three operands:
    //    (dst0) The destination register
    //    (src0) Source address
    //    (src1) Source texture
    // ld2dms has four operands:
    //    (dst0) The destination register
    //    (src0) Source address
    //    (src1) Source texture
    //    (src2) Sample number
    const auto& texture = m_textures.at(ins.src[1].idx[0].offset);
    const uint32_t imageLayerDim = getTexLayerDim(texture.imageInfo);
    
    bool isMultisampled = ins.op == DxbcOpcode::LdMs
                       || ins.op == DxbcOpcode::LdMsS;

    // Load the texture coordinates. The last component
    // contains the LOD if the resource is an image.
    const DxbcRegisterValue address = emitRegisterLoad(
      ins.src[0], DxbcRegMask(true, true, true, true));
    
    // Additional image operands. This will store
    // the LOD and the address offset if present.
    SpirvImageOperands imageOperands;
    imageOperands.sparse = ins.dstCount == 2;
    
    if (ins.sampleControls.u != 0 || ins.sampleControls.v != 0 || ins.sampleControls.w != 0) {
      const std::array<uint32_t, 3> offsetIds = {
        imageLayerDim >= 1 ? m_module.consti32(ins.sampleControls.u) : 0,
        imageLayerDim >= 2 ? m_module.consti32(ins.sampleControls.v) : 0,
        imageLayerDim >= 3 ? m_module.consti32(ins.sampleControls.w) : 0,
      };
      
      imageOperands.flags |= spv::ImageOperandsConstOffsetMask;
      imageOperands.sConstOffset = offsetIds[0];

      if (imageLayerDim > 1) {
        imageOperands.sConstOffset = m_module.constComposite(
          getVectorTypeId({ DxbcScalarType::Sint32, imageLayerDim }),
          imageLayerDim, offsetIds.data());
      }
    }
    
    // The LOD is not present when reading from
    // a buffer or from a multisample texture.
    if (texture.imageInfo.dim != spv::DimBuffer && texture.imageInfo.ms == 0) {
      DxbcRegisterValue imageLod;
      
      if (!isMultisampled) {
        imageLod = emitRegisterExtract(
          address, DxbcRegMask(false, false, false, true));
      } else {
        // If we force-disabled MSAA, fetch from LOD 0
        imageLod.type = { DxbcScalarType::Uint32, 1 };
        imageLod.id = m_module.constu32(0);
      }
      
      imageOperands.flags |= spv::ImageOperandsLodMask;
      imageOperands.sLod = imageLod.id;
    }
    
    // The ld2dms instruction has a sample index, but we
    // are only allowed to set it for multisample views
    if (isMultisampled && texture.imageInfo.ms == 1) {
      DxbcRegisterValue sampleId = emitRegisterLoad(
        ins.src[2], DxbcRegMask(true, false, false, false));
      
      imageOperands.flags |= spv::ImageOperandsSampleMask;
      imageOperands.sSampleId = sampleId.id;
    }
    
    // Extract coordinates from address
    const DxbcRegisterValue coord = emitCalcTexCoord(address, texture.imageInfo);
    
    // Reading a typed image or buffer view
    // always returns a four-component vector.
    const uint32_t imageId = m_module.opLoad(texture.imageTypeId, texture.varId);

    DxbcVectorType texelType;
    texelType.ctype = texture.sampledType;
    texelType.ccount = 4;

    uint32_t texelTypeId = getVectorTypeId(texelType);
    uint32_t resultTypeId = texelTypeId;
    uint32_t resultId = 0;

    if (imageOperands.sparse)
      resultTypeId = getSparseResultTypeId(texelTypeId);

    resultId = m_module.opImageFetch(resultTypeId,
      imageId, coord.id, imageOperands);

    DxbcRegisterValue result;
    result.type = texelType;
    result.id = imageOperands.sparse
      ? emitExtractSparseTexel(texelTypeId, resultId)
      : resultId;
    
    // Swizzle components using the texture swizzle
    // and the destination operand's write mask
    result = emitRegisterSwizzle(result,
      ins.src[1].swizzle, ins.dst[0].mask);
    
    emitRegisterStore(ins.dst[0], result);

    if (imageOperands.sparse)
      emitStoreSparseFeedback(ins.dst[1], resultId);
  }
  
  
  void DxbcCompiler::emitTextureGather(const DxbcShaderInstruction& ins) {
    // Gather4 takes the following operands:
    //    (dst0) The destination register
    //    (dst1) The residency code for sparse ops
    //    (src0) Texture coordinates
    //    (src1) The texture itself
    //    (src2) The sampler, with a component selector
    // Gather4C takes the following additional operand:
    //    (src3) The depth reference value
    // The Gather4Po variants take an additional operand
    // which defines an extended constant offset.
    // TODO reduce code duplication by moving some common code
    // in both sample() and gather() into separate methods
    const bool isExtendedGather = ins.op == DxbcOpcode::Gather4Po
                               || ins.op == DxbcOpcode::Gather4PoC
                               || ins.op == DxbcOpcode::Gather4PoS
                               || ins.op == DxbcOpcode::Gather4PoCS;
    
    const DxbcRegister& texCoordReg = ins.src[0];
    const DxbcRegister& textureReg  = ins.src[1 + isExtendedGather];
    const DxbcRegister& samplerReg  = ins.src[2 + isExtendedGather];
    
    // Texture and sampler register IDs
    const auto& texture = m_textures.at(textureReg.idx[0].offset);
    const auto& sampler = m_samplers.at(samplerReg.idx[0].offset);
    
    // Image type, which stores the image dimensions etc.
    const uint32_t imageLayerDim = getTexLayerDim(texture.imageInfo);
    
    // Load the texture coordinates. SPIR-V allows these
    // to be float4 even if not all components are used.
    DxbcRegisterValue coord = emitLoadTexCoord(texCoordReg, texture.imageInfo);
    
    // Load reference value for depth-compare operations
    const bool isDepthCompare = ins.op == DxbcOpcode::Gather4C
                             || ins.op == DxbcOpcode::Gather4PoC
                             || ins.op == DxbcOpcode::Gather4CS
                             || ins.op == DxbcOpcode::Gather4PoCS;

    const DxbcRegisterValue referenceValue = isDepthCompare
      ? emitRegisterLoad(ins.src[3 + isExtendedGather],
          DxbcRegMask(true, false, false, false))
      : DxbcRegisterValue();
    
    // Accumulate additional image operands.
    SpirvImageOperands imageOperands;
    imageOperands.sparse = ins.dstCount == 2;

    if (isExtendedGather) {
      m_module.enableCapability(spv::CapabilityImageGatherExtended);
      
      DxbcRegisterValue gatherOffset = emitRegisterLoad(
        ins.src[1], DxbcRegMask::firstN(imageLayerDim));
      
      imageOperands.flags |= spv::ImageOperandsOffsetMask;
      imageOperands.gOffset = gatherOffset.id;
    } else if (ins.sampleControls.u != 0 || ins.sampleControls.v != 0 || ins.sampleControls.w != 0) {
      const std::array<uint32_t, 3> offsetIds = {
        imageLayerDim >= 1 ? m_module.consti32(ins.sampleControls.u) : 0,
        imageLayerDim >= 2 ? m_module.consti32(ins.sampleControls.v) : 0,
        imageLayerDim >= 3 ? m_module.consti32(ins.sampleControls.w) : 0,
      };
      
      imageOperands.flags |= spv::ImageOperandsConstOffsetMask;
      imageOperands.sConstOffset = offsetIds[0];

      if (imageLayerDim > 1) {
        imageOperands.sConstOffset = m_module.constComposite(
          getVectorTypeId({ DxbcScalarType::Sint32, imageLayerDim }),
          imageLayerDim, offsetIds.data());
      }
    }

    // Gathering texels always returns a four-component
    // vector, even for the depth-compare variants.
    uint32_t sampledImageId = emitLoadSampledImage(texture, sampler, isDepthCompare);

    DxbcVectorType texelType;
    texelType.ctype = texture.sampledType;
    texelType.ccount = 4;

    uint32_t texelTypeId = getVectorTypeId(texelType);
    uint32_t resultTypeId = texelTypeId;
    uint32_t resultId = 0;

    if (imageOperands.sparse)
      resultTypeId = getSparseResultTypeId(texelTypeId);

    if (sampledImageId) {
      switch (ins.op) {
        // Simple image gather operation
        case DxbcOpcode::Gather4:
        case DxbcOpcode::Gather4S:
        case DxbcOpcode::Gather4Po:
        case DxbcOpcode::Gather4PoS: {
          resultId = m_module.opImageGather(
            resultTypeId, sampledImageId, coord.id,
            m_module.consti32(samplerReg.swizzle[0]),
            imageOperands);
        } break;

        // Depth-compare operation
        case DxbcOpcode::Gather4C:
        case DxbcOpcode::Gather4CS:
        case DxbcOpcode::Gather4PoC:
        case DxbcOpcode::Gather4PoCS: {
          resultId = m_module.opImageDrefGather(
            resultTypeId, sampledImageId, coord.id,
            referenceValue.id, imageOperands);
        } break;

        default:
          Logger::warn(str::format(
            "DxbcCompiler: Unhandled instruction: ",
            ins.op));
          return;
      }
    } else {
      Logger::warn(str::format("DxbcCompiler: ", ins.op, ": Unsupported image type"));
      resultId = m_module.constNull(resultTypeId);
    }

    // If necessary, deal with the sparse result
    DxbcRegisterValue result;
    result.type = texelType;
    result.id = imageOperands.sparse
      ? emitExtractSparseTexel(texelTypeId, resultId)
      : resultId;

    // Swizzle components using the texture swizzle
    // and the destination operand's write mask
    result = emitRegisterSwizzle(result,
      textureReg.swizzle, ins.dst[0].mask);
    
    emitRegisterStore(ins.dst[0], result);

    if (imageOperands.sparse)
      emitStoreSparseFeedback(ins.dst[1], resultId);
  }
  
  
  void DxbcCompiler::emitTextureSample(const DxbcShaderInstruction& ins) {
    // All sample instructions have at least these operands:
    //    (dst0) The destination register
    //    (src0) Texture coordinates
    //    (src1) The texture itself
    //    (src2) The sampler object
    const DxbcRegister& texCoordReg = ins.src[0];
    const DxbcRegister& textureReg  = ins.src[1];
    const DxbcRegister& samplerReg  = ins.src[2];
    
    // Texture and sampler register IDs
    const auto& texture = m_textures.at(textureReg.idx[0].offset);
    const auto& sampler = m_samplers.at(samplerReg.idx[0].offset);
    const uint32_t imageLayerDim = getTexLayerDim(texture.imageInfo);
    
    // Load the texture coordinates. SPIR-V allows these
    // to be float4 even if not all components are used.
    DxbcRegisterValue coord = emitLoadTexCoord(texCoordReg, texture.imageInfo);
    
    // Load reference value for depth-compare operations
    const bool isDepthCompare = ins.op == DxbcOpcode::SampleC
                             || ins.op == DxbcOpcode::SampleClz
                             || ins.op == DxbcOpcode::SampleCClampS
                             || ins.op == DxbcOpcode::SampleClzS;
    
    const DxbcRegisterValue referenceValue = isDepthCompare
      ? emitRegisterLoad(ins.src[3], DxbcRegMask(true, false, false, false))
      : DxbcRegisterValue();
    
    // Load explicit gradients for sample operations that require them
    const bool hasExplicitGradients = ins.op == DxbcOpcode::SampleD
                                   || ins.op == DxbcOpcode::SampleDClampS;
    
    const DxbcRegisterValue explicitGradientX = hasExplicitGradients
      ? emitRegisterLoad(ins.src[3], DxbcRegMask::firstN(imageLayerDim))
      : DxbcRegisterValue();
    
    const DxbcRegisterValue explicitGradientY = hasExplicitGradients
      ? emitRegisterLoad(ins.src[4], DxbcRegMask::firstN(imageLayerDim))
      : DxbcRegisterValue();
    
    // LOD for certain sample operations
    const bool hasLod = ins.op == DxbcOpcode::SampleL
                     || ins.op == DxbcOpcode::SampleLS
                     || ins.op == DxbcOpcode::SampleB
                     || ins.op == DxbcOpcode::SampleBClampS;
    
    const DxbcRegisterValue lod = hasLod
      ? emitRegisterLoad(ins.src[3], DxbcRegMask(true, false, false, false))
      : DxbcRegisterValue();

    // Min LOD for certain sparse operations
    const bool hasMinLod = ins.op == DxbcOpcode::SampleClampS
                        || ins.op == DxbcOpcode::SampleBClampS
                        || ins.op == DxbcOpcode::SampleDClampS
                        || ins.op == DxbcOpcode::SampleCClampS;

    const DxbcRegisterValue minLod = hasMinLod && ins.src[ins.srcCount - 1].type != DxbcOperandType::Null
      ? emitRegisterLoad(ins.src[ins.srcCount - 1], DxbcRegMask(true, false, false, false))
      : DxbcRegisterValue();

    // Accumulate additional image operands. These are
    // not part of the actual operand token in SPIR-V.
    SpirvImageOperands imageOperands;
    imageOperands.sparse = ins.dstCount == 2;

    if (ins.sampleControls.u != 0 || ins.sampleControls.v != 0 || ins.sampleControls.w != 0) {
      const std::array<uint32_t, 3> offsetIds = {
        imageLayerDim >= 1 ? m_module.consti32(ins.sampleControls.u) : 0,
        imageLayerDim >= 2 ? m_module.consti32(ins.sampleControls.v) : 0,
        imageLayerDim >= 3 ? m_module.consti32(ins.sampleControls.w) : 0,
      };
      
      imageOperands.flags |= spv::ImageOperandsConstOffsetMask;
      imageOperands.sConstOffset = offsetIds[0];

      if (imageLayerDim > 1) {
        imageOperands.sConstOffset = m_module.constComposite(
          getVectorTypeId({ DxbcScalarType::Sint32, imageLayerDim }),
          imageLayerDim, offsetIds.data());
      }
    }

    if (hasMinLod) {
      m_module.enableCapability(spv::CapabilityMinLod);

      imageOperands.flags |= spv::ImageOperandsMinLodMask;
      imageOperands.sMinLod = minLod.id;
    }

    // Combine the texture and the sampler into a sampled image
    uint32_t sampledImageId = emitLoadSampledImage(texture, sampler, isDepthCompare);
    
    // Sampling an image always returns a four-component
    // vector, whereas depth-compare ops return a scalar.
    DxbcVectorType texelType;
    texelType.ctype  = texture.sampledType;
    texelType.ccount = isDepthCompare ? 1 : 4;

    uint32_t texelTypeId = getVectorTypeId(texelType);
    uint32_t resultTypeId = texelTypeId;
    uint32_t resultId = 0;

    if (imageOperands.sparse)
      resultTypeId = getSparseResultTypeId(texelTypeId);

    if (sampledImageId) {
      switch (ins.op) {
        // Simple image sample operation
        case DxbcOpcode::Sample:
        case DxbcOpcode::SampleClampS: {
          resultId = m_module.opImageSampleImplicitLod(
            resultTypeId, sampledImageId, coord.id,
            imageOperands);
        } break;

        // Depth-compare operation
        case DxbcOpcode::SampleC:
        case DxbcOpcode::SampleCClampS: {
          resultId = m_module.opImageSampleDrefImplicitLod(
            resultTypeId, sampledImageId, coord.id,
            referenceValue.id, imageOperands);
        } break;

        // Depth-compare operation on mip level zero
        case DxbcOpcode::SampleClz:
        case DxbcOpcode::SampleClzS: {
          imageOperands.flags |= spv::ImageOperandsLodMask;
          imageOperands.sLod = m_module.constf32(0.0f);

          resultId = m_module.opImageSampleDrefExplicitLod(
            resultTypeId, sampledImageId, coord.id,
            referenceValue.id, imageOperands);
        } break;

        // Sample operation with explicit gradients
        case DxbcOpcode::SampleD:
        case DxbcOpcode::SampleDClampS: {
          imageOperands.flags |= spv::ImageOperandsGradMask;
          imageOperands.sGradX = explicitGradientX.id;
          imageOperands.sGradY = explicitGradientY.id;

          resultId = m_module.opImageSampleExplicitLod(
            resultTypeId, sampledImageId, coord.id,
            imageOperands);
        } break;

        // Sample operation with explicit LOD
        case DxbcOpcode::SampleL:
        case DxbcOpcode::SampleLS: {
          imageOperands.flags |= spv::ImageOperandsLodMask;
          imageOperands.sLod = lod.id;

          resultId = m_module.opImageSampleExplicitLod(
            resultTypeId, sampledImageId, coord.id,
            imageOperands);
        } break;

        // Sample operation with LOD bias
        case DxbcOpcode::SampleB:
        case DxbcOpcode::SampleBClampS: {
          imageOperands.flags |= spv::ImageOperandsBiasMask;
          imageOperands.sLodBias = lod.id;

          resultId = m_module.opImageSampleImplicitLod(
            resultTypeId, sampledImageId, coord.id,
            imageOperands);
        } break;

        default:
          Logger::warn(str::format(
            "DxbcCompiler: Unhandled instruction: ",
            ins.op));
          return;
      }
    } else {
      Logger::warn(str::format("DxbcCompiler: ", ins.op, ": Unsupported image type"));
      resultId = m_module.constNull(resultTypeId);
    }
    
    DxbcRegisterValue result;
    result.type = texelType;
    result.id = imageOperands.sparse
      ? emitExtractSparseTexel(texelTypeId, resultId)
      : resultId;

    // Swizzle components using the texture swizzle
    // and the destination operand's write mask
    if (result.type.ccount != 1) {
      result = emitRegisterSwizzle(result,
        textureReg.swizzle, ins.dst[0].mask);
    }
    
    emitRegisterStore(ins.dst[0], result);

    if (imageOperands.sparse)
      emitStoreSparseFeedback(ins.dst[1], resultId);
  }
  
  
  void DxbcCompiler::emitTypedUavLoad(const DxbcShaderInstruction& ins) {
    // load_uav_typed has three operands:
    //    (dst0) The destination register
    //    (src0) The texture or buffer coordinates
    //    (src1) The UAV to load from
    const uint32_t registerId = ins.src[1].idx[0].offset;
    const DxbcUav uavInfo = m_uavs.at(registerId);

    emitUavBarrier(uint64_t(1u) << registerId, 0u);

    // Load texture coordinates
    DxbcRegisterValue texCoord = emitLoadTexCoord(
      ins.src[0], uavInfo.imageInfo);

    SpirvImageOperands imageOperands;
    imageOperands.sparse = ins.dstCount == 2;

    if (uavInfo.coherence) {
      imageOperands.flags |= spv::ImageOperandsNonPrivateTexelMask
                          |  spv::ImageOperandsMakeTexelVisibleMask;
      imageOperands.makeVisible = m_module.constu32(uavInfo.coherence);
    }

    DxbcVectorType texelType;
    texelType.ctype = uavInfo.sampledType;
    texelType.ccount = 4;

    uint32_t texelTypeId = getVectorTypeId(texelType);
    uint32_t resultTypeId = texelTypeId;
    uint32_t resultId = 0;

    if (imageOperands.sparse)
      resultTypeId = getSparseResultTypeId(texelTypeId);

    // Load source value from the UAV
    resultId = m_module.opImageRead(resultTypeId,
      m_module.opLoad(uavInfo.imageTypeId, uavInfo.varId),
      texCoord.id, imageOperands);
    
    // Apply component swizzle and mask
    DxbcRegisterValue uavValue;
    uavValue.type = texelType;
    uavValue.id = imageOperands.sparse
      ? emitExtractSparseTexel(texelTypeId, resultId)
      : resultId;

    uavValue = emitRegisterSwizzle(uavValue,
      ins.src[1].swizzle, ins.dst[0].mask);
    
    emitRegisterStore(ins.dst[0], uavValue);

    if (imageOperands.sparse)
      emitStoreSparseFeedback(ins.dst[1], resultId);
  }
  
  
  void DxbcCompiler::emitTypedUavStore(const DxbcShaderInstruction& ins) {
    // store_uav_typed has three operands:
    //    (dst0) The destination UAV
    //    (src0) The texture or buffer coordinates
    //    (src1) The value to store
    const DxbcBufferInfo uavInfo = getBufferInfo(ins.dst[0]);
    emitUavBarrier(0u, uint64_t(1u) << ins.dst[0].idx[0].offset);

    // Set image operands for coherent access if necessary    
    SpirvImageOperands imageOperands;

    if (uavInfo.coherence) {
      imageOperands.flags |= spv::ImageOperandsNonPrivateTexelMask
                          |  spv::ImageOperandsMakeTexelAvailableMask;
      imageOperands.makeAvailable = m_module.constu32(uavInfo.coherence);
    }

    // Load texture coordinates
    DxbcRegisterValue texCoord = emitLoadTexCoord(ins.src[0], uavInfo.image);

    // Load the value that will be written to the image. We'll
    // have to cast it to the component type of the image.
    const DxbcRegisterValue texValue = emitRegisterBitcast(
      emitRegisterLoad(ins.src[1], DxbcRegMask(true, true, true, true)),
      uavInfo.stype);
    
    // Write the given value to the image
    m_module.opImageWrite(
      m_module.opLoad(uavInfo.typeId, uavInfo.varId),
      texCoord.id, texValue.id, imageOperands);
  }
  
  
  void DxbcCompiler::emitControlFlowIf(const DxbcShaderInstruction& ins) {
    // Load the first component of the condition
    // operand and perform a zero test on it.
    const DxbcRegisterValue condition = emitRegisterLoad(
      ins.src[0], DxbcRegMask(true, false, false, false));
    
    // Declare the 'if' block. We do not know if there
    // will be an 'else' block or not, so we'll assume
    // that there is one and leave it empty otherwise.
    DxbcCfgBlock block;
    block.type = DxbcCfgBlockType::If;
    block.b_if.ztestId   = emitRegisterZeroTest(condition, ins.controls.zeroTest()).id;
    block.b_if.labelIf   = m_module.allocateId();
    block.b_if.labelElse = 0;
    block.b_if.labelEnd  = m_module.allocateId();
    block.b_if.headerPtr = m_module.getInsertionPtr();
    m_controlFlowBlocks.push_back(block);
    
    // We'll insert the branch instruction when closing
    // the block, since we don't know whether or not an
    // else block is needed right now.
    m_module.opLabel(block.b_if.labelIf);
  }
  
  
  void DxbcCompiler::emitControlFlowElse(const DxbcShaderInstruction& ins) {
    if (m_controlFlowBlocks.size() == 0
     || m_controlFlowBlocks.back().type != DxbcCfgBlockType::If
     || m_controlFlowBlocks.back().b_if.labelElse != 0)
      throw DxvkError("DxbcCompiler: 'Else' without 'If' found");
    
    // Set the 'Else' flag so that we do
    // not insert a dummy block on 'EndIf'
    DxbcCfgBlock& block = m_controlFlowBlocks.back();
    block.b_if.labelElse = m_module.allocateId();
    
    // Close the 'If' block by branching to
    // the merge block we declared earlier
    m_module.opBranch(block.b_if.labelEnd);
    m_module.opLabel (block.b_if.labelElse);
  }
  
  
  void DxbcCompiler::emitControlFlowEndIf(const DxbcShaderInstruction& ins) {
    if (m_controlFlowBlocks.size() == 0
     || m_controlFlowBlocks.back().type != DxbcCfgBlockType::If)
      throw DxvkError("DxbcCompiler: 'EndIf' without 'If' found");
    
    // Remove the block from the stack, it's closed
    DxbcCfgBlock block = m_controlFlowBlocks.back();
    m_controlFlowBlocks.pop_back();
    
    // Write out the 'if' header
    m_module.beginInsertion(block.b_if.headerPtr);
    
    m_module.opSelectionMerge(
      block.b_if.labelEnd,
      spv::SelectionControlMaskNone);
    
    m_module.opBranchConditional(
      block.b_if.ztestId,
      block.b_if.labelIf,
      block.b_if.labelElse != 0
        ? block.b_if.labelElse
        : block.b_if.labelEnd);
    
    m_module.endInsertion();
    
    // End the active 'if' or 'else' block
    m_module.opBranch(block.b_if.labelEnd);
    m_module.opLabel (block.b_if.labelEnd);
  }
  
  
  void DxbcCompiler::emitControlFlowSwitch(const DxbcShaderInstruction& ins) {
    // Load the selector as a scalar unsigned integer
    const DxbcRegisterValue selector = emitRegisterLoad(
      ins.src[0], DxbcRegMask(true, false, false, false));
    
    // Declare switch block. We cannot insert the switch
    // instruction itself yet because the number of case
    // statements and blocks is unknown at this point.
    DxbcCfgBlock block;
    block.type = DxbcCfgBlockType::Switch;
    block.b_switch.insertPtr    = m_module.getInsertionPtr();
    block.b_switch.selectorId   = selector.id;
    block.b_switch.labelBreak   = m_module.allocateId();
    block.b_switch.labelCase    = m_module.allocateId();
    block.b_switch.labelDefault = 0;
    block.b_switch.labelCases   = nullptr;
    m_controlFlowBlocks.push_back(block);
    
    // Define the first 'case' label
    m_module.opLabel(block.b_switch.labelCase);
  }
  
  
  void DxbcCompiler::emitControlFlowCase(const DxbcShaderInstruction& ins) {
    if (m_controlFlowBlocks.size() == 0
     || m_controlFlowBlocks.back().type != DxbcCfgBlockType::Switch)
      throw DxvkError("DxbcCompiler: 'Case' without 'Switch' found");
    
    // The source operand must be a 32-bit immediate.
    if (ins.src[0].type != DxbcOperandType::Imm32)
      throw DxvkError("DxbcCompiler: Invalid operand type for 'Case'");

    // Use the last label allocated for 'case'.
    DxbcCfgBlockSwitch* block = &m_controlFlowBlocks.back().b_switch;

    if (caseBlockIsFallthrough()) {
      block->labelCase = m_module.allocateId();

      m_module.opBranch(block->labelCase);
      m_module.opLabel (block->labelCase);
    }

    DxbcSwitchLabel label;
    label.desc.literal = ins.src[0].imm.u32_1;
    label.desc.labelId = block->labelCase;
    label.next         = block->labelCases;
    block->labelCases = new DxbcSwitchLabel(label);
  }
  
  
  void DxbcCompiler::emitControlFlowDefault(const DxbcShaderInstruction& ins) {
    if (m_controlFlowBlocks.size() == 0
     || m_controlFlowBlocks.back().type != DxbcCfgBlockType::Switch)
      throw DxvkError("DxbcCompiler: 'Default' without 'Switch' found");
    
    DxbcCfgBlockSwitch* block = &m_controlFlowBlocks.back().b_switch;

    if (caseBlockIsFallthrough()) {
      block->labelCase = m_module.allocateId();

      m_module.opBranch(block->labelCase);
      m_module.opLabel (block->labelCase);
    }

    // Set the last label allocated for 'case' as the default label.
    block->labelDefault = block->labelCase;
  }
  
  
  void DxbcCompiler::emitControlFlowEndSwitch(const DxbcShaderInstruction& ins) {
    if (m_controlFlowBlocks.size() == 0
     || m_controlFlowBlocks.back().type != DxbcCfgBlockType::Switch)
      throw DxvkError("DxbcCompiler: 'EndSwitch' without 'Switch' found");
    
    // Remove the block from the stack, it's closed
    DxbcCfgBlock block = m_controlFlowBlocks.back();
    m_controlFlowBlocks.pop_back();

    if (!block.b_switch.labelDefault) {
      block.b_switch.labelDefault = caseBlockIsFallthrough()
        ? block.b_switch.labelBreak
        : block.b_switch.labelCase;
    }
    
    // Close the current 'case' block
    m_module.opBranch(block.b_switch.labelBreak);
    
    // Insert the 'switch' statement. For that, we need to
    // gather all the literal-label pairs for the construct.
    m_module.beginInsertion(block.b_switch.insertPtr);
    m_module.opSelectionMerge(
      block.b_switch.labelBreak,
      spv::SelectionControlMaskNone);
    
    // We'll restore the original order of the case labels here
    std::vector<SpirvSwitchCaseLabel> jumpTargets;
    for (auto i = block.b_switch.labelCases; i != nullptr; i = i->next)
      jumpTargets.insert(jumpTargets.begin(), i->desc);
    
    m_module.opSwitch(
      block.b_switch.selectorId,
      block.b_switch.labelDefault,
      jumpTargets.size(),
      jumpTargets.data());
    m_module.endInsertion();
    
    // Destroy the list of case labels
    // FIXME we're leaking memory if compilation fails.
    DxbcSwitchLabel* caseLabel = block.b_switch.labelCases;
    
    while (caseLabel != nullptr)
      delete std::exchange(caseLabel, caseLabel->next);

    // Begin new block after switch blocks
    m_module.opLabel(block.b_switch.labelBreak);
  }
  
    
  void DxbcCompiler::emitControlFlowLoop(const DxbcShaderInstruction& ins) {
    // Declare the 'loop' block
    DxbcCfgBlock block;
    block.type = DxbcCfgBlockType::Loop;
    block.b_loop.labelHeader   = m_module.allocateId();
    block.b_loop.labelBegin    = m_module.allocateId();
    block.b_loop.labelContinue = m_module.allocateId();
    block.b_loop.labelBreak    = m_module.allocateId();
    m_controlFlowBlocks.push_back(block);
    
    m_module.opBranch(block.b_loop.labelHeader);
    m_module.opLabel (block.b_loop.labelHeader);
    
    m_module.opLoopMerge(
      block.b_loop.labelBreak,
      block.b_loop.labelContinue,
      spv::LoopControlMaskNone);
    
    m_module.opBranch(block.b_loop.labelBegin);
    m_module.opLabel (block.b_loop.labelBegin);
  }
  
  
  void DxbcCompiler::emitControlFlowEndLoop(const DxbcShaderInstruction& ins) {
    if (m_controlFlowBlocks.size() == 0
     || m_controlFlowBlocks.back().type != DxbcCfgBlockType::Loop)
      throw DxvkError("DxbcCompiler: 'EndLoop' without 'Loop' found");
    
    // Remove the block from the stack, it's closed
    const DxbcCfgBlock block = m_controlFlowBlocks.back();
    m_controlFlowBlocks.pop_back();
    
    // Declare the continue block
    m_module.opBranch(block.b_loop.labelContinue);
    m_module.opLabel (block.b_loop.labelContinue);
    
    // Declare the merge block
    m_module.opBranch(block.b_loop.labelHeader);
    m_module.opLabel (block.b_loop.labelBreak);
  }
  
  
  void DxbcCompiler::emitControlFlowBreak(const DxbcShaderInstruction& ins) {
    const bool isBreak = ins.op == DxbcOpcode::Break;
    
    DxbcCfgBlock* cfgBlock = isBreak
      ? cfgFindBlock({ DxbcCfgBlockType::Loop, DxbcCfgBlockType::Switch })
      : cfgFindBlock({ DxbcCfgBlockType::Loop });
    
    if (cfgBlock == nullptr)
      throw DxvkError("DxbcCompiler: 'Break' or 'Continue' outside 'Loop' or 'Switch' found");
    
    if (cfgBlock->type == DxbcCfgBlockType::Loop) {
      m_module.opBranch(isBreak
        ? cfgBlock->b_loop.labelBreak
        : cfgBlock->b_loop.labelContinue);
    } else /* if (cfgBlock->type == DxbcCfgBlockType::Switch) */ {
      m_module.opBranch(cfgBlock->b_switch.labelBreak);
    }
    
    // Subsequent instructions assume that there is an open block
    const uint32_t labelId = m_module.allocateId();
    m_module.opLabel(labelId);
    
    // If this is on the same level as a switch-case construct,
    // rather than being nested inside an 'if' statement, close
    // the current 'case' block.
    if (m_controlFlowBlocks.back().type == DxbcCfgBlockType::Switch)
      cfgBlock->b_switch.labelCase = labelId;
  }
  
  
  void DxbcCompiler::emitControlFlowBreakc(const DxbcShaderInstruction& ins) {
    const bool isBreak = ins.op == DxbcOpcode::Breakc;
    
    DxbcCfgBlock* cfgBlock = isBreak
      ? cfgFindBlock({ DxbcCfgBlockType::Loop, DxbcCfgBlockType::Switch })
      : cfgFindBlock({ DxbcCfgBlockType::Loop });
    
    if (cfgBlock == nullptr)
      throw DxvkError("DxbcCompiler: 'Breakc' or 'Continuec' outside 'Loop' or 'Switch' found");
    
    // Perform zero test on the first component of the condition
    const DxbcRegisterValue condition = emitRegisterLoad(
      ins.src[0], DxbcRegMask(true, false, false, false));
    
    const DxbcRegisterValue zeroTest = emitRegisterZeroTest(
      condition, ins.controls.zeroTest());
    
    // We basically have to wrap this into an 'if' block
    const uint32_t breakBlock = m_module.allocateId();
    const uint32_t mergeBlock = m_module.allocateId();
    
    m_module.opSelectionMerge(mergeBlock,
      spv::SelectionControlMaskNone);
    
    m_module.opBranchConditional(
      zeroTest.id, breakBlock, mergeBlock);
    
    m_module.opLabel(breakBlock);
    
    if (cfgBlock->type == DxbcCfgBlockType::Loop) {
      m_module.opBranch(isBreak
        ? cfgBlock->b_loop.labelBreak
        : cfgBlock->b_loop.labelContinue);
    } else /* if (cfgBlock->type == DxbcCfgBlockType::Switch) */ {
      m_module.opBranch(cfgBlock->b_switch.labelBreak);
    }
    
    m_module.opLabel(mergeBlock);
  }
  
  
  void DxbcCompiler::emitControlFlowRet(const DxbcShaderInstruction& ins) {
    if (m_controlFlowBlocks.size() != 0) {
      uint32_t labelId = m_module.allocateId();
      
      m_module.opReturn();
      m_module.opLabel(labelId);

      // return can be used in place of break to terminate a case block
      if (m_controlFlowBlocks.back().type == DxbcCfgBlockType::Switch)
        m_controlFlowBlocks.back().b_switch.labelCase = labelId;

      m_topLevelIsUniform = false;
    } else {
      // Last instruction in the current function
      this->emitFunctionEnd();
    }
  }


  void DxbcCompiler::emitControlFlowRetc(const DxbcShaderInstruction& ins) {
    // Perform zero test on the first component of the condition
    const DxbcRegisterValue condition = emitRegisterLoad(
      ins.src[0], DxbcRegMask(true, false, false, false));
    
    const DxbcRegisterValue zeroTest = emitRegisterZeroTest(
      condition, ins.controls.zeroTest());
    
    // We basically have to wrap this into an 'if' block
    const uint32_t returnLabel = m_module.allocateId();
    const uint32_t continueLabel = m_module.allocateId();
    
    m_module.opSelectionMerge(continueLabel,
      spv::SelectionControlMaskNone);
    
    m_module.opBranchConditional(
      zeroTest.id, returnLabel, continueLabel);
    
    m_module.opLabel(returnLabel);
    m_module.opReturn();

    m_module.opLabel(continueLabel);

    // The return condition may be non-uniform
    m_topLevelIsUniform = false;
  }
  
  
  void DxbcCompiler::emitControlFlowDiscard(const DxbcShaderInstruction& ins) {
    // Discard actually has an operand that determines
    // whether or not the fragment should be discarded
    const DxbcRegisterValue condition = emitRegisterLoad(
      ins.src[0], DxbcRegMask(true, false, false, false));
    
    const DxbcRegisterValue zeroTest = emitRegisterZeroTest(
      condition, ins.controls.zeroTest());
    
    DxbcConditional cond;
    cond.labelIf  = m_module.allocateId();
    cond.labelEnd = m_module.allocateId();
    
    m_module.opSelectionMerge(cond.labelEnd, spv::SelectionControlMaskNone);
    m_module.opBranchConditional(zeroTest.id, cond.labelIf, cond.labelEnd);
    
    m_module.opLabel(cond.labelIf);
    m_module.opDemoteToHelperInvocation();
    m_module.opBranch(cond.labelEnd);
    
    m_module.opLabel(cond.labelEnd);

    m_module.enableCapability(spv::CapabilityDemoteToHelperInvocation);

    // Discard is just retc in a trenchcoat
    m_topLevelIsUniform = false;
  }

  
  void DxbcCompiler::emitControlFlowLabel(const DxbcShaderInstruction& ins) {
    uint32_t functionNr = ins.dst[0].idx[0].offset;
    uint32_t functionId = getFunctionId(functionNr);
    
    this->emitFunctionBegin(
      functionId,
      m_module.defVoidType(),
      m_module.defFunctionType(
        m_module.defVoidType(), 0, nullptr));
    
    m_module.opLabel(m_module.allocateId());
    m_module.setDebugName(functionId, str::format("label", functionNr).c_str());
    
    m_insideFunction = true;

    // We have to assume that this function gets
    // called from non-uniform control flow
    m_topLevelIsUniform = false;
  }

  
  void DxbcCompiler::emitControlFlowCall(const DxbcShaderInstruction& ins) {
    uint32_t functionNr = ins.src[0].idx[0].offset;
    uint32_t functionId = getFunctionId(functionNr);

    m_module.opFunctionCall(
      m_module.defVoidType(),
      functionId, 0, nullptr);
  }

  
  void DxbcCompiler::emitControlFlowCallc(const DxbcShaderInstruction& ins) {
    uint32_t functionNr = ins.src[1].idx[0].offset;
    uint32_t functionId = getFunctionId(functionNr);

    // Perform zero test on the first component of the condition
    const DxbcRegisterValue condition = emitRegisterLoad(
      ins.src[0], DxbcRegMask(true, false, false, false));
    
    const DxbcRegisterValue zeroTest = emitRegisterZeroTest(
      condition, ins.controls.zeroTest());
    
    // We basically have to wrap this into an 'if' block
    const uint32_t callLabel = m_module.allocateId();
    const uint32_t skipLabel = m_module.allocateId();
    
    m_module.opSelectionMerge(skipLabel,
      spv::SelectionControlMaskNone);
    
    m_module.opBranchConditional(
      zeroTest.id, callLabel, skipLabel);
    
    m_module.opLabel(callLabel);
    m_module.opFunctionCall(
      m_module.defVoidType(),
      functionId, 0, nullptr);

    m_module.opBranch(skipLabel);
    m_module.opLabel(skipLabel);
  }

  
  void DxbcCompiler::emitControlFlow(const DxbcShaderInstruction& ins) {
    switch (ins.op) {
      case DxbcOpcode::If:
        this->emitUavBarrier(0, 0);
        this->emitControlFlowIf(ins);
        break;
        
      case DxbcOpcode::Else:
        this->emitControlFlowElse(ins);
        break;
        
      case DxbcOpcode::EndIf:
        this->emitControlFlowEndIf(ins);
        this->emitUavBarrier(0, 0);
        break;
        
      case DxbcOpcode::Switch:
        this->emitUavBarrier(0, 0);
        this->emitControlFlowSwitch(ins);
        break;
        
      case DxbcOpcode::Case:
        this->emitControlFlowCase(ins);
        break;
        
      case DxbcOpcode::Default:
        this->emitControlFlowDefault(ins);
        break;
        
      case DxbcOpcode::EndSwitch:
        this->emitControlFlowEndSwitch(ins);
        this->emitUavBarrier(0, 0);
        break;

      case DxbcOpcode::Loop:
        this->emitUavBarrier(0, 0);
        this->emitControlFlowLoop(ins);
        break;
        
      case DxbcOpcode::EndLoop:
        this->emitControlFlowEndLoop(ins);
        this->emitUavBarrier(0, 0);
        break;

      case DxbcOpcode::Break:
      case DxbcOpcode::Continue:
        this->emitControlFlowBreak(ins);
        break;
        
      case DxbcOpcode::Breakc:
      case DxbcOpcode::Continuec:
        this->emitControlFlowBreakc(ins);
        break;

      case DxbcOpcode::Ret:
        this->emitControlFlowRet(ins);
        break;

      case DxbcOpcode::Retc:
        this->emitUavBarrier(0, 0);
        this->emitControlFlowRetc(ins);
        break;
        
      case DxbcOpcode::Discard:
        this->emitControlFlowDiscard(ins);
        break;
      
      case DxbcOpcode::Label:
        this->emitControlFlowLabel(ins);
        break;

      case DxbcOpcode::Call:
        this->emitUavBarrier(0, 0);
        this->emitControlFlowCall(ins);
        this->emitUavBarrier(-1, -1);
        break;

      case DxbcOpcode::Callc:
        this->emitUavBarrier(0, 0);
        this->emitControlFlowCallc(ins);
        this->emitUavBarrier(-1, -1);
        break;

      default:
        Logger::warn(str::format(
          "DxbcCompiler: Unhandled instruction: ",
          ins.op));
    }
  }
  
  
  DxbcRegisterValue DxbcCompiler::emitBuildConstVecf32(
          float                   x,
          float                   y,
          float                   z,
          float                   w,
    const DxbcRegMask&            writeMask) {
    // TODO refactor these functions into one single template
    std::array<uint32_t, 4> ids = { 0, 0, 0, 0 };
    uint32_t componentIndex = 0;
    
    if (writeMask[0]) ids[componentIndex++] = m_module.constf32(x);
    if (writeMask[1]) ids[componentIndex++] = m_module.constf32(y);
    if (writeMask[2]) ids[componentIndex++] = m_module.constf32(z);
    if (writeMask[3]) ids[componentIndex++] = m_module.constf32(w);
    
    DxbcRegisterValue result;
    result.type.ctype  = DxbcScalarType::Float32;
    result.type.ccount = componentIndex;
    result.id = componentIndex > 1
      ? m_module.constComposite(
          getVectorTypeId(result.type),
          componentIndex, ids.data())
      : ids[0];
    return result;
  }
  
  
  DxbcRegisterValue DxbcCompiler::emitBuildConstVecu32(
          uint32_t                x,
          uint32_t                y,
          uint32_t                z,
          uint32_t                w,
    const DxbcRegMask&            writeMask) {
    std::array<uint32_t, 4> ids = { 0, 0, 0, 0 };
    uint32_t componentIndex = 0;
    
    if (writeMask[0]) ids[componentIndex++] = m_module.constu32(x);
    if (writeMask[1]) ids[componentIndex++] = m_module.constu32(y);
    if (writeMask[2]) ids[componentIndex++] = m_module.constu32(z);
    if (writeMask[3]) ids[componentIndex++] = m_module.constu32(w);
    
    DxbcRegisterValue result;
    result.type.ctype  = DxbcScalarType::Uint32;
    result.type.ccount = componentIndex;
    result.id = componentIndex > 1
      ? m_module.constComposite(
          getVectorTypeId(result.type),
          componentIndex, ids.data())
      : ids[0];
    return result;
  }
  
  
  DxbcRegisterValue DxbcCompiler::emitBuildConstVeci32(
          int32_t                 x,
          int32_t                 y,
          int32_t                 z,
          int32_t                 w,
    const DxbcRegMask&            writeMask) {
    std::array<uint32_t, 4> ids = { 0, 0, 0, 0 };
    uint32_t componentIndex = 0;
    
    if (writeMask[0]) ids[componentIndex++] = m_module.consti32(x);
    if (writeMask[1]) ids[componentIndex++] = m_module.consti32(y);
    if (writeMask[2]) ids[componentIndex++] = m_module.consti32(z);
    if (writeMask[3]) ids[componentIndex++] = m_module.consti32(w);
    
    DxbcRegisterValue result;
    result.type.ctype  = DxbcScalarType::Sint32;
    result.type.ccount = componentIndex;
    result.id = componentIndex > 1
      ? m_module.constComposite(
          getVectorTypeId(result.type),
          componentIndex, ids.data())
      : ids[0];
    return result;
  }
  
  
  DxbcRegisterValue DxbcCompiler::emitBuildConstVecf64(
          double                  xy,
          double                  zw,
    const DxbcRegMask&            writeMask) {
    std::array<uint32_t, 2> ids = { 0, 0 };
    uint32_t componentIndex = 0;
    
    if (writeMask[0] && writeMask[1]) ids[componentIndex++] = m_module.constf64(xy);
    if (writeMask[2] && writeMask[3]) ids[componentIndex++] = m_module.constf64(zw);
    
    DxbcRegisterValue result;
    result.type.ctype  = DxbcScalarType::Float64;
    result.type.ccount = componentIndex;
    result.id = componentIndex > 1
      ? m_module.constComposite(
          getVectorTypeId(result.type),
          componentIndex, ids.data())
      : ids[0];
    return result;
  }
  
  
  DxbcRegisterValue DxbcCompiler::emitBuildVector(
          DxbcRegisterValue       scalar,
          uint32_t                count) {
    if (count == 1)
      return scalar;

    std::array<uint32_t, 4> scalarIds =
      { scalar.id, scalar.id, scalar.id, scalar.id };

    DxbcRegisterValue result;
    result.type.ctype = scalar.type.ctype;
    result.type.ccount = count;
    result.id = m_module.constComposite(
      getVectorTypeId(result.type),
      count, scalarIds.data());
    return result;
  }


  DxbcRegisterValue DxbcCompiler::emitBuildZeroVector(
          DxbcVectorType          type) {
    DxbcRegisterValue result;
    result.type.ctype = type.ctype;
    result.type.ccount = 1;

    switch (type.ctype) {
      case DxbcScalarType::Float32: result.id = m_module.constf32(0.0f); break;
      case DxbcScalarType::Uint32:  result.id = m_module.constu32(0u); break;
      case DxbcScalarType::Sint32:  result.id = m_module.consti32(0); break;
      default: throw DxvkError("DxbcCompiler: Invalid scalar type");
    }

    return emitBuildVector(result, type.ccount);
  }


  DxbcRegisterValue DxbcCompiler::emitRegisterBitcast(
          DxbcRegisterValue       srcValue,
          DxbcScalarType          dstType) {
    DxbcScalarType srcType = srcValue.type.ctype;

    if (srcType == dstType)
      return srcValue;
    
    DxbcRegisterValue result;
    result.type.ctype  = dstType;
    result.type.ccount = srcValue.type.ccount;

    if (isDoubleType(srcType)) result.type.ccount *= 2;
    if (isDoubleType(dstType)) result.type.ccount /= 2;

    result.id = m_module.opBitcast(
      getVectorTypeId(result.type),
      srcValue.id);
    return result;
  }
  
  
  DxbcRegisterValue DxbcCompiler::emitRegisterSwizzle(
          DxbcRegisterValue       value,
          DxbcRegSwizzle          swizzle,
          DxbcRegMask             writeMask) {
    if (value.type.ccount == 1)
      return emitRegisterExtend(value, writeMask.popCount());
    
    std::array<uint32_t, 4> indices;
    
    uint32_t dstIndex = 0;
    
    for (uint32_t i = 0; i < 4; i++) {
      if (writeMask[i])
        indices[dstIndex++] = swizzle[i];
    }
    
    // If the swizzle combined with the mask can be reduced
    // to a no-op, we don't need to insert any instructions.
    bool isIdentitySwizzle = dstIndex == value.type.ccount;
    
    for (uint32_t i = 0; i < dstIndex && isIdentitySwizzle; i++)
      isIdentitySwizzle &= indices[i] == i;
    
    if (isIdentitySwizzle)
      return value;
    
    // Use OpCompositeExtract if the resulting vector contains
    // only one component, and OpVectorShuffle if it is a vector.
    DxbcRegisterValue result;
    result.type.ctype  = value.type.ctype;
    result.type.ccount = dstIndex;
    
    const uint32_t typeId = getVectorTypeId(result.type);
    
    if (dstIndex == 1) {
      result.id = m_module.opCompositeExtract(
        typeId, value.id, 1, indices.data());
    } else {
      result.id = m_module.opVectorShuffle(
        typeId, value.id, value.id,
        dstIndex, indices.data());
    }
    
    return result;
  }
  
  
  DxbcRegisterValue DxbcCompiler::emitRegisterExtract(
          DxbcRegisterValue       value,
          DxbcRegMask             mask) {
    return emitRegisterSwizzle(value,
      DxbcRegSwizzle(0, 1, 2, 3), mask);
  }
  
  
  DxbcRegisterValue DxbcCompiler::emitRegisterInsert(
          DxbcRegisterValue       dstValue,
          DxbcRegisterValue       srcValue,
          DxbcRegMask             srcMask) {
    DxbcRegisterValue result;
    result.type = dstValue.type;
    
    const uint32_t typeId = getVectorTypeId(result.type);
    
    if (srcMask.popCount() == 0) {
      // Nothing to do if the insertion mask is empty
      result.id = dstValue.id;
    } else if (dstValue.type.ccount == 1) {
      // Both values are scalar, so the first component
      // of the write mask decides which one to take.
      result.id = srcMask[0] ? srcValue.id : dstValue.id;
    } else if (srcValue.type.ccount == 1) {
      // The source value is scalar. Since OpVectorShuffle
      // requires both arguments to be vectors, we have to
      // use OpCompositeInsert to modify the vector instead.
      const uint32_t componentId = srcMask.firstSet();
      
      result.id = m_module.opCompositeInsert(typeId,
        srcValue.id, dstValue.id, 1, &componentId);
    } else {
      // Both arguments are vectors. We can determine which
      // components to take from which vector and use the
      // OpVectorShuffle instruction.
      std::array<uint32_t, 4> components;
      uint32_t srcComponentId = dstValue.type.ccount;
      
      for (uint32_t i = 0; i < dstValue.type.ccount; i++)
        components.at(i) = srcMask[i] ? srcComponentId++ : i;
      
      result.id = m_module.opVectorShuffle(
        typeId, dstValue.id, srcValue.id,
        dstValue.type.ccount, components.data());
    }
    
    return result;
  }
  
  
  DxbcRegisterValue DxbcCompiler::emitRegisterConcat(
          DxbcRegisterValue       value1,
          DxbcRegisterValue       value2) {
    std::array<uint32_t, 2> ids =
      {{ value1.id, value2.id }};
    
    DxbcRegisterValue result;
    result.type.ctype  = value1.type.ctype;
    result.type.ccount = value1.type.ccount + value2.type.ccount;
    result.id = m_module.opCompositeConstruct(
      getVectorTypeId(result.type),
      ids.size(), ids.data());
    return result;
  }
  
  
  DxbcRegisterValue DxbcCompiler::emitRegisterExtend(
          DxbcRegisterValue       value,
          uint32_t                size) {
    if (size == 1)
      return value;
    
    std::array<uint32_t, 4> ids = {{
      value.id, value.id,
      value.id, value.id, 
    }};
    
    DxbcRegisterValue result;
    result.type.ctype  = value.type.ctype;
    result.type.ccount = size;
    result.id = m_module.opCompositeConstruct(
      getVectorTypeId(result.type),
      size, ids.data());
    return result;
  }
  
  
  DxbcRegisterValue DxbcCompiler::emitRegisterAbsolute(
          DxbcRegisterValue       value) {
    const uint32_t typeId = getVectorTypeId(value.type);
    
    switch (value.type.ctype) {
      case DxbcScalarType::Float32: value.id = m_module.opFAbs(typeId, value.id); break;
      case DxbcScalarType::Float64: value.id = m_module.opFAbs(typeId, value.id); break;
      case DxbcScalarType::Sint32:  value.id = m_module.opSAbs(typeId, value.id); break;
      case DxbcScalarType::Sint64:  value.id = m_module.opSAbs(typeId, value.id); break;
      default: Logger::warn("DxbcCompiler: Cannot get absolute value for given type");
    }
    
    return value;
  }
  
  
  DxbcRegisterValue DxbcCompiler::emitRegisterNegate(
          DxbcRegisterValue       value) {
    const uint32_t typeId = getVectorTypeId(value.type);
    
    switch (value.type.ctype) {
      case DxbcScalarType::Float32: value.id = m_module.opFNegate(typeId, value.id); break;
      case DxbcScalarType::Float64: value.id = m_module.opFNegate(typeId, value.id); break;
      case DxbcScalarType::Sint32:  value.id = m_module.opSNegate(typeId, value.id); break;
      case DxbcScalarType::Sint64:  value.id = m_module.opSNegate(typeId, value.id); break;
      default: Logger::warn("DxbcCompiler: Cannot negate given type");
    }
    
    return value;
  }
  
  
  DxbcRegisterValue DxbcCompiler::emitRegisterZeroTest(
          DxbcRegisterValue       value,
          DxbcZeroTest            test) {
    DxbcRegisterValue result;
    result.type.ctype  = DxbcScalarType::Bool;
    result.type.ccount = 1;
    
    const uint32_t zeroId = m_module.constu32(0u);
    const uint32_t typeId = getVectorTypeId(result.type);
    
    result.id = test == DxbcZeroTest::TestZ
      ? m_module.opIEqual   (typeId, value.id, zeroId)
      : m_module.opINotEqual(typeId, value.id, zeroId);
    return result;
  }
  
  
  DxbcRegisterValue DxbcCompiler::emitRegisterMaskBits(
          DxbcRegisterValue       value,
          uint32_t                mask) {
    DxbcRegisterValue maskVector = emitBuildConstVecu32(
      mask, mask, mask, mask, DxbcRegMask::firstN(value.type.ccount));
    
    DxbcRegisterValue result;
    result.type = value.type;
    result.id = m_module.opBitwiseAnd(
      getVectorTypeId(result.type),
      value.id, maskVector.id);
    return result;
  }


  DxbcRegisterValue DxbcCompiler::emitSrcOperandModifiers(
          DxbcRegisterValue       value,
          DxbcRegModifiers        modifiers) {
    if (modifiers.test(DxbcRegModifier::Abs))
      value = emitRegisterAbsolute(value);
    
    if (modifiers.test(DxbcRegModifier::Neg))
      value = emitRegisterNegate(value);
    return value;
  }
  
  
  uint32_t DxbcCompiler::emitExtractSparseTexel(
          uint32_t          texelTypeId,
          uint32_t          resultId) {
    uint32_t index = 1;

    return m_module.opCompositeExtract(
      texelTypeId, resultId, 1, &index);
  }


  void DxbcCompiler::emitStoreSparseFeedback(
    const DxbcRegister&     feedbackRegister,
          uint32_t          resultId) {
    if (feedbackRegister.type != DxbcOperandType::Null) {
      uint32_t index = 0;

      DxbcRegisterValue result;
      result.type = { DxbcScalarType::Uint32, 1 };
      result.id = m_module.opCompositeExtract(
        getScalarTypeId(DxbcScalarType::Uint32),
        resultId, 1, &index);

      emitRegisterStore(feedbackRegister, result);
    }
  }


  DxbcRegisterValue DxbcCompiler::emitDstOperandModifiers(
          DxbcRegisterValue       value,
          DxbcOpModifiers         modifiers) {
    const uint32_t typeId = getVectorTypeId(value.type);
    
    if (modifiers.saturate) {
      DxbcRegMask mask;
      DxbcRegisterValue vec0, vec1;

      if (value.type.ctype == DxbcScalarType::Float32) {
        mask = DxbcRegMask::firstN(value.type.ccount);
        vec0 = emitBuildConstVecf32(0.0f, 0.0f, 0.0f, 0.0f, mask);
        vec1 = emitBuildConstVecf32(1.0f, 1.0f, 1.0f, 1.0f, mask);
      } else if (value.type.ctype == DxbcScalarType::Float64) {
        mask = DxbcRegMask::firstN(value.type.ccount * 2);
        vec0 = emitBuildConstVecf64(0.0, 0.0, mask);
        vec1 = emitBuildConstVecf64(1.0, 1.0, mask);
      }

      if (mask)
        value.id = m_module.opNClamp(typeId, value.id, vec0.id, vec1.id);
    }
    
    return value;
  }
  
  
  DxbcRegisterPointer DxbcCompiler::emitArrayAccess(
          DxbcRegisterPointer     pointer,
          spv::StorageClass       sclass,
          uint32_t                index) {
    uint32_t ptrTypeId = m_module.defPointerType(
      getVectorTypeId(pointer.type), sclass);
    
    DxbcRegisterPointer result;
    result.type = pointer.type;
    result.id = m_module.opAccessChain(
      ptrTypeId, pointer.id, 1, &index);
    return result;
  }


  uint32_t DxbcCompiler::emitLoadSampledImage(
    const DxbcShaderResource&     textureResource,
    const DxbcSampler&            samplerResource,
          bool                    isDepthCompare) {
    uint32_t baseId = isDepthCompare
      ? textureResource.depthTypeId
      : textureResource.colorTypeId;

    if (!baseId)
      return 0;

    uint32_t sampledImageType = m_module.defSampledImageType(baseId);

    return m_module.opSampledImage(sampledImageType,
      m_module.opLoad(textureResource.imageTypeId, textureResource.varId),
      m_module.opLoad(samplerResource.typeId,      samplerResource.varId));
  }
  
  
  DxbcRegisterPointer DxbcCompiler::emitGetTempPtr(
    const DxbcRegister&           operand) {
    // r# regs are indexed as follows:
    //    (0) register index (immediate)
    uint32_t regIdx = operand.idx[0].offset;

    if (regIdx >= m_rRegs.size())
      m_rRegs.resize(regIdx + 1, 0u);

    if (!m_rRegs.at(regIdx)) {
      DxbcRegisterInfo info;
      info.type.ctype   = DxbcScalarType::Float32;
      info.type.ccount  = 4;
      info.type.alength = 0;
      info.sclass       = spv::StorageClassPrivate;

      uint32_t varId = emitNewVariable(info);
      m_rRegs.at(regIdx) = varId;

      m_module.setDebugName(varId,
        str::format("r", regIdx).c_str());
    }

    DxbcRegisterPointer result;
    result.type.ctype  = DxbcScalarType::Float32;
    result.type.ccount = 4;
    result.id = m_rRegs.at(regIdx);
    return result;
  }
  
  
  DxbcRegisterPointer DxbcCompiler::emitGetIndexableTempPtr(
    const DxbcRegister&           operand) {
    return getIndexableTempPtr(operand, emitIndexLoad(operand.idx[1]));
  }
  
  
  DxbcRegisterPointer DxbcCompiler::emitGetInputPtr(
    const DxbcRegister&           operand) {
    // In the vertex and pixel stages,
    // v# regs are indexed as follows:
    //    (0) register index (relative)
    // 
    // In the tessellation and geometry
    // stages, the index has two dimensions:
    //    (0) vertex index (relative)
    //    (1) register index (relative)
    DxbcRegisterPointer result;
    result.type.ctype  = DxbcScalarType::Float32;
    result.type.ccount = 4;
    
    std::array<uint32_t, 2> indices = {{ 0, 0 }};
    
    for (uint32_t i = 0; i < operand.idxDim; i++)
      indices.at(i) = emitIndexLoad(operand.idx[i]).id;
    
    // Pick the input array depending on
    // the program type and operand type
    struct InputArray {
      uint32_t          id;
      spv::StorageClass sclass;
    };
    
    const InputArray array = [&] () -> InputArray {
      switch (operand.type) {
        case DxbcOperandType::InputControlPoint:
          return m_programInfo.type() == DxbcProgramType::HullShader
                  ? InputArray { m_vArray,             spv::StorageClassPrivate }
                  : InputArray { m_ds.inputPerVertex,  spv::StorageClassInput   };
        case DxbcOperandType::InputPatchConstant:
          return m_programInfo.type() == DxbcProgramType::HullShader
                  ? InputArray { m_hs.outputPerPatch, spv::StorageClassPrivate }
                  : InputArray { m_ds.inputPerPatch,  spv::StorageClassInput   };
        case DxbcOperandType::OutputControlPoint:
          return InputArray { m_hs.outputPerVertex, spv::StorageClassOutput };
        default:
          return { m_vArray, spv::StorageClassPrivate };
      }
    }();
      
    DxbcRegisterInfo info;
    info.type.ctype   = result.type.ctype;
    info.type.ccount  = result.type.ccount;
    info.type.alength = 0;
    info.sclass = array.sclass;
      
    result.id = m_module.opAccessChain(
      getPointerTypeId(info), array.id,
      operand.idxDim, indices.data());
    
    return result;
  }
  
  
  DxbcRegisterPointer DxbcCompiler::emitGetOutputPtr(
    const DxbcRegister&           operand) {
    if (m_programInfo.type() == DxbcProgramType::HullShader) {
      // Hull shaders are special in that they have two sets of
      // output registers, one for per-patch values and one for
      // per-vertex values.
      DxbcRegisterPointer result;
      result.type.ctype  = DxbcScalarType::Float32;
      result.type.ccount = 4;
      
      uint32_t registerId = emitIndexLoad(operand.idx[0]).id;

      if (m_hs.currPhaseType == DxbcCompilerHsPhase::ControlPoint) {
        std::array<uint32_t, 2> indices = {{
          m_module.opLoad(m_module.defIntType(32, 0), m_hs.builtinInvocationId),
          registerId,
        }};
        
        uint32_t ptrTypeId  = m_module.defPointerType(
          getVectorTypeId(result.type),
          spv::StorageClassOutput);
      
        result.id = m_module.opAccessChain(
          ptrTypeId, m_hs.outputPerVertex,
          indices.size(), indices.data());
      } else {
        uint32_t ptrTypeId  = m_module.defPointerType(
          getVectorTypeId(result.type),
          spv::StorageClassPrivate);
        
        result.id = m_module.opAccessChain(
          ptrTypeId, m_hs.outputPerPatch,
          1, &registerId);
      }

      return result;
    } else {
      // Regular shaders have their output
      // registers set up at declaration time
      return m_oRegs.at(operand.idx[0].offset);
    }
  }
  
  
  DxbcRegisterPointer DxbcCompiler::emitGetImmConstBufPtr(
    const DxbcRegister&           operand) {
    DxbcRegisterValue constId = emitIndexLoad(operand.idx[0]);

    if (m_icbArray) {
      // We pad the icb array with an extra zero vector, so we can
      // clamp the index and get correct robustness behaviour.
      constId.id = m_module.opUMin(getVectorTypeId(constId.type),
        constId.id, m_module.constu32(m_icbSize));

      DxbcRegisterInfo ptrInfo;
      ptrInfo.type.ctype   = DxbcScalarType::Uint32;
      ptrInfo.type.ccount  = m_icbComponents;
      ptrInfo.type.alength = 0;
      ptrInfo.sclass = spv::StorageClassPrivate;

      DxbcRegisterPointer result;
      result.type.ctype  = ptrInfo.type.ctype;
      result.type.ccount = ptrInfo.type.ccount;
      result.id = m_module.opAccessChain(
        getPointerTypeId(ptrInfo),
        m_icbArray, 1, &constId.id);
      return result;
    } else if (m_constantBuffers.at(Icb_BindingSlotId).varId != 0) {
      const std::array<uint32_t, 2> indices =
        {{ m_module.consti32(0), constId.id }};
      
      DxbcRegisterInfo ptrInfo;
      ptrInfo.type.ctype   = DxbcScalarType::Float32;
      ptrInfo.type.ccount  = m_icbComponents;
      ptrInfo.type.alength = 0;
      ptrInfo.sclass = spv::StorageClassUniform;

      DxbcRegisterPointer result;
      result.type.ctype  = ptrInfo.type.ctype;
      result.type.ccount = ptrInfo.type.ccount;
      result.id = m_module.opAccessChain(
        getPointerTypeId(ptrInfo),
        m_constantBuffers.at(Icb_BindingSlotId).varId,
        indices.size(), indices.data());
      return result;
    } else {
      throw DxvkError("DxbcCompiler: Immediate constant buffer not defined");
    }
  }
  
  
  DxbcRegisterPointer DxbcCompiler::emitGetOperandPtr(
    const DxbcRegister&           operand) {
    switch (operand.type) {
      case DxbcOperandType::Temp:
        return emitGetTempPtr(operand);
      
      case DxbcOperandType::IndexableTemp:
        return emitGetIndexableTempPtr(operand);
      
      case DxbcOperandType::Input:
      case DxbcOperandType::InputControlPoint:
      case DxbcOperandType::InputPatchConstant:
      case DxbcOperandType::OutputControlPoint:
        return emitGetInputPtr(operand);
      
      case DxbcOperandType::Output:
        return emitGetOutputPtr(operand);
      
      case DxbcOperandType::ImmediateConstantBuffer:
        return emitGetImmConstBufPtr(operand);

      case DxbcOperandType::InputThreadId:
        return DxbcRegisterPointer {
          { DxbcScalarType::Uint32, 3 },
          m_cs.builtinGlobalInvocationId };
      
      case DxbcOperandType::InputThreadGroupId:
        return DxbcRegisterPointer {
          { DxbcScalarType::Uint32, 3 },
          m_cs.builtinWorkgroupId };
      
      case DxbcOperandType::InputThreadIdInGroup:
        return DxbcRegisterPointer {
          { DxbcScalarType::Uint32, 3 },
          m_cs.builtinLocalInvocationId };
      
      case DxbcOperandType::InputThreadIndexInGroup:
        return DxbcRegisterPointer {
          { DxbcScalarType::Uint32, 1 },
          m_cs.builtinLocalInvocationIndex };
      
      case DxbcOperandType::InputCoverageMask: {
        const std::array<uint32_t, 1> indices
          = {{ m_module.constu32(0) }};
        
        DxbcRegisterPointer result;
        result.type.ctype  = DxbcScalarType::Uint32;
        result.type.ccount = 1;
        result.id = m_module.opAccessChain(
          m_module.defPointerType(
            getVectorTypeId(result.type),
            spv::StorageClassInput),
          m_ps.builtinSampleMaskIn,
          indices.size(), indices.data());
        return result;
      }
        
      case DxbcOperandType::OutputCoverageMask: {
        const std::array<uint32_t, 1> indices
          = {{ m_module.constu32(0) }};
        
        DxbcRegisterPointer result;
        result.type.ctype  = DxbcScalarType::Uint32;
        result.type.ccount = 1;
        result.id = m_module.opAccessChain(
          m_module.defPointerType(
            getVectorTypeId(result.type),
            spv::StorageClassOutput),
          m_ps.builtinSampleMaskOut,
          indices.size(), indices.data());
        return result;
      }
        
      case DxbcOperandType::OutputDepth:
      case DxbcOperandType::OutputDepthGe:
      case DxbcOperandType::OutputDepthLe:
        return DxbcRegisterPointer {
          { DxbcScalarType::Float32, 1 },
          m_ps.builtinDepth };
      
      case DxbcOperandType::OutputStencilRef:
        return DxbcRegisterPointer {
          { DxbcScalarType::Sint32, 1 },
          m_ps.builtinStencilRef };

      case DxbcOperandType::InputPrimitiveId:
        return DxbcRegisterPointer {
          { DxbcScalarType::Uint32, 1 },
          m_primitiveIdIn };
      
      case DxbcOperandType::InputDomainPoint:
        return DxbcRegisterPointer {
          { DxbcScalarType::Float32, 3 },
          m_ds.builtinTessCoord };
      
      case DxbcOperandType::OutputControlPointId:
        return DxbcRegisterPointer {
          { DxbcScalarType::Uint32, 1 },
          m_hs.builtinInvocationId };
      
      case DxbcOperandType::InputForkInstanceId:
      case DxbcOperandType::InputJoinInstanceId:
        return DxbcRegisterPointer {
          { DxbcScalarType::Uint32, 1 },
          getCurrentHsForkJoinPhase()->instanceIdPtr };
      
      case DxbcOperandType::InputGsInstanceId:
        return DxbcRegisterPointer {
          { DxbcScalarType::Uint32, 1 },
          m_gs.builtinInvocationId };
        
      case DxbcOperandType::InputInnerCoverage:
        return DxbcRegisterPointer {
          { DxbcScalarType::Uint32, 1 },
          m_ps.builtinInnerCoverageId };
        
      default:
        throw DxvkError(str::format(
          "DxbcCompiler: Unhandled operand type: ",
          operand.type));
    }
  }
  
  
  DxbcRegisterPointer DxbcCompiler::emitGetAtomicPointer(
    const DxbcRegister&           operand,
    const DxbcRegister&           address) {
    // Query information about the resource itself
    const uint32_t registerId = operand.idx[0].offset;
    const DxbcBufferInfo resourceInfo = getBufferInfo(operand);
    
    // For UAVs and shared memory, different methods
    // of obtaining the final pointer are used.
    bool isTgsm = operand.type == DxbcOperandType::ThreadGroupSharedMemory;
    bool isSsbo = resourceInfo.isSsbo;
    
    // Compute the actual address into the resource
    const DxbcRegisterValue addressValue = [&] {
      switch (resourceInfo.type) {
        case DxbcResourceType::Raw:
          return emitCalcBufferIndexRaw(emitRegisterLoad(
            address, DxbcRegMask(true, false, false, false)));
          
        case DxbcResourceType::Structured: {
          const DxbcRegisterValue addressComponents = emitRegisterLoad(
            address, DxbcRegMask(true, true, false, false));
          
          return emitCalcBufferIndexStructured(
            emitRegisterExtract(addressComponents, DxbcRegMask(true, false, false, false)),
            emitRegisterExtract(addressComponents, DxbcRegMask(false, true, false, false)),
            resourceInfo.stride);
        };
        
        case DxbcResourceType::Typed: {
          if (isTgsm)
            throw DxvkError("DxbcCompiler: TGSM cannot be typed");
          
          return emitLoadTexCoord(address,
            m_uavs.at(registerId).imageInfo);
        }
        
        default:
          throw DxvkError("DxbcCompiler: Unhandled resource type");
      }
    }();
    
    // Compute the actual pointer
    DxbcRegisterPointer result;
    result.type.ctype  = resourceInfo.stype;
    result.type.ccount = 1;

    if (isTgsm) {
      result.id = m_module.opAccessChain(resourceInfo.typeId,
        resourceInfo.varId, 1, &addressValue.id);
    } else if (isSsbo) {
      uint32_t indices[2] = { m_module.constu32(0), addressValue.id };
      result.id = m_module.opAccessChain(resourceInfo.typeId,
        resourceInfo.varId, 2, indices);
    } else {
      result.id = m_module.opImageTexelPointer(
        m_module.defPointerType(getVectorTypeId(result.type), spv::StorageClassImage),
        resourceInfo.varId, addressValue.id, m_module.constu32(0));
    }

    return result;
  }
  
  
  DxbcRegisterValue DxbcCompiler::emitQueryBufferSize(
    const DxbcRegister&           resource) {
    const DxbcBufferInfo bufferInfo = getBufferInfo(resource);

    DxbcRegisterValue result;
    result.type.ctype  = DxbcScalarType::Uint32;
    result.type.ccount = 1;
    result.id = m_module.opArrayLength(
      getVectorTypeId(result.type),
      bufferInfo.varId, 0);

    return result;
  }
  
  
  DxbcRegisterValue DxbcCompiler::emitQueryTexelBufferSize(
    const DxbcRegister&           resource) {
    // Load the texel buffer object. This cannot be used with
    // constant buffers or any other type of resource.
    const DxbcBufferInfo bufferInfo = getBufferInfo(resource);
    
    const uint32_t bufferId = m_module.opLoad(
      bufferInfo.typeId, bufferInfo.varId);
    
    // We'll store this as a scalar unsigned integer
    DxbcRegisterValue result;
    result.type.ctype  = DxbcScalarType::Uint32;
    result.type.ccount = 1;
    result.id = m_module.opImageQuerySize(
      getVectorTypeId(result.type), bufferId);

    return result;
  }
  
  
  DxbcRegisterValue DxbcCompiler::emitQueryTextureLods(
    const DxbcRegister&           resource) {
    const DxbcBufferInfo info = getBufferInfo(resource);
    
    DxbcRegisterValue result;
    result.type.ctype  = DxbcScalarType::Uint32;
    result.type.ccount = 1;
    
    if (info.image.ms == 0 && info.image.sampled == 1) {
      result.id = m_module.opImageQueryLevels(
        getVectorTypeId(result.type),
        m_module.opLoad(info.typeId, info.varId));
    } else {
      // Report one LOD in case of UAVs or multisampled images
      result.id = m_module.constu32(1);
    }

    return result;
  }
  
  
  DxbcRegisterValue DxbcCompiler::emitQueryTextureSamples(
    const DxbcRegister&           resource) {
    if (resource.type == DxbcOperandType::Rasterizer) {
      // SPIR-V has no gl_NumSamples equivalent, so we
      // have to work around it using a push constant
      if (!m_ps.pushConstantId)
        m_ps.pushConstantId = emitPushConstants();

      uint32_t uintTypeId = m_module.defIntType(32, 0);
      uint32_t ptrTypeId = m_module.defPointerType(uintTypeId, spv::StorageClassPushConstant);
      uint32_t index = m_module.constu32(0);

      DxbcRegisterValue result;
      result.type.ctype  = DxbcScalarType::Uint32;
      result.type.ccount = 1;
      result.id = m_module.opLoad(uintTypeId,
        m_module.opAccessChain(ptrTypeId, m_ps.pushConstantId, 1, &index));
      return result;
    } else {
      DxbcBufferInfo info = getBufferInfo(resource);
      
      DxbcRegisterValue result;
      result.type.ctype  = DxbcScalarType::Uint32;
      result.type.ccount = 1;

      if (info.image.ms) {
        result.id = m_module.opImageQuerySamples(
          getVectorTypeId(result.type),
          m_module.opLoad(info.typeId, info.varId));
      } else {
        // OpImageQuerySamples requires MSAA images
        result.id = m_module.constu32(1);
      }
      
      return result;
    }
  }
  
  
  DxbcRegisterValue DxbcCompiler::emitQueryTextureSize(
    const DxbcRegister&           resource,
          DxbcRegisterValue       lod) {
    const DxbcBufferInfo info = getBufferInfo(resource);
    
    DxbcRegisterValue result;
    result.type.ctype  = DxbcScalarType::Uint32;
    result.type.ccount = getTexSizeDim(info.image);
    
    if (info.image.ms == 0 && info.image.sampled == 1) {
      result.id = m_module.opImageQuerySizeLod(
        getVectorTypeId(result.type),
        m_module.opLoad(info.typeId, info.varId),
        lod.id);
    } else {
      result.id = m_module.opImageQuerySize(
        getVectorTypeId(result.type),
        m_module.opLoad(info.typeId, info.varId));
    }

    return result;
  }
  
  
  DxbcRegisterValue DxbcCompiler::emitCalcBufferIndexStructured(
          DxbcRegisterValue       structId,
          DxbcRegisterValue       structOffset,
          uint32_t                structStride) {
    DxbcRegisterValue result;
    result.type.ctype  = DxbcScalarType::Sint32;
    result.type.ccount = 1;
    
    uint32_t typeId = getVectorTypeId(result.type);
    uint32_t offset = m_module.opShiftRightLogical(typeId, structOffset.id, m_module.consti32(2));
    
    result.id = m_module.opIAdd(typeId,
      m_module.opIMul(typeId, structId.id, m_module.consti32(structStride / 4)),
      offset);
    return result;
  }
  
  
  DxbcRegisterValue DxbcCompiler::emitCalcBufferIndexRaw(
          DxbcRegisterValue       byteOffset) {
    DxbcRegisterValue result;
    result.type.ctype  = DxbcScalarType::Sint32;
    result.type.ccount = 1;
    
    uint32_t typeId = getVectorTypeId(result.type);
    result.id = m_module.opShiftRightLogical(typeId, byteOffset.id, m_module.consti32(2));
    return result;
  }
  
  
  DxbcRegisterValue DxbcCompiler::emitCalcTexCoord(
          DxbcRegisterValue       coordVector,
    const DxbcImageInfo&          imageInfo) {
    const uint32_t dim = getTexCoordDim(imageInfo);
    
    if (dim != coordVector.type.ccount) {
      coordVector = emitRegisterExtract(
        coordVector, DxbcRegMask::firstN(dim));      
    }
    
    return coordVector;
  }
  
  
  DxbcRegisterValue DxbcCompiler::emitLoadTexCoord(
    const DxbcRegister&           coordReg,
    const DxbcImageInfo&          imageInfo) {
    return emitCalcTexCoord(emitRegisterLoad(coordReg,
      DxbcRegMask(true, true, true, true)), imageInfo);
  }
  
  
  DxbcRegisterValue DxbcCompiler::emitIndexLoad(
          DxbcRegIndex            index) {
    if (index.relReg != nullptr) {
      DxbcRegisterValue result = emitRegisterLoad(
        *index.relReg, DxbcRegMask(true, false, false, false));
      
      if (index.offset != 0) {
        result.id = m_module.opIAdd(
          getVectorTypeId(result.type), result.id,
          m_module.consti32(index.offset));
      }
      
      return result;
    } else {
      DxbcRegisterValue result;
      result.type.ctype  = DxbcScalarType::Sint32;
      result.type.ccount = 1;
      result.id = m_module.consti32(index.offset);
      return result;
    }
  }
  
  
  DxbcRegisterValue DxbcCompiler::emitValueLoad(
          DxbcRegisterPointer     ptr) {
    DxbcRegisterValue result;
    result.type = ptr.type;
    result.id   = m_module.opLoad(
      getVectorTypeId(result.type),
      ptr.id);
    return result;
  }
  
  
  void DxbcCompiler::emitValueStore(
          DxbcRegisterPointer     ptr,
          DxbcRegisterValue       value,
          DxbcRegMask             writeMask) {
    // If the component types are not compatible,
    // we need to bit-cast the source variable.
    if (value.type.ctype != ptr.type.ctype)
      value = emitRegisterBitcast(value, ptr.type.ctype);
    
    // If the source value consists of only one component,
    // it is stored in all components of the destination.
    if (value.type.ccount == 1)
      value = emitRegisterExtend(value, writeMask.popCount());
    
    if (ptr.type.ccount == writeMask.popCount()) {
      // Simple case: We write to the entire register
      m_module.opStore(ptr.id, value.id);
    } else {
      // We only write to part of the destination
      // register, so we need to load and modify it
      DxbcRegisterValue tmp = emitValueLoad(ptr);
      tmp = emitRegisterInsert(tmp, value, writeMask);
      
      m_module.opStore(ptr.id, tmp.id);
    }
  }


  DxbcRegisterValue DxbcCompiler::emitRegisterLoadRaw(
    const DxbcRegister&           reg) {
    // Try to find index range for the given register
    const DxbcIndexRange* indexRange = nullptr;

    if (reg.idxDim && reg.idx[reg.idxDim - 1u].relReg) {
      uint32_t offset = reg.idx[reg.idxDim - 1u].offset;

      for (const auto& range : m_indexRanges) {
        if (reg.type == range.type && offset >= range.start && offset < range.start + range.length)
          indexRange = &range;
      }
    }

    if (reg.type == DxbcOperandType::IndexableTemp || indexRange) {
      bool doBoundsCheck = reg.idx[reg.idxDim - 1u].relReg != nullptr;

      if (doBoundsCheck) {
        DxbcRegisterValue indexId = emitIndexLoad(reg.idx[reg.idxDim - 1u]);
        uint32_t boundsCheck = 0u;

        if (reg.type == DxbcOperandType::IndexableTemp) {
          boundsCheck = m_module.opULessThan(
            m_module.defBoolType(), indexId.id,
            m_module.constu32(m_xRegs.at(reg.idx[0].offset).alength));
        } else {
          uint32_t adjustedId = m_module.opISub(getVectorTypeId(indexId.type),
            indexId.id, m_module.consti32(indexRange->start));

          boundsCheck = m_module.opULessThan(
            m_module.defBoolType(), adjustedId,
            m_module.constu32(indexRange->length));
        }

        // Kind of ugly to have an empty else block here but there's no
        // way for us to know the current block ID for the phi below
        DxbcConditional cond;
        cond.labelIf   = m_module.allocateId();
        cond.labelElse = m_module.allocateId();
        cond.labelEnd  = m_module.allocateId();

        m_module.opSelectionMerge(cond.labelEnd, spv::SelectionControlMaskNone);
        m_module.opBranchConditional(boundsCheck, cond.labelIf, cond.labelElse);

        m_module.opLabel(cond.labelIf);

        DxbcRegisterValue returnValue = emitValueLoad(emitGetOperandPtr(reg));

        m_module.opBranch(cond.labelEnd);
        m_module.opLabel (cond.labelElse);

        DxbcRegisterValue zeroValue = emitBuildZeroVector(returnValue.type);

        m_module.opBranch(cond.labelEnd);
        m_module.opLabel (cond.labelEnd);

        std::array<SpirvPhiLabel, 2> phiLabels = {{
          { returnValue.id, cond.labelIf   },
          { zeroValue.id,   cond.labelElse },
        }};

        returnValue.id = m_module.opPhi(
          getVectorTypeId(returnValue.type),
          phiLabels.size(), phiLabels.data());
        return returnValue;
      }
    }

    DxbcRegisterValue value = emitValueLoad(emitGetOperandPtr(reg));

    // Pad icb values to a vec4 since the app may access components that are always 0
    if (reg.type == DxbcOperandType::ImmediateConstantBuffer && value.type.ccount < 4u) {
      DxbcVectorType zeroType;
      zeroType.ctype = value.type.ctype;
      zeroType.ccount = 4u - value.type.ccount;

      uint32_t zeroVector = emitBuildZeroVector(zeroType).id;

      std::array<uint32_t, 2> constituents = { value.id, zeroVector };

      value.type.ccount = 4u;
      value.id = m_module.opCompositeConstruct(getVectorTypeId(value.type),
        constituents.size(), constituents.data());
    }

    return value;
  }
  
  
  DxbcRegisterValue DxbcCompiler::emitConstantBufferLoad(
    const DxbcRegister&           reg,
          DxbcRegMask             writeMask) {
    // Constant buffers take a two-dimensional index:
    //    (0) register index (immediate)
    //    (1) constant offset (relative)
    DxbcRegisterInfo info;
    info.type.ctype   = DxbcScalarType::Float32;
    info.type.ccount  = 4;
    info.type.alength = 0;
    info.sclass = spv::StorageClassUniform;
    
    uint32_t regId = reg.idx[0].offset;
    DxbcRegisterValue constId = emitIndexLoad(reg.idx[1]);
    
    uint32_t ptrTypeId = getPointerTypeId(info);
    
    const std::array<uint32_t, 2> indices =
      {{ m_module.consti32(0), constId.id }};
    
    DxbcRegisterPointer ptr;
    ptr.type.ctype  = info.type.ctype;
    ptr.type.ccount = info.type.ccount;
    ptr.id = m_module.opAccessChain(ptrTypeId,
      m_constantBuffers.at(regId).varId,
      indices.size(), indices.data());

    // Load individual components from buffer
    std::array<uint32_t, 4> ccomps = { 0, 0, 0, 0 };
    std::array<uint32_t, 4> scomps = { 0, 0, 0, 0 };
    uint32_t                scount = 0;

    for (uint32_t i = 0; i < 4; i++) {
      uint32_t sindex = reg.swizzle[i];

      if (!writeMask[i] || ccomps[sindex])
        continue;
      
      uint32_t componentId = m_module.constu32(sindex);
      uint32_t componentPtr = m_module.opAccessChain(
        m_module.defPointerType(
          getScalarTypeId(DxbcScalarType::Float32),
          spv::StorageClassUniform),
        ptr.id, 1, &componentId);
      
      ccomps[sindex] = m_module.opLoad(
        getScalarTypeId(DxbcScalarType::Float32),
        componentPtr);
    }

    for (uint32_t i = 0; i < 4; i++) {
      uint32_t sindex = reg.swizzle[i];
      
      if (writeMask[i])
        scomps[scount++] = ccomps[sindex];
    }
    
    DxbcRegisterValue result;
    result.type.ctype  = DxbcScalarType::Float32;
    result.type.ccount = scount;
    result.id = scomps[0];
    
    if (scount > 1) {
      result.id = m_module.opCompositeConstruct(
        getVectorTypeId(result.type),
        scount, scomps.data());
    }

    // Apply any post-processing that might be necessary
    result = emitRegisterBitcast(result, reg.dataType);
    result = emitSrcOperandModifiers(result, reg.modifiers);
    return result;
  }
  
  
  DxbcRegisterValue DxbcCompiler::emitRegisterLoad(
    const DxbcRegister&           reg,
          DxbcRegMask             writeMask) {
    if (reg.type == DxbcOperandType::Imm32
     || reg.type == DxbcOperandType::Imm64) {
      DxbcRegisterValue result;
      
      if (reg.componentCount == DxbcComponentCount::Component1) {
        // Create one single u32 constant
        result.type.ctype  = DxbcScalarType::Uint32;
        result.type.ccount = 1;
        result.id = m_module.constu32(reg.imm.u32_1);

        result = emitRegisterExtend(result, writeMask.popCount());
      } else if (reg.componentCount == DxbcComponentCount::Component4) {
        // Create a u32 vector with as many components as needed
        std::array<uint32_t, 4> indices = { };
        uint32_t indexId = 0;
        
        for (uint32_t i = 0; i < indices.size(); i++) {
          if (writeMask[i]) {
            indices.at(indexId++) =
              m_module.constu32(reg.imm.u32_4[i]);
          }
        }
        
        result.type.ctype  = DxbcScalarType::Uint32;
        result.type.ccount = writeMask.popCount();
        result.id = indices.at(0);
        
        if (indexId > 1) {
          result.id = m_module.constComposite(
            getVectorTypeId(result.type),
            result.type.ccount, indices.data());
        }
        
      } else {
        // Something went horribly wrong in the decoder or the shader is broken
        throw DxvkError("DxbcCompiler: Invalid component count for immediate operand");
      }
      
      // Cast constants to the requested type
      return emitRegisterBitcast(result, reg.dataType);
    } else if (reg.type == DxbcOperandType::ConstantBuffer) {
      return emitConstantBufferLoad(reg, writeMask);
    } else {
      // Load operand from the operand pointer
      DxbcRegisterValue result = emitRegisterLoadRaw(reg);
      
      // Apply operand swizzle to the operand value
      result = emitRegisterSwizzle(result, reg.swizzle, writeMask);
      
      // Cast it to the requested type. We need to do
      // this after the swizzling for 64-bit types.
      result = emitRegisterBitcast(result, reg.dataType);
      
      // Apply operand modifiers
      result = emitSrcOperandModifiers(result, reg.modifiers);
      return result;
    }
  }
  
  
  void DxbcCompiler::emitRegisterStore(
    const DxbcRegister&           reg,
          DxbcRegisterValue       value) {
    if (reg.type == DxbcOperandType::IndexableTemp) {
      bool doBoundsCheck = reg.idx[1].relReg != nullptr;
      DxbcRegisterValue vectorId = emitIndexLoad(reg.idx[1]);

      if (doBoundsCheck) {
        uint32_t boundsCheck = m_module.opULessThan(
          m_module.defBoolType(), vectorId.id,
          m_module.constu32(m_xRegs.at(reg.idx[0].offset).alength));
        
        DxbcConditional cond;
        cond.labelIf  = m_module.allocateId();
        cond.labelEnd = m_module.allocateId();
        
        m_module.opSelectionMerge(cond.labelEnd, spv::SelectionControlMaskNone);
        m_module.opBranchConditional(boundsCheck, cond.labelIf, cond.labelEnd);
        
        m_module.opLabel(cond.labelIf);

        emitValueStore(getIndexableTempPtr(reg, vectorId), value, reg.mask);

        m_module.opBranch(cond.labelEnd);
        m_module.opLabel (cond.labelEnd);
      } else {
        emitValueStore(getIndexableTempPtr(reg, vectorId), value, reg.mask);
      }
    } else {
      emitValueStore(emitGetOperandPtr(reg), value, reg.mask);
    }
  }
  
  
  void DxbcCompiler::emitInputSetup() {
    m_module.setLateConst(m_vArrayLengthId, &m_vArrayLength);

    // Copy all defined v# registers into the input array
    const uint32_t vecTypeId = m_module.defVectorType(m_module.defFloatType(32), 4);
    const uint32_t ptrTypeId = m_module.defPointerType(vecTypeId, spv::StorageClassPrivate);
    
    for (uint32_t i = 0; i < m_vRegs.size(); i++) {
      if (m_vRegs.at(i).id != 0) {
        const uint32_t registerId = m_module.consti32(i);

        DxbcRegisterPointer srcPtr = m_vRegs.at(i);
        DxbcRegisterValue srcValue = emitRegisterBitcast(
          emitValueLoad(srcPtr), DxbcScalarType::Float32);
        
        DxbcRegisterPointer dstPtr;
        dstPtr.type = { DxbcScalarType::Float32, 4 };
        dstPtr.id = m_module.opAccessChain(
          ptrTypeId, m_vArray, 1, &registerId);
        
        emitValueStore(dstPtr, srcValue, DxbcRegMask::firstN(srcValue.type.ccount));
      }
    }
    
    // Copy all system value registers into the array,
    // preserving any previously written contents.
    for (const DxbcSvMapping& map : m_vMappings) {
      const uint32_t registerId = m_module.consti32(map.regId);
      
      const DxbcRegisterValue value = [&] {
        switch (m_programInfo.type()) {
          case DxbcProgramType::VertexShader:   return emitVsSystemValueLoad(map.sv, map.regMask);
          case DxbcProgramType::PixelShader:    return emitPsSystemValueLoad(map.sv, map.regMask);
          default: throw DxvkError(str::format("DxbcCompiler: Unexpected stage: ", m_programInfo.type()));
        }
      }();
      
      DxbcRegisterPointer inputReg;
      inputReg.type.ctype  = DxbcScalarType::Float32;
      inputReg.type.ccount = 4;
      inputReg.id = m_module.opAccessChain(
        ptrTypeId, m_vArray, 1, &registerId);
      emitValueStore(inputReg, value, map.regMask);
    }
  }
  
  
  void DxbcCompiler::emitInputSetup(uint32_t vertexCount) {
    m_module.setLateConst(m_vArrayLengthId, &m_vArrayLength);

    // Copy all defined v# registers into the input array. Note
    // that the outer index of the array is the vertex index.
    const uint32_t vecTypeId    = m_module.defVectorType(m_module.defFloatType(32), 4);
    const uint32_t dstPtrTypeId = m_module.defPointerType(vecTypeId, spv::StorageClassPrivate);
    
    for (uint32_t i = 0; i < m_vRegs.size(); i++) {
      if (m_vRegs.at(i).id != 0) {
        const uint32_t registerId = m_module.consti32(i);
        
        for (uint32_t v = 0; v < vertexCount; v++) {
          std::array<uint32_t, 2> indices
            = {{ m_module.consti32(v), registerId }};
          
          DxbcRegisterPointer srcPtr;
          srcPtr.type = m_vRegs.at(i).type;
          srcPtr.id = m_module.opAccessChain(
            m_module.defPointerType(getVectorTypeId(srcPtr.type), spv::StorageClassInput),
            m_vRegs.at(i).id, 1, indices.data());
          
          DxbcRegisterValue srcValue = emitRegisterBitcast(
            emitValueLoad(srcPtr), DxbcScalarType::Float32);
          
          DxbcRegisterPointer dstPtr;
          dstPtr.type = { DxbcScalarType::Float32, 4 };
          dstPtr.id = m_module.opAccessChain(
            dstPtrTypeId, m_vArray, 2, indices.data());

          emitValueStore(dstPtr, srcValue, DxbcRegMask::firstN(srcValue.type.ccount));
        }
      }
    }
    
    // Copy all system value registers into the array,
    // preserving any previously written contents.
    for (const DxbcSvMapping& map : m_vMappings) {
      const uint32_t registerId = m_module.consti32(map.regId);
      
      for (uint32_t v = 0; v < vertexCount; v++) {
        const DxbcRegisterValue value = [&] {
          switch (m_programInfo.type()) {
            case DxbcProgramType::GeometryShader: return emitGsSystemValueLoad(map.sv, map.regMask, v);
            default: throw DxvkError(str::format("DxbcCompiler: Unexpected stage: ", m_programInfo.type()));
          }
        }();
        
        std::array<uint32_t, 2> indices = {
          m_module.consti32(v), registerId,
        };
        
        DxbcRegisterPointer inputReg;
        inputReg.type.ctype  = DxbcScalarType::Float32;
        inputReg.type.ccount = 4;
        inputReg.id = m_module.opAccessChain(dstPtrTypeId,
          m_vArray, indices.size(), indices.data());
        emitValueStore(inputReg, value, map.regMask);
      }
    }
  }
  
  
  void DxbcCompiler::emitOutputSetup() {
    for (const DxbcSvMapping& svMapping : m_oMappings) {
      DxbcRegisterPointer outputReg = m_oRegs.at(svMapping.regId);
      
      if (m_programInfo.type() == DxbcProgramType::HullShader) {
        uint32_t registerIndex = m_module.constu32(svMapping.regId);
        
        outputReg.type = { DxbcScalarType::Float32, 4 };
        outputReg.id = m_module.opAccessChain(
          m_module.defPointerType(
            getVectorTypeId(outputReg.type),
            spv::StorageClassPrivate),
          m_hs.outputPerPatch,
          1, &registerIndex);
      }
      
      auto sv    = svMapping.sv;
      auto mask  = svMapping.regMask;
      auto value = emitValueLoad(outputReg);
      
      switch (m_programInfo.type()) {
        case DxbcProgramType::VertexShader:   emitVsSystemValueStore(sv, mask, value); break;
        case DxbcProgramType::GeometryShader: emitGsSystemValueStore(sv, mask, value); break;
        case DxbcProgramType::HullShader:     emitHsSystemValueStore(sv, mask, value); break;
        case DxbcProgramType::DomainShader:   emitDsSystemValueStore(sv, mask, value); break;
        case DxbcProgramType::PixelShader:    emitPsSystemValueStore(sv, mask, value); break;
        default: break;
      }
    }
  }


  void DxbcCompiler::emitOutputDepthClamp() {
    // HACK: Some drivers do not clamp FragDepth to [minDepth..maxDepth]
    // before writing to the depth attachment, but we do not have acccess
    // to those. Clamp to [0..1] instead.
    if (m_ps.builtinDepth) {
      DxbcRegisterPointer ptr;
      ptr.type = { DxbcScalarType::Float32, 1 };
      ptr.id = m_ps.builtinDepth;

      DxbcRegisterValue value = emitValueLoad(ptr);

      value.id = m_module.opNClamp(
        getVectorTypeId(ptr.type),
        value.id,
        m_module.constf32(0.0f),
        m_module.constf32(1.0f));
      
      emitValueStore(ptr, value,
        DxbcRegMask::firstN(1));
    }
  }
  
  
  void DxbcCompiler::emitInitWorkgroupMemory() {
    bool hasTgsm = false;

    SpirvMemoryOperands memoryOperands;
    memoryOperands.flags = spv::MemoryAccessNonPrivatePointerMask;

    for (uint32_t i = 0; i < m_gRegs.size(); i++) {
      if (!m_gRegs[i].varId)
        continue;
      
      if (!m_cs.builtinLocalInvocationIndex) {
        m_cs.builtinLocalInvocationIndex = emitNewBuiltinVariable({
          { DxbcScalarType::Uint32, 1, 0 },
          spv::StorageClassInput },
          spv::BuiltInLocalInvocationIndex,
          "vThreadIndexInGroup");
      }

      uint32_t intTypeId = getScalarTypeId(DxbcScalarType::Uint32);
      uint32_t ptrTypeId = m_module.defPointerType(
        intTypeId, spv::StorageClassWorkgroup);

      uint32_t numElements = m_gRegs[i].type == DxbcResourceType::Structured
        ? m_gRegs[i].elementCount * m_gRegs[i].elementStride / 4
        : m_gRegs[i].elementCount / 4;
      
      uint32_t numThreads = m_cs.workgroupSizeX *
        m_cs.workgroupSizeY * m_cs.workgroupSizeZ;
      
      uint32_t numElementsPerThread = numElements / numThreads;
      uint32_t numElementsRemaining = numElements % numThreads;

      uint32_t threadId = m_module.opLoad(
        intTypeId, m_cs.builtinLocalInvocationIndex);
      uint32_t zeroId = m_module.constu32(0);

      for (uint32_t e = 0; e < numElementsPerThread; e++) {
        uint32_t ofsId = m_module.opIAdd(intTypeId, threadId,
          m_module.constu32(numThreads * e));
        
        uint32_t ptrId = m_module.opAccessChain(
          ptrTypeId, m_gRegs[i].varId, 1, &ofsId);

        m_module.opStore(ptrId, zeroId, memoryOperands);
      }

      if (numElementsRemaining) {
        uint32_t condition = m_module.opULessThan(
          m_module.defBoolType(), threadId,
          m_module.constu32(numElementsRemaining));
        
        DxbcConditional cond;
        cond.labelIf  = m_module.allocateId();
        cond.labelEnd = m_module.allocateId();

        m_module.opSelectionMerge(cond.labelEnd, spv::SelectionControlMaskNone);
        m_module.opBranchConditional(condition, cond.labelIf, cond.labelEnd);

        m_module.opLabel(cond.labelIf);

        uint32_t ofsId = m_module.opIAdd(intTypeId, threadId,
          m_module.constu32(numThreads * numElementsPerThread));
        
        uint32_t ptrId = m_module.opAccessChain(
          ptrTypeId, m_gRegs[i].varId, 1, &ofsId);
        
        m_module.opStore(ptrId, zeroId, memoryOperands);

        m_module.opBranch(cond.labelEnd);
        m_module.opLabel (cond.labelEnd);
      }

      hasTgsm = true;
    }

    if (hasTgsm) {
      m_module.opControlBarrier(
        m_module.constu32(spv::ScopeWorkgroup),
        m_module.constu32(spv::ScopeWorkgroup),
        m_module.constu32(spv::MemorySemanticsWorkgroupMemoryMask
                        | spv::MemorySemanticsAcquireReleaseMask
                        | spv::MemorySemanticsMakeAvailableMask
                        | spv::MemorySemanticsMakeVisibleMask));
    }
  }


  DxbcRegisterValue DxbcCompiler::emitVsSystemValueLoad(
          DxbcSystemValue         sv,
          DxbcRegMask             mask) {
    switch (sv) {
      case DxbcSystemValue::VertexId: {
        const uint32_t typeId = getScalarTypeId(DxbcScalarType::Uint32);
        
        if (m_vs.builtinVertexId == 0) {
          m_vs.builtinVertexId = emitNewBuiltinVariable({
            { DxbcScalarType::Uint32, 1, 0 },
            spv::StorageClassInput },
            spv::BuiltInVertexIndex,
            "vs_vertex_index");
        }
        
        if (m_vs.builtinBaseVertex == 0) {
          m_vs.builtinBaseVertex = emitNewBuiltinVariable({
            { DxbcScalarType::Uint32, 1, 0 },
            spv::StorageClassInput },
            spv::BuiltInBaseVertex,
            "vs_base_vertex");
        }
        
        DxbcRegisterValue result;
        result.type.ctype   = DxbcScalarType::Uint32;
        result.type.ccount  = 1;
        result.id = m_module.opISub(typeId,
          m_module.opLoad(typeId, m_vs.builtinVertexId),
          m_module.opLoad(typeId, m_vs.builtinBaseVertex));
        return result;
      } break;
      
      case DxbcSystemValue::InstanceId: {
        const uint32_t typeId = getScalarTypeId(DxbcScalarType::Uint32);
        
        if (m_vs.builtinInstanceId == 0) {
          m_vs.builtinInstanceId = emitNewBuiltinVariable({
            { DxbcScalarType::Uint32, 1, 0 },
            spv::StorageClassInput },
            spv::BuiltInInstanceIndex,
            "vs_instance_index");
        }
          
        if (m_vs.builtinBaseInstance == 0) {
          m_vs.builtinBaseInstance = emitNewBuiltinVariable({
            { DxbcScalarType::Uint32, 1, 0 },
            spv::StorageClassInput },
            spv::BuiltInBaseInstance,
            "vs_base_instance");
        }
        
        DxbcRegisterValue result;
        result.type.ctype   = DxbcScalarType::Uint32;
        result.type.ccount  = 1;
        result.id = m_module.opISub(typeId,
          m_module.opLoad(typeId, m_vs.builtinInstanceId),
          m_module.opLoad(typeId, m_vs.builtinBaseInstance));
        return result;
      } break;
      
      default:
        throw DxvkError(str::format(
          "DxbcCompiler: Unhandled VS SV input: ", sv));
    }
  }
  
  
  DxbcRegisterValue DxbcCompiler::emitGsSystemValueLoad(
          DxbcSystemValue         sv,
          DxbcRegMask             mask,
          uint32_t                vertexId) {
    switch (sv) {
      case DxbcSystemValue::Position: {
        uint32_t arrayIndex = m_module.consti32(vertexId);

        if (!m_positionIn) {
          m_positionIn = emitNewBuiltinVariable({
            { DxbcScalarType::Float32, 4, primitiveVertexCount(m_gs.inputPrimitive) },
            spv::StorageClassInput },
            spv::BuiltInPosition,
            "in_position");
        }

        DxbcRegisterPointer ptrIn;
        ptrIn.type.ctype  = DxbcScalarType::Float32;
        ptrIn.type.ccount = 4;
        ptrIn.id = m_module.opAccessChain(
          m_module.defPointerType(getVectorTypeId(ptrIn.type), spv::StorageClassInput),
          m_positionIn, 1, &arrayIndex);
        
        return emitRegisterExtract(emitValueLoad(ptrIn), mask);
      } break;
      
      default:
        throw DxvkError(str::format(
          "DxbcCompiler: Unhandled GS SV input: ", sv));
    }
  }
  
  
  DxbcRegisterValue DxbcCompiler::emitPsSystemValueLoad(
          DxbcSystemValue         sv,
          DxbcRegMask             mask) {
    switch (sv) {
      case DxbcSystemValue::Position: {
        if (m_ps.builtinFragCoord == 0) {
          m_ps.builtinFragCoord = emitNewBuiltinVariable({
            { DxbcScalarType::Float32, 4, 0 },
            spv::StorageClassInput },
            spv::BuiltInFragCoord,
            "ps_frag_coord");
        }
        
        DxbcRegisterPointer ptrIn;
        ptrIn.type = { DxbcScalarType::Float32, 4 };
        ptrIn.id = m_ps.builtinFragCoord;
        
        // The X, Y and Z components of the SV_POSITION semantic
        // are identical to Vulkan's FragCoord builtin, but we
        // need to compute the reciprocal of the W component.
        DxbcRegisterValue fragCoord = emitValueLoad(ptrIn);
        
        uint32_t componentIndex = 3;
        uint32_t t_f32   = m_module.defFloatType(32);
        uint32_t v_wComp = m_module.opCompositeExtract(t_f32, fragCoord.id, 1, &componentIndex);
                 v_wComp = m_module.opFDiv(t_f32, m_module.constf32(1.0f), v_wComp);
        
        fragCoord.id = m_module.opCompositeInsert(
          getVectorTypeId(fragCoord.type),
          v_wComp, fragCoord.id,
          1, &componentIndex);
        
        return emitRegisterExtract(fragCoord, mask);
      } break;
      
      case DxbcSystemValue::IsFrontFace: {
        if (m_ps.builtinIsFrontFace == 0) {
          m_ps.builtinIsFrontFace = emitNewBuiltinVariable({
            { DxbcScalarType::Bool, 1, 0 },
            spv::StorageClassInput },
            spv::BuiltInFrontFacing,
            "ps_is_front_face");
        }
        
        DxbcRegisterValue result;
        result.type.ctype  = DxbcScalarType::Uint32;
        result.type.ccount = 1;
        result.id = m_module.opSelect(
          getVectorTypeId(result.type),
          m_module.opLoad(
            m_module.defBoolType(),
            m_ps.builtinIsFrontFace),
          m_module.constu32(0xFFFFFFFF),
          m_module.constu32(0x00000000));
        return result;
      } break;
      
      case DxbcSystemValue::PrimitiveId: {
        if (m_primitiveIdIn == 0) {
          m_module.enableCapability(spv::CapabilityGeometry);
          
          m_primitiveIdIn = emitNewBuiltinVariable({
            { DxbcScalarType::Uint32, 1, 0 },
            spv::StorageClassInput },
            spv::BuiltInPrimitiveId,
            "ps_primitive_id");
        }
        
        DxbcRegisterPointer ptrIn;
        ptrIn.type = { DxbcScalarType::Uint32, 1 };
        ptrIn.id   = m_primitiveIdIn;
        
        return emitValueLoad(ptrIn);
      } break;
      
      case DxbcSystemValue::SampleIndex: {
        if (m_ps.builtinSampleId == 0) {
          m_module.enableCapability(spv::CapabilitySampleRateShading);
          
          m_ps.builtinSampleId = emitNewBuiltinVariable({
            { DxbcScalarType::Uint32, 1, 0 },
            spv::StorageClassInput },
            spv::BuiltInSampleId,
            "ps_sample_id");
        }
        
        DxbcRegisterPointer ptrIn;
        ptrIn.type.ctype   = DxbcScalarType::Uint32;
        ptrIn.type.ccount  = 1;
        ptrIn.id = m_ps.builtinSampleId;
        
        return emitValueLoad(ptrIn);
      } break;
      
      case DxbcSystemValue::RenderTargetId: {
        if (m_ps.builtinLayer == 0) {
          m_module.enableCapability(spv::CapabilityGeometry);
          
          m_ps.builtinLayer = emitNewBuiltinVariable({
            { DxbcScalarType::Uint32, 1, 0 },
            spv::StorageClassInput },
            spv::BuiltInLayer,
            "v_layer");
        }
        
        DxbcRegisterPointer ptr;
        ptr.type.ctype   = DxbcScalarType::Uint32;
        ptr.type.ccount  = 1;
        ptr.id = m_ps.builtinLayer;
        
        return emitValueLoad(ptr);
      } break;
      
      case DxbcSystemValue::ViewportId: {
        if (m_ps.builtinViewportId == 0) {
          m_module.enableCapability(spv::CapabilityMultiViewport);
          
          m_ps.builtinViewportId = emitNewBuiltinVariable({
            { DxbcScalarType::Uint32, 1, 0 },
            spv::StorageClassInput },
            spv::BuiltInViewportIndex,
            "v_viewport");
        }
        
        DxbcRegisterPointer ptr;
        ptr.type.ctype   = DxbcScalarType::Uint32;
        ptr.type.ccount  = 1;
        ptr.id = m_ps.builtinViewportId;
        
        return emitValueLoad(ptr);
      } break;
      
      default:
        throw DxvkError(str::format(
          "DxbcCompiler: Unhandled PS SV input: ", sv));
    }
  }
  
  
  void DxbcCompiler::emitVsSystemValueStore(
          DxbcSystemValue         sv,
          DxbcRegMask             mask,
    const DxbcRegisterValue&      value) {
    switch (sv) {
      case DxbcSystemValue::Position: {
        if (!m_positionOut) {
          m_positionOut = emitNewBuiltinVariable({
            { DxbcScalarType::Float32, 4, 0 },
            spv::StorageClassOutput },
            spv::BuiltInPosition,
            "out_position");
        }

        DxbcRegisterPointer ptr;
        ptr.type.ctype  = DxbcScalarType::Float32;
        ptr.type.ccount = 4;
        ptr.id = m_positionOut;
        
        emitValueStore(ptr, value, mask);
      } break;
      
      case DxbcSystemValue::RenderTargetId: {
        if (m_programInfo.type() != DxbcProgramType::GeometryShader)
          m_module.enableCapability(spv::CapabilityShaderLayer);

        if (m_gs.builtinLayer == 0) {
          m_module.enableCapability(spv::CapabilityGeometry);

          m_gs.builtinLayer = emitNewBuiltinVariable({
            { DxbcScalarType::Uint32, 1, 0 },
            spv::StorageClassOutput },
            spv::BuiltInLayer,
            "o_layer");
        }
        
        DxbcRegisterPointer ptr;
        ptr.type = { DxbcScalarType::Uint32, 1 };
        ptr.id   = m_gs.builtinLayer;
        
        emitValueStore(
          ptr, emitRegisterExtract(value, mask),
          DxbcRegMask(true, false, false, false));
      } break;
      
      case DxbcSystemValue::ViewportId: {
        if (m_programInfo.type() != DxbcProgramType::GeometryShader)
          m_module.enableCapability(spv::CapabilityShaderViewportIndex);

        if (m_gs.builtinViewportId == 0) {
          m_module.enableCapability(spv::CapabilityMultiViewport);
          
          m_gs.builtinViewportId = emitNewBuiltinVariable({
            { DxbcScalarType::Uint32, 1, 0 },
            spv::StorageClassOutput },
            spv::BuiltInViewportIndex,
            "o_viewport");
        }
        
        DxbcRegisterPointer ptr;
        ptr.type = { DxbcScalarType::Uint32, 1};
        ptr.id   = m_gs.builtinViewportId;
        
        emitValueStore(
          ptr, emitRegisterExtract(value, mask),
          DxbcRegMask(true, false, false, false));
      } break;
      
      default:
        Logger::warn(str::format(
          "DxbcCompiler: Unhandled VS SV output: ", sv));
    }
  }
  
  
  void DxbcCompiler::emitHsSystemValueStore(
          DxbcSystemValue         sv,
          DxbcRegMask             mask,
    const DxbcRegisterValue&      value) {
    if (sv >= DxbcSystemValue::FinalQuadUeq0EdgeTessFactor
     && sv <= DxbcSystemValue::FinalLineDensityTessFactor) {
      struct TessFactor {
        uint32_t array = 0;
        uint32_t index = 0;
      };
      
      static const std::array<TessFactor, 12> s_tessFactors = {{
        { m_hs.builtinTessLevelOuter, 0 },  // FinalQuadUeq0EdgeTessFactor
        { m_hs.builtinTessLevelOuter, 1 },  // FinalQuadVeq0EdgeTessFactor
        { m_hs.builtinTessLevelOuter, 2 },  // FinalQuadUeq1EdgeTessFactor
        { m_hs.builtinTessLevelOuter, 3 },  // FinalQuadVeq1EdgeTessFactor
        { m_hs.builtinTessLevelInner, 0 },  // FinalQuadUInsideTessFactor
        { m_hs.builtinTessLevelInner, 1 },  // FinalQuadVInsideTessFactor
        { m_hs.builtinTessLevelOuter, 0 },  // FinalTriUeq0EdgeTessFactor
        { m_hs.builtinTessLevelOuter, 1 },  // FinalTriVeq0EdgeTessFactor
        { m_hs.builtinTessLevelOuter, 2 },  // FinalTriWeq0EdgeTessFactor
        { m_hs.builtinTessLevelInner, 0 },  // FinalTriInsideTessFactor
        { m_hs.builtinTessLevelOuter, 0 },  // FinalLineDensityTessFactor
        { m_hs.builtinTessLevelOuter, 1 },  // FinalLineDetailTessFactor
      }};
      
      const TessFactor tessFactor = s_tessFactors.at(uint32_t(sv)
        - uint32_t(DxbcSystemValue::FinalQuadUeq0EdgeTessFactor));
      
      const uint32_t tessFactorArrayIndex
        = m_module.constu32(tessFactor.index);
      
      // Apply global tess factor limit
      float maxTessFactor = m_hs.maxTessFactor;

      if (m_moduleInfo.tess != nullptr) {
        if (m_moduleInfo.tess->maxTessFactor < maxTessFactor)
          maxTessFactor = m_moduleInfo.tess->maxTessFactor;
      }

      DxbcRegisterValue tessValue = emitRegisterExtract(value, mask);
      tessValue.id = m_module.opNClamp(getVectorTypeId(tessValue.type),
        tessValue.id, m_module.constf32(0.0f),
        m_module.constf32(maxTessFactor));
      
      DxbcRegisterPointer ptr;
      ptr.type.ctype  = DxbcScalarType::Float32;
      ptr.type.ccount = 1;
      ptr.id = m_module.opAccessChain(
        m_module.defPointerType(
          getVectorTypeId(ptr.type),
          spv::StorageClassOutput),
        tessFactor.array, 1,
        &tessFactorArrayIndex);
      
      emitValueStore(ptr, tessValue,
        DxbcRegMask(true, false, false, false));
    } else {
      Logger::warn(str::format(
        "DxbcCompiler: Unhandled HS SV output: ", sv));
    }
  }
  
  
  void DxbcCompiler::emitGsSystemValueStore(
          DxbcSystemValue         sv,
          DxbcRegMask             mask,
    const DxbcRegisterValue&      value) {
    switch (sv) {
      case DxbcSystemValue::Position:
      case DxbcSystemValue::CullDistance:
      case DxbcSystemValue::ClipDistance:
      case DxbcSystemValue::RenderTargetId:
      case DxbcSystemValue::ViewportId:
        emitVsSystemValueStore(sv, mask, value);
        break;
      
      case DxbcSystemValue::PrimitiveId: {
        if (m_primitiveIdOut == 0) {
          m_primitiveIdOut = emitNewBuiltinVariable({
            { DxbcScalarType::Uint32, 1, 0 },
            spv::StorageClassOutput },
            spv::BuiltInPrimitiveId,
            "gs_primitive_id");
        }
        
        DxbcRegisterPointer ptr;
        ptr.type = { DxbcScalarType::Uint32, 1};
        ptr.id   = m_primitiveIdOut;
        
        emitValueStore(
          ptr, emitRegisterExtract(value, mask),
          DxbcRegMask(true, false, false, false));
      } break;
      
      default:
        Logger::warn(str::format(
          "DxbcCompiler: Unhandled GS SV output: ", sv));
    }
  }
  
  
  void DxbcCompiler::emitPsSystemValueStore(
          DxbcSystemValue         sv,
          DxbcRegMask             mask,
    const DxbcRegisterValue&      value) {
    Logger::warn(str::format(
      "DxbcCompiler: Unhandled PS SV output: ", sv));
  }
  
  
  void DxbcCompiler::emitDsSystemValueStore(
          DxbcSystemValue         sv,
          DxbcRegMask             mask,
    const DxbcRegisterValue&      value) {
    switch (sv) {
      case DxbcSystemValue::Position:
      case DxbcSystemValue::CullDistance:
      case DxbcSystemValue::ClipDistance:
      case DxbcSystemValue::RenderTargetId:
      case DxbcSystemValue::ViewportId:
        emitVsSystemValueStore(sv, mask, value);
        break;
      
      default:
        Logger::warn(str::format(
          "DxbcCompiler: Unhandled DS SV output: ", sv));
    }
  }
  
  
  void DxbcCompiler::emitClipCullStore(
          DxbcSystemValue         sv,
          uint32_t                dstArray) {
    uint32_t offset = 0;
    
    if (dstArray == 0)
      return;
    
    for (auto e = m_osgn->begin(); e != m_osgn->end(); e++) {
      if (e->systemValue == sv) {
        DxbcRegisterPointer srcPtr = m_oRegs.at(e->registerId);
        DxbcRegisterValue srcValue = emitValueLoad(srcPtr);
        
        for (uint32_t i = 0; i < 4; i++) {
          if (e->componentMask[i]) {
            uint32_t offsetId = m_module.consti32(offset++);
            
            DxbcRegisterValue component = emitRegisterExtract(
              srcValue, DxbcRegMask::select(i));
            
            DxbcRegisterPointer dstPtr;
            dstPtr.type = { DxbcScalarType::Float32, 1 };
            dstPtr.id = m_module.opAccessChain(
              m_module.defPointerType(
                getVectorTypeId(dstPtr.type),
                spv::StorageClassOutput),
              dstArray, 1, &offsetId);
            
            emitValueStore(dstPtr, component,
              DxbcRegMask(true, false, false, false));
          }
        }
      }
    }
  }
  
  
  void DxbcCompiler::emitClipCullLoad(
          DxbcSystemValue         sv,
          uint32_t                srcArray) {
    uint32_t offset = 0;
    
    if (srcArray == 0)
      return;
    
    for (auto e = m_isgn->begin(); e != m_isgn->end(); e++) {
      if (e->systemValue == sv) {
        // Load individual components from the source array
        uint32_t                componentIndex = 0;
        std::array<uint32_t, 4> componentIds   = {{ 0, 0, 0, 0 }};
        
        for (uint32_t i = 0; i < 4; i++) {
          if (e->componentMask[i]) {
            uint32_t offsetId = m_module.consti32(offset++);
            
            DxbcRegisterPointer srcPtr;
            srcPtr.type = { DxbcScalarType::Float32, 1 };
            srcPtr.id = m_module.opAccessChain(
              m_module.defPointerType(
                getVectorTypeId(srcPtr.type),
                spv::StorageClassInput),
              srcArray, 1, &offsetId);
            
            componentIds[componentIndex++]
              = emitValueLoad(srcPtr).id;
          }
        }
        
        // Put everything into one vector
        DxbcRegisterValue dstValue;
        dstValue.type = { DxbcScalarType::Float32, componentIndex };
        dstValue.id = componentIds[0];
        
        if (componentIndex > 1) {
          dstValue.id = m_module.opCompositeConstruct(
            getVectorTypeId(dstValue.type),
            componentIndex, componentIds.data());
        }
        
        // Store vector to the input array
        uint32_t registerId = m_module.consti32(e->registerId);
        
        DxbcRegisterPointer dstInput;
        dstInput.type = { DxbcScalarType::Float32, 4 };
        dstInput.id = m_module.opAccessChain(
          m_module.defPointerType(
            getVectorTypeId(dstInput.type),
            spv::StorageClassPrivate),
          m_vArray, 1, &registerId);
        
        emitValueStore(dstInput, dstValue, e->componentMask);
      }
    }
  }
  
  
  void DxbcCompiler::emitPointSizeStore() {
    if (m_moduleInfo.options.needsPointSizeExport) {
      uint32_t pointSizeId = emitNewBuiltinVariable(DxbcRegisterInfo {
        { DxbcScalarType::Float32, 1, 0 },
        spv::StorageClassOutput },
        spv::BuiltInPointSize,
        "point_size");

      m_module.opStore(pointSizeId, m_module.constf32(1.0f));
    }
  }


  void DxbcCompiler::emitInit() {
    // Set up common capabilities for all shaders
    m_module.enableCapability(spv::CapabilityShader);
    m_module.enableCapability(spv::CapabilityImageQuery);
    
    // Initialize the shader module with capabilities
    // etc. Each shader type has its own peculiarities.
    switch (m_programInfo.type()) {
      case DxbcProgramType::VertexShader:   emitVsInit(); break;
      case DxbcProgramType::HullShader:     emitHsInit(); break;
      case DxbcProgramType::DomainShader:   emitDsInit(); break;
      case DxbcProgramType::GeometryShader: emitGsInit(); break;
      case DxbcProgramType::PixelShader:    emitPsInit(); break;
      case DxbcProgramType::ComputeShader:  emitCsInit(); break;
      default: throw DxvkError("Invalid shader stage");
    }
  }
  
  
  void DxbcCompiler::emitFunctionBegin(
          uint32_t                entryPoint,
          uint32_t                returnType,
          uint32_t                funcType) {
    this->emitFunctionEnd();
      
    m_module.functionBegin(
      returnType, entryPoint, funcType,
      spv::FunctionControlMaskNone);
    
    m_insideFunction = true;
  }
  
  
  void DxbcCompiler::emitFunctionEnd() {
    if (m_insideFunction) {
      m_module.opReturn();
      m_module.functionEnd();
    }
    
    m_insideFunction = false;
  }
  
  
  void DxbcCompiler::emitFunctionLabel() {
    m_module.opLabel(m_module.allocateId());
  }
  
  
  void DxbcCompiler::emitMainFunctionBegin() {
    this->emitFunctionBegin(
      m_entryPointId,
      m_module.defVoidType(),
      m_module.defFunctionType(
        m_module.defVoidType(), 0, nullptr));
    this->emitFunctionLabel();
  }
  
  
  void DxbcCompiler::emitVsInit() {
    m_module.enableCapability(spv::CapabilityClipDistance);
    m_module.enableCapability(spv::CapabilityCullDistance);
    m_module.enableCapability(spv::CapabilityDrawParameters);
    
    // Standard input array
    emitDclInputArray(0);
    
    // Cull/clip distances as outputs
    m_clipDistances = emitDclClipCullDistanceArray(
      m_analysis->clipCullOut.numClipPlanes,
      spv::BuiltInClipDistance,
      spv::StorageClassOutput);
    
    m_cullDistances = emitDclClipCullDistanceArray(
      m_analysis->clipCullOut.numCullPlanes,
      spv::BuiltInCullDistance,
      spv::StorageClassOutput);
    
    // Main function of the vertex shader
    m_vs.functionId = m_module.allocateId();
    m_module.setDebugName(m_vs.functionId, "vs_main");
    
    this->emitFunctionBegin(
      m_vs.functionId,
      m_module.defVoidType(),
      m_module.defFunctionType(
        m_module.defVoidType(), 0, nullptr));
    this->emitFunctionLabel();
  }
  
  
  void DxbcCompiler::emitHsInit() {
    m_module.enableCapability(spv::CapabilityTessellation);
    m_module.enableCapability(spv::CapabilityClipDistance);
    m_module.enableCapability(spv::CapabilityCullDistance);
    
    m_hs.builtinInvocationId = emitNewBuiltinVariable(
      DxbcRegisterInfo {
        { DxbcScalarType::Uint32, 1, 0 },
        spv::StorageClassInput },
      spv::BuiltInInvocationId,
      "vOutputControlPointId");
    
    m_hs.builtinTessLevelOuter = emitBuiltinTessLevelOuter(spv::StorageClassOutput);
    m_hs.builtinTessLevelInner = emitBuiltinTessLevelInner(spv::StorageClassOutput);
  }
  
  
  void DxbcCompiler::emitDsInit() {
    m_module.enableCapability(spv::CapabilityTessellation);
    m_module.enableCapability(spv::CapabilityClipDistance);
    m_module.enableCapability(spv::CapabilityCullDistance);
    
    m_ds.builtinTessLevelOuter = emitBuiltinTessLevelOuter(spv::StorageClassInput);
    m_ds.builtinTessLevelInner = emitBuiltinTessLevelInner(spv::StorageClassInput);
    
    // Cull/clip distances as outputs
    m_clipDistances = emitDclClipCullDistanceArray(
      m_analysis->clipCullOut.numClipPlanes,
      spv::BuiltInClipDistance,
      spv::StorageClassOutput);
    
    m_cullDistances = emitDclClipCullDistanceArray(
      m_analysis->clipCullOut.numCullPlanes,
      spv::BuiltInCullDistance,
      spv::StorageClassOutput);
    
    // Main function of the domain shader
    m_ds.functionId = m_module.allocateId();
    m_module.setDebugName(m_ds.functionId, "ds_main");
    
    this->emitFunctionBegin(
      m_ds.functionId,
      m_module.defVoidType(),
      m_module.defFunctionType(
        m_module.defVoidType(), 0, nullptr));
    this->emitFunctionLabel();
  }
  
  
  void DxbcCompiler::emitGsInit() {
    m_module.enableCapability(spv::CapabilityGeometry);
    m_module.enableCapability(spv::CapabilityClipDistance);
    m_module.enableCapability(spv::CapabilityCullDistance);

    // Enable capabilities for xfb mode if necessary
    if (m_moduleInfo.xfb) {
      m_module.enableCapability(spv::CapabilityGeometryStreams);
      m_module.enableCapability(spv::CapabilityTransformFeedback);
      
      m_module.setExecutionMode(m_entryPointId, spv::ExecutionModeXfb);
    }

    // We only need outputs if rasterization is enabled
    m_gs.needsOutputSetup = !m_moduleInfo.xfb
      || m_moduleInfo.xfb->rasterizedStream >= 0;
    
    // Cull/clip distances as outputs
    m_clipDistances = emitDclClipCullDistanceArray(
      m_analysis->clipCullOut.numClipPlanes,
      spv::BuiltInClipDistance,
      spv::StorageClassOutput);
    
    m_cullDistances = emitDclClipCullDistanceArray(
      m_analysis->clipCullOut.numCullPlanes,
      spv::BuiltInCullDistance,
      spv::StorageClassOutput);
    
    // Emit Xfb variables if necessary
    if (m_moduleInfo.xfb)
      emitXfbOutputDeclarations();

    // Main function of the vertex shader
    m_gs.functionId = m_module.allocateId();
    m_module.setDebugName(m_gs.functionId, "gs_main");
    
    this->emitFunctionBegin(
      m_gs.functionId,
      m_module.defVoidType(),
      m_module.defFunctionType(
        m_module.defVoidType(), 0, nullptr));
    this->emitFunctionLabel();
  }
  
  
  void DxbcCompiler::emitPsInit() {
    m_module.enableCapability(spv::CapabilityDerivativeControl);
    
    m_module.setExecutionMode(m_entryPointId,
      spv::ExecutionModeOriginUpperLeft);
    
    // Standard input array
    emitDclInputArray(0);
    
    // Cull/clip distances as inputs
    m_clipDistances = emitDclClipCullDistanceArray(
      m_analysis->clipCullIn.numClipPlanes,
      spv::BuiltInClipDistance,
      spv::StorageClassInput);
    
    m_cullDistances = emitDclClipCullDistanceArray(
      m_analysis->clipCullIn.numCullPlanes,
      spv::BuiltInCullDistance,
      spv::StorageClassInput);
    
    // Main function of the pixel shader
    m_ps.functionId = m_module.allocateId();
    m_module.setDebugName(m_ps.functionId, "ps_main");
    
    this->emitFunctionBegin(
      m_ps.functionId,
      m_module.defVoidType(),
      m_module.defFunctionType(
        m_module.defVoidType(), 0, nullptr));
    this->emitFunctionLabel();
  }
  
  
  void DxbcCompiler::emitCsInit() {
    // Main function of the compute shader
    m_cs.functionId = m_module.allocateId();
    m_module.setDebugName(m_cs.functionId, "cs_main");
    
    this->emitFunctionBegin(
      m_cs.functionId,
      m_module.defVoidType(),
      m_module.defFunctionType(
        m_module.defVoidType(), 0, nullptr));
    this->emitFunctionLabel();
  }
  
  
  void DxbcCompiler::emitVsFinalize() {
    this->emitMainFunctionBegin();
    this->emitInputSetup();
    m_module.opFunctionCall(
      m_module.defVoidType(),
      m_vs.functionId, 0, nullptr);
    this->emitOutputSetup();
    this->emitClipCullStore(DxbcSystemValue::ClipDistance, m_clipDistances);
    this->emitClipCullStore(DxbcSystemValue::CullDistance, m_cullDistances);
    this->emitPointSizeStore();
    this->emitFunctionEnd();
  }
  
  
  void DxbcCompiler::emitHsFinalize() {
    if (m_hs.cpPhase.functionId == 0)
      m_hs.cpPhase = this->emitNewHullShaderPassthroughPhase();
    
    // Control point phase
    this->emitMainFunctionBegin();
    this->emitInputSetup(m_hs.vertexCountIn);
    this->emitHsControlPointPhase(m_hs.cpPhase);
    this->emitHsPhaseBarrier();
    
    // Fork-join phases and output setup
    this->emitHsInvocationBlockBegin(1);
    
    for (const auto& phase : m_hs.forkPhases)
      this->emitHsForkJoinPhase(phase);
    
    for (const auto& phase : m_hs.joinPhases)
      this->emitHsForkJoinPhase(phase);
    
    this->emitOutputSetup();
    this->emitHsOutputSetup();
    this->emitHsInvocationBlockEnd();
    this->emitFunctionEnd();
  }
  
  
  void DxbcCompiler::emitDsFinalize() {
    this->emitMainFunctionBegin();
    m_module.opFunctionCall(
      m_module.defVoidType(),
      m_ds.functionId, 0, nullptr);
    this->emitOutputSetup();
    this->emitClipCullStore(DxbcSystemValue::ClipDistance, m_clipDistances);
    this->emitClipCullStore(DxbcSystemValue::CullDistance, m_cullDistances);
    this->emitFunctionEnd();
  }
  
  
  void DxbcCompiler::emitGsFinalize() {
    if (!m_gs.invocationCount)
      m_module.setInvocations(m_entryPointId, 1);

    this->emitMainFunctionBegin();
    this->emitInputSetup(
      primitiveVertexCount(m_gs.inputPrimitive));
    m_module.opFunctionCall(
      m_module.defVoidType(),
      m_gs.functionId, 0, nullptr);
    // No output setup at this point as that was
    // already done during the EmitVertex step
    this->emitFunctionEnd();
  }
  
  
  void DxbcCompiler::emitPsFinalize() {
    this->emitMainFunctionBegin();
    this->emitInputSetup();
    this->emitClipCullLoad(DxbcSystemValue::ClipDistance, m_clipDistances);
    this->emitClipCullLoad(DxbcSystemValue::CullDistance, m_cullDistances);

    if (m_hasRasterizerOrderedUav) {
      // For simplicity, just lock the entire fragment shader
      // if there are any rasterizer ordered views.
      m_module.enableExtension("SPV_EXT_fragment_shader_interlock");

      if (m_module.hasCapability(spv::CapabilitySampleRateShading)
       && m_moduleInfo.options.enableSampleShadingInterlock) {
        m_module.enableCapability(spv::CapabilityFragmentShaderSampleInterlockEXT);
        m_module.setExecutionMode(m_entryPointId, spv::ExecutionModeSampleInterlockOrderedEXT);
      } else {
        m_module.enableCapability(spv::CapabilityFragmentShaderPixelInterlockEXT);
        m_module.setExecutionMode(m_entryPointId, spv::ExecutionModePixelInterlockOrderedEXT);
      }

      m_module.opBeginInvocationInterlock();
    }

    m_module.opFunctionCall(
      m_module.defVoidType(),
      m_ps.functionId, 0, nullptr);

    if (m_hasRasterizerOrderedUav)
      m_module.opEndInvocationInterlock();

    this->emitOutputSetup();

    if (m_moduleInfo.options.useDepthClipWorkaround)
      this->emitOutputDepthClamp();
    
    this->emitFunctionEnd();
  }
  
  
  void DxbcCompiler::emitCsFinalize() {
    this->emitMainFunctionBegin();

    if (m_moduleInfo.options.zeroInitWorkgroupMemory)
      this->emitInitWorkgroupMemory();

    m_module.opFunctionCall(
      m_module.defVoidType(),
      m_cs.functionId, 0, nullptr);
    
    this->emitFunctionEnd();
  }
  
  
  void DxbcCompiler::emitXfbOutputDeclarations() {
    for (uint32_t i = 0; i < m_moduleInfo.xfb->entryCount; i++) {
      const DxbcXfbEntry* xfbEntry = m_moduleInfo.xfb->entries + i;
      const DxbcSgnEntry* sigEntry = m_osgn->find(
        xfbEntry->semanticName,
        xfbEntry->semanticIndex,
        xfbEntry->streamId);

      if (sigEntry == nullptr)
        continue;
      
      DxbcRegisterInfo varInfo;
      varInfo.type.ctype = DxbcScalarType::Float32;
      varInfo.type.ccount = xfbEntry->componentCount;
      varInfo.type.alength = 0;
      varInfo.sclass = spv::StorageClassOutput;
      
      uint32_t dstComponentMask = (1 << xfbEntry->componentCount) - 1;
      uint32_t srcComponentMask = dstComponentMask
        << sigEntry->componentMask.firstSet()
        << xfbEntry->componentIndex;
      
      DxbcXfbVar xfbVar;
      xfbVar.varId = emitNewVariable(varInfo);
      xfbVar.streamId = xfbEntry->streamId;
      xfbVar.outputId = sigEntry->registerId;
      xfbVar.srcMask = DxbcRegMask(srcComponentMask);
      xfbVar.dstMask = DxbcRegMask(dstComponentMask);
      m_xfbVars.push_back(xfbVar);

      m_module.setDebugName(xfbVar.varId,
        str::format("xfb", i).c_str());
      
      m_module.decorateXfb(xfbVar.varId,
        xfbEntry->streamId, xfbEntry->bufferId, xfbEntry->offset,
        m_moduleInfo.xfb->strides[xfbEntry->bufferId]);
    }

    // TODO Compact location/component assignment
    for (uint32_t i = 0; i < m_xfbVars.size(); i++) {
      m_xfbVars[i].location  = i;
      m_xfbVars[i].component = 0;
    }

    for (uint32_t i = 0; i < m_xfbVars.size(); i++) {
      const DxbcXfbVar* var = &m_xfbVars[i];

      m_module.decorateLocation (var->varId, var->location);
      m_module.decorateComponent(var->varId, var->component);
    }
  }


  void DxbcCompiler::emitXfbOutputSetup(
          uint32_t                          streamId,
          bool                              passthrough) {
    for (size_t i = 0; i < m_xfbVars.size(); i++) {
      if (m_xfbVars[i].streamId == streamId) {
        DxbcRegisterPointer srcPtr = passthrough
          ? m_vRegs[m_xfbVars[i].outputId]
          : m_oRegs[m_xfbVars[i].outputId];

        if (passthrough) {
          srcPtr = emitArrayAccess(srcPtr,
            spv::StorageClassInput,
            m_module.constu32(0));
        }
        
        DxbcRegisterPointer dstPtr;
        dstPtr.type.ctype  = DxbcScalarType::Float32;
        dstPtr.type.ccount = m_xfbVars[i].dstMask.popCount();
        dstPtr.id = m_xfbVars[i].varId;

        DxbcRegisterValue value = emitRegisterExtract(
          emitValueLoad(srcPtr), m_xfbVars[i].srcMask);
        emitValueStore(dstPtr, value, m_xfbVars[i].dstMask);
      }
    }
  }

  
  void DxbcCompiler::emitHsControlPointPhase(
    const DxbcCompilerHsControlPointPhase&  phase) {
    m_module.opFunctionCall(
      m_module.defVoidType(),
      phase.functionId, 0, nullptr);
  }
  
  
  void DxbcCompiler::emitHsForkJoinPhase(
    const DxbcCompilerHsForkJoinPhase&      phase) {
    for (uint32_t i = 0; i < phase.instanceCount; i++) {
      uint32_t invocationId = m_module.constu32(i);
      
      m_module.opFunctionCall(
        m_module.defVoidType(),
        phase.functionId, 1,
        &invocationId);
    }
  }
  
  
  void DxbcCompiler::emitDclInputArray(uint32_t vertexCount) {
    DxbcVectorType info;
    info.ctype   = DxbcScalarType::Float32;
    info.ccount  = 4;

    // Define the array type. This will be two-dimensional
    // in some shaders, with the outer index representing
    // the vertex ID within an invocation.
    m_vArrayLength   = m_isgn != nullptr ? std::max(1u, m_isgn->maxRegisterCount()) : 1;
    m_vArrayLengthId = m_module.lateConst32(getScalarTypeId(DxbcScalarType::Uint32));

    uint32_t vectorTypeId = getVectorTypeId(info);
    uint32_t arrayTypeId = m_module.defArrayType(vectorTypeId, m_vArrayLengthId);
    
    if (vertexCount != 0) {
      arrayTypeId = m_module.defArrayType(
        arrayTypeId, m_module.constu32(vertexCount));
    }
    
    // Define the actual variable. Note that this is private
    // because we will copy input registers and some system
    // variables to the array during the setup phase.
    const uint32_t ptrTypeId = m_module.defPointerType(
      arrayTypeId, spv::StorageClassPrivate);
    
    const uint32_t varId = m_module.newVar(
      ptrTypeId, spv::StorageClassPrivate);
    
    m_module.setDebugName(varId, "shader_in");
    m_vArray = varId;
  }
  
  
  uint32_t DxbcCompiler::emitDclClipCullDistanceArray(
          uint32_t          length,
          spv::BuiltIn      builtIn,
          spv::StorageClass storageClass) {
    if (length == 0)
      return 0;
    
    uint32_t t_f32 = m_module.defFloatType(32);
    uint32_t t_arr = m_module.defArrayType(t_f32, m_module.constu32(length));
    uint32_t t_ptr = m_module.defPointerType(t_arr, storageClass);
    uint32_t varId = m_module.newVar(t_ptr, storageClass);
    
    m_module.decorateBuiltIn(varId, builtIn);
    m_module.setDebugName(varId,
      builtIn == spv::BuiltInClipDistance
        ? "clip_distances"
        : "cull_distances");
    
    return varId;
  }
  
  
  DxbcCompilerHsControlPointPhase DxbcCompiler::emitNewHullShaderControlPointPhase() {
    uint32_t funTypeId = m_module.defFunctionType(
      m_module.defVoidType(), 0, nullptr);
    
    uint32_t funId = m_module.allocateId();
    
    this->emitFunctionBegin(funId,
      m_module.defVoidType(),
      funTypeId);
    this->emitFunctionLabel();
    
    DxbcCompilerHsControlPointPhase result;
    result.functionId = funId;
    return result;
  }
  
  
  DxbcCompilerHsControlPointPhase DxbcCompiler::emitNewHullShaderPassthroughPhase() {
    uint32_t funTypeId = m_module.defFunctionType(
      m_module.defVoidType(), 0, nullptr);
    
    // Begin passthrough function
    uint32_t funId = m_module.allocateId();
    m_module.setDebugName(funId, "hs_passthrough");
    
    this->emitFunctionBegin(funId,
      m_module.defVoidType(),
      funTypeId);
    this->emitFunctionLabel();
    
    // We'll basically copy each input variable to the corresponding
    // output, using the shader's invocation ID as the array index.
    const uint32_t invocationId = m_module.opLoad(
      getScalarTypeId(DxbcScalarType::Uint32),
      m_hs.builtinInvocationId);
    
    for (auto i = m_isgn->begin(); i != m_isgn->end(); i++) {
      this->emitDclInput(
        i->registerId, m_hs.vertexCountIn,
        i->componentMask,
        DxbcSystemValue::None,
        DxbcInterpolationMode::Undefined);
      
      // Vector type index
      const std::array<uint32_t, 2> dstIndices
        = {{ invocationId, m_module.constu32(i->registerId) }};
      
      DxbcRegisterPointer srcPtr;
      srcPtr.type = m_vRegs.at(i->registerId).type;
      srcPtr.id = m_module.opAccessChain(
        m_module.defPointerType(getVectorTypeId(srcPtr.type), spv::StorageClassInput),
        m_vRegs.at(i->registerId).id, 1, &invocationId);
      
      DxbcRegisterValue srcValue = emitRegisterBitcast(
        emitValueLoad(srcPtr), DxbcScalarType::Float32);

      DxbcRegisterPointer dstPtr;
      dstPtr.type = { DxbcScalarType::Float32, 4 };
      dstPtr.id = m_module.opAccessChain(
        m_module.defPointerType(getVectorTypeId(dstPtr.type), spv::StorageClassOutput),
        m_hs.outputPerVertex, dstIndices.size(), dstIndices.data());

      emitValueStore(dstPtr, srcValue, DxbcRegMask::firstN(srcValue.type.ccount));
    }
    
    // End function
    this->emitFunctionEnd();
    
    DxbcCompilerHsControlPointPhase result;
    result.functionId = funId;
    return result;
  }
  
  
  DxbcCompilerHsForkJoinPhase DxbcCompiler::emitNewHullShaderForkJoinPhase() {
    uint32_t argTypeId = m_module.defIntType(32, 0);
    uint32_t funTypeId = m_module.defFunctionType(
      m_module.defVoidType(), 1, &argTypeId);
    
    uint32_t funId = m_module.allocateId();
    
    this->emitFunctionBegin(funId,
      m_module.defVoidType(),
      funTypeId);
    
    uint32_t argId = m_module.functionParameter(argTypeId);
    this->emitFunctionLabel();
    
    DxbcCompilerHsForkJoinPhase result;
    result.functionId = funId;
    result.instanceId = argId;
    return result;
  }
  
  
  void DxbcCompiler::emitHsPhaseBarrier() {
    uint32_t exeScopeId = m_module.constu32(spv::ScopeWorkgroup);
    uint32_t memScopeId = m_module.constu32(spv::ScopeWorkgroup);
    uint32_t semanticId = m_module.constu32(
      spv::MemorySemanticsOutputMemoryMask |
      spv::MemorySemanticsAcquireReleaseMask |
      spv::MemorySemanticsMakeAvailableMask |
      spv::MemorySemanticsMakeVisibleMask);
    
    m_module.opControlBarrier(exeScopeId, memScopeId, semanticId);
  }
  
  
  void DxbcCompiler::emitHsInvocationBlockBegin(uint32_t count) {
    uint32_t invocationId = m_module.opLoad(
      getScalarTypeId(DxbcScalarType::Uint32),
      m_hs.builtinInvocationId);
    
    uint32_t condition = m_module.opULessThan(
      m_module.defBoolType(), invocationId,
      m_module.constu32(count));
    
    m_hs.invocationBlockBegin = m_module.allocateId();
    m_hs.invocationBlockEnd   = m_module.allocateId();
    
    m_module.opSelectionMerge(
      m_hs.invocationBlockEnd,
      spv::SelectionControlMaskNone);
    
    m_module.opBranchConditional(
      condition,
      m_hs.invocationBlockBegin,
      m_hs.invocationBlockEnd);
    
    m_module.opLabel(
      m_hs.invocationBlockBegin);
  }
  
  
  void DxbcCompiler::emitHsInvocationBlockEnd() {
    m_module.opBranch (m_hs.invocationBlockEnd);
    m_module.opLabel  (m_hs.invocationBlockEnd);
    
    m_hs.invocationBlockBegin = 0;
    m_hs.invocationBlockEnd   = 0;
  }


  void DxbcCompiler::emitHsOutputSetup() {
    uint32_t outputPerPatch = emitTessInterfacePerPatch(spv::StorageClassOutput);

    if (!outputPerPatch)
      return;

    uint32_t vecType = getVectorTypeId({ DxbcScalarType::Float32, 4 });

    uint32_t srcPtrType = m_module.defPointerType(vecType, spv::StorageClassPrivate);
    uint32_t dstPtrType = m_module.defPointerType(vecType, spv::StorageClassOutput);

    for (uint32_t i = 0; i < 32; i++) {
      if (m_hs.outputPerPatchMask & (1 << i)) {
        uint32_t index = m_module.constu32(i);

        uint32_t srcPtr = m_module.opAccessChain(srcPtrType, m_hs.outputPerPatch, 1, &index);
        uint32_t dstPtr = m_module.opAccessChain(dstPtrType, outputPerPatch,      1, &index);

        m_module.opStore(dstPtr, m_module.opLoad(vecType, srcPtr));
      }
    }
  }
  
  
  uint32_t DxbcCompiler::emitTessInterfacePerPatch(spv::StorageClass storageClass) {
    const char* name = "vPatch";

    if (storageClass == spv::StorageClassPrivate)
      name = "rPatch";
    if (storageClass == spv::StorageClassOutput)
      name = "oPatch";
    
    uint32_t arrLen  = m_psgn != nullptr ? m_psgn->maxRegisterCount() : 0;

    if (!arrLen)
      return 0;

    uint32_t vecType = m_module.defVectorType (m_module.defFloatType(32), 4);
    uint32_t arrType = m_module.defArrayType  (vecType, m_module.constu32(arrLen));
    uint32_t ptrType = m_module.defPointerType(arrType, storageClass);
    uint32_t varId   = m_module.newVar        (ptrType, storageClass);
    
    m_module.setDebugName     (varId, name);
    
    if (storageClass != spv::StorageClassPrivate) {
      m_module.decorate         (varId, spv::DecorationPatch);
      m_module.decorateLocation (varId, 0);
    }

    return varId;
  }
  
  
  uint32_t DxbcCompiler::emitTessInterfacePerVertex(spv::StorageClass storageClass, uint32_t vertexCount) {
    const bool isInput = storageClass == spv::StorageClassInput;
    
    uint32_t arrLen = isInput
      ? (m_isgn != nullptr ? m_isgn->maxRegisterCount() : 0)
      : (m_osgn != nullptr ? m_osgn->maxRegisterCount() : 0);
    
    if (!arrLen)
      return 0;
    
    uint32_t locIdx = m_psgn != nullptr
      ? m_psgn->maxRegisterCount()
      : 0;
    
    uint32_t vecType      = m_module.defVectorType (m_module.defFloatType(32), 4);
    uint32_t arrTypeInner = m_module.defArrayType  (vecType,      m_module.constu32(arrLen));
    uint32_t arrTypeOuter = m_module.defArrayType  (arrTypeInner, m_module.constu32(vertexCount));
    uint32_t ptrType      = m_module.defPointerType(arrTypeOuter, storageClass);
    uint32_t varId        = m_module.newVar        (ptrType,      storageClass);
    
    m_module.setDebugName     (varId, isInput ? "vVertex" : "oVertex");
    m_module.decorateLocation (varId, locIdx);
    return varId;
  }
  
  
  uint32_t DxbcCompiler::emitSamplePosArray() {
    const std::array<uint32_t, 32> samplePosVectors = {{
      // Invalid sample count / unbound resource
      m_module.constvec2f32( 0.0f, 0.0f),
      // VK_SAMPLE_COUNT_1_BIT
      m_module.constvec2f32( 0.0f, 0.0f),
      // VK_SAMPLE_COUNT_2_BIT
      m_module.constvec2f32( 0.25f, 0.25f),
      m_module.constvec2f32(-0.25f,-0.25f),
      // VK_SAMPLE_COUNT_4_BIT
      m_module.constvec2f32(-0.125f,-0.375f),
      m_module.constvec2f32( 0.375f,-0.125f),
      m_module.constvec2f32(-0.375f, 0.125f),
      m_module.constvec2f32( 0.125f, 0.375f),
      // VK_SAMPLE_COUNT_8_BIT
      m_module.constvec2f32( 0.0625f,-0.1875f),
      m_module.constvec2f32(-0.0625f, 0.1875f),
      m_module.constvec2f32( 0.3125f, 0.0625f),
      m_module.constvec2f32(-0.1875f,-0.3125f),
      m_module.constvec2f32(-0.3125f, 0.3125f),
      m_module.constvec2f32(-0.4375f,-0.0625f),
      m_module.constvec2f32( 0.1875f, 0.4375f),
      m_module.constvec2f32( 0.4375f,-0.4375f),
      // VK_SAMPLE_COUNT_16_BIT
      m_module.constvec2f32( 0.0625f, 0.0625f),
      m_module.constvec2f32(-0.0625f,-0.1875f),
      m_module.constvec2f32(-0.1875f, 0.1250f),
      m_module.constvec2f32( 0.2500f,-0.0625f),
      m_module.constvec2f32(-0.3125f,-0.1250f),
      m_module.constvec2f32( 0.1250f, 0.3125f),
      m_module.constvec2f32( 0.3125f, 0.1875f),
      m_module.constvec2f32( 0.1875f,-0.3125f),
      m_module.constvec2f32(-0.1250f, 0.3750f),
      m_module.constvec2f32( 0.0000f,-0.4375f),
      m_module.constvec2f32(-0.2500f,-0.3750f),
      m_module.constvec2f32(-0.3750f, 0.2500f),
      m_module.constvec2f32(-0.5000f, 0.0000f),
      m_module.constvec2f32( 0.4375f,-0.2500f),
      m_module.constvec2f32( 0.3750f, 0.4375f),
      m_module.constvec2f32(-0.4375f,-0.5000f),
    }};
    
    uint32_t arrayTypeId = getArrayTypeId({
      DxbcScalarType::Float32, 2,
      static_cast<uint32_t>(samplePosVectors.size()) });
    
    uint32_t samplePosArray = m_module.constComposite(
      arrayTypeId,
      samplePosVectors.size(),
      samplePosVectors.data());
    
    uint32_t varId = m_module.newVarInit(
      m_module.defPointerType(arrayTypeId, spv::StorageClassPrivate),
      spv::StorageClassPrivate, samplePosArray);
    
    m_module.setDebugName(varId, "g_sample_pos");
    m_module.decorate(varId, spv::DecorationNonWritable);
    return varId;
  }
  

  void DxbcCompiler::emitFloatControl() {
    DxbcFloatControlFlags flags = m_moduleInfo.options.floatControl;

    if (flags.isClear())
      return;

    const uint32_t width32 = 32;
    const uint32_t width64 = 64;

    if (flags.test(DxbcFloatControlFlag::DenormFlushToZero32)) {
      m_module.enableCapability(spv::CapabilityDenormFlushToZero);
      m_module.setExecutionMode(m_entryPointId, spv::ExecutionModeDenormFlushToZero, 1, &width32);
    }

    if (flags.test(DxbcFloatControlFlag::PreserveNan32)) {
      m_module.enableCapability(spv::CapabilitySignedZeroInfNanPreserve);
      m_module.setExecutionMode(m_entryPointId, spv::ExecutionModeSignedZeroInfNanPreserve, 1, &width32);
    }

    if (m_module.hasCapability(spv::CapabilityFloat64)) {
      if (flags.test(DxbcFloatControlFlag::DenormPreserve64)) {
        m_module.enableCapability(spv::CapabilityDenormPreserve);
        m_module.setExecutionMode(m_entryPointId, spv::ExecutionModeDenormPreserve, 1, &width64);
      }

      if (flags.test(DxbcFloatControlFlag::PreserveNan64)) {
        m_module.enableCapability(spv::CapabilitySignedZeroInfNanPreserve);
        m_module.setExecutionMode(m_entryPointId, spv::ExecutionModeSignedZeroInfNanPreserve, 1, &width64);
      }
    }
  }


  uint32_t DxbcCompiler::emitNewVariable(const DxbcRegisterInfo& info) {
    const uint32_t ptrTypeId = this->getPointerTypeId(info);
    return m_module.newVar(ptrTypeId, info.sclass);
  }
  
  
  uint32_t DxbcCompiler::emitNewBuiltinVariable(
    const DxbcRegisterInfo& info,
          spv::BuiltIn      builtIn,
    const char*             name) {
    const uint32_t varId = emitNewVariable(info);
    
    if (name)
      m_module.setDebugName(varId, name);

    m_module.decorateBuiltIn(varId, builtIn);

    if (m_programInfo.type() == DxbcProgramType::PixelShader
     && info.type.ctype != DxbcScalarType::Float32
     && info.type.ctype != DxbcScalarType::Bool
     && info.sclass == spv::StorageClassInput)
      m_module.decorate(varId, spv::DecorationFlat);

    return varId;
  }
  
  
  uint32_t DxbcCompiler::emitBuiltinTessLevelOuter(spv::StorageClass storageClass) {
    uint32_t id = emitNewBuiltinVariable(
      DxbcRegisterInfo {
        { DxbcScalarType::Float32, 0, 4 },
        storageClass },
      spv::BuiltInTessLevelOuter,
      "bTessLevelOuter");
    
    m_module.decorate(id, spv::DecorationPatch);
    return id;
  }
  
  
  uint32_t DxbcCompiler::emitBuiltinTessLevelInner(spv::StorageClass storageClass) {
    uint32_t id = emitNewBuiltinVariable(
      DxbcRegisterInfo {
        { DxbcScalarType::Float32, 0, 2 },
        storageClass },
      spv::BuiltInTessLevelInner,
      "bTessLevelInner");
    
    m_module.decorate(id, spv::DecorationPatch);
    return id;
  }
  
  
  uint32_t DxbcCompiler::emitPushConstants() {
    uint32_t uintTypeId = m_module.defIntType(32, 0);
    uint32_t structTypeId = m_module.defStructTypeUnique(1, &uintTypeId);

    m_module.setDebugName(structTypeId, "pc_t");
    m_module.setDebugMemberName(structTypeId, 0, "RasterizerSampleCount");
    m_module.memberDecorateOffset(structTypeId, 0, 0);

    uint32_t ptrTypeId = m_module.defPointerType(structTypeId, spv::StorageClassPushConstant);
    uint32_t varId = m_module.newVar(ptrTypeId, spv::StorageClassPushConstant);

    m_module.setDebugName(varId, "pc");
    return varId;
  }


  DxbcCfgBlock* DxbcCompiler::cfgFindBlock(
    const std::initializer_list<DxbcCfgBlockType>& types) {
    for (auto cur =  m_controlFlowBlocks.rbegin();
              cur != m_controlFlowBlocks.rend(); cur++) {
      for (auto type : types) {
        if (cur->type == type)
          return &(*cur);
      }
    }
    
    return nullptr;
  }
  
  
  DxbcBufferInfo DxbcCompiler::getBufferInfo(const DxbcRegister& reg) {
    const uint32_t registerId = reg.idx[0].offset;
    
    switch (reg.type) {
      case DxbcOperandType::Resource: {
        const auto& texture = m_textures.at(registerId);

        DxbcBufferInfo result;
        result.image  = texture.imageInfo;
        result.stype  = texture.sampledType;
        result.type   = texture.type;
        result.typeId = texture.imageTypeId;
        result.varId  = texture.varId;
        result.stride = texture.structStride;
        result.coherence = 0;
        result.isSsbo = texture.isRawSsbo;
        return result;
      } break;
        
      case DxbcOperandType::UnorderedAccessView: {
        const auto& uav = m_uavs.at(registerId);

        DxbcBufferInfo result;
        result.image  = uav.imageInfo;
        result.stype  = uav.sampledType;
        result.type   = uav.type;
        result.typeId = uav.imageTypeId;
        result.varId  = uav.varId;
        result.stride = uav.structStride;
        result.coherence = uav.coherence;
        result.isSsbo = uav.isRawSsbo;
        return result;
      } break;
        
      case DxbcOperandType::ThreadGroupSharedMemory: {
        DxbcBufferInfo result;
        result.image  = { spv::DimBuffer, 0, 0, 0 };
        result.stype  = DxbcScalarType::Uint32;
        result.type   = m_gRegs.at(registerId).type;
        result.typeId = m_module.defPointerType(
          getScalarTypeId(DxbcScalarType::Uint32),
          spv::StorageClassWorkgroup);
        result.varId  = m_gRegs.at(registerId).varId;
        result.stride = m_gRegs.at(registerId).elementStride;
        result.coherence = spv::ScopeInvocation;
        result.isSsbo = false;
        return result;
      } break;
        
      default:
        throw DxvkError(str::format("DxbcCompiler: Invalid operand type for buffer: ", reg.type));
    }
  }
  
  
  uint32_t DxbcCompiler::getTexSizeDim(const DxbcImageInfo& imageType) const {
    switch (imageType.dim) {
      case spv::DimBuffer:  return 1 + imageType.array;
      case spv::Dim1D:      return 1 + imageType.array;
      case spv::Dim2D:      return 2 + imageType.array;
      case spv::Dim3D:      return 3 + imageType.array;
      case spv::DimCube:    return 2 + imageType.array;
      default: throw DxvkError("DxbcCompiler: getTexLayerDim: Unsupported image dimension");
    }
  }
  
  
  uint32_t DxbcCompiler::getTexLayerDim(const DxbcImageInfo& imageType) const {
    switch (imageType.dim) {
      case spv::DimBuffer:  return 1;
      case spv::Dim1D:      return 1;
      case spv::Dim2D:      return 2;
      case spv::Dim3D:      return 3;
      case spv::DimCube:    return 3;
      default: throw DxvkError("DxbcCompiler: getTexLayerDim: Unsupported image dimension");
    }
  }
  
  
  uint32_t DxbcCompiler::getTexCoordDim(const DxbcImageInfo& imageType) const {
    return getTexLayerDim(imageType) + imageType.array;
  }
  
  
  DxbcRegMask DxbcCompiler::getTexCoordMask(const DxbcImageInfo& imageType) const {
    return DxbcRegMask::firstN(getTexCoordDim(imageType));
  }
  

  bool DxbcCompiler::ignoreInputSystemValue(DxbcSystemValue sv) const {
    switch (sv) {
      case DxbcSystemValue::Position:
      case DxbcSystemValue::IsFrontFace:
      case DxbcSystemValue::SampleIndex:
      case DxbcSystemValue::PrimitiveId:
      case DxbcSystemValue::Coverage:
        return m_programInfo.type() == DxbcProgramType::PixelShader;

      default:
        return false;
    }
  }


  void DxbcCompiler::emitUavBarrier(uint64_t readMask, uint64_t writeMask) {
    if (!m_moduleInfo.options.forceComputeUavBarriers
     || m_programInfo.type() != DxbcProgramType::ComputeShader)
      return;

    // If both masks are 0, emit a barrier in case at least one read-write UAV
    // has a pending unsynchronized access. Only consider read-after-write and
    // write-after-read hazards, assume that back-to-back stores are safe and
    // do not overlap in memory. Atomics are also completely ignored here.
    uint64_t rdMask = m_uavRdMask;
    uint64_t wrMask = m_uavWrMask;

    bool insertBarrier = bool(rdMask & wrMask);

    if (readMask || writeMask) {
      rdMask &= m_uavWrMask;
      wrMask &= m_uavRdMask;
    }

    for (auto uav : bit::BitMask(rdMask | wrMask)) {
      constexpr VkAccessFlags rwAccess = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
      insertBarrier |= (m_analysis->uavInfos[uav].accessFlags & rwAccess) == rwAccess;
    }

    // Need to be in uniform top-level control flow, or otherwise
    // it is not safe to insert control barriers.
    if (insertBarrier && m_controlFlowBlocks.empty() && m_topLevelIsUniform) {
      m_module.opControlBarrier(
        m_module.constu32(spv::ScopeWorkgroup),
        m_module.constu32(m_hasGloballyCoherentUav ? spv::ScopeQueueFamily : spv::ScopeWorkgroup),
        m_module.constu32(spv::MemorySemanticsWorkgroupMemoryMask
                        | spv::MemorySemanticsImageMemoryMask
                        | spv::MemorySemanticsUniformMemoryMask
                        | spv::MemorySemanticsAcquireReleaseMask
                        | spv::MemorySemanticsMakeAvailableMask
                        | spv::MemorySemanticsMakeVisibleMask));

      m_uavWrMask = 0u;
      m_uavRdMask = 0u;
    }

    // Mark pending accesses
    m_uavWrMask |= writeMask;
    m_uavRdMask |= readMask;
  }

  
  DxbcVectorType DxbcCompiler::getInputRegType(uint32_t regIdx) const {
    switch (m_programInfo.type()) {
      case DxbcProgramType::VertexShader: {
        const DxbcSgnEntry* entry = m_isgn->findByRegister(regIdx);
        
        DxbcVectorType result;
        result.ctype  = DxbcScalarType::Float32;
        result.ccount = 4;
        
        if (entry != nullptr) {
          result.ctype  = entry->componentType;
          result.ccount = entry->componentMask.popCount();
        }
        
        return result;
      }

      case DxbcProgramType::DomainShader: {
        DxbcVectorType result;
        result.ctype  = DxbcScalarType::Float32;
        result.ccount = 4;
        return result;
      }

      default: {
        DxbcVectorType result;
        result.ctype  = DxbcScalarType::Float32;
        result.ccount = 4;

        if (m_isgn == nullptr || !m_isgn->findByRegister(regIdx))
          return result;

        DxbcRegMask mask(0u);
        DxbcRegMask used(0u);

        for (const auto& e : *m_isgn) {
          if (e.registerId == regIdx && !ignoreInputSystemValue(e.systemValue)) {
            mask |= e.componentMask;
            used |= e.componentUsed;
          }
        }

        if (m_programInfo.type() == DxbcProgramType::PixelShader) {
          if ((used.raw() & mask.raw()) == used.raw())
            mask = used;
        }

        result.ccount = mask.minComponents();
        return result;
      }
    }
  }
  
  
  DxbcVectorType DxbcCompiler::getOutputRegType(uint32_t regIdx) const {
    switch (m_programInfo.type()) {
      case DxbcProgramType::PixelShader: {
        const DxbcSgnEntry* entry = m_osgn->findByRegister(regIdx);

        DxbcVectorType result;
        result.ctype  = DxbcScalarType::Float32;
        result.ccount = 4;
        
        if (entry != nullptr) {
          result.ctype  = entry->componentType;
          result.ccount = entry->componentMask.popCount();
        }

        return result;
      }

      case DxbcProgramType::HullShader: {
        DxbcVectorType result;
        result.ctype  = DxbcScalarType::Float32;
        result.ccount = 4;
        return result;
      }

      default: {
        DxbcVectorType result;
        result.ctype  = DxbcScalarType::Float32;
        result.ccount = 4;

        if (m_osgn->findByRegister(regIdx))
          result.ccount = m_osgn->regMask(regIdx).minComponents();
        return result;
      }
    }
  }
  
  
  DxbcImageInfo DxbcCompiler::getResourceType(
          DxbcResourceDim   resourceType,
          bool              isUav) const {
    uint32_t ms = m_moduleInfo.options.disableMsaa ? 0 : 1;

    switch (resourceType) {
      case DxbcResourceDim::Buffer:         return { spv::DimBuffer, 0, 0, isUav ? 2u : 1u, VK_IMAGE_VIEW_TYPE_MAX_ENUM   };
      case DxbcResourceDim::Texture1D:      return { spv::Dim1D,     0, 0, isUav ? 2u : 1u, VK_IMAGE_VIEW_TYPE_1D         };
      case DxbcResourceDim::Texture1DArr:   return { spv::Dim1D,     1, 0, isUav ? 2u : 1u, VK_IMAGE_VIEW_TYPE_1D_ARRAY   };
      case DxbcResourceDim::Texture2D:      return { spv::Dim2D,     0, 0, isUav ? 2u : 1u, VK_IMAGE_VIEW_TYPE_2D         };
      case DxbcResourceDim::Texture2DArr:   return { spv::Dim2D,     1, 0, isUav ? 2u : 1u, VK_IMAGE_VIEW_TYPE_2D_ARRAY   };
      case DxbcResourceDim::Texture2DMs:    return { spv::Dim2D,     0, ms,isUav ? 2u : 1u, VK_IMAGE_VIEW_TYPE_2D         };
      case DxbcResourceDim::Texture2DMsArr: return { spv::Dim2D,     1, ms,isUav ? 2u : 1u, VK_IMAGE_VIEW_TYPE_2D_ARRAY   };
      case DxbcResourceDim::Texture3D:      return { spv::Dim3D,     0, 0, isUav ? 2u : 1u, VK_IMAGE_VIEW_TYPE_3D         };
      case DxbcResourceDim::TextureCube:    return { spv::DimCube,   0, 0, isUav ? 2u : 1u, VK_IMAGE_VIEW_TYPE_CUBE       };
      case DxbcResourceDim::TextureCubeArr: return { spv::DimCube,   1, 0, isUav ? 2u : 1u, VK_IMAGE_VIEW_TYPE_CUBE_ARRAY };
      default: throw DxvkError(str::format("DxbcCompiler: Unsupported resource type: ", resourceType));
    }
  }
  
  
  spv::ImageFormat DxbcCompiler::getScalarImageFormat(DxbcScalarType type) const {
    switch (type) {
      case DxbcScalarType::Float32: return spv::ImageFormatR32f;
      case DxbcScalarType::Sint32:  return spv::ImageFormatR32i;
      case DxbcScalarType::Uint32:  return spv::ImageFormatR32ui;
      default: throw DxvkError("DxbcCompiler: Unhandled scalar resource type");
    }
  }
  
  
  bool DxbcCompiler::isDoubleType(DxbcScalarType type) const {
    return type == DxbcScalarType::Sint64
        || type == DxbcScalarType::Uint64
        || type == DxbcScalarType::Float64;
  }

  DxbcRegisterPointer DxbcCompiler::getIndexableTempPtr(
    const DxbcRegister&           operand,
          DxbcRegisterValue       vectorId) {
    // x# regs are indexed as follows:
    //    (0) register index (immediate)
    //    (1) element index (relative)
    const uint32_t regId = operand.idx[0].offset;
    
    DxbcRegisterInfo info;
    info.type.ctype   = DxbcScalarType::Float32;
    info.type.ccount  = m_xRegs[regId].ccount;
    info.type.alength = 0;
    info.sclass       = spv::StorageClassPrivate;
    
    DxbcRegisterPointer result;
    result.type.ctype  = info.type.ctype;
    result.type.ccount = info.type.ccount;
    result.id = m_module.opAccessChain(
      getPointerTypeId(info),
      m_xRegs.at(regId).varId,
      1, &vectorId.id);

    return result;
  }

  bool DxbcCompiler::caseBlockIsFallthrough() const {
    return m_lastOp != DxbcOpcode::Case
        && m_lastOp != DxbcOpcode::Default
        && m_lastOp != DxbcOpcode::Break
        && m_lastOp != DxbcOpcode::Ret;
  }


  uint32_t DxbcCompiler::getUavCoherence(uint32_t registerId, DxbcUavFlags flags) {
    // For any ROV with write access, we must ensure that
    // availability operations happen within the locked scope.
    if (flags.test(DxbcUavFlag::RasterizerOrdered)
     && (m_analysis->uavInfos[registerId].accessFlags & VK_ACCESS_SHADER_WRITE_BIT)) {
      m_hasGloballyCoherentUav = true;
      m_hasRasterizerOrderedUav = true;
      return spv::ScopeQueueFamily;
    }

    // Ignore any resources that can't both be read and written in
    // the current shader, explicit availability/visibility operands
    // are not useful in that case.
    if (m_analysis->uavInfos[registerId].accessFlags != (VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT))
      return 0;

    // If the globally coherent flag is set, the resource must be
    // coherent across multiple workgroups of the same dispatch
    if (flags.test(DxbcUavFlag::GloballyCoherent)) {
      m_hasGloballyCoherentUav = true;
      return spv::ScopeQueueFamily;
    }

    // In compute shaders, UAVs are implicitly workgroup coherent,
    // but we can rely on memory barrier instructions to make any
    // access available and visible to the entire workgroup.
    if (m_programInfo.type() == DxbcProgramType::ComputeShader)
      return spv::ScopeInvocation;

    return 0;
  }


  uint32_t DxbcCompiler::getScalarTypeId(DxbcScalarType type) {
    if (type == DxbcScalarType::Float64)
      m_module.enableCapability(spv::CapabilityFloat64);
    
    if (type == DxbcScalarType::Sint64 || type == DxbcScalarType::Uint64)
      m_module.enableCapability(spv::CapabilityInt64);
    
    switch (type) {
      case DxbcScalarType::Uint32:  return m_module.defIntType(32, 0);
      case DxbcScalarType::Uint64:  return m_module.defIntType(64, 0);
      case DxbcScalarType::Sint32:  return m_module.defIntType(32, 1);
      case DxbcScalarType::Sint64:  return m_module.defIntType(64, 1);
      case DxbcScalarType::Float32: return m_module.defFloatType(32);
      case DxbcScalarType::Float64: return m_module.defFloatType(64);
      case DxbcScalarType::Bool:    return m_module.defBoolType();
    }

    throw DxvkError("DxbcCompiler: Invalid scalar type");
  }
  
  
  uint32_t DxbcCompiler::getVectorTypeId(const DxbcVectorType& type) {
    uint32_t typeId = this->getScalarTypeId(type.ctype);
    
    if (type.ccount > 1)
      typeId = m_module.defVectorType(typeId, type.ccount);
    
    return typeId;
  }
  
  
  uint32_t DxbcCompiler::getArrayTypeId(const DxbcArrayType& type) {
    DxbcVectorType vtype;
    vtype.ctype  = type.ctype;
    vtype.ccount = type.ccount;
    
    uint32_t typeId = this->getVectorTypeId(vtype);
    
    if (type.alength != 0) {
      typeId = m_module.defArrayType(typeId,
        m_module.constu32(type.alength));
    }
    
    return typeId;
  }
  
  
  uint32_t DxbcCompiler::getPointerTypeId(const DxbcRegisterInfo& type) {
    return m_module.defPointerType(
      this->getArrayTypeId(type.type),
      type.sclass);
  }
  
  
  uint32_t DxbcCompiler::getSparseResultTypeId(uint32_t baseType) {
    m_module.enableCapability(spv::CapabilitySparseResidency);

    uint32_t uintType = getScalarTypeId(DxbcScalarType::Uint32);
    std::array<uint32_t, 2> typeIds = { uintType, baseType };
    return m_module.defStructType(typeIds.size(), typeIds.data());
  }


  uint32_t DxbcCompiler::getFunctionId(
          uint32_t          functionNr) {
    auto entry = m_subroutines.find(functionNr);
    if (entry != m_subroutines.end())
      return entry->second;
    
    uint32_t functionId = m_module.allocateId();
    m_subroutines.insert({ functionNr, functionId });
    return functionId;
  }


  DxbcCompilerHsForkJoinPhase* DxbcCompiler::getCurrentHsForkJoinPhase() {
    switch (m_hs.currPhaseType) {
      case DxbcCompilerHsPhase::Fork: return &m_hs.forkPhases.at(m_hs.currPhaseId);
      case DxbcCompilerHsPhase::Join: return &m_hs.joinPhases.at(m_hs.currPhaseId);
      default:                        return nullptr;
    }
  }
  
}
