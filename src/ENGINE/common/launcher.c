/***
launcher.c - direct xash3d launcher
Copyright (C) 2015 Mittorn

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details
***/

// [FWGS, 01.07.25]
#if XASH_ENABLE_MAIN
#include "..\library_suffix\build.h"
#include "common.h"

#if XASH_SDLMAIN
	#include <SDL.h>
#endif

// [FWGS, 01.03.26]
/*if XASH_EMSCRIPTEN
	include <emscripten.h>
endif*/

// [FWGS, 01.08.24]
#ifndef XASH_GAMEDIR
	#define XASH_GAMEDIR "valve" // ! Replace with your default (base) game directory !
#endif

// [FWGS, 05.04.26]
/*if XASH_WIN32
	error "Single-binary or dedicated builds aren't supported for Windows!"
endif*/

// [FWGS, 01.04.26]
/*static char		szGameDir[128]; // safe place to keep gamedir
static int		szArgc;
static char		**szArgv;*/
static int	szArgc;
static char	**szArgv;

// [FWGS, 22.01.25]
static void Sys_ChangeGame (const char *progname)
	{
	// stub
	}

// [FWGS, 01.04.26] removed Sys_Start

/*// [FWGS, 01.07.25]
static int Sys_Start (void)
	{
	Q_strncpy (szGameDir, XASH_GAMEDIR, sizeof (szGameDir));

// [FWGS, 01.03.26]

// [FWGS, 01.11.25]
if XASH_IOS
	IOS_LaunchDialog ();
	szArgc = IOS_GetArgs (&szArgv);
endif

	return Host_Main (szArgc, szArgv, szGameDir, 0, Sys_ChangeGame);
	}*/

int main (int argc, char **argv)
	{
#if XASH_PSVITA
	
	// inject -dev -console into args if required
	szArgc = PSVita_GetArgv (argc, argv, &szArgv);

	// [FWGS, 01.04.26]
#elif XASH_IOS

	IOS_LaunchDialog ();
	szArgc = IOS_GetArgs (&szArgv);

#else

	szArgc = argc;
	szArgv = argv;

#endif

	// [FWGS, 01.04.26]
	/*return Sys_Start ();*/
	return Host_Main (szArgc, szArgv, XASH_GAMEDIR, 0, Sys_ChangeGame);
	}

#endif
