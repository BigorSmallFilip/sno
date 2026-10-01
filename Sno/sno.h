#ifndef sno_H
#define sno_H



#ifdef _DEBUG
#define sno_DEBUG
#endif



#ifdef _MSC_VER
#define sno_MSVC
#endif

#ifdef __EMSCRIPTEN__
#define sno_EMSCRIPTEN
#endif

#ifdef __GNUC__
#define sno_GNUC
#endif



// During development
#ifdef sno_MSVC

#define sno_USE_DEBUG_BREAK
#define sno_USE_ANSI_COLOR
//#define sno_USE_32BIT_FLOAT_NUMBERS

#ifdef sno_DEBUG
#define sno_USE_ASSERT
#else
#define sno_USE_ASSUME
#endif

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



#ifdef sno_USE_ASSERT

#include <stdio.h>

#define sno_stringify(x) sno_stringify2(x)
#define sno_stringify2(x) #x
#define sno_location_macro " | " __FILE__ " | " __FUNCTION__ "() | Line " sno_stringify(__LINE__)

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
#ifdef sno_USE_ASSUME

#define sno_assert(expr) __assume(expr)
#define sno_assert_ptr(ptr) __assume(ptr)
#define sno_assert_msg(expr, msg) __assume(expr)
#define sno_unreachable __assume(0)
#define sno_not_implemented __assume(0)

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



#ifdef sno_BUILD_LIB
#ifdef sno_EMSCRIPTEN
#include <emscripten/emscripten.h>
#define sno_API extern EMSCRIPTEN_KEEPALIVE
#else
#define sno_API __declspec(dllexport)
#endif
#else
#define sno_API extern
#endif



#define __STDC_LIMIT_MACROS
#include <stdint.h>
#include <stddef.h>

#ifndef NULL
#define NULL ((void*)0)
#endif

#include <stdbool.h>
#ifdef sno_MSVC
typedef _Bool sno_Bool;
#else
#include <stdbool.h>
#define sno_Bool bool
#endif
#define sno_FALSE false
#define sno_TRUE true



#ifndef sno_USE_32BIT_FLOAT_NUMBERS
typedef double sno_Number;
typedef int32_t sno_NumberInt;
#define sno_floor floor
#define sno_ceil ceil
#define sno_round round
#define sno_idiv(l, r) \
	((sno_Number)(((sno_NumberInt)(l)) / ((sno_NumberInt)(r))))
#define sno_mod fmod
#define sno_pow pow
#else
typedef float sno_Number;
typedef int32_t sno_NumberInt;
#define sno_floor floorf
#define sno_ceil ceilf
#define sno_round roundf
#define sno_idiv(l, r) \
	((sno_Number)(((sno_NumberInt)(l)) / ((sno_NumberInt)(r))))
#define sno_mod fmodf
#define sno_pow powf
#endif

#define sno_number_is_valid_u8(n) \
	((sno_floor(n) == (n)) && ((n) >= 0) && ((n) <= UINT8_MAX))
#define sno_number_is_valid_i8(n) \
	((sno_floor(n) == (n)) && ((n) >= INT8_MIN) && ((n) <= INT8_MAX))
#define sno_number_is_valid_u16(n) \
	((sno_floor(n) == (n)) && ((n) >= 0) && ((n) <= UINT16_MAX))
#define sno_number_is_valid_i16(n) \
	((sno_floor(n) == (n)) && ((n) >= INT16_MIN) && ((n) <= INT16_MAX))
#define sno_number_is_valid_u32(n) \
	((sno_floor(n) == (n)) && ((n) >= 0) && ((n) <= UINT32_MAX))
#define sno_number_is_valid_i32(n) \
	((sno_floor(n) == (n)) && ((n) >= INT32_MIN) && ((n) <= INT32_MAX))
#define sno_number_is_valid_u64(n) \
	((sno_floor(n) == (n)) && ((n) >= 0) && ((n) <= UINT64_MAX))
#define sno_number_is_valid_i64(n) \
	((sno_floor(n) == (n)) && ((n) >= INT64_MIN) && ((n) <= INT64_MAX))



#define sno_is_power_of_2(n) (((n) & ((n) - 1)) == 0)

#define sno_min(a, b) ((a) < (b) ? (a) : (b))
#define sno_max(a, b) ((a) > (b) ? (a) : (b))

#define sno_string_comma_length(string) (string), (sizeof(string) - 1)



typedef struct sno_State sno_State;
typedef struct sno_VM sno_VM;

sno_API sno_State* sno_create_state(void);
sno_API void sno_free_state(sno_State* state);



sno_API sno_no_return void sno_throw_runtime_error(
	sno_VM* vm,
	const char* const message,
	size_t message_length
);

#endif
