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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "gamerules.h"


#define	CROWBAR_BODYHIT_VOLUME 128
#define	CROWBAR_WALLHIT_VOLUME 512

LINK_ENTITY_TO_CLASS (weapon_crowbar, CCrowbar);
LINK_ENTITY_TO_CLASS (weapon_spear, CCrowbar);			// ESHQ: ��������� �������� AOMDC
LINK_ENTITY_TO_CLASS (weapon_knife, CCrowbar);
LINK_ENTITY_TO_CLASS (weapon_hammer, CCrowbar);

// ESHQ: ����� �������
void FindHullIntersection (const Vector &vecSrc, TraceResult &tr, float *mins, float *maxs, edict_t *pEntity)
	{
	int			i, j, k;
	float		distance;
	float		*minmaxs[2] = { mins, maxs };
	TraceResult	tmpTrace;
	Vector		vecHullEnd = tr.vecEndPos;
	Vector		vecEnd;

	distance = 1e6f;

	vecHullEnd = vecSrc + ((vecHullEnd - vecSrc) * 2);
	UTIL_TraceLine (vecSrc, vecHullEnd, dont_ignore_monsters, pEntity, &tmpTrace);
	if (tmpTrace.flFraction < 1.0)
		{
		tr = tmpTrace;
		return;
		}

	for (i = 0; i < 2; i++)
		{
		for (j = 0; j < 2; j++)
			{
			for (k = 0; k < 2; k++)
				{
				vecEnd.x = vecHullEnd.x + minmaxs[i][0];
				vecEnd.y = vecHullEnd.y + minmaxs[j][1];
				vecEnd.z = vecHullEnd.z + minmaxs[k][2];

				UTIL_TraceLine (vecSrc, vecEnd, dont_ignore_monsters, pEntity, &tmpTrace);
				if (tmpTrace.flFraction < 1.0)
					{
					float thisDistance = (tmpTrace.vecEndPos - vecSrc).Length ();
					if (thisDistance < distance)
						{
						tr = tmpTrace;
						distance = thisDistance;
						}
					}
				}
			}
		}
	}

// ESHQ: ������� ��� ����������
enum gauss_e
	{
	CROWBAR_IDLE = 0,
	CROWBAR_DRAW,
	CROWBAR_HOLSTER,
	CROWBAR_ATTACK1HIT,
	CROWBAR_ATTACK1MISS,
	CROWBAR_ATTACK2MISS,
	CROWBAR_ATTACK2HIT,
	CROWBAR_ATTACK3MISS,
	CROWBAR_ATTACK3HIT
	};

void CCrowbar::Spawn ()
	{
	pev->classname = MAKE_STRING ("weapon_crowbar");	// ESHQ: ������ ��� ������� AOMDC
	Precache ();
	m_iId = WEAPON_CROWBAR;
	SET_MODEL (ENT (pev), "models/w_crowbar.mdl");
	m_iClip = -1;

	FallInit ();	// get ready to fall down
	}

void CCrowbar::Precache (void)
	{
	PRECACHE_MODEL ("models/v_crowbar.mdl");
	PRECACHE_MODEL ("models/w_crowbar.mdl");
	PRECACHE_MODEL ("models/p_crowbar.mdl");
	PRECACHE_SOUND ("weapons/cbar_hit1.wav");
	PRECACHE_SOUND ("weapons/cbar_hit2.wav");
	PRECACHE_SOUND ("weapons/cbar_hitbod1.wav");
	PRECACHE_SOUND ("weapons/cbar_hitbod2.wav");
	PRECACHE_SOUND ("weapons/cbar_hitbod3.wav");
	PRECACHE_SOUND ("weapons/cbar_miss1.wav");

	m_usCrowbar = PRECACHE_EVENT (1, "events/crowbar.sc");
	}

int CCrowbar::GetItemInfo (ItemInfo* p)
	{
	p->pszName = STRING (pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 0;
	p->iId = WEAPON_CROWBAR;
	p->iWeight = CROWBAR_WEIGHT;

	return 1;
	}

BOOL CCrowbar::Deploy ()
	{
	return DefaultDeploy ("models/v_crowbar.mdl", "models/p_crowbar.mdl", CROWBAR_DRAW, "crowbar");
	}

void CCrowbar::Holster (int skiplocal)
	{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase () + 0.5;
	SendWeaponAnim (CROWBAR_HOLSTER);
	}

void CCrowbar::PrimaryAttack ()
	{
	if (!Swing (1))
		{
		SetThink (&CCrowbar::SwingAgain);
		pev->nextthink = gpGlobals->time + 0.1;
		}
	}

void CCrowbar::Smack ()
	{
	DecalGunshot (&m_trHit, BULLET_PLAYER_CROWBAR);
	}

void CCrowbar::SwingAgain (void)
	{
	Swing (0);
	}

int CCrowbar::Swing (int fFirst)
	{
	int fDidHit = FALSE;

	TraceResult tr;

	UTIL_MakeVectors (m_pPlayer->pev->v_angle);
	Vector vecSrc = m_pPlayer->GetGunPosition ();
	Vector vecEnd = vecSrc + gpGlobals->v_forward * 32;

	UTIL_TraceLine (vecSrc, vecEnd, dont_ignore_monsters, ENT (m_pPlayer->pev), &tr);

#ifndef CLIENT_DLL
	if (tr.flFraction >= 1.0)
		{
		UTIL_TraceHull (vecSrc, vecEnd, dont_ignore_monsters, head_hull, ENT (m_pPlayer->pev), &tr);
		if (tr.flFraction < 1.0)
			{
			// Calculate the point of intersection of the line (or hull) and the object we hit
			// This is and approximation of the "best" intersection
			CBaseEntity* pHit = CBaseEntity::Instance (tr.pHit);
			if (!pHit || pHit->IsBSPModel ())
				FindHullIntersection (vecSrc, tr, VEC_DUCK_HULL_MIN, VEC_DUCK_HULL_MAX, m_pPlayer->edict ());
			vecEnd = tr.vecEndPos;	// This is the point on the actual surface (the hull could have hit space)
			}
		}
#endif

	PLAYBACK_EVENT_FULL (FEV_NOTHOST, m_pPlayer->edict (), m_usCrowbar,
		0.0, (float*)&g_vecZero, (float*)&g_vecZero, 0, 0, 0, 0.0, 0, 0.0);

	if (tr.flFraction >= 1.0)
		{
		if (fFirst)
			{
			// miss
			m_flNextPrimaryAttack = UTIL_WeaponTimeBase () + 0.5;

			// player "shoot" animation
			m_pPlayer->SetAnimation (PLAYER_ATTACK1);
			}
		}
	else
		{
		switch (((m_iSwing++) % 2) + 1)
			{
			case 0:
				SendWeaponAnim (CROWBAR_ATTACK1HIT); 
				break;
			case 1:
				SendWeaponAnim (CROWBAR_ATTACK2HIT); 
				break;
			case 2:
				SendWeaponAnim (CROWBAR_ATTACK3HIT); 
				break;
			}

		// player "shoot" animation
		m_pPlayer->SetAnimation (PLAYER_ATTACK1);

#ifndef CLIENT_DLL

		// hit
		fDidHit = TRUE;
		CBaseEntity* pEntity = CBaseEntity::Instance (tr.pHit);

		ClearMultiDamage ();

		if ((m_flNextPrimaryAttack + 1 < UTIL_WeaponTimeBase ()) || g_pGameRules->IsMultiplayer ())
			{
			// first swing does full damage
			pEntity->TraceAttack (m_pPlayer->pev, gSkillData.plrDmgCrowbar, gpGlobals->v_forward, 
				&tr, DMG_CLUB);
			}
		else
			{
			// subsequent swings do half
			pEntity->TraceAttack (m_pPlayer->pev, gSkillData.plrDmgCrowbar / 2, gpGlobals->v_forward, 
				&tr, DMG_CLUB);
			}
		ApplyMultiDamage (m_pPlayer->pev, m_pPlayer->pev);

		// play thwack, smack, or dong sound
		float flVol = 1.0;
		int fHitWorld = TRUE;

		if (pEntity)
			{
			if ((pEntity->Classify () != CLASS_NONE) && (pEntity->Classify () != CLASS_MACHINE))
				{
				// play thwack or smack sound
				switch (RANDOM_LONG (0, 2))
					{
					case 0:
						EMIT_SOUND (ENT (m_pPlayer->pev), CHAN_ITEM, "weapons/cbar_hitbod1.wav", 1, ATTN_MEDIUM);
						break;
					case 1:
						EMIT_SOUND (ENT (m_pPlayer->pev), CHAN_ITEM, "weapons/cbar_hitbod2.wav", 1, ATTN_MEDIUM);
						break;
					case 2:
						EMIT_SOUND (ENT (m_pPlayer->pev), CHAN_ITEM, "weapons/cbar_hitbod3.wav", 1, ATTN_MEDIUM);
						break;
					}

				m_pPlayer->m_iWeaponVolume = CROWBAR_BODYHIT_VOLUME;
				if (!pEntity->IsAlive ())
					return TRUE;
				else
					flVol = 0.1;

				fHitWorld = FALSE;
				}
			}

		// play texture hit sound
		// UNDONE: Calculate the correct point of intersection when we hit with the hull instead of the line

		if (fHitWorld)
			{
			float fvolbar = TEXTURETYPE_PlaySound (&tr, vecSrc, vecSrc + (vecEnd - vecSrc) * 2,
				BULLET_PLAYER_CROWBAR);

			if (g_pGameRules->IsMultiplayer ())
				{
				// override the volume here, cause we don't play texture sounds in multiplayer, 
				// and fvolbar is going to be 0 from the above call.

				fvolbar = 1;
				}

			// also play crowbar strike
			switch (RANDOM_LONG (0, 1))
				{
				case 0:
					EMIT_SOUND_DYN (ENT (m_pPlayer->pev), CHAN_ITEM, "weapons/cbar_hit1.wav", fvolbar, ATTN_MEDIUM, 0, 
						98 + RANDOM_LONG (0, 3));
					break;
				case 1:
					EMIT_SOUND_DYN (ENT (m_pPlayer->pev), CHAN_ITEM, "weapons/cbar_hit2.wav", fvolbar, ATTN_MEDIUM, 0, 
						98 + RANDOM_LONG (0, 3));
					break;
				}

			// delay the decal a bit
			m_trHit = tr;
			}

		m_pPlayer->m_iWeaponVolume = flVol * CROWBAR_WALLHIT_VOLUME;
#endif
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase () + 0.25;

		SetThink (&CCrowbar::Smack);
		pev->nextthink = UTIL_WeaponTimeBase () + 0.2;
		}

	return fDidHit;
	}

// ESHQ: ������� ��� ������
LINK_ENTITY_TO_CLASS (weapon_axe, CAxe);
	
enum axe_e
	{
	AXE_IDLE = 0,
	AXE_ATTACK,
	AXE_DRAW,
	AXE_HOLSTER
	};

void CAxe::Spawn ()
	{
	pev->classname = MAKE_STRING ("weapon_axe");
	Precache ();
	m_iId = WEAPON_AXE;
	SET_MODEL (ENT (pev), "models/w_axe.mdl");
	m_iClip = -1;

	FallInit ();	// get ready to fall down
	}

void CAxe::Precache (void)
	{
	PRECACHE_MODEL ("models/v_axe.mdl");
	PRECACHE_MODEL ("models/w_axe.mdl");
	PRECACHE_SOUND ("weapons/axe_hit.wav");
	PRECACHE_SOUND ("weapons/axe_swing.wav");

	m_usAxe = PRECACHE_EVENT (1, "events/axe.sc");
	}

int CAxe::GetItemInfo (ItemInfo *p)
	{
	p->pszName = STRING (pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 1;
	p->iId = WEAPON_AXE;
	p->iWeight = AXE_WEIGHT;

	return 1;
	}

BOOL CAxe::Deploy ()
	{
	return DefaultDeploy ("models/v_axe.mdl", "models/p_crowbar.mdl", AXE_DRAW, "axe");
	}

void CAxe::Holster (int skiplocal)
	{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase () + 0.5;
	SendWeaponAnim (AXE_HOLSTER);
	}

void CAxe::PrimaryAttack ()
	{
	if (CAxe::Swing ())
		return;
	}

void CAxe::Smack ()
	{
	DecalGunshot (&m_trHit, BULLET_PLAYER_CROWBAR);	// ESHQ: �������������
	}

int CAxe::Swing ()
	{
	int fDidHit = FALSE;
	TraceResult tr;

	UTIL_MakeVectors (m_pPlayer->pev->v_angle);
	Vector vecSrc = m_pPlayer->GetGunPosition ();
	Vector vecEnd = vecSrc + gpGlobals->v_forward * 32;

	UTIL_TraceLine (vecSrc, vecEnd, dont_ignore_monsters, ENT (m_pPlayer->pev), &tr);

#ifndef CLIENT_DLL

	if (tr.flFraction >= 1.0)
		{
		UTIL_TraceHull (vecSrc, vecEnd, dont_ignore_monsters, head_hull, ENT (m_pPlayer->pev), &tr);

		if (tr.flFraction < 1.0)
			{
			CBaseEntity *pHit = CBaseEntity::Instance (tr.pHit);
			if (!pHit || pHit->IsBSPModel ())
				FindHullIntersection (vecSrc, tr, Vector (-8, -8, -8), Vector (8, 8, 8),
					m_pPlayer->edict ());

			vecEnd = tr.vecEndPos;	// This is the point on the actual surface
			}
		}

#endif

	PLAYBACK_EVENT_FULL (FEV_NOTHOST, m_pPlayer->edict (), m_usAxe,
		0.0, (float *)&g_vecZero, (float *)&g_vecZero, 0, 0, 0, 0.0, 0, 0.0);

	if (tr.flFraction >= 1.0)
		{
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase () + 1.0;
		m_pPlayer->SetAnimation (PLAYER_ATTACK1);
		}
	else
		{
		SendWeaponAnim (AXE_ATTACK);
		m_pPlayer->SetAnimation (PLAYER_ATTACK1);

#ifndef CLIENT_DLL

		// hit
		fDidHit = TRUE;
		CBaseEntity *pEntity = CBaseEntity::Instance (tr.pHit);

		ClearMultiDamage ();

		// ESHQ: ������ ������ �����
		pEntity->TraceAttack (m_pPlayer->pev, gSkillData.plrDmgCrowbar * 10.0f, gpGlobals->v_forward,
			&tr, DMG_CLUB);
		ApplyMultiDamage (m_pPlayer->pev, m_pPlayer->pev);

		// play thwack, smack, or dong sound
		float flVol = 1.0;
		int fHitWorld = TRUE;

		if (pEntity)
			{
			if ((pEntity->Classify () != CLASS_NONE) && (pEntity->Classify () != CLASS_MACHINE))
				{
				m_pPlayer->m_iWeaponVolume = CROWBAR_BODYHIT_VOLUME;
				if (!pEntity->IsAlive ())
					return TRUE;
				else
					flVol = 0.1;

				fHitWorld = FALSE;
				}
			}

		if (fHitWorld)
			{
			float fvolbar = 1;
			if (!g_pGameRules->IsMultiplayer ())
				fvolbar = TEXTURETYPE_PlaySound (&tr, vecSrc, vecSrc + (vecEnd - vecSrc) * 2, BULLET_PLAYER_CROWBAR);

			EMIT_SOUND_DYN (ENT (m_pPlayer->pev), CHAN_ITEM, "weapons/axe_hit.wav", fvolbar, ATTN_MEDIUM,
				0, 95 + RANDOM_LONG (0, 10));

			// delay the decal a bit
			m_trHit = tr;
			}

		m_pPlayer->m_iWeaponVolume = flVol * CROWBAR_WALLHIT_VOLUME;

#endif

		m_flNextPrimaryAttack = UTIL_WeaponTimeBase () + 1.0;

		SetThink (&CAxe::Smack);
		pev->nextthink = UTIL_WeaponTimeBase () + 0.2;
		}

	return fDidHit;
	}
