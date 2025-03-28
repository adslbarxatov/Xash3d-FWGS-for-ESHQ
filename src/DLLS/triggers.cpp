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
===== triggers.cpp ========================================================

spawn and use functions for editor-placed triggers
***/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "saverestore.h"
#include "trains.h"			// trigger_camera has train functionality
#include "gamerules.h"

#define	SF_TRIGGER_PUSH_START_OFF	2		// spawnflag that makes trigger_push spawn turned OFF
#define SF_TRIGGER_HURT_TARGETONCE	1		// Only fire hurt target once
#define	SF_TRIGGER_HURT_START_OFF	2		// spawnflag that makes trigger_push spawn turned OFF
#define	SF_TRIGGER_HURT_NO_CLIENTS	8		// spawnflag that makes trigger_push spawn turned OFF
#define SF_TRIGGER_HURT_CLIENTONLYFIRE	16	// trigger hurt will only fire its target if it is hurting a client
#define SF_TRIGGER_HURT_CLIENTONLYTOUCH 32	// only clients may touch this trigger

extern DLL_GLOBAL BOOL		g_fGameOver;

extern void SetMovedir (entvars_t *pev);
extern Vector VecBModelOrigin (entvars_t *pevBModel);

class CFrictionModifier : public CBaseEntity
	{
	public:
		void		Spawn (void);
		void		KeyValue (KeyValueData *pkvd);
		void EXPORT	ChangeFriction (CBaseEntity *pOther);
		virtual int		Save (CSave &save);
		virtual int		Restore (CRestore &restore);

		virtual int	ObjectCaps (void) { return CBaseEntity::ObjectCaps () & ~FCAP_ACROSS_TRANSITION; }

		static	TYPEDESCRIPTION m_SaveData[];

		float		m_frictionFraction;		// Sorry, couldn't resist this name :)
	};

LINK_ENTITY_TO_CLASS (func_friction, CFrictionModifier);

// Global Savedata for changelevel friction modifier
TYPEDESCRIPTION	CFrictionModifier::m_SaveData[] =
	{
		DEFINE_FIELD (CFrictionModifier, m_frictionFraction, FIELD_FLOAT),
	};

IMPLEMENT_SAVERESTORE (CFrictionModifier, CBaseEntity);


// Modify an entity's friction
void CFrictionModifier::Spawn (void)
	{
	pev->solid = SOLID_TRIGGER;
	SET_MODEL (ENT (pev), STRING (pev->model));    // set size and link into world
	pev->movetype = MOVETYPE_NONE;
	SetTouch (&CFrictionModifier::ChangeFriction);
	}

// Sets toucher's friction to m_frictionFraction (1.0 = normal friction)
void CFrictionModifier::ChangeFriction (CBaseEntity *pOther)
	{
	if ((pOther->pev->movetype != MOVETYPE_BOUNCEMISSILE) && (pOther->pev->movetype != MOVETYPE_BOUNCE))
		pOther->pev->friction = m_frictionFraction;
	}

// Sets toucher's friction to m_frictionFraction (1.0 = normal friction)
void CFrictionModifier::KeyValue (KeyValueData *pkvd)
	{
	if (FStrEq (pkvd->szKeyName, "modifier"))
		{
		m_frictionFraction = atof (pkvd->szValue) / 100.0;
		pkvd->fHandled = TRUE;
		}
	else
		{
		CBaseEntity::KeyValue (pkvd);
		}
	}

// This trigger will fire when the level spawns (or respawns if not fire once)
// It will check a global state before firing.  It supports delay and killtargets

#define SF_AUTO_FIREONCE		0x0001

class CAutoTrigger : public CBaseDelay
	{
	public:
		void KeyValue (KeyValueData *pkvd);
		void Spawn (void);
		void Precache (void);
		void Think (void);

		int ObjectCaps (void) { return CBaseDelay::ObjectCaps () & ~FCAP_ACROSS_TRANSITION; }
		virtual int		Save (CSave &save);
		virtual int		Restore (CRestore &restore);

		static	TYPEDESCRIPTION m_SaveData[];

	private:
		int			m_globalstate;
		USE_TYPE	triggerType;
	};
LINK_ENTITY_TO_CLASS (trigger_auto, CAutoTrigger);

TYPEDESCRIPTION	CAutoTrigger::m_SaveData[] =
	{
		DEFINE_FIELD (CAutoTrigger, m_globalstate, FIELD_STRING),
		DEFINE_FIELD (CAutoTrigger, triggerType, FIELD_INTEGER),
	};

IMPLEMENT_SAVERESTORE (CAutoTrigger, CBaseDelay);

void CAutoTrigger::KeyValue (KeyValueData *pkvd)
	{
	if (FStrEq (pkvd->szKeyName, "globalstate"))
		{
		m_globalstate = ALLOC_STRING (pkvd->szValue);
		pkvd->fHandled = TRUE;
		}
	else if (FStrEq (pkvd->szKeyName, "triggerstate"))
		{
		int type = atoi (pkvd->szValue);
		switch (type)
			{
			case 0:
				triggerType = USE_OFF;
				break;
			case 2:
				triggerType = USE_TOGGLE;
				break;
			default:
				triggerType = USE_ON;
				break;
			}
		pkvd->fHandled = TRUE;
		}
	else
		CBaseDelay::KeyValue (pkvd);
	}

void CAutoTrigger::Spawn (void)
	{
	Precache ();
	}

void CAutoTrigger::Precache (void)
	{
	pev->nextthink = gpGlobals->time + 0.1;
	}

void CAutoTrigger::Think (void)
	{
	if (!m_globalstate || gGlobalState.EntityGetState (m_globalstate) == GLOBAL_ON)
		{
		SUB_UseTargets (this, triggerType, 0);
		if (pev->spawnflags & SF_AUTO_FIREONCE)
			UTIL_Remove (this);
		}
	}

#define SF_RELAY_FIREONCE		0x0001

class CTriggerRelay : public CBaseDelay
	{
	public:
		void KeyValue (KeyValueData *pkvd);
		void Spawn (void);
		void Use (CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

		int ObjectCaps (void) { return CBaseDelay::ObjectCaps () & ~FCAP_ACROSS_TRANSITION; }
		virtual int		Save (CSave &save);
		virtual int		Restore (CRestore &restore);

		static	TYPEDESCRIPTION m_SaveData[];

	private:
		USE_TYPE	triggerType;
	};
LINK_ENTITY_TO_CLASS (trigger_relay, CTriggerRelay);

TYPEDESCRIPTION	CTriggerRelay::m_SaveData[] =
	{
		DEFINE_FIELD (CTriggerRelay, triggerType, FIELD_INTEGER),
	};

IMPLEMENT_SAVERESTORE (CTriggerRelay, CBaseDelay);

void CTriggerRelay::KeyValue (KeyValueData *pkvd)
	{
	if (FStrEq (pkvd->szKeyName, "triggerstate"))
		{
		int type = atoi (pkvd->szValue);
		switch (type)
			{
			case 0:
				triggerType = USE_OFF;
				break;
			case 2:
				triggerType = USE_TOGGLE;
				break;
			default:
				triggerType = USE_ON;
				break;
			}
		pkvd->fHandled = TRUE;
		}
	else
		CBaseDelay::KeyValue (pkvd);
	}

void CTriggerRelay::Spawn (void)
	{
	}

void CTriggerRelay::Use (CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
	{
	SUB_UseTargets (this, triggerType, 0);
	if (pev->spawnflags & SF_RELAY_FIREONCE)
		UTIL_Remove (this);
	}

// **********************************************************
// The Multimanager Entity - when fired, will fire up to 16 targets 
// at specified times.
// FLAG:		THREAD (create clones when triggered)
// FLAG:		CLONE (this is a clone for a threaded execution)

#define SF_MULTIMAN_CLONE		0x80000000
#define SF_MULTIMAN_THREAD		0x00000001

class CMultiManager : public CBaseToggle
	{
	public:
		void KeyValue (KeyValueData *pkvd);
		void Spawn (void);
		void HLEXPORT ManagerThink (void);
		void HLEXPORT ManagerUse (CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

#if _DEBUG
		void HLEXPORT ManagerReport (void);
#endif

		BOOL	HasTarget (string_t targetname);

		int ObjectCaps (void) { return CBaseToggle::ObjectCaps () & ~FCAP_ACROSS_TRANSITION; }

		virtual int		Save (CSave &save);
		virtual int		Restore (CRestore &restore);

		static	TYPEDESCRIPTION m_SaveData[];

		int		m_cTargets;	// the total number of targets in this manager's fire list.
		int		m_index;	// Current target
		float	m_startTime;// Time we started firing
		int		m_iTargetName[MAX_MULTI_TARGETS];// list if indexes into global string array
		float	m_flTargetDelay[MAX_MULTI_TARGETS];// delay (in seconds) from time of manager fire to target fire
	private:
		inline BOOL IsClone (void) { return (pev->spawnflags & SF_MULTIMAN_CLONE) ? TRUE : FALSE; }
		inline BOOL ShouldClone (void)
			{
			if (IsClone ())
				return FALSE;

			return (pev->spawnflags & SF_MULTIMAN_THREAD) ? TRUE : FALSE;
			}

		CMultiManager *Clone (void);
	};

LINK_ENTITY_TO_CLASS (multi_manager, CMultiManager);

// Global Savedata for multi_manager
TYPEDESCRIPTION	CMultiManager::m_SaveData[] =
	{
		DEFINE_FIELD (CMultiManager, m_cTargets, FIELD_INTEGER),
		DEFINE_FIELD (CMultiManager, m_index, FIELD_INTEGER),
		DEFINE_FIELD (CMultiManager, m_startTime, FIELD_TIME),
		DEFINE_ARRAY (CMultiManager, m_iTargetName, FIELD_STRING, MAX_MULTI_TARGETS),
		DEFINE_ARRAY (CMultiManager, m_flTargetDelay, FIELD_FLOAT, MAX_MULTI_TARGETS),
	};

IMPLEMENT_SAVERESTORE (CMultiManager, CBaseToggle);

void CMultiManager::KeyValue (KeyValueData *pkvd)
	{
	if (FStrEq (pkvd->szKeyName, "wait"))
		{
		m_flWait = atof (pkvd->szValue);
		pkvd->fHandled = TRUE;
		}
	else // add this field to the target list
		{
		// this assumes that additional fields are targetnames and their values are delay values
		if (m_cTargets < MAX_MULTI_TARGETS)
			{
			char tmp[128];

			UTIL_StripToken (pkvd->szKeyName, tmp);
			m_iTargetName[m_cTargets] = ALLOC_STRING (tmp);
			m_flTargetDelay[m_cTargets] = atof (pkvd->szValue);
			m_cTargets++;
			pkvd->fHandled = TRUE;
			}
		}
	}

void CMultiManager::Spawn (void)
	{
	pev->solid = SOLID_NOT;
	SetUse (&CMultiManager::ManagerUse);
	SetThink (&CMultiManager::ManagerThink);

	// Sort targets
	// Quick and dirty bubble sort
	int swapped = 1;

	while (swapped)
		{
		swapped = 0;
		for (int i = 1; i < m_cTargets; i++)
			{
			if (m_flTargetDelay[i] < m_flTargetDelay[i - 1])
				{
				// Swap out of order elements
				int name = m_iTargetName[i];
				float delay = m_flTargetDelay[i];
				m_iTargetName[i] = m_iTargetName[i - 1];
				m_flTargetDelay[i] = m_flTargetDelay[i - 1];
				m_iTargetName[i - 1] = name;
				m_flTargetDelay[i - 1] = delay;
				swapped = 1;
				}
			}
		}
	}

BOOL CMultiManager::HasTarget (string_t targetname)
	{
	for (int i = 0; i < m_cTargets; i++)
		if (FStrEq (STRING (targetname), STRING (m_iTargetName[i])))
			return TRUE;

	return FALSE;
	}

// Designers were using this to fire targets that may or may not exist -- 
// so I changed it to use the standard target fire code, made it a little simpler.
void CMultiManager::ManagerThink (void)
	{
	float	time;

	time = gpGlobals->time - m_startTime;
	while (m_index < m_cTargets && m_flTargetDelay[m_index] <= time)
		{
		FireTargets (STRING (m_iTargetName[m_index]), m_hActivator, this, USE_TOGGLE, 0);
		m_index++;
		}

	if (m_index >= m_cTargets)// have we fired all targets?
		{
		SetThink (NULL);
		if (IsClone ())
			{
			UTIL_Remove (this);
			return;
			}
		SetUse (&CMultiManager::ManagerUse);// allow manager re-use 
		}
	else
		{
		pev->nextthink = m_startTime + m_flTargetDelay[m_index];
		}
	}

CMultiManager *CMultiManager::Clone (void)
	{
	CMultiManager *pMulti = GetClassPtr ((CMultiManager *)NULL);

	edict_t *pEdict = pMulti->pev->pContainingEntity;
	memcpy (pMulti->pev, pev, sizeof (*pev));
	pMulti->pev->pContainingEntity = pEdict;

	pMulti->pev->spawnflags |= SF_MULTIMAN_CLONE;
	pMulti->m_cTargets = m_cTargets;
	memcpy (pMulti->m_iTargetName, m_iTargetName, sizeof (m_iTargetName));
	memcpy (pMulti->m_flTargetDelay, m_flTargetDelay, sizeof (m_flTargetDelay));

	return pMulti;
	}


// The USE function builds the time table and starts the entity thinking.
void CMultiManager::ManagerUse (CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
	{
	// In multiplayer games, clone the MM and execute in the clone (like a thread)
	// to allow multiple players to trigger the same multimanager
	if (ShouldClone ())
		{
		CMultiManager *pClone = Clone ();
		pClone->ManagerUse (pActivator, pCaller, useType, value);
		return;
		}

	m_hActivator = pActivator;
	m_index = 0;
	m_startTime = gpGlobals->time;

	SetUse (NULL);// disable use until all targets have fired

	SetThink (&CMultiManager::ManagerThink);
	pev->nextthink = gpGlobals->time;
	}

#if _DEBUG
void CMultiManager::ManagerReport (void)
	{
	int	cIndex;

	for (cIndex = 0; cIndex < m_cTargets; cIndex++)
		{
		ALERT (at_console, "%s %f\n", STRING (m_iTargetName[cIndex]), m_flTargetDelay[cIndex]);
		}
	}
#endif

// ***********************************************************
// Render parameters trigger
// 
// This entity will copy its render parameters (renderfx, rendermode, rendercolor, renderamt)
// to its targets when triggered

// Flags to indicate masking off various render parameters that are normally copied to the targets
#define SF_RENDER_MASKFX	(1<<0)
#define SF_RENDER_MASKAMT	(1<<1)
#define SF_RENDER_MASKMODE	(1<<2)
#define SF_RENDER_MASKCOLOR	(1<<3)

class CRenderFxManager : public CBaseEntity
	{
	public:
		void Spawn (void);
		void Use (CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	};

LINK_ENTITY_TO_CLASS (env_render, CRenderFxManager);

void CRenderFxManager::Spawn (void)
	{
	pev->solid = SOLID_NOT;
	}

void CRenderFxManager::Use (CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
	{
	if (!FStringNull (pev->target))
		{
		edict_t *pentTarget = NULL;
		while (1)
			{
			pentTarget = FIND_ENTITY_BY_TARGETNAME (pentTarget, STRING (pev->target));
			if (FNullEnt (pentTarget))
				break;

			entvars_t *pevTarget = VARS (pentTarget);
			if (!FBitSet (pev->spawnflags, SF_RENDER_MASKFX))
				pevTarget->renderfx = pev->renderfx;
			if (!FBitSet (pev->spawnflags, SF_RENDER_MASKAMT))
				pevTarget->renderamt = pev->renderamt;
			if (!FBitSet (pev->spawnflags, SF_RENDER_MASKMODE))
				pevTarget->rendermode = pev->rendermode;
			if (!FBitSet (pev->spawnflags, SF_RENDER_MASKCOLOR))
				pevTarget->rendercolor = pev->rendercolor;
			}
		}
	}

class CBaseTrigger : public CBaseToggle
	{
	public:
		void HLEXPORT TeleportTouch (CBaseEntity *pOther);
		void KeyValue (KeyValueData *pkvd);
		void HLEXPORT MultiTouch (CBaseEntity *pOther);
		void HLEXPORT HurtTouch (CBaseEntity *pOther);
		void HLEXPORT CDAudioTouch (CBaseEntity *pOther);
		// ESHQ: ��������� �������������� ������������
		void ActivateMultiTrigger (CBaseEntity *pActivator, int RoomType);
		void HLEXPORT MultiWaitOver (void);
		void HLEXPORT CounterUse (CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
		void HLEXPORT ToggleUse (CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
		void InitTrigger (void);

		virtual int	ObjectCaps (void) { return CBaseToggle::ObjectCaps () & ~FCAP_ACROSS_TRANSITION; }
	};

LINK_ENTITY_TO_CLASS (trigger, CBaseTrigger);

/***
================
InitTrigger
================
***/
void CBaseTrigger::InitTrigger ()
	{
	// trigger angles are used for one-way touches.  An angle of 0 is assumed
	// to mean no restrictions, so use a yaw of 360 instead.
	if (pev->angles != g_vecZero)
		SetMovedir (pev);
	pev->solid = SOLID_TRIGGER;
	pev->movetype = MOVETYPE_NONE;
	SET_MODEL (ENT (pev), STRING (pev->model));    // set size and link into world
	if (CVAR_GET_FLOAT ("showtriggers") == 0)
		SetBits (pev->effects, EF_NODRAW);
	}

// Cache user-entity-field values until spawn is called
void CBaseTrigger::KeyValue (KeyValueData *pkvd)
	{
	if (FStrEq (pkvd->szKeyName, "damage"))
		{
		pev->dmg = atof (pkvd->szValue);
		pkvd->fHandled = TRUE;
		}
	else if (FStrEq (pkvd->szKeyName, "count"))
		{
		m_cTriggersLeft = (int)atof (pkvd->szValue);
		pkvd->fHandled = TRUE;
		}
	else if (FStrEq (pkvd->szKeyName, "damagetype"))
		{
		m_bitsDamageInflict = atoi (pkvd->szValue);
		pkvd->fHandled = TRUE;
		}
	else
		{
		CBaseToggle::KeyValue (pkvd);
		}
	}

class CTriggerHurt : public CBaseTrigger
	{
	public:
		void Spawn (void);
		void HLEXPORT RadiationThink (void);
	};

LINK_ENTITY_TO_CLASS (trigger_hurt, CTriggerHurt);

// trigger_monsterjump
class CTriggerMonsterJump : public CBaseTrigger
	{
	public:
		void Spawn (void);
		void Touch (CBaseEntity *pOther);
		void Think (void);
	};

LINK_ENTITY_TO_CLASS (trigger_monsterjump, CTriggerMonsterJump);

void CTriggerMonsterJump::Spawn (void)
	{
	SetMovedir (pev);

	InitTrigger ();

	pev->nextthink = 0;
	pev->speed = 200;
	m_flHeight = 150;

	if (!FStringNull (pev->targetname))
		{// if targetted, spawn turned off
		pev->solid = SOLID_NOT;
		UTIL_SetOrigin (pev, pev->origin); // Unlink from trigger list
		SetUse (&CBaseTrigger::ToggleUse);
		}
	}

void CTriggerMonsterJump::Think (void)
	{
	pev->solid = SOLID_NOT;// kill the trigger for now !!!UNDONE
	UTIL_SetOrigin (pev, pev->origin); // Unlink from trigger list
	SetThink (NULL);
	}

void CTriggerMonsterJump::Touch (CBaseEntity *pOther)
	{
	entvars_t *pevOther = pOther->pev;

	if (!FBitSet (pevOther->flags, FL_MONSTER))
		{// touched by a non-monster.
		return;
		}

	pevOther->origin.z += 1;

	if (FBitSet (pevOther->flags, FL_ONGROUND))
		{// clear the onground so physics don't bitch
		pevOther->flags &= ~FL_ONGROUND;
		}

	// toss the monster!
	pevOther->velocity = pev->movedir * pev->speed;
	pevOther->velocity.z += m_flHeight;
	pev->nextthink = gpGlobals->time;
	}


// =====================================
// trigger_cdaudio - starts/stops cd audio tracks
class CTriggerCDAudio : public CBaseTrigger
	{
	public:
		void Spawn (void);

		virtual void Use (CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
		void PlayTrack (void);
		void Touch (CBaseEntity *pOther);
	};

LINK_ENTITY_TO_CLASS (trigger_cdaudio, CTriggerCDAudio);

// Changes tracks or stops CD when player touches
// !!!HACK - overloaded HEALTH to avoid adding new field
void CTriggerCDAudio::Touch (CBaseEntity *pOther)
	{
	// only clients may trigger these events
	if (!pOther->IsPlayer ())
		return;

	PlayTrack ();
	}

void CTriggerCDAudio::Spawn (void)
	{
	InitTrigger ();
	}

void CTriggerCDAudio::Use (CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
	{
	PlayTrack ();
	}

void PlayCDTrack (int iTrack)
	{
	edict_t *pClient;

	// manually find the single player
	pClient = g_engfuncs.pfnPEntityOfEntIndex (1);

	// Can't play if the client is not connected!
	if (!pClient)
		return;

	if ((iTrack < -1) || (iTrack > 30))
		{
		ALERT (at_console, "TriggerCDAudio - Track %d out of range\n");
		return;
		}

	if (iTrack == -1)
		{
		CLIENT_COMMAND (pClient, "cd pause\n");
		}
	else
		{
		char string[64];

		sprintf (string, "cd play %3d\n", iTrack);
		CLIENT_COMMAND (pClient, string);
		}
	}

// only plays for ONE client, so only use in single play!
void CTriggerCDAudio::PlayTrack (void)
	{
	PlayCDTrack ((int)pev->health);

	SetTouch (NULL);
	UTIL_Remove (this);
	}

// This plays a CD track when fired or when the player enters it's radius
class CTargetCDAudio : public CPointEntity
	{
	public:
		void			Spawn (void);
		void			KeyValue (KeyValueData *pkvd);

		virtual void	Use (CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
		void			Think (void);
		void			Play (void);
	};

LINK_ENTITY_TO_CLASS (target_cdaudio, CTargetCDAudio);

void CTargetCDAudio::KeyValue (KeyValueData *pkvd)
	{
	if (FStrEq (pkvd->szKeyName, "radius"))
		{
		pev->scale = atof (pkvd->szValue);
		pkvd->fHandled = TRUE;
		}
	else
		{
		CPointEntity::KeyValue (pkvd);
		}
	}

void CTargetCDAudio::Spawn (void)
	{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;

	if (pev->scale > 0)
		pev->nextthink = gpGlobals->time + 1.0;
	}

void CTargetCDAudio::Use (CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
	{
	Play ();
	}

// only plays for ONE client, so only use in single play!
void CTargetCDAudio::Think (void)
	{
	edict_t *pClient;

	// manually find the single player. 
	pClient = g_engfuncs.pfnPEntityOfEntIndex (1);

	// Can't play if the client is not connected!
	if (!pClient)
		return;

	pev->nextthink = gpGlobals->time + 0.5;

	if ((pClient->v.origin - pev->origin).Length () <= pev->scale)
		Play ();

	}

void CTargetCDAudio::Play (void)
	{
	PlayCDTrack ((int)pev->health);
	UTIL_Remove (this);
	}

// =====================================

// trigger_hurt - hurts anything that touches it. if the trigger has a targetname, firing it will toggle state
void CTriggerHurt::Spawn (void)
	{
	InitTrigger ();
	SetTouch (&CBaseTrigger::HurtTouch);

	if (!FStringNull (pev->targetname))
		SetUse (&CBaseTrigger::ToggleUse);
	else
		SetUse (NULL);

	if (m_bitsDamageInflict & DMG_RADIATION)
		{
		SetThink (&CTriggerHurt::RadiationThink);
		pev->nextthink = gpGlobals->time + RANDOM_FLOAT (0.0, 0.5);
		}

	if (FBitSet (pev->spawnflags, SF_TRIGGER_HURT_START_OFF))	// if flagged to Start Turned Off, make trigger nonsolid
		pev->solid = SOLID_NOT;

	UTIL_SetOrigin (pev, pev->origin);		// Link into the list
	}

// trigger hurt that causes radiation will do a radius
// check and set the player's geiger counter level
// according to distance from center of trigger
void CTriggerHurt::RadiationThink (void)
	{
	edict_t *pentPlayer;
	CBasePlayer *pPlayer = NULL;
	float flRange;
	entvars_t *pevTarget;
	Vector vecSpot1;
	Vector vecSpot2;
	Vector vecRange;
	Vector origin;
	Vector view_ofs;

	// check to see if a player is in pvs
	// if not, continue	

	// set origin to center of trigger so that this check works
	origin = pev->origin;
	view_ofs = pev->view_ofs;

	pev->origin = (pev->absmin + pev->absmax) * 0.5;
	pev->view_ofs = pev->view_ofs * 0.0;

	pentPlayer = FIND_CLIENT_IN_PVS (edict ());

	pev->origin = origin;
	pev->view_ofs = view_ofs;

	// reset origin
	if (!FNullEnt (pentPlayer))
		{
		pPlayer = GetClassPtr ((CBasePlayer *)VARS (pentPlayer));

		pevTarget = VARS (pentPlayer);

		// get range to player
		vecSpot1 = (pev->absmin + pev->absmax) * 0.5;
		vecSpot2 = (pevTarget->absmin + pevTarget->absmax) * 0.5;

		vecRange = vecSpot1 - vecSpot2;
		flRange = vecRange.Length ();

		// if player's current geiger counter range is larger
		// than range to this trigger hurt, reset player's
		// geiger counter range 
		if (pPlayer->m_flgeigerRange >= flRange)
			pPlayer->m_flgeigerRange = flRange;
		}

	pev->nextthink = gpGlobals->time + 0.25;
	}

// ToggleUse - If this is the USE function for a trigger, its state will toggle every time it's fired
void CBaseTrigger::ToggleUse (CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
	{
	// if the trigger is off, turn it on
	if (pev->solid == SOLID_NOT)
		{
		pev->solid = SOLID_TRIGGER;

		// Force retouch
		gpGlobals->force_retouch++;
		}
	else
		{// turn the trigger off
		pev->solid = SOLID_NOT;
		}

	UTIL_SetOrigin (pev, pev->origin);
	}

// When touched, a hurt trigger does DMG points of damage each half-second
void CBaseTrigger::HurtTouch (CBaseEntity *pOther)
	{
	float fldmg;

	if (!pOther->pev->takedamage)
		return;

	if ((pev->spawnflags & SF_TRIGGER_HURT_CLIENTONLYTOUCH) && !pOther->IsPlayer ())
		{
		// this trigger is only allowed to touch clients, and this ain't a client
		return;
		}

	if ((pev->spawnflags & SF_TRIGGER_HURT_NO_CLIENTS) && pOther->IsPlayer ())
		return;

	// HACKHACK -- In multiplayer, players touch this based on packet receipt.
	// So the players who send packets later aren't always hurt.  Keep track of
	// how much time has passed and whether or not you've touched that player
	if (g_pGameRules->IsMultiplayer ())
		{
		if (pev->dmgtime > gpGlobals->time)
			{
			if (gpGlobals->time != pev->pain_finished)
				{// too early to hurt again, and not same frame with a different entity
				if (pOther->IsPlayer ())
					{
					int playerMask = 1 << (pOther->entindex () - 1);

					// If I've already touched this player (this time), then bail out
					if (pev->impulse & playerMask)
						return;

					// Mark this player as touched
					// BUGBUG - There can be only 32 players!
					pev->impulse |= playerMask;
					}
				else
					{
					return;
					}
				}
			}
		else
			{
			// New clock, "un-touch" all players
			pev->impulse = 0;
			if (pOther->IsPlayer ())
				{
				int playerMask = 1 << (pOther->entindex () - 1);

				// Mark this player as touched
				// BUGBUG - There can be only 32 players!
				pev->impulse |= playerMask;
				}
			}
		}
	else	// Original code -- single player
		{
		if ((pev->dmgtime > gpGlobals->time) && (gpGlobals->time != pev->pain_finished))
			// too early to hurt again, and not same frame with a different entity
			return;
		}

	// If this is time_based damage (poison, radiation), override the pev->dmg with a 
	// default for the given damage type. Monsters only take time-based damage
	// while touching the trigger. Player continues taking damage for a while after
	// leaving the trigger
	fldmg = pev->dmg * 0.5;	// 0.5 seconds worth of damage, pev->dmg is damage/second

	if (fldmg < 0)
		pOther->TakeHealth (-fldmg, m_bitsDamageInflict);
	else
		pOther->TakeDamage (pev, pev, fldmg, m_bitsDamageInflict);

	// Store pain time so we can get all of the other entities on this frame
	pev->pain_finished = gpGlobals->time;

	// Apply damage every half second
	pev->dmgtime = gpGlobals->time + 0.5;	// half second delay until this trigger can hurt toucher again

	if (pev->target)
		{
		// trigger has a target it wants to fire 
		if (pev->spawnflags & SF_TRIGGER_HURT_CLIENTONLYFIRE)
			{
			// if the toucher isn't a client, don't fire the target!
			if (!pOther->IsPlayer ())
				return;
			}

		SUB_UseTargets (pOther, USE_TOGGLE, 0);
		if (pev->spawnflags & SF_TRIGGER_HURT_TARGETONCE)
			pev->target = 0;
		}
	}

/***
QUAKED trigger_multiple (.5 .5 .5) ? notouch
Variable sized repeatable trigger.  Must be targeted at one or more entities.
If "health" is set, the trigger must be killed to activate each time.
If "delay" is set, the trigger waits some time after activating before firing.
"wait" : Seconds between triggerings. (.2 default)
If notouch is set, the trigger is only fired by other entities, not by touching.
NOTOUCH has been obsoleted by trigger_relay!
sounds
1)      secret
2)      beep beep
3)      large switch
4)
NEW
if a trigger has a NETNAME, that NETNAME will become the TARGET of the triggered object.
***/
class CTriggerMultiple : public CBaseTrigger
	{
	public:
		void Spawn (void);
	};

LINK_ENTITY_TO_CLASS (trigger_multiple, CTriggerMultiple);

void CTriggerMultiple::Spawn (void)
	{
	if (m_flWait == 0)
		m_flWait = 0.2;

	InitTrigger ();

	ASSERTSZ (pev->health == 0, "trigger_multiple with health");
	SetTouch (&CBaseTrigger::MultiTouch);
	}

// ESHQ: ����������� brush entity ��� ��������� ����������� env_sound ��� ����� �������������� ���������
class CTriggerSound : public CBaseTrigger
	{
	public:
		void Spawn (void);
		void KeyValue (KeyValueData *pkvd);

		// trigger_sound
		void HLEXPORT MultiTouch_Sound (CBaseEntity *pOther);

		virtual int		Save (CSave &save);
		virtual int		Restore (CRestore &restore);

		// trigger_sound
		static	TYPEDESCRIPTION m_SaveData[];
		float m_flRoomtype;
		float m_flRoomtype2;
		unsigned char m_direction;
	};

LINK_ENTITY_TO_CLASS (trigger_sound, CTriggerSound);
TYPEDESCRIPTION	CTriggerSound::m_SaveData[] =
	{
		DEFINE_FIELD (CTriggerSound, m_flRoomtype, FIELD_FLOAT),
		DEFINE_FIELD (CTriggerSound, m_flRoomtype2, FIELD_FLOAT),
		DEFINE_FIELD (CTriggerSound, m_direction, FIELD_CHARACTER),
	};
IMPLEMENT_SAVERESTORE (CTriggerSound, CBaseTrigger);

void CTriggerSound::Spawn (void)
	{
	// ����� �������������
	m_flWait = 1.0;
	InitTrigger ();

	m_direction = 0x00;
	if (pev->spawnflags & SF_BIDIRECTIONAL_SOUND_T)
		{
		if ((pev->size.x <= pev->size.y) && (pev->size.x <= pev->size.z))
			m_direction = 0x01;
		else if ((pev->size.y <= pev->size.x) && (pev->size.y <= pev->size.z))
			m_direction = 0x02;
		else // if ((y <= x) && (y <= z))
			m_direction = 0x04;
		}

	SetTouch (&CTriggerSound::MultiTouch_Sound);
	}

void CTriggerSound::KeyValue (KeyValueData *pkvd)
	{
	if (FStrEq (pkvd->szKeyName, "roomtype"))
		{
		m_flRoomtype = atoi (pkvd->szValue);
		pkvd->fHandled = TRUE;
		}
	else if (FStrEq (pkvd->szKeyName, "roomtype2"))
		{
		m_flRoomtype2 = atoi (pkvd->szValue);
		pkvd->fHandled = TRUE;
		}
	else
		{
		CBaseTrigger::KeyValue (pkvd);
		}
	}

// ESHQ: brush entity ��� ������� ������
class CTriggerFog : public CBaseTrigger
	{
	public:
		void Spawn (void);
		void KeyValue (KeyValueData *pkvd);

		// trigger_sound
		void HLEXPORT MultiTouch_Fog (CBaseEntity *pOther);

		virtual int	Save (CSave &save);
		virtual int	Restore (CRestore &restore);

		// trigger_sound
		static	TYPEDESCRIPTION m_SaveData[];
		unsigned char m_enablingMove;
		unsigned char m_currentEnablingGrade;
		float m_multiplier;
	};

LINK_ENTITY_TO_CLASS (trigger_fog, CTriggerFog);
TYPEDESCRIPTION	CTriggerFog::m_SaveData[] =
	{
	DEFINE_FIELD (CTriggerFog, m_enablingMove, FIELD_CHARACTER),
	};
IMPLEMENT_SAVERESTORE (CTriggerFog, CBaseTrigger);

void CTriggerFog::Spawn (void)
	{
	// ����� �������������
	m_flWait = 1.0;
	InitTrigger ();

	SetTouch (&CTriggerFog::MultiTouch_Fog);
	}

void CTriggerFog::KeyValue (KeyValueData *pkvd)
	{
	if (FStrEq (pkvd->szKeyName, "enablingMove"))
		{
		m_enablingMove = atoi (pkvd->szValue);
		if (m_enablingMove > 7)
			m_enablingMove = 7;	// ����������

		pkvd->fHandled = TRUE;
		}
	else
		{
		CBaseTrigger::KeyValue (pkvd);
		}
	}

void CTriggerFog::MultiTouch_Fog (CBaseEntity *pOther)
	{
	entvars_t	*pevToucher;
	float		enablingGrade = 0.0f;	// �� 0 �� 1 ������������
	unsigned int	packed = 0;
	float		multiplier = 0.0f;
	float		size = 0.0f;

	// Only touch clients
	pevToucher = pOther->pev;
	if (!(pevToucher->flags & FL_CLIENT))
		return;

	// ����������� �����������
	if ((m_enablingMove != 0) && (m_enablingMove != 7))
		{
		switch (m_enablingMove)
			{
			case 1:
			case 2:
				size = pev->absmax.x - pev->absmin.x;
				enablingGrade = (pevToucher->origin.x - pev->absmin.x) / size;
				break;

			case 3:
			case 4:
				size = pev->absmax.y - pev->absmin.y;
				enablingGrade = (pevToucher->origin.y - pev->absmin.y) / size;
				break;

			case 5:
			case 6:
				size = pev->absmax.z - pev->absmin.z;
				enablingGrade = (pevToucher->origin.z - pev->absmin.z) / size;
				break;
			}

		m_flWait = 0.01;	// ������������� ����� ����� �� ������� ��� ����������� ������������
		}
	else
		{
		m_flWait = 0.1;		// ����������� �����
		}

	// ������� ������ ������������� ���������, �� ������������ � �������
	if (m_enablingMove == 0)
		{
		packed = (0xFF & (int)pev->rendercolor.x) << 24;
		packed |= (0xFF & (int)pev->rendercolor.y) << 16;
		packed |= (0xFF & (int)pev->rendercolor.z) << 8;
		packed |= (0xFF & (int)(pev->renderamt));
		SETUP_FOG (packed);

		return;
		}

	// ��������� ������� ������ �� ������� �������� � ��������� ��������
	if (enablingGrade < 0.0f)
		enablingGrade = 0.0f;
	if (enablingGrade > 1.0f)
		enablingGrade = 1.0f;
	if (m_enablingMove % 2)
		enablingGrade = 1.0f - enablingGrade;

	// ������� ������� ����������
	if ((m_enablingMove == 7) || (enablingGrade < 0.05f))
		{
		SETUP_FOG (0x00000001);
		return;
		}

	// ������ ���������-��������������
	if (!m_multiplier)
		{
		float multiplier1 = 0.0f, multiplier2 = 0.0f;

		multiplier1 = pev->renderamt / 4.0f;
		if (multiplier1 < 10.0f)
			multiplier1 = 10.0f;
		if (multiplier1 > 64.0f)
			multiplier1 = 64.0f;	// 10 <= m <= 64

		multiplier2 = size / 8.0f;
		if (multiplier2 < 10.0f)
			multiplier2 = 10.0f;
		if (multiplier2 > 100.0f)
			multiplier2 = 100.0f;	// 10 <= m <= 100

		m_multiplier = multiplier2 > multiplier1 ? multiplier2 : multiplier1;
		}
	multiplier = enablingGrade * m_multiplier + 0.5f;

	// ������� �������� ������� � ���������� (�� runtime) ������
	if ((int)multiplier != m_currentEnablingGrade)
		{
		m_currentEnablingGrade = (int)multiplier;

		packed = (0xFF & (int)pev->rendercolor.x) << 24;
		packed |= (0xFF & (int)pev->rendercolor.y) << 16;
		packed |= (0xFF & (int)pev->rendercolor.z) << 8;
		packed |= (0xFF & (int)(pev->renderamt * enablingGrade));
		SETUP_FOG (packed);
		}
	}


/***
QUAKED trigger_once (.5 .5 .5) ? notouch
Variable sized trigger. Triggers once, then removes itself.  You must set the key "target"
to the name of another object in the level that has a matching
"targetname".  If "health" is set, the trigger must be killed to activate.
If notouch is set, the trigger is only fired by other entities, not by touching.
if "killtarget" is set, any objects that have a matching "target" will be removed when the trigger is fired.
if "angle" is set, the trigger will only fire when someone is facing the direction of the angle.
Use "360" for an angle of 0.
sounds
1)      secret
2)      beep beep
3)      large switch
4)
***/
class CTriggerOnce : public CTriggerMultiple
	{
	public:
		void Spawn (void);
	};

LINK_ENTITY_TO_CLASS (trigger_once, CTriggerOnce);
void CTriggerOnce::Spawn (void)
	{
	m_flWait = -1;

	CTriggerMultiple::Spawn ();
	}

void CBaseTrigger::MultiTouch (CBaseEntity *pOther)
	{
	entvars_t *pevToucher;
	pevToucher = pOther->pev;

	// Only touch clients, monsters, or pushables (depending on flags)
	if (((pevToucher->flags & FL_CLIENT) && !(pev->spawnflags & SF_TRIGGER_NOCLIENTS)) ||
		((pevToucher->flags & FL_MONSTER) && (pev->spawnflags & SF_TRIGGER_ALLOWMONSTERS)) ||
		(pev->spawnflags & SF_TRIGGER_PUSHABLES) && FClassnameIs (pevToucher, "func_pushable"))
		{
		ActivateMultiTrigger (pOther, -1);
		}
	}

// ESHQ: ��������� brush entity ��� ��������� ����������� env_sound
void CTriggerSound::MultiTouch_Sound (CBaseEntity *pOther)
	{
	entvars_t	*pevToucher;
	float		rt = m_flRoomtype;

	// Only touch clients
	pevToucher = pOther->pev;
	if (!(pevToucher->flags & FL_CLIENT))
		return;

	// ����������� �����������
	if (m_direction)
		{
		switch (m_direction)
			{
			case 0x01:
				if (pevToucher->origin.x > (pev->absmin.x + pev->absmax.x) / 2.0f)
					rt = m_flRoomtype2;
				break;

			case 0x02:
				if (pevToucher->origin.y > (pev->absmin.y + pev->absmax.y) / 2.0f)
					rt = m_flRoomtype2;
				break;

			case 0x04:
				if (pevToucher->origin.z > (pev->absmin.z + pev->absmax.z) / 2.0f)
					rt = m_flRoomtype2;
				break;
			}

		m_flWait = 0.01;	// ������������� ����� ����� �� ������� ��� ����������� ������������
		}
	else
		{
		m_flWait = 0.1;		// ����������� �����
		}

	ActivateMultiTrigger (pOther, rt);
	WRITE_ACHIEVEMENTS_SCRIPT (2, -1);	// ESHQ: ������ �������� � ������������ ����
	}

// the trigger was just touched/killed/used
// self.enemy should be set to the activator so it can be held through a delay
// so wait for the delay time before firing

// ESHQ: RoomType ������������ ��� ��������� ��������� ������� ������ ������ ���� (-1 - ������������)
void CBaseTrigger::ActivateMultiTrigger (CBaseEntity *pActivator, int RoomType)
	{
	if (pev->nextthink > gpGlobals->time)
		return;         // still waiting for reset time

	if (!UTIL_IsMasterTriggered (m_sMaster, pActivator))
		return;

	if (FClassnameIs (pev, "trigger_secret"))
		{
		if ((pev->enemy == NULL) || !FClassnameIs (pev->enemy, "player"))
			return;
		gpGlobals->found_secrets++;
		}

	if (!FStringNull (pev->noise))
		EMIT_SOUND (ENT (pev), CHAN_VOICE, (char *)STRING (pev->noise), 1, ATTN_MEDIUM);

	m_hActivator = pActivator;

	// ESHQ: ������ �����, ���� �� ������
	if (RoomType >= 0)
		{
		edict_t *pentPlayer = ENT (pActivator->pev);
		CBasePlayer *pPlayer = NULL;

		if (!FNullEnt (pentPlayer))
			{
			pPlayer = GetClassPtr ((CBasePlayer *)VARS (pentPlayer));
			pPlayer->m_flSndRoomtype = RoomType;

			MESSAGE_BEGIN (MSG_ONE, SVC_ROOMTYPE, NULL, pentPlayer);
			WRITE_SHORT ((short)RoomType);
			MESSAGE_END ();
			}
		}
	else
		{
		SUB_UseTargets (m_hActivator, USE_TOGGLE, 0);
		}

	if (pev->message && pActivator->IsPlayer ())
		{
		UTIL_ShowMessage (STRING (pev->message), pActivator);
		}

	if (m_flWait > 0)
		{
		SetThink (&CBaseTrigger::MultiWaitOver);
		pev->nextthink = gpGlobals->time + m_flWait;
		}
	else
		{
		// we can't just remove (self) here, because this is a touch function
		// called while C code is looping through area links...
		SetTouch (NULL);
		pev->nextthink = gpGlobals->time + 0.1;
		SetThink (&CBaseEntity::SUB_Remove);
		}
	}

// the wait time has passed, so set back up for another activation
void CBaseTrigger::MultiWaitOver (void)
	{
	SetThink (NULL);
	}

// ========================= COUNTING TRIGGER =====================================
// GLOBALS ASSUMED SET: g_eoActivator
void CBaseTrigger::CounterUse (CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
	{
	m_cTriggersLeft--;
	m_hActivator = pActivator;

	if (m_cTriggersLeft < 0)
		return;

	/*BOOL fTellActivator = (m_hActivator != 0) &&
		FClassnameIs (m_hActivator->pev, "player") &&
		!FBitSet (pev->spawnflags, SPAWNFLAG_NOMESSAGE);
	if (m_cTriggersLeft != 0)
		{
		if (fTellActivator)
			{
			// UNDONE: I don't think we want these Quakesque messages
			switch (m_cTriggersLeft)
				{
				case 1:		ALERT (at_console, "Only 1 more to go...");		break;
				case 2:		ALERT (at_console, "Only 2 more to go...");		break;
				case 3:		ALERT (at_console, "Only 3 more to go...");		break;
				default:	ALERT (at_console, "There are more to go...");	break;
				}
			}
		return;
		}

	// !!!UNDONE: I don't think we want these Quakesque messages
	if (fTellActivator)
		ALERT (at_console, "Sequence completed!");*/

	ActivateMultiTrigger (m_hActivator, -1);
	}

/***
QUAKED trigger_counter (.5 .5 .5) ? nomessage
Acts as an intermediary for an action that takes multiple inputs.
If nomessage is not set, it will print "1 more.. " etc when triggered and
"sequence complete" when finished.  After the counter has been triggered "cTriggersLeft"
times (default 2), it will fire all of it's targets and remove itself.
***/
class CTriggerCounter : public CBaseTrigger
	{
	public:
		void Spawn (void);
	};
LINK_ENTITY_TO_CLASS (trigger_counter, CTriggerCounter);

void CTriggerCounter::Spawn (void)
	{
	// By making the flWait be -1, this counter-trigger will disappear after it's activated
	// (but of course it needs cTriggersLeft "uses" before that happens).
	m_flWait = -1;

	if (m_cTriggersLeft == 0)
		m_cTriggersLeft = 2;
	SetUse (&CBaseTrigger::CounterUse);
	}

// ====================== TRIGGER_CHANGELEVEL ================================
class CTriggerVolume : public CPointEntity	// Derive from point entity so this doesn't move across levels
	{
	public:
		void	Spawn (void);
	};

LINK_ENTITY_TO_CLASS (trigger_transition, CTriggerVolume);

// Define space that travels across a level transition
void CTriggerVolume::Spawn (void)
	{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	SET_MODEL (ENT (pev), STRING (pev->model));    // set size and link into world
	pev->model = NULL;
	pev->modelindex = 0;
	}

// Fires a target after level transition and then dies
class CFireAndDie : public CBaseDelay
	{
	public:
		void Spawn (void);
		void Precache (void);
		void Think (void);
		int ObjectCaps (void) { return CBaseDelay::ObjectCaps () | FCAP_FORCE_TRANSITION; }
		// Always go across transitions
	};
LINK_ENTITY_TO_CLASS (fireanddie, CFireAndDie);

void CFireAndDie::Spawn (void)
	{
	pev->classname = MAKE_STRING ("fireanddie");
	// Don't call Precache() - it should be called on restore
	}


void CFireAndDie::Precache (void)
	{
	// This gets called on restore
	pev->nextthink = gpGlobals->time + m_flDelay;
	}


void CFireAndDie::Think (void)
	{
	SUB_UseTargets (this, USE_TOGGLE, 0);
	UTIL_Remove (this);
	}


#define SF_CHANGELEVEL_USEONLY		0x0002
class CChangeLevel : public CBaseTrigger
	{
	public:
		void Spawn (void);
		void KeyValue (KeyValueData *pkvd);
		void HLEXPORT UseChangeLevel (CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
		void HLEXPORT TriggerChangeLevel (void);
		void HLEXPORT ExecuteChangeLevel (void);
		void HLEXPORT TouchChangeLevel (CBaseEntity *pOther);
		void ChangeLevelNow (CBaseEntity *pActivator);

		static edict_t *FindLandmark (const char *pLandmarkName);
		static int ChangeList (LEVELLIST *pLevelList, int maxList);
		static int AddTransitionToList (LEVELLIST *pLevelList, int listCount, const char *pMapName, const char *pLandmarkName, edict_t *pentLandmark);
		static int InTransitionVolume (CBaseEntity *pEntity, char *pVolumeName);

		virtual int		Save (CSave &save);
		virtual int		Restore (CRestore &restore);

		static	TYPEDESCRIPTION m_SaveData[];

		char m_szMapName[cchMapNameMost];		// trigger_changelevel only:  next map
		char m_szLandmarkName[cchMapNameMost];		// trigger_changelevel only:  landmark on next map
		int		m_changeTarget;
		float	m_changeTargetDelay;
	};

LINK_ENTITY_TO_CLASS (trigger_changelevel, CChangeLevel);

// Global Savedata for changelevel trigger
TYPEDESCRIPTION	CChangeLevel::m_SaveData[] =
	{
		DEFINE_ARRAY (CChangeLevel, m_szMapName, FIELD_CHARACTER, cchMapNameMost),
		DEFINE_ARRAY (CChangeLevel, m_szLandmarkName, FIELD_CHARACTER, cchMapNameMost),
		DEFINE_FIELD (CChangeLevel, m_changeTarget, FIELD_STRING),
		DEFINE_FIELD (CChangeLevel, m_changeTargetDelay, FIELD_FLOAT),
	};

IMPLEMENT_SAVERESTORE (CChangeLevel, CBaseTrigger);

// Cache user-entity-field values until spawn is called
void CChangeLevel::KeyValue (KeyValueData *pkvd)
	{
	if (FStrEq (pkvd->szKeyName, "map"))
		{
		if (strlen (pkvd->szValue) >= cchMapNameMost)
			ALERT (at_error, "Map name '%s' too long (32 chars)\n", pkvd->szValue);
		strcpy (m_szMapName, pkvd->szValue);
		pkvd->fHandled = TRUE;
		}
	else if (FStrEq (pkvd->szKeyName, "landmark"))
		{
		if (strlen (pkvd->szValue) >= cchMapNameMost)
			ALERT (at_error, "Landmark name '%s' too long (32 chars)\n", pkvd->szValue);
		strcpy (m_szLandmarkName, pkvd->szValue);
		pkvd->fHandled = TRUE;
		}
	else if (FStrEq (pkvd->szKeyName, "changetarget"))
		{
		m_changeTarget = ALLOC_STRING (pkvd->szValue);
		pkvd->fHandled = TRUE;
		}
	else if (FStrEq (pkvd->szKeyName, "changedelay"))
		{
		m_changeTargetDelay = atof (pkvd->szValue);
		pkvd->fHandled = TRUE;
		}
	else
		{
		CBaseTrigger::KeyValue (pkvd);
		}
	}

/***
QUAKED trigger_changelevel (0.5 0.5 0.5) ? NO_INTERMISSION
When the player touches this, he gets sent to the map listed in the "map" variable. Unless the NO_INTERMISSION flag
is set, the view will go to the info_intermission spot and display stats.
***/
void CChangeLevel::Spawn (void)
	{
	if (FStrEq (m_szMapName, ""))
		ALERT (at_console, "a trigger_changelevel doesn't have a map");

	if (FStrEq (m_szLandmarkName, ""))
		ALERT (at_console, "trigger_changelevel to %s doesn't have a landmark\n", m_szMapName);

	if (!FStringNull (pev->targetname))
		SetUse (&CChangeLevel::UseChangeLevel);

	InitTrigger ();
	if (!(pev->spawnflags & SF_CHANGELEVEL_USEONLY))
		SetTouch (&CChangeLevel::TouchChangeLevel);
	}

void CChangeLevel::ExecuteChangeLevel (void)
	{
	MESSAGE_BEGIN (MSG_ALL, SVC_CDTRACK);
	WRITE_BYTE (3);
	WRITE_BYTE (3);
	MESSAGE_END ();

	MESSAGE_BEGIN (MSG_ALL, SVC_INTERMISSION);
	MESSAGE_END ();
	}

FILE_GLOBAL char st_szNextMap[cchMapNameMost];
FILE_GLOBAL char st_szNextSpot[cchMapNameMost];

edict_t *CChangeLevel::FindLandmark (const char *pLandmarkName)
	{
	edict_t *pentLandmark;

	pentLandmark = FIND_ENTITY_BY_STRING (NULL, "targetname", pLandmarkName);
	while (!FNullEnt (pentLandmark))
		{
		// Found the landmark
		if (FClassnameIs (pentLandmark, "info_landmark"))
			return pentLandmark;
		else
			pentLandmark = FIND_ENTITY_BY_STRING (pentLandmark, "targetname", pLandmarkName);
		}

	ALERT (at_error, "Can't find landmark %s\n", pLandmarkName);
	return NULL;
	}

// =========================================================
// CChangeLevel :: Use - allows level transitions to be 
// triggered by buttons, etc.
// =========================================================
void CChangeLevel::UseChangeLevel (CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
	{
	ChangeLevelNow (pActivator);
	}

void CChangeLevel::ChangeLevelNow (CBaseEntity *pActivator)
	{
	edict_t *pentLandmark;
	LEVELLIST	levels[16];

	ASSERT (!FStrEq (m_szMapName, ""));

	// Don't work in deathmatch
	if (g_pGameRules->IsDeathmatch ())
		return;

	// Some people are firing these multiple times in a frame, disable
	if (gpGlobals->time == pev->dmgtime)
		return;

	pev->dmgtime = gpGlobals->time;

	CBaseEntity *pPlayer = CBaseEntity::Instance (g_engfuncs.pfnPEntityOfEntIndex (1));
	if (!InTransitionVolume (pPlayer, m_szLandmarkName))
		{
		ALERT (at_aiconsole, "Player isn't in the transition volume %s, aborting\n", m_szLandmarkName);
		return;
		}

	// Create an entity to fire the changetarget
	if (m_changeTarget)
		{
		CFireAndDie *pFireAndDie = GetClassPtr ((CFireAndDie *)NULL);
		if (pFireAndDie)
			{
			// Set target and delay
			pFireAndDie->pev->target = m_changeTarget;
			pFireAndDie->m_flDelay = m_changeTargetDelay;
			pFireAndDie->pev->origin = pPlayer->pev->origin;
			// Call spawn
			DispatchSpawn (pFireAndDie->edict ());
			}
		}

	// This object will get removed in the call to CHANGE_LEVEL, copy the params into "safe" memory
	strcpy (st_szNextMap, m_szMapName);

	m_hActivator = pActivator;
	SUB_UseTargets (pActivator, USE_TOGGLE, 0);
	st_szNextSpot[0] = 0;	// Init landmark to NULL

	// look for a landmark entity		
	pentLandmark = FindLandmark (m_szLandmarkName);
	if (!FNullEnt (pentLandmark))
		{
		strcpy (st_szNextSpot, m_szLandmarkName);
		gpGlobals->vecLandmarkOffset = VARS (pentLandmark)->origin;
		}

	ALERT (at_console, "CHANGE LEVEL: %s %s\n", st_szNextMap, st_szNextSpot);
	CHANGE_LEVEL (st_szNextMap, st_szNextSpot);
	}

// GLOBALS ASSUMED SET:  st_szNextMap
void CChangeLevel::TouchChangeLevel (CBaseEntity *pOther)
	{
	if (!FClassnameIs (pOther->pev, "player"))
		return;

	ChangeLevelNow (pOther);
	}

// Add a transition to the list, but ignore duplicates 
// (a designer may have placed multiple trigger_changelevels with the same landmark)
int CChangeLevel::AddTransitionToList (LEVELLIST *pLevelList, int listCount, const char *pMapName,
	const char *pLandmarkName, edict_t *pentLandmark)
	{
	int i;

	if (!pLevelList || !pMapName || !pLandmarkName || !pentLandmark)
		return 0;

	for (i = 0; i < listCount; i++)
		{
		if (pLevelList[i].pentLandmark == pentLandmark && strcmp (pLevelList[i].mapName, pMapName) == 0)
			return 0;
		}

	strcpy (pLevelList[listCount].mapName, pMapName);
	strcpy (pLevelList[listCount].landmarkName, pLandmarkName);
	pLevelList[listCount].pentLandmark = pentLandmark;
	pLevelList[listCount].vecLandmarkOrigin = VARS (pentLandmark)->origin;

	return 1;
	}

int BuildChangeList (LEVELLIST *pLevelList, int maxList)
	{
	return CChangeLevel::ChangeList (pLevelList, maxList);
	}

int CChangeLevel::InTransitionVolume (CBaseEntity *pEntity, char *pVolumeName)
	{
	edict_t *pentVolume;

	if (pEntity->ObjectCaps () & FCAP_FORCE_TRANSITION)
		return 1;

	// If you're following another entity, follow it through the transition (weapons follow the player)
	if (pEntity->pev->movetype == MOVETYPE_FOLLOW)
		{
		if (pEntity->pev->aiment != NULL)
			pEntity = CBaseEntity::Instance (pEntity->pev->aiment);
		}

	int inVolume = 1;	// Unless we find a trigger_transition, everything is in the volume

	pentVolume = FIND_ENTITY_BY_TARGETNAME (NULL, pVolumeName);
	while (!FNullEnt (pentVolume))
		{
		CBaseEntity *pVolume = CBaseEntity::Instance (pentVolume);

		if (pVolume && FClassnameIs (pVolume->pev, "trigger_transition"))
			{
			if (pVolume->Intersects (pEntity))	// It touches one, it's in the volume
				return 1;
			else
				inVolume = 0;	// Found a trigger_transition, but I don't intersect it -- if I don't find another, don't go!
			}
		pentVolume = FIND_ENTITY_BY_TARGETNAME (pentVolume, pVolumeName);
		}

	return inVolume;
	}

// We can only ever move 512 entities across a transition
#define MAX_ENTITY 512

// This has grown into a complicated beast
// Can we make this more elegant?
// This builds the list of all transitions on this level and which entities are in their PVS's and can / should
// be moved across.
int CChangeLevel::ChangeList (LEVELLIST *pLevelList, int maxList)
	{
	edict_t *pentChangelevel, *pentLandmark;
	int			i, count;

	count = 0;

	// Find all of the possible level changes on this BSP
	pentChangelevel = FIND_ENTITY_BY_STRING (NULL, "classname", "trigger_changelevel");
	if (FNullEnt (pentChangelevel))
		return 0;
	while (!FNullEnt (pentChangelevel))
		{
		CChangeLevel *pTrigger;

		pTrigger = GetClassPtr ((CChangeLevel *)VARS (pentChangelevel));
		if (pTrigger)
			{
			// Find the corresponding landmark
			pentLandmark = FindLandmark (pTrigger->m_szLandmarkName);
			if (pentLandmark)
				{
				// Build a list of unique transitions
				if (AddTransitionToList (pLevelList, count, pTrigger->m_szMapName, pTrigger->m_szLandmarkName, pentLandmark))
					{
					count++;
					if (count >= maxList)		// FULL!!
						break;
					}
				}
			}
		pentChangelevel = FIND_ENTITY_BY_STRING (pentChangelevel, "classname", "trigger_changelevel");
		}

	if (gpGlobals->pSaveData && ((SAVERESTOREDATA *)gpGlobals->pSaveData)->pTable)
		{
		CSave saveHelper ((SAVERESTOREDATA *)gpGlobals->pSaveData);

		for (i = 0; i < count; i++)
			{
			int j, entityCount = 0;
			CBaseEntity *pEntList[MAX_ENTITY];
			int			 entityFlags[MAX_ENTITY];

			// Follow the linked list of entities in the PVS of the transition landmark
			edict_t *pent = UTIL_EntitiesInPVS (pLevelList[i].pentLandmark);

			// Build a list of valid entities in this linked list (we're going to use pent->v.chain again)
			while (!FNullEnt (pent))
				{
				CBaseEntity *pEntity = CBaseEntity::Instance (pent);
				if (pEntity)
					{
					int caps = pEntity->ObjectCaps ();
					if (!(caps & FCAP_DONT_SAVE))
						{
						int flags = 0;

						// If this entity can be moved or is global, mark it
						if (caps & FCAP_ACROSS_TRANSITION)
							flags |= FENTTABLE_MOVEABLE;
						if (pEntity->pev->globalname && !pEntity->IsDormant ())
							flags |= FENTTABLE_GLOBAL;
						if (flags)
							{
							pEntList[entityCount] = pEntity;
							entityFlags[entityCount] = flags;
							entityCount++;
							if (entityCount > MAX_ENTITY)
								ALERT (at_error, "Too many entities across a transition!");
							}
						}
					}
				pent = pent->v.chain;
				}

			for (j = 0; j < entityCount; j++)
				{
				// Check to make sure the entity isn't screened out by a trigger_transition
				if (entityFlags[j] && InTransitionVolume (pEntList[j], pLevelList[i].landmarkName))
					{
					// Mark entity table with 1<<i
					int index = saveHelper.EntityIndex (pEntList[j]);
					// Flag it with the level number
					saveHelper.EntityFlagsSet (index, entityFlags[j] | (1 << i));
					}
				}
			}
		}

	return count;
	}

/***
go to the next level for deathmatch
only called if a time or frag limit has expired
***/
void NextLevel (void)
	{
	edict_t *pent;
	CChangeLevel *pChange;

	// find a trigger_changelevel
	pent = FIND_ENTITY_BY_CLASSNAME (NULL, "trigger_changelevel");

	// go back to start if no trigger_changelevel
	if (FNullEnt (pent))
		{
		gpGlobals->mapname = ALLOC_STRING ("start");
		pChange = GetClassPtr ((CChangeLevel *)NULL);
		strcpy (pChange->m_szMapName, "start");
		}
	else
		{
		pChange = GetClassPtr ((CChangeLevel *)VARS (pent));
		}

	strcpy (st_szNextMap, pChange->m_szMapName);
	g_fGameOver = TRUE;

	if (pChange->pev->nextthink < gpGlobals->time)
		{
		pChange->SetThink (&CChangeLevel::ExecuteChangeLevel);
		pChange->pev->nextthink = gpGlobals->time + 0.1;
		}
	}

// ============================== LADDER =======================================

class CLadder : public CBaseTrigger
	{
	public:
		void KeyValue (KeyValueData *pkvd);
		void Spawn (void);
		void Precache (void);
	};
LINK_ENTITY_TO_CLASS (func_ladder, CLadder);


void CLadder::KeyValue (KeyValueData *pkvd)
	{
	CBaseTrigger::KeyValue (pkvd);
	}


// =========================================================
// func_ladder - makes an area vertically negotiable
// =========================================================
void CLadder::Precache (void)
	{
	// Do all of this in here because we need to 'convert' old saved games
	pev->solid = SOLID_NOT;
	pev->skin = CONTENTS_LADDER;
	if (CVAR_GET_FLOAT ("showtriggers") == 0)
		{
		pev->rendermode = kRenderTransTexture;
		pev->renderamt = 0;
		}
	pev->effects &= ~EF_NODRAW;
	}

void CLadder::Spawn (void)
	{
	Precache ();

	SET_MODEL (ENT (pev), STRING (pev->model));    // set size and link into world
	pev->movetype = MOVETYPE_PUSH;
	}


// ========================== A TRIGGER THAT PUSHES YOU ===============================

class CTriggerPush : public CBaseTrigger
	{
	public:
		void Spawn (void);
		void KeyValue (KeyValueData *pkvd);
		void Touch (CBaseEntity *pOther);
	};
LINK_ENTITY_TO_CLASS (trigger_push, CTriggerPush);


void CTriggerPush::KeyValue (KeyValueData *pkvd)
	{
	CBaseTrigger::KeyValue (pkvd);
	}

// QUAKED trigger_push (.5 .5 .5) ? TRIG_PUSH_ONCE
// Pushes the player
void CTriggerPush::Spawn ()
	{
	if (pev->angles == g_vecZero)
		pev->angles.y = 360;
	InitTrigger ();

	if (pev->speed == 0)
		pev->speed = 100;

	// this flag was changed and flying barrels on c2a5 stay broken
	if (FStrEq (STRING (gpGlobals->mapname), "c2a5") && (pev->spawnflags & 4))
		pev->spawnflags |= SF_TRIG_PUSH_ONCE;

	if (FBitSet (pev->spawnflags, SF_TRIGGER_PUSH_START_OFF))// if flagged to Start Turned Off, make trigger nonsolid.
		pev->solid = SOLID_NOT;

	SetUse (&CBaseTrigger::ToggleUse);

	UTIL_SetOrigin (pev, pev->origin);		// Link into the list
	}

void CTriggerPush::Touch (CBaseEntity *pOther)
	{
	entvars_t *pevToucher = pOther->pev;

	// UNDONE: Is there a better way than health to detect things that have physics? (clients/monsters)
	switch (pevToucher->movetype)
		{
		case MOVETYPE_NONE:
		case MOVETYPE_PUSH:
		case MOVETYPE_NOCLIP:
		case MOVETYPE_FOLLOW:
			return;
		}

	if ((pevToucher->solid != SOLID_NOT) && (pevToucher->solid != SOLID_BSP))
		{
		// Instant trigger, just transfer velocity and remove
		if (FBitSet (pev->spawnflags, SF_TRIG_PUSH_ONCE))
			{
			pevToucher->velocity = pevToucher->velocity + (pev->speed * pev->movedir);
			if (pevToucher->velocity.z > 0)
				pevToucher->flags &= ~FL_ONGROUND;
			UTIL_Remove (this);
			}
		else
			{	// Push field, transfer to base velocity
			Vector vecPush = (pev->speed * pev->movedir);
			if (pevToucher->flags & FL_BASEVELOCITY)
				vecPush = vecPush + pevToucher->basevelocity;

			pevToucher->basevelocity = vecPush;
			pevToucher->flags |= FL_BASEVELOCITY;
			}
		}
	}


// ======================================
// teleport trigger
void CBaseTrigger::TeleportTouch (CBaseEntity *pOther)
	{
	entvars_t *pevToucher = pOther->pev;
	edict_t *pentTarget = NULL;

	// Only teleport monsters or clients
	if (!FBitSet (pevToucher->flags, FL_CLIENT | FL_MONSTER))
		return;

	if (!UTIL_IsMasterTriggered (m_sMaster, pOther))
		return;

	if (!(pev->spawnflags & SF_TRIGGER_ALLOWMONSTERS))
		{// no monsters allowed!
		if (FBitSet (pevToucher->flags, FL_MONSTER))
			{
			return;
			}
		}

	if ((pev->spawnflags & SF_TRIGGER_NOCLIENTS))
		{// no clients allowed
		if (pOther->IsPlayer ())
			{
			return;
			}
		}

	pentTarget = FIND_ENTITY_BY_TARGETNAME (pentTarget, STRING (pev->target));
	if (FNullEnt (pentTarget))
		return;

	Vector tmp = VARS (pentTarget)->origin;

	if (pOther->IsPlayer ())
		{
		tmp.z -= pOther->pev->mins.z;
		// make origin adjustments in case the teleportee is a player. (origin in center, not at feet)
		}

	tmp.z++;

	pevToucher->flags &= ~FL_ONGROUND;

	UTIL_SetOrigin (pevToucher, tmp);

	// ESHQ: ������ ���� ������ � �������� ��� �������������
	if (!FBitSet (pev->spawnflags, SF_TELEPORT_PRESERVE_VIEW))
		{
		pevToucher->angles = pentTarget->v.angles;

		if (pOther->IsPlayer ())
			{
			pevToucher->v_angle = pentTarget->v.angles;
			}

		pevToucher->fixangle = TRUE;

		pevToucher->velocity = pevToucher->basevelocity = g_vecZero;
		}
	else
		{
		pevToucher->fixangle = FALSE;
		}
	}

class CTriggerTeleport : public CBaseTrigger
	{
	public:
		void Spawn (void);
	};
LINK_ENTITY_TO_CLASS (trigger_teleport, CTriggerTeleport);

void CTriggerTeleport::Spawn (void)
	{
	InitTrigger ();

	SetTouch (&CBaseTrigger::TeleportTouch);
	}

LINK_ENTITY_TO_CLASS (info_teleport_destination, CPointEntity);

class CTriggerSave : public CBaseTrigger
	{
	public:
		void Spawn (void);
		void HLEXPORT SaveTouch (CBaseEntity *pOther);
	};
LINK_ENTITY_TO_CLASS (trigger_autosave, CTriggerSave);

void CTriggerSave::Spawn (void)
	{
	if (g_pGameRules->IsDeathmatch ())
		{
		REMOVE_ENTITY (ENT (pev));
		return;
		}

	InitTrigger ();
	SetTouch (&CTriggerSave::SaveTouch);
	}

void CTriggerSave::SaveTouch (CBaseEntity *pOther)
	{
	if (!UTIL_IsMasterTriggered (m_sMaster, pOther))
		return;

	// Only save on clients
	if (!pOther->IsPlayer ())
		return;

	SetTouch (NULL);
	UTIL_Remove (this);
	SERVER_COMMAND ("autosave\n");
	}

#define SF_ENDSECTION_USEONLY		0x0001

class CTriggerEndSection : public CBaseTrigger
	{
	public:
		void Spawn (void);
		void HLEXPORT EndSectionTouch (CBaseEntity *pOther);
		void KeyValue (KeyValueData *pkvd);
		void HLEXPORT EndSectionUse (CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	};
LINK_ENTITY_TO_CLASS (trigger_endsection, CTriggerEndSection);


void CTriggerEndSection::EndSectionUse (CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
	{
	// Only save on clients
	if (pActivator && !pActivator->IsNetClient ())
		return;

	SetUse (NULL);

	if (pev->message)
		{
		g_engfuncs.pfnEndSection (STRING (pev->message));
		}
	UTIL_Remove (this);
	}

void CTriggerEndSection::Spawn (void)
	{
	if (g_pGameRules->IsDeathmatch ())
		{
		REMOVE_ENTITY (ENT (pev));
		return;
		}

	InitTrigger ();

	SetUse (&CTriggerEndSection::EndSectionUse);

	// If it is a "use only" trigger, then don't set the touch function
	if (!(pev->spawnflags & SF_ENDSECTION_USEONLY))
		SetTouch (&CTriggerEndSection::EndSectionTouch);
	}

void CTriggerEndSection::EndSectionTouch (CBaseEntity *pOther)
	{
	// Only save on clients
	if (!pOther->IsNetClient ())
		return;

	SetTouch (NULL);

	if (pev->message)
		g_engfuncs.pfnEndSection (STRING (pev->message));

	UTIL_Remove (this);
	}

void CTriggerEndSection::KeyValue (KeyValueData *pkvd)
	{
	if (FStrEq (pkvd->szKeyName, "section"))
		{
		pev->message = ALLOC_STRING (pkvd->szValue);
		pkvd->fHandled = TRUE;
		}
	else
		{
		CBaseTrigger::KeyValue (pkvd);
		}
	}


class CTriggerGravity : public CBaseTrigger
	{
	public:
		void Spawn (void);
		void HLEXPORT GravityTouch (CBaseEntity *pOther);
		void HLEXPORT GravityUse (CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	};
LINK_ENTITY_TO_CLASS (trigger_gravity, CTriggerGravity);

void CTriggerGravity::Spawn (void)
	{
	InitTrigger ();
	SetTouch (&CTriggerGravity::GravityTouch);
	SetUse (&CTriggerGravity::GravityUse);
	}

// ESHQ: ����������� ��������� �������� ����������
static cvar_t *g_psv_gravity = NULL;

void CTriggerGravity::GravityTouch (CBaseEntity *pOther)
	{
	// Only save on clients
	if (!pOther->IsPlayer ())
		return;

	// ESHQ: ����������� ��������� �������� ����������
	// 27.06.24 � �������-���� ���������!

	// ���������� �� ������
	if (pev->gravity < 80.0f)
		{
		pOther->pev->gravity = pev->gravity;
		}

	// ���������� ����� ������������
	else
		{
		// ���� ���
		if (!g_psv_gravity)
			g_psv_gravity = CVAR_GET_POINTER ("sv_gravity");

		// ������ �������� � ������������ ����
		if (g_psv_gravity)
			{
			g_psv_gravity->value = pev->gravity;
			WRITE_ACHIEVEMENTS_SCRIPT (1, -1);
			}

		pOther->pev->gravity = 1.0f;
		}
	}

// ESHQ: ���������� ����� ����� ���
void CTriggerGravity::GravityUse (CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
	{
	GravityTouch (UTIL_PlayerByIndex (1));
	}

// this is a really bad idea
class CTriggerChangeTarget : public CBaseDelay
	{
	public:
		void KeyValue (KeyValueData *pkvd);
		void Spawn (void);
		void Use (CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

		int ObjectCaps (void) { return CBaseDelay::ObjectCaps () & ~FCAP_ACROSS_TRANSITION; }
		virtual int		Save (CSave &save);
		virtual int		Restore (CRestore &restore);

		static	TYPEDESCRIPTION m_SaveData[];

	private:
		int		m_iszNewTarget;
	};
LINK_ENTITY_TO_CLASS (trigger_changetarget, CTriggerChangeTarget);

TYPEDESCRIPTION	CTriggerChangeTarget::m_SaveData[] =
	{
		DEFINE_FIELD (CTriggerChangeTarget, m_iszNewTarget, FIELD_STRING),
	};

IMPLEMENT_SAVERESTORE (CTriggerChangeTarget, CBaseDelay);

void CTriggerChangeTarget::KeyValue (KeyValueData *pkvd)
	{
	if (FStrEq (pkvd->szKeyName, "m_iszNewTarget"))
		{
		m_iszNewTarget = ALLOC_STRING (pkvd->szValue);
		pkvd->fHandled = TRUE;
		}
	else
		{
		CBaseDelay::KeyValue (pkvd);
		}
	}

void CTriggerChangeTarget::Spawn (void)
	{
	}

void CTriggerChangeTarget::Use (CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
	{
	CBaseEntity *pTarget = UTIL_FindEntityByString (NULL, "targetname", STRING (pev->target));

	if (pTarget)
		{
		pTarget->pev->target = m_iszNewTarget;
		CBaseMonster *pMonster = pTarget->MyMonsterPointer ();
		if (pMonster)
			{
			pMonster->m_pGoalEnt = NULL;
			}
		}
	}

#define SF_CAMERA_PLAYER_POSITION	1
#define SF_CAMERA_PLAYER_TARGET		2
#define SF_CAMERA_PLAYER_TAKECONTROL 4

class CTriggerCamera : public CBaseDelay
	{
	public:
		void Spawn (void);
		void KeyValue (KeyValueData *pkvd);
		void Use (CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
		void HLEXPORT FollowTarget (void);
		void Move (void);

		virtual int		Save (CSave &save);
		virtual int		Restore (CRestore &restore);
		virtual int	ObjectCaps (void) { return CBaseEntity::ObjectCaps () & ~FCAP_ACROSS_TRANSITION; }
		static	TYPEDESCRIPTION m_SaveData[];

		EHANDLE m_hPlayer;
		EHANDLE m_hTarget;
		CBaseEntity *m_pentPath;
		int	  m_sPath;
		float m_flWait;
		float m_flReturnTime;
		float m_flStopTime;
		float m_moveDistance;
		float m_targetSpeed;
		float m_initialSpeed;
		float m_acceleration;
		float m_deceleration;
		int	  m_state;

	};
LINK_ENTITY_TO_CLASS (trigger_camera, CTriggerCamera);

// Global Savedata for changelevel friction modifier
TYPEDESCRIPTION	CTriggerCamera::m_SaveData[] =
	{
	DEFINE_FIELD (CTriggerCamera, m_hPlayer, FIELD_EHANDLE),
	DEFINE_FIELD (CTriggerCamera, m_hTarget, FIELD_EHANDLE),
	DEFINE_FIELD (CTriggerCamera, m_pentPath, FIELD_CLASSPTR),
	DEFINE_FIELD (CTriggerCamera, m_sPath, FIELD_STRING),
	DEFINE_FIELD (CTriggerCamera, m_flWait, FIELD_FLOAT),
	DEFINE_FIELD (CTriggerCamera, m_flReturnTime, FIELD_TIME),
	DEFINE_FIELD (CTriggerCamera, m_flStopTime, FIELD_TIME),
	DEFINE_FIELD (CTriggerCamera, m_moveDistance, FIELD_FLOAT),
	DEFINE_FIELD (CTriggerCamera, m_targetSpeed, FIELD_FLOAT),
	DEFINE_FIELD (CTriggerCamera, m_initialSpeed, FIELD_FLOAT),
	DEFINE_FIELD (CTriggerCamera, m_acceleration, FIELD_FLOAT),
	DEFINE_FIELD (CTriggerCamera, m_deceleration, FIELD_FLOAT),
	DEFINE_FIELD (CTriggerCamera, m_state, FIELD_INTEGER),
	};

IMPLEMENT_SAVERESTORE (CTriggerCamera, CBaseDelay);

void CTriggerCamera::Spawn (void)
	{
	pev->movetype = MOVETYPE_NOCLIP;
	pev->solid = SOLID_NOT;				// Remove model & collisions
	pev->renderamt = 0;					// The engine won't draw this model if this is set to 0 and blending is on
	pev->rendermode = kRenderTransTexture;

	m_initialSpeed = pev->speed;

	// ESHQ: ������� �������� m_acceleration ����� ������������ ��� ������ ���������
	}

void CTriggerCamera::KeyValue (KeyValueData *pkvd)
	{
	if (FStrEq (pkvd->szKeyName, "wait"))
		{
		m_flWait = atof (pkvd->szValue);
		pkvd->fHandled = TRUE;
		}
	else if (FStrEq (pkvd->szKeyName, "moveto"))
		{
		m_sPath = ALLOC_STRING (pkvd->szValue);
		pkvd->fHandled = TRUE;
		}
	else if (FStrEq (pkvd->szKeyName, "acceleration"))
		{
		m_acceleration = atof (pkvd->szValue);
		pkvd->fHandled = TRUE;
		}
	else if (FStrEq (pkvd->szKeyName, "deceleration"))
		{
		m_deceleration = atof (pkvd->szValue);
		pkvd->fHandled = TRUE;
		}
	else
		{
		CBaseDelay::KeyValue (pkvd);
		}
	}

void CTriggerCamera::Use (CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
	{
	if (!ShouldToggle (useType, m_state))
		return;

	// Toggle state
	m_state = !m_state;
	if (m_state == 0)
		{
		m_flReturnTime = gpGlobals->time;
		return;
		}
	if (!pActivator || !pActivator->IsPlayer ())
		pActivator = CBaseEntity::Instance (g_engfuncs.pfnPEntityOfEntIndex (1));

	m_hPlayer = pActivator;
	m_flReturnTime = gpGlobals->time + m_flWait;
	pev->speed = m_initialSpeed;
	m_targetSpeed = m_initialSpeed;

	if (FBitSet (pev->spawnflags, SF_CAMERA_PLAYER_TARGET))
		m_hTarget = m_hPlayer;
	else
		m_hTarget = GetNextTarget ();

	// Nothing to look at!
	if (m_hTarget == NULL)
		return;

	if (FBitSet (pev->spawnflags, SF_CAMERA_PLAYER_TAKECONTROL))
		((CBasePlayer *)pActivator)->EnableControl (FALSE);

	if (m_sPath)
		m_pentPath = Instance (FIND_ENTITY_BY_TARGETNAME (NULL, STRING (m_sPath)));
	else
		m_pentPath = NULL;

	m_flStopTime = gpGlobals->time;
	if (m_pentPath)
		{
		if (m_pentPath->pev->speed != 0)
			m_targetSpeed = m_pentPath->pev->speed;

		m_flStopTime += m_pentPath->GetDelay ();
		}

	// copy over player information
	if (FBitSet (pev->spawnflags, SF_CAMERA_PLAYER_POSITION))
		{
		UTIL_SetOrigin (pev, pActivator->pev->origin + pActivator->pev->view_ofs);
		pev->angles.x = -pActivator->pev->angles.x;
		pev->angles.y = pActivator->pev->angles.y;
		pev->angles.z = 0;
		pev->velocity = pActivator->pev->velocity;
		}
	else
		{
		pev->velocity = Vector (0, 0, 0);
		}

	SET_VIEW (pActivator->edict (), edict ());
	SET_MODEL (ENT (pev), STRING (pActivator->pev->model));

	// follow the player down
	SetThink (&CTriggerCamera::FollowTarget);
	pev->nextthink = gpGlobals->time;

	m_moveDistance = 0;
	Move ();
	}

void CTriggerCamera::FollowTarget ()
	{
	if (m_hPlayer == NULL)
		return;

	if ((m_hTarget == NULL) || (m_flReturnTime < gpGlobals->time))
		{
		if (m_hPlayer->IsAlive ())
			{
			SET_VIEW (m_hPlayer->edict (), m_hPlayer->edict ());
			((CBasePlayer *)((CBaseEntity *)m_hPlayer))->EnableControl (TRUE);
			}

		SUB_UseTargets (this, USE_TOGGLE, 0);
		pev->avelocity = Vector (0, 0, 0);
		m_state = 0;
		return;
		}

	Vector vecGoal = UTIL_VecToAngles (m_hTarget->pev->origin - pev->origin);
	vecGoal.x = -vecGoal.x;

	if (pev->angles.y > 360)
		pev->angles.y -= 360;

	if (pev->angles.y < 0)
		pev->angles.y += 360;

	float dx = vecGoal.x - pev->angles.x;
	float dy = vecGoal.y - pev->angles.y;

	if (dx < -180)
		dx += 360;
	if (dx > 180)
		dx = dx - 360;

	if (dy < -180)
		dy += 360;
	if (dy > 180)
		dy = dy - 360;

	if ((int)m_deceleration * (int)m_acceleration != 0)
		{
		pev->avelocity.x = dx * 40 * gpGlobals->frametime;
		pev->avelocity.y = dy * 40 * gpGlobals->frametime;
		}
	else
		{
		pev->angles = vecGoal;
		}

	if (!(FBitSet (pev->spawnflags, SF_CAMERA_PLAYER_TAKECONTROL)))
		{
		pev->velocity = pev->velocity * 0.8;
		if (pev->velocity.Length () < 10.0)
			pev->velocity = g_vecZero;
		}

	pev->nextthink = gpGlobals->time;

	Move ();
	}

void CTriggerCamera::Move ()
	{
	// Not moving on a path, return
	if (!m_pentPath)
		return;

	// Subtract movement from the previous frame
	m_moveDistance -= pev->speed * gpGlobals->frametime;

	// Have we moved enough to reach the target?
	if (m_moveDistance <= 0)
		{
		// Fire the passtarget if there is one
		if (m_pentPath->pev->message)
			{
			FireTargets (STRING (m_pentPath->pev->message), this, this, USE_TOGGLE, 0);
			if (FBitSet (m_pentPath->pev->spawnflags, SF_CORNER_FIREONCE))
				m_pentPath->pev->message = 0;
			}

		// Time to go to the next target
		m_pentPath = m_pentPath->GetNextTarget ();

		// Set up next corner
		if (!m_pentPath)
			{
			pev->velocity = g_vecZero;
			}
		else
			{
			if (m_pentPath->pev->speed != 0)
				m_targetSpeed = m_pentPath->pev->speed;

			Vector delta = m_pentPath->pev->origin - pev->origin;
			m_moveDistance = delta.Length ();
			pev->movedir = delta.Normalize ();
			m_flStopTime = gpGlobals->time + m_pentPath->GetDelay ();
			}
		}

	// ESHQ: ��������� ���������� ����������
	if ((int)m_deceleration * (int)m_acceleration != 0)
		{
		if (m_flStopTime > gpGlobals->time)
			pev->speed = UTIL_Approach (0, pev->speed, m_deceleration * gpGlobals->frametime);
		else
			pev->speed = UTIL_Approach (m_targetSpeed, pev->speed, m_acceleration * gpGlobals->frametime);
		}

	float fraction = 2 * gpGlobals->frametime;
	pev->velocity = ((pev->movedir * pev->speed) * fraction) + (pev->velocity * (1 - fraction));
	}

// **********************************************************
// ESHQ: ��������� ���������� ������������
// Trigger random - when fired, will randomly fire one of up to 16 targets
// FLAG:		THREAD (create clones when triggered)
// FLAG:		CLONE (this is a clone for a threaded execution)

#define MAX_RANDOM_RATE	10

// Generic implementation
class CTriggerRandom : public CBaseToggle
	{
	public:
		void KeyValue (KeyValueData *pkvd);
		void Spawn (void);
		void HLEXPORT ManagerThink (void);
		void HLEXPORT ManagerUse (CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

		BOOL HasTarget (string_t targetname);

		int ObjectCaps (void)
			{
			return CBaseToggle::ObjectCaps () & ~FCAP_ACROSS_TRANSITION;
			}

		virtual int Save (CSave &save);
		virtual int Restore (CRestore &restore);

		static	TYPEDESCRIPTION m_SaveData[];

		int		m_cTargets;		// Targets in list
		float	m_startTime;	// (probably, not needed)

		// List of targets; for more or less probable events
		// same targets will be added one or more times (up to MAX_RANDOM_RATE)
		int		m_iTargetName[MAX_MULTI_TARGETS * MAX_RANDOM_RATE];

	private:
		inline BOOL IsClone (void)
			{
			return (pev->spawnflags & SF_MULTIMAN_CLONE) ? TRUE : FALSE;
			}
		inline BOOL ShouldClone (void)
			{
			if (IsClone ())
				return FALSE;

			return (pev->spawnflags & SF_MULTIMAN_THREAD) ? TRUE : FALSE;
			}

		CTriggerRandom *Clone (void);
	};

LINK_ENTITY_TO_CLASS (trigger_random, CTriggerRandom);

TYPEDESCRIPTION	CTriggerRandom::m_SaveData[] =
	{
	DEFINE_FIELD (CTriggerRandom, m_cTargets, FIELD_INTEGER),
	DEFINE_FIELD (CTriggerRandom, m_startTime, FIELD_TIME),
	DEFINE_ARRAY (CTriggerRandom, m_iTargetName, FIELD_STRING, MAX_MULTI_TARGETS * MAX_RANDOM_RATE),
	};

IMPLEMENT_SAVERESTORE (CTriggerRandom, CBaseToggle);

// Creating a list of targets taking into account the probabilities
void CTriggerRandom::KeyValue (KeyValueData *pkvd)
	{
	if (m_cTargets < MAX_MULTI_TARGETS * MAX_RANDOM_RATE)
		{
		char tmp[128];

		// Rendering a proportion value
		int rate = atoi (pkvd->szValue);
		if ((rate < 1) || (rate > MAX_RANDOM_RATE))
			rate = 1;

		// Adding a target
		UTIL_StripToken (pkvd->szKeyName, tmp);
		for (int i = 0; i < rate; i++)
			{
			m_iTargetName[m_cTargets] = ALLOC_STRING (tmp);
			m_cTargets++;
			}
		pkvd->fHandled = TRUE;
		}
	}

// Initializing an entity
void CTriggerRandom::Spawn (void)
	{
	pev->solid = SOLID_NOT;
	SetUse (&CTriggerRandom::ManagerUse);
	SetThink (&CTriggerRandom::ManagerThink);
	}

BOOL CTriggerRandom::HasTarget (string_t targetname)
	{
	for (int i = 0; i < m_cTargets; i++)
		if (FStrEq (STRING (targetname), STRING (m_iTargetName[i])))
			return TRUE;

	return FALSE;
	}

// Shooting target
void CTriggerRandom::ManagerThink (void)
	{
	float time;
	time = gpGlobals->time - m_startTime;	// probably, not needed

	// Shooting
	// Due to different counts of same targets you will have different probabilities for them.
	// Highest count - highest probability
	FireTargets (STRING (m_iTargetName[RANDOM_LONG (0, m_cTargets - 1)]), m_hActivator, this, USE_TOGGLE, 0);

	// Stand-by
	SetThink (NULL);
	if (IsClone ())
		{
		UTIL_Remove (this);
		return;
		}
	SetUse (&CTriggerRandom::ManagerUse);	// Allow to re-use trigger
	}

// Cloning for multiplayer mode
CTriggerRandom *CTriggerRandom::Clone (void)
	{
	CTriggerRandom *pMulti = GetClassPtr ((CTriggerRandom *)NULL);

	edict_t *pEdict = pMulti->pev->pContainingEntity;
	memcpy (pMulti->pev, pev, sizeof (*pev));
	pMulti->pev->pContainingEntity = pEdict;

	pMulti->pev->spawnflags |= SF_MULTIMAN_CLONE;
	pMulti->m_cTargets = m_cTargets;
	memcpy (pMulti->m_iTargetName, m_iTargetName, sizeof (m_iTargetName));

	return pMulti;
	}

// Activating a trigger
void CTriggerRandom::ManagerUse (CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
	{
	// In multiplayer games, clone the MM and execute in the clone (like a thread)
	// to allow multiple players to trigger the same multimanager
	if (ShouldClone ())
		{
		CTriggerRandom *pClone = Clone ();
		pClone->ManagerUse (pActivator, pCaller, useType, value);
		return;
		}

	m_hActivator = pActivator;
	m_startTime = gpGlobals->time;

	SetUse (NULL);	// Disable use until target have fired

	SetThink (&CTriggerRandom::ManagerThink);
	pev->nextthink = gpGlobals->time;
	}
