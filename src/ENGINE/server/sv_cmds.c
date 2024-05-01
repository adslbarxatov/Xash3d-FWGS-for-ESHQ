/*
sv_cmds.c - server console commands
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
#include "server.h"
#include <shellapi.h>	// ESHQ: ��������� ������ ���������� ���� ESRM

extern convar_t con_gamemaps;

/*
=================
SV_ClientPrintf

Sends text across to be displayed if the level passes
=================
*/
void SV_ClientPrintf (sv_client_t *cl, const char *fmt, ...)
	{
	char	string[MAX_SYSPATH];
	va_list	argptr;

	if (FBitSet (cl->flags, FCL_FAKECLIENT))
		return;

	va_start (argptr, fmt);
	Q_vsnprintf (string, sizeof (string), fmt, argptr);
	va_end (argptr);

	MSG_BeginServerCmd (&cl->netchan.message, svc_print);
	MSG_WriteString (&cl->netchan.message, string);
	}

/*
=================
SV_BroadcastPrintf

Sends text to all active clients
=================
*/
void SV_BroadcastPrintf (sv_client_t *ignore, const char *fmt, ...)
	{
	char		string[MAX_SYSPATH];
	va_list		argptr;
	sv_client_t *cl;
	int			i;

	va_start (argptr, fmt);
	Q_vsnprintf (string, sizeof (string), fmt, argptr);
	va_end (argptr);

	if (sv.state == ss_active)
		{
		for (i = 0, cl = svs.clients; i < svs.maxclients; i++, cl++)
			{
			if (FBitSet (cl->flags, FCL_FAKECLIENT))
				continue;

			if ((cl == ignore) || (cl->state != cs_spawned))
				continue;

			MSG_BeginServerCmd (&cl->netchan.message, svc_print);
			MSG_WriteString (&cl->netchan.message, string);
			}
		}

	if (Host_IsDedicated ())
		{
		// echo to console
		Con_DPrintf ("%s", string);
		}
	}

/*
=================
SV_BroadcastCommand

Sends text to all active clients
=================
*/
void SV_BroadcastCommand (const char *fmt, ...)
	{
	char	string[MAX_SYSPATH];
	va_list	argptr;

	if (sv.state == ss_dead)
		return;

	va_start (argptr, fmt);
	Q_vsnprintf (string, sizeof (string), fmt, argptr);
	va_end (argptr);

	MSG_BeginServerCmd (&sv.reliable_datagram, svc_stufftext);
	MSG_WriteString (&sv.reliable_datagram, string);
	}

/*
==================
SV_SetPlayer

Sets sv_client and sv_player to the player with idnum Cmd_Argv(1)
==================
*/
static sv_client_t *SV_SetPlayer (void)
	{
	const char *s;
	sv_client_t *cl;
	int		i, idnum;

	if (!svs.clients || sv.background)
		return NULL;

	if ((svs.maxclients == 1) || (Cmd_Argc () < 2))
		{
		// special case for local client
		return svs.clients;
		}

	s = Cmd_Argv (1);

	// numeric values are just slot numbers
	if (Q_isdigit (s) || ((s[0] == '-') && Q_isdigit (s + 1)))
		{
		idnum = Q_atoi (s);

		if ((idnum < 0) || (idnum >= svs.maxclients))
			{
			Con_Printf ("Bad client slot: %i\n", idnum);
			return NULL;
			}

		cl = &svs.clients[idnum];

		if (!cl->state)
			{
			Con_Printf ("Client %i is not active\n", idnum);
			return NULL;
			}
		return cl;
		}

	// check for a name match
	for (i = 0, cl = svs.clients; i < svs.maxclients; i++, cl++)
		{
		if (!cl->state) continue;

		if (!Q_strcmp (cl->name, s))
			return cl;
		}

	Con_Printf ("Userid %s is not on the server\n", s);
	return NULL;
	}

/*
==================
SV_ValidateMap

check map for typically errors
==================
*/
static qboolean SV_ValidateMap (const char *pMapName, qboolean check_spawn)
	{
	char	*spawn_entity;
	int		flags;

	// determine spawn entity classname
	if (!check_spawn || ((int)sv_maxclients.value <= 1))
		spawn_entity = GI->sp_entity;
	else
		spawn_entity = GI->mp_entity;

	flags = SV_MapIsValid (pMapName, spawn_entity, NULL);

	if (FBitSet (flags, MAP_INVALID_VERSION))
		{
		Con_Printf (S_ERROR "map %s is invalid or not supported\n", pMapName);
		return false;
		}

	if (!FBitSet (flags, MAP_IS_EXIST))
		{
		Con_Printf (S_ERROR "map %s doesn't exist\n", pMapName);
		return false;
		}

	if (check_spawn && !FBitSet (flags, MAP_HAS_SPAWNPOINT))
		{
		Con_Printf (S_ERROR "map %s doesn't have a valid spawnpoint\n", pMapName);
		return false;
		}

	return true;
	}

/*
==================
SV_ESRM_Command

ESHQ: ���������� ��������������� ����������� ������ ��� ���� ESRM
==================
*/
void SV_ESRM_Command (void)
	{
	char cmdLine[MAX_QPATH];

	// �������� ������������ ������ (�� ���������, �.�. ��� ������� �� ����� ������� �� ������ �����)
	//if (!strstr (host.gamefolder, "esrm"))
	//return;

	char rebuild = (strstr (Cmd_Argv (0), "esrm_rebuild") != NULL);
	if ((Cmd_Argc () != 2) && !rebuild)
		{
		Con_Printf (S_USAGE "%s <value>\n", Cmd_Argv (0));
		return;
		}

	// ������ ������� ��� ����������
	if (rebuild)
		Q_strncpy (cmdLine, "-r", MAX_QPATH);
	else
		Q_strncpy (cmdLine, "-s ", MAX_QPATH);

	if (strstr (Cmd_Argv (0), "esrm_size"))
		Q_strncat (cmdLine, "MS ", MAX_QPATH);

	else if (strstr (Cmd_Argv (0), "esrm_enemies_list"))
		Q_strncat (cmdLine, "EP ", MAX_QPATH);
	else if (strstr (Cmd_Argv (0), "esrm_items_list"))
		Q_strncat (cmdLine, "IP ", MAX_QPATH);

	else if (strstr (Cmd_Argv (0), "esrm_enemies"))
		Q_strncat (cmdLine, "DF ", MAX_QPATH);
	else if (strstr (Cmd_Argv (0), "esrm_items"))
		Q_strncat (cmdLine, "ID ", MAX_QPATH);

	else if (strstr (Cmd_Argv (0), "esrm_walls"))
		Q_strncat (cmdLine, "WD ", MAX_QPATH);
	else if (strstr (Cmd_Argv (0), "esrm_light"))
		Q_strncat (cmdLine, "LG ", MAX_QPATH);
	else if (strstr (Cmd_Argv (0), "esrm_crates"))
		Q_strncat (cmdLine, "CD ", MAX_QPATH);
	else if (strstr (Cmd_Argv (0), "esrm_gravity"))
		Q_strncat (cmdLine, "GR ", MAX_QPATH);

	else if (strstr (Cmd_Argv (0), "esrm_button"))
		Q_strncat (cmdLine, "BM ", MAX_QPATH);
	else if (strstr (Cmd_Argv (0), "esrm_sections"))
		Q_strncat (cmdLine, "ST ", MAX_QPATH);
	else if (strstr (Cmd_Argv (0), "esrm_two_floors"))
		Q_strncat (cmdLine, "TF ", MAX_QPATH);
	else if (strstr (Cmd_Argv (0), "esrm_expl_crates"))
		Q_strncat (cmdLine, "XC ", MAX_QPATH);
	else if (strstr (Cmd_Argv (0), "esrm_item_crates"))
		Q_strncat (cmdLine, "IC ", MAX_QPATH);
	else if (strstr (Cmd_Argv (0), "esrm_makers"))
		Q_strncat (cmdLine, "MM ", MAX_QPATH);
	else if (strstr (Cmd_Argv (0), "esrm_barriers"))
		Q_strncat (cmdLine, "BT ", MAX_QPATH);
	else if (strstr (Cmd_Argv (0), "esrm_fog"))
		Q_strncat (cmdLine, "FC ", MAX_QPATH);
	else if (strstr (Cmd_Argv (0), "esrm_items_on_2nd_floor"))
		Q_strncat (cmdLine, "SF ", MAX_QPATH);

	else if (!rebuild)
		return;

	if (!rebuild)
		Q_strncat (cmdLine, Cmd_Argv (1), MAX_QPATH);

	// ������������ ��������, �.�. ��� �������� ����� ������� ����� ���������� �� ������������ ����� �����,
	// ���� ���������� �������� ������
	else
		Cbuf_AddText ("echo Rebuilding...; wait; save quick");

	ShellExecute (NULL, "open", "esrm\\randomaze.exe", cmdLine, NULL, SW_SHOWMINNOACTIVE);
	}

/*
==================
SV_Map_f

Goes directly to a given map without any savegame archiving.
For development work
==================
*/
static void SV_Map_f (void)
	{
	char	mapname[MAX_QPATH];

	if (Cmd_Argc () != 2)
		{
		Con_Printf (S_USAGE "map <mapname>\n");
		return;
		}

	// hold mapname to other place
	Q_strncpy (mapname, Cmd_Argv (1), sizeof (mapname));
	COM_StripExtension (mapname);

	if (!SV_ValidateMap (mapname, true))
		return;

	Cvar_DirectSet (&sv_hostmap, mapname);
	COM_LoadLevel (mapname, false);
	}

/*
==================
SV_Maps_f

Lists maps according to given substring
==================
*/
static void SV_Maps_f (void)
	{
	const char	*separator = "-------------------";
	const char	*argStr = Cmd_Argv (1); // Substr
	int			nummaps;
	search_t	*mapList;

	if (Cmd_Argc () != 2)
		{
		// [FWGS, 01.04.23]
		Msg (S_USAGE "maps <substring>\nmaps * for full listing\n");
		return;
		}

	mapList = FS_Search (va ("maps/*%s*.bsp", argStr), true, true);
	if (!mapList)
		{
		Msg ("No related map found in \"%s/maps\"\n", GI->gamefolder);
		return;
		}

	nummaps = Cmd_ListMaps (mapList, NULL, 0);
	Mem_Free (mapList);

	Msg ("%s\nDirectory: \"%s/maps\" - Maps listed: %d\n", separator, GI->gamefolder, nummaps);
	}

/*
==================
SV_MapBackground_f

Set background map (enable physics in menu)
==================
*/
static void SV_MapBackground_f (void)
	{
	char	mapname[MAX_QPATH];

	if (Cmd_Argc () != 2)
		{
		Con_Printf (S_USAGE "map_background <mapname>\n");
		return;
		}

	if (SV_Active () && !sv.background)
		{
		if (GameState->nextstate == STATE_RUNFRAME)
			Con_Printf (S_ERROR "can't set background map while game is active\n");
		return;
		}

	// hold mapname to other place
	Q_strncpy (mapname, Cmd_Argv (1), sizeof (mapname));
	COM_StripExtension (mapname);

	if (!SV_ValidateMap (mapname, false))
		return;

	// background map is always run as singleplayer
	Cvar_FullSet ("maxplayers", "1", FCVAR_LATCH);
	Cvar_FullSet ("deathmatch", "0", FCVAR_LATCH);
	Cvar_FullSet ("coop", "0", FCVAR_LATCH);

	COM_LoadLevel (mapname, true);
	}

/*
==================
SV_NextMap_f

Change map for next in alpha-bethical ordering
For development work
==================
*/
static void SV_NextMap_f (void)
	{
	char		nextmap[MAX_QPATH];
	int			i, next;
	search_t	*t;

	// [FWGS, 01.07.23]
	t = FS_Search ("maps\\*.bsp", true, con_gamemaps.value);	// only in gamedir
	if (!t)
		t = FS_Search ("maps/*.bsp", true, con_gamemaps.value);	// only in gamedir

	if (!t)
		{
		Con_Printf ("next map can't be found\n");
		return;
		}

	for (i = 0; i < t->numfilenames; i++)
		{
		const char *ext = COM_FileExtension (t->filenames[i]);

		if (Q_stricmp (ext, "bsp"))
			continue;

		COM_FileBase (t->filenames[i], nextmap, sizeof (nextmap));	// [FWGS, 01.05.23]
		if (Q_stricmp (sv_hostmap.string, nextmap))
			continue;

		next = (i + 1) % t->numfilenames;
		COM_FileBase (t->filenames[next], nextmap, sizeof (nextmap));	// [FWGS, 01.05.23]
		Cvar_DirectSet (&sv_hostmap, nextmap);

		// found current point, check for valid
		if (SV_ValidateMap (nextmap, true))
			{
			// found and valid
			COM_LoadLevel (nextmap, false);
			Mem_Free (t);
			return;
			}
		// jump to next map
		}

	Con_Printf ("failed to load next map\n");
	Mem_Free (t);
	}

/*
==============
SV_NewGame_f [FWGS, 01.03.24]
==============
*/
static void SV_NewGame_f (void)
	{
	if (Cmd_Argc () == 1)
		COM_NewGame (GI->startmap);
	else if (Cmd_Argc () == 2)
		COM_NewGame (Cmd_Argv (1));
	else
		Con_Printf (S_USAGE "newgame\n");
	}

/*
==============
SV_HazardCourse_f
==============
*/
static void SV_HazardCourse_f (void)
	{
	if (Cmd_Argc () != 1)
		{
		Con_Printf (S_USAGE "hazardcourse\n");
		return;
		}

	// special case for Gunman Chronicles: playing avi-file
	if (FS_FileExists (va ("media/%s.avi", GI->trainmap), false))
		{
		// [FWGS, 01.04.23]
		Cbuf_AddTextf ("wait; movie %s\n", GI->trainmap);
		Host_EndGame (true, DEFAULT_ENDGAME_MESSAGE);
		}
	else
		{
		COM_NewGame (GI->trainmap);
		}
	}

/*
==============
ESHQ: ��������� ��� ��������� ������
SV_Credits_f
==============
*/
void SV_Credits_f (void)
	{
	if (Cmd_Argc () != 1)
		{
		Msg ("Usage: credits\n");
		return;
		}

	COM_NewGame (GI->creditsmap);
	}

/*
==============
SV_Load_f
==============
*/
static void SV_Load_f (void)
	{
	char	path[MAX_QPATH];

	if (Cmd_Argc () != 2)
		{
		Con_Printf (S_USAGE "load <savename>\n");
		return;
		}

	Q_snprintf (path, sizeof (path), DEFAULT_SAVE_DIRECTORY "%s.%s", Cmd_Argv (1), DEFAULT_SAVE_EXTENSION);
	SV_LoadGame (path);
	}

/*
==============
SV_QuickLoad_f
==============
*/
static void SV_QuickLoad_f (void)
	{
	Cbuf_AddText ("echo Quick Loading...; wait; load quick");
	}

/*
==============
SV_Save_f [FWGS, 01.04.23]
==============
*/
static void SV_Save_f (void)
	{
	qboolean ret = false;

	switch (Cmd_Argc ())
		{
		case 1:
			ret = SV_SaveGame ("new");
			break;

		case 2:
			ret = SV_SaveGame (Cmd_Argv (1));
			break;

		default:
			Con_Printf (S_USAGE "save <savename>\n");
			break;
		}

	if (ret && CL_Active () && !FBitSet (host.features, ENGINE_QUAKE_COMPATIBLE))
		CL_HudMessage ("GAMESAVED"); // defined in titles.txt
	}

/*
==============
SV_QuickSave_f
==============
*/
static void SV_QuickSave_f (void)
	{
	Cbuf_AddText ("echo Quick Saving...; wait; save quick");
	}

/*
==============
SV_DeleteSave_f
==============
*/
static void SV_DeleteSave_f (void)
	{
	if (Cmd_Argc () != 2)
		{
		Con_Printf (S_USAGE "killsave <name>\n");
		return;
		}

	// delete save and saveshot
	FS_Delete (va (DEFAULT_SAVE_DIRECTORY "%s.%s", Cmd_Argv (1), DEFAULT_SAVE_EXTENSION));
	FS_Delete (va (DEFAULT_SAVE_DIRECTORY "%s.bmp", Cmd_Argv (1)));
	}

/*
==============
SV_AutoSave_f
==============
*/
static void SV_AutoSave_f (void)
	{
	if (Cmd_Argc () != 1)
		{
		Con_Printf (S_USAGE "autosave\n");
		return;
		}

	if (Cvar_VariableInteger ("sv_autosave"))	// [FWGS, 01.04.23]
		SV_SaveGame ("autosave");

	// ESHQ
	if (CL_Active () && !FBitSet (host.features, ENGINE_QUAKE_COMPATIBLE))
		CL_HudMessage ("GAMESAVED"); // defined in titles.txt
	}

/*
==================
SV_Restart_f

restarts current level
==================
*/
static void SV_Restart_f (void)
	{
	// because restart can be multiple issued
	if (sv.state != ss_active)
		return;

	COM_LoadLevel (sv.name, sv.background);
	}

/*
==================
SV_Reload_f

continue from latest savedgame
==================
*/
static void SV_Reload_f (void)
	{
	// because reload can be multiple issued
	if (GameState->nextstate != STATE_RUNFRAME)
		return;

	if (!SV_LoadGame (SV_GetLatestSave ()))
		COM_LoadLevel (sv_hostmap.string, false);
	}

/*
==================
SV_ChangeLevel_f

classic change level
==================
*/
static void SV_ChangeLevel_f (void)
	{
	if (Cmd_Argc () != 2)
		{
		Con_Printf (S_USAGE "changelevel <mapname>\n");
		return;
		}

	SV_QueueChangeLevel (Cmd_Argv (1), NULL);
	}

/*
==================
SV_ChangeLevel2_f

smooth change level
==================
*/
static void SV_ChangeLevel2_f (void)
	{
	if (Cmd_Argc () != 3)
		{
		Con_Printf (S_USAGE "changelevel2 <mapname> <landmark>\n");
		return;
		}

	SV_QueueChangeLevel (Cmd_Argv (1), Cmd_Argv (2));
	}

/*
==================
SV_Kick_f [FWGS, 01.07.23]

Kick a user off of the server
==================
*/
static void SV_Kick_f (void)
	{
	sv_client_t		*cl;
	const char		*param;

	if (Cmd_Argc () != 2)
		{
		Con_Printf (S_USAGE "kick <#id|name> [reason]\n");
		return;
		}

	param = Cmd_Argv (1);

	if ((*param == '#') && Q_isdigit (param + 1))
		cl = SV_ClientById (Q_atoi (param + 1));
	else
		cl = SV_ClientByName (param);

	if (!cl)
		{
		Con_Printf ("Client is not on the server\n");
		return;
		}

	SV_KickPlayer (cl, "%s", Cmd_Argv (2));
	}

/*
==================
SV_EntPatch_f
==================
*/
static void SV_EntPatch_f (void)
	{
	const char	*mapname;

	if (Cmd_Argc () < 2)
		{
		if (sv.state != ss_dead)
			{
			mapname = sv.name;
			}
		else
			{
			Con_Printf (S_USAGE "entpatch <mapname>\n");
			return;
			}
		}
	else
		{
		mapname = Cmd_Argv (1);
		}

	SV_WriteEntityPatch (mapname);
	}

/*
================
SV_Status_f
================
*/
static void SV_Status_f (void)
	{
	sv_client_t	*cl;
	int			i;

	if (!svs.clients || sv.background)
		{
		Con_Printf ("^3no server running.\n");
		return;
		}

	Con_Printf ("map: %s\n", sv.name);
	Con_Printf ("num score ping    name            lastmsg address               port \n");
	Con_Printf ("--- ----- ------- --------------- ------- --------------------- ------\n");

	for (i = 0, cl = svs.clients; i < svs.maxclients; i++, cl++)
		{
		int	j, l;
		const char *s;

		if (!cl->state) continue;

		Con_Printf ("%3i ", i);
		Con_Printf ("%5i ", (int)cl->edict->v.frags);

		if (cl->state == cs_connected)
			Con_Printf ("Connect");
		else if (cl->state == cs_zombie)
			Con_Printf ("Zombie ");
		else if (FBitSet (cl->flags, FCL_FAKECLIENT))
			Con_Printf ("Bot   ");
		else
			Con_Printf ("%7i ", SV_CalcPing (cl));

		Con_Printf ("%s", cl->name);
		l = 24 - Q_strlen (cl->name);

		for (j = 0; j < l; j++)
			Con_Printf (" ");
		Con_Printf ("%g ", (host.realtime - cl->netchan.last_received));
		s = NET_BaseAdrToString (cl->netchan.remote_address);
		Con_Printf ("%s", s);
		l = 22 - Q_strlen (s);

		for (j = 0; j < l; j++)
			Con_Printf (" ");
		Con_Printf ("%5i", cl->netchan.qport);
		Con_Printf ("\n");
		}

	Con_Printf ("\n");
	}

/*
==================
SV_ConSay_f
==================
*/
static void SV_ConSay_f (void)
	{
	const char	*p;
	char		text[MAX_SYSPATH];

	if (Cmd_Argc () < 2) return;

	if (!svs.clients || sv.background)
		{
		Con_Printf ("^3no server running.\n");
		return;
		}

	p = Cmd_Args ();
	Q_strncpy (text, *p == '"' ? p + 1 : p, MAX_SYSPATH);

	if (*p == '"')
		{
		text[Q_strlen (text) - 1] = 0;
		}

	Log_Printf ("Server say: \"%s\"\n", text);
	Q_snprintf (text, sizeof (text), "%s: %s", Cvar_VariableString ("hostname"), p);
	SV_BroadcastPrintf (NULL, "%s\n", text);
	}

/*
==================
SV_Heartbeat_f [FWGS, 01.05.23]
==================
*/
static void SV_Heartbeat_f (void)
	{
	NET_MasterClear ();
	}

/*
===========
SV_ServerInfo_f

Examine or change the serverinfo string
===========
*/
static void SV_ServerInfo_f (void)
	{
	convar_t	*var;

	if (Cmd_Argc () == 1)
		{
		Con_Printf ("Server info settings:\n");
		Info_Print (svs.serverinfo);
		Con_Printf ("Total %lu symbols\n", Q_strlen (svs.serverinfo));
		return;
		}

	if (Cmd_Argc () != 3)
		{
		Con_Printf (S_USAGE "serverinfo [ <key> <value> ]\n");
		return;
		}

	if (Cmd_Argv (1)[0] == '*')
		{
		Con_Printf ("Star variables cannot be changed.\n");
		return;
		}

	// if this is a cvar, change it too
	var = Cvar_FindVar (Cmd_Argv (1));
	if (var)
		{
		freestring (var->string); // free the old value string
		var->string = copystring (Cmd_Argv (2));
		var->value = Q_atof (var->string);
		}

	Info_SetValueForStarKey (svs.serverinfo, Cmd_Argv (1), Cmd_Argv (2), MAX_SERVERINFO_STRING);
	SV_BroadcastCommand ("fullserverinfo \"%s\"\n", SV_Serverinfo ());
	}

/*
===========
SV_LocalInfo_f

Examine or change the localinfo string
===========
*/
static void SV_LocalInfo_f (void)
	{
	if (Cmd_Argc () == 1)
		{
		Con_Printf ("Local info settings:\n");
		Info_Print (svs.localinfo);
		Con_Printf ("Total %lu symbols\n", Q_strlen (svs.localinfo));
		return;
		}

	if (Cmd_Argc () != 3)
		{
		Con_Printf (S_USAGE "localinfo [ <key> <value> ]\n");
		return;
		}

	if (Cmd_Argv (1)[0] == '*')
		{
		Con_Printf ("Star variables cannot be changed.\n");
		return;
		}

	Info_SetValueForStarKey (svs.localinfo, Cmd_Argv (1), Cmd_Argv (2), MAX_LOCALINFO_STRING);
	}

/*
===========
SV_ClientInfo_f

Examine all a users info strings
===========
*/
static void SV_ClientInfo_f (void)
	{
	sv_client_t	*cl;

	if (Cmd_Argc () != 2)
		{
		Con_Printf (S_USAGE "clientinfo <userid>\n");
		return;
		}

	if ((cl = SV_SetPlayer ()) == NULL)
		return;

	Con_Printf ("userinfo\n");
	Con_Printf ("--------\n");
	Info_Print (cl->userinfo);
	}

/*
===========
SV_ClientUserAgent_f

Examine useragent strings
===========
*/
static void SV_ClientUserAgent_f (void)
	{
	sv_client_t	*cl;

	if (Cmd_Argc () != 2)
		{
		Con_Printf (S_USAGE "clientuseragent <userid>\n");
		return;
		}

	if ((cl = SV_SetPlayer ()) == NULL)
		return;

	Con_Printf ("useragent\n");
	Con_Printf ("---------\n");
	Info_Print (cl->useragent);
	}

/*
===============
SV_KillServer_f

Kick everyone off, possibly in preparation for a new game
===============
*/
static void SV_KillServer_f (void)
	{
	Host_ShutdownServer ();
	}

/*
===============
SV_PlayersOnly_f

disable plhysics but players
===============
*/
static void SV_PlayersOnly_f (void)
	{
	if (!Cvar_VariableInteger ("sv_cheats"))
		return;

	sv.playersonly ^= 1;

	SV_BroadcastPrintf (NULL, "%s game physic\n", sv.playersonly ? "Freeze" : "Resume");
	}

/*
===============
SV_EdictUsage_f
===============
*/
static void SV_EdictUsage_f (void)
	{
	int	active;

	if (sv.state != ss_active)
		{
		Con_Printf ("^3no server running.\n");
		return;
		}

	active = pfnNumberOfEntities ();
	Con_Printf ("%5i edicts is used\n", active);
	Con_Printf ("%5i edicts is free\n", GI->max_edicts - active);
	Con_Printf ("%5i total\n", GI->max_edicts);
	}

/*
===============
SV_EntityInfo_f
===============
*/
static void SV_EntityInfo_f (void)
	{
	edict_t	*ent;
	int		i;

	if (sv.state != ss_active)
		{
		Con_Printf ("^3no server running.\n");
		return;
		}

	for (i = 0; i < svgame.numEntities; i++)
		{
		ent = EDICT_NUM (i);
		if (!SV_IsValidEdict (ent)) continue;

		Con_Printf ("%5i origin: %.f %.f %.f", i, ent->v.origin[0], ent->v.origin[1], ent->v.origin[2]);

		if (ent->v.classname)
			Con_Printf (", class: %s", STRING (ent->v.classname));

		if (ent->v.globalname)
			Con_Printf (", global: %s", STRING (ent->v.globalname));

		if (ent->v.targetname)
			Con_Printf (", name: %s", STRING (ent->v.targetname));

		if (ent->v.target)
			Con_Printf (", target: %s", STRING (ent->v.target));

		if (ent->v.model)
			Con_Printf (", model: %s", STRING (ent->v.model));

		Con_Printf ("\n");
		}
	}

/*
================
Rcon_Redirect_f

Force redirect N lines of console output to client
================
*/
static void Rcon_Redirect_f (void)
	{
	int lines = 2000;

	if (!host.rd.target)
		{
		Msg ("redirect is only valid from rcon\n");
		return;
		}

	if (Cmd_Argc () == 2)
		lines = Q_atoi (Cmd_Argv (1));

	host.rd.lines = lines;
	Msg ("Redirection enabled for next %d lines\n", lines);
	}

/*
==================
SV_InitHostCommands

commands that create server
is available always
==================
*/
void SV_InitHostCommands (void)
	{
	Cmd_AddRestrictedCommand ("map", SV_Map_f, "start new level");
	Cmd_AddCommand ("maps", SV_Maps_f, "list maps");

	if (host.type == HOST_NORMAL)
		{
		Cmd_AddRestrictedCommand ("newgame", SV_NewGame_f, "begin new game");
		Cmd_AddRestrictedCommand ("hazardcourse", SV_HazardCourse_f, "starting a Hazard Course");
		Cmd_AddRestrictedCommand ("map_background", SV_MapBackground_f, "set background map");
		Cmd_AddRestrictedCommand ("load", SV_Load_f, "load a saved game file");
		Cmd_AddRestrictedCommand ("loadquick", SV_QuickLoad_f, "load a quick-saved game file");
		Cmd_AddRestrictedCommand ("reload", SV_Reload_f, "continue from latest save or restart level");
		Cmd_AddRestrictedCommand ("killsave", SV_DeleteSave_f, "delete a saved game file and saveshot");
		Cmd_AddRestrictedCommand ("nextmap", SV_NextMap_f, "load next level");

		// ESHQ: ��������� ��� ��������� ������
		Cmd_AddRestrictedCommand ("credits", SV_Credits_f, "starting a credits");

		// ESHQ: ������� ��� ESRM
		if (strstr (host.gamefolder, "esrm"))
			{
			Cmd_AddRestrictedCommand ("esrm_size", SV_ESRM_Command,
				"Sets the size of the next map (coeff, 1 - 8)");
			Cmd_AddRestrictedCommand ("esrm_enemies", SV_ESRM_Command,
				"Sets the enemies density for the next map (coeff, 1 - 8)");
			Cmd_AddRestrictedCommand ("esrm_items", SV_ESRM_Command,
				"Sets the items density for the next map (coeff, 1 - 8)");
			Cmd_AddRestrictedCommand ("esrm_walls", SV_ESRM_Command,
				"Sets the walls density for the next map (coeff, 1 - 12)");
			Cmd_AddRestrictedCommand ("esrm_light", SV_ESRM_Command,
				"Sets the lighting intensity for the next map (coeff, 1 - 6)");
			Cmd_AddRestrictedCommand ("esrm_crates", SV_ESRM_Command,
				"Sets the crates density for the next map (coeff, 1 - 5)");
			Cmd_AddRestrictedCommand ("esrm_gravity", SV_ESRM_Command,
				"Sets the gravity multiplier (x * 10%) for the next map (coeff, 1 - 20)");

			Cmd_AddRestrictedCommand ("esrm_button", SV_ESRM_Command,
				"Sets the button mode for the next map (0 = none, 1 = one button, 2 = two buttons)");
			Cmd_AddRestrictedCommand ("esrm_sections", SV_ESRM_Command,
				"Sets types of map sections for the next map (0 = all, 1 = only under sky, 2 = only inside)");
			Cmd_AddRestrictedCommand ("esrm_two_floors", SV_ESRM_Command,
				"Disables / enables the two floors mode for the next map (flag, 0 / 1)");
			Cmd_AddRestrictedCommand ("esrm_expl_crates", SV_ESRM_Command,
				"Disables / enables crates with explosives for the next map (flag, 0 / 1)");
			Cmd_AddRestrictedCommand ("esrm_item_crates", SV_ESRM_Command,
				"Disables / enables crates with items (weapons, bugs) for the next map (flag, 0 / 1)");
			Cmd_AddRestrictedCommand ("esrm_makers", SV_ESRM_Command,
				"Disables / enables monster makers for the next map (flag, 0 / 1)");
			Cmd_AddRestrictedCommand ("esrm_barriers", SV_ESRM_Command,
				"Sets types of barriers between map sections for the next map (0 = glass, 1 = fabric, 2 = both)");
			Cmd_AddRestrictedCommand ("esrm_fog", SV_ESRM_Command,
				"Sets the fog density multiplier (x * 10%) for the next map (coeff, 0 - 10)");
			Cmd_AddRestrictedCommand ("esrm_items_on_2nd_floor", SV_ESRM_Command,
				"Disables / enables generation of items on balconies for the next map (flag, 0 / 1)");

			Cmd_AddRestrictedCommand ("esrm_enemies_list", SV_ESRM_Command,
				"Enumerates the allowed enemies (the line of letters for [a]ssassins, [b]ullchickens, "
				"[c]ontrollers, hound[e]yes, human [g]runts, [h]eadcrabs, trip[m]ines, bar[n]acles, "
				"alien g[r]unts, alien [s]laves, [t]urrets and [z]ombies");
			Cmd_AddRestrictedCommand ("esrm_items_list", SV_ESRM_Command,
				"Enumerates the allowed items (the line of letters for health[k]its, [b]atteries, [g]renades, "
				"[9] mm handguns, [s]atchels, .[3]57 pythons, [c]rossbows, ga[u]ss, cro[w]bars, [h]ornetguns, "
				"9 mm A[R]s, sh[o]tguns, R[P]Gs, [t]ripmines, s[n]arks, [e]gons and [a]xes");

			Cmd_AddRestrictedCommand ("esrm_rebuild", SV_ESRM_Command,
				"Forces the rebuilding of the next map. Settings will be applied right after the next teleportation, but the compilation may take one or two minutes (you'll be unable to teleport there during the process)");
			}
		}
	}

/*
==================
SV_InitOperatorCommands
==================
*/
void SV_InitOperatorCommands (void)
	{
	Cmd_AddCommand ("heartbeat", SV_Heartbeat_f, "send a heartbeat to the master server");
	Cmd_AddCommand ("kick", SV_Kick_f, "kick a player off the server by number or name");
	Cmd_AddCommand ("status", SV_Status_f, "print server status information");
	Cmd_AddCommand ("localinfo", SV_LocalInfo_f, "examine or change the localinfo string");
	Cmd_AddCommand ("serverinfo", SV_ServerInfo_f, "examine or change the serverinfo string");
	Cmd_AddCommand ("clientinfo", SV_ClientInfo_f, "print user infostring (player num required)");
	Cmd_AddCommand ("clientuseragent", SV_ClientUserAgent_f, "print user agent (player num required)");
	Cmd_AddCommand ("playersonly", SV_PlayersOnly_f, "freezes time, except for players");
	Cmd_AddCommand ("restart", SV_Restart_f, "restarting current level");
	Cmd_AddCommand ("entpatch", SV_EntPatch_f, "write entity patch to allow external editing");
	Cmd_AddCommand ("edict_usage", SV_EdictUsage_f, "show info about edicts usage");
	Cmd_AddCommand ("entity_info", SV_EntityInfo_f, "show more info about edicts");
	Cmd_AddCommand ("shutdownserver", SV_KillServer_f, "shutdown current server");
	Cmd_AddCommand ("changelevel", SV_ChangeLevel_f, "change level");
	Cmd_AddCommand ("changelevel2", SV_ChangeLevel2_f, "smooth change level");
	Cmd_AddCommand ("redirect", Rcon_Redirect_f, "force enable rcon redirection");
	Cmd_AddCommand ("logaddress", SV_SetLogAddress_f, "sets address and port for remote logging host");
	Cmd_AddCommand ("log", SV_ServerLog_f, "enables logging to file");

	if (host.type == HOST_NORMAL)
		{
		Cmd_AddCommand ("save", SV_Save_f, "save the game to a file");
		Cmd_AddCommand ("savequick", SV_QuickSave_f, "save the game to the quicksave");
		Cmd_AddCommand ("autosave", SV_AutoSave_f, "save the game to 'autosave' file");
		}
	else if (host.type == HOST_DEDICATED)
		{
		Cmd_AddCommand ("say", SV_ConSay_f, "send a chat message to everyone on the server");
		}
	}

/*
==================
SV_KillOperatorCommands
==================
*/
void SV_KillOperatorCommands (void)
	{
	Cmd_RemoveCommand ("heartbeat");
	Cmd_RemoveCommand ("kick");
	Cmd_RemoveCommand ("status");
	Cmd_RemoveCommand ("localinfo");
	Cmd_RemoveCommand ("serverinfo");
	Cmd_RemoveCommand ("clientinfo");
	Cmd_RemoveCommand ("clientuseragent");	// [FWGS, 01.07.23]
	Cmd_RemoveCommand ("playersonly");
	Cmd_RemoveCommand ("restart");
	Cmd_RemoveCommand ("entpatch");
	Cmd_RemoveCommand ("edict_usage");
	Cmd_RemoveCommand ("entity_info");
	Cmd_RemoveCommand ("shutdownserver");
	Cmd_RemoveCommand ("changelevel");
	Cmd_RemoveCommand ("changelevel2");
	Cmd_RemoveCommand ("redirect");	// [FWGS, 01.07.23]
	Cmd_RemoveCommand ("logaddress");
	Cmd_RemoveCommand ("log");

	if (host.type == HOST_NORMAL)
		{
		Cmd_RemoveCommand ("save");
		Cmd_RemoveCommand ("savequick");
		Cmd_RemoveCommand ("autosave");
		}
	else if (host.type == HOST_DEDICATED)
		{
		Cmd_RemoveCommand ("say");
		}
	}
