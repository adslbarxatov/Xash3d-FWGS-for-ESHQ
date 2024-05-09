/*
com_strings.h - all paths to external resources that hardcoded into engine
Copyright (C) 2018 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#ifndef COM_STRINGS_H
#define COM_STRINGS_H

// [FWGS, 01.03.24] default colored message headers
#define S_BLACK		"^0"
#define S_RED		"^1"
#define S_GREEN		"^2"
#define S_YELLOW	"^3"
#define S_BLUE		"^4"
#define S_CYAN		"^5"
#define S_MAGENTA	"^6"
#define S_DEFAULT	"^7"

#define S_NOTE S_GREEN	"Note: " S_DEFAULT
#define S_WARN S_YELLOW	"Warning: " S_DEFAULT
#define S_ERROR S_RED	"Error: " S_DEFAULT
#define S_USAGE			"Usage: "
#define S_USAGE_INDENT	"\t"

#define S_OPENGL_NOTE S_GREEN	"OpenGL Note: " S_DEFAULT
#define S_OPENGL_WARN S_YELLOW	"OpenGL Warning: " S_DEFAULT
#define S_OPENGL_ERROR S_RED	"OpenGL Error: " S_DEFAULT

// end game final default message
#define DEFAULT_ENDGAME_MESSAGE	"The End"

// path to the hash-pak that contain custom player decals
#define CUSTOM_RES_PATH		"custom.hpk"

// path to default playermodel in GoldSrc
#define DEFAULT_PLAYER_PATH_HALFLIFE	"models/player.mdl"

// path to default playermodel in Quake
#define DEFAULT_PLAYER_PATH_QUAKE	"progs/player.mdl"

// debug beams
#define DEFAULT_LASERBEAM_PATH	"sprites/laserbeam.spr"

#define DEFAULT_INTERNAL_PALETTE	"gfx/palette.lmp"

#define DEFAULT_EXTERNAL_PALETTE	"gfx/palette.pal"

// path to folders where placed all sounds
#define DEFAULT_SOUNDPATH		"sound/"

// path store saved games
#define DEFAULT_SAVE_DIRECTORY	"save/"

// ESHQ: ��������� ����������� � � basemenu.h
#define DEFAULT_SAVE_EXTENSION	"x3v"
#define EXTENDED_SAVE_EXTENSION	"xv"

// fallback to this skybox
#define DEFAULT_SKYBOX_PATH		"gfx/env/desert"

// playlist for startup videos
#define DEFAULT_VIDEOLIST_PATH	"media/StartupVids.txt"

#define CVAR_GLCONFIG_DESCRIPTION	"enable or disable %s"

#define DEFAULT_BSP_BUILD_ERROR	"%s can't be loaded in this build. Please rebuild engine with enabled SUPPORT_BSP2_FORMAT\n"

#define DEFAULT_UPDATE_PAGE "https://github.com/FWGS/xash3d-fwgs/releases/latest"

#define XASH_ENGINE_NAME	"Xash3D FWGS"
#define XASH_DEDICATED_SERVER_NAME	"XashDS"	// [FWGS, 01.05.24]
#define XASH_VERSION		"0.20"	// engine current version
#define XASH_COMPAT_VERSION	"0.99"	// version we are based on

#endif
