/*
build.c - returns a engine build number
Copyright (C) 2010 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "crtlib.h"
#include "buildenums.h"		// [FWGS, 01.04.23]

static const char *date = __DATE__;
static const char *mon[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
static const char mond[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

/*
===============
Q_buildnum

returns days since Apr 1 2015
===============
*/
int Q_buildnum (void)
	{
	int m = 0, d = 0, y = 0;
	static int b = 0;

	if (b != 0) 
		return b;

	for (m = 0; m < 11; m++)
		{
		if (!Q_strnicmp (&date[0], mon[m], 3))
			break;
		d += mond[m];
		}

	d += Q_atoi (&date[4]) - 1;
	y = Q_atoi (&date[7]) - 1900;
	b = d + (int)((y - 1) * 365.25f);

	if (((y % 4) == 0) && m > 1)
		{
		b += 1;
		}
	b -= 41728; // Apr 1 2015

	return b;
	}

/*
=============
Q_buildnum_compat

Returns a Xash3D build number. This is left for compability with original Xash3D.
IMPORTANT: this value must be changed ONLY after updating to newer Xash3D base
IMPORTANT: this value must be acquired through "build" cvar
=============
*/
int Q_buildnum_compat (void)
	{
	// do not touch this! Only author of Xash3D can increase buildnumbers!
	return 4529;
	}

/*
============
Q_GetPlatformStringByID [FWGS, 01.04.23]

Returns name of operating system by ID. Without any spaces.
============
*/
const char *Q_PlatformStringByID (const int platform)
	{
	switch (platform)
		{
		case PLATFORM_WIN32:
			return "win32";
		case PLATFORM_ANDROID:
			return "android";
		case PLATFORM_LINUX_UNKNOWN:
			return "linuxunkabi";
		case PLATFORM_LINUX:
			return "linux";
		case PLATFORM_APPLE:
			return "apple";
		case PLATFORM_FREEBSD:
			return "freebsd";
		case PLATFORM_NETBSD:
			return "netbsd";
		case PLATFORM_OPENBSD:
			return "openbsd";
		case PLATFORM_EMSCRIPTEN:
			return "emscripten";
		case PLATFORM_DOS4GW:
			return "DOS4GW";
		case PLATFORM_HAIKU:
			return "haiku";
		case PLATFORM_SERENITY:
			return "serenity";
		case PLATFORM_IRIX:
			return "irix";
		case PLATFORM_NSWITCH:
			return "nswitch";
		case PLATFORM_PSVITA:
			return "psvita";
		}

	assert (0);
	return "unknown";
	}

/*
============
Q_buildos [FWGS, 01.04.23]

Shortcut for Q_buildos_
============
*/
const char *Q_buildos (void)
	{
	return Q_PlatformStringByID (XASH_PLATFORM);
	/*const char *osname;

#if XASH_MINGW
	osname = "win32-mingw";
#elif XASH_WIN32
	osname = "win32";
#elif XASH_ANDROID
	osname = "android";
#elif XASH_LINUX
	osname = "linux";
#elif XASH_APPLE
	osname = "apple";
#elif XASH_FREEBSD
	osname = "freebsd";
#elif XASH_NETBSD
	osname = "netbsd";
#elif XASH_OPENBSD
	osname = "openbsd";
#elif XASH_EMSCRIPTEN
	osname = "emscripten";
#elif XASH_DOS4GW
	osname = "DOS4GW";
#elif XASH_HAIKU
	osname = "haiku";
#elif XASH_SERENITY
	osname = "serenityos";
#else
#error "Place your operating system name here! If this is a mistake, try to fix conditions above and report a bug"
#endif

	return osname;*/
	}

/*
============
Q_ArchitectureStringByID [FWGS, 01.04.23]

Returns name of the architecture by it's ID. Without any spaces.
============
*/
const char *Q_ArchitectureStringByID (const int arch, const uint abi, const int endianness, const qboolean is64)
	{
	// I don't want to change this function prototype
	// and don't want to use static buffer either
	// so encode all possible variants... :)
	switch (arch)
		{
		case ARCHITECTURE_AMD64:
			return "amd64";
		case ARCHITECTURE_X86:
			return "i386";
		case ARCHITECTURE_E2K:
			return "e2k";
		case ARCHITECTURE_JS:
			return "javascript";
		case ARCHITECTURE_MIPS:
			return endianness == ENDIANNESS_LITTLE ?
				(is64 ? "mips64el" : "mipsel") :
				(is64 ? "mips64" : "mips");
		case ARCHITECTURE_ARM:
			// no support for big endian ARM here
			if (endianness == ENDIANNESS_LITTLE)
				{
				const int ver = (abi >> ARCHITECTURE_ARM_VER_SHIFT) & ARCHITECTURE_ARM_VER_MASK;
				const qboolean hardfp = FBitSet (abi, ARCHITECTURE_ARM_HARDFP);

				if (is64)
					return "arm64"; // keep as arm64, it's not aarch64!

				switch (ver)
					{
					case 8:
						return hardfp ? "armv8_32hf" : "armv8_32l";
					case 7:
						return hardfp ? "armv7hf" : "armv7l";
					case 6:
						return "armv6l";
					case 5:
						return "armv5l";
					case 4:
						return "armv4l";
					}
				}
			break;
		case ARCHITECTURE_RISCV:
			switch (abi)
				{
				case ARCHITECTURE_RISCV_FP_SOFT:
					return is64 ? "riscv64" : "riscv32";
				case ARCHITECTURE_RISCV_FP_SINGLE:
					return is64 ? "riscv64f" : "riscv32f";
				case ARCHITECTURE_RISCV_FP_DOUBLE:
					return is64 ? "riscv64d" : "riscv64f";
				}
			break;
		}

	assert (0);
	return is64 ?
		(endianness == ENDIANNESS_LITTLE ? "unknown64el" : "unknownel") :
		(endianness == ENDIANNESS_LITTLE ? "unknown64be" : "unknownbe");
	}

/*
============
Q_buildarch

Returns current name of the architecture. Without any spaces.
============
*/
const char *Q_buildarch (void)
	{
	/*const char *archname;

#if XASH_AMD64
	archname = "amd64";
#elif XASH_X86
	archname = "i386";
#elif XASH_ARM && XASH_64BIT
	archname = "arm64";
#elif XASH_ARM
	archname = "armv"
#if XASH_ARM == 8
		"8_32" // for those who (mis)using 32-bit OS on 64-bit CPU
#elif XASH_ARM == 7
		"7"
#elif XASH_ARM == 6
		"6"
#elif XASH_ARM == 5
		"5"
#elif XASH_ARM == 4
		"4"
#endif

#if XASH_ARM_HARDFP
		"hf"
#else
		"l"
#endif
		;
#elif XASH_MIPS && XASH_BIG_ENDIAN
	archname = "mips"
#if XASH_64BIT
		"64"
#endif
#if XASH_LITTLE_ENDIAN
		"el"
#endif
		;
#elif XASH_RISCV
	archname = "riscv"*/
	return Q_ArchitectureStringByID (
		XASH_ARCHITECTURE,
		XASH_ARCHITECTURE_ABI,
		XASH_ENDIANNESS,

#if XASH_64BIT
		/*"64"
		#else
		"32"
		#endif
		#if XASH_RISCV_SINGLEFP
		"d"
		#elif XASH_RISCV_DOUBLEFP
		"f"
		#endif
		;
		#elif XASH_JS
		archname = "javascript";
		#elif XASH_E2K
		archname = "e2k";*/
		true
#else
		/*#error "Place your architecture name here! If this is a mistake,
		try to fix conditions above and report a bug"*/
		false
#endif
	);
	/*return archname;*/
	}

/*
=============
Q_buildcommit

Returns a short hash of current commit in VCS as string.
XASH_BUILD_COMMIT must be passed in quotes

if XASH_BUILD_COMMIT is not defined,
Q_buildcommit will identify this build as "notset"
=============
*/
const char *Q_buildcommit (void)
	{
#ifdef XASH_BUILD_COMMIT
	return XASH_BUILD_COMMIT;
#else
	return "notset";
#endif
	}
