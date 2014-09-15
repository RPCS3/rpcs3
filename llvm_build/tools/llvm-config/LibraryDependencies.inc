//===- llvm-build generated file --------------------------------*- C++ -*-===//
//
// Component Library Depenedency Table
//
// Automatically generated file, do not edit!
//
//===----------------------------------------------------------------------===//

struct AvailableComponent {
  /// The name of the component.
  const char *Name;

  /// The name of the library for this component (or NULL).
  const char *Library;

  /// Whether the component is installed.
  bool IsInstalled;

  /// The list of libraries required when linking this component.
  const char *RequiredLibraries[17];
} AvailableComponents[51] = {
  { "all", 0, 1, { "mcjit", "profiledata", "interpreter", "instrumentation", "mcdisassembler", "mcanalysis", "lineeditor", "engine", "option", "debuginfo", "nativecodegen", "tablegen", "gtest_main", "all-targets", "irreader", "lto" } },
  { "all-targets", 0, 1, { "x86" } },
  { "analysis", "libLLVMAnalysis.a", 1, { "core", "support", "target" } },
  { "asmparser", "libLLVMAsmParser.a", 1, { "core", "support" } },
  { "asmprinter", "libLLVMAsmPrinter.a", 1, { "analysis", "codegen", "core", "mc", "mcparser", "support", "target", "transformutils" } },
  { "bitreader", "libLLVMBitReader.a", 1, { "core", "support" } },
  { "bitwriter", "libLLVMBitWriter.a", 1, { "core", "support" } },
  { "codegen", "libLLVMCodeGen.a", 1, { "analysis", "core", "mc", "scalaropts", "support", "target", "transformutils" } },
  { "core", "libLLVMCore.a", 1, { "support" } },
  { "debuginfo", "libLLVMDebugInfo.a", 1, { "object", "support" } },
  { "engine", 0, 1, { "jit", "native" } },
  { "executionengine", "libLLVMExecutionEngine.a", 1, { "core", "mc", "support" } },
  { "gtest", "libgtest.a", 0, { "support" } },
  { "gtest_main", "libgtest_main.a", 0, { "gtest" } },
  { "instcombine", "libLLVMInstCombine.a", 1, { "analysis", "core", "support", "target", "transformutils" } },
  { "instrumentation", "libLLVMInstrumentation.a", 1, { "analysis", "core", "support", "target", "transformutils" } },
  { "interpreter", "libLLVMInterpreter.a", 1, { "codegen", "core", "executionengine", "support" } },
  { "ipa", "libLLVMipa.a", 1, { "analysis", "core", "support" } },
  { "ipo", "libLLVMipo.a", 1, { "analysis", "core", "ipa", "instcombine", "scalaropts", "support", "target", "transformutils", "vectorize" } },
  { "irreader", "libLLVMIRReader.a", 1, { "asmparser", "bitreader", "core", "support" } },
  { "jit", "libLLVMJIT.a", 1, { "codegen", "core", "executionengine", "support" } },
  { "lineeditor", "libLLVMLineEditor.a", 1, { "support" } },
  { "linker", "libLLVMLinker.a", 1, { "core", "support", "transformutils" } },
  { "lto", "libLLVMLTO.a", 1, { "bitreader", "bitwriter", "core", "ipa", "ipo", "instcombine", "linker", "mc", "objcarcopts", "object", "scalaropts", "support", "target", "transformutils" } },
  { "mc", "libLLVMMC.a", 1, { "support" } },
  { "mcanalysis", "libLLVMMCAnalysis.a", 1, { "mc", "object", "support" } },
  { "mcdisassembler", "libLLVMMCDisassembler.a", 1, { "mc", "support" } },
  { "mcjit", "libLLVMMCJIT.a", 1, { "core", "executionengine", "object", "runtimedyld", "support", "target" } },
  { "mcparser", "libLLVMMCParser.a", 1, { "mc", "support" } },
  { "native", 0, 1, { "x86" } },
  { "nativecodegen", 0, 1, { "x86codegen" } },
  { "objcarcopts", "libLLVMObjCARCOpts.a", 1, { "analysis", "core", "support", "transformutils" } },
  { "object", "libLLVMObject.a", 1, { "bitreader", "core", "mc", "mcparser", "support" } },
  { "option", "libLLVMOption.a", 1, { "support" } },
  { "profiledata", "libLLVMProfileData.a", 1, { "support" } },
  { "runtimedyld", "libLLVMRuntimeDyld.a", 1, { "mc", "object", "support" } },
  { "scalaropts", "libLLVMScalarOpts.a", 1, { "analysis", "core", "ipa", "instcombine", "support", "target", "transformutils" } },
  { "selectiondag", "libLLVMSelectionDAG.a", 1, { "analysis", "codegen", "core", "mc", "support", "target", "transformutils" } },
  { "support", "libLLVMSupport.a", 1, {  } },
  { "tablegen", "libLLVMTableGen.a", 1, { "support" } },
  { "target", "libLLVMTarget.a", 1, { "core", "mc", "support" } },
  { "transformutils", "libLLVMTransformUtils.a", 1, { "analysis", "core", "ipa", "support", "target" } },
  { "vectorize", "libLLVMVectorize.a", 1, { "analysis", "core", "support", "target", "transformutils" } },
  { "x86", 0, 1, { "x86utils", "x86asmprinter", "x86info", "x86desc", "x86codegen", "x86asmparser", "x86disassembler" } },
  { "x86asmparser", "libLLVMX86AsmParser.a", 1, { "mc", "mcparser", "support", "x86desc", "x86info" } },
  { "x86asmprinter", "libLLVMX86AsmPrinter.a", 1, { "mc", "support", "x86utils" } },
  { "x86codegen", "libLLVMX86CodeGen.a", 1, { "analysis", "asmprinter", "codegen", "core", "mc", "selectiondag", "support", "target", "x86asmprinter", "x86desc", "x86info", "x86utils" } },
  { "x86desc", "libLLVMX86Desc.a", 1, { "mc", "object", "support", "x86asmprinter", "x86info" } },
  { "x86disassembler", "libLLVMX86Disassembler.a", 1, { "mc", "support", "x86info" } },
  { "x86info", "libLLVMX86Info.a", 1, { "support" } },
  { "x86utils", "libLLVMX86Utils.a", 1, { "support" } },
};
