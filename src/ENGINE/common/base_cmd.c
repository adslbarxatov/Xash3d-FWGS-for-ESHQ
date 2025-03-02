/***
base_cmd.c - command & cvar hashmap. Insipred by Doom III
Copyright (C) 2016 a1batross

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
#include "base_cmd.h"
#include "cdll_int.h"

// [FWGS, 01.02.25]
/*define HASH_SIZE 128 // 128 * 4 * 4 == 2048 bytes*/
#define HASH_SIZE 64	// 64 * 4 * 4 == 1024 bytes

// [FWGS, 01.05.24]
typedef struct base_command_hashmap_s base_command_hashmap_t;

// [FWGS, 01.02.25]
struct base_command_hashmap_s
	{
	base_command_t *basecmd;	// base command: cvar, alias or command
	base_command_hashmap_t *next;
	base_command_type_e type;	// type for faster searching
	/*char name[1];	// key for searching*/
	char name[]; // key for searching
	};

// [FWGS, 01.02.25]
static base_command_hashmap_t *hashed_cmds[HASH_SIZE];
static poolhandle_t basecmd_pool;

#define BaseCmd_HashKey(x) COM_HashKey(name, HASH_SIZE)

/***
============
BaseCmd_FindInBucket [FWGS, 01.02.25]

Find base command in bucket
============
***/
static base_command_hashmap_t *BaseCmd_FindInBucket (base_command_hashmap_t *bucket, base_command_type_e type,
	const char *name)
	{
	/*base_command_hashmap_t *i = bucket;
	for (; i && ((i->type != type) || Q_stricmp (name, i->name)); // filter out
		i = i->next);*/
	base_command_hashmap_t *i;

	/*return i;*/
	for (i = bucket; i != NULL; i = i->next)
		{
		int cmp;

		if (i->type != type)
			continue;

		cmp = Q_stricmp (i->name, name);

		if (cmp < 0)
			continue;

		if (cmp > 0)
			break;

		return i;
		}

	return NULL;
	}

/***
============
BaseCmd_GetBucket

Get bucket which contain basecmd by given name
============
***/
static base_command_hashmap_t *BaseCmd_GetBucket (const char *name)
	{
	return hashed_cmds[COM_HashKey (name, HASH_SIZE)];
	}

/***
============
BaseCmd_Find

Find base command in hashmap
============
***/
base_command_t *BaseCmd_Find (base_command_type_e type, const char *name)
	{
	base_command_hashmap_t *base = BaseCmd_GetBucket (name);
	base_command_hashmap_t *found = BaseCmd_FindInBucket (base, type, name);

	if (found)
		return found->basecmd;
	return NULL;
	}

/***
============
BaseCmd_Find [FWGS, 01.02.25]

Find every type of base command and write into arguments
============
***/
void BaseCmd_FindAll (const char *name, base_command_t **cmd, base_command_t **alias, base_command_t **cvar)
	{
	base_command_hashmap_t *base = BaseCmd_GetBucket (name);
	base_command_hashmap_t *i = base;

	/*ASSERT (cmd && alias && cvar);*/

	*cmd = *alias = *cvar = NULL;

	for (; i; i = i->next)
		{
		/*if (!Q_stricmp (i->name, name))*/
		int cmp = Q_stricmp (i->name, name);

		if (cmp < 0)
			continue;

		if (cmp > 0)
			break;

		switch (i->type)
			{
			/*switch (i->type)
				{
				case HM_CMD:
					*cmd = i->basecmd;
					break;
				case HM_CMDALIAS:
					*alias = i->basecmd;
					break;
				case HM_CVAR:
					*cvar = i->basecmd;
					break;
				default: break;
				}*/
			case HM_CMD:
				*cmd = i->basecmd;
				break;

			case HM_CMDALIAS:
				*alias = i->basecmd;
				break;

			case HM_CVAR:
				*cvar = i->basecmd;
				break;

			default:
				break;
			}
		}
	}

/***
============
BaseCmd_Insert

Add new typed base command to hashmap
============
***/
void BaseCmd_Insert (base_command_type_e type, base_command_t *basecmd, const char *name)
	{
	base_command_hashmap_t *elem, *cur, *find;
	uint hash = BaseCmd_HashKey (name);
	size_t len = Q_strlen (name);

	// [FWGS, 01.02.25]
	/*elem = Z_Malloc (sizeof (base_command_hashmap_t) + len);*/
	elem = Mem_Malloc (basecmd_pool, sizeof (base_command_hashmap_t) + len + 1);

	elem->basecmd = basecmd;
	elem->type = type;
	Q_strncpy (elem->name, name, len + 1);

	// [FWGS, 01.02.25] link the variable in alphanumerical order
	for (cur = NULL, find = hashed_cmds[hash];
		/*find && Q_strcmp (find->name, elem->name) < 0;*/
		find && Q_stricmp (find->name, elem->name) < 0;
		cur = find, find = find->next);

	if (cur)
		cur->next = elem;
	else
		hashed_cmds[hash] = elem;

	elem->next = find;
	}

/***
============
BaseCmd_Remove [FWGS, 01.02.25]

Remove base command from hashmap
============
***/
void BaseCmd_Remove (base_command_type_e type, const char *name)
	{
	uint hash = BaseCmd_HashKey (name);
	base_command_hashmap_t *i, *prev;

	/*for (prev = NULL, i = hashed_cmds[hash]; i &&
		(Q_strcmp (i->name, name) || (i->type != type)); // filter out
		prev = i, i = i->next);*/
	for (prev = NULL, i = hashed_cmds[hash]; i != NULL; prev = i, i = i->next)
		{
		int cmp;

		if (i->type != type)
			continue;

		cmp = Q_stricmp (i->name, name);

		if (cmp < 0)
			continue;

		if (cmp > 0)
			i = NULL;

		break;
		}

	if (!i)
		{
		Con_Reportf (S_ERROR "%s: Couldn't find %s in buckets\n", __func__, name);
		return;
		}

	if (prev)
		prev->next = i->next;
	else
		hashed_cmds[hash] = i->next;

	Z_Free (i);
	}

/***
============
BaseCmd_Init [FWGS, 01.02.25]

initialize base command hashmap system
============
***/
void BaseCmd_Init (void)
	{
	basecmd_pool = Mem_AllocPool ("BaseCmd");
	memset (hashed_cmds, 0, sizeof (hashed_cmds));
	}

// [FWGS, 01.02.25]
void BaseCmd_Shutdown (void)
	{
	Mem_FreePool (&basecmd_pool);
	}

/***
============
BaseCmd_Stats_f
============
***/
void BaseCmd_Stats_f (void)
	{
	int i, minsize = 99999, maxsize = -1, empty = 0;

	for (i = 0; i < HASH_SIZE; i++)
		{
		base_command_hashmap_t *hm;
		int len = 0;

		// count bucket length
		for (hm = hashed_cmds[i]; hm; hm = hm->next, len++);

		if (len == 0)
			{
			empty++;
			continue;
			}

		if (len < minsize)
			minsize = len;

		if (len > maxsize)
			maxsize = len;
		}

	Con_Printf ("min length: %d, max length: %d, empty: %d\n", minsize, maxsize, empty);
	}

// [FWGS, 01.04.23]
typedef struct
	{
	qboolean	valid;
	int			lookups;
	} basecmd_test_stats_t;

// [FWGS, 01.04.23]
static void BaseCmd_CheckCvars (const char *key, const char *value, const void *unused, void *ptr)
	{
	basecmd_test_stats_t *stats = ptr;

	stats->lookups++;
	if (!BaseCmd_Find (HM_CVAR, key))
		{
		Con_Printf ("Cvar %s is missing in basecmd\n", key);
		stats->valid = false;
		}
	}

/***
============
BaseCmd_Stats_f [FWGS, 01.04.23]

testing order matches cbuf execute
============
***/
void BaseCmd_Test_f (void)
	{
	basecmd_test_stats_t stats;
	double start, end, dt;
	int i;

	stats.valid = true;
	stats.lookups = 0;

	start = Sys_DoubleTime () * 1000;

	for (i = 0; i < 1000; i++)
		{
		cmdalias_t *a;
		void *cmd;

		// Cmd_LookupCmds don't allows to check alias, so just iterate
		for (a = Cmd_AliasGetList (); a; a = a->next, stats.lookups++)
			{
			if (!BaseCmd_Find (HM_CMDALIAS, a->name))
				{
				Con_Printf ("Alias %s is missing in basecmd\n", a->name);
				stats.valid = false;
				}
			}

		for (cmd = Cmd_GetFirstFunctionHandle (); cmd;
			cmd = Cmd_GetNextFunctionHandle (cmd), stats.lookups++)
			{
			if (!BaseCmd_Find (HM_CMD, Cmd_GetName (cmd)))
				{
				Con_Printf ("Command %s is missing in basecmd\n", Cmd_GetName (cmd));
				stats.valid = false;
				}
			}

		Cvar_LookupVars (0, NULL, &stats.valid, (setpair_t)BaseCmd_CheckCvars);
		}

	end = Sys_DoubleTime () * 1000;
	dt = end - start;

	if (!stats.valid)
		Con_Printf ("BaseCmd is valid\n");

	Con_Printf ("Test took %.3f ms, %d lookups, %.3f us/lookup\n", dt, stats.lookups, dt / stats.lookups * 1000);
	BaseCmd_Stats_f ();
	}
