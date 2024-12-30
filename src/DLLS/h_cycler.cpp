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
===== h_cycler.cpp ========================================================
  The Halflife Cycler Monsters
***/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "animation.h"
#include "weapons.h"
#include "player.h"
// ESHQ
#include "func_break.h"
#include "explode.h"

#define HC_CYCLER_PASSABLE	0x08
#define HC_CYCLER_BREAKABLE	0x10

#define TEMP_FOR_SCREEN_SHOTS
#ifdef TEMP_FOR_SCREEN_SHOTS

class CCycler : public CBaseMonster
	{
	public:
		virtual int	ObjectCaps (void) { return (CBaseEntity::ObjectCaps () | FCAP_IMPULSE_USE); }
		int TakeDamage (entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
		void Spawn (void);
		void Think (void);
		void Precache (void);

		// ESHQ: поддержка звуков ударов
		float GetVolume (void);
		int GetPitch (void);
		void CCycler::DamageSound ();

		void Use (CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

		// Don't treat as a live target
		virtual BOOL IsAlive (void) { return FALSE; }

		virtual int		Save (CSave &save);
		virtual int		Restore (CRestore &restore);
		static	TYPEDESCRIPTION m_SaveData[];

		// ESHQ: поддержка звуков ударов
		void KeyValue (KeyValueData *pkvd);

		int			m_animate;

		// ESHQ: поддержка разрушаемости
		Materials2	m_Material;
		int			m_magnitude;
		float		m_sizeFactor;
		int			m_idShard;
	};

// ESHQ: контроль корректности задания параметров
#define CHECK_CYCLER_SIZE(coord)	\
	if (fabs ((double)(pev->startpos.coord - pev->endpos.coord)) < 2.0)	\
		{	\
		pev->startpos.coord -= 1.0f;	\
		pev->endpos.coord += 1.0f;	\
		}	\
	else if (pev->startpos.coord > pev->endpos.coord)	\
		{	\
		vec_t coord = pev->startpos.coord;	\
		pev->startpos.coord = pev->endpos.coord;	\
		pev->endpos.coord = coord;	\
		}

TYPEDESCRIPTION	CCycler::m_SaveData[] =
	{
	DEFINE_FIELD (CCycler, m_animate, FIELD_INTEGER),
	DEFINE_FIELD (CCycler, m_Material, FIELD_INTEGER),
	DEFINE_FIELD (CCycler, m_magnitude, FIELD_INTEGER),
	DEFINE_FIELD (CCycler, m_sizeFactor, FIELD_FLOAT)
	};

IMPLEMENT_SAVERESTORE (CCycler, CBaseMonster);

// ESHQ: поддержка возможности задания физического размера модели и разрушаемости
void CCycler::KeyValue (KeyValueData *pkvd)
	{
	// Физические размеры
	if (FStrEq (pkvd->szKeyName, "MinPoint"))
		{
		Vector tmp;
		UTIL_StringToVector ((float *)tmp, pkvd->szValue);
		pev->startpos = tmp;
		}
	else if (FStrEq (pkvd->szKeyName, "MaxPoint"))
		{
		Vector tmp;
		UTIL_StringToVector ((float *)tmp, pkvd->szValue);
		pev->endpos = tmp;
		}

	// Материал для звука и обломков
	else if (FStrEq (pkvd->szKeyName, "material"))
		{
		int i = atoi (pkvd->szValue);

		if ((i < 0) || (i >= matLastMaterial))
			m_Material = matMetal;
		else
			m_Material = (Materials2)i;

		pkvd->fHandled = TRUE;
		}

	// Разрушаемость
	else if (FStrEq (pkvd->szKeyName, "explodemagnitude"))
		{
		pev->impulse = atoi (pkvd->szValue);
		pkvd->fHandled = TRUE;
		}

	else
		{
		CBaseMonster::KeyValue (pkvd);
		}
	}

LINK_ENTITY_TO_CLASS (cycler, CCycler);
LINK_ENTITY_TO_CLASS (env_model, CCycler);	// ESHQ: совместимость с AOMDC

// ESHQ: удалена поддержка CCyclerSprite, соответствующие сущности переназначены
LINK_ENTITY_TO_CLASS (cycler_sprite, CCycler);
LINK_ENTITY_TO_CLASS (cycler_weapon, CCycler);

void CCycler::Precache (void) 
	{
	const char *pGibName;
	pGibName = CBreakable::PrecacheSounds (m_Material);

	// ESHQ: обработка звуков и моделей разрушения
	/*switch (m_Material)
		{
		case matWood:
			pGibName = "models/woodgibs.mdl";
			PRECACHE_SOUND ("debris/bustcrate1.wav");
			PRECACHE_SOUND ("debris/bustcrate2.wav");
			PRECACHE_SOUND ("debris/bustcrate3.wav");
			break;

		case matFlesh:
			pGibName = "models/fleshgibs.mdl";
			PRECACHE_SOUND ("debris/bustflesh1.wav");
			PRECACHE_SOUND ("debris/bustflesh2.wav");
			break;

		case matComputer:
			PRECACHE_SOUND ("buttons/spark5.wav");
			PRECACHE_SOUND ("buttons/spark6.wav");
			pGibName = "models/computergibs.mdl";
			PRECACHE_SOUND ("debris/bustmetal1.wav");
			PRECACHE_SOUND ("debris/bustmetal2.wav");
			break;

		case matUnbreakableGlass:
		case matGlass:
			pGibName = "models/glassgibs.mdl";
			PRECACHE_SOUND ("debris/bustglass1.wav");
			PRECACHE_SOUND ("debris/bustglass2.wav");
			PRECACHE_SOUND ("debris/bustglass3.wav");
			break;

		case matMetal:
		default:
			pGibName = "models/metalplategibs.mdl";
			PRECACHE_SOUND ("debris/bustmetal1.wav");
			PRECACHE_SOUND ("debris/bustmetal2.wav");
			break;

		case matCinderBlock:
			pGibName = "models/cindergibs.mdl";
			PRECACHE_SOUND ("debris/bustconcrete1.wav");
			PRECACHE_SOUND ("debris/bustconcrete2.wav");
			break;

		case matRocks:
			pGibName = "models/rockgibs.mdl";
			PRECACHE_SOUND ("debris/bustconcrete1.wav");
			PRECACHE_SOUND ("debris/bustconcrete2.wav");
			break;

		case matCeilingTile:
			pGibName = "models/ceilinggibs.mdl";
			PRECACHE_SOUND ("debris/bustceiling1.wav");
			PRECACHE_SOUND ("debris/bustceiling2.wav");
			break;
		}*/

	m_idShard = PRECACHE_MODEL ((char *)pGibName);
	}

// Cycler member functions
void CCycler::Spawn ()
	{
	const char *szModel = (char *)STRING (pev->model);

	if (!szModel || !*szModel)
		{
		ALERT (at_error, "cycler at %.0f %.0f %0.f missing modelname", pev->origin.x, pev->origin.y, pev->origin.z);
		REMOVE_ENTITY (ENT (pev));
		return;
		}

	pev->classname = MAKE_STRING ("cycler");

	m_magnitude = pev->impulse;
	if (pev->health == 0.0f)
		pev->health = 100;

	PRECACHE_MODEL (szModel);
	SET_MODEL (ENT (pev), szModel);

	CCycler::Precache ();

	// Создание объекта
	InitBoneControllers ();
	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_NONE;
	pev->takedamage = DAMAGE_YES;
	pev->effects = 0;
	pev->yaw_speed = 5;
	pev->ideal_yaw = pev->angles.y;
	ChangeYaw (360);
	m_flFrameRate = 75;
	m_flGroundSpeed = 0;
	m_bloodColor = DONT_BLEED;	// ESHQ: странно, когда, например, деревья брызгают жёлтой кровью

	pev->nextthink += 1.0;

	ResetSequenceInfo ();

	if (pev->frame != 0)	// ESHQ: непонятно, зачем было отключать анимацию на ненулевых секъюэнсах
		{
		m_animate = 0;
		pev->framerate = 0;
		}
	else
		{
		m_animate = 1;
		}

	// Контроль размеров
	if (!FBitSet (pev->spawnflags, HC_CYCLER_PASSABLE))
		{
		CHECK_CYCLER_SIZE (x);
		CHECK_CYCLER_SIZE (y);
		CHECK_CYCLER_SIZE (z);

		UTIL_SetSize (pev, pev->startpos, pev->endpos);

		// ESHQ: расчёт фактора размера для звуков
		m_sizeFactor = (pev->size.x * pev->size.x + pev->size.y * pev->size.y +
			pev->size.z * pev->size.z) / 6144.0f;	// Коробка 32 х 32 х 32 с двойным запасом
		if (m_sizeFactor > 1.0f)
			m_sizeFactor = 1.0f;
		}
	else
		{
		m_sizeFactor = 0.75f;
		}
	}

// cycler think
void CCycler::Think (void)
	{
	pev->nextthink = gpGlobals->time + 0.1;

	if (m_animate)
		StudioFrameAdvance ();

	if (m_fSequenceFinished && !m_fSequenceLoops)
		{
		// hack to avoid reloading model every frame
		pev->animtime = gpGlobals->time;
		pev->framerate = 1.0;
		m_fSequenceFinished = FALSE;
		m_flLastEventCheck = gpGlobals->time;
		pev->frame = 0;

		if (!m_animate)
			pev->framerate = 0.0;	// FIX: don't reset framerate
		}
	}

// CyclerUse - starts a rotation trend
void CCycler::Use (CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
	{
	m_animate = !m_animate;
	if (m_animate)
		pev->framerate = 1.0;
	else
		pev->framerate = 0.0;

	// Подмена звука
	if (FStringNull (pev->target))
		EMIT_SOUND (ENT (pev), CHAN_ITEM, "common/wpn_denyselect.wav", 0.4, ATTN_MEDIUM);

	// ESHQ: активация привязанной цели
	else
		SUB_UseTargets (NULL, USE_TOGGLE, 0);
	}

// ESHQ: обработка звукового сопровождения
// Почти полная копия из CBreakable
float CCycler::GetVolume (void)
	{
	return RANDOM_FLOAT (0.20, 0.35) + 0.6 * m_sizeFactor;
	}

int CCycler::GetPitch (void)
	{
	return RANDOM_LONG (130, 145) - (int)(45 * m_sizeFactor);
	}

void CCycler::DamageSound ()
	{
	CBreakable::MakeDamageSound (m_Material, GetVolume (), GetPitch (), ENT (pev),
		FBitSet (pev->spawnflags, HC_CYCLER_BREAKABLE));

	/*int pitch;
	float fvol;
	char *rgpsz[6];
	int i;
	int material = m_Material;

	// Отмена звука, если cycler нематериальный
	if (FBitSet (pev->spawnflags, HC_CYCLER_PASSABLE))
		return;

	// Настройка звука
	fvol = GetVolume ();
	pitch = GetPitch ();

	if (material == matComputer && RANDOM_LONG (0, 1))
		material = matMetal;

	switch (material)
		{
		case matComputer:
		case matGlass:
		case matUnbreakableGlass:
			rgpsz[0] = "debris/glass1.wav";
			rgpsz[1] = "debris/glass2.wav";
			rgpsz[2] = "debris/glass3.wav";
			i = 3;
			break;

		case matWood:
			rgpsz[0] = "debris/wood5.wav";
			rgpsz[1] = "debris/wood6.wav";
			rgpsz[2] = "debris/wood7.wav";
			i = 3;
			break;

		case matMetal:
			rgpsz[0] = "player/pl_metal5.wav";
			rgpsz[1] = "player/pl_metal6.wav";
			rgpsz[2] = "player/pl_metal7.wav";
			rgpsz[3] = "player/pl_metal8.wav";
			i = 4;
			break;

		case matFlesh:
			rgpsz[0] = "debris/flesh2.wav";
			rgpsz[1] = "debris/flesh3.wav";
			rgpsz[2] = "debris/flesh4.wav";
			rgpsz[3] = "debris/flesh5.wav";
			rgpsz[4] = "debris/flesh6.wav";
			rgpsz[5] = "debris/flesh7.wav";
			i = 6;
			break;

		case matRocks:
		case matCinderBlock:
			rgpsz[0] = "debris/concrete1.wav";
			rgpsz[1] = "debris/concrete2.wav";
			rgpsz[2] = "debris/concrete3.wav";
			i = 3;
			break;

		case matCeilingTile:
			i = 0;
			break;
		}

	if (i)
		EMIT_SOUND_DYN (ENT (pev), CHAN_VOICE, rgpsz[RANDOM_LONG (0, i - 1)], fvol, ATTN_MEDIUM, 0, pitch);*/
	}

// ESHQ: обработка получения урона
int CCycler::TakeDamage (entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType)
	{
	Vector vecSpot;		// shard origin
	Vector vecVelocity;	// shard velocity
	CBaseEntity *pEntity = NULL;
	char cFlag = 0;
	/*int pitch;
	float fvol;*/
	float ampl, freq, duration;

	// Защита от посторонних обработок
	if (pev->health - flDamage > 0.0f)
		{
		if (m_animate)
			{
			ResetSequenceInfo ();
			pev->frame = 0;
			}
		else
			{
			pev->framerate = 1.0;
			StudioFrameAdvance (0.1);
			pev->framerate = 0;
			/*ALERT (at_console, "sequence: %d, frame %.0f\n", pev->sequence, pev->frame);*/
			}
		}

	// Защита от двойного входа
	else if (pev->health <= 0.0f)
		{
		return 0;
		}

	// Звук материала
	DamageSound ();

	if (!FBitSet (pev->spawnflags, HC_CYCLER_BREAKABLE))
		return 1;

	// Разрушение
	pev->health -= flDamage;
	if (pev->health > 0.0f)
		return 1;

	pev->takedamage = DAMAGE_NO;
	pev->deadflag = DEAD_KILLED;
	UTIL_Remove (this);

	cFlag = CBreakable::MakeBustSound (m_Material, GetVolume (), GetPitch (), ENT (pev));

	/*// ESHQ: громкость и высота теперь зависят от размера объекта
	fvol = GetVolume ();
	pitch = GetPitch ();

	switch (m_Material)
		{
		case matGlass:
			switch (RANDOM_LONG (0, 2))
				{
				case 0:
					EMIT_SOUND_DYN (ENT (pev), CHAN_VOICE, "debris/bustglass1.wav", fvol, ATTN_MEDIUM, 0, pitch);
					break;
				case 1:
					EMIT_SOUND_DYN (ENT (pev), CHAN_VOICE, "debris/bustglass2.wav", fvol, ATTN_MEDIUM, 0, pitch);
					break;
				case 2:
					EMIT_SOUND_DYN (ENT (pev), CHAN_VOICE, "debris/bustglass3.wav", fvol, ATTN_MEDIUM, 0, pitch);
					break;
				}
			cFlag = BREAK_GLASS;
			break;

		case matWood:
			switch (RANDOM_LONG (0, 2))
				{
				case 0:
					EMIT_SOUND_DYN (ENT (pev), CHAN_VOICE, "debris/bustcrate1.wav", fvol, ATTN_MEDIUM, 0, pitch);
					break;
				case 1:
					EMIT_SOUND_DYN (ENT (pev), CHAN_VOICE, "debris/bustcrate2.wav", fvol, ATTN_MEDIUM, 0, pitch);
					break;
				case 2:
					EMIT_SOUND_DYN (ENT (pev), CHAN_VOICE, "debris/bustcrate3.wav", fvol, ATTN_MEDIUM, 0, pitch);
					break;
				}
			cFlag = BREAK_WOOD;
			break;

		case matComputer:
		case matMetal:
			switch (RANDOM_LONG (0, 1))
				{
				case 0:
					EMIT_SOUND_DYN (ENT (pev), CHAN_VOICE, "debris/bustmetal1.wav", fvol, ATTN_MEDIUM, 0, pitch);
					break;
				case 1:
					EMIT_SOUND_DYN (ENT (pev), CHAN_VOICE, "debris/bustmetal2.wav", fvol, ATTN_MEDIUM, 0, pitch);
					break;
				}
			cFlag = BREAK_METAL;
			break;

		case matFlesh:
			switch (RANDOM_LONG (0, 1))
				{
				case 0:
					EMIT_SOUND_DYN (ENT (pev), CHAN_VOICE, "debris/bustflesh1.wav", fvol, ATTN_MEDIUM, 0, pitch);
					break;
				case 1:
					EMIT_SOUND_DYN (ENT (pev), CHAN_VOICE, "debris/bustflesh2.wav", fvol, ATTN_MEDIUM, 0, pitch);
					break;
				}
			cFlag = BREAK_FLESH;
			break;

		case matRocks:
		case matCinderBlock:
			switch (RANDOM_LONG (0, 1))
				{
				case 0:
					EMIT_SOUND_DYN (ENT (pev), CHAN_VOICE, "debris/bustconcrete1.wav", fvol, ATTN_MEDIUM, 0, pitch);
					break;
				case 1:
					EMIT_SOUND_DYN (ENT (pev), CHAN_VOICE, "debris/bustconcrete2.wav", fvol, ATTN_MEDIUM, 0, pitch);
					break;
				}
			cFlag = BREAK_CONCRETE;
			break;

		case matCeilingTile:
			switch (RANDOM_LONG (0, 1))
				{
				case 0:
					EMIT_SOUND_DYN (ENT (pev), CHAN_VOICE, "debris/bustceiling1.wav", fvol, ATTN_MEDIUM, 0, pitch);
					break;
				case 1:
					EMIT_SOUND_DYN (ENT (pev), CHAN_VOICE, "debris/bustceiling2.wav", fvol, ATTN_MEDIUM, 0, pitch);
					break;
				}
			break;
		}*/

	/*// Направление
	vecVelocity.x = 0;
	vecVelocity.y = 0;
	vecVelocity.z = 0;*/
	// Разброс обломков (armortype не равен нулю только при ударах по объекту оружием)
	ampl = flDamage * 15.0f;
	if (ampl > 400.0f)
		ampl = 400.0f;

	// Определение вектора атаки
	Vector vecTemp;
	if (pevAttacker == pevInflictor)
		{
		vecTemp = pevInflictor->origin - (pev->absmin + (pev->size * 0.5));

		if (FBitSet (pevAttacker->flags, FL_CLIENT) &&
			FBitSet (pev->spawnflags, SF_BREAK_CROWBAR) && (bitsDamageType & DMG_CLUB))
			flDamage = pev->health;
		}
	else
		{
		vecTemp = pevInflictor->origin - (pev->absmin + (pev->size * 0.5));
		}

	// Запуск на сервер
	vecVelocity = vecTemp.Normalize () * -ampl;
	vecSpot = pev->origin + (pev->mins + pev->maxs) * 0.5;

	MESSAGE_BEGIN (MSG_PVS, SVC_TEMPENTITY, vecSpot);
	WRITE_BYTE (TE_BREAKMODEL);

	WRITE_COORD (vecSpot.x);
	WRITE_COORD (vecSpot.y);
	WRITE_COORD (vecSpot.z);
	WRITE_COORD (pev->size.x);
	WRITE_COORD (pev->size.y);
	WRITE_COORD (pev->size.z);
	WRITE_COORD (vecVelocity.x);
	WRITE_COORD (vecVelocity.y);
	WRITE_COORD (vecVelocity.z);
	WRITE_BYTE (10);	// RND
	WRITE_SHORT (m_idShard);
	WRITE_BYTE (0);		// Количество
	WRITE_BYTE (50);	// ESHQ: увеличено до 5 секунд
	WRITE_BYTE (cFlag);
	MESSAGE_END ();

	// Падение всего, что стояло на уничтоженном объекте
	Vector mins = pev->absmin;
	Vector maxs = pev->absmax;
	mins.z = pev->absmax.z;
	maxs.z += 8;

	CBaseEntity *pList[256];
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

	SetThink (&CBaseEntity::SUB_Remove);
	pev->nextthink = pev->ltime + 0.1;

	if (m_magnitude)
		{
		ExplosionCreate (Center (), pev->angles, edict (), m_magnitude, TRUE);

		// ESHQ: добавление дрожи к эффекту взрыва
		ampl = m_magnitude / 20.0f;
		if (ampl > 16.0f)
			ampl = 16.0f;

		freq = m_magnitude / 10.0f;
		if (freq > 60.0f)
			freq = 60.0f;

		duration = m_magnitude / 75.0f;
		if (duration > 4.0f)
			duration = 4.0f;

		UTIL_ScreenShake (VecBModelOrigin (pev), ampl, freq, duration, m_magnitude * 3.0f);
		}

	return 0;
	}

#endif

// Flaming wreckage
class CWreckage : public CBaseMonster
	{
	int		Save (CSave &save);
	int		Restore (CRestore &restore);
	static	TYPEDESCRIPTION m_SaveData[];

	void Spawn (void);
	void Precache (void);
	void Think (void);

	int m_flStartTime;
	};

TYPEDESCRIPTION	CWreckage::m_SaveData[] =
	{
		DEFINE_FIELD (CWreckage, m_flStartTime, FIELD_TIME),
	};

IMPLEMENT_SAVERESTORE (CWreckage, CBaseMonster);
LINK_ENTITY_TO_CLASS (cycler_wreckage, CWreckage);

void CWreckage::Spawn (void)
	{
	pev->solid = SOLID_NOT;
	pev->movetype = MOVETYPE_NONE;
	pev->takedamage = 0;
	pev->effects = 0;

	pev->frame = 0;
	pev->nextthink = gpGlobals->time + 0.1;

	if (pev->model)
		{
		PRECACHE_MODEL ((char *)STRING (pev->model));
		SET_MODEL (ENT (pev), STRING (pev->model));
		}

	m_flStartTime = gpGlobals->time;
	}

void CWreckage::Precache ()
	{
	if (pev->model)
		PRECACHE_MODEL ((char *)STRING (pev->model));
	}

void CWreckage::Think (void)
	{
	StudioFrameAdvance ();
	pev->nextthink = gpGlobals->time + 0.2;

	if (pev->dmgtime)
		{
		if (pev->dmgtime < gpGlobals->time)
			{
			UTIL_Remove (this);
			return;
			}
		else if (RANDOM_FLOAT (0, pev->dmgtime - m_flStartTime) > pev->dmgtime - gpGlobals->time)
			{
			return;
			}
		}

	Vector VecSrc;

	VecSrc.x = RANDOM_FLOAT (pev->absmin.x, pev->absmax.x);
	VecSrc.y = RANDOM_FLOAT (pev->absmin.y, pev->absmax.y);
	VecSrc.z = RANDOM_FLOAT (pev->absmin.z, pev->absmax.z);

	MESSAGE_BEGIN (MSG_PVS, SVC_TEMPENTITY, VecSrc);
	WRITE_BYTE (TE_SMOKE);
	WRITE_COORD (VecSrc.x);
	WRITE_COORD (VecSrc.y);
	WRITE_COORD (VecSrc.z);
	WRITE_SHORT (g_sModelIndexSmoke);
	WRITE_BYTE (RANDOM_LONG (0, 49) + 50); // scale * 10
	WRITE_BYTE (RANDOM_LONG (0, 3) + 8); // framerate
	MESSAGE_END ();
	}
