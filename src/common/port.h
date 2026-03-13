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

#include "..\library_suffix\build.h"

// [FWGS, 01.11.25]
#if XASH_POSIX
	#include <unistd.h>

	#define OS_LIB_PREFIX "lib"
#endif

// [FWGS, 01.03.26]
#if XASH_APPLE
	#include <sys/syslimits.h>
	#define OS_LIB_EXT "dylib"
	#define OPEN_COMMAND "open"
#elif XASH_POSIX
	#define OS_LIB_EXT "so"
	#define OPEN_COMMAND "xdg-open"
#elif XASH_WIN32
	#define HSPRITE WINAPI_HSPRITE
	#define WIN32_LEAN_AND_MEAN
	/*define NOMINMAX*/
	#define VC_EXTRALEAN

	#ifndef NOMINMAX
		#define NOMINMAX
	#endif

	#include <windows.h>
	
	#undef HSPRITE
	#define open _open
	#define read _read

	/*define alloca _alloca*/
	#ifndef alloca
		#define alloca _alloca
	#endif

	#define OS_LIB_PREFIX	""
	#define OS_LIB_EXT		"dll"
	#define VGUI_S_LIB		"vguis." OS_LIB_EXT	// [ESHQ: переопределение]
	#define OPEN_COMMAND	"open"
#elif XASH_PSP
	#define OS_LIB_EXT		"prx"
#endif

// [FWGS, 01.03.26]
/*if !defined( _MSC_VER )
	#define __cdecl
	#define __stdcall
endif*/

#if !XASH_WIN32
	typedef void *HINSTANCE;
	typedef struct tagPOINT { int x, y; } POINT; // one nasty function in cdll_int.h needs it
#endif

#ifndef XASH_LOW_MEMORY
	#define XASH_LOW_MEMORY 0
#endif

#endif
