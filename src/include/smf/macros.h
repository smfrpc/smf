// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once
#include <cstddef>

template <typename T, std::size_t N>
char (&smf_array_size_helper(T (&array)[N]))[N];
#define SMF_ARRAYSIZE(array) (sizeof(::smf_array_size_helper(array)))

#define SMF_DISALLOW_COPY(TypeName) TypeName(const TypeName &) = delete

#define SMF_DISALLOW_ASSIGN(TypeName) void operator=(const TypeName &) = delete

// A macro to disallow the copy constructor and operator= functions
// This is usually placed in the private: declarations for a class.
#define SMF_DISALLOW_COPY_AND_ASSIGN(TypeName)                                 \
  TypeName(const TypeName &) = delete;                                         \
  void operator=(const TypeName &) = delete

#define SMF_DISALLOW_IMPLICIT_CONSTRUCTORS(TypeName)                           \
  TypeName() = delete;                                                         \
  SMF_DISALLOW_COPY_AND_ASSIGN(TypeName)

// GCC can be told that a certain branch is not likely to be taken (for
// instance, a CHECK failure), and use that information in static analysis.
// Giving it this information can help it optimize for the common case in
// the absence of better information (ie. -fprofile-arcs).
#if defined(__GNUC__)
#define SMF_LIKELY(x) (__builtin_expect((x), 1))
#define SMF_UNLIKELY(x) (__builtin_expect((x), 0))
#else
#define SMF_LIKELY(x) (x)
#define SMF_UNLIKELY(x) (x)
#endif

// we only compile on linux. both clang & gcc support this
#if defined(__clang__) || defined(__GNUC__)
#define SMF_ALWAYS_INLINE inline __attribute__((__always_inline__))
#else
#define SMF_ALWAYS_INLINE inline
#endif

#if defined(__clang__) || defined(__GNUC__)
#define SMF_NOINLINE __attribute__((__noinline__))
#else
#define SMF_NOINLINE
#endif
