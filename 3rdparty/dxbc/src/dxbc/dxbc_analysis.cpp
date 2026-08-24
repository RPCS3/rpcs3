#include "dxbc_analysis.h"

namespace dxvk {
  
  DxbcAnalyzer::DxbcAnalyzer(
    const DxbcModuleInfo&     moduleInfo,
    const DxbcProgramInfo&    programInfo,
    const Rc<DxbcIsgn>&       isgn,
    const Rc<DxbcIsgn>&       osgn,
    const Rc<DxbcIsgn>&       psgn,
          DxbcAnalysisInfo&   analysis)
  : m_isgn    (isgn),
    m_osgn    (osgn),
    m_psgn    (psgn),
    m_analysis(&analysis) {
    // Get number of clipping and culling planes from the
    // input and output signatures. We will need this to
    // declare the shader input and output interfaces.
    m_analysis->clipCullIn  = getClipCullInfo(m_isgn);
    m_analysis->clipCullOut = getClipCullInfo(m_osgn);
  }
  
  
  DxbcAnalyzer::~DxbcAnalyzer() {
    
  }
  
  
  void DxbcAnalyzer::processInstruction(const DxbcShaderInstruction& ins) {
    switch (ins.opClass) {
      case DxbcInstClass::Atomic: {
        const uint32_t operandId = ins.dstCount - 1;

        if (ins.dst[operandId].type == DxbcOperandType::UnorderedAccessView) {
          const uint32_t registerId = ins.dst[operandId].idx[0].offset;
          m_analysis->uavInfos[registerId].accessAtomicOp = true;
          m_analysis->uavInfos[registerId].accessFlags |= VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

          // Check whether the atomic operation is order-invariant
          DxvkAccessOp op = DxvkAccessOp::None;

          switch (ins.op) {
            case DxbcOpcode::AtomicAnd:  op = DxvkAccessOp::And;  break;
            case DxbcOpcode::AtomicOr:   op = DxvkAccessOp::Or;   break;
            case DxbcOpcode::AtomicXor:  op = DxvkAccessOp::Xor;  break;
            case DxbcOpcode::AtomicIAdd: op = DxvkAccessOp::Add;  break;
            case DxbcOpcode::AtomicIMax: op = DxvkAccessOp::IMax; break;
            case DxbcOpcode::AtomicIMin: op = DxvkAccessOp::IMin; break;
            case DxbcOpcode::AtomicUMax: op = DxvkAccessOp::UMax; break;
            case DxbcOpcode::AtomicUMin: op = DxvkAccessOp::UMin; break;
            default: break;
          }

          setUavAccessOp(registerId, op);
        }
      } break;

      case DxbcInstClass::TextureSample:
      case DxbcInstClass::TextureGather:
      case DxbcInstClass::TextureQueryLod:
      case DxbcInstClass::VectorDeriv: {
        m_analysis->usesDerivatives = true;
      } break;

      case DxbcInstClass::ControlFlow: {
        if (ins.op == DxbcOpcode::Discard)
          m_analysis->usesKill = true;
      } break;

      case DxbcInstClass::BufferLoad: {
        uint32_t operandId = ins.op == DxbcOpcode::LdStructured ? 2 : 1;
        bool sparseFeedback = ins.dstCount == 2;

        if (ins.src[operandId].type == DxbcOperandType::UnorderedAccessView) {
          const uint32_t registerId = ins.src[operandId].idx[0].offset;
          m_analysis->uavInfos[registerId].accessFlags |= VK_ACCESS_SHADER_READ_BIT;
          m_analysis->uavInfos[registerId].sparseFeedback |= sparseFeedback;

          setUavAccessOp(registerId, DxvkAccessOp::None);
        } else if (ins.src[operandId].type == DxbcOperandType::Resource) {
          const uint32_t registerId = ins.src[operandId].idx[0].offset;
          m_analysis->srvInfos[registerId].sparseFeedback |= sparseFeedback;
        }
      } break;

      case DxbcInstClass::BufferStore: {
        if (ins.dst[0].type == DxbcOperandType::UnorderedAccessView) {
          const uint32_t registerId = ins.dst[0].idx[0].offset;
          m_analysis->uavInfos[registerId].accessFlags |= VK_ACCESS_SHADER_WRITE_BIT;

          setUavAccessOp(registerId, getStoreAccessOp(ins.dst[0].mask, ins.src[ins.srcCount - 1u]));
        }
      } break;

      case DxbcInstClass::TypedUavLoad: {
        const uint32_t registerId = ins.src[1].idx[0].offset;
        m_analysis->uavInfos[registerId].accessTypedLoad = true;
        m_analysis->uavInfos[registerId].accessFlags |= VK_ACCESS_SHADER_READ_BIT;

        setUavAccessOp(registerId, DxvkAccessOp::None);
      } break;

      case DxbcInstClass::TypedUavStore: {
        const uint32_t registerId = ins.dst[0].idx[0].offset;
        m_analysis->uavInfos[registerId].accessFlags |= VK_ACCESS_SHADER_WRITE_BIT;

        // The UAV format may change between dispatches, so be conservative here
        // and only allow this optimization when the app is writing zeroes.
        DxvkAccessOp storeOp = getStoreAccessOp(DxbcRegMask(0xf), ins.src[1u]);

        if (storeOp != DxvkAccessOp(DxvkAccessOp::OpType::StoreUi, 0u))
          storeOp = DxvkAccessOp::None;

        setUavAccessOp(registerId, storeOp);
      } break;

      case DxbcInstClass::Declaration: {
        switch (ins.op) {
          case DxbcOpcode::DclConstantBuffer: {
            uint32_t registerId = ins.dst[0].idx[0].offset;

            if (registerId < DxbcConstBufBindingCount)
              m_analysis->bindings.cbvMask |= 1u << registerId;
          } break;

          case DxbcOpcode::DclSampler: {
            uint32_t registerId = ins.dst[0].idx[0].offset;

            if (registerId < DxbcSamplerBindingCount)
              m_analysis->bindings.samplerMask |= 1u << registerId;
          } break;

          case DxbcOpcode::DclResource:
          case DxbcOpcode::DclResourceRaw:
          case DxbcOpcode::DclResourceStructured: {
            uint32_t registerId = ins.dst[0].idx[0].offset;

            uint32_t idx = registerId / 64u;
            uint32_t bit = registerId % 64u;

            if (registerId < DxbcResourceBindingCount)
              m_analysis->bindings.srvMask[idx] |= uint64_t(1u) << bit;
          } break;

          case DxbcOpcode::DclUavTyped:
          case DxbcOpcode::DclUavRaw:
          case DxbcOpcode::DclUavStructured: {
            uint32_t registerId = ins.dst[0].idx[0].offset;

            if (registerId < DxbcUavBindingCount)
              m_analysis->bindings.uavMask |= uint64_t(1u) << registerId;
          } break;

          default: ;
        }
      } break;

      default:
        break;
    }

    for (uint32_t i = 0; i < ins.dstCount; i++) {
      if (ins.dst[i].type == DxbcOperandType::IndexableTemp) {
        uint32_t index = ins.dst[i].idx[0].offset;
        m_analysis->xRegMasks[index] |= ins.dst[i].mask;
      }
    }
  }
  
  
  DxbcClipCullInfo DxbcAnalyzer::getClipCullInfo(const Rc<DxbcIsgn>& sgn) const {
    DxbcClipCullInfo result;
    
    if (sgn != nullptr) {
      for (auto e = sgn->begin(); e != sgn->end(); e++) {
        const uint32_t componentCount = e->componentMask.popCount();
        
        if (e->systemValue == DxbcSystemValue::ClipDistance)
          result.numClipPlanes += componentCount;
        if (e->systemValue == DxbcSystemValue::CullDistance)
          result.numCullPlanes += componentCount;
      }
    }
    
    return result;
  }


  void DxbcAnalyzer::setUavAccessOp(uint32_t uav, DxvkAccessOp op) {
    if (m_analysis->uavInfos[uav].accessOp == DxvkAccessOp::None)
      m_analysis->uavInfos[uav].accessOp = op;

    // Maintain ordering if the UAV is accessed via other operations as well
    if (op == DxvkAccessOp::None || m_analysis->uavInfos[uav].accessOp != op)
      m_analysis->uavInfos[uav].nonInvariantAccess = true;
  }


  DxvkAccessOp DxbcAnalyzer::getStoreAccessOp(DxbcRegMask writeMask, const DxbcRegister& src) {
    if (src.type != DxbcOperandType::Imm32)
      return DxvkAccessOp::None;

    // Trivial case, same value is written to all components
    if (src.componentCount == DxbcComponentCount::Component1)
      return getConstantStoreOp(src.imm.u32_1);

    if (src.componentCount != DxbcComponentCount::Component4)
      return DxvkAccessOp::None;

    // Otherwise, make sure that all written components are equal
    DxvkAccessOp op = DxvkAccessOp::None;

    for (uint32_t i = 0u; i < 4u; i++) {
      if (!writeMask[i])
        continue;

      // If the written value can't be represented, skip
      DxvkAccessOp scalarOp = getConstantStoreOp(src.imm.u32_4[i]);

      if (scalarOp == DxvkAccessOp::None)
        return DxvkAccessOp::None;

      // First component written
      if (op == DxvkAccessOp::None)
        op = scalarOp;

      // Conflicting store ops
      if (op != scalarOp)
        return DxvkAccessOp::None;
    }

    return op;
  }


  DxvkAccessOp DxbcAnalyzer::getConstantStoreOp(uint32_t value) {
    constexpr uint32_t mask = 0xfffu;

    uint32_t ubits = value & mask;
    uint32_t fbits = (value >> 20u);

    if (value == ubits)
      return DxvkAccessOp(DxvkAccessOp::OpType::StoreUi, ubits);

    if (value == (ubits | ~mask))
      return DxvkAccessOp(DxvkAccessOp::OpType::StoreSi, ubits);

    if (value == (fbits << 20u))
      return DxvkAccessOp(DxvkAccessOp::OpType::StoreF, fbits);

    return DxvkAccessOp::None;
  }

}
