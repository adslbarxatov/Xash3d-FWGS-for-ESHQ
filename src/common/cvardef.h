/*
Copyright (c) 1996-2002, Valve LLC. All rights reserved.
	
This product contains software technology licensed from Id 
Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
All Rights Reserved.

Use, distribution, and modification of this source code and/or resulting
object code is restricted to non-commercial enhancements to products from
Valve LLC.  All other use, distribution, or modification is prohibited
without written permission from Valve LLC
/

ifndef CVARDEF_H
define CVARDEF_H

define FCVAR_ARCHIVE		(1<<0)	// set to cause it to be saved to vars.rc
define FCVAR_USERINFO		(1<<1)	// changes the client's info string
define FCVAR_SERVER		(1<<2)	// notifies players when changed
define FCVAR_EXTDLL		(1<<3)	// defined by external DLL
define FCVAR_CLIENTDLL		(1<<4)	// defined by the client dll
define FCVAR_PROTECTED		(1<<5)	// It's a server cvar, but we don't send the data since it's a password, etc.  Sends 1 if it's not bland/zero, 0 otherwise as value
define FCVAR_SPONLY		(1<<6)	// This cvar cannot be changed by clients connected to a multiplayer server.
define FCVAR_PRINTABLEONLY		(1<<7)	// This cvar's string cannot contain unprintable characters ( e.g., used for player name etc ).
define FCVAR_UNLOGGED		(1<<8)	// If this is a FCVAR_SERVER, don't log changes to the log file / console if we are creating a log
define FCVAR_NOEXTRAWHITEPACE	(1<<9)	// strip trailing/leading white space from this cvar

// FWGS
define FCVAR_PRIVILEGED	(1<<10) // only available in privileged mode
define FCVAR_FILTERABLE	(1<<11) // filtered in unprivileged mode if cl_filterstuffcmd is 1
define FCVAR_GLCONFIG		(1<<12)	// write it into <renderer>.cfg(see RefAPI)

define FCVAR_CHANGED		(1<<13)	// set each time the cvar is changed
define FCVAR_GAMEUIDLL		(1<<14)	// defined by the menu DLL
define FCVAR_CHEAT			(1<<15)	// can not be changed if cheats are disabled

// FWGS: a1ba: let's reuse higher bits for flags extensions from now on
define FCVAR_LATCH			(1<<30)	// notify client what this cvar will be applied only after server restart (but don't does more nothing)

typedef struct cvar_s*/

/***
cvardef.h - quake cvar definition
Copyright (C) 1997-2001 Id Software, Inc.
Copyright (C) 2024 Alibek Omarov

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
***/

#ifndef CVAR
#define CVAR

// [FWGS, 01.12.24] ��������� ���������
#include <stdint.h>		// ESHQ: ������ �����������
#include "xash3d_types.h"

/***
cvar_t variables are used to hold scalar or string variables that can be changed
or displayed at the console or prog code as well as accessed directly
in C code.

The user can access cvars from the console in three ways:
r_draworder prints the current value
r_draworder 0 sets the current value to 0
set r_draworder 0 as above, but creates the cvar if not present

Cvars are restricted from having the same names as commands to keep this
interface from being ambiguous.

The are also occasionally used to communicated information between different
modules of the program
***/

enum
	{
	/*char	*name;
	char	*string;
	int		flags;
	float	value;
	struct cvar_s	*next;
	} cvar_t;

	endif*/
	// GoldSrc compatibility flags
	FCVAR_ARCHIVE = 1 << 0,		// set to cause it to be saved to vars.rc
	FCVAR_USERINFO = 1 << 1,	// added to userinfo when changed
	FCVAR_SERVER = 1 << 2,		// added to serverinfo when changed, will notify clients by default
	FCVAR_EXTDLL = 1 << 3,		// defined by server.dll
	FCVAR_CLIENTDLL = 1 << 4,	// defined by client.dll
	FCVAR_PROTECTED = 1 << 5,	// private server cvar
	FCVAR_SPONLY = 1 << 6,		// can be set only in singleplayer
	FCVAR_PRINTABLEONLY = 1 << 7,		// only allows printable characters
	FCVAR_UNLOGGED = 1 << 8,	// disables notifying client about server cvar change
	FCVAR_NOEXTRAWHITESPACE = 1 << 9,	// removes space characters from the beginning and the end of the string
	FCVAR_PRIVILEGED = 1 << 10,	// only available in privileged mode
	FCVAR_FILTERABLE = 1 << 11,	// treated as privileged if cl_filterstuffcmd is 1, otherwise ignored

	// Xash3D public flags
	// FCVAR_LATCH = 1 << 11,	// deprecated Xash3D flag, conflicts with FCVAR_FILTERABLE. Use another FCVAR_LATCH!
	FCVAR_GLCONFIG = 1 << 12,	// set to cause it to be saved to <renderer>.cfg (see RefAPI)
	FCVAR_CHANGED = 1 << 13,	// set each time the cvar changed
	FCVAR_GAMEUIDLL = 1 << 14,	// defined by menu.dll
	FCVAR_CHEAT = 1 << 15,		// cannot be changed if sv_cheats is 0

	// Xash3D internal flags, MUST NOT be used outside of engine
#if REF_DLL || ENGINE_DLL
	FCVAR_RENDERINFO = 1 << 16,	// set to cause it to be saved to video.cfg
	FCVAR_READ_ONLY = 1 << 17,	// display only, cannot be set by user at all
	FCVAR_EXTENDED = 1 << 18,	// extended cvar structure
	FCVAR_ALLOCATED = 1 << 19,	// allocated by the engine, must be freed with Mem_Free
	FCVAR_VIDRESTART = 1 << 20,	// triggers video subsystem to recreate/modify window parameters
	FCVAR_TEMPORARY = 1 << 21,	// only used to temporarly hold some value, can be unlinked
	FCVAR_MOVEVARS = 1 << 22,	// access to movevars_t structure, synchornized between client and server
	FCVAR_USER_CREATED = 1 << 23,	// created by a set command

	FCVAR_REFDLL = 1 << 29,		// (Xash3D FWGS internal flag) defined by the renderer DLL
#endif

	FCVAR_LATCH = 1 << 30,		// (Xash3D FWGS public flag, was FCVAR_FILTERABLE in Xash3D) save changes until server restart
	};

struct cvar_s
	{
	char		*name;
	char		*string;
	uint32_t	flags;
	float		value;
	struct cvar_s	*next;
	};
typedef struct cvar_s cvar_t;

STATIC_CHECK_SIZEOF (struct cvar_s, 20, 32);

#if REF_DLL || ENGINE_DLL

struct convar_s
	{
	char		*name;
	char		*string;
	uint32_t	flags;
	float		value;
	struct convar_s	*next;
	char		*desc;
	char		*def_string;
	};
typedef struct convar_s convar_t;

#if XASH_64BIT
	#define CVAR_SENTINEL (uintptr_t)0xDEADBEEFDEADBEEF
#else
	#define CVAR_SENTINEL (uintptr_t)0xDEADBEEF
#endif

#define CVAR_CHECK_SENTINEL( cv ) ((uintptr_t)(cv)->next == CVAR_SENTINEL)

#define CVAR_DEFINE( cv, cvname, cvstr, cvflags, cvdesc ) \
	convar_t cv = { (char*)cvname, (char*)cvstr, cvflags, 0.0f, (void *)CVAR_SENTINEL, (char*)cvdesc, NULL }

#define CVAR_DEFINE_AUTO( cv, cvstr, cvflags, cvdesc ) CVAR_DEFINE( cv, #cv, cvstr, cvflags, cvdesc )
#endif

#endif
