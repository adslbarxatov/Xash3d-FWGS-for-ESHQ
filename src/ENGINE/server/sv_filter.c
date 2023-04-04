/*
sv_filter.c - server ID/IP filter
Copyright (C) 2017 a1batross

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

/* [FWGS, 01.04.23]
typedef struct ipfilter_s
	{
	float time;
	float endTime; // -1 for permanent ban
	struct ipfilter_s *next;
	uint mask;
	uint ip;
	} ipfilter_t;

static ipfilter_t *ipfilter = NULL;*/

/*
=============================================================================

PLAYER ID FILTER

=============================================================================
*/
typedef struct cidfilter_s
	{
	float endTime;
	struct cidfilter_s *next;
	string id;
	} cidfilter_t;

static cidfilter_t *cidfilter = NULL;

static void SV_RemoveID (const char *id)
	{
	cidfilter_t *filter, *prevfilter = NULL;

	for (filter = cidfilter; filter; filter = filter->next)
		{
		if (Q_strcmp (filter->id, id))
			{
			prevfilter = filter;
			continue;
			}

		if (filter == cidfilter)
			{
			cidfilter = cidfilter->next;
			Mem_Free (filter);
			return;
			}

		if (prevfilter)
			prevfilter->next = filter->next;
		Mem_Free (filter);
		return;
		}
	}

/* [FWGS, 01.04.23]
static void SV_RemoveIP (uint ip, uint mask)
	{
	ipfilter_t *filter, *prevfilter = NULL;

	for (filter = ipfilter; filter; filter = filter->next)
		{
		if (filter->ip != ip || mask != filter->mask)
			{
			prevfilter = filter;
			continue;
			}

		if (filter == ipfilter)
			{
			ipfilter = ipfilter->next;
			Mem_Free (filter);
			return;
			}

		if (prevfilter)
			prevfilter->next = filter->next;
		Mem_Free (filter);
		return;
		}
	}*/

qboolean SV_CheckID (const char *id)
	{
	qboolean ret = false;
	cidfilter_t *filter;

	for (filter = cidfilter; filter; filter = filter->next)
		{
		int len1 = Q_strlen (id), len2 = Q_strlen (filter->id);
		int len = Q_min (len1, len2);

		while (filter->endTime && host.realtime > filter->endTime)
			{
			char *fid = filter->id;
			filter = filter->next;
			SV_RemoveID (fid);
			if (!filter)
				return false;
			}

		if (!Q_strncmp (id, filter->id, len))
			{
			ret = true;
			break;
			}
		}

	return ret;
	}

/* [FWGS, 01.04.23]
qboolean SV_CheckIP (netadr_t *addr)
	{
	uint ip = addr->ip[0] << 24 | addr->ip[1] << 16 | addr->ip[2] << 8 | addr->ip[3];
	qboolean ret = false;
	ipfilter_t *filter;

	for (filter = ipfilter; filter; filter = filter->next)
		{
		while (filter->endTime && host.realtime > filter->endTime)
			{
			uint rip = filter->ip;
			uint rmask = filter->mask;
			SV_RemoveIP (rip, rmask);
			filter = filter->next;
			if (!filter)
				return false;
			}

		if ((ip & filter->mask) == (filter->ip & filter->mask))
			{
			ret = true;
			break;
			}
		}

	return ret;
	}*/

static void SV_BanID_f (void)
	{
	float time = Q_atof (Cmd_Argv (1));
	const char *id = Cmd_Argv (2);
	sv_client_t *cl = NULL;
	cidfilter_t *filter;

	if (time)
		time = host.realtime + time * 60.0f;

	if (!id[0])
		{
		// [FWGS, 01.04.23]
		/*Con_Reportf ("Usage: banid <minutes> <#userid or unique id>\n0 minutes for permanent ban\n");*/
		Con_Reportf (S_USAGE "banid <minutes> <#userid or unique id>\n0 minutes for permanent ban\n");
		return;
		}

	if (!Q_strnicmp (id, "STEAM_", 6) || !Q_strnicmp (id, "VALVE_", 6))
		id += 6;
	if (!Q_strnicmp (id, "XASH_", 5))
		id += 5;

	if (svs.clients)
		{
		if (id[0] == '#')
			cl = SV_ClientById (Q_atoi (id + 1));

		if (!cl)
			{
			int i;
			sv_client_t *cl1;
			int len = Q_strlen (id);

			for (i = 0, cl1 = svs.clients; i < sv_maxclients->value; i++, cl1++)
				{
				if (!Q_strncmp (id, Info_ValueForKey (cl1->useragent, "uuid"), len))
					{
					cl = cl1;
					break;
					}
				}
			}

		if (!cl)
			{
			Con_DPrintf (S_WARN "banid: no such player\n");
			}
		else
			id = Info_ValueForKey (cl->useragent, "uuid");

		if (!id[0])
			{
			Con_DPrintf (S_ERROR "Could not ban, not implemented yet\n");
			return;
			}
		}

	if (!id[0] || id[0] == '#')
		{
		Con_DPrintf (S_ERROR "banid: bad id\n");
		return;
		}

	SV_RemoveID (id);

	filter = Mem_Malloc (host.mempool, sizeof (cidfilter_t));
	filter->endTime = time;
	filter->next = cidfilter;
	Q_strncpy (filter->id, id, sizeof (filter->id));
	cidfilter = filter;

	// [FWGS, 01.04.23]
	if (cl && !Q_stricmp (Cmd_Argv (Cmd_Argc () - 1), "kick"))
		Cbuf_AddTextf ("kick #%d \"Kicked and banned\"\n", cl->userid);
	/*Cbuf_AddText (va ("kick #%d \"Kicked and banned\"\n", cl->userid));*/
	}

static void SV_ListID_f (void)
	{
	cidfilter_t *filter;

	Con_Reportf ("id ban list\n");
	Con_Reportf ("-----------\n");

	for (filter = cidfilter; filter; filter = filter->next)
		{
		if (filter->endTime && host.realtime > filter->endTime)
			continue; // no negative time

		if (filter->endTime)
			Con_Reportf ("%s expries in %f minutes\n", filter->id, (filter->endTime - host.realtime) / 60.0f);
		else
			Con_Reportf ("%s permanent\n", filter->id);
		}
	}

static void SV_RemoveID_f (void)
	{
	const char *id = Cmd_Argv (1);

	if ((id[0] == '#') && svs.clients)
		{
		int num = Q_atoi (id + 1);

		if ((num >= sv_maxclients->value) || (num < 0))
			return;

		id = Info_ValueForKey (svs.clients[num].useragent, "uuid");
		}

	if (!id[0])
		{
		// [FWGS, 01.04.23]
		/*Con_Reportf ("Usage: removeid <#slotnumber or uniqueid>\n");*/
		Con_Reportf( S_USAGE "removeid <#slotnumber or uniqueid>\n");
		return;
		}

	SV_RemoveID (id);
	}

static void SV_WriteID_f (void)
	{
	file_t *f = FS_Open (Cvar_VariableString ("bannedcfgfile"), "w", false);
	cidfilter_t *filter;

	if (!f)
		{
		Con_DPrintf (S_ERROR "Could not write %s\n", Cvar_VariableString ("bannedcfgfile"));
		return;
		}

	FS_Printf (f, "//=======================================================================\n");
	FS_Printf (f, "//\t\tCopyright Flying With Gauss Team %s ©\n", Q_timestamp (TIME_YEAR_ONLY));
	FS_Printf (f, "//\t\t    %s - archive of id blacklist\n", Cvar_VariableString ("bannedcfgfile"));
	FS_Printf (f, "//=======================================================================\n");

	for (filter = cidfilter; filter; filter = filter->next)
		if (!filter->endTime) // only permanent
			FS_Printf (f, "banid 0 %s\n", filter->id);

	FS_Close (f);
	}

// [FWGS, 01.04.23]
//static qboolean StringToIP (const char *str, const char *maskstr, uint *outip, uint *outmask)
static void SV_InitIDFilter (void)
	{
	/*byte ip[4] = { 0 };
	byte mask[4] = { 0 };
	int i = 0;*/
	Cmd_AddRestrictedCommand ("banid", SV_BanID_f, "ban player by ID");
	Cmd_AddRestrictedCommand ("listid", SV_ListID_f, "list banned players");
	Cmd_AddRestrictedCommand ("removeid", SV_RemoveID_f, "remove player from banned list");
	Cmd_AddRestrictedCommand ("writeid", SV_WriteID_f, "write banned.cfg");
	}

/*if (*str > '9' || *str < '0')
return false;*/

// [FWGS, 01.04.23]
static void SV_ShutdownIDFilter (void)
	{
	cidfilter_t *cidList, *cidNext;

	// should be called manually because banned.cfg is not executed by engine
	Cmd_RemoveCommand ("banid");
	Cmd_RemoveCommand ("listid");
	Cmd_RemoveCommand ("removeid");
	Cmd_RemoveCommand ("writeid");

	for (cidList = cidfilter; cidList; cidList = cidNext)
		{
		cidNext = cidList->next;
		Mem_Free (cidList);
		}

	cidfilter = NULL;
	}

/*
=============================================================================

CLIENT IP FILTER

=============================================================================
*/

// [FWGS, 01.04.23]
typedef struct ipfilter_s
	{
	float endTime;
	struct ipfilter_s *next;
	netadr_t adr;
	uint prefixlen;
	} ipfilter_t;
static ipfilter_t *ipfilter = NULL;

// [FWGS, 01.04.23]
/*do*/
static void SV_CleanExpiredIPFilters (void)
	{
	/*while (*str <= '9' && *str >= '0')*/
	ipfilter_t *f, **back;

	back = &ipfilter;
	while (1)
		{
		f = *back;
		if (!f) 
			return;

		if (f->endTime && host.realtime > f->endTime)
			{
			/*ip[i] *= 10;
			ip[i] += *str - '0';
			str++;*/
			*back = f->next;
			back = &f->next;

			Mem_Free (f);
			}
		else
			{
			back = &f->next;
			}
		}
	}

/*mask[i] = 255;
i++;
if (*str != '.') break;
str++;
} while (i < 4);
i = 0;*/

// [FWGS, 01.04.23]
static int SV_FilterToString (char *dest, size_t size, qboolean config, ipfilter_t *f)
	{
	const char *strformat;

	/*if (!maskstr || *maskstr > '9' || *maskstr < '0')
		goto end;*/

	if (config)
		return Q_snprintf (dest, size, "addip 0 %s/%d\n", NET_AdrToString (f->adr), f->prefixlen);

	else if (f->endTime)
		return Q_snprintf (dest, size, "%s/%d (%f minutes)", NET_AdrToString (f->adr), f->prefixlen, f->endTime);

	/*do*/
	return Q_snprintf (dest, size, "%s/%d (permanent)", NET_AdrToString (f->adr), f->prefixlen);
	}

// [FWGS, 01.04.23]
static qboolean SV_IPFilterIncludesIPFilter (ipfilter_t *a, ipfilter_t *b)
	{
	if (a->adr.type6 != b->adr.type6)
		return false;

	// can't include bigger subnet in small
	if (a->prefixlen < b->prefixlen)
		return false;

	if (a->prefixlen == b->prefixlen)
		return NET_CompareAdr (a->adr, b->adr);

	return NET_CompareAdrByMask (a->adr, b->adr, b->prefixlen);
	}

// [FWGS, 01.04.23]
static void SV_RemoveIPFilter (ipfilter_t *toremove, qboolean removeAll, qboolean verbose)
	{
	ipfilter_t *f, **back;

	back = &ipfilter;
	while (1)
		{
		f = *back;
		if (!f) return;

		if (SV_IPFilterIncludesIPFilter (toremove, f))
			{
			/*byte mask1 = 0;
			while (*maskstr <= '9' && *maskstr >= '0')*/
			if (verbose)
				{
				/*mask1 *= 10;
				mask1 += *maskstr - '0';
				maskstr++;*/
				string filterStr;
				SV_FilterToString (filterStr, sizeof (filterStr), false, f);
				Con_Printf ("%s removed.\n", filterStr);
				}

			/*mask[i] &= mask1;
			i++;
			if (*maskstr != '.') break;
			maskstr++;
			} while (i < 4);

end:
			*outip = ip[0] << 24 | ip[1] << 16 | ip[2] << 8 | ip[3];
			if (outmask)
				*outmask = mask[0] << 24 | mask[1] << 16 | mask[2] << 8 | mask[3];*/

			*back = f->next;
			back = &f->next;

			Mem_Free (f);

			if (!removeAll)
				break;
			}
		else
			{
			back = &f->next;
			}
		}
	}

// [FWGS, 01.04.23]
qboolean SV_CheckIP (netadr_t *adr)
	{
	ipfilter_t *entry = ipfilter;

	for (; entry; entry = entry->next)
		{
		switch (entry->adr.type6)
			{
			case NA_IP:
			case NA_IP6:
				if (NET_CompareAdrByMask (*adr, entry->adr, entry->prefixlen))
					return true;
				break;
			}
		}

	return false;
	}

// [FWGS, 01.04.23]
static void SV_AddIP_PrintUsage (void)
	{
	Con_Printf (S_USAGE "addip <minutes> <ipaddress>\n"
		S_USAGE_INDENT  "addip <minutes> <ipaddress/CIDR>\n"
		"Use 0 minutes for permanent\n"
		"ipaddress A.B.C.D/24 is equivalent to A.B.C.0 and A.B.C\n"
		"NOTE: IPv6 addresses only support prefix format!\n");
	}

// [FWGS, 01.04.23]
static void SV_RemoveIP_PrintUsage (void)
	{
	Con_Printf (S_USAGE "removeip <ipaddress> [removeAll]\n"
		S_USAGE_INDENT  "removeip <ipaddress/CIDR> [removeAll]\n"
		"Use removeAll to delete all ip filters which ipaddress or ipaddress/CIDR includes\n");
	}

// [FWGS, 01.04.23]
static void SV_ListIP_PrintUsage (void)
	{
	Con_Printf (S_USAGE "listip [ipaddress]\n"
		S_USAGE_INDENT  "listip [ipaddress/CIDR]\n");
	}

/* [FWGS, 01.04.23]
#define IPARGS(ip) (ip >> 24) & 0xFF, (ip >> 16) & 0xFF, (ip >> 8) & 0xFF, ip & 0xFF
*/

// [FWGS, 01.04.23]
static void SV_AddIP_f (void)
	{
	/*float time = Q_atof (Cmd_Argv (1));
	const char *ipstr = Cmd_Argv (2);
	const char *maskstr = Cmd_Argv (3);
	uint ip, mask;
	ipfilter_t *filter;*/
	const char *szMinutes = Cmd_Argv (1);
	const char *adr = Cmd_Argv (2);
	ipfilter_t filter, *newfilter;
	float minutes;
	int i;

	/*if (time)
		time = host.realtime + time * 60.0f;*/
	if (Cmd_Argc () != 3)
		{
		// a1ba: kudos to rehlds for an idea of using CIDR prefixes
		// in these commands :)
		SV_AddIP_PrintUsage ();
		return;
		}

	/*if (!StringToIP (ipstr, maskstr, &ip, &mask))*/
	minutes = Q_atof (szMinutes);
	if (minutes < 0.1f)
		minutes = 0;

	if (minutes != 0.0f)
		filter.endTime = host.realtime + minutes * 60;
	else 
		filter.endTime = 0;

	if (!NET_StringToFilterAdr (adr, &filter.adr, &filter.prefixlen))
		{
		/*Con_Reportf ("Usage: addip <minutes> <ip> [mask]\n0 minutes for permanent ban\n");*/
		Con_Printf ("Invalid IP address!\n");
		SV_AddIP_PrintUsage ();
		return;
		}

	/*SV_RemoveIP (ip, mask);*/
	newfilter = Mem_Malloc (host.mempool, sizeof (*newfilter));
	newfilter->endTime = filter.endTime;
	newfilter->adr = filter.adr;
	newfilter->prefixlen = filter.prefixlen;
	newfilter->next = ipfilter;

	/*filter = Mem_Malloc (host.mempool, sizeof (ipfilter_t));
	filter->endTime = time;
	filter->ip = ip;
	filter->mask = mask;
	filter->next = ipfilter;*/
	ipfilter = newfilter;

	for (i = 0; i < svs.maxclients; i++)
		{
		netadr_t clientadr = svs.clients[i].netchan.remote_address;

		if (!NET_CompareAdrByMask (clientadr, filter.adr, filter.prefixlen))
			continue;

		/*ipfilter = filter;*/
		SV_ClientPrintf (&svs.clients[i], "The server operator has added you to banned list\n");
		SV_DropClient (&svs.clients[i], false);
		}
	}

// [FWGS, 01.04.23]
static void SV_ListIP_f (void)
	{
	/*ipfilter_t *filter;*/
	qboolean haveFilter = false;
	ipfilter_t filter, *f;

	/*Con_Reportf ("ip ban list\n");
	Con_Reportf ("-----------\n");*/
	if (Cmd_Argc () > 2)
		{
		SV_ListIP_PrintUsage ();
		return;
		}

	/*for (filter = ipfilter; filter; filter = filter->next)*/
	if (ipfilter == NULL)
		{
		/*if (filter->endTime && host.realtime > filter->endTime)
			continue; // no negative time*/
		Con_Printf ("IP filter list is empty\n");
		return;
		}

	/*if (filter->endTime)
		Con_Reportf ("%d.%d.%d.%d %d.%d.%d.%d expries in %f minutes\n", IPARGS (filter->ip), IPARGS (filter->mask), (filter->endTime - host.realtime) / 60.0f);
	else
		Con_Reportf ("%d.%d.%d.%d %d.%d.%d.%d permanent\n", IPARGS (filter->ip), IPARGS (filter->mask));*/

	if (Cmd_Argc () == 2)
		{
		haveFilter = NET_StringToFilterAdr (Cmd_Argv (1), &filter.adr, &filter.prefixlen);

		if (!haveFilter)
			{
			Con_Printf ("Invalid IP address!\n");
			SV_ListIP_PrintUsage ();
			return;
			}
		}

	Con_Printf ("IP filter list:\n");

	for (f = ipfilter; f; f = f->next)
		{
		string filterStr;

		if (haveFilter && !SV_IPFilterIncludesIPFilter (&filter, f))
			continue;

		SV_FilterToString (filterStr, sizeof (filterStr), false, f);
		Con_Printf ("%s\n", filterStr);
		}
	}

// [FWGS, 01.04.23]
static void SV_RemoveIP_f (void)
	{
	/*uint ip, mask;*/
	const char *adr = Cmd_Argv (1);
	qboolean removeAll;
	ipfilter_t filter;
	int i;

	/*if (!StringToIP (Cmd_Argv (1), Cmd_Argv (2), &ip, &mask))*/
	if ((Cmd_Argc () != 2) && (Cmd_Argc () != 3))
		{
		/*Con_Reportf ("Usage: removeip <ip> [mask]\n");*/
		SV_RemoveIP_PrintUsage ();
		return;
		}

	/*SV_RemoveIP (ip, mask);*/
	removeAll = (Cmd_Argc () == 3) && !Q_strcmp (Cmd_Argv (2), "removeAll");

	if (!NET_StringToFilterAdr (adr, &filter.adr, &filter.prefixlen))
		{
		Con_Printf ("Invalid IP address!\n");
		SV_RemoveIP_PrintUsage ();
		return;
		}

	SV_RemoveIPFilter (&filter, removeAll, true);
	}

// [FWGS, 01.04.23]
static void SV_WriteIP_f (void)
	{
	/*file_t *f = FS_Open (Cvar_VariableString ("listipcfgfile"), "w", false);
	ipfilter_t *filter;*/
	file_t *fd = FS_Open (Cvar_VariableString ("listipcfgfile"), "w", true);
	ipfilter_t *f;

	/*if (!f)*/
	if (!fd)
		{
		/*Con_DPrintf (S_ERROR "Could not write %s\n", Cvar_VariableString ("listipcfgfile"));*/
		Con_Printf ("Couldn't open listip.cfg\n");
		return;
		}

	/*FS_Printf (f, "//=======================================================================\n");
	FS_Printf (f, "//\t\tCopyright Flying With Gauss Team %s ©\n", Q_timestamp (TIME_YEAR_ONLY));
	FS_Printf (f, "//\t\t    %s - archive of IP blacklist\n", Cvar_VariableString ("listipcfgfile"));
	FS_Printf (f, "//=======================================================================\n");*/
	for (f = ipfilter; f; f = f->next)
		{
		string filterStr;
		int size;

		/*for (filter = ipfilter; filter; filter = filter->next)
		if (!filter->endTime) // only permanent
			FS_Printf (f, "addip 0 %d.%d.%d.%d %d.%d.%d.%d\n", IPARGS (filter->ip), IPARGS (filter->mask));*/
		// do not save temporary bans
		if (f->endTime)
			continue;

		/*FS_Close (f);*/
		size = SV_FilterToString (filterStr, sizeof (filterStr), true, f);
		FS_Write (fd, filterStr, size);
		}

	/*void SV_InitFilter (void)*/
	FS_Close (fd);
	}

// [FWGS, 01.04.23]
static void SV_InitIPFilter (void)
	{
	/*Cmd_AddRestrictedCommand ("banid", SV_BanID_f, "ban player by ID");
	Cmd_AddRestrictedCommand ("listid", SV_ListID_f, "list banned players");
	Cmd_AddRestrictedCommand ("removeid", SV_RemoveID_f, "remove player from banned list");
	Cmd_AddRestrictedCommand ("writeid", SV_WriteID_f, "write banned.cfg");*/
	Cmd_AddRestrictedCommand ("addip", SV_AddIP_f, "add entry to IP filter");
	Cmd_AddRestrictedCommand ("listip", SV_ListIP_f, "list current IP filter");
	Cmd_AddRestrictedCommand ("removeip", SV_RemoveIP_f, "remove IP filter");
	Cmd_AddRestrictedCommand ("writeip", SV_WriteIP_f, "write listip.cfg");
	}

// [FWGS, 01.04.23]
//void SV_ShutdownFilter (void)
static void SV_ShutdownIPFilter (void)
	{
	ipfilter_t *ipList, *ipNext;
	/*cidfilter_t *cidList, *cidNext;*/

	for (ipList = ipfilter; ipList; ipList = ipNext)
		{
		ipNext = ipList->next;
		Mem_Free (ipList);
		}

	/*for (cidList = cidfilter; cidList; cidList = cidNext)*/
	ipfilter = NULL;
	}

// [FWGS, 01.04.23]
void SV_InitFilter (void)
	{
	/*cidNext = cidList->next;
	Mem_Free (cidList);*/
	SV_InitIPFilter ();
	SV_InitIDFilter ();
	}

/*cidfilter = NULL;
ipfilter = NULL;*/

// [FWGS, 01.04.23]
void SV_ShutdownFilter (void)
	{
	SV_ShutdownIPFilter ();
	SV_ShutdownIDFilter ();
	}

#if XASH_ENGINE_TESTS

#include "tests.h"

// [FWGS, 01.04.23]
void Test_StringToFilterAdr (void)
	{
	ipfilter_t f1;
	int i;
	struct
		{
		const char *str;
		qboolean valid;
		int prefixlen;
		int a, b, c, d;
		} ipv4tests[] =
			{
			{ "127.0.0.0/8", true, 8, 127, 0, 0, 0 },
			{ "192.168",     true, 16, 192, 168, 0, 0 },
			{ "192.168/23",  true, 23, 192, 168, 0, 0 },
			{ "192.168./23", true, 23, 192, 168, 0, 0 },
			{ "192.168../23", true, 23, 192, 168, 0, 0 },
			{ "..192...168/23", false },
			{ "", false },
			{ "abcd", false }
			};
		struct
			{
			const char *str;
			qboolean valid;
			int prefixlen;
			uint8_t x[16];
			} ipv6tests[] =
				{
				{ "::1", true, 128, { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 } },
				{ "fd18:b9d4:65cf:83de::/64", true, 64, { 0xfd, 0x18, 0xb9, 0xd4, 0x65, 0xcf, 0x83, 0xde } },
				{ "kkljnljkhfjnkj", false },
				{ "fd8a:63d5:e014:0d62:ffff:ffff:ffff:ffff:ffff", false },
				};

			for (i = 0; i < ARRAYSIZE (ipv4tests); i++)
				{
				qboolean ret = NET_StringToFilterAdr (ipv4tests[i].str, &f1.adr, &f1.prefixlen);

				TASSERT_EQi (ret, ipv4tests[i].valid);

				if (ret)
					{
					TASSERT_EQi (f1.prefixlen, ipv4tests[i].prefixlen);
					TASSERT_EQi (f1.adr.ip[0], ipv4tests[i].a);
					TASSERT_EQi (f1.adr.ip[1], ipv4tests[i].b);
					TASSERT_EQi (f1.adr.ip[2], ipv4tests[i].c);
					TASSERT_EQi (f1.adr.ip[3], ipv4tests[i].d);
					}
				}

			for (i = 0; i < ARRAYSIZE (ipv6tests); i++)
				{
				qboolean ret = NET_StringToFilterAdr (ipv6tests[i].str, &f1.adr, &f1.prefixlen);
				uint8_t x[16];

				TASSERT_EQi (ret, ipv6tests[i].valid);

				if (ret)
					{
					TASSERT_EQi (f1.prefixlen, ipv6tests[i].prefixlen);

					NET_NetadrToIP6Bytes ((uint8_t *)x, &f1.adr);

					TASSERT (memcmp (x, ipv6tests[i].x, sizeof (x)) == 0);
					}
				}
	}

// [FWGS, 01.04.23]
void Test_IPFilterIncludesIPFilter (void)
	{
	qboolean ret;
	const char *adrs[] =
		{
			"127.0.0.1/8", // 0
			"127.0.0.1", // 1
			"192.168/16", // 2
			"fe80::/64", // 3
			"fe80::96ab:9a49:2944:1808", // 4
			"2a00:1370:8190:f9eb::/62", // 5
			"2a00:1370:8190:f9eb:3866:6126:330c:b82b" // 6
		};
	ipfilter_t f[7];
	int i;
	int tests[][3] =
		{
		// ipv4
		{ 0, 0, true },
		{ 0, 1, false },
		{ 1, 0, true },
		{ 0, 2, false },
		{ 2, 0, false },

		// mixed
		{ 0, 3, false },
		{ 1, 4, false },

		// ipv6
		{ 3, 3, true },
		{ 3, 4, false },
		{ 4, 3, true },
		{ 5, 3, false },
		{ 3, 5, false },
		{ 6, 5, true },
		};

	for (i = 0; i < 7; i++)
		{
		NET_StringToFilterAdr (adrs[i], &f[i].adr, &f[i].prefixlen);
		}

	for (i = 0; i < ARRAYSIZE (tests); i++)
		{
		ret = SV_IPFilterIncludesIPFilter (&f[tests[i][0]], &f[tests[i][1]]);

		TASSERT_EQi (ret, tests[i][2]);
		}
	}

// [FWGS, 01.04.23]
void Test_RunIPFilter (void)
	{
	Test_StringToFilterAdr ();
	Test_IPFilterIncludesIPFilter ();
	}

#endif
