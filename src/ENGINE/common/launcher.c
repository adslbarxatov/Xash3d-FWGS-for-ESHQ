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
#include "build.h"
#include "common.h"

#if XASH_SDLMAIN
	#include <SDL.h>
#endif

#if XASH_EMSCRIPTEN
	#include <emscripten.h>
#endif

/*// [FWGS, 01.01.24]
#include "build.h"
#include "common.h"*/

// [FWGS, 01.08.24]
#ifndef XASH_GAMEDIR
	#define XASH_GAMEDIR "valve" // ! Replace with your default (base) game directory !
#endif

// [FWGS, 01.11.23]
#if XASH_WIN32
	#error "Single-binary or dedicated builds aren't supported for Windows!"
#endif

static char		szGameDir[128]; // safe place to keep gamedir
static int		szArgc;
static char		**szArgv;

// [FWGS, 22.01.25]
static void Sys_ChangeGame (const char *progname)
	{
	// stub
	}

// [FWGS, 01.07.25]
static int Sys_Start (void)
	{
	Q_strncpy (szGameDir, XASH_GAMEDIR, sizeof (szGameDir));

#if XASH_EMSCRIPTEN
	/*ifdef EMSCRIPTEN_LIB_FS
	// For some unknown reason emscripten refusing to load libraries later
	COM_LoadLibrary ("menu", 0);
	COM_LoadLibrary ("server", 0);
	COM_LoadLibrary ("client", 0);
	endif

	if XASH_DEDICATED
	// NodeJS support for debug*/
#if !XASH_DEDICATED

	EM_ASM (try
		{
		FS.mkdir ('/rwdir');
		FS.mount (IDBFS, { root: '.' }, '/rwdir');
		}
	catch (e) {};);

#else

	EM_ASM (try
		{
		FS.mkdir ('/xash');
		FS.mount (NODEFS, { root: '.' }, '/xash');
		/*FS.chdir ('/xash');*/
		}
	catch (e) {};);

/*endif

elif XASH_IOS*/
#endif

#endif

#if XASH_IOS
	IOS_LaunchDialog ();
#endif

	return Host_Main (szArgc, szArgv, szGameDir, 0, Sys_ChangeGame);
	}

int main (int argc, char **argv)
	{
#if XASH_PSVITA
	
	// inject -dev -console into args if required
	szArgc = PSVita_GetArgv (argc, argv, &szArgv);

#else

	szArgc = argc;
	szArgv = argv;

#endif

	return Sys_Start ();
	}

#endif
