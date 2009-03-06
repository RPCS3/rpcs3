// Copyright (c) 2006, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

// ----
// Author: Matt Austern
//
// Define a small subset of tr1 type traits. The traits we define are:
//   is_integral
//   is_floating_point
//   is_pointer
//   is_reference
//   is_pod
//   has_trivial_constructor
//   has_trivial_copy
//   has_trivial_assign
//   has_trivial_destructor
//   remove_const
//   remove_volatile
//   remove_cv
//   remove_reference
//   remove_pointer
//   is_convertible
// We can add more type traits as required.

#ifndef BASE_TYPE_TRAITS_H_
#define BASE_TYPE_TRAITS_H_

#include <google/sparsehash/sparseconfig.h>
#include <utility>                  // For pair

_START_GOOGLE_NAMESPACE_

// integral_constant, defined in tr1, is a wrapper for an integer
// value. We don't really need this generality; we could get away
// with hardcoding the integer type to bool. We use the fully
// general integer_constant for compatibility with tr1.

template<class T, T v>
struct integral_constant {
  static const T value = v;
  typedef T value_type;
  typedef integral_constant<T, v> type;
};

template <class T, T v> const T integral_constant<T, v>::value;

// Abbreviations: true_type and false_type are structs that represent
// boolean true and false values.
typedef integral_constant<bool, true>  true_type;
typedef integral_constant<bool, false> false_type;

// Types small_ and big_ are guaranteed such that sizeof(small_) <
// sizeof(big_)
typedef char small_;

struct big_ {
  char dummy[2];
};

// is_integral is false except for the built-in integer types.
template <class T> struct is_integral : false_type { };
template<> struct is_integral<bool> : true_type { };
template<> struct is_integral<char> : true_type { };
template<> struct is_integral<unsigned char> : true_type { };
template<> struct is_integral<signed char> : true_type { };
#if defined(_MSC_VER)
// wchar_t is not by default a distinct type from unsigned short in
// Microsoft C.
// See http://msdn2.microsoft.com/en-us/library/dh8che7s(VS.80).aspx
template<> struct is_integral<__wchar_t> : true_type { };
#else
template<> struct is_integral<wchar_t> : true_type { };
#endif
template<> struct is_integral<short> : true_type { };
template<> struct is_integral<unsigned short> : true_type { };
template<> struct is_integral<int> : true_type { };
template<> struct is_integral<unsigned int> : true_type { };
template<> struct is_integral<long> : true_type { };
template<> struct is_integral<unsigned long> : true_type { };
#ifdef HAVE_LONG_LONG
template<> struct is_integral<long long> : true_type { };
template<> struct is_integral<unsigned long long> : true_type { };
#endif


// is_floating_point is false except for the built-in floating-point types.
template <class T> struct is_floating_point : false_type { };
template<> struct is_floating_point<float> : true_type { };
template<> struct is_floating_point<double> : true_type { };
template<> struct is_floating_point<long double> : true_type { };


// is_pointer is false except for pointer types.
template <class T> struct is_pointer : false_type { };
template <class T> struct is_pointer<T*> : true_type { };


// is_reference is false except for reference types.
template<typename T> struct is_reference : false_type {};
template<typename T> struct is_reference<T&> : true_type {};


// We can't get is_pod right without compiler help, so fail conservatively.
// We will assume it's false except for arithmetic types and pointers,
// and const versions thereof. Note that std::pair is not a POD.
template <class T> struct is_pod
 : integral_constant<bool, (is_integral<T>::value ||
                            is_floating_point<T>::value ||
                            is_pointer<T>::value)> { };
template <class T> struct is_pod<const T> : is_pod<T> { };


// We can't get has_trivial_constructor right without compiler help, so
// fail conservatively. We will assume it's false except for: (1) types
// for which is_pod is true. (2) std::pair of types with trivial
// constructors. (3) array of a type with a trivial constructor.
// (4) const versions thereof.
template <class T> struct has_trivial_constructor : is_pod<T> { };
template <class T, class U> struct has_trivial_constructor<std::pair<T, U> >
  : integral_constant<bool,
                      (has_trivial_constructor<T>::value &&
                       has_trivial_constructor<U>::value)> { };
template <class A, int N> struct has_trivial_constructor<A[N]>
  : has_trivial_constructor<A> { };
template <class T> struct has_trivial_constructor<const T>
  : has_trivial_constructor<T> { };

// We can't get has_trivial_copy right without compiler help, so fail
// conservatively. We will assume it's false except for: (1) types
// for which is_pod is true. (2) std::pair of types with trivial copy
// constructors. (3) array of a type with a trivial copy constructor.
// (4) const versions thereof.
template <class T> struct has_trivial_copy : is_pod<T> { };
template <class T, class U> struct has_trivial_copy<std::pair<T, U> >
  : integral_constant<bool,
                      (has_trivial_copy<T>::value &&
                       has_trivial_copy<U>::value)> { };
template <class A, int N> struct has_trivial_copy<A[N]>
  : has_trivial_copy<A> { };
template <class T> struct has_trivial_copy<const T> : has_trivial_copy<T> { };

// We can't get has_trivial_assign right without compiler help, so fail
// conservatively. We will assume it's false except for: (1) types
// for which is_pod is true. (2) std::pair of types with trivial copy
// constructors. (3) array of a type with a trivial assign constructor.
template <class T> struct has_trivial_assign : is_pod<T> { };
template <class T, class U> struct has_trivial_assign<std::pair<T, U> >
  : integral_constant<bool,
                      (has_trivial_assign<T>::value &&
                       has_trivial_assign<U>::value)> { };
template <class A, int N> struct has_trivial_assign<A[N]>
  : has_trivial_assign<A> { };

// We can't get has_trivial_destructor right without compiler help, so
// fail conservatively. We will assume it's false except for: (1) types
// for which is_pod is true. (2) std::pair of types with trivial
// destructors. (3) array of a type with a trivial destructor.
// (4) const versions thereof.
template <class T> struct has_trivial_destructor : is_pod<T> { };
template <class T, class U> struct has_trivial_destructor<std::pair<T, U> >
  : integral_constant<bool,
                      (has_trivial_destructor<T>::value &&
                       has_trivial_destructor<U>::value)> { };
template <class A, int N> struct has_trivial_destructor<A[N]>
  : has_trivial_destructor<A> { };
template <class T> struct has_trivial_destructor<const T>
  : has_trivial_destructor<T> { };

// Specified by TR1 [4.7.1]
template<typename T> struct remove_const { typedef T type; };
template<typename T> struct remove_const<T const> { typedef T type; };
template<typename T> struct remove_volatile { typedef T type; };
template<typename T> struct remove_volatile<T volatile> { typedef T type; };
template<typename T> struct remove_cv {
  typedef typename remove_const<typename remove_volatile<T>::type>::type type;
};


// Specified by TR1 [4.7.2]
template<typename T> struct remove_reference { typedef T type; };
template<typename T> struct remove_reference<T&> { typedef T type; };

// Specified by TR1 [4.7.4] Pointer modifications.
template<typename T> struct remove_pointer { typedef T type; };
template<typename T> struct remove_pointer<T*> { typedef T type; };
template<typename T> struct remove_pointer<T* const> { typedef T type; };
template<typename T> struct remove_pointer<T* volatile> { typedef T type; };
template<typename T> struct remove_pointer<T* const volatile> {
  typedef T type; };

// Specified by TR1 [4.6] Relationships between types
#ifndef _MSC_VER
namespace internal {

// This class is an implementation detail for is_convertible, and you
// don't need to know how it works to use is_convertible. For those
// who care: we declare two different functions, one whose argument is
// of type To and one with a variadic argument list. We give them
// return types of different size, so we can use sizeof to trick the
// compiler into telling us which function it would have chosen if we
// had called it with an argument of type From.  See Alexandrescu's
// _Modern C++ Design_ for more details on this sort of trick.

template <typename From, typename To>
struct ConvertHelper {
  static small_ Test(To);
  static big_ Test(...);
  static From Create();
};
}  // namespace internal

// Inherits from true_type if From is convertible to To, false_type otherwise.
template <typename From, typename To>
struct is_convertible
    : integral_constant<bool,
                        sizeof(internal::ConvertHelper<From, To>::Test(
                                  internal::ConvertHelper<From, To>::Create()))
                        == sizeof(small_)> {
};
#endif

_END_GOOGLE_NAMESPACE_

#endif  // BASE_TYPE_TRAITS_H_
