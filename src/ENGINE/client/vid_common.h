/*pragma once*/
/***
vid_common.h - common implementation of platform-specific vid component
Copyright (C) 2025 Xash3D FWGS contributors

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details
***/

#ifndef VID_COMMON
#define VID_COMMON

// [FWGS, 01.03.26]
#include "ref_api.h"

// [FWGS, 01.11.25]
typedef struct vidmode_s
	{
	const char	*desc;
	int			width;
	int			height;
	} vidmode_t;

// [FWGS, 01.03.26]
/*typedef enum window_mode_e
	{
	WINDOW_MODE_WINDOWED = 0,
	WINDOW_MODE_FULLSCREEN,
	WINDOW_MODE_BORDERLESS,
	WINDOW_MODE_COUNT,
	} window_mode_t;*/

// [FWGS, 01.03.26]
typedef struct
	{
	void		*context; // handle to GL rendering context
	/*int			safe;
	int			desktopBitsPixel;*/
	ref_safegl_context_t	safe;
	qboolean	software;
	} glwstate_t;

extern glwstate_t glw_state;

#define VID_MIN_HEIGHT	200
#define VID_MIN_WIDTH	320

// [FWGS, 01.03.26]
extern convar_t vid_fullscreen;
extern convar_t vid_maximized;
/*extern convar_t vid_highdpi;*/
extern convar_t window_width;
extern convar_t window_height;
/*extern convar_t window_xpos;
extern convar_t window_ypos;*/
extern convar_t gl_msaa_samples;

// [FWGS, 01.01.24]
void R_SaveVideoMode (int w, int h, int render_w, int render_h, qboolean maximized);
void VID_SetDisplayTransform (int *render_w, int *render_h);
void VID_CheckChanges (void);
const char *VID_GetModeString (int vid_mode);

#endif
