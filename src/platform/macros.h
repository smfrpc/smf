// Copyright (c) 2016 Alexander Gallego. All rights reserved.
//
#pragma once

// Don't put inside a namespace
// Prefix the namespace using SNAKE_UPPER_CASE
// #define is a preprocessor directive. The macros are being replaced before
// anything else apart from removing comments (which means, before compilation).
// So at the time macros are replaced, the compiler knows nothing about your
// namespaces.


#define SMF_DISALLOW_COPY(TypeName) TypeName(const TypeName &) = delete

#define SMF_DISALLOW_ASSIGN(TypeName) void operator=(const TypeName &) = delete

// A macro to disallow the copy constructor and operator= functions
// This is usually placed in the private: declarations for a class.
#define SMF_DISALLOW_COPY_AND_ASSIGN(TypeName) \
  TypeName(const TypeName &) = delete;         \
  void operator=(const TypeName &) = delete

#define SMF_DISALLOW_IMPLICIT_CONSTRUCTORS(TypeName) \
  TypeName() = delete;                               \
  SMF_DISALLOW_COPY_AND_ASSIGN(TypeName)

// GCC can be told that a certain branch is not likely to be taken (for
// instance, a CHECK failure), and use that information in static analysis.
// Giving it this information can help it optimize for the common case in
// the absence of better information (ie. -fprofile-arcs).
#if defined(COMPILER_GCC3)
#define SMF_UNLIKELY(x) (__builtin_expect(x, 0))
#define SMF_LIKELY(x) (__builtin_expect(!!(x), 1))
#else
#define SMF_UNLIKELY(x) (x)
#define SMF_LIKELY(x) (x)
#endif


#ifndef SMF_GCC_CONCEPTS
#define SMF_CONCEPT(x...)
#else
#define SMF_CONCEPT(x...) x
#endif
