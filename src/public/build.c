/***
build.c - returns a engine build number
Copyright (C) 2010 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details
***/

// [FWGS, 01.03.25]
#include <stdio.h>
#include "crtlib.h"
#include "buildenums.h"

static const char *const mon[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
static const char mond[12] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

// [FWGS, 01.03.25]
int Q_buildnum_date (const char *date)
	{
	int b;
	int m = 0;
	int d = 0;
	int y = 0;

	for (m = 0; m < 11; m++)
		{
		if (!Q_strnicmp (&date[0], mon[m], 3))
			break;
		d += mond[m];
		}

	d += Q_atoi (&date[4]) - 1;
	y = Q_atoi (&date[7]) - 1900;
	b = d + (int)((y - 1) * 365.25f);

	if (((y % 4) == 0) && (m > 1))
		b += 1;
	b -= 41728; // Apr 1 2015

	return b;
	}

// [FWGS, 01.03.25]
int Q_buildnum_iso (const char *date)
	{
	int y, m, d, b, i;

	if (sscanf (date, "%d-%d-%d", &y, &m, &d) != 3 || (y <= 1900) || (m <= 0) || (d <= 0))
		return -1;

	// fixup day and month
	m--;
	d--;

	for (i = 0; i < m; i++)
		d += mond[i];

	y -= 1900;
	b = d + (int)((y - 1) * 365.25f);

	if (((y % 4) == 0) && m > 1)
		b += 1;
	b -= 41728; // Apr 1 2015

	return b;
	}

/***
===============
Q_buildnum [FWGS, 01.03.25]

returns days since Apr 1 2015
===============
***/
int Q_buildnum (void)
	{
	static int b = 0;

	/*if (!b)
		b = Q_buildnum_date (__DATE__);*/
	if (b)
		return b;

	if (COM_CheckString (g_buildcommit_date))
		b = Q_buildnum_iso (g_buildcommit_date);

	if (b <= 0)
		b = Q_buildnum_date (g_build_date);

	return b;
	}

/***
=============
Q_buildnum_compat

Returns a Xash3D build number. This is left for compability with original Xash3D.
IMPORTANT: this value must be changed ONLY after updating to newer Xash3D base
IMPORTANT: this value must be acquired through "build" cvar
=============
***/
int Q_buildnum_compat (void)
	{
	// do not touch this! Only author of Xash3D can increase buildnumbers!
	return 4529;
	}

/***
============
Q_GetPlatformStringByID

Returns name of operating system by ID. Without any spaces
============
***/
const char *Q_PlatformStringByID (const int platform)
	{
	switch (platform)
		{
		case PLATFORM_WIN32:
			return "win32";
		case PLATFORM_ANDROID:
			return "android";
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

		// [FWGS, 01.12.24]
		case PLATFORM_WASI:
			return "wasi";
		case PLATFORM_SUNOS:
			return "sunos";
		}

	assert (0);
	return "unknown";
	}

/***
============
Q_buildos [FWGS, 01.04.23]

Shortcut for Q_buildos_
============
***/
const char *Q_buildos (void)
	{
	return Q_PlatformStringByID (XASH_PLATFORM);
	}

/***
============
Q_ArchitectureStringByID

Returns name of the architecture by it's ID. Without any spaces.
============
***/
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

		// [FWGS, 01.07.23]
		case ARCHITECTURE_PPC:
			return endianness == ENDIANNESS_LITTLE ? (is64 ? "ppc64el" : "ppcel") : (is64 ? "ppc64" : "ppc");

		case ARCHITECTURE_MIPS:
			return endianness == ENDIANNESS_LITTLE ?
				(is64 ? "mips64el" : "mipsel") :
				(is64 ? "mips64" : "mips");

		case ARCHITECTURE_ARM:
			// no support for big endian ARM here
			if (endianness == ENDIANNESS_LITTLE)
				{
				// [FWGS, 01.05.23]
				const uint ver = (abi >> ARCH_ARM_VER_SHIFT) & ARCH_ARM_VER_MASK;
				const qboolean hardfp = FBitSet (abi, ARCH_ARM_HARDFP);

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
			// [FWGS, 01.05.23]
			switch (abi)
				{
				case ARCH_RISCV_FP_SOFT:
					return is64 ? "riscv64" : "riscv32";
				case ARCH_RISCV_FP_SINGLE:
					return is64 ? "riscv64f" : "riscv32f";
				case ARCH_RISCV_FP_DOUBLE:
					return is64 ? "riscv64d" : "riscv32d";
				}
			break;

		// [FWGS, 01.12.24]
		case ARCHITECTURE_WASM:
			return is64 ? "wasm64" : "wasm32";
		}

	assert (0);
	return is64 ?
		(endianness == ENDIANNESS_LITTLE ? "unknown64el" : "unknownel") :
		(endianness == ENDIANNESS_LITTLE ? "unknown64be" : "unknownbe");
	}

/***
============
Q_buildarch

Returns current name of the architecture. Without any spaces.
============
***/
const char *Q_buildarch (void)
	{
	return Q_ArchitectureStringByID (
		XASH_ARCHITECTURE,
		XASH_ARCHITECTURE_ABI,
		XASH_ENDIANNESS,

#if XASH_64BIT
		true
#else
		false
#endif
	);
	}

// [FWGS, 01.02.25] removed Q_buildcommit, Q_buildbranch
/*
=============
Q_buildcommit [FWGS, 01.05.24]

Returns a short hash of current commit in VCS as string
XASH_BUILD_COMMIT must be passed in quotes
=============
/
const char *Q_buildcommit (void)
	{
	return XASH_BUILD_COMMIT;
	}

/*
=============
Q_buildbranch [FWGS, 01.05.24]

Returns current branch name in VCS as string
XASH_BUILD_BRANCH must be passed in quotes
=============
/
const char *Q_buildbranch (void)
	{
	return XASH_BUILD_BRANCH;
	}*/
