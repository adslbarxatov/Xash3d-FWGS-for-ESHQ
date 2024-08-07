/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*
*	This product contains software technology licensed from Id
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc.
*	All Rights Reserved.
*
*   This source code contains proprietary and confidential information of
*   Valve LLC and its suppliers.  Access to this code is restricted to
*   persons who have executed a written SDK license with Valve.  Any access,
*   use or distribution of this code by or to any unlicensed person is illegal.
*
****/
// =========================================================
// Zombie
// =========================================================

// UNDONE: Don't flinch every time you get hit
#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"

// =========================================================
// Monster's Anim Events Go Here
// =========================================================
#define	ZOMBIE_AE_ATTACK_RIGHT		0x01
#define	ZOMBIE_AE_ATTACK_LEFT		0x02
#define	ZOMBIE_AE_ATTACK_BOTH		0x03

#define ZOMBIE_FLINCH_DELAY			2		// at most one flinch every n secs

class CZombie: public CBaseMonster
	{
	public:
		void Spawn (void);
		void Precache (void);
		void SetYawSpeed (void);
		int  Classify (void);
		void HandleAnimEvent (MonsterEvent_t* pEvent);
		int IgnoreConditions (void);

		float m_flNextFlinch;

		void PainSound (void);
		void AlertSound (void);
		void IdleSound (void);
		void AttackSound (void);

		static const char* pAttackSounds[];
		static const char* pIdleSounds[];
		static const char* pAlertSounds[];
		static const char* pPainSounds[];
		static const char* pAttackHitSounds[];
		static const char* pAttackMissSounds[];

		// No range attacks
		BOOL CheckRangeAttack1 (float flDot, float flDist) { return FALSE; }
		BOOL CheckRangeAttack2 (float flDot, float flDist) { return FALSE; }
		int TakeDamage (entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType);

		// ESHQ: ��������� �������������� ����������
		void KeyValue (KeyValueData* pkvd);
	};

LINK_ENTITY_TO_CLASS (monster_zombie, CZombie);
LINK_ENTITY_TO_CLASS (monster_zombie2, CZombie);		// ESHQ: ��������� AOMDC
LINK_ENTITY_TO_CLASS (monster_wheelchair, CZombie);
LINK_ENTITY_TO_CLASS (monster_zombie3, CZombie);
LINK_ENTITY_TO_CLASS (monster_zombie4, CZombie);
LINK_ENTITY_TO_CLASS (monster_david, CZombie);

const char* CZombie::pAttackHitSounds[] =
	{
		"zombie/claw_strike1.wav",
		"zombie/claw_strike2.wav",
		"zombie/claw_strike3.wav",
	};

const char* CZombie::pAttackMissSounds[] =
	{
		"zombie/claw_miss1.wav",
		"zombie/claw_miss2.wav",
	};

const char* CZombie::pAttackSounds[] =
	{
		"zombie/zo_attack1.wav",
		"zombie/zo_attack2.wav",
	};

const char* CZombie::pIdleSounds[] =
	{
		"zombie/zo_idle1.wav",
		"zombie/zo_idle2.wav",
		"zombie/zo_idle3.wav",
		"zombie/zo_idle4.wav",
	};

const char* CZombie::pAlertSounds[] =
	{
		"zombie/zo_alert10.wav",
		"zombie/zo_alert20.wav",
		"zombie/zo_alert30.wav",
	};

const char* CZombie::pPainSounds[] =
	{
		"zombie/zo_pain1.wav",
		"zombie/zo_pain2.wav",
	};

// =========================================================
// Classify - indicates this monster's place in the 
// relationship table
// =========================================================
int	CZombie::Classify (void)
	{
	// ESHQ: ����� ��������� ������
	return	CLASS_HUMAN_ASSASSIN;
	}

// =========================================================
// SetYawSpeed - allows each sequence to have a different
// turn rate associated with it.
// =========================================================
void CZombie::SetYawSpeed (void)
	{
	pev->yaw_speed = 120;
	}

int CZombie::TakeDamage (entvars_t* pevInflictor, entvars_t* pevAttacker, float flDamage, int bitsDamageType)
	{
	// Take 30% damage from bullets
	if (bitsDamageType == DMG_BULLET)
		{
		Vector vecDir = pev->origin - (pevInflictor->absmin + pevInflictor->absmax) * 0.5;
		vecDir = vecDir.Normalize ();
		float flForce = DamageForce (flDamage);
		pev->velocity = pev->velocity + vecDir * flForce;
		flDamage *= 0.3;
		}

	// HACK HACK -- until we fix this
	if (IsAlive ())
		PainSound ();

	return CBaseMonster::TakeDamage (pevInflictor, pevAttacker, flDamage, bitsDamageType);
	}

void CZombie::PainSound (void)
	{
	int pitch = 95 + RANDOM_LONG (0, 9);

	if (RANDOM_LONG (0, 5) < 2)
		EMIT_SOUND_DYN (ENT (pev), CHAN_VOICE, pPainSounds[RANDOM_LONG (0, HLARRAYSIZE (pPainSounds) - 1)], 
			1.0, ATTN_MEDIUM, 0, pitch);
	}

void CZombie::AlertSound (void)
	{
	int pitch = 95 + RANDOM_LONG (0, 9);

	EMIT_SOUND_DYN (ENT (pev), CHAN_VOICE, pAlertSounds[RANDOM_LONG (0, HLARRAYSIZE (pAlertSounds) - 1)], 
		1.0, ATTN_MEDIUM, 0, pitch);
	}

void CZombie::IdleSound (void)
	{
	int pitch = 95 + RANDOM_LONG (0, 9);

	// Play a random idle sound
	EMIT_SOUND_DYN (ENT (pev), CHAN_VOICE, pIdleSounds[RANDOM_LONG (0, HLARRAYSIZE (pIdleSounds) - 1)], 1.0,
		ATTN_SMALL, 0, 100 + RANDOM_LONG (-5, 5));
	}

void CZombie::AttackSound (void)
	{
	// Play a random attack sound
	EMIT_SOUND_DYN (ENT (pev), CHAN_VOICE, pAttackSounds[RANDOM_LONG (0, HLARRAYSIZE (pAttackSounds) - 1)], 1.0,
		ATTN_MEDIUM, 0, 100 + RANDOM_LONG (-5, 5));
	}


// =========================================================
// HandleAnimEvent - catches the monster-specific messages
// that occur when tagged animation frames are played
// =========================================================
void CZombie::HandleAnimEvent (MonsterEvent_t* pEvent)
	{
	switch (pEvent->event)
		{
		case ZOMBIE_AE_ATTACK_RIGHT:
			{
			// do stuff for this event
			CBaseEntity* pHurt = CheckTraceHullAttack (70, gSkillData.zombieDmgOneSlash, DMG_SLASH);
			if (pHurt)
				{
				if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT))
					{
					pHurt->pev->punchangle.z = -18;
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity - gpGlobals->v_right * 100;
					}

				// Play a random attack hit sound
				EMIT_SOUND_DYN (ENT (pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG (0,
					HLARRAYSIZE (pAttackHitSounds) - 1)], 1.0, ATTN_MEDIUM, 0, 100 + RANDOM_LONG (-5, 5));
				}
			else // Play a random attack miss sound
				{
				EMIT_SOUND_DYN (ENT (pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG (0,
					HLARRAYSIZE (pAttackMissSounds) - 1)], 1.0, ATTN_MEDIUM, 0, 100 + RANDOM_LONG (-5, 5));
				}

			if (RANDOM_LONG (0, 1))
				AttackSound ();
			}
			break;

		case ZOMBIE_AE_ATTACK_LEFT:
			{
			// do stuff for this event
			CBaseEntity* pHurt = CheckTraceHullAttack (70, gSkillData.zombieDmgOneSlash, DMG_SLASH);
			if (pHurt)
				{
				if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT))
					{
					pHurt->pev->punchangle.z = 18;
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_right * 100;
					}
				EMIT_SOUND_DYN (ENT (pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG (0,
					HLARRAYSIZE (pAttackHitSounds) - 1)], 1.0, ATTN_MEDIUM, 0, 100 + RANDOM_LONG (-5, 5));
				}
			else
				{
				EMIT_SOUND_DYN (ENT (pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG (0,
					HLARRAYSIZE (pAttackMissSounds) - 1)], 1.0, ATTN_MEDIUM, 0, 100 + RANDOM_LONG (-5, 5));
				}

			if (RANDOM_LONG (0, 1))
				AttackSound ();
			}
			break;

		case ZOMBIE_AE_ATTACK_BOTH:
			{
			// do stuff for this event
			CBaseEntity* pHurt = CheckTraceHullAttack (70, gSkillData.zombieDmgBothSlash, DMG_SLASH);
			if (pHurt)
				{
				if (pHurt->pev->flags & (FL_MONSTER | FL_CLIENT))
					{
					pHurt->pev->punchangle.x = 5;
					pHurt->pev->velocity = pHurt->pev->velocity + gpGlobals->v_forward * -100;
					}
				EMIT_SOUND_DYN (ENT (pev), CHAN_WEAPON, pAttackHitSounds[RANDOM_LONG (0,
					HLARRAYSIZE (pAttackHitSounds) - 1)], 1.0, ATTN_MEDIUM, 0, 100 + RANDOM_LONG (-5, 5));
				}
			else
				{
				EMIT_SOUND_DYN (ENT (pev), CHAN_WEAPON, pAttackMissSounds[RANDOM_LONG (0,
					HLARRAYSIZE (pAttackMissSounds) - 1)], 1.0, ATTN_MEDIUM, 0, 100 + RANDOM_LONG (-5, 5));
				}

			if (RANDOM_LONG (0, 1))
				AttackSound ();
			}
			break;

		default:
			CBaseMonster::HandleAnimEvent (pEvent);
			break;
		}
	}

// =========================================================
// Spawn
// =========================================================
void CZombie::Spawn ()
	{
	Precache ();

	SET_MODEL (ENT (pev), "models/zombie.mdl");
	UTIL_SetSize (pev, VEC_HUMAN_HULL_MIN, VEC_HUMAN_HULL_MAX);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_STEP;

	// ESHQ
	m_bloodColor = BLOOD_COLOR_RED;

	pev->health = gSkillData.zombieHealth;
	pev->view_ofs = VEC_VIEW;	// position of the eyes relative to monster's origin
	m_flFieldOfView = 0.5;		// indicates the width of this monster's forward view cone (as a dotproduct result)
	m_MonsterState = MONSTERSTATE_NONE;
	m_afCapability = bits_CAP_DOORS_GROUP;

	// ESHQ: ��������� ������
	if ((pev->skin < 0) || (pev->skin > 1))
		pev->skin = 0;

	MonsterInit ();
	}

// =========================================================
// Precache - precaches all resources this monster needs
// =========================================================
void CZombie::Precache ()
	{
	int i;

	PRECACHE_MODEL ("models/zombie.mdl");

	for (i = 0; i < HLARRAYSIZE (pAttackHitSounds); i++)
		PRECACHE_SOUND ((char*)pAttackHitSounds[i]);

	for (i = 0; i < HLARRAYSIZE (pAttackMissSounds); i++)
		PRECACHE_SOUND ((char*)pAttackMissSounds[i]);

	for (i = 0; i < HLARRAYSIZE (pAttackSounds); i++)
		PRECACHE_SOUND ((char*)pAttackSounds[i]);

	for (i = 0; i < HLARRAYSIZE (pIdleSounds); i++)
		PRECACHE_SOUND ((char*)pIdleSounds[i]);

	for (i = 0; i < HLARRAYSIZE (pAlertSounds); i++)
		PRECACHE_SOUND ((char*)pAlertSounds[i]);

	for (i = 0; i < HLARRAYSIZE (pPainSounds); i++)
		PRECACHE_SOUND ((char*)pPainSounds[i]);
	}

int CZombie::IgnoreConditions (void)
	{
	int iIgnore = CBaseMonster::IgnoreConditions ();

	if ((m_Activity == ACT_MELEE_ATTACK1) || (m_Activity == ACT_MELEE_ATTACK1))
		{
		if (m_flNextFlinch >= gpGlobals->time)
			iIgnore |= (bits_COND_LIGHT_DAMAGE | bits_COND_HEAVY_DAMAGE);
		}

	if ((m_Activity == ACT_SMALL_FLINCH) || (m_Activity == ACT_BIG_FLINCH))
		{
		if (m_flNextFlinch < gpGlobals->time)
			m_flNextFlinch = gpGlobals->time + ZOMBIE_FLINCH_DELAY;
		}

	return iIgnore;
	}

// ESHQ: ��������� ������
void CZombie::KeyValue (KeyValueData* pkvd)
	{
	if (FStrEq (pkvd->szKeyName, "skin"))
		{
		pev->skin = atoi (pkvd->szValue);
		pkvd->fHandled = TRUE;
		}
	else
		{
		CBaseMonster::KeyValue (pkvd);
		}
	}


// =========================================================
// ESHQ: ������ �����
// =========================================================
class CDeadZombie: public CBaseMonster
	{
	public:
		void Spawn (void);
		int	Classify (void) { return CLASS_HUMAN_ASSASSIN; }

		void KeyValue (KeyValueData* pkvd);

		int	m_iPose;
		static char* m_szPoses[2];
	};

char* CDeadZombie::m_szPoses[] = {"dead_back", "dead_stomach"};

void CDeadZombie::KeyValue (KeyValueData* pkvd)
	{
	if (FStrEq (pkvd->szKeyName, "pose"))
		{
		m_iPose = atoi (pkvd->szValue);
		pkvd->fHandled = TRUE;
		}
	else if (FStrEq (pkvd->szKeyName, "skin"))
		{
		pev->skin = atoi (pkvd->szValue);
		pkvd->fHandled = TRUE;
		}
	else
		{
		CBaseMonster::KeyValue (pkvd);
		}
	}

LINK_ENTITY_TO_CLASS (monster_zombie_dead, CDeadZombie);

void CDeadZombie::Spawn (void)
	{
	PRECACHE_MODEL ("models/zombie.mdl");
	SET_MODEL (ENT (pev), "models/zombie.mdl");

	pev->effects = 0;
	pev->yaw_speed = 8;
	pev->sequence = 0;
	m_bloodColor = BLOOD_COLOR_RED;

	pev->sequence = LookupSequence (m_szPoses[m_iPose]);
	if (pev->sequence == -1)
		{
		ALERT (at_console, "Dead zombie with bad pose\n");
		}

	// Corpses have less health
	pev->health = 20;

	if ((pev->skin < 0) || (pev->skin > 1))
		pev->skin = 0;

	MonsterInitDead ();
	}

// =========================================================
// ESHQ: ������� �����
// =========================================================
class CBurnedZombie: public CBaseMonster
	{
	public:
		void Spawn (void);
		int	Classify (void) { return	CLASS_HUMAN_ASSASSIN; }

		void KeyValue (KeyValueData* pkvd);

		int	m_iPose;
		static char* m_szPoses[2];
	};

char* CBurnedZombie::m_szPoses[] = {"dead_back", "dead_stomach"};

void CBurnedZombie::KeyValue (KeyValueData* pkvd)
	{
	if (FStrEq (pkvd->szKeyName, "pose"))
		{
		m_iPose = atoi (pkvd->szValue);
		pkvd->fHandled = TRUE;
		}
	else
		{
		CBaseMonster::KeyValue (pkvd);
		}
	}

LINK_ENTITY_TO_CLASS (monster_zombie_burned, CBurnedZombie);

void CBurnedZombie::Spawn (void)
	{
	PRECACHE_MODEL ("models/zombie.mdl");
	SET_MODEL (ENT (pev), "models/zombie.mdl");

	pev->effects = 0;
	pev->yaw_speed = 8;
	pev->sequence = 0;
	m_bloodColor = BLOOD_COLOR_RED;

	pev->sequence = LookupSequence (m_szPoses[m_iPose]);
	if (pev->sequence == -1)
		{
		ALERT (at_console, "Burned zombie with bad pose\n");
		}

	pev->health = 20;
	pev->skin = 1;

	MonsterInitDead ();
	}
