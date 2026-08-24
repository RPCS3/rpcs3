#pragma once

#include "dxbc_chunk_isgn.h"
#include "dxbc_decoder.h"
#include "dxbc_defs.h"
#include "dxbc_names.h"
#include "dxbc_modinfo.h"
#include "dxbc_util.h"

namespace dxvk {
  
  /**
   * \brief Info about unordered access views
   * 
   * Stores whether an UAV is accessed with typed
   * read or atomic instructions. This information
   * will be used to generate image types.
   */
  struct DxbcUavInfo {
    bool accessTypedLoad    = false;
    bool accessAtomicOp     = false;
    bool sparseFeedback     = false;
    bool nonInvariantAccess = false;
    DxvkAccessOp accessOp   = DxvkAccessOp::None;
    VkAccessFlags accessFlags = 0;
  };
  
  /**
   * \brief Info about shader resource views
   *
   * Stores whether an SRV is accessed with
   * sparse feedback. Useful for buffers.
   */
  struct DxbcSrvInfo {
    bool sparseFeedback  = false;
  };

  /**
   * \brief Counts cull and clip distances
   */
  struct DxbcClipCullInfo {
    uint32_t numClipPlanes = 0;
    uint32_t numCullPlanes = 0;
  };
  
  /**
   * \brief Shader analysis info
   */
  struct DxbcAnalysisInfo {
    std::array<DxbcUavInfo, 64>   uavInfos;
    std::array<DxbcSrvInfo, 128>  srvInfos;
    std::array<DxbcRegMask, 4096> xRegMasks;
    
    DxbcClipCullInfo clipCullIn;
    DxbcClipCullInfo clipCullOut;

    DxbcBindingMask bindings = { };
    
    bool usesDerivatives  = false;
    bool usesKill         = false;
  };
  
  /**
   * \brief DXBC shader analysis pass
   * 
   * Collects information about the shader itself
   * and the resources used by the shader, which
   * will later be used by the actual compiler.
   */
  class DxbcAnalyzer {
    
  public:
    
    DxbcAnalyzer(
      const DxbcModuleInfo&     moduleInfo,
      const DxbcProgramInfo&    programInfo,
      const Rc<DxbcIsgn>&       isgn,
      const Rc<DxbcIsgn>&       osgn,
      const Rc<DxbcIsgn>&       psgn,
            DxbcAnalysisInfo&   analysis);
    
    ~DxbcAnalyzer();
    
    /**
     * \brief Processes a single instruction
     * \param [in] ins The instruction
     */
    void processInstruction(
      const DxbcShaderInstruction&  ins);
    
  private:
    
    Rc<DxbcIsgn> m_isgn;
    Rc<DxbcIsgn> m_osgn;
    Rc<DxbcIsgn> m_psgn;
    
    DxbcAnalysisInfo* m_analysis = nullptr;
    
    DxbcClipCullInfo getClipCullInfo(
      const Rc<DxbcIsgn>& sgn) const;

    void setUavAccessOp(uint32_t uav, DxvkAccessOp op);

    static DxvkAccessOp getStoreAccessOp(DxbcRegMask writeMask, const DxbcRegister& src);

    static DxvkAccessOp getConstantStoreOp(uint32_t value);

  };
  
}
