#ifndef sno_UTILITY_H
#define sno_UTILITY_H



#ifdef _MSC_VER
#define sno_MSVC
#endif

#ifdef __EMSCRIPTEN__
#define sno_EMSCRIPTEN
#endif



#ifdef _DEBUG
#define sno_DEBUG
#endif

#ifdef _WIN32
#define sno_WINDOWS
#ifdef _WIN64
#define sno_64_BIT
#else
#define sno_32_BIT
#endif
#endif

#if __GNUC__
#if __x86_64__ || __ppc64__
#define sno_64_BIT
#else
#define sno_32_BIT
#endif
#endif

#if !(defined(sno_64_BIT) ^ defined(sno_32_BIT))
#error "Could not determine if compilation target is 32-bit or 64-bit"
#endif

#ifdef sno_64_BIT
#define sno_SIZE_T_LIMIT (0x4000000000000000)
#else
#define sno_SIZE_T_LIMIT (0x40000000)
#endif

// String buffers on the stack
#define sno_STACK_BUFFER_LENGTH (4096)



// For me
#ifdef sno_MSVC
#define sno_USE_DEBUG_BREAK
#define sno_USE_ANSI_COLOR
#else

#endif

#ifdef sno_DEBUG
#ifdef sno_USE_DEBUG_BREAK
#define sno_DEBUG_BREAK __debugbreak()
#else
#define sno_DEBUG_BREAK exit(EXIT_FAILURE)
#endif
#else
#define sno_DEBUG_BREAK
#endif

#ifdef sno_USE_ANSI_COLOR
#define sno_ANSI_NORMAL "\x1B[0m"
#define sno_ANSI_RED "\x1B[0;91m"
#define sno_ANSI_BLUE "\x1B[0;94m"
#define sno_ANSI_GREEN "\x1B[0;92m"
#define sno_ANSI_YELLOW "\x1B[0;93m"
#define sno_ANSI_PURPLE "\x1B[0;95m"
#define sno_ANSI_CYAN "\x1B[0;96m"
#else
#define sno_ANSI_NORMAL ""
#define sno_ANSI_RED ""
#define sno_ANSI_BLUE ""
#define sno_ANSI_GREEN ""
#define sno_ANSI_YELLOW ""
#define sno_ANSI_PURPLE ""
#define sno_ANSI_CYAN ""
#endif



#include <stdlib.h>
#include <stdio.h>

// Assertions
#ifdef sno_MSVC

#define sno_stringify(x) sno_stringify2(x)
#define sno_stringify2(x) #x

#define sno_location_macro " | " __FILE__ " | " __FUNCTION__ "() | Line " sno_stringify(__LINE__)

#define sno_panic(msg) \
	fputs(sno_ANSI_RED "Sno fatal error!" sno_location_macro "\n" \
	msg sno_ANSI_NORMAL "\n", stderr), exit(EXIT_FAILURE)

#ifdef sno_DEBUG
#define sno_assert(expr) if (!(expr)) \
	fputs(sno_ANSI_RED "Assertion failed!" sno_location_macro "\n" \
	"Expression: " #expr sno_ANSI_NORMAL "\n", stderr), sno_DEBUG_BREAK
#define sno_assert_ptr(ptr) if (!(ptr)) \
	fputs(sno_ANSI_RED "Assertion failed!" sno_location_macro "\n" \
	"Pointer \"" #ptr "\" was null" sno_ANSI_NORMAL "\n", stderr), sno_DEBUG_BREAK
#define sno_assert_msg(expr, msg) if (!(expr)) \
	fputs(sno_ANSI_RED "Assertion failed!" sno_location_macro "\n" \
	"Expression: " #expr " | " msg sno_ANSI_NORMAL "\n", stderr), sno_DEBUG_BREAK
#define sno_unreachable \
	fputs(sno_ANSI_RED "Unreachable code!" sno_location_macro sno_ANSI_NORMAL "\n", stderr), sno_DEBUG_BREAK
#define sno_not_implemented \
	fputs(sno_ANSI_RED "Not implemented!" sno_location_macro sno_ANSI_NORMAL "\n", stderr), sno_DEBUG_BREAK
#else
#define sno_assert(expr) __assume(expr)
#define sno_assert_ptr(ptr) __assume(ptr)
#define sno_assert_msg(expr, msg) __assume(expr)
#define sno_unreachable __assume(0)
#define sno_not_implemented __assume(0)
#endif

#else

#define sno_panic(msg) \
	fputs(sno_ANSI_RED "Sno fatal error!\n" \
	msg sno_ANSI_NORMAL "\n", stderr), exit(EXIT_FAILURE)

#ifdef sno_DEBUG
#define sno_assert(expr) if (!(expr)) \
	fputs(sno_ANSI_RED "Assertion failed!\n" \
	"Expression: " #expr sno_ANSI_NORMAL "\n", stderr), sno_DEBUG_BREAK
#define sno_assert_ptr(ptr) if (!(ptr)) \
	fputs(sno_ANSI_RED "Assertion failed!\n" \
	"Pointer \"" #ptr "\" was null" sno_ANSI_NORMAL "\n", stderr), sno_DEBUG_BREAK
#define sno_assert_msg(expr, msg) if (!(expr)) \
	fputs(sno_ANSI_RED "Assertion failed!\n" \
	"Expression: " #expr " | " msg sno_ANSI_NORMAL "\n", stderr), sno_DEBUG_BREAK
#define sno_unreachable \
	fputs(sno_ANSI_RED "Unreachable code!" sno_ANSI_NORMAL "\n", stderr), sno_DEBUG_BREAK
#define sno_not_implemented \
	fputs(sno_ANSI_RED "Not implemented!" sno_ANSI_NORMAL "\n", stderr), sno_DEBUG_BREAK
#else
#define sno_assert(expr)
#define sno_assert_ptr(ptr)
#define sno_assert_msg(expr, msg)
#define sno_unreachable
#define sno_not_implemented
#endif

#endif



#ifdef sno_MSVC
#define sno_likely(expr) (expr)
#define sno_unlikely(expr) (expr)
#define sno_inline __inline
#define sno_no_inline __declspec(noinline)
#define sno_restrict __restrict
#define sno_no_return __declspec(noreturn)
#else
#define sno_likely(expr) (expr)
#define sno_unlikely(expr) (expr)
#define sno_inline
#define sno_no_inline
#define sno_restrict
#define sno_no_return
#endif



#ifdef sno_BUILD_DLL
#define sno_API __declspec(dllexport)
#else
#define sno_API extern
#endif

#ifdef sno_EMSCRIPTEN
#include <emscripten/emscripten.h>
#define EMSCRIPTEN_EXPORT extern EMSCRIPTEN_KEEPALIVE
#else
#define EMSCRIPTEN_EXPORT
#endif



// Types
#include <stdint.h>

#ifndef NULL
#define NULL ((void*)0)
#endif

#ifdef sno_MSVC
#include <stdbool.h>
typedef _Bool sno_Bool;
#define sno_FALSE false
#define sno_TRUE true
#else
#include <stdbool.h>
#define sno_Bool bool
#define sno_FALSE false
#define sno_TRUE true
#endif

#include <math.h>

#ifdef sno_USE_32BIT_NUMBERS
typedef float sno_Number;
typedef int32_t sno_Int;
#define sno_floor floorf
#define sno_ceil ceilf
#define sno_round roundf
#define sno_i_div(l, r) ((sno_Number)(((sno_Int)(l)) / ((sno_Int)(r))))
#define sno_mod(l, r) fmodf
#define sno_pow(l, r) powf
#else
typedef double sno_Number;
typedef int64_t sno_Int;
#define sno_floor floor
#define sno_ceil ceil
#define sno_round round
#define sno_idiv(l, r) ((sno_Number)(((sno_Int)(l)) / ((sno_Int)(r))))
#define sno_mod fmod
#define sno_pow pow
#endif

#define sno_number_is_valid_u8(num) ((sno_floor(num) == (num)) && ((num) >= 0) && ((num) <= UINT8_MAX))
#define sno_number_is_valid_i8(num) ((sno_floor(num) == (num)) && ((num) >= INT8_MIN) && ((num) <= INT8_MAX))
#define sno_number_is_valid_u16(num) ((sno_floor(num) == (num)) && ((num) >= 0) && ((num) <= UINT16_MAX))
#define sno_number_is_valid_i16(num) ((sno_floor(num) == (num)) && ((num) >= INT16_MIN) && ((num) <= INT16_MAX))
#define sno_number_is_valid_u32(num) ((sno_floor(num) == (num)) && ((num) >= 0) && ((num) <= UINT32_MAX))
#define sno_number_is_valid_i32(num) ((sno_floor(num) == (num)) && ((num) >= INT32_MIN) && ((num) <= INT32_MAX))
#define sno_number_is_valid_u64(num) ((sno_floor(num) == (num)) && ((num) >= 0) && ((num) <= UINT64_MAX))
#define sno_number_is_valid_i64(num) ((sno_floor(num) == (num)) && ((num) >= INT64_MIN) && ((num) <= INT64_MAX))

#define sno_is_power_of_2(num) (((num) & ((num) - 1)) == 0)

typedef uint8_t sno_GCMark;
typedef uint32_t sno_Hash;
typedef uint16_t sno_LocalID;
typedef uint8_t sno_LocalSlot;
typedef uint16_t sno_ConstID;

typedef uint16_t sno_Instruction;

#define sno_str_comma_len(str) (str), (sizeof(str) - 1)

struct sno_State;
struct sno_IString;

#endif
