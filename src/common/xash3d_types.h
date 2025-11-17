// basic typedefs
#ifndef XASH_TYPES_H
#define XASH_TYPES_H

// [FWGS, 01.11.25]
/*include "build.h"

if XASH_IRIX
	include <port.h>
endif

if XASH_WIN32*/
#include <wchar.h>
/*endif*/

#include <sys/types.h>

// [FWGS, 01.11.25]
/*ifdef STDINT_H
	include STDINT_H
else*/
#include <stdint.h>
/*endif*/

#include <assert.h>

// [FWGS, 01.11.25]
#include "build.h"
#include "port.h"

#define MAX_STRING		256		// generic string
#define MAX_VA_STRING	1024	// compatibility macro
#define MAX_SYSPATH		1024	// system filepath
#define MAX_OSPATH		260		// max length of a filesystem pathname
#define CS_SIZE			64		// size of one config string
#define CS_TIME			16		// size of time string

// platform-specific alignment for types, to not break ABI
#if XASH_PSP
	#define MAYBE_ALIGNED( x ) __attribute__(( aligned( 16 )))
#endif

#if !defined( MAYBE_ALIGNED )
	#define MAYBE_ALIGNED( x )
#endif

// [FWGS, 01.02.25]
typedef uint8_t	byte;
typedef float	vec_t;
typedef vec_t	vec2_t[2];

// ESHQ: защита от переопределения
#if XASH_DL || XASH_FS

// [FWGS, 01.03.25]
#ifndef vec3_t
typedef vec_t	vec3_t[3];
#endif
#endif

// [FWGS, 01.11.25]
/*typedef vec_t	vec4_t[4];
typedef vec_t	quat_t[4];
typedef byte	rgba_t[4];	// unsigned byte colorpack
typedef byte	rgb_t[3];	// unsigned byte colorpack
typedef vec_t	matrix3x4[3][4];
typedef vec_t	matrix4x4[4][4];*/
typedef vec_t	vec4_t[4] MAYBE_ALIGNED (16);
typedef vec_t	quat_t[4] MAYBE_ALIGNED (16);
typedef byte	rgba_t[4];		// unsigned byte colorpack
typedef byte	rgb_t[3];		// unsigned byte colorpack
typedef vec_t	matrix3x4[3][4] MAYBE_ALIGNED (16);
typedef vec_t	matrix4x4[4][4] MAYBE_ALIGNED (16);

typedef uint32_t	poolhandle_t;

// [FWGS, 01.11.25]
typedef uint32_t dword;
typedef char string[MAX_STRING];
typedef off_t fs_offset_t;

#if XASH_WIN32
	typedef int		fs_size_t; // return type of _read, _write funcs
#else
	typedef ssize_t	fs_size_t;
#endif

typedef unsigned int uint;
typedef void *(*pfnCreateInterface_t)(const char *, int *);

#undef true
#undef false

// [FWGS, 01.03.25] true and false are keywords in C++ and C23
#if !__cplusplus && __STDC_VERSION__ < 202311L
	enum { false, true };
#endif

// [FWGS, 01.03.25]
typedef int qboolean;

// [FWGS, 01.11.25]
/*define MAX_STRING		256		// generic string
define MAX_VA_STRING	1024	// compatibility macro
define MAX_SYSPATH		1024	// system filepath
define MAX_MODS		512		// environment games that engine can keep visible

define BIT( n )		( 1U << ( n ))
define BIT64( n )		( 1ULL << ( n ))*/
#if XASH_LOW_MEMORY == 1 || XASH_PSP
	#define MAX_QPATH	48
	#define MAX_MODS	16
#elif XASH_LOW_MEMORY == 2
	#define MAX_QPATH	32	// should be enough for singleplayer
	#define MAX_MODS	4
#else
	#define MAX_QPATH	64
	#define MAX_MODS	512
#endif

// [FWGS, 01.11.25]
/*define SetBits( iBitVector, bits )	((iBitVector) = (iBitVector) | (bits))
define ClearBits( iBitVector, bits )	((iBitVector) = (iBitVector) & ~(bits))
define FBitSet( iBitVector, bit )	((iBitVector) & (bit))*/
#define BIT( n )	( 1U << ( n ))
#define BIT64( n )	( 1ULL << ( n ))

/*ifndef __cplusplus
ifdef NULL
undef NULL
endif

define NULL	((void *)0)
endif*/
#define SetBits( bit_vector, bits ) (( bit_vector ) |= ( bits ))
#define ClearBits( bit_vector, bits ) (( bit_vector ) &= ~( bits ))
#define FBitSet( bit_vector, bits ) (( bit_vector ) & ( bits ))

// color strings
/*define IsColorString( p )	( p && *( p ) == '^' && *(( p ) + 1) && *(( p ) + 1) >= '0' && *(( p ) + 1 ) <= '9' )
define ColorIndex( c )	((( c ) - '0' ) & 7 )

// [FWGS, 01.03.25]
undef EXPORT*/
#define IsColorString( p ) (( p ) && *( p ) == '^' && *(( p ) + 1) && *(( p ) + 1) >= '0' && *(( p ) + 1 ) <= '9' )
#define ColorIndex( c ) ((( c ) - '0' ) & 7 )

// [FWGS, 01.11.25]
#if defined( __GNUC__ )
	#if defined( __i386__ )
		#define HLEXPORT __attribute__(( visibility( "default" ), force_align_arg_pointer ))
		#define GAME_EXPORT __attribute__(( force_align_arg_pointer ))
	/*else
		define HLEXPORT __attribute__(( visibility ( "default" )))
		define GAME_EXPORT
	endif

	define MALLOC __attribute__(( malloc ))*/
	#else
		#define EXPORT __attribute__(( visibility ( "default" )))
	#endif

	#if __GNUC__ >= 11
		// might want to set noclone due to https://gcc.gnu.org/bugzilla/show_bug.cgi?id=116893
		// but it's easier to not force mismatched-dealloc to error yet
		#define MALLOC_LIKE( x, y ) __attribute__(( malloc( x, y )))
	#else
		/*define MALLOC_LIKE( x, y ) MALLOC*/
		#define MALLOC_LIKE( x, y ) __attribute__(( malloc ))
	#endif

	/*define NORETURN __attribute__(( noreturn ))
	define NONNULL __attribute__(( nonnull ))*/

	#define RETURNS_NONNULL __attribute__(( returns_nonnull ))
	
	/*if __clang__ || __MCST__*/
	#if !__clang__ && !__MCST__
		// clang has bugged returns_nonnull for functions pointers, it's ignored and generates a warning about objective-c? O_o
		// lcc doesn't support it at all
		/*define PFN_RETURNS_NONNULL
		else*/
		#define PFN_RETURNS_NONNULL RETURNS_NONNULL
	#endif

	#define NORETURN __attribute__(( noreturn ))
	#define NONNULL __attribute__(( nonnull ))

	#define FORMAT_CHECK( x ) __attribute__(( format( printf, x, x + 1 )))
	#define ALLOC_CHECK( x ) __attribute__(( alloc_size( x )))
	/*define NO_ASAN __attribute__(( no_sanitize( "address" )))*/
	#define WARN_UNUSED_RESULT __attribute__(( warn_unused_result ))
	#define RENAME_SYMBOL( x ) asm( x )

	// [FWGS, 01.11.25]
	/*else

	// [FWGS, 01.12.24]
	if defined( _MSC_VER )
		define HLEXPORT __declspec( dllexport )
		define NO_ASAN // ESHQ: отключён; по умолчанию равен: __declspec( no_sanitize_address )
	else
		define HLEXPORT
		define NO_ASAN
	endif

	define GAME_EXPORT
	define NORETURN
	define NONNULL

	define RETURNS_NONNULL
	define PFN_RETURNS_NONNULL
	define FORMAT_CHECK( x )

	define ALLOC_CHECK( x )
	define RENAME_SYMBOL( x )

	define MALLOC
	define MALLOC_LIKE( x, y )
	define WARN_UNUSED_RESULT*/
#elif defined( _MSC_VER )
	#define HLEXPORT __declspec( dllexport )
#endif

// [FWGS, 01.11.25]
/*if defined( __has_feature )
	if __has_feature( address_sanitizer )
		define USE_ASAN 1
	endif
endif*/
#if defined( __SANITIZE_ADDRESS__ )
	#define USE_ASAN 1

	/*// [FWGS, 01.12.24]
	if !defined( USE_ASAN ) && defined( __SANITIZE_ADDRESS__ )
	define USE_ASAN 1*/
	#if defined( __GNUC__ )
		#define NO_ASAN __attribute__(( no_sanitize( "address" )))
	#elif defined( _MSC_VER )
		#define NO_ASAN __declspec( no_sanitize_address )
	#endif
#endif

// [FWGS, 01.07.24]
#if __GNUC__ >= 3
	#define unlikely( x ) __builtin_expect( x, 0 )
	#define likely( x ) __builtin_expect( x, 1 )
#elif defined( __has_builtin )
	#if __has_builtin( __builtin_expect ) // this must be after defined() check
		#define unlikely( x ) __builtin_expect( x, 0 )
		#define likely( x ) __builtin_expect( x, 1 )
	#endif
#endif

// [FWGS, 01.11.25]
// [ESHQ: где-то есть проблема с переключением, принудительно выставлен __restrict]
#if !defined( __cplusplus ) && __STDC_VERSION__ >= 199101L && false // not C++ and C99 or newer
	#define XASH_RESTRICT restrict
#elif _MSC_VER || __GNUC__ || __clang__ // compiler-specific extensions
	#define XASH_RESTRICT __restrict
#endif

#if !defined( EXPORT )
	#define EXPORT
#endif

#if !defined( GAME_EXPORT )
	#define GAME_EXPORT
#endif

#if !defined( MALLOC_LIKE )
	#define MALLOC_LIKE( x, y )
#endif

#if !defined( NO_ASAN )
	#define NO_ASAN
#endif

#if !defined( RETURNS_NONNULL )
	#define RETURNS_NONNULL
#endif

#if !defined( PFN_RETURNS_NONNULL )
	#define PFN_RETURNS_NONNULL
#endif

#if !defined( NORETURN )
	#define NORETURN
#endif

#if !defined( NONNULL )
	#define NONNULL
#endif

#if !defined( FORMAT_CHECK )
	#define FORMAT_CHECK( x )
#endif

#if !defined( ALLOC_CHECK )
	#define ALLOC_CHECK( x )
#endif

#if !defined( WARN_UNUSED_RESULT )
	#define WARN_UNUSED_RESULT
#endif

#if !defined( RENAME_SYMBOL )
	#define RENAME_SYMBOL( x )
#endif

// [FWGS, 01.11.25]
#if !defined( unlikely ) || !defined( likely )
	#define unlikely( x ) ( x )
	#define likely( x ) ( x )
#endif

#if !defined( XASH_RESTRICT )
	#define XASH_RESTRICT
#endif

// [FWGS, 01.01.24]
#if __STDC_VERSION__ >= 202311L || __cplusplus >= 201103L // C23 or C++ static_assert is a keyword
	#define STATIC_ASSERT_( ignore, x, y ) static_assert( x, y )
	#define STATIC_ASSERT static_assert
#elif __STDC_VERSION__ >= 201112L // in C11 it's _Static_assert
	#define STATIC_ASSERT_( ignore, x, y ) _Static_assert( x, y )
	#define STATIC_ASSERT _Static_assert
#else
	#define STATIC_ASSERT_( id, x, y ) extern int id[( x ) ? 1 : -1]

	// need these to correctly expand the line macro
	#define STATIC_ASSERT_3( line, x, y ) STATIC_ASSERT_( static_assert_ ## line, x, y )
	#define STATIC_ASSERT_2( line, x, y ) STATIC_ASSERT_3( line, x, y )
	#define STATIC_ASSERT( x, y ) STATIC_ASSERT_2( __LINE__, x, y )
#endif

// [FWGS, 01.11.25] at least, statically check size of some public structures
#if XASH_64BIT
	/*define STATIC_CHECK_SIZEOF( type, size32, size64 ) \
		STATIC_ASSERT( sizeof( type ) == size64, #type " unexpected size" )*/
	#define STATIC_CHECK_SIZEOF( type, size32, size64 ) \
		STATIC_ASSERT( sizeof( type ) == size64, #type " unexpected size" )
#else
	/*define STATIC_CHECK_SIZEOF( type, size32, size64 ) \
		STATIC_ASSERT (sizeof (type) == size32, #type " unexpected size" )*/
	#define STATIC_CHECK_SIZEOF( type, size32, size64 ) \
		STATIC_ASSERT( sizeof( type ) == size32, #type " unexpected size" )
#endif

/*// [FWGS, 01.07.24] [ESHQ: где-то есть проблема с переключением, принудительно выставлен __restrict]
if !defined( __cplusplus ) && __STDC_VERSION__ >= 199101L && false	// not C++ and C99 or newer
	define XASH_RESTRICT restrict
elif _MSC_VER || __GNUC__ || __clang__ // compiler-specific extensions
	define XASH_RESTRICT __restrict
else
	define XASH_RESTRICT // nothing
endif*/

// [FWGS, 01.11.25]
#define Swap32( x ) (((uint32_t)((( x ) & 255 ) << 24 )) + ((uint32_t)(((( x ) >> 8 ) & 255 ) << 16 )) + ((uint32_t)((( x ) >> 16 ) & 255 ) << 8 ) + ((( x ) >> 24 ) & 255 ))
#define Swap16( x ) ((uint16_t)((((uint16_t)( x ) >> 8 ) & 255 ) + (((uint16_t)( x ) & 255 ) << 8 )))
#define Swap32Store( x ) ( x = Swap32( x ))
#define Swap16Store( x ) ( x = Swap16( x ))

// [FWGS, 01.11.25]
#ifdef XASH_BIG_ENDIAN
	/*define LittleLong(x) (((int)(((x)&255)<<24)) + ((int)((((x)>>8)&255)<<16)) + ((int)(((x)>>16)&255)<<8) + (((x) >> 24)&255))
	define LittleLongSW(x) (x = LittleLong(x) )
	define LittleShort(x) ((short)( (((short)(x) >> 8) & 255) + (((short)(x) & 255) << 8)))
	define LittleShortSW(x) (x = LittleShort(x) )
	_inline float LittleFloat (float f)
	{
	union
		{
		float f;
		unsigned char b[4];
		} dat1, dat2;

	dat1.f = f;
	dat2.b[0] = dat1.b[3];
	dat2.b[1] = dat1.b[2];
	dat2.b[2] = dat1.b[1];
	dat2.b[3] = dat1.b[0];

	return dat2.f;
	}*/
	#define LittleLong( x ) Swap32( x )
	#define LittleShort( x ) Swap16( x )
	#define LittleLongSW( x ) Swap32Store( x )
	#define LittleShortSW( x ) Swap16Store( x )
	#define LittleFloat( x ) SwapFloat( x )
	#define BigLong( x ) ( x )
	#define BigShort( x ) ( x )
	#define BigFloat( x ) ( x )
#else
	/*define LittleLong(x) (x)
	define LittleLongSW(x)
	define LittleShort(x) (x)
	define LittleShortSW(x)
	define LittleFloat(x) (x)
	endif

	// [FWGS, 01.02.25]
	typedef unsigned int dword;
	typedef unsigned int uint;
	typedef char string[MAX_STRING];
	typedef off_t fs_offset_t;

	// [FWGS, 01.02.25]
	if XASH_WIN32
	typedef int fs_size_t;	// return type of _read, _write funcs
	else
	typedef ssize_t fs_size_t;
	endif

	// [FWGS, 01.11.23]
	typedef void *(*pfnCreateInterface_t)(const char *, int *);

	// config strings are a general means of communication from
	// the server to all connected clients.
	// each config string can be at most CS_SIZE characters
	if XASH_LOW_MEMORY == 0
	define MAX_QPATH	64	// max length of a game pathname
	elif XASH_LOW_MEMORY == 2
	define MAX_QPATH	32	// should be enough for singleplayer
	elif XASH_LOW_MEMORY == 1
	define MAX_QPATH	48*/
	#define LittleLong( x ) ( x )
	#define LittleShort( x ) ( x )
	#define LittleFloat( x ) ( x )
	#define LittleLongSW( x )
	#define LittleShortSW( x )
	#define BigLong( x ) Swap32( x )
	#define BigShort( x ) Swap16( x )
	#define BigFloat( x ) SwapFloat( x )
#endif

// [FWGS, 01.11.25]
/*define MAX_OSPATH		260	// max length of a filesystem pathname
define CS_SIZE			64	// size of one config string
define CS_TIME			16	// size of time string*/

#endif
