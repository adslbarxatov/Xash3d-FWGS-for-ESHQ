/*
render_api.h - Xash3D extension for client interface
Copyright (C) 2011 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#ifndef RENDER_API_H
#define RENDER_API_H

#include "lightstyle.h"
#include "dlight.h"

#define CL_RENDER_INTERFACE_VERSION	37	// Xash3D 1.0
#define MAX_STUDIO_DECALS			4096	// + unused space of BSP decals

// render info parms
#define PARM_TEX_WIDTH		1	// all parms with prefix 'TEX_' receive arg as texnum
#define PARM_TEX_HEIGHT		2	// otherwise it's not used
#define PARM_TEX_SRC_WIDTH	3
#define PARM_TEX_SRC_HEIGHT	4
#define PARM_TEX_SKYBOX		5	// second arg as skybox ordering num
#define PARM_TEX_SKYTEXNUM	6	// skytexturenum for quake sky
#define PARM_TEX_LIGHTMAP	7	// second arg as number 0 - 128
#define PARM_TEX_TARGET		8
#define PARM_TEX_TEXNUM		9
#define PARM_TEX_FLAGS		10
#define PARM_TEX_DEPTH		11	// 3D texture depth or 2D array num layers
//reserved
#define PARM_TEX_GLFORMAT	13	// get a texture GL-format
#define PARM_TEX_ENCODE		14	// custom encoding for DXT image
#define PARM_TEX_MIPCOUNT	15	// count of mipmaps (0 - autogenerated, 1 - disabled of mipmapping)
#define PARM_BSP2_SUPPORTED	16	// tell custom renderer what engine is support BSP2 in this build
#define PARM_SKY_SPHERE		17	// sky is quake sphere ?
#define PARAM_GAMEPAUSED	18	// game is paused
#define PARM_MAP_HAS_DELUXE	19	// map has deluxedata
#define PARM_MAX_ENTITIES	20
#define PARM_WIDESCREEN		21
#define PARM_FULLSCREEN		22
#define PARM_SCREEN_WIDTH	23
#define PARM_SCREEN_HEIGHT	24
#define PARM_CLIENT_INGAME	25
#define PARM_FEATURES		26	// same as movevars->features
#define PARM_ACTIVE_TMU		27	// for debug
#define PARM_LIGHTSTYLEVALUE	28	// second arg is stylenum
#define PARM_MAX_IMAGE_UNITS	29
#define PARM_CLIENT_ACTIVE	30
#define PARM_REBUILD_GAMMA	31	// if true lightmaps rebuilding for gamma change
#define PARM_DEDICATED_SERVER	32
#define PARM_SURF_SAMPLESIZE	33	// lightmap resolution per face (second arg interpret as facenumber)
#define PARM_GL_CONTEXT_TYPE	34	// opengl or opengles
#define PARM_GLES_WRAPPER	35	//
#define PARM_STENCIL_ACTIVE	36
#define PARM_WATER_ALPHA	37

// 4529
#define PARM_TEX_MEMORY		38	// returns total memory of uploaded texture in bytes
#define PARM_DELUXEDATA		39	// nasty hack, convert int to pointer
#define PARM_SHADOWDATA		40	// nasty hack, convert int to pointer

// skybox ordering
enum
	{
	SKYBOX_RIGHT = 0,
	SKYBOX_BACK,
	SKYBOX_LEFT,
	SKYBOX_FORWARD,
	SKYBOX_UP,
	SKYBOX_DOWN,
	};

typedef enum
	{
	TF_COLORMAP = 0,		// just for tabulate source
	TF_NEAREST = (1 << 0),		// disable texfilter
	TF_KEEP_SOURCE = (1 << 1),		// some images keep source
	TF_NOFLIP_TGA = (1 << 2),		// Steam background completely ignore tga attribute 0x20
	TF_EXPAND_SOURCE = (1 << 3),		// Don't keep source as 8-bit expand to RGBA
	// FWGS: reserved, was: TF_ALLOW_EMBOSS = (1 << 4), // Allow emboss-mapping for this image
	TF_RECTANGLE = (1 << 5),		// this is GL_TEXTURE_RECTANGLE
	TF_CUBEMAP = (1 << 6),		// it's cubemap texture
	TF_DEPTHMAP = (1 << 7),		// custom texture filter used
	TF_QUAKEPAL = (1 << 8),		// image has an quake1 palette
	TF_LUMINANCE = (1 << 9),		// force image to grayscale
	TF_SKYSIDE = (1 << 10),	// this is a part of skybox
	TF_CLAMP = (1 << 11),	// clamp texcoords to [0..1] range
	TF_NOMIPMAP = (1 << 12),	// don't build mips for this image
	TF_HAS_LUMA = (1 << 13),	// sets by GL_UploadTexture
	TF_MAKELUMA = (1 << 14),	// create luma from quake texture (only q1 textures contain luma-pixels)
	TF_NORMALMAP = (1 << 15),	// is a normalmap
	TF_HAS_ALPHA = (1 << 16),	// image has alpha (used only for GL_CreateTexture)
	TF_FORCE_COLOR = (1 << 17),	// force upload monochrome textures as RGB (detail textures)
	// 4529
	TF_UPDATE = (1 << 18),	// allow to update already loaded texture
	TF_BORDER = (1 << 19),	// zero clamp for projected textures
	TF_TEXTURE_3D = (1 << 20),	// this is GL_TEXTURE_3D
	TF_ATLAS_PAGE = (1 << 21),	// bit who indicate lightmap page or deluxemap page
	TF_ALPHACONTRAST = (1 << 22),	// special texture mode for A2C
	// reserved
	// reserved
	TF_IMG_UPLOADED = (1 << 25),	// this is set for first time when called glTexImage, otherwise it will be call glTexSubImage
	TF_ARB_FLOAT = (1 << 26),	// float textures
	TF_NOCOMPARE = (1 << 27),	// disable comparing for depth textures
	TF_ARB_16BIT = (1 << 28),	// keep image as 16-bit (not 24)
	TF_MULTISAMPLE	= (1<<29)	// FWGS: multisampling texture
	} texFlags_t;

typedef enum
	{
	CONTEXT_TYPE_GL = 0,	// FWGS: compatibility profile
	CONTEXT_TYPE_GLES_1_X,
	CONTEXT_TYPE_GLES_2_X,
	CONTEXT_TYPE_GL_CORE	// FWGS
	} gl_context_type_t;

typedef enum
	{
	GLES_WRAPPER_NONE = 0,		// native GL
	GLES_WRAPPER_NANOGL,		// used on GLES platforms
	GLES_WRAPPER_WES,			// FWGS: used on GLES platforms
	GLES_WRAPPER_GL4ES,			// FWGS: used on GLES platforms
	} gles_wrapper_t;

// 30 bytes here
typedef struct modelstate_s
	{
	short		sequence;
	short		frame;		// 10 bits multiple by 4, should be enough
	byte		blending[2];
	byte		controller[4];
	byte		poseparam[16];
	byte		body;
	byte		skin;
	short		scale;		// model scale (multiplied by 16)
	} modelstate_t;

typedef struct decallist_s
	{
	vec3_t		position;
	char		name[64];
	short		entityIndex;
	byte		depth;
	byte		flags;
	float		scale;

	// this is the surface plane that we hit so that
	// we can move certain decals across
	// transitions if they hit similar geometry
	vec3_t		impactPlaneNormal;

	modelstate_t	studio_state;	// studio decals only
	} decallist_t;

struct ref_viewpass_s;	// FWGS

typedef struct render_api_s
	{
	// Get renderer info (doesn't changes engine state at all)
	intptr_t	(*RenderGetParm)(int parm, int arg);	// FWGS: generic
	void		(*GetDetailScaleForTexture)(int texture, float* xScale, float* yScale);
	void		(*GetExtraParmsForTexture)(int texture, byte* red, byte* green, byte* blue, byte* alpha);
	lightstyle_t* (*GetLightStyle)(int number);
	dlight_t* (*GetDynamicLight)(int number);
	dlight_t* (*GetEntityLight)(int number);
	byte (*LightToTexGamma)(byte color);	// software gamma support
	float		(*GetFrameTime)(void);

	// Set renderer info (tell engine about changes)
	void		(*R_SetCurrentEntity)(struct cl_entity_s* ent); // tell engine about both currententity and currentmodel
	void		(*R_SetCurrentModel)(struct model_s* mod);	// change currentmodel but leave currententity unchanged
	int		(*R_FatPVS)(const float* org, float radius, byte* visbuffer, qboolean merge, qboolean fullvis);
	void		(*R_StoreEfrags)(struct efrag_s** ppefrag, int framecount);// store efrags for static entities

	// Texture tools
	int		(*GL_FindTexture)(const char* name);
	const char* (*GL_TextureName)(unsigned int texnum);
	const byte* (*GL_TextureData)(unsigned int texnum); // may be NULL
	int		(*GL_LoadTexture)(const char* name, const byte* buf, size_t size, int flags);
	int		(*GL_CreateTexture)(const char* name, int width, int height, const void* buffer, texFlags_t flags);	// FWGS
	int		(*GL_LoadTextureArray)(const char** names, int flags);
	int		(*GL_CreateTextureArray)(const char* name, int width, int height, int depth, const void* buffer, texFlags_t flags);	// FWGS
	void		(*GL_FreeTexture)(unsigned int texnum);

	// Decals manipulating (draw & remove)
	void		(*DrawSingleDecal)(struct decal_s* pDecal, struct msurface_s* fa);
	float* (*R_DecalSetupVerts)(struct decal_s* pDecal, struct msurface_s* surf, int texture, int* outCount);
	void		(*R_EntityRemoveDecals)(struct model_s* mod); // remove all the decals from specified entity (BSP only)

	// AVIkit support
	void* (*AVI_LoadVideo)(const char* filename, qboolean load_audio);
	int		(*AVI_GetVideoInfo)(void* Avi, int* xres, int* yres, float* duration);
	int		(*AVI_GetVideoFrameNumber)(void* Avi, float time);
	byte* (*AVI_GetVideoFrame)(void* Avi, int frame);
	void		(*AVI_UploadRawFrame)(int texture, int cols, int rows, int width, int height, const byte* data);
	void		(*AVI_FreeVideo)(void* Avi);
	int		(*AVI_IsActive)(void* Avi);
	void		(*AVI_StreamSound)(void* Avi, int entnum, float fvol, float attn, float synctime);
	void		(*AVI_Reserved0)(void);	// for potential interface expansion without broken compatibility
	void		(*AVI_Reserved1)(void);

	// glState related calls (must use this instead of normal gl-calls to prevent de-synchornize local states between engine and the client)
	void		(*GL_Bind)(int tmu, unsigned int texnum);
	void		(*GL_SelectTexture)(int tmu);
	void		(*GL_LoadTextureMatrix)(const float* glmatrix);
	void		(*GL_TexMatrixIdentity)(void);
	void		(*GL_CleanUpTextureUnits)(int last);	// pass 0 for clear all the texture units
	void		(*GL_TexGen)(unsigned int coord, unsigned int mode);
	void		(*GL_TextureTarget)(unsigned int target); // change texture unit mode without bind texture
	void		(*GL_TexCoordArrayMode)(unsigned int texmode);
	void* (*GL_GetProcAddress)(const char* name);
	void		(*GL_UpdateTexSize)(int texnum, int width, int height, int depth); // recalc statistics
	void		(*GL_Reserved0)(void);	// for potential interface expansion without broken compatibility
	void		(*GL_Reserved1)(void);

	// Misc renderer functions
	void		(*GL_DrawParticles)(const struct ref_viewpass_s* rvp, qboolean trans_pass, float frametime);
	void		(*EnvShot)(const float* vieworg, const char* name, qboolean skyshot, int shotsize); // store skybox into gfx\env folder
	int		(*SPR_LoadExt)(const char* szPicName, unsigned int texFlags); // extended version of SPR_Load
	colorVec (*LightVec)(const float* start, const float* end, float* lightspot, float* lightvec);
	struct mstudiotex_s* (*StudioGetTexture)(struct cl_entity_s* e);
	const struct ref_overview_s* (*GetOverviewParms)(void);
	const char* (*GetFileByIndex)(int fileindex);
	// [FWGS, 01.12.22]
	int			(*pfnSaveFile)(const char* filename, const void* data, int len);
	void		(*R_Reserved0)(void);	// for potential interface expansion without broken compatibility

	// [FWGS, 01.01.24] static allocations
	/*void* (*pfnMemAlloc)(size_t cb, const char* filename, const int fileline);*/
	void		*(*pfnMemAlloc)(size_t cb, const char *filename, const int fileline) ALLOC_CHECK (1);
	void		(*pfnMemFree)(void* mem, const char* filename, const int fileline);

	// engine utils (not related with render API but placed here)
	char** (*pfnGetFilesList)(const char* pattern, int* numFiles, int gamedironly);
	unsigned int	(*pfnFileBufferCRC32)(const void* buffer, const int length);
	int		(*COM_CompareFileTime)(const char* filename1, const char* filename2, int* iCompare);
	void		(*Host_Error)(const char* error, ...); // cause Host Error
	void* (*pfnGetModel)(int modelindex);
	float		(*pfnTime)(void);				// Sys_DoubleTime
	void		(*Cvar_Set)(const char* name, const char* value);
	void		(*S_FadeMusicVolume)(float fadePercent);	// fade background track (0-100 percents)

	// [FWGS, 01.04.23] a1ba: changed long to int
	void		(*SetRandomSeed)(int lSeed);		// set custom seed for RANDOM_FLOAT\RANDOM_LONG for predictable random

	// ONLY ADD NEW FUNCTIONS TO THE END OF THIS STRUCT. INTERFACE VERSION IS FROZEN AT 37
	} render_api_t;

// render callbacks
typedef struct render_interface_s
	{
	int		version;
	// passed through R_RenderFrame (0 - use engine renderer, 1 - use custom client renderer)
	int		(*GL_RenderFrame)(const struct ref_viewpass_s* rvp);
	// build all the lightmaps on new level or when gamma is changed
	void		(*GL_BuildLightmaps)(void);
	// setup map bounds for ortho-projection when we in dev_overview mode
	void		(*GL_OrthoBounds)(const float* mins, const float* maxs);
	// prepare studio decals for save
	int		(*R_CreateStudioDecalList)(decallist_t* pList, int count);
	// clear decals by engine request (e.g. for demo recording or vid_restart)
	void		(*R_ClearStudioDecals)(void);
	// grab r_speeds message
	qboolean (*R_SpeedsMessage)(char* out, size_t size);
	// alloc or destroy model custom data
	void		(*Mod_ProcessUserData)(struct model_s* mod, qboolean create, const byte* buffer);
	// alloc or destroy entity custom data
	void		(*R_ProcessEntData)(qboolean allocate);
	// get visdata for current frame from custom renderer
	byte* (*Mod_GetCurrentVis)(void);
	// tell the renderer what new map is started
	void		(*R_NewMap)(void);
	// clear the render entities before each frame
	void		(*R_ClearScene)(void);
	// 4529: shuffle previous & next states for lerping
	void		(*CL_UpdateLatchedVars)(struct cl_entity_s* e, qboolean reset);
	} render_interface_t;

#endif
