/*
defaults.h - set up default configuration
Copyright (C) 2016 Mittorn

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#ifndef DEFAULTS_H
#define DEFAULTS_H

#include "backends.h"
#include "build.h"

/*
===================================================================

SETUP BACKENDS DEFINITIONS

===================================================================
*/
#if !XASH_DEDICATED
	#if XASH_SDL
		// we are building using libSDL
		#ifndef XASH_VIDEO
			#define XASH_VIDEO VIDEO_SDL
		#endif

		#ifndef XASH_INPUT
			#define XASH_INPUT INPUT_SDL
		#endif

		#ifndef XASH_SOUND
			#define XASH_SOUND SOUND_SDL
		#endif

		#if XASH_SDL == 2
			#ifndef XASH_TIMER
				#define XASH_TIMER TIMER_SDL
			#endif

			#ifndef XASH_MESSAGEBOX
				// [FWGS, 01.04.23] SDL2 messageboxes not available
				#if !XASH_NSWITCH
					#define XASH_MESSAGEBOX MSGBOX_SDL
				#endif			
			#endif
		#endif

	#elif XASH_ANDROID

		// we are building for Android platform, use Android APIs
		#ifndef XASH_VIDEO
			#define XASH_VIDEO VIDEO_ANDROID
		#endif // XASH_VIDEO

		#ifndef XASH_INPUT
			#define XASH_INPUT INPUT_ANDROID
		#endif // XASH_INPUT

		#ifndef XASH_SOUND
			#define XASH_SOUND SOUND_OPENSLES
		#endif // XASH_SOUND

		#ifndef XASH_MESSAGEBOX
			#define XASH_MESSAGEBOX MSGBOX_ANDROID
		#endif // XASH_MESSAGEBOX

		#define XASH_USE_EVDEV
		#define XASH_DYNAMIC_DLADDR

	#elif XASH_LINUX

		// we are building for Linux without SDL2, can draw only to framebuffer yet
		#ifndef XASH_VIDEO
			#define XASH_VIDEO VIDEO_FBDEV
		#endif // XASH_VIDEO

		#ifndef XASH_INPUT
			#define XASH_INPUT INPUT_EVDEV
		#endif // XASH_INPUT

		#ifndef XASH_SOUND
			#define XASH_SOUND SOUND_ALSA
		#endif // XASH_SOUND

		#define XASH_USE_EVDEV

	#elif XASH_DOS4GW

		#ifndef XASH_VIDEO
			#define XASH_VIDEO VIDEO_DOS
		#endif
		#ifndef XASH_TIMER
			#define XASH_TIMER TIMER_DOS
		#endif

		// usually only 10-20 fds availiable
		#define XASH_REDUCE_FD

	#endif

#endif

//
// select messagebox implementation
//
#ifndef XASH_MESSAGEBOX

	#if XASH_WIN32
		#define XASH_MESSAGEBOX MSGBOX_WIN32
	#elif XASH_NSWITCH	// [Xash3D, 20.03.23]
		#define XASH_MESSAGEBOX MSGBOX_NSWITCH
	#else // !XASH_WIN32
		#define XASH_MESSAGEBOX MSGBOX_STDERR
	#endif // !XASH_WIN32

#endif

/* [FWGS, 01.04.23]
// select crashhandler based on defines
//
#ifndef XASH_CRASHHANDLER
	#if XASH_WIN32 && defined(DBGHELP)
		#define XASH_CRASHHANDLER CRASHHANDLER_DBGHELP
	#elif XASH_LINUX || XASH_BSD
		#define XASH_CRASHHANDLER CRASHHANDLER_UCONTEXT
	#endif // !(XASH_LINUX || XASH_BSD || XASH_WIN32)
#endif*/

//
// no timer - no xash
//
#ifndef XASH_TIMER
	#if XASH_WIN32
		#define XASH_TIMER TIMER_WIN32
	#else
		#define XASH_TIMER TIMER_POSIX
	#endif
#endif

#ifdef XASH_STATIC_LIBS
#define XASH_LIB LIB_STATIC
#define XASH_INTERNAL_GAMELIBS
#define XASH_ALLOW_SAVERESTORE_OFFSETS
#elif XASH_WIN32
#define XASH_LIB LIB_WIN32
#elif XASH_POSIX
#define XASH_LIB LIB_POSIX
#endif

//
// fallback to NULL
//
#ifndef XASH_VIDEO
	#define XASH_VIDEO VIDEO_NULL
#endif

#ifndef XASH_SOUND
	#define XASH_SOUND SOUND_NULL
#endif

#ifndef XASH_INPUT
	#define XASH_INPUT INPUT_NULL
#endif

/* [FWGS, 01.04.23]
#ifndef XASH_CRASHHANDLER
	#define XASH_CRASHHANDLER CRASHHANDLER_NULL
#endif // XASH_CRASHHANDLER 
*/

/*
=========================================================================

Default build-depended cvar and constant values

=========================================================================
*/

/* [FWGS, 01.04.23]
#if XASH_MOBILE_PLATFORM
	#define DEFAULT_TOUCH_ENABLE	"1"
*/

// Platform overrides
#if XASH_NSWITCH
	#define DEFAULT_TOUCH_ENABLE	"0"
	#define DEFAULT_M_IGNORE		"1"

/* [FWGS, 01.04.23]
#else // !XASH_MOBILE_PLATFORM 
*/
	#define DEFAULT_MODE_WIDTH		1280
	#define DEFAULT_MODE_HEIGHT		720
	#define DEFAULT_ALLOWCONSOLE	1
#elif XASH_PSVITA
	#define DEFAULT_TOUCH_ENABLE	"0"

/* [FWGS, 01.04.23]
#define DEFAULT_M_IGNORE		"0"
#endif // !XASH_MOBILE_PLATFORM
*/
	#define DEFAULT_M_IGNORE		"1"
	#define DEFAULT_MODE_WIDTH		960
	#define DEFAULT_MODE_HEIGHT		544
	#define DEFAULT_ALLOWCONSOLE	1
#elif XASH_MOBILE_PLATFORM
	#define DEFAULT_TOUCH_ENABLE	"1"
	#define DEFAULT_M_IGNORE		"1"
#endif

#if XASH_ANDROID || XASH_IOS || XASH_EMSCRIPTEN
#define XASH_INTERNAL_GAMELIBS
// this means that libraries are provided with engine, but not in game data
// You need add library loading code to library.c when adding new platform
#endif

// [FWGS, 01.04.23] Defaults
#ifndef DEFAULT_TOUCH_ENABLE
	#define DEFAULT_TOUCH_ENABLE	"0"
#endif

#ifndef DEFAULT_M_IGNORE
	#define DEFAULT_M_IGNORE		"0"
#endif

#ifndef DEFAULT_DEV
	#define DEFAULT_DEV 0
#endif

// [FWGS, 01.04.23]
#ifndef DEFAULT_ALLOWCONSOLE
	#define DEFAULT_ALLOWCONSOLE	0
#endif

#ifndef DEFAULT_FULLSCREEN
	#define DEFAULT_FULLSCREEN		1
#endif

#endif
