/***
dedicated.c - stubs for dedicated server
Copyright (C) 2018 a1batross, mittorn

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details
***/

#if XASH_DEDICATED
#include "common.h"
#include "xash3d_mathlib.h"
#include "ref_api.h"
#include "server.h"	// [FWGS, 01.12.24]

ref_globals_t refState;

// [FWGS, 01.12.24] removed CL_ProcessFile

// [FWGS, 01.12.24]
const char *CL_MsgInfo (int cmd)
	{
	static string sz;
	Q_strncpy (sz, "???", sizeof (sz));

	if ((cmd >= 0) && (cmd <= svc_lastmsg))
		{
		// get engine message name
		const char *svc_string = svc_strings[cmd];

		Q_strncpy (sz, svc_string, sizeof (sz));
		}
	else if ((cmd > svc_lastmsg) && (cmd <= (svc_lastmsg + MAX_USER_MESSAGES)))
		{
		int i;

		for (i = 0; i < MAX_USER_MESSAGES; i++)
			{
			if (svgame.msg[i].number == cmd)
				{
				Q_strncpy (sz, svgame.msg[i].name, sizeof (sz));
				break;
				}
			}
		}

	return sz;
	}

// [FWGS, 01.08.24] removed CL_IsInMenu
// [FWGS, 01.02.25] removed CL_Active, CL_Initialized, CL_IsInGame, CL_IsInConsole,
// CL_IsIntermission, CL_IsPlaybackDemo, CL_IsRecordDemo, CL_DisableVisibility, CL_Init

void Key_Init (void)
	{
	}

void IN_Init (void)
	{
	}

void CL_Drop (void)
	{
	}

void CL_ClearEdicts (void)
	{
	}

void GAME_EXPORT Key_SetKeyDest (int key_dest)
	{
	}

void UI_SetActiveMenu (qboolean fActive)
	{
	}

void CL_WriteMessageHistory (void)
	{
	}

// [FWGS, 01.02.25] removed Host_ClientBegin, Host_ClientFrame

void Host_InputFrame (void)
	{
	}

// [FWGS, 01.03.26] removed VID_InitDefaultResolution
/*void VID_InitDefaultResolution (void)
	{
	}*/

void Con_Init (void)
	{
	}

// [FWGS, 01.02.24] removed R_ClearAllDecals, R_CreateDecalList

void GAME_EXPORT S_StopSound (int entnum, int channel, const char *soundname)
	{
	}

// [FWGS, 01.02.24] removed S_GetCurrentStaticSounds
// [FWGS, 01.02.25] removed CL_GetMaxClients

void IN_TouchInitConfig (void)
	{
	}

void CL_Disconnect (void)
	{
	}

// [FWGS, 01.02.25] removed CL_Shutdown

// [FWGS, 01.03.26] removed R_ClearStaticEntities
/*void R_ClearStaticEntities (void)
	{
	}*/

void Host_Credits (void)
	{
	}

// [FWGS, 01.02.25] removed UI_CreditsActive

void S_StopBackgroundTrack (void)
	{
	}

void SCR_BeginLoadingPlaque (qboolean is_background)
	{
	}

// [FWGS, 01.02.25] removed S_GetCurrentDynamicSounds

void S_StopAllSounds (qboolean ambient)
	{
	}

void GAME_EXPORT Con_NPrintf (int idx, const char *fmt, ...)
	{
	}

void GAME_EXPORT Con_NXPrintf (struct  con_nprint_s *info, const char *fmt, ...)
	{
	}

// [FWGS, 01.05.24] removed GL_TextureData

void SCR_CheckStartupVids (void)
	{
	}

void CL_StopPlayback (void)
	{
	}

void CL_ClearStaticEntities (void)
	{
	}

void UI_ShowConnectionWarning (void)
	{
	}

void CL_Crashed (void)
	{
	}

void CL_HudMessage (const char *pMessage)
	{
	}

// [FWGS, 01.01.24]
byte TextureToGamma (byte b)
	{
	return b;
	}

byte LightToTexGamma (byte b)
	{
	return b;
	}

#endif
