/***
platform_sdl2.h - SDL backend internal header
Copyright (C) 2015-2018 a1batross

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
#ifndef KEYWRAPPER_H
#define KEYWRAPPER_H
#ifdef  XASH_SDL

// [FWGS, 01.06.25]
/*include "platform/platform.h"*/
#include "platform.h"

// window management
void VID_RestoreScreenResolution (void);
qboolean  VID_CreateWindow (int width, int height, window_mode_t window_mode);
void      VID_DestroyWindow (void);
void GL_InitExtensions (void);
qboolean GL_DeleteContext (void);
void VID_SaveWindowSize (int width, int height, qboolean maximized);

//
// in_sdl.c
//
void SDLash_InitCursors (void);
void SDLash_FreeCursors (void);

//
// joy_sdl.c [FWGS, 01.03.25]
//
void SDLash_HandleGameControllerEvent (SDL_Event *ev);

#endif
#endif
