/*
filesystem.c - game filesystem based on DP fs
Copyright (C) 2007 Uncle Mike

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.
*/

#include "common.h"
#include "library.h"
#include "server.h"

fs_api_t g_fsapi;
fs_globals_t *FI;

static HINSTANCE fs_hInstance;

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

static fs_interface_t fs_memfuncs =
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
	};

static void FS_UnloadProgs (void)
	{
	COM_FreeLibrary (fs_hInstance);
	fs_hInstance = 0;
	}

#ifdef XASH_INTERNAL_GAMELIBS
#define FILESYSTEM_STDIO_DLL "filesystem_stdio"
#else
// ESHQ: другое название DLL
#define FILESYSTEM_STDIO_DLL "FS." OS_LIB_EXT
#endif

qboolean FS_LoadProgs (void)
	{
	const char *name = FILESYSTEM_STDIO_DLL;
	FSAPI GetFSAPI;

	fs_hInstance = COM_LoadLibrary (name, false, true);

	if (!fs_hInstance)
		{
		Host_Error ("FS_LoadProgs: can't load filesystem library %s: %s\n", name, COM_GetLibraryError ());
		return false;
		}

	if (!(GetFSAPI = (FSAPI)COM_GetProcAddress (fs_hInstance, GET_FS_API)))
		{
		FS_UnloadProgs ();
		Host_Error ("FS_LoadProgs: can't find GetFSAPI entry point in %s\n", name);
		return false;
		}

	if (!GetFSAPI (FS_API_VERSION, &g_fsapi, &FI, &fs_memfuncs))
		{
		FS_UnloadProgs ();
		Host_Error ("FS_LoadProgs: can't initialize filesystem API: wrong version\n");
		return false;
		}

	Con_DPrintf ("FS_LoadProgs: filesystem_stdio successfully loaded\n");

	return true;
	}

/*
================
FS_Init
================
*/
void FS_Init (void)
	{
	qboolean		hasBaseDir = false;
	qboolean		hasGameDir = false;
	qboolean		caseinsensitive = true;
	int		i;
	string gamedir;

	Cmd_AddRestrictedCommand ("fs_rescan", FS_Rescan_f, "rescan filesystem search pathes");
	Cmd_AddRestrictedCommand ("fs_path", FS_Path_f_, "show filesystem search pathes");
	Cmd_AddRestrictedCommand ("fs_clearpaths", FS_ClearPaths_f, "clear filesystem search pathes");

#if !XASH_WIN32
	if (Sys_CheckParm ("-casesensitive"))
		caseinsensitive = false;
#endif

	if (!Sys_GetParmFromCmdLine ("-game", gamedir))
		Q_strncpy (gamedir, SI.basedirName, sizeof (gamedir)); // gamedir == basedir

	if (!FS_InitStdio (caseinsensitive, host.rootdir, SI.basedirName, gamedir, host.rodir))
		{
		Host_Error ("Can't init filesystem_stdio!\n");
		return;
		}

	if (!Sys_GetParmFromCmdLine ("-dll", SI.gamedll))
		SI.gamedll[0] = 0;

	if (!Sys_GetParmFromCmdLine ("-clientlib", SI.clientlib))
		SI.clientlib[0] = 0;
	}

/*
================
FS_Shutdown
================
*/
void FS_Shutdown (void)
	{
	int	i;

	FS_ShutdownStdio ();

	memset (&SI, 0, sizeof (sysinfo_t));

	FS_UnloadProgs ();
	}

/*
================
ESHQ: поддержка достижений
================
*/
/*#define ACHI_OLD_SCRIPT_FN	"achi.cfg"*/
#define ACHI_SCRIPT_FN		"achi2.cfg"
#define ACHI_EXEC_STRING	"exec " ACHI_SCRIPT_FN "\n"

/*qboolean FS_UpdateAchievementsScript (void)
	{
	// Переменные
	file_t *f;
	unsigned int level = 0;
	char temp[16];

	// Чтение предыдущего состояния (ошибки игнорируются)
	f = FS_Open (ACHI_OLD_SCRIPT_FN, "r", false);
	if (f)
		{
		FS_Getc (f); FS_Getc (f); FS_Getc (f);
		level = ((unsigned int)FS_Getc (f)) & 0x0F;
		FS_Close (f);
		}
	else
		{
		return true;
		}

	if (level < 1)
		return true;

	// Запись
	f = FS_Open (ACHI_SCRIPT_FN, "w", false);
	if (!f)
		return false;

	Q_sprintf (temp, "//%c\\\\\n", level + 0x70);
	FS_Print (f, temp);
	FS_Print (f, "bind \"6\" \"impulse 211\"\n");

	if (level > 0)
		FS_Print (f, "bind \"7\" \"impulse 219\"\n");

	if (level > 1)
		FS_Print (f, "bind \"8\" \"impulse 228\"\n");

	// Завершено
	FS_Close (f);
	FS_Delete (ACHI_OLD_SCRIPT_FN);
	return true;
	}*/

// ESHQ: состояние теперь хранится в памяти, чтобы постоянно не дёргать файл
static unsigned int WAS_Level = 0, WAS_Code = 0;
qboolean FS_WriteAchievementsScript (int NewLevel)
	{
	// Переменные
	file_t *f;
	char temp[32];

	convar_t *rt = Cvar_FindVar ("room_type");
	unsigned int gravity = (unsigned int)sv_gravity.value;
	unsigned int roomtype = 0;
	unsigned int newCode;

	if (rt)
		roomtype = (unsigned int)rt->value;

	// Чтение предыдущего состояния (ошибки игнорируются)
	if (!WAS_Code)
		{
		f = FS_Open (ACHI_SCRIPT_FN, "r", false);
		if (f)
			{
			FS_Getc (f); FS_Getc (f);
			WAS_Level = ((unsigned int)FS_Getc (f)) & 0x0F;
			WAS_Code = ((unsigned int)FS_Getc (f) - 0x20) & 0x3F;
			WAS_Code |= (((unsigned int)FS_Getc (f) - 0x20) & 0x3F) << 8;
			FS_Close (f);
			}
		}

	// Проверочный код, позволяющий избежать постоянной перезаписи файла и повторного его исполнения
	newCode = ((gravity >> 4) & 0x3F) | ((roomtype & 0x3F) << 8);
	if (((NewLevel <= (int)WAS_Level) || (WAS_Level >= 3)) && (WAS_Code == newCode))
		return true;	// Уровень уже достигнут или является максимальным
	WAS_Code = newCode;

	// Условие для последующего повышения
	// int, потому что иначепри сравнении происходит приведение к uint
	if ((NewLevel > (int)WAS_Level) && (WAS_Level < 3))
		WAS_Level++;

	// Запись
	f = FS_Open (ACHI_SCRIPT_FN, "w", false);
	if (!f)
		return false;

	// Код проверки скрипта
	Q_sprintf (temp, "//%c%c%c\\\\\n", WAS_Level + 0x70, ((gravity >> 4) & 0x3F) + 0x20,
		(roomtype & 0x3F) + 0x20);
	FS_Print (f, temp);

	// Достижения, зависящие от уровня
	if (WAS_Level > 0)
		FS_Print (f, "bind \"6\" \"impulse 211\"\n");

	if (WAS_Level > 1)
		FS_Print (f, "bind \"7\" \"impulse 219\"\n");

	if (WAS_Level > 2)
		FS_Print (f, "bind \"8\" \"impulse 228\"\n");

	// Независимые параметры
	Q_sprintf (temp, "sv_gravity \"%u\"\n", gravity);
	FS_Print (f, temp);

	Q_sprintf (temp, "room_type \"%u\"\n", roomtype);
	FS_Print (f, temp);

	// Завершено. Принудительное выполнение
	FS_Close (f);

	Cbuf_AddText (ACHI_EXEC_STRING);
	Cbuf_Execute ();

	return true;
	}
