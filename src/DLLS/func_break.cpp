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

/***
===== bmodels.cpp ========================================================
spawn, think, and use functions for entities that use brush models
***/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "saverestore.h"
#include "func_break.h"
#include "decals.h"
#include "explode.h"

extern DLL_GLOBAL Vector g_vecAttackDir;

// =================== FUNC_Breakable ==============================================

// Just add more items to the bottom of this array and they will automagically be supported
// This is done instead of just a classname in the FGD so we can control which entities can
// be spawned, and still remain fairly flexible
const char* CBreakable::pSpawnObjects[] =
	{
	NULL,				// 0
	"item_battery",		// 1
	"item_healthkit",	// 2
	"weapon_9mmhandgun",// 3
	"ammo_9mmclip",		// 4
	"weapon_9mmAR",		// 5
	"ammo_9mmAR",		// 6
	"ammo_ARgrenades",	// 7
	"weapon_shotgun",	// 8
	"ammo_buckshot",	// 9
	"weapon_crossbow",	// 10
	"ammo_crossbow",	// 11
	"weapon_357",		// 12
	"ammo_357",			// 13
	"weapon_rpg",		// 14
	"ammo_rpgclip",		// 15
	"ammo_gaussclip",	// 16
	"weapon_handgrenade",// 17
	"weapon_tripmine",	// 18
	"weapon_satchel",	// 19
	"weapon_snark",		// 20
	"weapon_hornetgun",	// 21

	// ESHQ: ����� ����������� �������
	"weapon_crowbar",	// 22
	"weapon_gauss",		// 23
	"weapon_egon",		// 24
	"ammo_9mmbox",		// 25
	"weapon_axe",		// 26
	"monster_snark",	// 27
	"monster_babycrab",	// 28
	"monster_headcrab",	// 29
	};

void CBreakable::KeyValue (KeyValueData* pkvd)
	{
	// ESHQ: �������, �������������� �� ������ ���� �����
	/*if (FStrEq (pkvd->szKeyName, "explosion"))
		{
		if (!stricmp (pkvd->szValue, "directed"))	// ESHQ: ��������, ��������� �����
			m_Explosion = expDirected;
		else if (!stricmp (pkvd->szValue, "1"))
			m_Explosion = expDirected;
		else
			m_Explosion = expRandom;

		pkvd->fHandled = TRUE;
		}
	else*/
	if (FStrEq (pkvd->szKeyName, "material"))
		{
		int i = atoi (pkvd->szValue);

		if ((i < 0) || (i >= matLastMaterial))
			m_Material = matMetal;
		else
			m_Material = (Materials2)i;

		pkvd->fHandled = TRUE;
		}
	else if (FStrEq (pkvd->szKeyName, "gibmodel"))
		{
		m_iszGibModel = ALLOC_STRING (pkvd->szValue);
		pkvd->fHandled = TRUE;
		}
	else if (FStrEq (pkvd->szKeyName, "spawnobject"))
		{
		int object = atoi (pkvd->szValue);
		if ((object > 0) && (object < HLARRAYSIZE (pSpawnObjects)))
			m_iszSpawnObject = MAKE_STRING (pSpawnObjects[object]);
		pkvd->fHandled = TRUE;
		}
	else if (FStrEq (pkvd->szKeyName, "explodemagnitude"))
		{
		ExplosionSetMagnitude (atoi (pkvd->szValue));
		pkvd->fHandled = TRUE;
		}
	else if (FStrEq (pkvd->szKeyName, "lip"))
		{
		pkvd->fHandled = TRUE;
		}
	else
		{
		CBaseDelay::KeyValue (pkvd);
		}
	}

// func_breakable - bmodel that breaks into pieces after taking damage
LINK_ENTITY_TO_CLASS (func_breakable, CBreakable);
TYPEDESCRIPTION CBreakable::m_SaveData[] =
	{
	DEFINE_FIELD (CBreakable, m_Material, FIELD_INTEGER),
	/*DEFINE_FIELD (CBreakable, m_Explosion, FIELD_INTEGER),*/
	DEFINE_FIELD (CBreakable, m_angle, FIELD_FLOAT),
	DEFINE_FIELD (CBreakable, m_iszGibModel, FIELD_STRING),
	DEFINE_FIELD (CBreakable, m_iszSpawnObject, FIELD_STRING),
	DEFINE_FIELD (CBreakable, m_sizeFactor, FIELD_FLOAT)
	// Explosion magnitude is stored in pev->impulse
	};

IMPLEMENT_SAVERESTORE (CBreakable, CBaseEntity);

void CBreakable::Spawn (void)
	{
	Precache ();

	if (FBitSet (pev->spawnflags, SF_BREAK_TRIGGER_ONLY))
		pev->takedamage = DAMAGE_NO;
	else
		pev->takedamage = DAMAGE_YES;

	pev->solid = SOLID_BSP;
	pev->movetype = MOVETYPE_PUSH;
	m_angle = pev->angles.y;
	pev->angles.y = 0;

	// HACK: matGlass can receive decals, we need the client to know about this
	// so use class to store the material flag
	if (m_Material == matGlass)
		pev->playerclass = 1;

	SET_MODEL (ENT (pev), STRING (pev->model));	// set size and link into world

	SetTouch (&CBreakable::BreakTouch);
	if (FBitSet (pev->spawnflags, SF_BREAK_TRIGGER_ONLY))		// Only break on trigger
		SetTouch (NULL);

	// Flag unbreakable glass as "worldbrush" so it will block ALL tracelines
	if (!IsBreakable () && (pev->rendermode != kRenderNormal))
		pev->flags |= FL_WORLDBRUSH;

	// ESHQ: ������ ������� ������� ��� ������
	m_sizeFactor = (pev->size.x * pev->size.x + pev->size.y * pev->size.y + 
		pev->size.z * pev->size.z) / 6144.0;	// ������� 32 � 32 � 32 � ������� �������
	if (m_sizeFactor > 1.0)
		m_sizeFactor = 1.0;
	}

const char *CBreakable::pSoundsWood[] =
	{
	// Debris
	"debris/wood1.wav",
	"debris/wood2.wav",
	"debris/wood3.wav",

	// Hit
	"debris/wood5.wav",
	"debris/wood6.wav",
	"debris/wood7.wav",
	};

const char *CBreakable::pSoundsBustWood[] =
	{
	"debris/bustcrate1.wav",
	"debris/bustcrate2.wav",
	"debris/bustcrate3.wav",
	};

const char *CBreakable::pSoundsFlesh[] =
	{
	"debris/flesh2.wav",
	"debris/flesh3.wav",
	"debris/flesh4.wav",
	"debris/flesh5.wav",
	"debris/flesh6.wav",
	"debris/flesh7.wav",
	};

const char *CBreakable::pSoundsBustFlesh[] =
	{
	"debris/bustflesh1.wav",
	"debris/bustflesh2.wav",
	};

const char *CBreakable::pSoundsMetal[] =
	{
	// Breakable
	"debris/metal1.wav",
	"debris/metal2.wav",
	"debris/metal3.wav",
	"debris/metal4.wav",
	"debris/metal5.wav",
	"debris/metal6.wav",

	// Unbreakable
	"player/pl_metal5.wav",
	"player/pl_metal6.wav",
	"player/pl_metal7.wav",
	"player/pl_metal8.wav",
	};

const char *CBreakable::pSoundsBustMetal[] =
	{
	"debris/bustmetal1.wav",
	"debris/bustmetal2.wav",
	};

const char *CBreakable::pSoundsConcrete[] =
	{
	"debris/concrete1.wav",
	"debris/concrete2.wav",
	"debris/concrete3.wav",
	};

const char *CBreakable::pSoundsBustConcrete[] =
	{
	"debris/bustconcrete1.wav",
	"debris/bustconcrete2.wav",
	};

const char *CBreakable::pSoundsGlass[] =
	{
	"debris/glass1.wav",
	"debris/glass2.wav",
	"debris/glass3.wav",
	};

const char *CBreakable::pSoundsBustGlass[] =
	{
	"debris/bustglass1.wav",
	"debris/bustglass2.wav",
	"debris/bustglass3.wav",
	};

const char *CBreakable::pSoundsFabric[] =
	{
	"debris/fabric1.wav",
	};

const char *CBreakable::pSoundsBustFabric[] =
	{
	"debris/fabric2.wav",
	"debris/fabric3.wav",
	};

const char *CBreakable::pSoundsBustCeiling[] =
	{
	"debris/bustceiling1.wav",
	"debris/bustceiling2.wav",
	};

const char *CBreakable::pSoundsSparks[] =
	{
	"buttons/spark5.wav",
	"buttons/spark6.wav",
	};

const char** CBreakable::MaterialSoundList (Materials2 precacheMaterial, int& soundCount, qboolean Precache)
	{
	const char** pSoundList = NULL;

	switch (precacheMaterial)
		{
		// ESHQ: �����
		case matFabric:
			pSoundList = pSoundsFabric;
			soundCount = HLARRAYSIZE (pSoundsFabric);
			break;

		case matWood:
			pSoundList = pSoundsWood;
			if (Precache)
				soundCount = HLARRAYSIZE (pSoundsWood);
			else
				soundCount = 3;
			break;

		case matFlesh:
			pSoundList = pSoundsFlesh;
			soundCount = HLARRAYSIZE (pSoundsFlesh);
			break;

		case matComputer:
		case matUnbreakableGlass:
		case matGlass:
			pSoundList = pSoundsGlass;
			soundCount = HLARRAYSIZE (pSoundsGlass);
			break;

		case matMetal:
			pSoundList = pSoundsMetal;
			if (Precache)
				soundCount = HLARRAYSIZE (pSoundsMetal);
			else
				soundCount = 6;
			break;

		case matCinderBlock:
		case matRocks:
			pSoundList = pSoundsConcrete;
			soundCount = HLARRAYSIZE (pSoundsConcrete);
			break;

		case matCeilingTile:
		case matNone:
		default:
			soundCount = 0;
			break;
		}

	return pSoundList;
	}

void CBreakable::MaterialSoundPrecache (Materials2 precacheMaterial)
	{
	const char**	pSoundList;
	int				i, soundCount = 0;

	pSoundList = MaterialSoundList (precacheMaterial, soundCount, true);

	for (i = 0; i < soundCount; i++)
		PRECACHE_SOUND ((char*)pSoundList[i]);
	}

void CBreakable::MaterialSoundRandom (edict_t* pEdict, Materials2 soundMaterial, float volume)
	{
	const char**	pSoundList;
	int				soundCount = 0;

	pSoundList = MaterialSoundList (soundMaterial, soundCount, false);

	// ESHQ: ����������� ������� ��� ����� ��������
	if (soundCount)
		EMIT_SOUND (pEdict, CHAN_BODY, pSoundList[RANDOM_LONG (0, soundCount - 1)], volume, ATTN_SMALL);
	}

char *CBreakable::PrecacheSounds (Materials2 material)
	{
	char *pGibName;
	int sz, i;

	switch (material)
		{
		case matWood:
			pGibName = "models/woodgibs.mdl";

			/*PRECACHE_SOUND ("debris/ bustcrate1.wav");
			PRECACHE_SOUND ("debris/bustcrate2.wav");
			// ESHQ
			PRECACHE_SOUND ("debris/bustcrate3.wav");*/
			sz = HLARRAYSIZE (pSoundsBustWood);
			for (i = 0; i < sz; i++)
				PRECACHE_SOUND (pSoundsBustWood[i]);
			break;

		case matFlesh:
			pGibName = "models/fleshgibs.mdl";

			/*PRECACHE_SOUND ("debris/ bustflesh1.wav");
			PRECACHE_SOUND ("debris/bustflesh2.wav");*/
			sz = HLARRAYSIZE (pSoundsBustFlesh);
			for (i = 0; i < sz; i++)
				PRECACHE_SOUND (pSoundsBustFlesh[i]);
			break;

		case matComputer:
			/*PRECACHE_SOUND ("buttons/ spark5.wav");
			PRECACHE_SOUND ("buttons/spark6.wav");*/
			sz = HLARRAYSIZE (pSoundsSparks);
			for (i = 0; i < sz; i++)
				PRECACHE_SOUND (pSoundsSparks[i]);

			pGibName = "models/computergibs.mdl";

			/*PRECACHE_SOUND ("debris/ bustmetal1.wav");
			PRECACHE_SOUND ("debris/bustmetal2.wav");*/
			sz = HLARRAYSIZE (pSoundsBustMetal);
			for (i = 0; i < sz; i++)
				PRECACHE_SOUND (pSoundsBustMetal[i]);
			break;

		case matUnbreakableGlass:
		case matGlass:
			pGibName = "models/glassgibs.mdl";

			sz = HLARRAYSIZE (pSoundsBustGlass);
			for (i = 0; i < sz; i++)
				PRECACHE_SOUND (pSoundsBustGlass[i]);

			/*PRECACHE_SOUND ("debris/ bustglass1.wav");
			PRECACHE_SOUND ("debris/ bustglass2.wav");
			PRECACHE_SOUND ("debris/ bustglass3.wav");*/
			break;

		default:
		case matMetal:
			pGibName = "models/metalplategibs.mdl";

			/*PRECACHE_SOUND ("debris/ bustmetal1.wav");
			PRECACHE_SOUND ("debris/bustmetal2.wav");*/
			sz = HLARRAYSIZE (pSoundsBustMetal);
			for (i = 0; i < sz; i++)
				PRECACHE_SOUND (pSoundsBustMetal[i]);
			break;

		case matCinderBlock:
			pGibName = "models/cindergibs.mdl";

			/*PRECACHE_SOUND ("debris/ bustconcrete1.wav");
			PRECACHE_SOUND ("debris/bustconcrete2.wav");*/
			sz = HLARRAYSIZE (pSoundsBustConcrete);
			for (i = 0; i < sz; i++)
				PRECACHE_SOUND (pSoundsBustConcrete[i]);
			break;

		case matRocks:
			pGibName = "models/rockgibs.mdl";

			/*PRECACHE_SOUND ("debris/ bustconcrete1.wav");
			PRECACHE_SOUND ("debris/bustconcrete2.wav");*/
			sz = HLARRAYSIZE (pSoundsBustConcrete);
			for (i = 0; i < sz; i++)
				PRECACHE_SOUND (pSoundsBustConcrete[i]);
			break;

		case matCeilingTile:
			pGibName = "models/ceilinggibs.mdl";

			/*PRECACHE_SOUND ("debris/ bustceiling1.wav");
			PRECACHE_SOUND ("debris/bustceiling2.wav");*/
			sz = HLARRAYSIZE (pSoundsBustCeiling);
			for (i = 0; i < sz; i++)
				PRECACHE_SOUND (pSoundsBustCeiling[i]);
			break;

		case matFabric:
			pGibName = "models/fabricgibs.mdl";

			/*// ESHQ
			PRECACHE_SOUND ("player/ pl_dirt1.wav");
			PRECACHE_SOUND ("player/pl_dirt2.wav");
			PRECACHE_SOUND ("player/pl_dirt3.wav");
			PRECACHE_SOUND ("player/pl_dirt4.wav");*/
			sz = HLARRAYSIZE (pSoundsBustFabric);
			for (i = 0; i < sz; i++)
				PRECACHE_SOUND (pSoundsBustFabric[i]);
			break;
		}

	MaterialSoundPrecache (material);
	return pGibName;
	}

void CBreakable::Precache (void)
	{
	const char *pGibName;
	pGibName = PrecacheSounds (m_Material);

	if (m_iszGibModel)
		pGibName = STRING (m_iszGibModel);

	m_idShard = PRECACHE_MODEL ((char*)pGibName);

	// Precache the spawn item's data
	if (m_iszSpawnObject)
		UTIL_PrecacheOther ((char*)STRING (m_iszSpawnObject));
	}

// ESHQ: ��������� � ������ ������ ������� �� ������� �������
float CBreakable::GetVolume (void)
	{
	return RANDOM_FLOAT (0.20, 0.35) + 0.6 * m_sizeFactor;
	}

int CBreakable::GetPitch (void)
	{
	return RANDOM_LONG (130, 145) - (int)(45 * m_sizeFactor);
	}

// Play shard sound when func_breakable takes damage
void CBreakable::DamageSound (void)
	{
	MakeDamageSound (m_Material, GetVolume (), GetPitch (), ENT (pev), true);
	}

void CBreakable::MakeDamageSound (Materials2 material, float volume, int pitch, edict_t *entity, qboolean breakable)
	{
	/*int pitch;
	float fvol;*/
	char* rgpsz[6];
	int i, j;
	int mt = material;

	/*// ESHQ: ��������� � ������ ������ ������� �� ������� �������
	fvol = GetVolume ();
	pitch = GetPitch ();*/

	if ((mt == matComputer) && RANDOM_LONG (0, 1))
		mt = matMetal;

	// ESHQ: ����� ����������� ������� breakable
	switch (mt)
		{
		case matComputer:
		case matGlass:
		case matUnbreakableGlass:
			/*rgpsz[0] = "debris/glass1.wav";
			rgpsz[1] = "debris/glass2.wav";
			rgpsz[2] = "debris/glass3.wav";
			i = 3;*/
			i = HLARRAYSIZE (pSoundsGlass);
			for (int j = 0; j < i; j++)
				rgpsz[j] = (char *)pSoundsGlass[j];
			break;

		case matWood:
			/*rgpsz[0] = "debris/wood5.wav";
			rgpsz[1] = "debris/wood6.wav";
			rgpsz[2] = "debris/wood7.wav";
			i = 3;*/

			// ESHQ: ������ ���������� ������ ������� �� ��� � ����� �� �����������
			i = 3;
			for (int j = 0; j < i; j++)
				rgpsz[j] = (char *)pSoundsWood[j + 3];
			break;

		default:
		case matMetal:
			/*rgpsz[0] = "debris/metal1.wav";
			rgpsz[1] = "debris/metal3.wav";*/

			if (breakable)
				{
				rgpsz[0] = (char *)pSoundsMetal[0];
				rgpsz[1] = (char *)pSoundsMetal[2];
				i = 2;
				}
			else
				{
				rgpsz[0] = (char *)pSoundsMetal[6];
				rgpsz[1] = (char *)pSoundsMetal[7];
				rgpsz[2] = (char *)pSoundsMetal[8];
				rgpsz[3] = (char *)pSoundsMetal[9];
				i = 4;
				}
			break;

		case matFlesh:
			/*rgpsz[0] = "debris/flesh2.wav";
			rgpsz[1] = "debris/flesh3.wav";
			rgpsz[2] = "debris/flesh4.wav";
			rgpsz[3] = "debris/flesh5.wav";
			rgpsz[4] = "debris/flesh6.wav";
			rgpsz[5] = "debris/flesh7.wav";
			i = 6;*/
			i = HLARRAYSIZE (pSoundsFlesh);
			for (int j = 0; j < i; j++)
				rgpsz[j] = (char *)pSoundsFlesh[j];
			break;

		case matRocks:
		case matCinderBlock:
			/*rgpsz[0] = "debris/concrete1.wav";
			rgpsz[1] = "debris/concrete2.wav";
			rgpsz[2] = "debris/concrete3.wav";
			i = 3;*/
			i = HLARRAYSIZE (pSoundsConcrete);
			for (int j = 0; j < i; j++)
				rgpsz[j] = (char *)pSoundsConcrete[j];
			break;

		// ESHQ: ����� ���������
		case matFabric:
			i = HLARRAYSIZE (pSoundsFabric);
			for (int j = 0; j < i; j++)
				rgpsz[j] = (char *)pSoundsFabric[j];
			break;

		case matCeilingTile:
			i = 0;
			rgpsz[0] = "-";
			break;
		}

	if (i)
		EMIT_SOUND_DYN (entity, CHAN_VOICE, rgpsz[RANDOM_LONG (0, i - 1)], volume, ATTN_MEDIUM, 0, pitch);
	}

void CBreakable::BreakTouch (CBaseEntity* pOther)
	{
	float		flDamage;
	entvars_t	*pevToucher = pOther->pev;

	// only players can break these right now
	if (!pOther->IsPlayer () || !IsBreakable ())
		return;

	if (FBitSet (pev->spawnflags, SF_BREAK_TOUCH))
		{// can be broken when run into 
		flDamage = pevToucher->velocity.Length () * 0.01;

		if (flDamage >= pev->health)
			{
			SetTouch (NULL);
			TakeDamage (pevToucher, pevToucher, flDamage, DMG_CRUSH);

			// do a little damage to player if we broke glass or computer
			pOther->TakeDamage (pev, pev, flDamage / 4, DMG_SLASH);
			}
		}

	// can be broken when stood upon
	if (FBitSet (pev->spawnflags, SF_BREAK_PRESSURE) && (pevToucher->absmin.z >= pev->maxs.z - 2))
		{
		// play creaking sound here
		DamageSound ();

		SetThink (&CBreakable::Die);
		SetTouch (NULL);

		if (m_flDelay == 0)
			m_flDelay = 0.1;

		pev->nextthink = pev->ltime + m_flDelay;
		}
	}

// Break when triggered
void CBreakable::Use (CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
	{
	if (!IsBreakable ())
		return;

	pev->angles.y = m_angle;
	UTIL_MakeVectors (pev->angles);
	g_vecAttackDir = gpGlobals->v_forward;

	Die ();
	}

void CBreakable::TraceAttack (entvars_t* pevAttacker, float flDamage, Vector vecDir,
	TraceResult* ptr, int bitsDamageType)
	{
	float flVolume;
	int i;

	// ESHQ: �������������� ����� ��� ����� �� �������
	if (m_Material == matMetal)
		UTIL_Sparks (ptr->vecEndPos);

	// ESHQ: ��������� ����� ��� ����� �� ����������� � ������������� ������
	if (RANDOM_LONG (0, 1))
		{
		switch (m_Material)
			{
			// �����������
			case matComputer:
				{
				UTIL_Sparks (ptr->vecEndPos);

				flVolume = RANDOM_FLOAT (0.7, 1.0);
				/*switch (RANDOM_LONG (0, 1))
					{
					case 0: 
						EMIT_SOUND (ENT (pev), CHAN_VOICE, "buttons/ spark5.wav", flVolume, ATTN_MEDIUM);	
						break;

					case 1: 
						EMIT_SOUND (ENT (pev), CHAN_VOICE, "buttons/spark6.wav", flVolume, ATTN_MEDIUM);	
						break;
					}*/
				i = RANDOM_LONG (0, HLARRAYSIZE (pSoundsSparks) - 1);
				EMIT_SOUND (ENT (pev), CHAN_VOICE, pSoundsSparks[i], flVolume, ATTN_MEDIUM);
				}
				break;

			// ������������ ������
			case matUnbreakableGlass:
				UTIL_Ricochet (ptr->vecEndPos, RANDOM_FLOAT (0.5, 1.5));
				break;
			}
		}

	CBaseDelay::TraceAttack (pevAttacker, flDamage, vecDir, ptr, bitsDamageType);
	}

// =========================================================
// Special takedamage for func_breakable. Allows us to make
// exceptions that are breakable-specific.
// bitsDamageType indicates the type of damage sustained ie: DMG_CRUSH
// =========================================================
int CBreakable::TakeDamage (entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
	{
	Vector	vecTemp;

	// if Attacker == Inflictor, the attack was a melee or other instant-hit attack.
	// (that is, no actual entity projectile was involved in the attack so use the shooter's origin)
	if (pevAttacker == pevInflictor)
		{
		vecTemp = pevInflictor->origin - (pev->absmin + (pev->size * 0.5));

		// if a client hit the breakable with a crowbar, and breakable is crowbar-sensitive, break it now
		if (FBitSet (pevAttacker->flags, FL_CLIENT) &&
			FBitSet (pev->spawnflags, SF_BREAK_CROWBAR) && (bitsDamageType & DMG_CLUB))
			flDamage = pev->health;
		}
	else
		// an actual missile was involved
		{
		vecTemp = pevInflictor->origin - (pev->absmin + (pev->size * 0.5));
		}

	if (!IsBreakable ())
		return 0;

	// Breakables take double damage from the crowbar
	if (bitsDamageType & DMG_CLUB)
		flDamage *= 2;

	// Boxes / glass / etc. don't take much poison damage, just the impact of the dart - consider that 10%
	if (bitsDamageType & DMG_POISON)
		flDamage *= 0.1;

	// this global is still used for glass and other non-monster killables, along with decals
	g_vecAttackDir = vecTemp.Normalize ();

	// do the damage
	pev->health -= flDamage;
	if (pev->health <= 0)
		{
		Killed (pevAttacker, GIB_NORMAL);
		pev->armortype = flDamage;	// ESHQ: hack, I guess
		Die ();
		return 0;
		}

	// Make a shard noise each time func_breakable is hit.
	// Don't play shard noise if breakable actually died
	DamageSound ();
	return 1;
	}

unsigned char CBreakable::MakeBustSound (Materials2 material, float volume, int pitch, edict_t *entity)
	{
	int i;
	unsigned char cFlag;

	switch (material)
		{
		case matGlass:
			/*switch (RANDOM_LONG (0, 2))
				{
				case 0:
					EMIT_SOUND_DYN (ENT (pev), CHAN_VOICE, "debris/ bustglass1.wav", fvol, ATTN_MEDIUM, 0, pitch);
					break;
				case 1:
					EMIT_SOUND_DYN (ENT (pev), CHAN_VOICE, "debris/ bustglass2.wav", fvol, ATTN_MEDIUM, 0, pitch);
					break;
				case 2:
					EMIT_SOUND_DYN (ENT (pev), CHAN_VOICE, "debris/ bustglass3.wav", fvol, ATTN_MEDIUM, 0, pitch);
					break;
				}*/
			i = RANDOM_LONG (0, HLARRAYSIZE (pSoundsBustGlass) - 1);
			EMIT_SOUND_DYN (entity, CHAN_VOICE, pSoundsBustGlass[i], volume, ATTN_MEDIUM, 0, pitch);
			cFlag = BREAK_GLASS;
			break;

		case matWood:
			/*switch (RANDOM_LONG (0, 2))
				{
				case 0:
					EMIT_SOUND_DYN (ENT (pev), CHAN_VOICE, "debris/ bustcrate1.wav", fvol, ATTN_MEDIUM, 0, pitch);
					break;
				case 1:
					EMIT_SOUND_DYN (ENT (pev), CHAN_VOICE, "debris/bustcrate2.wav", fvol, ATTN_MEDIUM, 0, pitch);
					break;
				case 2:
					EMIT_SOUND_DYN (ENT (pev), CHAN_VOICE, "debris/bustcrate3.wav", fvol, ATTN_MEDIUM, 0, pitch);
					break;
				}*/
			i = RANDOM_LONG (0, HLARRAYSIZE (pSoundsBustWood) - 1);
			EMIT_SOUND_DYN (entity, CHAN_VOICE, pSoundsBustWood[i], volume, ATTN_MEDIUM, 0, pitch);
			cFlag = BREAK_WOOD;
			break;

		default:
		case matComputer:
		case matMetal:
			/*switch (RANDOM_LONG (0, 1))
				{
				case 0:
					EMIT_SOUND_DYN (ENT (pev), CHAN_VOICE, "debris/ bustmetal1.wav", fvol, ATTN_MEDIUM, 0, pitch);
					break;
				case 1:
					EMIT_SOUND_DYN (ENT (pev), CHAN_VOICE, "debris/bustmetal2.wav", fvol, ATTN_MEDIUM, 0, pitch);
					break;
				}*/
			i = RANDOM_LONG (0, HLARRAYSIZE (pSoundsBustMetal) - 1);
			EMIT_SOUND_DYN (entity, CHAN_VOICE, pSoundsBustMetal[i], volume, ATTN_MEDIUM, 0, pitch);
			cFlag = BREAK_METAL;
			break;

		case matFlesh:
			/*switch (RANDOM_LONG (0, 1))
				{
				case 0:
					EMIT_SOUND_DYN (ENT (pev), CHAN_VOICE, "debris/ bustflesh1.wav", fvol, ATTN_MEDIUM, 0, pitch);
					break;
				case 1:
					EMIT_SOUND_DYN (ENT (pev), CHAN_VOICE, "debris/bustflesh2.wav", fvol, ATTN_MEDIUM, 0, pitch);
					break;
				}*/
			i = RANDOM_LONG (0, HLARRAYSIZE (pSoundsBustFlesh) - 1);
			EMIT_SOUND_DYN (entity, CHAN_VOICE, pSoundsBustFlesh[i], volume, ATTN_MEDIUM, 0, pitch);
			cFlag = BREAK_FLESH;
			break;

		case matRocks:
		case matCinderBlock:
			/*switch (RANDOM_LONG (0, 1))
				{
				case 0:
					EMIT_SOUND_DYN (ENT (pev), CHAN_VOICE, "debris /bustconcrete1.wav", fvol, ATTN_MEDIUM, 0, pitch);
					break;
				case 1:
					EMIT_SOUND_DYN (ENT (pev), CHAN_VOICE, "debris/bustconcrete2.wav", fvol, ATTN_MEDIUM, 0, pitch);
					break;
				}*/
			i = RANDOM_LONG (0, HLARRAYSIZE (pSoundsBustConcrete) - 1);
			EMIT_SOUND_DYN (entity, CHAN_VOICE, pSoundsBustConcrete[i], volume, ATTN_MEDIUM, 0, pitch);
			cFlag = BREAK_CONCRETE;
			break;

		case matCeilingTile:
			/*switch (RANDOM_LONG (0, 1))
				{
				case 0:
					EMIT_SOUND_DYN (ENT (pev), CHAN_VOICE, "debris/ bustceiling1.wav", fvol, ATTN_MEDIUM, 0, pitch);
					break;
				case 1:
					EMIT_SOUND_DYN (ENT (pev), CHAN_VOICE, "debris/bustceiling2.wav", fvol, ATTN_MEDIUM, 0, pitch);
					break;
				}*/
			i = RANDOM_LONG (0, HLARRAYSIZE (pSoundsBustCeiling) - 1);
			EMIT_SOUND_DYN (entity, CHAN_VOICE, pSoundsBustCeiling[i], volume, ATTN_MEDIUM, 0, pitch);
			/*cFlag = BREAK_CONCRETE;*/
			cFlag = 0;
			break;

		case matFabric:
			/*switch (RANDOM_LONG (0, 1))
				{
				case 0:
					EMIT_SOUND_DYN (ENT (pev), CHAN_VOICE, "player/ pl_dirt1.wav", fvol, ATTN_MEDIUM, 0, pitch);
					break;
				case 1:
					EMIT_SOUND_DYN (ENT (pev), CHAN_VOICE, "player/pl_dirt2.wav", fvol, ATTN_MEDIUM, 0, pitch);
					break;
				case 2:
					EMIT_SOUND_DYN (ENT (pev), CHAN_VOICE, "player/pl_dirt3.wav", fvol, ATTN_MEDIUM, 0, pitch);
					break;
				case 3:
					EMIT_SOUND_DYN (ENT (pev), CHAN_VOICE, "player/pl_dirt4.wav", fvol, ATTN_MEDIUM, 0, pitch);
					break;
				}*/
			i = RANDOM_LONG (0, HLARRAYSIZE (pSoundsBustFabric) - 1);
			EMIT_SOUND_DYN (entity, CHAN_VOICE, pSoundsBustFabric[i], volume, ATTN_MEDIUM, 0, pitch);
			/*cFlag = BREAK_FLESH;*/
			cFlag = 0;
			break;
		}

	return cFlag;
	}

void CBreakable::Die (void)
	{
	Vector vecSpot;		// shard origin
	Vector vecVelocity;	// shard velocity
	CBaseEntity* pEntity = NULL;
	char cFlag = 0;
	/*int pitch, i;
	float fvol;*/
	CBaseEntity *pOnBreak;
	float ampl, freq, duration;

	/*// ESHQ: ��������� � ������ ������ ������� �� ������� �������
	fvol = GetVolume ();
	pitch = GetPitch ();*/

	cFlag = MakeBustSound (m_Material, GetVolume (), GetPitch (), ENT (pev));

	/*if (m_Explosion == expDirected)
		{*/
	// ������� �������� (armortype �� ����� ���� ������ ��� ������ �� ������� �������)
	ampl = pev->armortype * 15.0f;
	if (ampl > 400.0f)
		ampl = 400.0f;

	vecVelocity = g_vecAttackDir * -ampl;
	/*}
	else
		{
		vecVelocity.x = 0;
		vecVelocity.y = 0;
		vecVelocity.z = 0;
		}*/

	vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;
	MESSAGE_BEGIN (MSG_PVS, SVC_TEMPENTITY, vecSpot);
	WRITE_BYTE (TE_BREAKMODEL);

	// position
	WRITE_COORD (vecSpot.x);
	WRITE_COORD (vecSpot.y);
	WRITE_COORD (vecSpot.z);

	// size
	WRITE_COORD (pev->size.x);
	WRITE_COORD (pev->size.y);
	WRITE_COORD (pev->size.z);

	// velocity
	WRITE_COORD (vecVelocity.x);
	WRITE_COORD (vecVelocity.y);
	WRITE_COORD (vecVelocity.z);

	// randomization
	WRITE_BYTE (10);

	// Model
	WRITE_SHORT (m_idShard);	// model id#

	// # of shards
	WRITE_BYTE (0);		// let client decide

	// duration
	WRITE_BYTE (50);	// ESHQ: ��������� �� 5 ������

	// flags
	WRITE_BYTE (cFlag);
	MESSAGE_END ();

	float size = pev->size.x;
	if (size < pev->size.y)
		size = pev->size.y;
	if (size < pev->size.z)
		size = pev->size.z;

	// !!! HACK  This should work!
	// Build a box above the entity that looks like an 8 pixel high sheet
	Vector mins = pev->absmin;
	Vector maxs = pev->absmax;
	mins.z = pev->absmax.z;
	maxs.z += 8;

	// BUGBUG -- can only find 256 entities on a breakable - should be enough
	CBaseEntity* pList[256];
	int count = UTIL_EntitiesInBox (pList, 256, mins, maxs, FL_ONGROUND);
	if (count)
		{
		for (int i = 0; i < count; i++)
			{
			ClearBits (pList[i]->pev->flags, FL_ONGROUND);
			pList[i]->pev->groundentity = NULL;
			}
		}

	// Don't fire something that could fire myself
	pev->targetname = 0;
	pev->solid = SOLID_NOT;

	// Fire targets on break
	SUB_UseTargets (NULL, USE_TOGGLE, 0);

	SetThink (&CBaseEntity::SUB_Remove);
	pev->nextthink = pev->ltime + 0.1;

	if (m_iszSpawnObject)
		{
		// ESHQ: ��� ������ ��������� ��������� ������� ��� ���������� � ������������� ������� (������ �� ���������)
		if ((pev->origin.x == 0) && (pev->origin.y == 0) || !FClassnameIs (pev, "func_breakable"))
			pOnBreak = CBaseEntity::Create ((char*)STRING (m_iszSpawnObject), VecBModelOrigin (pev), 
				pev->angles, edict ());
		else
			pOnBreak = CBaseEntity::Create ((char*)STRING (m_iszSpawnObject), pev->origin, 
				pev->angles, edict ());
		}

	if (Explodable ())
		{
		ExplosionCreate (Center (), pev->angles, edict (), ExplosionMagnitude (), TRUE);

		// ESHQ: ���������� ����� � ������� ������
		ampl = ExplosionMagnitude () / 20.0f;
		if (ampl > 16.0f) 
			ampl = 16.0f;

		freq = ExplosionMagnitude () / 10.0f;
		if (freq > 60.0f) 
			freq = 60.0f;
		
		duration = ExplosionMagnitude () / 75.0f;
		if (duration > 4.0f) 
			duration = 4.0f;

		UTIL_ScreenShake (VecBModelOrigin (pev), ampl, freq, duration, ExplosionMagnitude () * 3.0f);
		}
	}

BOOL CBreakable::IsBreakable (void)
	{
	return (m_Material != matUnbreakableGlass);
	}

int	CBreakable::DamageDecal (int bitsDamageType)
	{
	if (m_Material == matGlass)
		return DECAL_GLASSBREAK1 + RANDOM_LONG (0, 2);

	if (m_Material == matUnbreakableGlass)
		return DECAL_BPROOF1;

	return CBaseEntity::DamageDecal (bitsDamageType);
	}

// ESHQ: ��������� ������ ������ �����������
#define PUSH_SOUND_NAMES	9

class CPushable: public CBreakable
	{
	public:
		void	Spawn (void);
		void	Precache (void);
		void	Touch (CBaseEntity* pOther);
		void	Move (CBaseEntity* pMover, int push);
		void	KeyValue (KeyValueData* pkvd);
		void	Use (CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value);
		void	EXPORT StopSound (void);

		virtual int	ObjectCaps (void) { return (CBaseEntity::ObjectCaps () & ~FCAP_ACROSS_TRANSITION) | FCAP_CONTINUOUS_USE; }
		virtual int		Save (CSave& save);
		virtual int		Restore (CRestore& restore);

		inline float MaxSpeed (void) { return m_maxSpeed; }

		// breakables use an overridden takedamage
		virtual int TakeDamage (entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);

		static	TYPEDESCRIPTION m_SaveData[];

		// ESHQ
		static char* m_soundNames[PUSH_SOUND_NAMES];

		// no need to save/restore, just keeps the same sound from playing twice in a row
		int		m_lastSound;
		float	m_maxSpeed;
		float	m_soundTime;
		Vector	shoot_dir;

	};

TYPEDESCRIPTION	CPushable::m_SaveData[] =
	{
		DEFINE_FIELD (CPushable, m_maxSpeed, FIELD_FLOAT),
		DEFINE_FIELD (CPushable, m_soundTime, FIELD_TIME),
	};

IMPLEMENT_SAVERESTORE (CPushable, CBreakable);

LINK_ENTITY_TO_CLASS (func_pushable, CPushable);

// ESHQ: ��������� ������ ������ �����������
char* CPushable::m_soundNames[PUSH_SOUND_NAMES] =
	{
	"debris/pushbox1.wav",	// �������
	"debris/pushbox2.wav",
	"debris/pushbox3.wav",
	"debris/pushbox4.wav",	// ������
	"debris/pushbox5.wav",
	"debris/pushbox6.wav",
	"debris/pushbox7.wav",	// ������
	"debris/pushbox8.wav",
	"debris/pushbox9.wav",
	};

void CPushable::Spawn (void)
	{
	CBreakable::Spawn ();

	pev->movetype = MOVETYPE_PUSHSTEP;
	pev->solid = SOLID_BBOX;
	SET_MODEL (ENT (pev), STRING (pev->model));

	if (pev->friction > 399)
		pev->friction = 399;

	m_maxSpeed = 400 - pev->friction;
	SetBits (pev->flags, FL_FLOAT);
	pev->friction = 0;

	pev->origin.z += 1;	// Pick up off of the floor
	UTIL_SetOrigin (pev, pev->origin);

	// Multiply by area of the box's cross-section (assume 1000 units^3 standard volume)
	pev->skin = (pev->skin * (pev->maxs.x - pev->mins.x) * (pev->maxs.y - pev->mins.y)) * 0.0005;
	m_soundTime = 0;
	}

void CPushable::Precache (void)
	{
	for (int i = 0; i < PUSH_SOUND_NAMES; i++)
		PRECACHE_SOUND (m_soundNames[i]);

	if (pev->spawnflags & SF_PUSH_BREAKABLE)
		CBreakable::Precache ();
	}

void CPushable::KeyValue (KeyValueData* pkvd)
	{
	// ESHQ: size ����� �� �������������
	if (FStrEq (pkvd->szKeyName, "buoyancy"))
		{
		pev->skin = atof (pkvd->szValue);
		pkvd->fHandled = TRUE;
		}
	else
		{
		CBreakable::KeyValue (pkvd);
		}
	}

// Pull the func_pushable
void CPushable::Use (CBaseEntity* pActivator, CBaseEntity* pCaller, USE_TYPE useType, float value)
	{
	if (!pActivator || !pActivator->IsPlayer ())
		{
		if (pev->spawnflags & SF_PUSH_BREAKABLE)
			this->CBreakable::Use (pActivator, pCaller, useType, value);
		return;
		}

	if (pActivator->pev->velocity != g_vecZero)
		Move (pActivator, 0);
	}

void CPushable::Touch (CBaseEntity* pOther)
	{
	if (FClassnameIs (pOther->pev, "worldspawn"))
		return;

	Move (pOther, 1);
	}

// ESHQ: ���������� ��� ������� �� ��������
void CPushable::Move (CBaseEntity* pOther, int push)
	{
	entvars_t* pevToucher = pOther->pev;
	int playerTouch = 0;

	// Is entity standing on this pushable?
	if (FBitSet (pevToucher->flags, FL_ONGROUND) && pevToucher->groundentity && (VARS (pevToucher->groundentity) == pev))
		{
		// Only push if floating
		if (pev->waterlevel > 0)
			pev->velocity.z += pevToucher->velocity.z * 0.1;

		return;
		}

	// g-cont. fix pushable acceleration bug
	if (pOther->IsPlayer ())
		{
		// Don't push unless the player is pushing forward and NOT use (pull)
		if ((push == 1) && !(pevToucher->button & (IN_FORWARD | IN_MOVERIGHT | IN_MOVELEFT | IN_BACK)))
			return;
		if ((push == 0) && !(pevToucher->button & (IN_BACK)))
			return;

		playerTouch = 1;
		}

	float factor;

	if (playerTouch)
		{
		// Don't push away from jumping/falling players unless in water
		if (!(pevToucher->flags & FL_ONGROUND))
			{
			if (pev->waterlevel < 1)
				return;
			else
				factor = 0.1;
			}
		else
			{
			factor = 1;
			}
		}
	else
		{
		factor = 0.25;
		}

	if (push < 2)
		{
		pev->velocity.x += pevToucher->velocity.x * factor;
		pev->velocity.y += pevToucher->velocity.y * factor;
		}
	else
		{
		pev->velocity.x += shoot_dir.x * factor;
		pev->velocity.y += shoot_dir.y * factor;
		}

	float length = sqrt (pev->velocity.x * pev->velocity.x + pev->velocity.y * pev->velocity.y);

	// ESHQ: ����������� ����������� ���� ���������
	if (push && (length > (MaxSpeed () * push * push)))
		{
		pev->velocity.x = (pev->velocity.x * MaxSpeed () / length);
		pev->velocity.y = (pev->velocity.y * MaxSpeed () / length);
		}

	if (playerTouch)
		{
		if (push == 1)
			{
			pevToucher->velocity.x = pev->velocity.x;
			pevToucher->velocity.y = pev->velocity.y;
			}

		if ((gpGlobals->time - m_soundTime) > 0.7)
			{
			m_soundTime = gpGlobals->time;

			// ESHQ: ��������� ��������� (� ��������� ���������� �������)
			if ((pev->health > 0) && (length > 0) && FBitSet (pev->flags, FL_ONGROUND))
				{
				switch (m_Material)
					{
					case matGlass:
					case matCeilingTile:
					case matUnbreakableGlass:
					default:
						m_lastSound = RANDOM_LONG (0, 2);
						break;

					case matWood:
					case matFlesh:
					case matFabric:
						m_lastSound = RANDOM_LONG (3, 5);
						break;

					case matMetal:
					case matCinderBlock:
					case matComputer:
					case matRocks:
						m_lastSound = RANDOM_LONG (6, 8);
						break;
					}

				// ������ ����� ������ �������� ��������� � ������������� ���������
				EMIT_SOUND (ENT (pev), CHAN_WEAPON, m_soundNames[m_lastSound], 0.5, ATTN_MEDIUM);
				SetThink (&CPushable::StopSound);
				pev->nextthink = pev->ltime + 0.1;
				}
			else
				{
				STOP_SOUND (ENT (pev), CHAN_WEAPON, m_soundNames[m_lastSound]);
				}
			}
		}
	}

// ESHQ: ��������� ��������� ������
void CPushable::StopSound (void)
	{
	pev->nextthink = pev->ltime + 0.1;

	// ���������� ����, ���� �������� ������������; �������� �������
	if (((int)(pev->velocity.x * 100.0f) == 0) && ((int)(pev->velocity.y * 100.0f) == 0))
		{
		STOP_SOUND (ENT (pev), CHAN_WEAPON, m_soundNames[m_lastSound]);
		SetThink (NULL);
		}
	}

int CPushable::TakeDamage (entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
	{
	// ����������� ���������
	int res = 1;
	if (pev->spawnflags & SF_PUSH_BREAKABLE)
		res = CBreakable::TakeDamage (pevInflictor, pevAttacker, flDamage, bitsDamageType);

	// ESHQ: ���������� ������� �� �������
	if (!FNullEnt (pevInflictor))
		{
		CBaseEntity *pInflictor = CBaseEntity::Instance (pevInflictor);
		
		if (pInflictor)
			{
			shoot_dir = (pInflictor->Center () - Vector (0, 0, 10) - Center ()).Normalize ();
			shoot_dir = shoot_dir.Normalize () * -flDamage * 50.0f;

			Move (pInflictor, 2);
			}
		}

	return res;
	}
