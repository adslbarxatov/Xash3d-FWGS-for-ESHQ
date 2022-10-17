/*
wad.c - WAD support for filesystem
Copyright (C) 2007 Uncle Mike
Copyright (C) 2022 Alibek Omarov

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#if XASH_POSIX
#include <unistd.h>
#endif
#include <errno.h>
#include <stddef.h>
#include "port.h"
#include "filesystem_internal.h"
#include "crtlib.h"
#include "common/com_strings.h"
#include "wadfile.h"

/*
========================================================================
.WAD archive format	(WhereAllData - WAD)

List of compressed files, that can be identify only by TYPE_*

<format>
header:	dwadinfo_t[dwadinfo_t]
file_1:	byte[dwadinfo_t[num]->disksize]
file_2:	byte[dwadinfo_t[num]->disksize]
file_3:	byte[dwadinfo_t[num]->disksize]
...
file_n:	byte[dwadinfo_t[num]->disksize]
infotable	dlumpinfo_t[dwadinfo_t->numlumps]
========================================================================
*/
#define WAD3_NAMELEN	16
#define HINT_NAMELEN	5	// e.g. _mask, _norm
#define MAX_FILES_IN_WAD	65535	// real limit as above <2Gb size not a lumpcount

#include "const.h"

typedef struct
	{
	int		ident;		// should be WAD3
	int		numlumps;		// num files
	int		infotableofs;	// LUT offset
	} dwadinfo_t;

typedef struct
	{
	int		filepos;		// file offset in WAD
	int		disksize;		// compressed or uncompressed
	int		size;		// uncompressed
	signed char	type;		// TYP_*
	signed char	attribs;		// file attribs
	signed char	pad0;
	signed char	pad1;
	char		name[WAD3_NAMELEN];	// must be null terminated
	} dlumpinfo_t;

struct wfile_s
	{
	string		filename;
	int		infotableofs;
	int		numlumps;
	poolhandle_t mempool;			// W_ReadLump temp buffers
	file_t *handle;
	dlumpinfo_t *lumps;
	time_t		filetime;
	};

// WAD errors
#define WAD_LOAD_OK			0
#define WAD_LOAD_COULDNT_OPEN		1
#define WAD_LOAD_BAD_HEADER		2
#define WAD_LOAD_BAD_FOLDERS		3
#define WAD_LOAD_TOO_MANY_FILES	4
#define WAD_LOAD_NO_FILES		5
#define WAD_LOAD_CORRUPTED		6

typedef struct wadtype_s
	{
	const char *ext;
	signed char		type;
	} wadtype_t;

// associate extension with wad type
static const wadtype_t wad_types[7] =
	{
	{ "pal", TYP_PALETTE	}, // palette
	{ "dds", TYP_DDSTEX 	}, // DDS image
	{ "lmp", TYP_GFXPIC		}, // quake1, hl pic
	{ "fnt", TYP_QFONT		}, // hl qfonts
	{ "mip", TYP_MIPTEX		}, // hl/q1 mip
	{ "txt", TYP_SCRIPT		}, // scripts
	{ NULL,  TYP_NONE		}
	};

/*
===========
W_TypeFromExt

Extracts file type from extension
===========
*/
static signed char W_TypeFromExt (const char *lumpname)
	{
	const char *ext = COM_FileExtension (lumpname);
	const wadtype_t *type;

	// we not known about filetype, so match only by filename
	if (!Q_strcmp (ext, "*") || !Q_strcmp (ext, ""))
		return TYP_ANY;

	for (type = wad_types; type->ext; type++)
		{
		if (!Q_stricmp (ext, type->ext))
			return type->type;
		}
	return TYP_NONE;
	}

/*
===========
W_ExtFromType

Convert type to extension
===========
*/
static const char *W_ExtFromType (signed char lumptype)
	{
	const wadtype_t *type;

	// we not known aboyt filetype, so match only by filename
	if (lumptype == TYP_NONE || lumptype == TYP_ANY)
		return "";

	for (type = wad_types; type->ext; type++)
		{
		if (lumptype == type->type)
			return type->ext;
		}
	return "";
	}

/*
====================
W_AddFileToWad

Add a file to the list of files contained into a package
and sort LAT in alpha-bethical order
====================
*/
static dlumpinfo_t *W_AddFileToWad (const char *name, wfile_t *wad, dlumpinfo_t *newlump)
	{
	int		left, right;
	dlumpinfo_t *plump;

	// look for the slot we should put that file into (binary search)
	left = 0;
	right = wad->numlumps - 1;

	while (left <= right)
		{
		int	middle = (left + right) / 2;
		int	diff = Q_stricmp (wad->lumps[middle].name, name);

		if (!diff)
			{
			if (wad->lumps[middle].type < newlump->type)
				diff = 1;
			else if (wad->lumps[middle].type > newlump->type)
				diff = -1;
			else Con_Reportf (S_WARN "Wad %s contains the file %s several times\n", wad->filename, name);
			}

		// If we're too far in the list
		if (diff > 0) right = middle - 1;
		else left = middle + 1;
		}

	// we have to move the right of the list by one slot to free the one we need
	plump = &wad->lumps[left];
	memmove (plump + 1, plump, (wad->numlumps - left) * sizeof (*plump));
	wad->numlumps++;

	*plump = *newlump;
	memcpy (plump->name, name, sizeof (plump->name));

	return plump;
	}

/*
===========
FS_CloseWAD

finalize wad or just close
===========
*/
void FS_CloseWAD (wfile_t *wad)
	{
	Mem_FreePool (&wad->mempool);
	if (wad->handle != NULL)
		FS_Close (wad->handle);
	Mem_Free (wad); // free himself
	}

/*
===========
W_Open

open the wad for reading & writing
===========
*/
static wfile_t *W_Open (const char *filename, int *error)
	{
	wfile_t *wad = (wfile_t *)Mem_Calloc (fs_mempool, sizeof (wfile_t));
	const char *basename;
	int		i, lumpcount;
	dlumpinfo_t *srclumps;
	size_t		lat_size;
	dwadinfo_t	header;

	// NOTE: FS_Open is load wad file from the first pak in the list (while fs_ext_path is false)
	if (fs_ext_path) basename = filename;
	else basename = COM_FileWithoutPath (filename);

	wad->handle = FS_Open (basename, "rb", false);

	// HACKHACK: try to open WAD by full path for RoDir, when searchpaths are not ready
	if (COM_CheckStringEmpty (fs_rodir) && fs_ext_path && wad->handle == NULL)
		wad->handle = FS_SysOpen (filename, "rb");

	if (wad->handle == NULL)
		{
		Con_Reportf (S_ERROR "W_Open: couldn't open %s\n", filename);
		if (error) *error = WAD_LOAD_COULDNT_OPEN;
		FS_CloseWAD (wad);
		return NULL;
		}

	// copy wad name
	Q_strncpy (wad->filename, filename, sizeof (wad->filename));
	wad->filetime = FS_SysFileTime (filename);
	wad->mempool = Mem_AllocPool (filename);

	if (FS_Read (wad->handle, &header, sizeof (dwadinfo_t)) != sizeof (dwadinfo_t))
		{
		Con_Reportf (S_ERROR "W_Open: %s can't read header\n", filename);
		if (error) *error = WAD_LOAD_BAD_HEADER;
		FS_CloseWAD (wad);
		return NULL;
		}

	if (header.ident != IDWAD2HEADER && header.ident != IDWAD3HEADER)
		{
		Con_Reportf (S_ERROR "W_Open: %s is not a WAD2 or WAD3 file\n", filename);
		if (error) *error = WAD_LOAD_BAD_HEADER;
		FS_CloseWAD (wad);
		return NULL;
		}

	lumpcount = header.numlumps;

	if (lumpcount >= MAX_FILES_IN_WAD)
		{
		Con_Reportf (S_WARN "W_Open: %s is full (%i lumps)\n", filename, lumpcount);
		if (error) *error = WAD_LOAD_TOO_MANY_FILES;
		}
	else if (lumpcount <= 0)
		{
		Con_Reportf (S_ERROR "W_Open: %s has no lumps\n", filename);
		if (error) *error = WAD_LOAD_NO_FILES;
		FS_CloseWAD (wad);
		return NULL;
		}
	else if (error) *error = WAD_LOAD_OK;

	wad->infotableofs = header.infotableofs; // save infotableofs position

	if (FS_Seek (wad->handle, wad->infotableofs, SEEK_SET) == -1)
		{
		Con_Reportf (S_ERROR "W_Open: %s can't find lump allocation table\n", filename);
		if (error) *error = WAD_LOAD_BAD_FOLDERS;
		FS_CloseWAD (wad);
		return NULL;
		}

	lat_size = lumpcount * sizeof (dlumpinfo_t);

	// NOTE: lumps table can be reallocated for O_APPEND mode
	srclumps = (dlumpinfo_t *)Mem_Malloc (wad->mempool, lat_size);

	if (FS_Read (wad->handle, srclumps, lat_size) != lat_size)
		{
		Con_Reportf (S_ERROR "W_ReadLumpTable: %s has corrupted lump allocation table\n", wad->filename);
		if (error) *error = WAD_LOAD_CORRUPTED;
		Mem_Free (srclumps);
		FS_CloseWAD (wad);
		return NULL;
		}

	// starting to add lumps
	wad->lumps = (dlumpinfo_t *)Mem_Calloc (wad->mempool, lat_size);
	wad->numlumps = 0;

	// sort lumps for binary search
	for (i = 0; i < lumpcount; i++)
		{
		char	name[16];
		int	k;

		// cleanup lumpname
		Q_strnlwr (srclumps[i].name, name, sizeof (srclumps[i].name));

		// check for '*' symbol issues (quake1)
		k = Q_strlen (Q_strrchr (name, '*'));
		if (k) name[Q_strlen (name) - k] = '!';

		// check for Quake 'conchars' issues (only lmp loader really allows to read this lame pic)
		if (srclumps[i].type == 68 && !Q_stricmp (srclumps[i].name, "conchars"))
			srclumps[i].type = TYP_GFXPIC;

		W_AddFileToWad (name, wad, &srclumps[i]);
		}

	// release source lumps
	Mem_Free (srclumps);

	// and leave the file open
	return wad;
	}

/*
====================
FS_AddWad_Fullpath
====================
*/
qboolean FS_AddWad_Fullpath (const char *wadfile, qboolean *already_loaded, int flags)
	{
	searchpath_t *search;
	wfile_t *wad = NULL;
	const char *ext = COM_FileExtension (wadfile);
	int		errorcode = WAD_LOAD_COULDNT_OPEN;

	for (search = fs_searchpaths; search; search = search->next)
		{
		if (search->type == SEARCHPATH_WAD && !Q_stricmp (search->wad->filename, wadfile))
			{
			if (already_loaded) *already_loaded = true;
			return true; // already loaded
			}
		}

	if (already_loaded)
		*already_loaded = false;

	if (!Q_stricmp (ext, "wad"))
		wad = W_Open (wadfile, &errorcode);

	if (wad)
		{
		search = (searchpath_t *)Mem_Calloc (fs_mempool, sizeof (searchpath_t));
		search->wad = wad;
		search->type = SEARCHPATH_WAD;
		search->next = fs_searchpaths;
		search->flags |= flags;
		fs_searchpaths = search;

		Con_Reportf ("Adding wadfile: %s (%i files)\n", wadfile, wad->numlumps);
		return true;
		}
	else
		{
		if (errorcode != WAD_LOAD_NO_FILES)
			Con_Reportf (S_ERROR "FS_AddWad_Fullpath: unable to load wad \"%s\"\n", wadfile);
		return false;
		}
	}

/*
=============================================================================

WADSYSTEM PRIVATE ROUTINES

=============================================================================
*/

/*
===========
W_FindLump

Serach for already existed lump
===========
*/
static dlumpinfo_t *W_FindLump (wfile_t *wad, const char *name, const signed char matchtype)
	{
	int	left, right;

	if (!wad || !wad->lumps || matchtype == TYP_NONE)
		return NULL;

	// look for the file (binary search)
	left = 0;
	right = wad->numlumps - 1;

	while (left <= right)
		{
		int	middle = (left + right) / 2;
		int	diff = Q_stricmp (wad->lumps[middle].name, name);

		if (!diff)
			{
			if ((matchtype == TYP_ANY) || (matchtype == wad->lumps[middle].type))
				return &wad->lumps[middle]; // found
			else if (wad->lumps[middle].type < matchtype)
				diff = 1;
			else if (wad->lumps[middle].type > matchtype)
				diff = -1;
			else break; // not found
			}

		// if we're too far in the list
		if (diff > 0) right = middle - 1;
		else left = middle + 1;
		}

	return NULL;
	}

/*
===========
W_ReadLump

reading lump into temp buffer
===========
*/
static byte *W_ReadLump (wfile_t *wad, dlumpinfo_t *lump, fs_offset_t *lumpsizeptr)
	{
	size_t	oldpos, size = 0;
	byte *buf;

	// assume error
	if (lumpsizeptr) *lumpsizeptr = 0;

	// no wads loaded
	if (!wad || !lump) return NULL;

	oldpos = FS_Tell (wad->handle); // don't forget restore original position

	if (FS_Seek (wad->handle, lump->filepos, SEEK_SET) == -1)
		{
		Con_Reportf (S_ERROR "W_ReadLump: %s is corrupted\n", lump->name);
		FS_Seek (wad->handle, oldpos, SEEK_SET);
		return NULL;
		}

	buf = (byte *)Mem_Malloc (wad->mempool, lump->disksize);
	size = FS_Read (wad->handle, buf, lump->disksize);

	if (size < lump->disksize)
		{
		Con_Reportf (S_WARN "W_ReadLump: %s is probably corrupted\n", lump->name);
		FS_Seek (wad->handle, oldpos, SEEK_SET);
		Mem_Free (buf);
		return NULL;
		}

	if (lumpsizeptr) *lumpsizeptr = lump->disksize;
	FS_Seek (wad->handle, oldpos, SEEK_SET);

	return buf;
	}

/*
===========
FS_LoadWADFile

loading lump into the tmp buffer
===========
*/
byte *FS_LoadWADFile (const char *path, fs_offset_t *lumpsizeptr, qboolean gamedironly)
	{
	searchpath_t *search;
	int		index;

	search = FS_FindFile (path, &index, gamedironly);
	if (search && search->type == SEARCHPATH_WAD)
		return W_ReadLump (search->wad, &search->wad->lumps[index], lumpsizeptr);
	return NULL;
	}

int FS_FileTimeWAD (wfile_t *wad)
	{
	return wad->filetime;
	}

void FS_PrintWADInfo (char *dst, size_t size, wfile_t *wad)
	{
	Q_snprintf (dst, size, "%s (%i files)", wad->filename, wad->numlumps);
	}

int FS_FindFileWAD (wfile_t *wad, const char *name)
	{
	dlumpinfo_t *lump;
	signed char		type = W_TypeFromExt (name);
	qboolean		anywadname = true;
	string		wadname, wadfolder;
	string		shortname;

	// quick reject by filetype
	if (type == TYP_NONE)
		return -1;

	COM_ExtractFilePath (name, wadname);
	wadfolder[0] = '\0';

	if (COM_CheckStringEmpty (wadname))
		{
		COM_FileBase (wadname, wadname);
		Q_strncpy (wadfolder, wadname, sizeof (wadfolder));
		COM_DefaultExtension (wadname, ".wad");
		anywadname = false;
		}

	// make wadname from wad fullpath
	COM_FileBase (wad->filename, shortname);
	COM_DefaultExtension (shortname, ".wad");

	// quick reject by wadname
	if (!anywadname && Q_stricmp (wadname, shortname))
		return -1;

	// NOTE: we can't using long names for wad,
	// because we using original wad names[16];
	COM_FileBase (name, shortname);

	lump = W_FindLump (wad, shortname, type);

	if (lump)
		{
		return lump - wad->lumps;
		}

	return -1;

	}

void FS_SearchWAD (stringlist_t *list, wfile_t *wad, const char *pattern)
	{
	string	wadpattern, wadname, temp2;
	signed char	type = W_TypeFromExt (pattern);
	qboolean	anywadname = true;
	string	wadfolder, temp;
	int j, i;
	const char *slash, *backslash, *colon, *separator;

	// quick reject by filetype
	if (type == TYP_NONE)
		return;

	COM_ExtractFilePath (pattern, wadname);
	COM_FileBase (pattern, wadpattern);
	wadfolder[0] = '\0';

	if (COM_CheckStringEmpty (wadname))
		{
		COM_FileBase (wadname, wadname);
		Q_strncpy (wadfolder, wadname, sizeof (wadfolder));
		COM_DefaultExtension (wadname, ".wad");
		anywadname = false;
		}

	// make wadname from wad fullpath
	COM_FileBase (wad->filename, temp2);
	COM_DefaultExtension (temp2, ".wad");

	// quick reject by wadname
	if (!anywadname && Q_stricmp (wadname, temp2))
		return;

	for (i = 0; i < wad->numlumps; i++)
		{
		// if type not matching, we already have no chance ...
		if (type != TYP_ANY && wad->lumps[i].type != type)
			continue;

		// build the lumpname with image suffix (if present)
		Q_strncpy (temp, wad->lumps[i].name, sizeof (temp));

		while (temp[0])
			{
			if (matchpattern (temp, wadpattern, true))
				{
				for (j = 0; j < list->numstrings; j++)
					{
					if (!Q_strcmp (list->strings[j], temp))
						break;
					}

				if (j == list->numstrings)
					{
					// build path: wadname/lumpname.ext
					Q_snprintf (temp2, sizeof (temp2), "%s/%s", wadfolder, temp);
					COM_DefaultExtension (temp2, va (".%s", W_ExtFromType (wad->lumps[i].type)));
					stringlistappend (list, temp2);
					}
				}

			// strip off one path element at a time until empty
			// this way directories are added to the listing if they match the pattern
			slash = Q_strrchr (temp, '/');
			backslash = Q_strrchr (temp, '\\');
			colon = Q_strrchr (temp, ':');
			separator = temp;
			if (separator < slash)
				separator = slash;
			if (separator < backslash)
				separator = backslash;
			if (separator < colon)
				separator = colon;
			*((char *)separator) = 0;
			}
		}
	}
