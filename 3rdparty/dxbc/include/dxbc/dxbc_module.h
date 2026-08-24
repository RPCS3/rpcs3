#pragma once

#include "dxbc_chunk_isgn.h"
#include "dxbc_chunk_shex.h"
#include "dxbc_header.h"
#include "dxbc_modinfo.h"
#include "dxbc_reader.h"
#include "dxbc_util.h"

#include "spirv_code_buffer.h"

#include <optional>

// References used for figuring out DXBC:
// - https://github.com/tgjones/slimshader-cpp
// - Wine

namespace dxvk {
  
  class DxbcAnalyzer;
  class DxbcCompiler;
  
  /**
   * \brief Immediate constant buffer properties
   */
  struct DxbcIcbInfo {
    size_t      size = 0u;
    const void* data = nullptr;
  };


  /**
   * \brief DXBC shader module
   * 
   * Reads the DXBC byte code and extracts information
   * about the resource bindings and the instruction
   * stream. A module can then be compiled to SPIR-V.
   */
  class DxbcModule {
    
  public:
    
    DxbcModule(DxbcReader& reader);
    ~DxbcModule();
    
    /**
     * \brief Shader type
     * \returns Shader type
     */
    std::optional<DxbcProgramInfo> programInfo() const {
      if (m_shexChunk == nullptr)
        return std::nullopt;

      return m_shexChunk->programInfo();
    }

    /**
     * \brief Queries shader binding mask
     *
     * Only valid after successfully compiling the shader.
     */
    std::optional<DxbcBindingMask> bindings() const {
      return m_bindings;
    }

    /**
     * \brief Retrieves immediate constant buffer info
     *
     * Only valid after successfully compiling the shader.
     * \returns Immediate constant buffer data
     */
    DxbcIcbInfo icbInfo() const {
      DxbcIcbInfo result = { };
      result.size = m_icb.size() * sizeof(uint32_t);
      result.data = m_icb.data();
      return result;
    }

    /**
     * \brief Input and output signature chunks
     * 
     * Parts of the D3D11 API need access to the
     * input or output signature of the shader.
     */
    Rc<DxbcIsgn> isgn() const { return m_isgnChunk; }
    Rc<DxbcIsgn> osgn() const { return m_osgnChunk; }

    /**
     * \brief Compiles DXBC shader to SPIR-V module
     * 
     * \param [in] moduleInfo DXBC module info
     * \param [in] fileName File name, will be added to
     *        the compiled SPIR-V for debugging purposes.
     * \returns The compiled shader object
     */
    SpirvCodeBuffer compile(
      const DxbcModuleInfo& moduleInfo,
      const std::string&    fileName);
    
    /**
     * \brief Compiles a pass-through geometry shader
     *
     * Applications can pass a vertex shader to create
     * a geometry shader with stream output. In this
     * case, we have to create a passthrough geometry
     * shader, which operates in point to point mode.
     * \param [in] moduleInfo DXBC module info
     * \param [in] fileName SPIR-V shader name
     */
    SpirvCodeBuffer compilePassthroughShader(
      const DxbcModuleInfo& moduleInfo,
      const std::string&    fileName) const;

  private:
    
    DxbcHeader   m_header;
    
    Rc<DxbcIsgn> m_isgnChunk;
    Rc<DxbcIsgn> m_osgnChunk;
    Rc<DxbcIsgn> m_psgnChunk;
    Rc<DxbcShex> m_shexChunk;

    std::vector<uint32_t> m_icb;

    std::optional<DxbcBindingMask> m_bindings;
    
    void runAnalyzer(
            DxbcAnalyzer&       analyzer,
            DxbcCodeSlice       slice) const;
    
    void runCompiler(
            DxbcCompiler&       compiler,
            DxbcCodeSlice       slice) const;
    
  };
  
}
