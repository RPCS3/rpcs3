import os
import shutil
import xml.etree.ElementTree as ET
import re

RPCS3DIR = ""

cmake_dirs = ["CMakeFiles",]

cmake_files = ["INSTALL.vcxproj",
               "INSTALL.vcxproj.filters",
               "PACKAGE.vcxproj",
               "PACKAGE.vcxproj.filters",
               "cmake_install.cmake",
               ]

vcxproj_files_blacklist = ["llvm_build.vcxproj","llvm_build.vcxproj.filters"]

vcxproj_files = ["lib\Analysis\LLVMAnalysis.vcxproj",
                "lib\Analysis\IPA\LLVMipa.vcxproj",
                "lib\AsmParser\LLVMAsmParser.vcxproj",
                "lib\Bitcode\Reader\LLVMBitReader.vcxproj",
                "lib\Bitcode\Writer\LLVMBitWriter.vcxproj",
                "lib\CodeGen\LLVMCodeGen.vcxproj",
                "lib\CodeGen\AsmPrinter\LLVMAsmPrinter.vcxproj",
                "lib\CodeGen\SelectionDAG\LLVMSelectionDAG.vcxproj",
                "lib\DebugInfo\LLVMDebugInfo.vcxproj",
                "lib\ExecutionEngine\LLVMExecutionEngine.vcxproj",
                "lib\ExecutionEngine\Interpreter\LLVMInterpreter.vcxproj",
                "lib\ExecutionEngine\JIT\LLVMJIT.vcxproj",
                "lib\ExecutionEngine\MCJIT\LLVMMCJIT.vcxproj",
                "lib\ExecutionEngine\RuntimeDyld\LLVMRuntimeDyld.vcxproj",
                "lib\IR\LLVMCore.vcxproj",
                "lib\IRReader\LLVMIRReader.vcxproj",
                "lib\LineEditor\LLVMLineEditor.vcxproj",
                "lib\Linker\LLVMLinker.vcxproj",
                "lib\LTO\LLVMLTO.vcxproj",
                "lib\MC\LLVMMC.vcxproj",
                "lib\MC\MCAnalysis\LLVMMCAnalysis.vcxproj",
                "lib\MC\MCDisassembler\LLVMMCDisassembler.vcxproj",
                "lib\MC\MCParser\LLVMMCParser.vcxproj",
                "lib\Object\LLVMObject.vcxproj",
                "lib\Option\LLVMOption.vcxproj",
                "lib\ProfileData\LLVMProfileData.vcxproj",
                "lib\Support\LLVMSupport.vcxproj",
                "lib\TableGen\LLVMTableGen.vcxproj",
                "lib\Target\LLVMTarget.vcxproj",
                "lib\Target\X86\LLVMX86CodeGen.vcxproj",
                "lib\Target\X86\X86CommonTableGen.vcxproj",
                "lib\Target\X86\AsmParser\LLVMX86AsmParser.vcxproj",
                "lib\Target\X86\Disassembler\LLVMX86Disassembler.vcxproj",
                "lib\Target\X86\InstPrinter\LLVMX86AsmPrinter.vcxproj",
                "lib\Target\X86\MCTargetDesc\LLVMX86Desc.vcxproj",
                "lib\Target\X86\TargetInfo\LLVMX86Info.vcxproj",
                "lib\Target\X86\Utils\LLVMX86Utils.vcxproj",
                "lib\Transforms\Hello\LLVMHello.vcxproj",
                "lib\Transforms\InstCombine\LLVMInstCombine.vcxproj",
                "lib\Transforms\Instrumentation\LLVMInstrumentation.vcxproj",
                "lib\Transforms\IPO\LLVMipo.vcxproj",
                "lib\Transforms\ObjCARC\LLVMObjCARCOpts.vcxproj",
                "lib\Transforms\Scalar\LLVMScalarOpts.vcxproj",
                "lib\Transforms\Utils\LLVMTransformUtils.vcxproj",
                "lib\Transforms\Vectorize\LLVMVectorize.vcxproj",
                "include\llvm\IR\intrinsics_gen.vcxproj",
                "utils\TableGen\llvm-tblgen.vcxproj",
                 ]

def get_parent_dir():
    path = os.getcwd()
    os.chdir("..")
    rpcs3_dir = os.getcwd()
    os.chdir(path)
    return rpcs3_dir

def rem_cmake_files(root_path):
    for root, dirs, files in os.walk(root_path):
        for directory in dirs:
            if directory in cmake_dirs:
                print("deleting: " + os.path.join(root,directory))
                shutil.rmtree(os.path.join(root,directory))
        dirs = [item for item in dirs if item not in cmake_dirs]
        for file in files:
            if file in cmake_files:
                print("deleting: " + os.path.join(root,file))
                os.remove(os.path.join(root,file))

def repl_cmake_copy(match):
    newtext = "copy /y "
    files = match.group(1)
    files = files.replace('/','\\')
    return newtext+files

def make_paths_relative(file):
    global vcxproj_files
    global vcxproj_files_blacklist
    this_vcxproj = os.path.relpath(file,os.getcwd())
    if this_vcxproj in vcxproj_files_blacklist:
        return
    if (file.find('.vcxproj')!=len(file)-8) and (file.find('.vcxproj.filters')!=len(file)-16):
        print('ERROR: File "'+file+'" is not vcxproj file')
        return
    proj_path = os.path.dirname(file)
    if proj_path[1] != ':':
        print('ERROR: Path "'+proj_path+'" is not in the Windows format')
        return
    #check if we expected this project file
    if file[-8:] == '.vcxproj':
        if this_vcxproj in vcxproj_files:
            vcxproj_files.remove(this_vcxproj)
        else:
            print('ERROR: unexpected vcxproj file: "' + this_vcxproj +'" please update the script')
            return

    #open the file and text-replace the absolute paths
    with open(file,'r') as f:
        file_cont = f.read()
    rel_path = '$(ProjDir)'+os.path.relpath(RPCS3DIR,proj_path)
    file_cont = file_cont.replace(RPCS3DIR,rel_path)
    rpcs3_path_alt = RPCS3DIR.replace('\\','/')
    rel_path_alt = rel_path.replace('\\','/')
    file_cont = file_cont.replace(rpcs3_path_alt,rel_path_alt)
    
    #interpret the XML to remove the cmake commands from the "Command" tags
    ET.register_namespace('','http://schemas.microsoft.com/developer/msbuild/2003')
    tree = ET.fromstring(file_cont)
    
    for parent in tree.findall('.//{http://schemas.microsoft.com/developer/msbuild/2003}Command/..'):
        for element in parent.findall('{http://schemas.microsoft.com/developer/msbuild/2003}Command'):
            text = str(element.text)
            rgx = re.compile(r'[^\r\n<>]+cmake.exe["]?\s+-E copy_if_different([^\r\n]+)')
            text, num_rep = rgx.subn(repl_cmake_copy,text)
            rgx = re.compile(r'[^\r\n<>]+cmake.exe([^\r\n]*)')
            text, num_rep2 = rgx.subn(r'REM OMMITTED CMAKE COMMAND',text)
            num_rep += num_rep2
            rgx = re.compile(r'[^\r\n<>]+ml64.exe"?([^\r\n]*)')
            text, num_rep2 = rgx.subn(r'"$(VCInstallDir)bin\x86_amd64\ml64.exe" \1',text)
            num_rep += num_rep2
            #if (text.find('cmake') != -1) or (text.find('python') != -1):
            #    parent.remove(element)
            if num_rep > 0:
                element.text = text

    #re-create the XML and save the file
    file_cont = ET.tostring(tree,'utf-8')                                  
    with open(file,'w') as f:
        f.write(file_cont)
                                      
    
def iterate_proj_file(root_path):
    for root, dirs, files in os.walk(root_path):
        for file in files:
            if file.find('.vcxproj') != -1 :
                make_paths_relative(os.path.join(root,file))

def main_func():
    global RPCS3DIR
    RPCS3DIR = get_parent_dir()
    rem_cmake_files(os.getcwd())
    iterate_proj_file(os.getcwd())
    for a in vcxproj_files:
        print('ERROR: project file was not found "'+ a + '"')

if __name__ == "__main__":
    main_func()
