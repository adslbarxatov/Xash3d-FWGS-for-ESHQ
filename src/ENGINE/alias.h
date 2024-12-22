/***
Copyright (c) 1996-2002, Valve LLC. All rights reserved.

This product contains software technology licensed from Id
Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
All Rights Reserved.

Use, distribution, and modification of this source code and/or resulting
object code is restricted to non-commercial enhancements to products from
Valve LLC.  All other use, distribution, or modification is prohibited
without written permission from Valve LLC
***/

#ifndef ALIAS_H
#define ALIAS_H

#include "build.h"
#include <stdint.h>		// [FWGS, 01.12.24]
#include "synctype.h"

/***
==============================================================================
ALIAS MODELS

Alias models are position independent, so the cache manager can move them
==============================================================================
*/

#define IDALIASHEADER	(('O'<<24)+('P'<<16)+('D'<<8)+'I')	// little-endian "IDPO"

#define ALIAS_VERSION	6

// client-side model flags
#define ALIAS_ROCKET		0x0001	// leave a trail
#define ALIAS_GRENADE		0x0002	// leave a trail
#define ALIAS_GIB			0x0004	// leave a trail
#define ALIAS_ROTATE		0x0008	// rotate (bonus items)
#define ALIAS_TRACER		0x0010	// green split trail
#define ALIAS_ZOMGIB		0x0020	// small blood trail
#define ALIAS_TRACER2		0x0040	// orange split trail + rotate
#define ALIAS_TRACER3		0x0080	// purple trail

typedef enum
	{
	ALIAS_SINGLE = 0,
	ALIAS_GROUP
	} aliasframetype_t;

typedef enum
	{
	ALIAS_SKIN_SINGLE = 0,
	ALIAS_SKIN_GROUP
	} aliasskintype_t;

// [FWGS, 01.04.23]
typedef struct
	{
	int32_t		ident;
	int32_t		version;
	vec3_t		scale;
	vec3_t		scale_origin;
	float		boundingradius;
	vec3_t		eyeposition;
	int32_t		numskins;
	int32_t		skinwidth;
	int32_t		skinheight;
	int32_t		numverts;
	int32_t		numtris;
	int32_t		numframes;
	uint32_t	synctype;
	int32_t		flags;
	float		size;
	} daliashdr_t;

// [FWGS, 01.12.24]
/*STATIC_ASSERT (sizeof (daliashdr_t) == 84, "invalid daliashdr_t size");*/
STATIC_CHECK_SIZEOF (daliashdr_t, 84, 84);

// [FWGS, 01.04.23]
typedef struct
	{
	int32_t		onseam;
	int32_t		s;
	int32_t		t;
	} stvert_t;

// [FWGS, 01.12.24]
/*STATIC_ASSERT (sizeof (stvert_t) == 12, "invalid stvert_t size");*/
STATIC_CHECK_SIZEOF (stvert_t, 12, 12);

// [FWGS, 01.04.23]
typedef struct dtriangle_s
	{
	int32_t		facesfront;
	int32_t		vertindex[3];
	} dtriangle_t;

// [FWGS, 01.12.24]
/*STATIC_ASSERT (sizeof (dtriangle_t) == 16, "invalid dtriangle_t size");*/
STATIC_CHECK_SIZEOF (dtriangle_t, 16, 16);

#define DT_FACES_FRONT	0x0010
#define ALIAS_ONSEAM	0x0020

// [FWGS, 01.04.23]
typedef struct
	{
	trivertex_t	bboxmin;	// lightnormal isn't used
	trivertex_t	bboxmax;	// lightnormal isn't used
	char		name[16];	// frame name from grabbing
	} daliasframe_t;

// [FWGS, 01.12.24]
/*STATIC_ASSERT (sizeof (daliasframe_t) == 24, "invalid daliasframe_t size");*/
STATIC_CHECK_SIZEOF (daliasframe_t, 24, 24);

// [FWGS, 01.04.23]
typedef struct
	{
	int32_t		numframes;
	trivertex_t	bboxmin;	// lightnormal isn't used
	trivertex_t	bboxmax;	// lightnormal isn't used
	} daliasgroup_t;

// [FWGS, 01.12.24]
/*STATIC_ASSERT (sizeof (daliasgroup_t) == 12, "invalid daliasgrou_t size");*/
STATIC_CHECK_SIZEOF (daliasgroup_t, 12, 12);

// [FWGS, 01.04.23]
typedef struct
	{
	int32_t		numskins;
	} daliasskingroup_t;

// [FWGS, 01.12.24]
/*STATIC_ASSERT (sizeof (daliasskingroup_t) == 4, "invalid daliasskingroup_t size");*/
STATIC_CHECK_SIZEOF (daliasskingroup_t, 4, 4);

// [FWGS, 01.04.23]
typedef struct
	{
	float		interval;
	} daliasinterval_t;

// [FWGS, 01.12.24]
/*STATIC_ASSERT (sizeof (daliasinterval_t) == 4, "invalid daliasinterval_t size");*/
STATIC_CHECK_SIZEOF (daliasinterval_t, 4, 4);

// [FWGS, 01.04.23]
typedef struct
	{
	float		interval;
	} daliasskininterval_t;

// [FWGS, 01.12.24]
/*STATIC_ASSERT (sizeof (daliasskininterval_t) == 4, "invalid daliasskininterval_t size");*/
STATIC_CHECK_SIZEOF (daliasskininterval_t, 4, 4);

// [FWGS, 01.04.23]
typedef struct
	{
	uint32_t	type;
	} daliasframetype_t;

// [FWGS, 01.12.24]
/*STATIC_ASSERT (sizeof (daliasframetype_t) == 4, "invalid daliasframetype_t size");*/
STATIC_CHECK_SIZEOF (daliasframetype_t, 4, 4);

// [FWGS, 01.04.23]
typedef struct
	{
	uint32_t	type;
	} daliasskintype_t;

// [FWGS, 01.12.24]
/*STATIC_ASSERT (sizeof (daliasskintype_t) == 4, "invalid daliasskintype_t size");*/
STATIC_CHECK_SIZEOF (daliasskintype_t, 4, 4);

#endif
