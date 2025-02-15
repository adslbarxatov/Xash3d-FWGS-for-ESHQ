/***
sv_query.c - Source-engine like server querying
Copyright (C) 2023 jeefo

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
GNU General Public License for more details
***/

#include "common.h"
#include "server.h"

// [FWGS, 01.12.24]
/*#define SOURCE_QUERY_DETAILS 'T'
#define SOURCE_QUERY_DETAILS_RESPONSE 'I'

#define SOURCE_QUERY_RULES 'V'
#define SOURCE_QUERY_RULES_RESPONSE 'E'

#define SOURCE_QUERY_PLAYERS 'U'
#define SOURCE_QUERY_PLAYERS_RESPONSE 'D'*/

/***
==================
SV_SourceQuery_Details [FWGS, 01.12.24]
==================
***/
static void SV_SourceQuery_Details (netadr_t from)
	{
	sizebuf_t	buf;
	char		answer[2048];
	int			bot_count, client_count;

	SV_GetPlayerCount (&client_count, &bot_count);
	client_count += bot_count; // bots are counted as players in this reply

	MSG_Init (&buf, "TSourceEngineQuery", answer, sizeof (answer));

	/*MSG_WriteByte (&buf, SOURCE_QUERY_DETAILS_RESPONSE);*/
	MSG_WriteDword (&buf, 0xFFFFFFFFU);
	MSG_WriteByte (&buf, S2A_GOLDSRC_INFO);
	MSG_WriteByte (&buf, PROTOCOL_VERSION);

	MSG_WriteString (&buf, hostname.string);
	MSG_WriteString (&buf, sv.name);
	MSG_WriteString (&buf, GI->gamefolder);
	MSG_WriteString (&buf, svgame.dllFuncs.pfnGetGameDescription ());

	MSG_WriteShort (&buf, 0);
	MSG_WriteByte (&buf, client_count);
	MSG_WriteByte (&buf, svs.maxclients);
	MSG_WriteByte (&buf, bot_count);

	MSG_WriteByte (&buf, Host_IsDedicated () ? 'd' : 'l');
#if XASH_WIN32
	MSG_WriteByte (&buf, 'w');
#elif XASH_APPLE
	MSG_WriteByte (&buf, 'm');
#else
	MSG_WriteByte (&buf, 'l');
#endif

	if (SV_HavePassword ())
		MSG_WriteByte (&buf, 1);
	else
		MSG_WriteByte (&buf, 0);

	MSG_WriteByte (&buf, GI->secure);
	MSG_WriteString (&buf, XASH_VERSION);

	/*Netchan_OutOfBand (NS_SERVER, from, MSG_GetNumBytesWritten (&buf), MSG_GetData (&buf));*/
	NET_SendPacket (NS_SERVER, MSG_GetNumBytesWritten (&buf), MSG_GetData (&buf), from);
	}

/***
==================
SV_SourceQuery_Rules [FWGS, 01.12.24]
==================
***/
static void SV_SourceQuery_Rules (netadr_t from)
	{
	const cvar_t	*cvar;
	sizebuf_t		buf;
	char	answer[MAX_PRINT_MSG - 4];
	int		pos;
	uint	cvar_count = 0;

	MSG_Init (&buf, "TSourceEngineQueryRules", answer, sizeof (answer));

	/*MSG_WriteByte (&buf, SOURCE_QUERY_RULES_RESPONSE);*/
	MSG_WriteDword (&buf, 0xFFFFFFFFU);
	MSG_WriteByte (&buf, S2A_GOLDSRC_RULES);
	pos = MSG_GetNumBitsWritten (&buf);
	MSG_WriteShort (&buf, 0);

	for (cvar = Cvar_GetList (); cvar; cvar = cvar->next)
		{
		if (!FBitSet (cvar->flags, FCVAR_SERVER))
			continue;

		MSG_WriteString (&buf, cvar->name);

		if (FBitSet (cvar->flags, FCVAR_PROTECTED))
			{
			if (COM_CheckStringEmpty (cvar->string) && Q_stricmp (cvar->string, "none"))
				MSG_WriteString (&buf, "1");
			else
				MSG_WriteString (&buf, "0");
			}
		else
			{
			MSG_WriteString (&buf, cvar->string);
			}

		cvar_count++;
		}

	if (cvar_count != 0)
		{
		int total = MSG_GetNumBytesWritten (&buf);

		MSG_SeekToBit (&buf, pos, SEEK_SET);
		MSG_WriteShort (&buf, cvar_count);

		/*Netchan_OutOfBand (NS_SERVER, from, total, MSG_GetData (&buf));*/
		NET_SendPacket (NS_SERVER, total, MSG_GetData (&buf), from);
		}
	}

/***
==================
SV_SourceQuery_Players [FWGS, 01.12.24]
==================
***/
static void SV_SourceQuery_Players (netadr_t from)
	{
	sizebuf_t	buf;
	char		answer[MAX_PRINT_MSG - 4];
	int			i, count = 0;
	int			pos;
	
	// respect players privacy
	if (!sv_expose_player_list.value || SV_HavePassword ())
		return;

	MSG_Init (&buf, "TSourceEngineQueryPlayers", answer, sizeof (answer));

	/*MSG_WriteByte (&buf, SOURCE_QUERY_PLAYERS_RESPONSE);*/
	MSG_WriteDword (&buf, 0xFFFFFFFFU);
	MSG_WriteByte (&buf, S2A_GOLDSRC_PLAYERS);
	pos = MSG_GetNumBitsWritten (&buf);
	MSG_WriteByte (&buf, 0);

	for (i = 0; i < svs.maxclients; i++)
		{
		const sv_client_t *cl = &svs.clients[i];
		if (cl->state < cs_connected)
			continue;

		MSG_WriteByte (&buf, count);
		MSG_WriteString (&buf, cl->name);
		MSG_WriteLong (&buf, cl->edict->v.frags);
		if (FBitSet (cl->flags, FCL_FAKECLIENT))
			MSG_WriteFloat (&buf, -1.0f);
		else
			MSG_WriteFloat (&buf, host.realtime - cl->connection_started);

		count++;
		}

	if (count != 0)
		{
		int total = MSG_GetNumBytesWritten (&buf);

		MSG_SeekToBit (&buf, pos, SEEK_SET);
		MSG_WriteByte (&buf, count);

		/*Netchan_OutOfBand (NS_SERVER, from, total, MSG_GetData (&buf));*/
		NET_SendPacket (NS_SERVER, total, MSG_GetData (&buf), from);
		}
	}

/***
==================
SV_SourceQuery_HandleConnnectionlessPacket [FWGS, 01.12.24]
==================
***/
/*qboolean SV_SourceQuery_HandleConnnectionlessPacket (const char *c, netadr_t from)*/
void SV_SourceQuery_HandleConnnectionlessPacket (const char *c, netadr_t from)
	{
	/*switch (c[0])*/
	if (!Q_strcmp (c, A2S_GOLDSRC_INFO))
		{
		/*case SOURCE_QUERY_DETAILS:*/
		SV_SourceQuery_Details (from);
		/*return true;

		case SOURCE_QUERY_RULES:*/
		}
	else switch (c[0])
		{
		case A2S_GOLDSRC_RULES:
			SV_SourceQuery_Rules (from);
			/*return true;

			case SOURCE_QUERY_PLAYERS:*/
			break;

		case A2S_GOLDSRC_PLAYERS:
			SV_SourceQuery_Players (from);
			/*return true;

			default:
			return false;*/
			break;
		}
	/*return false;*/
	}
