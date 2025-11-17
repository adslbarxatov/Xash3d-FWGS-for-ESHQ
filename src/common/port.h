/***
port.h -- Portability Layer for Windows types
Copyright (C) 2015 Alibek Omarov

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details
***/

#pragma once
#ifndef PORT_H
#define PORT_H

// [FWGS, 01.11.25]
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "build.h"

// [FWGS, 01.11.25]
/*if !XASH_WIN32
	if XASH_APPLE
		include <sys/syslimits.h>
		define OS_LIB_EXT "dylib"
		define OPEN_COMMAND "open"
	// [FWGS, 01.07.25]
	elif XASH_EMSCRIPTEN
		define OS_LIB_EXT "wasm"
		define OPEN_COMMAND "???"
	else
		define OS_LIB_EXT "so"
		define OPEN_COMMAND "xdg-open"
	endif*/
#if XASH_POSIX
	#include <unistd.h>

	#define OS_LIB_PREFIX "lib"

	/*define VGUI_SUPPORT_DLL "libvgui_support." OS_LIB_EXT

	// Windows-specific
	define __cdecl
	define __stdcall
	define _inline	static inline

	if XASH_POSIX
		include <unistd.h>

		if XASH_NSWITCH
			define SOLDER_LIBDL_COMPAT
			include <solder.h>
		elif XASH_PSVITA
			define VRTLD_LIBDL_COMPAT
			include <vrtld.h>
			define O_BINARY 0
		else
			include <dlfcn.h>

			define HAVE_DUP

			define O_BINARY	0
		endif

		define O_TEXT			0
		define _mkdir( x ) mkdir( x, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH )
	endif

	typedef void* HANDLE;
	typedef void* HINSTANCE;

	typedef struct tagPOINT
	{
		int x, y;
	} POINT;

else

	// [FWGS, 01.02.24]
	define open _open
	define read _read
	define alloca _alloca*/
#endif

	// [FWGS, 01.11.25]
#if XASH_APPLE
	#include <sys/syslimits.h>
	#define OS_LIB_EXT "dylib"
	#define OPEN_COMMAND "open"
#elif XASH_POSIX
	#define OS_LIB_EXT "so"
	#define OPEN_COMMAND "xdg-open"
#elif XASH_WIN32
	#define HSPRITE WINAPI_HSPRITE
	/*define WIN32_LEAN_AND_MEAN
		include <winsock2.h>
		include <windows.h>
		undef HSPRITE*/
	#define WIN32_LEAN_AND_MEAN
	#define NOMINMAX
	#define VC_EXTRALEAN

	#include <windows.h>
	
	#undef HSPRITE
	#define open _open
	#define read _read
	#define alloca _alloca

	#define OS_LIB_PREFIX	""
	/*define OS_LIB_EXT "dll"
	define VGUI_S_DLL	"vguis." OS_LIB_EXT
	define VGUI_SUPPORT_DLL "../" VGUI_S_DLL
	define HAVE_DUP
	endif*/
	#define OS_LIB_EXT	"dll"
	#define VGUI_S_DLL	"vguis." OS_LIB_EXT	// [ESHQ: переопределение]
	#define OPEN_COMMAND	"open"
#elif XASH_PSP
	#define OS_LIB_EXT	"prx"
#endif

// [FWGS, 01.11.25]
/*ifndef XASH_LOW_MEMORY
	define XASH_LOW_MEMORY 0*/
#if !defined( _MSC_VER )
	#define __cdecl
	#define __stdcall
#endif

/*include <stdlib.h>
include <string.h>
include <limits.h>*/
#if !XASH_WIN32
	typedef void *HINSTANCE;
	typedef struct tagPOINT { int x, y; } POINT; // one nasty function in cdll_int.h needs it
#endif

#ifndef XASH_LOW_MEMORY
	#define XASH_LOW_MEMORY 0
#endif

#endif
