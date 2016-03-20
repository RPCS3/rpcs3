================================================================================
OpenGL Mathematics (GLM)
--------------------------------------------------------------------------------
GLM can be distributed and/or modified under the terms of either
a) The Happy Bunny License, or b) the MIT License.

================================================================================
The Happy Bunny License (Modified MIT License)
--------------------------------------------------------------------------------
Copyright (c) 2005 - 2015 G-Truc Creation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

Restrictions:
 By making use of the Software for military purposes, you choose to make a
 Bunny unhappy.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

================================================================================
The MIT License
--------------------------------------------------------------------------------
Copyright (c) 2005 - 2015 G-Truc Creation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

================================================================================
GLM Usage
--------------------------------------------------------------------------------
GLM is a header only library, there is nothing to build, just include it.
#include <glm/glm.hpp>

More informations in GLM manual:
http://glm.g-truc.net/glm.pdf

================================================================================
GLM 0.9.6.3: 2015-02-15
--------------------------------------------------------------------------------
Fixes:
- Fixed Android doesn't have C++ 11 STL #284

================================================================================
GLM 0.9.6.2: 2015-02-15
--------------------------------------------------------------------------------
Features:
- Added display of GLM version with other GLM_MESSAGES
- Added ARM instruction set detection

Improvements:
- Removed assert for perspective with zFar < zNear #298
- Added Visual Studio natvis support for vec1, quat and dualqual types
- Cleaned up C++11 feature detections
- Clarify GLM licensing

Fixes:
- Fixed faceforward build #289
- Fixed conflict with Xlib #define True 1 #293
- Fixed decompose function VS2010 templating issues #294
- Fixed mat4x3 = mat2x3 * mat4x2 operator #297
- Fixed warnings in F2x11_1x10 packing function in GTC_packing #295
- Fixed Visual Studio natvis support for vec4 #288
- Fixed GTC_packing *pack*norm*x* build and added tests #292
- Disabled GTX_scalar_multiplication for GCC, failing to build tests #242
- Fixed Visual C++ 2015 constexpr errors: Disabled only partial support
- Fixed functions not inlined with Clang #302
- Fixed memory corruption (undefined behaviour) #303

================================================================================
GLM 0.9.6.1: 2014-12-10
--------------------------------------------------------------------------------
Features:
- Added GLM_LANG_CXX14_FLAG and GLM_LANG_CXX1Z_FLAG language feature flags
- Added C++14 detection

Improvements:
- Clean up GLM_MESSAGES compilation log to report only detected capabilities

Fixes:
- Fixed scalar uaddCarry build error with Cuda #276
- Fixed C++11 explicit conversion operators detection #282
- Fixed missing explicit convertion when using integer log2 with *vec1 types
- Fixed 64 bits integer GTX_string_cast to_string on VC 32 bit compiler
- Fixed Android build issue, STL C++11 is not supported by the NDK #284
- Fixed unsupported _BitScanForward64 and _BitScanReverse64 in VC10
- Fixed Visual C++ 32 bit build #283
- Fixed GLM_FORCE_SIZE_FUNC pragma message
- Fixed C++98 only build
- Fixed conflict between GTX_compatibility and GTC_quaternion #286
- Fixed C++ language restriction using GLM_FORCE_CXX**

================================================================================
GLM 0.9.6.0: 2014-11-30
--------------------------------------------------------------------------------
Features:
- Exposed template vector and matrix types in 'glm' namespace #239, #244
- Added GTX_scalar_multiplication for C++ 11 compiler only #242
- Added GTX_range for C++ 11 compiler only #240
- Added closestPointOnLine function for tvec2 to GTX_closest_point #238
- Added GTC_vec1 extension, *vec1 support to *vec* types
- Updated GTX_associated_min_max with vec1 support
- Added support of precision and integers to linearRand #230
- Added Integer types support to GTX_string_cast #249
- Added vec3 slerp #237
- Added GTX_common with isdenomal #223
- Added GLM_FORCE_SIZE_FUNC to replace .length() by .size() #245
- Added GLM_FORCE_NO_CTOR_INIT
- Added 'uninitialize' to explicitly not initialize a GLM type
- Added GTC_bitfield extension, promoted GTX_bit
- Added GTC_integer extension, promoted GTX_bit and GTX_integer
- Added GTC_round extension, promoted GTX_bit
- Added GLM_FORCE_EXPLICIT_CTOR to require explicit type conversions #269
- Added GTX_type_aligned for aligned vector, matrix and quaternion types

Improvements:
- Rely on C++11 to implement isinf and isnan
- Removed GLM_FORCE_CUDA, Cuda is implicitly detected
- Separated Apple Clang and LLVM compiler detection
- Used pragma once
- Undetected C++ compiler automatically compile with GLM_FORCE_CXX98 and 
  GLM_FORCE_PURE
- Added not function (from GLSL specification) on VC12
- Optimized bitfieldReverse and bitCount functions
- Optimized findLSB and findMSB functions.
- Optimized matrix-vector multiple performance with Cuda #257, #258
- Reduced integer type redifinitions #233
- Rewrited of GTX_fast_trigonometry #264 #265
- Made types trivially copyable #263
- Removed <iostream> in GLM tests
- Used std features within GLM without redeclaring
- Optimized cot function #272
- Optimized sign function #272
- Added explicit cast from quat to mat3 and mat4 #275

Fixes:
- Fixed std::nextafter not supported with C++11 on Android #217
- Fixed missing value_type for dual quaternion
- Fixed return type of dual quaternion length
- Fixed infinite loop in isfinite function with GCC #221
- Fixed Visual Studio 14 compiler warnings
- Fixed implicit conversion from another tvec2 type to another tvec2 #241
- Fixed lack of consistency of quat and dualquat constructors
- Fixed uaddCarray #253
- Fixed float comparison warnings #270

Deprecation:
- Removed degrees for function parameters
- Removed GLM_FORCE_RADIANS, active by default
- Removed VC 2005 / 8 and 2008 / 9 support
- Removed GCC 3.4 to 4.3 support
- Removed LLVM GCC support
- Removed LLVM 2.6 to 3.1 support
- Removed CUDA 3.0 to 3.2 support

================================================================================
GLM 0.9.5.4: 2014-06-21
--------------------------------------------------------------------------------
- Fixed non-utf8 character #196
- Added FindGLM install for CMake #189
- Fixed GTX_color_space - saturation #195
- Fixed glm::isinf and glm::isnan for with Android NDK 9d #191
- Fixed builtin GLM_ARCH_SSE4 #204
- Optimized Quaternion vector rotation #205
- Fixed missing doxygen @endcond tag #211
- Fixed instruction set detection with Clang #158
- Fixed orientate3 function #207
- Fixed lerp when cosTheta is close to 1 in quaternion slerp #210
- Added GTX_io for io with <iostream> #144
- Fixed fastDistance ambiguity #215
- Fixed tweakedInfinitePerspective #208 and added user-defined epsilon to
  tweakedInfinitePerspective
- Fixed std::copy and std::vector with GLM types #214
- Fixed strict aliasing issues #212, #152
- Fixed std::nextafter not supported with C++11 on Android #213
- Fixed corner cases in exp and log functions for quaternions #199

================================================================================
GLM 0.9.5.3: 2014-04-02
--------------------------------------------------------------------------------
- Added instruction set auto detection with Visual C++ using _M_IX86_FP - /arch
  compiler argument
- Fixed GTX_raw_data code dependency
- Fixed GCC instruction set detection
- Added GLM_GTX_matrix_transform_2d extension (#178, #176)
- Fixed CUDA issues (#169, #168, #183, #182)
- Added support for all extensions but GTX_string_cast to CUDA
- Fixed strict aliasing warnings in GCC 4.8.1 / Android NDK 9c (#152)
- Fixed missing bitfieldInterleave definisions
- Fixed usubBorrow (#171)
- Fixed eulerAngle*** not consistent for right-handed coordinate system (#173)
- Added full tests for eulerAngle*** functions (#173)
- Added workaround for a CUDA compiler bug (#186, #185)

================================================================================
GLM 0.9.5.2: 2014-02-08
--------------------------------------------------------------------------------
- Fixed initializer list ambiguity (#159, #160)
- Fixed warnings with the Android NDK 9c
- Fixed non power of two matrix products
- Fixed mix function link error
- Fixed SSE code included in GLM tests on "pure" platforms
- Fixed undefined reference to fastInverseSqrt (#161)
- Fixed GLM_FORCE_RADIANS with <glm/ext.hpp> build error (#165)
- Fix dot product clamp range for vector angle functions. (#163)
- Tentative fix for strict aliasing warning in GCC 4.8.1 / Android NDK 9c (#152)
- Fixed GLM_GTC_constants description brief (#162)

================================================================================
GLM 0.9.5.1: 2014-01-11
--------------------------------------------------------------------------------
- Fixed angle and orientedAngle that sometimes return NaN values (#145)
- Deprecated degrees for function parameters and display a message
- Added possible static_cast conversion of GLM types (#72)
- Fixed error 'inverse' is not a member of 'glm' from glm::unProject (#146)
- Fixed mismatch between some declarations and definitions
- Fixed inverse link error when using namespace glm; (#147)
- Optimized matrix inverse and division code (#149)
- Added intersectRayPlane function (#153)
- Fixed outerProduct return type (#155)

================================================================================
GLM 0.9.5.0: 2013-12-25
--------------------------------------------------------------------------------
- Added forward declarations (glm/fwd.hpp) for faster compilations
- Added per feature headers
- Minimized GLM internal dependencies
- Improved Intel Compiler detection
- Added bitfieldInterleave and _mm_bit_interleave_si128 functions
- Added GTX_scalar_relational
- Added GTX_dual_quaternion
- Added rotation function to GTX_quaternion (#22)
- Added precision variation of each type
- Added quaternion comparison functions
- Fixed GTX_multiple for negative value
- Removed GTX_ocl_type extension
- Fixed post increment and decrement operators
- Fixed perspective with zNear == 0 (#71)
- Removed l-value swizzle operators
- Cleaned up compiler detection code for unsupported compilers
- Replaced C cast by C++ casts
- Fixed .length() that should return a int and not a size_t
- Added GLM_FORCE_SIZE_T_LENGTH and glm::length_t
- Removed unnecessary conversions
- Optimized packing and unpacking functions
- Removed the normalization of the up argument of lookAt function (#114)
- Added low precision specializations of inversesqrt
- Fixed ldexp and frexp implementations
- Increased assert coverage
- Increased static_assert coverage
- Replaced GLM traits by STL traits when possible
- Allowed including individual core feature
- Increased unit tests completness
- Added creating of a quaternion from two vectors
- Added C++11 initializer lists
- Fixed umulExtended and imulExtended implementations for vector types (#76)
- Fixed CUDA coverage for GTC extensions
- Added GTX_io extension
- Improved GLM messages enabled when defining GLM_MESSAGES
- Hidden matrix _inverse function implementation detail into private section

================================================================================
GLM 0.9.4.6: 2013-09-20
--------------------------------------------------------------------------------
- Fixed detection to select the last known compiler if newer version #106
- Fixed is_int and is_uint code duplication with GCC and C++11 #107 
- Fixed test suite build while using Clang in C++11 mode
- Added c++1y mode support in CMake test suite
- Removed ms extension mode to CMake when no using Visual C++
- Added pedantic mode to CMake test suite for Clang and GCC
- Added use of GCC frontend on Unix for ICC and Visual C++ fronted on Windows
  for ICC
- Added compilation errors for unsupported compiler versions
- Fixed glm::orientation with GLM_FORCE_RADIANS defined #112
- Fixed const ref issue on assignment operator taking a scalar parameter #116
- Fixed glm::eulerAngleY implementation #117

================================================================================
GLM 0.9.4.5: 2013-08-12
--------------------------------------------------------------------------------
- Fixed CUDA support
- Fixed inclusion of intrinsics in "pure" mode #92
- Fixed language detection on GCC when the C++0x mode isn't enabled #95
- Fixed issue #97: register is deprecated in C++11
- Fixed issue #96: CUDA issues
- Added Windows CE detection #92
- Added missing value_ptr for quaternions #99

================================================================================
GLM 0.9.4.4: 2013-05-29
--------------------------------------------------------------------------------
- Fixed slerp when costheta is close to 1 #65
- Fixed mat4x2 value_type constructor #70
- Fixed glm.natvis for Visual C++ 12 #82
- Added assert in inversesqrt to detect division by zero #61
- Fixed missing swizzle operators #86
- Fixed CUDA warnings #86
- Fixed GLM natvis for VC11 #82
- Fixed GLM_GTX_multiple with negative values #79
- Fixed glm::perspective when zNear is zero #71

================================================================================
GLM 0.9.4.3: 2013-03-20
--------------------------------------------------------------------------------
- Detected qualifier for Clang
- Fixed C++11 mode for GCC, couldn't be enabled without MS extensions
- Fixed squad, intermediate and exp quaternion functions
- Fixed GTX_polar_coordinates euclidean function, takes a vec2 instead of a vec3
- Clarify the license applying on the manual
- Added a docx copy of the manual
- Fixed GLM_GTX_matrix_interpolation
- Fixed isnan and isinf on Android with Clang
- Autodetected C++ version using __cplusplus value
- Fixed mix for bool and bvec* third parameter

================================================================================
GLM 0.9.4.2: 2013-02-14
--------------------------------------------------------------------------------
- Fixed compAdd from GTX_component_wise
- Fixed SIMD support for Intel compiler on Windows
- Fixed isnan and isinf for CUDA compiler
- Fixed GLM_FORCE_RADIANS on glm::perspective
- Fixed GCC warnings
- Fixed packDouble2x32 on XCode
- Fixed mix for vec4 SSE implementation
- Fixed 0x2013 dash character in comments that cause issue in Windows 
  Japanese mode
- Fixed documentation warnings
- Fixed CUDA warnings

================================================================================
GLM 0.9.4.1: 2012-12-22
--------------------------------------------------------------------------------
- Improved half support: -0.0 case and implicit conversions
- Fixed Intel Composer Compiler support on Linux
- Fixed interaction between quaternion and euler angles
- Fixed GTC_constants build
- Fixed GTX_multiple
- Fixed quat slerp using mix function when cosTheta close to 1
- Improved fvec4SIMD and fmat4x4SIMD implementations
- Fixed assert messages
- Added slerp and lerp quaternion functions and tests

================================================================================
GLM 0.9.4.0: 2012-11-18
--------------------------------------------------------------------------------
- Added Intel Composer Compiler support
- Promoted GTC_espilon extension
- Promoted GTC_ulp extension
- Removed GLM website from the source repository
- Added GLM_FORCE_RADIANS so that all functions takes radians for arguments
- Fixed detection of Clang and LLVM GCC on MacOS X
- Added debugger visualizers for Visual C++ 2012

================================================================================
GLM 0.9.3.4: 2012-06-30
--------------------------------------------------------------------------------
- Added SSE4 and AVX2 detection.
- Removed VIRTREV_xstream and the incompatibility generated with GCC
- Fixed C++11 compiler option for GCC
- Removed MS language extension option for GCC (not fonctionnal)
- Fixed bitfieldExtract for vector types
- Fixed warnings
- Fixed SSE includes

================================================================================
GLM 0.9.3.3: 2012-05-10
--------------------------------------------------------------------------------
- Fixed isinf and isnan
- Improved compatibility with Intel compiler
- Added CMake test build options: SIMD, C++11, fast math and MS land ext
- Fixed SIMD mat4 test on GCC
- Fixed perspectiveFov implementation
- Fixed matrixCompMult for none-square matrices
- Fixed namespace issue on stream operators
- Fixed various warnings
- Added VC11 support

================================================================================
GLM 0.9.3.2: 2012-03-15
--------------------------------------------------------------------------------
- Fixed doxygen documentation
- Fixed Clang version detection
- Fixed simd mat4 /= operator

================================================================================
GLM 0.9.3.1: 2012-01-25
--------------------------------------------------------------------------------
- Fixed platform detection
- Fixed warnings
- Removed detail code from Doxygen doc

================================================================================
GLM 0.9.3.0: 2012-01-09
--------------------------------------------------------------------------------
- Added CPP Check project
- Fixed conflict with Windows headers
- Fixed isinf implementation
- Fixed Boost conflict
- Fixed warnings

================================================================================
GLM 0.9.3.B: 2011-12-12
--------------------------------------------------------------------------------
- Added support for Chrone Native Client
- Added epsilon constant
- Removed value_size function from vector types
- Fixed roundEven on GCC
- Improved API documentation
- Fixed modf implementation
- Fixed step function accuracy
- Fixed outerProduct

================================================================================
GLM 0.9.3.A: 2011-11-11
--------------------------------------------------------------------------------
- Improved doxygen documentation
- Added new swizzle operators for C++11 compilers
- Added new swizzle operators declared as functions
- Added GLSL 4.20 length for vector and matrix types
- Promoted GLM_GTC_noise extension: simplex, perlin, periodic noise functions
- Promoted GLM_GTC_random extension: linear, gaussian and various random number 
generation distribution
- Added GLM_GTX_constants: provides usefull constants
- Added extension versioning
- Removed many unused namespaces
- Fixed half based type contructors
- Added GLSL core noise functions

================================================================================
GLM 0.9.2.7: 2011-10-24
--------------------------------------------------------------------------------
- Added more swizzling constructors
- Added missing none-squared matrix products

================================================================================
GLM 0.9.2.6: 2011-10-01
--------------------------------------------------------------------------------
- Fixed half based type build on old GCC
- Fixed /W4 warnings on Visual C++
- Fixed some missing l-value swizzle operators

================================================================================
GLM 0.9.2.5: 2011-09-20
--------------------------------------------------------------------------------
- Fixed floatBitToXint functions
- Fixed pack and unpack functions
- Fixed round functions

================================================================================
GLM 0.9.2.4: 2011-09-03
--------------------------------------------------------------------------------
- Fixed extensions bugs

================================================================================
GLM 0.9.2.3: 2011-06-08
--------------------------------------------------------------------------------
- Fixed build issues

================================================================================
GLM 0.9.2.2: 2011-06-02
--------------------------------------------------------------------------------
- Expend matrix constructors flexibility
- Improved quaternion implementation
- Fixed many warnings across platforms and compilers

================================================================================
GLM 0.9.2.1: 2011-05-24
--------------------------------------------------------------------------------
- Automatically detect CUDA support
- Improved compiler detection
- Fixed errors and warnings in VC with C++ extensions disabled
- Fixed and tested GLM_GTX_vector_angle
- Fixed and tested GLM_GTX_rotate_vector

================================================================================
GLM 0.9.2.0: 2011-05-09
--------------------------------------------------------------------------------
- Added CUDA support
- Added CTest test suite
- Added GLM_GTX_ulp extension
- Added GLM_GTX_noise extension
- Added GLM_GTX_matrix_interpolation extension
- Updated quaternion slerp interpolation

================================================================================
GLM 0.9.1.3: 2011-05-07
--------------------------------------------------------------------------------
- Fixed bugs

================================================================================
GLM 0.9.1.2: 2011-04-15
--------------------------------------------------------------------------------
- Fixed bugs

================================================================================
GLM 0.9.1.1: 2011-03-17
--------------------------------------------------------------------------------
- Fixed bugs

================================================================================
GLM 0.9.1.0: 2011-03-03
--------------------------------------------------------------------------------
- Fixed bugs

================================================================================
GLM 0.9.1.B: 2011-02-13
--------------------------------------------------------------------------------
- Updated API documentation
- Improved SIMD implementation
- Fixed Linux build

================================================================================
GLM 0.9.0.8: 2011-02-13
--------------------------------------------------------------------------------
- Added quaternion product operator.
- Clarify that GLM is a header only library.

================================================================================
GLM 0.9.1.A: 2011-01-31
--------------------------------------------------------------------------------
- Added SIMD support
- Added new swizzle functions
- Improved static assert error message with C++0x static_assert
- New setup system
- Reduced branching
- Fixed trunc implementation

================================================================================
GLM 0.9.0.7: 2011-01-30
--------------------------------------------------------------------------------
- Added GLSL 4.10 packing functions
- Added == and != operators for every types.

================================================================================
GLM 0.9.0.6: 2010-12-21
--------------------------------------------------------------------------------
- Many matrices bugs fixed

================================================================================
GLM 0.9.0.5: 2010-11-01
--------------------------------------------------------------------------------
- Improved Clang support
- Fixed bugs

================================================================================
GLM 0.9.0.4: 2010-10-04
--------------------------------------------------------------------------------
- Added autoexp for GLM
- Fixed bugs

================================================================================
GLM 0.9.0.3: 2010-08-26
--------------------------------------------------------------------------------
- Fixed non-squared matrix operators

================================================================================
GLM 0.9.0.2: 2010-07-08
--------------------------------------------------------------------------------
- Added GLM_GTX_int_10_10_10_2
- Fixed bugs

================================================================================
GLM 0.9.0.1: 2010-06-21
--------------------------------------------------------------------------------
- Fixed extensions errors

================================================================================
GLM 0.9.0.0: 2010-05-25
--------------------------------------------------------------------------------
- Objective-C support
- Fixed warnings
- Updated documentation

================================================================================
GLM 0.9.B.2: 2010-04-30
--------------------------------------------------------------------------------
- Git transition
- Removed experimental code from releases
- Fixed bugs

================================================================================
GLM 0.9.B.1: 2010-04-03
--------------------------------------------------------------------------------
- Based on GLSL 4.00 specification
- Added the new core functions
- Added some implicit conversion support

================================================================================
GLM 0.9.A.2: 2010-02-20
--------------------------------------------------------------------------------
- Improved some possible errors messages
- Improved declarations and definitions match

================================================================================
GLM 0.9.A.1: 2010-02-09
--------------------------------------------------------------------------------
- Removed deprecated features
- Internal redesign

================================================================================
GLM 0.8.4.4 final: 2010-01-25
--------------------------------------------------------------------------------
- Fixed warnings

================================================================================
GLM 0.8.4.3 final: 2009-11-16
--------------------------------------------------------------------------------
- Fixed Half float arithmetic
- Fixed setup defines

================================================================================
GLM 0.8.4.2 final: 2009-10-19
--------------------------------------------------------------------------------
- Fixed Half float adds

================================================================================
GLM 0.8.4.1 final: 2009-10-05
--------------------------------------------------------------------------------
- Updated documentation
- Fixed MacOS X build

================================================================================
GLM 0.8.4.0 final: 2009-09-16
--------------------------------------------------------------------------------
- Added GCC 4.4 and VC2010 support
- Added matrix optimizations

================================================================================
GLM 0.8.3.5 final: 2009-08-11
--------------------------------------------------------------------------------
- Fixed bugs

================================================================================
GLM 0.8.3.4 final: 2009-08-10
--------------------------------------------------------------------------------
- Updated GLM according GLSL 1.5 spec
- Fixed bugs

================================================================================
GLM 0.8.3.3 final: 2009-06-25
--------------------------------------------------------------------------------
- Fixed bugs

================================================================================
GLM 0.8.3.2 final: 2009-06-04
--------------------------------------------------------------------------------
- Added GLM_GTC_quaternion
- Added GLM_GTC_type_precision

================================================================================
GLM 0.8.3.1 final: 2009-05-21
--------------------------------------------------------------------------------
- Fixed old extension system.

================================================================================
GLM 0.8.3.0 final: 2009-05-06
--------------------------------------------------------------------------------
- Added stable extensions.
- Added new extension system.

================================================================================
GLM 0.8.2.3 final: 2009-04-01
--------------------------------------------------------------------------------
- Fixed bugs.

================================================================================
GLM 0.8.2.2 final: 2009-02-24
--------------------------------------------------------------------------------
- Fixed bugs.

================================================================================
GLM 0.8.2.1 final: 2009-02-13
--------------------------------------------------------------------------------
- Fixed bugs.

================================================================================
GLM 0.8.2 final: 2009-01-21
--------------------------------------------------------------------------------
- Fixed bugs.

================================================================================
GLM 0.8.1 final: 2008-10-30
--------------------------------------------------------------------------------
- Fixed bugs.

================================================================================
GLM 0.8.0 final: 2008-10-23
--------------------------------------------------------------------------------
- New method to use extension.

================================================================================
GLM 0.8.0 beta3: 2008-10-10
--------------------------------------------------------------------------------
- Added CMake support for GLM tests.

================================================================================
GLM 0.8.0 beta2: 2008-10-04
--------------------------------------------------------------------------------
- Improved half scalars and vectors support.

================================================================================
GLM 0.8.0 beta1: 2008-09-26
--------------------------------------------------------------------------------
- Improved GLSL conformance
- Added GLSL 1.30 support
- Improved API documentation

================================================================================
GLM 0.7.6 final: 2008-08-08
--------------------------------------------------------------------------------
- Improved C++ standard comformance
- Added Static assert for types checking

================================================================================
GLM 0.7.5 final: 2008-07-05
--------------------------------------------------------------------------------
- Added build message system with Visual Studio
- Pedantic build with GCC

================================================================================
GLM 0.7.4 final: 2008-06-01
--------------------------------------------------------------------------------
- Added external dependencies system.

================================================================================
GLM 0.7.3 final: 2008-05-24
--------------------------------------------------------------------------------
- Fixed bugs
- Added new extension group

================================================================================
GLM 0.7.2 final: 2008-04-27
--------------------------------------------------------------------------------
- Updated documentation
- Added preprocessor options

================================================================================
GLM 0.7.1 final: 2008-03-24
--------------------------------------------------------------------------------
- Disabled half on GCC
- Fixed extensions

================================================================================
GLM 0.7.0 final: 2008-03-22
--------------------------------------------------------------------------------
- Changed to MIT license
- Added new documentation

================================================================================
GLM 0.6.4 : 2007-12-10
--------------------------------------------------------------------------------
- Fixed swizzle operators

================================================================================
GLM 0.6.3 : 2007-11-05
--------------------------------------------------------------------------------
- Fixed type data accesses
- Fixed 3DSMax sdk conflict

================================================================================
GLM 0.6.2 : 2007-10-08
--------------------------------------------------------------------------------
- Fixed extension

================================================================================
GLM 0.6.1 : 2007-10-07
--------------------------------------------------------------------------------
- Fixed a namespace error
- Added extensions

================================================================================
GLM 0.6.0 : 2007-09-16
--------------------------------------------------------------------------------
- Added new extension namespace mecanium
- Added Automatic compiler detection

================================================================================
GLM 0.5.1 : 2007-02-19
--------------------------------------------------------------------------------
- Fixed swizzle operators

================================================================================
GLM 0.5.0 : 2007-01-06
--------------------------------------------------------------------------------
- Upgrated to GLSL 1.2
- Added swizzle operators
- Added setup settings

================================================================================
GLM 0.4.1 : 2006-05-22
--------------------------------------------------------------------------------
- Added OpenGL examples

================================================================================
GLM 0.4.0 : 2006-05-17
--------------------------------------------------------------------------------
- Added missing operators to vec* and mat*
- Added first GLSL 1.2 features
- Fixed windows.h before glm.h when windows.h required

================================================================================
GLM 0.3.2 : 2006-04-21
--------------------------------------------------------------------------------
- Fixed texcoord components access.
- Fixed mat4 and imat4 division operators.

================================================================================
GLM 0.3.1 : 2006-03-28
--------------------------------------------------------------------------------
- Added GCC 4.0 support under MacOS X.
- Added GCC 4.0 and 4.1 support under Linux.
- Added code optimisations.

================================================================================
GLM 0.3 : 2006-02-19
--------------------------------------------------------------------------------
- Improved GLSL type conversion and construction compliance.
- Added experimental extensions.
- Added Doxygen Documentation.
- Added code optimisations.
- Fixed bugs.

================================================================================
GLM 0.2: 2005-05-05
--------------------------------------------------------------------------------
- Improve adaptative from GLSL.
- Add experimental extensions based on OpenGL extension process.
- Fixe bugs.

================================================================================
GLM 0.1: 2005-02-21
--------------------------------------------------------------------------------
- Add vec2, vec3, vec4 GLSL types
- Add ivec2, ivec3, ivec4 GLSL types
- Add bvec2, bvec3, bvec4 GLSL types
- Add mat2, mat3, mat4 GLSL types
- Add almost all functions

================================================================================
