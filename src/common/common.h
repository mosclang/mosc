//
// Created by Mahamadou DOUMBIA [OML DSI] on 12/01/2022.
//

#ifndef CPMSC_COMMON_H
#define CPMSC_COMMON_H

#define __STDC_LIMIT_MACROS

#include <stdint.h>


#ifndef MSC_NAN_TAGGING
#define MSC_NAN_TAGGING 1
#endif


#ifndef MSC_COMPUTED_GOTO
#if defined(_MSC_VER) && !defined(__clang__)
// No computed gotos in Visual Studio.
#define MSC_COMPUTED_GOTO 0
#else
#define MSC_COMPUTED_GOTO 1
#endif
#endif

// The VM includes a number of optional modules. You can choose to include
// these or not. By default, they are all available. To disable one, set the
// corresponding `MSC_OPT_<name>` define to `0`.
#ifndef MSC_OPT_FAN
#define MSC_OPT_FAN 1
#endif

#ifndef MSC_OPT_KUNFE
#define MSC_OPT_KUNFE 1
#endif


// #define MSC_DEBUG_TRACE_GC 0

// #define MSC_DEBUG_TRACE_MEMORY 0

// #define MSC_DEBUG_DUMP_COMPILED_CODE 0
// #define MSC_DEBUG_TRACE_INSTRUCTIONS 0

// Use the VM's allocator to allocate an object of [type].
#define ALLOCATE(vm, type)                                                     \
    ((type*)MSCReallocate((vm)->gc, NULL, 0, sizeof(type)))


// Use the VM's allocator to allocate an object of [mainType] containing a
// flexible array of [count] objects of [arrayType].
#define ALLOCATE_FLEX(vm, mainType, arrayType, count)                          \
    ((mainType*)MSCReallocate((vm)->gc, NULL, 0,                                    \
        sizeof(mainType) + sizeof(arrayType) * (count)))

// Use the VM's allocator to allocate an array of [count] elements of [type].
#define ALLOCATE_ARRAY(vm, type, count)                                        \
    ((type*)MSCReallocate((vm)->gc, NULL, 0, sizeof(type) * (count)))

#define DEALLOCATE(vm, pointer) MSCReallocate((vm)->gc, pointer, 0, 0)

// The Microsoft compiler does not support the "inline" modifier when compiling
// as plain C.
#if defined( _MSC_VER ) && !defined(__cplusplus)
#define inline _inline
#endif

// This is used to clearly mark flexible-sized arrays that appear at the end of
// some dynamically-allocated structs, known as the "struct hack".
#if __STDC_VERSION__ >= 199901L
// In C99, a flexible array member is just "[]".
#define FLEXIBLE_ARRAY
#else
// Elsewhere, use a zero-sized array. It's technically undefined behavior,
// but works reliably in most known compilers.
#define FLEXIBLE_ARRAY 0
#endif


#ifdef DEBUG



#define ASSERT(condition, message)                                           \
      do                                                                       \
      {                                                                        \
        if (!(condition))                                                      \
        {                                                                      \
          fprintf(stderr, "[%s:%d] Assert failed in %s(): %s\n",               \
              __FILE__, __LINE__, __func__, message);                          \
          abort();                                                             \
        }                                                                      \
      } while (false)

// Indicates that we know execution should never reach this point in the
// program. In debug mode, we assert this fact because it's a bug to get here.
//
// In release mode, we use compiler-specific built in functions to tell the
// compiler the code can't be reached. This avoids "missing return" warnings
// in some cases and also lets it perform some optimizations by assuming the
// code is never reached.
#define UNREACHABLE()                                                        \
      do                                                                       \
      {                                                                        \
        fprintf(stderr, "[%s:%d] This code should not be reached in %s()\n",   \
            __FILE__, __LINE__, __func__);                                     \
        abort();                                                               \
      } while (false)

#else

#define ASSERT(condition, message) do { } while (false)

// Tell the compiler that this part of the code will never be reached.
#if defined( _MSC_VER )
#define UNREACHABLE() __assume(0)
#elif (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5))
#define UNREACHABLE() __builtin_unreachable()
#else
#define UNREACHABLE()
#endif

#endif


// A union to let us reinterpret a double as raw bits and back.
typedef union {
    uint64_t bits64;
    uint32_t bits32[2];
    double num;
} DoubleBits;

#define MSC_DOUBLE_QNAN_POS_MIN_BITS (UINT64_C(0x7FF8000000000000))
#define MSC_DOUBLE_QNAN_POS_MAX_BITS (UINT64_C(0x7FFFFFFFFFFFFFFF))

#define MSC_DOUBLE_NAN (MSCDoubleFromBits(MSC_DOUBLE_QNAN_POS_MIN_BITS))

static inline double MSCDoubleFromBits(uint64_t bits) {
    DoubleBits data;
    data.bits64 = bits;
    return data.num;
}

static inline uint64_t MSCDoubleToBits(double num) {
    DoubleBits data;
    data.num = num;
    return data.bits64;
}


#endif //CPMSC_COMMON_H
