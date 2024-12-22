/***
filesystem.c - game filesystem based on DP fs
Copyright (C) 2007 Uncle Mike
Copyright (C) 2003-2006 Mathieu Olivier
Copyright (C) 2000-2007 DarkPlaces contributors
Copyright (C) 2015-2023 Xash3D FWGS contributors

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details
***/

#include <errno.h>		// [FWGS, 01.07.24]
#include "common.h"
#include "library.h"
#include "server.h"		// ESHQ
#include "platform/platform.h"

fs_api_t g_fsapi;
fs_globals_t *FI;

static pfnCreateInterface_t fs_pfnCreateInterface;
static HINSTANCE fs_hInstance;

// [FWGS, 01.12.24]
search_t *FS_Search (const char *pattern, int caseinsensitive, int gamedironly)
	{
	return g_fsapi.Search (pattern, caseinsensitive, gamedironly);
	}

// [FWGS, 01.12.24]
int FS_Close (file_t *file)
	{
	return g_fsapi.Close (file);
	}

// [FWGS, 01.12.24]
file_t *FS_Open (const char *filepath, const char *mode, qboolean gamedironly)
	{
	return g_fsapi.Open (filepath, mode, gamedironly);
	}

// [FWGS, 01.12.24]
byte *FS_LoadFile (const char *path, fs_offset_t *filesizeptr, qboolean gamedironly)
	{
	return g_fsapi.LoadFile (path, filesizeptr, gamedironly);
	}

// [FWGS, 01.12.24]
byte *FS_LoadDirectFile (const char *path, fs_offset_t *filesizeptr)
	{
	return g_fsapi.LoadDirectFile (path, filesizeptr);
	}

// [FWGS, 01.07.24]
static void COM_StripDirectorySlash (char *pname)
	{
	size_t len;

	len = Q_strlen (pname);
	if (len > 0 && pname[len - 1] == '/')
		pname[len - 1] = 0;
	}

void *FS_GetNativeObject (const char *obj)
	{
	if (fs_pfnCreateInterface)
		return fs_pfnCreateInterface (obj, NULL);

	return NULL;
	}

static void FS_Rescan_f (void)
	{
	FS_Rescan ();
	}

static void FS_ClearPaths_f (void)
	{
	FS_ClearSearchPath ();
	}

static void FS_Path_f_ (void)
	{
	FS_Path_f ();
	}

// [FWGS, 01.08.24]
static const fs_interface_t fs_memfuncs =
	{
	Con_Printf,
	Con_DPrintf,
	Con_Reportf,
	Sys_Error,
	_Mem_AllocPool,
	_Mem_FreePool,
	_Mem_Alloc,
	_Mem_Realloc,
	_Mem_Free,
	Sys_GetNativeObject,
	};

// [FWGS, 01.05.23]
static void FS_UnloadProgs (void)
	{
	if (fs_hInstance)
		{
		COM_FreeLibrary (fs_hInstance);
		fs_hInstance = 0;
		}
	}

#ifdef XASH_INTERNAL_GAMELIBS
#define FILESYSTEM_STDIO_DLL "filesystem_stdio"
#else
// ESHQ: другое название DLL
#define FILESYSTEM_STDIO_DLL "FS." OS_LIB_EXT
#endif

// [FWGS, 01.07.24]
static qboolean FS_LoadProgs (void)
	{
	const char *name = FILESYSTEM_STDIO_DLL;
	FSAPI GetFSAPI;

	fs_hInstance = COM_LoadLibrary (name, false, true);

	if (!fs_hInstance)
		{
		Sys_Error ("%s: can't load filesystem library %s: %s\n", __func__, name, COM_GetLibraryError ());
		return false;
		}

	if (!(GetFSAPI = (FSAPI)COM_GetProcAddress (fs_hInstance, GET_FS_API)))
		{
		FS_UnloadProgs ();
		Sys_Error ("%s: can't find GetFSAPI entry point in %s\n", __func__, name);
		return false;
		}

	if (GetFSAPI (FS_API_VERSION, &g_fsapi, &FI, &fs_memfuncs) != FS_API_VERSION)
		{
		FS_UnloadProgs ();
		Sys_Error ("%s: can't initialize filesystem API: wrong version\n", __func__);
		return false;
		}

	if (!(fs_pfnCreateInterface = (pfnCreateInterface_t)COM_GetProcAddress (fs_hInstance, "CreateInterface")))
		{
		FS_UnloadProgs ();
		Sys_Error ("%s: can't find CreateInterface entry point in %s\n", __func__, name);
		return false;
		}

	Con_DPrintf ("%s: filesystem_stdio successfully loaded\n", __func__);
	return true;
	}

// [FWGS, 01.07.24]
static qboolean FS_DetermineRootDirectory (char *out, size_t size)
	{
	const char *path = getenv ("XASH3D_BASEDIR");

	if (COM_CheckString (path))
		{
		Q_strncpy (out, path, size);
		return true;
		}

#if TARGET_OS_IOS

	Q_strncpy (out, IOS_GetDocsDir (), size);
	return true;

#elif XASH_ANDROID && XASH_SDL

	path = SDL_AndroidGetExternalStoragePath ();
	if (path != NULL)
		{
		Q_strncpy (out, path, size);
		return true;
		}
	Sys_Error ("couldn't determine Android external storage path: %s", SDL_GetError ());
	return false;

#elif XASH_PSVITA

	if (PSVita_GetBasePath (out, size))
		return true;
	Sys_Error ("couldn't find Xash3D data directory");
	return false;

#elif ( XASH_SDL == 2 ) && !XASH_NSWITCH // GetBasePath not impl'd in switch-sdl2

	path = SDL_GetBasePath ();
	if (path != NULL)
		{
		Q_strncpy (out, path, size);
		SDL_free ((void *)path);
		return true;
		}

#if XASH_POSIX || XASH_WIN32

	if (getcwd (out, size))
		return true;
	Sys_Error ("couldn't determine current directory: %s, getcwd: %s", SDL_GetError (), strerror (errno));

#else

	Sys_Error ("couldn't determine current directory: %s", SDL_GetError ());

#endif

	return false;

#else

	if (getcwd (out, size))
		return true;

	Sys_Error ("couldn't determine current directory: %s", strerror (errno));
	return false;

#endif
	}

// [FWGS, 01.07.24]
static qboolean FS_DetermineReadOnlyRootDirectory (char *out, size_t size)
	{
	const char *env_rodir = getenv ("XASH3D_RODIR");

	if (_Sys_GetParmFromCmdLine ("-rodir", out, size))
		return true;

	if (COM_CheckString (env_rodir))
		{
		Q_strncpy (out, env_rodir, size);
		return true;
		}

	return false;
	}

/***
================
FS_Init [FWGS, 01.08.24]
================
***/
void FS_Init (const char *basedir)
	{
	string	gamedir;
	char	rodir[MAX_OSPATH], rootdir[MAX_OSPATH];
	rodir[0] = rootdir[0] = 0;

	if (!FS_DetermineRootDirectory (rootdir, sizeof (rootdir)) || !COM_CheckStringEmpty (rootdir))
		{
		Sys_Error ("couldn't determine current directory (empty string)");
		return;
		}

	COM_FixSlashes (rootdir);
	COM_StripDirectorySlash (rootdir);

	FS_DetermineReadOnlyRootDirectory (rodir, sizeof (rodir));
	COM_FixSlashes (rodir);
	COM_StripDirectorySlash (rodir);

	if (!Sys_GetParmFromCmdLine ("-game", gamedir))
		{
		char *env = getenv ("XASH3D_GAME");
		if (env)
			Q_strncpy (gamedir, env, sizeof (gamedir));
		else
			Q_strncpy (gamedir, basedir, sizeof (gamedir)); // gamedir == basedir
		}

	FS_LoadProgs ();

	// TODO: this function will cause engine to stop in case of fail
	// when it will have an option to return string error, restore Sys_Error
	// FIXME: why do we call this function before InitStdio?
	// because InitStdio immediately scans all available game directories
	// and this better be reworked at some point
	g_fsapi.SetCurrentDirectory (rootdir);
	if (!g_fsapi.InitStdio (true, rootdir, basedir, gamedir, rodir))
		{
		Sys_Error ("Can't init filesystem_stdio!\n");
		return;
		}

	Cmd_AddRestrictedCommand ("fs_rescan", FS_Rescan_f, "rescan filesystem search pathes");
	Cmd_AddRestrictedCommand ("fs_path", FS_Path_f_, "show filesystem search pathes");
	Cmd_AddRestrictedCommand ("fs_clearpaths", FS_ClearPaths_f, "clear filesystem search pathes");

	if (!Sys_GetParmFromCmdLine ("-dll", host.gamedll))
		host.gamedll[0] = 0;

	if (!Sys_GetParmFromCmdLine ("-clientlib", host.clientlib))
		host.clientlib[0] = 0;
	}

/***
================
FS_Shutdown [FWGS, 01.07.24]
================
***/
void FS_Shutdown (void)
	{
	if (g_fsapi.ShutdownStdio)
		g_fsapi.ShutdownStdio ();

	FS_UnloadProgs ();
	}

/***
================
ESHQ: поддержка достижений
================
***/

// ESHQ: состояние теперь хранится в памяти, чтобы постоянно не дёргать файл
static unsigned int WAS_Level = 0, WAS_Code_Gr = 0, WAS_Code_Rt;
static unsigned char WAS_entered = 0;

// Метод запрашивает настройки из сохранённой конфигурации, обновляет их до нового формата
// и контролирует их целостность.
// Режимы: 0 - достижения, 1 - гравитация, 2 - тип помещения
// Новый уровень учитывается только в режиме 0

void FS_WriteAchievementsScript (byte Mode, int NewLevel)
	{
	// Переменные
	file_t *f;
	char temp[32];

	convar_t *rt = Cvar_FindVar ("room_type");
	unsigned int gravity = (unsigned int)sv_gravity.value;
	unsigned int roomtype = 0;

	// Защита от двойного входа
	if (WAS_entered)
		return;
	WAS_entered = 1;

	if (rt)
		roomtype = (unsigned int)rt->value;

	// Чтение предыдущего состояния (ошибки игнорируются)
	if (!WAS_Code_Gr)
		{
		f = FS_Open (ACHI_SCRIPT_С, "r", false);
		if (f)
			{
			WAS_Level = ((unsigned int)FS_Getc (f)) & 0x0F;
			WAS_Code_Gr = ((unsigned int)FS_Getc (f) - 0x20) & 0x3F;
			WAS_Code_Rt |= ((unsigned int)FS_Getc (f) - 0x20) & 0x3F;
			FS_Close (f);
			}
		}

	// Обработка в зависимости от режима вызова
	switch (Mode)
		{
		case 0:
			if ((NewLevel <= (int)WAS_Level) || (WAS_Level >= 3))
				{
				WAS_entered = 0;
				return;
				}
			break;

		case 1:
			if (WAS_Code_Gr == ((gravity >> 4) & 0x3F))
				{
				WAS_entered = 0;
				return;
				}
			break;

		case 2:
			if (WAS_Code_Rt == roomtype & 0x3F)
				{
				WAS_entered = 0;
				return;
				}
			break;
		}

	// Попытка записать здесь приводила к тому, что в случае одномоментного или близкого к этому
	// срабатывания обоих типов триггеров второй из них игнорировался из-за совпадения проверочного
	// кода (он к этому моменту уже был актуальным). Поэтому изменение версии кода, хранящегося в памяти,
	// теперь выполняется только в связке с режимом, в котором вызвана функция. Так проверочный код
	// не сможет совпасть с вычисляемым в реальном времени вариантом, пока все проверяемые им команды
	// не будут выполнены
	// WAS_Code = newCode;

	// Условие для последующего повышения.
	// int, потому что иначе при сравнении происходит приведение к uint
	if ((NewLevel > (int)WAS_Level) && (WAS_Level < 3))
		WAS_Level++;

	// Запись проверочного кода (некритично)
	f = FS_Open (ACHI_SCRIPT_С, "w", false);
	if (f)
		{
		Q_snprintf (temp, sizeof (temp), "%c%c%c", WAS_Level + 0x70, ((gravity >> 4) & 0x3F) + 0x20,
			(roomtype & 0x3F) + 0x20);
		FS_Print (f, temp);
		FS_Close (f);
		}

	// Достижения, зависящие от уровня
	if (Mode == 0)
		{
		f = FS_Open (ACHI_SCRIPT_A, "w", false);
		if (f)
			{
			if (WAS_Level > 0)
				FS_Print (f, "bind \"6\" \"impulse 211\"\n");

			if (WAS_Level > 1)
				FS_Print (f, "bind \"7\" \"impulse 219\"\n");

			if (WAS_Level > 2)
				FS_Print (f, "bind \"8\" \"impulse 228\"\n");

			FS_Close (f);

			Cbuf_AddText (ACHI_EXEC_LINE_A);
			Cbuf_Execute ();
			}
		}

	// Гравитация
	if (Mode == 1)
		{
		f = FS_Open (ACHI_SCRIPT_G, "w", false);
		if (f)
			{
			Q_snprintf (temp, sizeof (temp), "sv_gravity \"%u\"\n", gravity);
			FS_Print (f, temp);

			FS_Close (f);

			Cbuf_AddText (ACHI_EXEC_LINE_G);
			Cbuf_Execute ();
			}

		WAS_Code_Gr = (gravity >> 4) & 0x3F;
		}

	// Тип помещения
	if (Mode == 2)
		{
		f = FS_Open (ACHI_SCRIPT_R, "w", false);
		if (f)
			{
			Q_snprintf (temp, sizeof (temp), "room_type \"%u\"\n", roomtype);
			FS_Print (f, temp);

			FS_Close (f);

			Cbuf_AddText (ACHI_EXEC_LINE_R);
			Cbuf_Execute ();
			}

		WAS_Code_Rt = roomtype & 0x3F;
		}

	WAS_entered = 0;
	return;
	}
