#pragma once
#ifndef VID_COMMON
#define VID_COMMON

// [FWGS, 01.11.25]
typedef struct vidmode_s
	{
	/*const char *desc;
	int			width;
	int			height;*/
	const char	*desc;
	int			width;
	int			height;
	} vidmode_t;

typedef enum window_mode_e
	{
	WINDOW_MODE_WINDOWED = 0,
	WINDOW_MODE_FULLSCREEN,
	WINDOW_MODE_BORDERLESS,
	WINDOW_MODE_COUNT,
	} window_mode_t;

// [FWGS, 01.11.25]
typedef struct
	{
	/*void	*context; // handle to GL rendering context
	int		safe;

	int		desktopBitsPixel;
	int		desktopHeight;

	qboolean	initialized;	// OpenGL subsystem started
	qboolean	extended;		// extended context allows to GL_Debug
	qboolean       software;*/
	void		*context; // handle to GL rendering context
	int			safe;
	int			desktopBitsPixel;
	qboolean	software;
	} glwstate_t;

extern glwstate_t glw_state;

#define VID_MIN_HEIGHT	200
/*define VID_MIN_WIDTH 320*/
#define VID_MIN_WIDTH	320

// [FWGS, 01.11.25]
/*extern convar_t vid_fullscreen;*/
extern convar_t vid_fullscreen;
extern convar_t vid_maximized;
/*extern convar_t vid_highdpi;*/
extern convar_t vid_highdpi;
extern convar_t window_width;
extern convar_t window_height;
extern convar_t window_xpos;
extern convar_t window_ypos;
/*extern convar_t gl_msaa_samples;*/
extern convar_t gl_msaa_samples;

// [FWGS, 01.01.24]
void R_SaveVideoMode (int w, int h, int render_w, int render_h, qboolean maximized);
void VID_SetDisplayTransform (int *render_w, int *render_h);
void VID_CheckChanges (void);
const char *VID_GetModeString (int vid_mode);

#endif
