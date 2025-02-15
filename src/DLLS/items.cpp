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
===== items.cpp ========================================================
functions governing the selection/use of weapons for players
***/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "weapons.h"
#include "player.h"
#include "skill.h"
#include "items.h"
#include "gamerules.h"

extern int gmsgItemPickup;

// ESHQ: ��������� ��������� ��� ���������� ��������
#define SEC_SET_TEXT_PARAMS		\
		textParams.a1 = textParams.a2 = 255;\
		textParams.g1 = textParams.b2 = 255;\
		textParams.b1 = textParams.g2 = 128;\
		\
		textParams.effect = 2;\
		textParams.fadeinTime = 0.04f;\
		textParams.fadeoutTime = 1.0f;\
		textParams.fxTime = 0.16f;\
		textParams.holdTime = 3.0f;\
		\
		textParams.x = -1.0f;\
		textParams.y = 0.15f;

class CWorldItem : public CBaseEntity
	{
	public:
		void	KeyValue (KeyValueData *pkvd);
		void	Spawn (void);
		int		m_iType;
	};

LINK_ENTITY_TO_CLASS (world_items, CWorldItem);

void CWorldItem::KeyValue (KeyValueData *pkvd)
	{
	if (FStrEq (pkvd->szKeyName, "type"))
		{
		m_iType = atoi (pkvd->szValue);
		pkvd->fHandled = TRUE;
		}
	else
		{
		CBaseEntity::KeyValue (pkvd);
		}
	}

void CWorldItem::Spawn (void)
	{
	CBaseEntity *pEntity = NULL;

	switch (m_iType)
		{
		case 44: // ITEM_BATTERY:
			pEntity = CBaseEntity::Create ("item_battery", pev->origin, pev->angles);
			break;
		case 42: // ITEM_ANTIDOTE:
			pEntity = CBaseEntity::Create ("item_antidote", pev->origin, pev->angles);
			break;
		case 43: // ITEM_SECURITY:
			pEntity = CBaseEntity::Create ("item_security", pev->origin, pev->angles);
			break;
		case 45: // ITEM_SUIT:
			pEntity = CBaseEntity::Create ("item_suit", pev->origin, pev->angles);
			break;
		}

	if (!pEntity)
		{
		ALERT (at_console, "unable to create world_item %d\n", m_iType);
		}
	else
		{
		pEntity->pev->target = pev->target;
		pEntity->pev->targetname = pev->targetname;
		pEntity->pev->spawnflags = pev->spawnflags;
		}

	REMOVE_ENTITY (edict ());
	}

// ESHQ: ����� ��������� ��������� �� titles.txt
char *CItem::DereferenceTextMessage (const char *MessageName)
	{
	// ���������� �������� �� �������� ����� ��������� client_textmessage_t
	char *msg0 = g_engfuncs.pfnTextMessageGet (MessageName);
	if (!msg0)
		return NULL;

	msg0 = msg0 + sizeof (int) + 8 * sizeof (byte) + 6 * sizeof (float) + sizeof (char *);

	// � ���������� ��������� �������� �����, � �� ������ - ��������� ���������������
	__int32 *msg1 = (__int32 *)msg0;
	msg0 = (char *)(msg1[0]);

	return msg0;
	}


// ESHQ: ��������� ������ � ������� ��� ���������� ����������
#define SF_KEY_AS_CARD		0x0002

#define SF_SIMPLE_DOCUMENT	0x0002
#define SF_THELASTONE		0x0004

void CItem::Spawn (void)
	{
	// ESHQ: ������ ����������� ������������ ��������� MOVETYPE_TOSS; ��������� ������
	pev->movetype = MOVETYPE_STEP;
	pev->solid = SOLID_TRIGGER;

	// ESHQ: ��������� ������� ����� ��������
	if (FStrEq (STRING (pev->classname), "item_key") && (pev->spawnflags & SF_KEY_AS_CARD))
		{
		pev->body = 1;
		}
	if (FStrEq (STRING (pev->classname), "item_security") && (pev->spawnflags & SF_SIMPLE_DOCUMENT))
		{
		pev->skin = RANDOM_LONG (1, 4);
		}

	UTIL_SetOrigin (pev, pev->origin);
	UTIL_SetSize (pev, Vector (-12, -12, 0), Vector (12, 12, 12));
	SetTouch (&CItem::ItemTouch);

	if (DROP_TO_FLOOR (ENT (pev)) == 0)
		{
		ALERT (at_error, "Item %s fell out of level at %f,%f,%f\n", STRING (pev->classname),
			pev->origin.x, pev->origin.y, pev->origin.z);
		UTIL_Remove (this);
		return;
		}
	}

extern int gEvilImpulse101;

void CItem::ItemTouch (CBaseEntity *pOther)
	{
	// if it's not a player, ignore
	if (!pOther->IsPlayer ())
		{
		return;
		}

	CBasePlayer *pPlayer = (CBasePlayer *)pOther;

	// ok, a player is touching this item, but can he have it?
	if (!g_pGameRules->CanHaveItem (pPlayer, this))
		{
		// no? Ignore the touch.
		return;
		}

	// ESHQ: ���������, ������ ���� �� ���������� ���� ��������������� ����� �������� (1)
	int mt = MyTouch (pPlayer);
	if (mt >= 0)
		{
		if (mt == 0)
			{
			SUB_UseTargets (pOther, USE_TOGGLE, 0);
			}
		SetTouch (NULL);

		// player grabbed the item
		g_pGameRules->PlayerGotItem (pPlayer, this);
		if (g_pGameRules->ItemShouldRespawn (this) == GR_ITEM_RESPAWN_YES)
			{
			Respawn ();
			}
		else
			{
			UTIL_Remove (this);
			}
		}
	else if (gEvilImpulse101)
		{
		UTIL_Remove (this);
		}
	}

CBaseEntity *CItem::Respawn (void)
	{
	SetTouch (NULL);
	pev->effects |= EF_NODRAW;

	UTIL_SetOrigin (pev, g_pGameRules->VecItemRespawnSpot (this));	// blip to whereever you should respawn

	SetThink (&CItem::Materialize);
	pev->nextthink = g_pGameRules->FlItemRespawnTime (this);
	return this;
	}

void CItem::Materialize (void)
	{
	if (pev->effects & EF_NODRAW)
		{
		// changing from invisible state to visible
		EMIT_SOUND_DYN (ENT (pev), CHAN_WEAPON, "items/suitchargeok1.wav", 1, ATTN_MEDIUM, 0, 150);
		pev->effects &= ~EF_NODRAW;
		pev->effects |= EF_MUZZLEFLASH;
		}

	SetTouch (&CItem::ItemTouch);
	}

// ESHQ: ��������� ���������� ���������� ��������
TYPEDESCRIPTION	CItem::m_SaveData[] =
	{
	DEFINE_FIELD (CItem, minimumToTrigger, FIELD_INTEGER),
	};

IMPLEMENT_SAVERESTORE (CItem, CBaseEntity);

void CItem::KeyValue (KeyValueData *pkvd)
	{
	if (FStrEq (pkvd->szKeyName, "MinimumToTrigger"))
		{
		if ((minimumToTrigger = atoi (pkvd->szValue)) < 0)
			minimumToTrigger = 0;

		pkvd->fHandled = TRUE;
		}
	else
		{
		CBaseEntity::KeyValue (pkvd);
		}
	}

#define SF_SUIT_SHORTLOGON		0x0001

class CItemSuit : public CItem
	{
	void Spawn (void)
		{
		Precache ();
		SET_MODEL (ENT (pev), "models/w_suit.mdl");
		CItem::Spawn ();
		}

	void Precache (void)
		{
		PRECACHE_MODEL ("models/w_suit.mdl");
		}

	int MyTouch (CBasePlayer *pPlayer)
		{
		if (pPlayer->pev->weapons & (1 << WEAPON_SUIT))
			return -1;

		if (pev->spawnflags & SF_SUIT_SHORTLOGON)
			EMIT_SOUND_SUIT (pPlayer->edict (), "!HEV_A0");		// short version of suit logon
		else
			EMIT_SOUND_SUIT (pPlayer->edict (), "!HEV_AAx");	// long version of suit logon

		pPlayer->pev->weapons |= (1 << WEAPON_SUIT);
		return 0;
		}
	};

LINK_ENTITY_TO_CLASS (item_suit, CItemSuit);
LINK_ENTITY_TO_CLASS (item_flashlight, CItemSuit);	// EHSQ: ��������� AOMDC

class CItemBattery : public CItem
	{
	void Spawn (void)
		{
		Precache ();

		// ESHQ: ����� ���� ��� ���
		SET_MODEL (ENT (pev), "models/mil_crate.mdl");
		switch (RANDOM_LONG (0, 2))
			{
			case 0:
				pev->body = 7;
				break;

			case 1:
				pev->body = 8;
				break;

			case 2:
				pev->body = 11;
				break;
			}
		CItem::Spawn ();
		}

	void Precache (void)
		{
		PRECACHE_MODEL ("models/mil_crate.mdl");
		PRECACHE_SOUND ("items/gunpickup2.wav");
		}

	int MyTouch (CBasePlayer *pPlayer)
		{
		if (pPlayer->pev->deadflag != DEAD_NO)
			return -1;

		if ((pPlayer->pev->armorvalue < MAX_NORMAL_BATTERY) &&
			(pPlayer->pev->weapons & (1 << WEAPON_SUIT)))
			{
			int pct;
			char szcharge[64];

			pPlayer->pev->armorvalue += gSkillData.batteryCapacity;
			pPlayer->pev->armorvalue = min (pPlayer->pev->armorvalue, MAX_NORMAL_BATTERY);

			EMIT_SOUND (pPlayer->edict (), CHAN_ITEM, "items/gunpickup2.wav", 1, ATTN_MEDIUM);

			MESSAGE_BEGIN (MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev);
			WRITE_STRING (STRING (pev->classname));
			MESSAGE_END ();

			// Suit reports new power level
			// For some reason this wasn't working in release build -- round it.
			pct = (int)((float)(pPlayer->pev->armorvalue * 100.0) * (1.0 / MAX_NORMAL_BATTERY) + 0.5);
			pct = (pct / 5);
			if (pct > 0)
				pct--;

			sprintf (szcharge, "!HEV_%1dP", pct);

			pPlayer->SetSuitUpdate (szcharge, FALSE, SUIT_NEXT_IN_30SEC);
			return 0;
			}

		return -1;
		}
	};

LINK_ENTITY_TO_CLASS (item_battery, CItemBattery);
LINK_ENTITY_TO_CLASS (item_helmet, CItemBattery);		// ESHQ: ��������� Blue shift
LINK_ENTITY_TO_CLASS (item_armorvest, CItemBattery);

class CItemAntidote : public CItem
	{
	// ESHQ: ��������� ���������
	struct hudtextparms_s textParams;
	char getMessage[128];

	void Spawn (void)
		{
		Precache ();
		SET_MODEL (ENT (pev), "models/w_antidote.mdl");
		CItem::Spawn ();
		}

	void Precache (void)
		{
		PRECACHE_MODEL ("models/w_antidote.mdl");
		PRECACHE_SOUND ("items/suitchargeok1.wav");
		}

	// ESHQ: ������ ������� ������������ ��� ���������� ������.
	// ������� 10 ��� - ����������, ��������� 4 ���� - ������� ���������� (������������ ������������
	// �������� ���������� � ����������� ����� �����������)
	int MyTouch (CBasePlayer *pPlayer)
		{
		EMIT_SOUND (pPlayer->edict (), CHAN_ITEM, "items/suitchargeok1.wav", 1, ATTN_MEDIUM);
		pPlayer->m_rgItems[ITEM_ANTIDOTE] += 1;

		// ��������� ����� ����������� ���������
		SEC_SET_TEXT_PARAMS;
		textParams.channel = 5;

		// ���������
		int lowestMinimumToTrigger = 91 * minimumToTrigger / 100;	// 91%
		int itemsCount = pPlayer->m_rgItems[ITEM_ANTIDOTE] & 0x03FF;

		// ���������
		if (minimumToTrigger == 0)
			{
			sprintf (getMessage, "%s: %2u", DereferenceTextMessage ("HCFOUND"), itemsCount);
			}

		// �������� ������ ������� ������ - ����� ������������
		else if ((lowestMinimumToTrigger != minimumToTrigger) &&
			(itemsCount < lowestMinimumToTrigger))
			{
			sprintf (getMessage, "%s: %u / %u\n%s", DereferenceTextMessage ("HDFOUND"),
				itemsCount, minimumToTrigger, DereferenceTextMessage ("HCFOUND1"));
			}

		// �������� ������ ������� ������ - ����� ����������, �� ��� �� �����
		else if (itemsCount < minimumToTrigger)
			{
			sprintf (getMessage, "%s: %u / %u\n%s", DereferenceTextMessage ("HDFOUND"),
				itemsCount, minimumToTrigger, DereferenceTextMessage ("HCFOUND2"));
			}

		// ������� ��� ���������� �� ������
		else
			{
			sprintf (getMessage, "%s: %u\n%s!", DereferenceTextMessage ("HCFOUND"), itemsCount,
				(pev->spawnflags & SF_THELASTONE) ? (DereferenceTextMessage ("HCFOUND4")) :
				(DereferenceTextMessage ("HCFOUND3")));

			// �����
			pPlayer->m_rgItems[ITEM_ANTIDOTE] += (1 << 10);
			WRITE_ACHIEVEMENTS_SCRIPT (0, (pPlayer->m_rgItems[ITEM_ANTIDOTE] & 0x3C00) >> 10);
			}
		UTIL_HudMessageAll (textParams, getMessage);

		if (itemsCount < lowestMinimumToTrigger)
			return 1;	// �������, �� ���� ����������� �� ������

		return 0;
		}
	};

LINK_ENTITY_TO_CLASS (item_antidote, CItemAntidote);

class CItemSecurity : public CItem
	{
	// ESHQ: ��������� ���������
	struct hudtextparms_s textParams;
	char getMessage[128];

	void Spawn (void)
		{
		Precache ();
		SET_MODEL (ENT (pev), "models/w_security.mdl");
		CItem::Spawn ();
		}

	void Precache (void)
		{
		PRECACHE_MODEL ("models/w_security.mdl");
		PRECACHE_SOUND ("debris/flesh1.wav");
		}

	// ESHQ: ��������� ���������� ��������
	int MyTouch (CBasePlayer *pPlayer)
		{
		EMIT_SOUND (pPlayer->edict (), CHAN_ITEM, "debris/flesh1.wav", 1, ATTN_MEDIUM);
		pPlayer->m_rgItems[ITEM_SECURITY] += 1;

		// ��������� ����� ����������� ���������
		SEC_SET_TEXT_PARAMS;
		textParams.channel = 6;

		// ���������
		int lowestMinimumToTrigger = 91 * minimumToTrigger / 100;	// 91%
		int itemsCount = pPlayer->m_rgItems[ITEM_SECURITY];

		// �������� �� ������ - ������� ����
		if (minimumToTrigger == 0)
			{
			sprintf (getMessage, "%s: %3u", DereferenceTextMessage ("HDFOUND"), itemsCount);
			}

		// �������� ������ ������� ������ - ����� ������������
		else if ((lowestMinimumToTrigger != minimumToTrigger) &&
			(itemsCount < lowestMinimumToTrigger))
			{
			sprintf (getMessage, "%s: %u / %u\n%s", DereferenceTextMessage ("HDFOUND"),
				itemsCount, minimumToTrigger, DereferenceTextMessage ("HCFOUND1"));
			}

		// �������� ������ ������� ������ - ����� ����������, �� ��� �� �����
		else if (itemsCount < minimumToTrigger)
			{
			sprintf (getMessage, "%s: %u / %u\n%s", DereferenceTextMessage ("HDFOUND"),
				itemsCount, minimumToTrigger, DereferenceTextMessage ("HCFOUND2"));
			}

		// ������� ��� ��������� �� ������
		else
			{
			sprintf (getMessage, "%s: %u\n%s!", DereferenceTextMessage ("HDFOUND"), itemsCount,
				(pev->spawnflags & SF_THELASTONE) ? (DereferenceTextMessage ("HCFOUND4")) :
				(DereferenceTextMessage ("HCFOUND3")));
			}
		UTIL_HudMessageAll (textParams, getMessage);

		if (itemsCount < lowestMinimumToTrigger)
			return 1;	// �������, �� ���� ����������� �� ������

		return 0;
		}
	};

LINK_ENTITY_TO_CLASS (item_security, CItemSecurity);

// ESHQ: ��������� ��������-�����
class CItemKey : public CItem
	{
	void Spawn (void)
		{
		Precache ();
		SET_MODEL (ENT (pev), "models/w_key.mdl");
		CItem::Spawn ();
		}

	void Precache (void)
		{
		PRECACHE_MODEL ("models/w_key.mdl");
		PRECACHE_SOUND ("items/9mmclip1.wav");
		}

	int MyTouch (CBasePlayer *pPlayer)
		{
		EMIT_SOUND (pPlayer->edict (), CHAN_ITEM, "items/9mmclip1.wav", 1, ATTN_MEDIUM);
		pPlayer->m_rgItems[ITEM_KEY] += 1;
		return 0;
		}
	};

LINK_ENTITY_TO_CLASS (item_key, CItemKey);

class CItemLongJump : public CItem
	{
	void Spawn (void)
		{
		Precache ();
		SET_MODEL (ENT (pev), "models/w_longjump.mdl");
		CItem::Spawn ();
		}

	void Precache (void)
		{
		PRECACHE_MODEL ("models/w_longjump.mdl");
		}

	int MyTouch (CBasePlayer *pPlayer)
		{
		if (pPlayer->m_fLongJump)
			return -1;

		if ((pPlayer->pev->weapons & (1 << WEAPON_SUIT)))
			{
			pPlayer->m_fLongJump = TRUE;	// player now has longjump module

			g_engfuncs.pfnSetPhysicsKeyValue (pPlayer->edict (), "slj", "1");

			MESSAGE_BEGIN (MSG_ONE, gmsgItemPickup, NULL, pPlayer->pev);
			WRITE_STRING (STRING (pev->classname));
			MESSAGE_END ();

			EMIT_SOUND_SUIT (pPlayer->edict (), "!HEV_A1");	// Play the longjump sound UNDONE: Kelly? correct sound?
			return 0;
			}

		return -1;
		}
	};

LINK_ENTITY_TO_CLASS (item_longjump, CItemLongJump);
