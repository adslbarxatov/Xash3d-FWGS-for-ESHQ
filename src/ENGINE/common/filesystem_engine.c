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

// [FWGS, 01.06.25]
#if XASH_SDL == 2
	#include <SDL.h>	// SDL_GetBasePath
#elif XASH_SDL == 3
	#include <SDL3/SDL.h>
#endif

#include <errno.h>		// [FWGS, 01.07.24]
#include "common.h"
#include "library.h"
#include "server.h"		// ESHQ
#include "platform/platform.h"

// [FWGS, 01.11.25]
/*CVAR_DEFINE_AUTO (fs_mount_hd, "0", FCVAR_ARCHIVE | FCVAR_PRIVILEGED | FCVAR_LATCH,
	"mount high definition content folder");
CVAR_DEFINE_AUTO (fs_mount_lv, "0", FCVAR_ARCHIVE | FCVAR_PRIVILEGED | FCVAR_LATCH,
	"mount low violence models content folder");
CVAR_DEFINE_AUTO (fs_mount_addon, "0", FCVAR_ARCHIVE | FCVAR_PRIVILEGED | FCVAR_LATCH,
	"mount addon content folder");
CVAR_DEFINE_AUTO (fs_mount_l10n, "0", FCVAR_ARCHIVE | FCVAR_PRIVILEGED | FCVAR_LATCH,
	"mount localization content folder");
CVAR_DEFINE_AUTO (ui_language, "english", FCVAR_ARCHIVE | FCVAR_PRIVILEGED | FCVAR_LATCH,
	"selected game language");*/
#if XASH_WIN32
#include <direct.h>
#endif

// [FWGS, 01.11.25]
static CVAR_DEFINE_AUTO (fs_mount_hd, "0", FCVAR_PRIVILEGED,
	"mount high definition content folder");
static CVAR_DEFINE_AUTO (fs_mount_lv, "0", FCVAR_PRIVILEGED,
	"mount low violence models content folder");
static CVAR_DEFINE_AUTO (fs_mount_addon, "0", FCVAR_PRIVILEGED,
	"mount addon content folder");
static CVAR_DEFINE_AUTO (fs_mount_l10n, "0", FCVAR_PRIVILEGED,
	"mount localization content folder");
static CVAR_DEFINE_AUTO (ui_language, "english", FCVAR_PRIVILEGED,
	"selected game language");

fs_api_t		g_fsapi;
fs_globals_t	*FI;

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
	if ((len > 0) && (pname[len - 1] == '/'))
		pname[len - 1] = 0;
	}

void *FS_GetNativeObject (const char *obj)
	{
	if (fs_pfnCreateInterface)
		return fs_pfnCreateInterface (obj, NULL);

	return NULL;
	}

// [FWGS, 01.11.25]
/*void FS_Rescan_f (void)*/
static uint32_t FS_MountFlags (void)
	{
	uint32_t flags = 0;

	// FIXME: VFS shouldn't care about this, allow engine to mount gamedirs
	if (fs_mount_lv.value)
		SetBits (flags, FS_MOUNT_LV);
	if (fs_mount_hd.value)
		SetBits (flags, FS_MOUNT_HD);
	if (fs_mount_addon.value)
		SetBits (flags, FS_MOUNT_ADDON);
	if (fs_mount_l10n.value)
		SetBits (flags, FS_MOUNT_L10N);

	/*g_fsapi.Rescan (flags, ui_language.string);*/
	return flags;
	}

// [FWGS, 01.11.25]
void FS_Rescan_f (void)
	{
	g_fsapi.Rescan (FS_MountFlags (), ui_language.string);
	}

// [FWGS, 01.11.25]
static void FS_LoadVFSConfig (const char *gamedir)
	{
	string parm;

	if (Host_IsDedicated ())
		return;

	Cbuf_AddTextf ("exec %s/vfs.cfg\n", gamedir);
	Cbuf_Execute ();

	if (Sys_GetParmFromCmdLine ("-language", parm))
		{
		Cvar_DirectSet (&ui_language, parm);
		Cvar_DirectSet (&fs_mount_l10n, "1");
		}

	/*ClearBits (fs_mount_lv.flags, FCVAR_CHANGED);*/
	ClearBits (fs_mount_hd.flags, FCVAR_CHANGED);
	ClearBits (fs_mount_lv.flags, FCVAR_CHANGED);
	ClearBits (fs_mount_l10n.flags, FCVAR_CHANGED);

	ClearBits (fs_mount_addon.flags, FCVAR_CHANGED);
	ClearBits (ui_language.flags, FCVAR_CHANGED);
	}

// [FWGS, 01.11.25]
void FS_SaveVFSConfig (void)
	{
	file_t *f;

	if (!FBitSet (fs_mount_hd.flags | fs_mount_lv.flags | fs_mount_l10n.flags | fs_mount_addon.flags | ui_language.flags, FCVAR_CHANGED))
		{
		Con_Reportf ("%s: no need to save vfs.cfg\n", __func__);
		return;
		}

	Con_Printf ("%s()\n", __func__);

	f = FS_Open ("vfs.cfg.new", "w", true);
	if (!f)
		{
		Con_Printf (S_ERROR "%s: couldn't open vfs.cfg for write\n", __func__);
		return;
		}

	FS_Printf (f, "%s \"%d\"\n", fs_mount_hd.name, (int)fs_mount_hd.value);
	FS_Printf (f, "%s \"%d\"\n", fs_mount_lv.name, (int)fs_mount_lv.value);
	FS_Printf (f, "%s \"%d\"\n", fs_mount_l10n.name, (int)fs_mount_l10n.value);
	FS_Printf (f, "%s \"%d\"\n", fs_mount_addon.name, (int)fs_mount_addon.value);
	FS_Printf (f, "%s \"%s\"\n", ui_language.name, ui_language.string);

	Host_FinalizeConfig (f, "vfs.cfg");

	ClearBits (fs_mount_hd.flags, FCVAR_CHANGED);
	ClearBits (fs_mount_lv.flags, FCVAR_CHANGED);
	ClearBits (fs_mount_l10n.flags, FCVAR_CHANGED);
	ClearBits (fs_mount_addon.flags, FCVAR_CHANGED);
	ClearBits (ui_language.flags, FCVAR_CHANGED);
	}

// [FWGS, 01.11.25]
void FS_LoadGameInfo (void)
	{
	FS_LoadVFSConfig (g_fsapi.Gamedir ());

	g_fsapi.LoadGameInfo (FS_MountFlags (), ui_language.string);
	}

static void FS_ClearPaths_f (void)
	{
	FS_ClearSearchPath ();
	}

static void FS_Path_f_ (void)
	{
	FS_Path_f ();
	}

// [FWGS, 01.02.25]
static void FS_MakeGameInfo_f (void)
	{
	g_fsapi.MakeGameInfo ();
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

static void FS_UnloadProgs (void)
	{
	if (fs_hInstance)
		{
		COM_FreeLibrary (fs_hInstance);
		fs_hInstance = 0;
		}
	}

// [FWGS, 01.09.25]
#ifdef XASH_INTERNAL_GAMELIBS
#define FILESYSTEM_STDIO_DLL "filesystem_stdio"
#elif XASH_ANDROID
#define FILESYSTEM_STDIO_DLL "libfilesystem_stdio.so"
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

static qboolean FS_DetermineRootDirectory (char *out, size_t size)
	{
	const char *path = getenv ("XASH3D_BASEDIR");

	if (COM_CheckString (path))
		{
		Q_strncpy (out, path, size);
		return true;
		}

	// [FWGS, 01.11.25]
/*if XASH_EMSCRIPTEN
	Q_strncpy (out, "/rwdir", size);
	return true;

elif TARGET_OS_IOS*/
#if TARGET_OS_IOS
	Q_strncpy (out, IOS_GetDocsDir (), size);
	return true;

/*elif XASH_ANDROID && XASH_SDL

	path = SDL_AndroidGetExternalStoragePath ();
	if (path != NULL)
		{
		Q_strncpy (out, path, size);
		return true;
		}
	Sys_Error ("couldn't determine Android external storage path: %s", SDL_GetError ());
	return false;*/

#elif XASH_PSVITA

	if (PSVita_GetBasePath (out, size))
		return true;
	Sys_Error ("couldn't find %s data directory", XASH_ENGINE_NAME);

	return false;

#elif ( XASH_SDL >= 2 ) && !XASH_NSWITCH // GetBasePath not impl'd in switch-sdl2

	path = SDL_GetBasePath ();

#if XASH_APPLE
	if (path != NULL && Q_stristr (path, ".app"))
		{
		SDL_free ((void *)path);
		path = SDL_GetPrefPath (NULL, XASH_ENGINE_NAME);
		}
#endif

	// [FWGS, 01.02.25]
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

// [FWGS, 01.11.25]
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

	/*if XASH_EMSCRIPTEN
	Q_strncpy (out, "/rodir", size);
	return true;
endif*/

	return false;
	}

// [FWGS, 01.11.25]
/*void FS_CheckConfig (void)
	{
	if (fs_mount_lv.value || fs_mount_hd.value || fs_mount_addon.value || fs_mount_l10n.value)
		FS_Rescan_f ();
	}*/

/***
================
FS_Init
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

	// [FWGS, 01.02.25]
	Cmd_AddRestrictedCommand ("fs_rescan", FS_Rescan_f, "rescan filesystem search pathes");
	Cmd_AddRestrictedCommand ("fs_path", FS_Path_f_, "show filesystem search pathes");
	Cmd_AddRestrictedCommand ("fs_clearpaths", FS_ClearPaths_f, "clear filesystem search pathes");
	Cmd_AddRestrictedCommand ("fs_make_gameinfo", FS_MakeGameInfo_f, "create gameinfo.txt for current running game");

	// [FWGS, 01.03.25]
	Cvar_RegisterVariable (&fs_mount_hd);
	Cvar_RegisterVariable (&fs_mount_lv);
	Cvar_RegisterVariable (&fs_mount_addon);
	Cvar_RegisterVariable (&fs_mount_l10n);

	// [FWGS, 01.09.25]
	Cvar_RegisterVariable (&ui_language);

	if (!Sys_GetParmFromCmdLine ("-dll", host.gamedll))
		host.gamedll[0] = 0;

	if (!Sys_GetParmFromCmdLine ("-clientlib", host.clientlib))
		host.clientlib[0] = 0;

	// [FWGS, 01.05.25]
	if (!Sys_GetParmFromCmdLine ("-menulib", host.menulib))
		host.menulib[0] = 0;
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
