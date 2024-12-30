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

#ifndef FUNC_BREAK_H
#define FUNC_BREAK_H

// ESHQ: удалено, обрабатываетс€ на основе силы удара
/*typedef enum {
	expRandom = 1,
	expDirected = 0,
	} Explosions;*/
typedef enum {
	matGlass = 0,
	matWood,
	matMetal,
	matFlesh,
	matCinderBlock,
	matCeilingTile,
	matComputer,
	matUnbreakableGlass,
	matRocks,
	matFabric,	// ESHQ: ткань
	matNone,
	matLastMaterial,
	} Materials2;

#define	NUM_SHARDS 6	// this many shards spawned when breakable objects break

class CBreakable: public CBaseDelay
	{
	public:
		// basic functions
		void Spawn (void);
		void Precache (void);
		void KeyValue (KeyValueData* pkvd);
		void HLEXPORT BreakTouch (CBaseEntity* pOther);
		void Use (CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
		void DamageSound (void);

		// ESHQ: расчЄт громкости с помощью размера
		float GetVolume (void);
		int GetPitch (void);

		// breakables use an overridden takedamage
		virtual int TakeDamage (entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType);
		// To spark when hit
		void TraceAttack (entvars_t *pevAttacker, float flDamage, Vector vecDir, TraceResult *ptr, int bitsDamageType);

		BOOL IsBreakable (void);
		BOOL SparkWhenHit (void);

		int	 DamageDecal (int bitsDamageType);

		void EXPORT		Die ();
		virtual int		ObjectCaps (void) { return (CBaseEntity::ObjectCaps () & ~FCAP_ACROSS_TRANSITION); }
		virtual int		Save (CSave &save);
		virtual int		Restore (CRestore &restore);

		inline BOOL		Explodable (void) { return ExplosionMagnitude () > 0; }
		inline int		ExplosionMagnitude (void) { return pev->impulse; }
		inline void		ExplosionSetMagnitude (int magnitude) { pev->impulse = magnitude; }

		static void		MaterialSoundPrecache (Materials2 precacheMaterial);
		static void		MaterialSoundRandom (edict_t *pEdict, Materials2 soundMaterial, float volume);
		static const char	**MaterialSoundList (Materials2 precacheMaterial, int &soundCount, qboolean Precache);
		
		// ESHQ: общие вызовы между breakable, pushable и cycler
		static char		*PrecacheSounds (Materials2 material);
		static void		MakeDamageSound (Materials2 material, float volume, int pitch, edict_t *entity, qboolean breakable);
		static unsigned char	MakeBustSound (Materials2 material, float volume, int pitch, edict_t *entity);

		// ESHQ: нова€ схема загрузки звуков
		static const char *pSoundsFabric[];
		static const char *pSoundsBustFabric[];

		static const char *pSoundsWood[];
		static const char *pSoundsBustWood[];

		static const char *pSoundsFlesh[];
		static const char *pSoundsBustFlesh[];

		static const char *pSoundsGlass[];
		static const char *pSoundsBustGlass[];

		static const char *pSoundsMetal[];
		static const char *pSoundsBustMetal[];

		static const char *pSoundsConcrete[];
		static const char *pSoundsBustConcrete[];

		static const char *pSoundsBustCeiling[];

		static const char *pSoundsSparks[];

		static const char *pSpawnObjects[];

		static	TYPEDESCRIPTION m_SaveData[];

		Materials2	m_Material;
		/*Explosions	m_Explosion;*/
		int			m_idShard;
		float		m_angle;
		int			m_iszGibModel;
		int			m_iszSpawnObject;

		// ESHQ: переменна€ дл€ хранени€ значени€, определ€ющего громкость и высоту звуков
		float		m_sizeFactor;
	};

#endif
