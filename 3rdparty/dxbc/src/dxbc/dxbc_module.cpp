#include "dxbc_analysis.h"
#include "dxbc_compiler.h"
#include "dxbc_module.h"

namespace dxvk {
  
  DxbcModule::DxbcModule(DxbcReader& reader)
  : m_header(reader) {
    for (uint32_t i = 0; i < m_header.numChunks(); i++) {
      
      // The chunk tag is stored at the beginning of each chunk
      auto chunkReader = reader.clone(m_header.chunkOffset(i));
      auto tag         = chunkReader.readTag();
      
      // The chunk size follows right after the four-character
      // code. This does not include the eight bytes that are
      // consumed by the FourCC and chunk length entry.
      auto chunkLength = chunkReader.readu32();
      
      chunkReader = chunkReader.clone(8);
      chunkReader = chunkReader.resize(chunkLength);
      
      if ((tag == "SHDR") || (tag == "SHEX"))
        m_shexChunk = new DxbcShex(chunkReader);
      
      if ((tag == "ISGN") || (tag == "ISG1"))
        m_isgnChunk = new DxbcIsgn(chunkReader, tag);
      
      if ((tag == "OSGN") || (tag == "OSG5") || (tag == "OSG1"))
        m_osgnChunk = new DxbcIsgn(chunkReader, tag);
      
      if ((tag == "PCSG") || (tag == "PSG1"))
        m_psgnChunk = new DxbcIsgn(chunkReader, tag);
    }
  }
  
  
  DxbcModule::~DxbcModule() {
    
  }
  
  
  SpirvCodeBuffer DxbcModule::compile(
    const DxbcModuleInfo& moduleInfo,
    const std::string&    fileName) {
    if (m_shexChunk == nullptr)
      throw DxvkError("DxbcModule::compile: No SHDR/SHEX chunk");
    
    DxbcAnalysisInfo analysisInfo;
    
    DxbcAnalyzer analyzer(moduleInfo,
      m_shexChunk->programInfo(),
      m_isgnChunk, m_osgnChunk,
      m_psgnChunk, analysisInfo);
    
    this->runAnalyzer(analyzer, m_shexChunk->slice());

    m_bindings = std::make_optional(analysisInfo.bindings);
    
    DxbcCompiler compiler(
      fileName, moduleInfo,
      m_shexChunk->programInfo(),
      m_isgnChunk, m_osgnChunk,
      m_psgnChunk, analysisInfo);
    
    this->runCompiler(compiler, m_shexChunk->slice());

    m_icb = compiler.getIcbData();

    return compiler.finalize();
  }
  
  
  SpirvCodeBuffer DxbcModule::compilePassthroughShader(
    const DxbcModuleInfo& moduleInfo,
    const std::string&    fileName) const {
    if (m_shexChunk == nullptr)
      throw DxvkError("DxbcModule::compile: No SHDR/SHEX chunk");
    
    DxbcAnalysisInfo analysisInfo;

    DxbcCompiler compiler(
      fileName, moduleInfo,
      DxbcProgramType::GeometryShader,
      m_osgnChunk, m_osgnChunk,
      m_psgnChunk, analysisInfo);
    
    compiler.processXfbPassthrough();
    return compiler.finalize();
  }


  void DxbcModule::runAnalyzer(
          DxbcAnalyzer&       analyzer,
          DxbcCodeSlice       slice) const {
    DxbcDecodeContext decoder;
    
    while (!slice.atEnd()) {
      decoder.decodeInstruction(slice);
      
      analyzer.processInstruction(
        decoder.getInstruction());
    }
  }
  
  
  void DxbcModule::runCompiler(
          DxbcCompiler&       compiler,
          DxbcCodeSlice       slice) const {
    DxbcDecodeContext decoder;
    
    while (!slice.atEnd()) {
      decoder.decodeInstruction(slice);
      
      compiler.processInstruction(
        decoder.getInstruction());
    }
  }
  
}
