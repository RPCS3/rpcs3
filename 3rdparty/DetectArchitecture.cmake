# From https://github.com/merryhime/dynarmic
include(CheckSymbolExists)

if (CMAKE_OSX_ARCHITECTURES)
    set(DYNARMIC_MULTIARCH_BUILD 1)
    set(ARCHITECTURE "${CMAKE_OSX_ARCHITECTURES}")
    return()
endif()

function(detect_architecture symbol arch)
    if (NOT DEFINED ARCHITECTURE)
        set(CMAKE_REQUIRED_QUIET YES)
        check_symbol_exists("${symbol}" "" DETECT_ARCHITECTURE_${arch})
        unset(CMAKE_REQUIRED_QUIET)

        if (DETECT_ARCHITECTURE_${arch})
            set(ARCHITECTURE "${arch}" PARENT_SCOPE)
        endif()

        unset(DETECT_ARCHITECTURE_${arch} CACHE)
    endif()
endfunction()

detect_architecture("__ARM64__" arm64)
detect_architecture("__aarch64__" arm64)
detect_architecture("_M_ARM64" arm64)

detect_architecture("__arm__" arm)
detect_architecture("__TARGET_ARCH_ARM" arm)
detect_architecture("_M_ARM" arm)

detect_architecture("__x86_64" x86_64)
detect_architecture("__x86_64__" x86_64)
detect_architecture("__amd64" x86_64)
detect_architecture("_M_X64" x86_64)

detect_architecture("__i386" x86)
detect_architecture("__i386__" x86)
detect_architecture("_M_IX86" x86)

detect_architecture("__ia64" ia64)
detect_architecture("__ia64__" ia64)
detect_architecture("_M_IA64" ia64)

detect_architecture("__mips" mips)
detect_architecture("__mips__" mips)
detect_architecture("_M_MRX000" mips)

detect_architecture("__ppc64__" ppc64)
detect_architecture("__powerpc64__" ppc64)

detect_architecture("__ppc__" ppc)
detect_architecture("__ppc" ppc)
detect_architecture("__powerpc__" ppc)
detect_architecture("_ARCH_COM" ppc)
detect_architecture("_ARCH_PWR" ppc)
detect_architecture("_ARCH_PPC" ppc)
detect_architecture("_M_MPPC" ppc)
detect_architecture("_M_PPC" ppc)

detect_architecture("__riscv" riscv)

detect_architecture("__EMSCRIPTEN__" wasm)
