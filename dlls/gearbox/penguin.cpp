/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
#if !defined( OEM_BUILD ) && !defined( HLDEMO_BUILD )

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"
#include "soundent.h"
#include "gamerules.h"

enum w_penguin_e
{
	WPENGUIN_IDLE1 = 0,
	WPENGUIN_FIDGET,
	WPENGUIN_JUMP,
	WPENGUIN_RUN
};

enum penguin_e
{
	PENGUIN_IDLE1 = 0,
	PENGUIN_FIDGETFIT,
	PENGUIN_FIDGETNIP,
	PENGUIN_DOWN,
	PENGUIN_UP,
	PENGUIN_THROW
};

#ifndef CLIENT_DLL
class CPenguinGrenade : public CGrenade
{
	void Spawn( void );
	void Precache( void );
	int Classify( void );
	void EXPORT SuperBounceTouch( CBaseEntity *pOther );
	void EXPORT HuntThink( void );
	int BloodColor( void ) { return BLOOD_COLOR_RED; }
	void Killed( entvars_t *pevAttacker, int iGib );
	void GibMonster( void );

	virtual int Save( CSave &save ); 
	virtual int Restore( CRestore &restore );

	static TYPEDESCRIPTION m_SaveData[];

	virtual int SizeForGrapple() { return GRAPPLE_SMALL; }

	static float m_flNextBounceSoundTime;

	// CBaseEntity *m_pTarget;
	float m_flDie;
	Vector m_vecTarget;
	float m_flNextHunt;
	float m_flNextHit;
	Vector m_posPrev;
	EHANDLE m_hOwner;
	int m_iMyClass;
};

float CPenguinGrenade::m_flNextBounceSoundTime = 0;

LINK_ENTITY_TO_CLASS( monster_penguin, CPenguinGrenade )

TYPEDESCRIPTION	CPenguinGrenade::m_SaveData[] =
{
	DEFINE_FIELD( CPenguinGrenade, m_flDie, FIELD_TIME ),
	DEFINE_FIELD( CPenguinGrenade, m_vecTarget, FIELD_VECTOR ),
	DEFINE_FIELD( CPenguinGrenade, m_flNextHunt, FIELD_TIME ),
	DEFINE_FIELD( CPenguinGrenade, m_flNextHit, FIELD_TIME ),
	DEFINE_FIELD( CPenguinGrenade, m_posPrev, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( CPenguinGrenade, m_hOwner, FIELD_EHANDLE ),
};

IMPLEMENT_SAVERESTORE( CPenguinGrenade, CGrenade )

#define PENGUIN_DETONATE_DELAY	15.0

int CPenguinGrenade::Classify( void )
{
	if( m_iMyClass != 0 )
		return m_iMyClass; // protect against recursion

	if( m_hEnemy != 0 )
	{
		m_iMyClass = CLASS_INSECT; // no one cares about it
		switch( m_hEnemy->Classify() )
		{
			case CLASS_PLAYER:
			case CLASS_HUMAN_PASSIVE:
			case CLASS_HUMAN_MILITARY:
				m_iMyClass = 0;
				return CLASS_ALIEN_MILITARY; // barney's get mad, grunts get mad at it
		}
		m_iMyClass = 0;
	}

	return CLASS_ALIEN_BIOWEAPON;
}

void CPenguinGrenade::Spawn( void )
{
	Precache();

	// motor
	pev->movetype = MOVETYPE_BOUNCE;
	pev->solid = SOLID_BBOX;

	SET_MODEL( ENT( pev ), "models/w_penguin.mdl" );
	UTIL_SetSize( pev, Vector( -4, -4, 0 ), Vector( 4, 4, 8 ) );
	UTIL_SetOrigin( pev, pev->origin );

	SetTouch( &CPenguinGrenade::SuperBounceTouch );
	SetThink( &CPenguinGrenade::HuntThink );
	pev->nextthink = gpGlobals->time + 0.1;
	m_flNextHunt = gpGlobals->time + 1E6;

	pev->flags |= FL_MONSTER;
	pev->takedamage = DAMAGE_AIM;
	pev->health = gSkillData.snarkHealth;
	pev->gravity = 0.5;
	pev->friction = 0.5;

	pev->dmg = gSkillData.plrDmgHandGrenade;

	m_flDie = gpGlobals->time + PENGUIN_DETONATE_DELAY;

	m_flFieldOfView = 0; // 180 degrees

	if( pev->owner )
		m_hOwner = Instance( pev->owner );

	m_flNextBounceSoundTime = gpGlobals->time;// reset each time a penguin is spawned.

	pev->sequence = WPENGUIN_RUN;
	ResetSequenceInfo();
}

void CPenguinGrenade::Precache( void )
{
	PRECACHE_MODEL( "models/w_penguin.mdl" );
	PRECACHE_SOUND( "squeek/sqk_blast1.wav" );
	PRECACHE_SOUND( "common/bodysplat.wav" );
	PRECACHE_SOUND( "penguin/penguin_die1.wav" );
	PRECACHE_SOUND( "penguin/penguin_hunt1.wav" );
	PRECACHE_SOUND( "penguin/penguin_hunt2.wav" );
	PRECACHE_SOUND( "penguin/penguin_hunt3.wav" );
	PRECACHE_SOUND( "penguin/penguin_deploy1.wav" );
}

void CPenguinGrenade::Killed( entvars_t *pevAttacker, int iGib )
{
	if( m_hOwner != 0 )
		pev->owner = m_hOwner->edict();

	CGrenade::Detonate();

	UTIL_BloodDrips( pev->origin, g_vecZero, BloodColor(), 80 );

	if( m_hOwner != 0 )
		pev->owner = m_hOwner->edict();
}

void CPenguinGrenade::GibMonster( void )
{
	EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "common/bodysplat.wav", 0.75, ATTN_NORM, 0, 200 );
}

void CPenguinGrenade::HuntThink( void )
{
	// ALERT( at_console, "think\n" );

	if( !IsInWorld() )
	{
		SetTouch( NULL );
		UTIL_Remove( this );
		return;
	}

	StudioFrameAdvance();
	pev->nextthink = gpGlobals->time + 0.1;

	// explode when ready
	if( gpGlobals->time >= m_flDie )
	{
		g_vecAttackDir = pev->velocity.Normalize();
		pev->health = -1;
		Killed( pev, 0 );
		return;
	}

	// float
	if( pev->waterlevel != 0 )
	{
		if( pev->movetype == MOVETYPE_BOUNCE )
		{
			pev->movetype = MOVETYPE_FLY;
		}
		pev->velocity = pev->velocity * 0.9;
		pev->velocity.z += 8.0;
	}
	else if( pev->movetype == MOVETYPE_FLY )
	{
		pev->movetype = MOVETYPE_BOUNCE;
	}

	// return if not time to hunt
	if( m_flNextHunt > gpGlobals->time )
		return;

	m_flNextHunt = gpGlobals->time + 2.0;

	//CBaseEntity *pOther = NULL;
	Vector vecDir;
	TraceResult tr;

	Vector vecFlat = pev->velocity;
	vecFlat.z = 0;
	vecFlat = vecFlat.Normalize();

	UTIL_MakeVectors( pev->angles );

	if( m_hEnemy == 0 || !m_hEnemy->IsAlive() )
	{
		// find target, bounce a bit towards it.
		Look( 512 );
		m_hEnemy = BestVisibleEnemy();
	}

	// squeek if it's about time blow up
	if( ( m_flDie - gpGlobals->time <= 0.5 ) && ( m_flDie - gpGlobals->time >= 0.3 ) )
	{
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "penguin/penguin_die1.wav", 1, ATTN_NORM, 0, 100 + RANDOM_LONG( 0, 0x3F ) );
		CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 256, 0.25 );
	}

	// higher pitch as squeeker gets closer to detonation time
	float flpitch = 155.0 - 60.0 * ( ( m_flDie - gpGlobals->time ) / PENGUIN_DETONATE_DELAY );
	if( flpitch < 80 )
		flpitch = 80;

	if( m_hEnemy != 0 )
	{
		if( FVisible( m_hEnemy ) )
		{
			vecDir = m_hEnemy->EyePosition() - pev->origin;
			m_vecTarget = vecDir.Normalize();
		}

		float flVel = pev->velocity.Length();
		float flAdj = 50.0 / ( flVel + 10.0 );

		if( flAdj > 1.2 )
			flAdj = 1.2;
		
		// ALERT( at_console, "think : enemy\n");

		// ALERT( at_console, "%.0f %.2f %.2f %.2f\n", flVel, m_vecTarget.x, m_vecTarget.y, m_vecTarget.z );

		pev->velocity = pev->velocity * flAdj + m_vecTarget * 300;
	}

	if( pev->flags & FL_ONGROUND )
	{
		pev->avelocity = Vector( 0, 0, 0 );
	}
	else
	{
		if( pev->avelocity == Vector( 0, 0, 0 ) )
		{
			pev->avelocity.x = RANDOM_FLOAT( -100, 100 );
			pev->avelocity.z = RANDOM_FLOAT( -100, 100 );
		}
	}

	if( ( pev->origin - m_posPrev ).Length() < 1.0 )
	{
		pev->velocity.x = RANDOM_FLOAT( -100, 100 );
		pev->velocity.y = RANDOM_FLOAT( -100, 100 );
	}
	m_posPrev = pev->origin;

	pev->angles = UTIL_VecToAngles( pev->velocity );
	pev->angles.z = 0;
	pev->angles.x = 0;
}

void CPenguinGrenade::SuperBounceTouch( CBaseEntity *pOther )
{
	float flpitch;

	TraceResult tr = UTIL_GetGlobalTrace();

	// don't hit the guy that launched this grenade
	if( pev->owner && pOther->edict() == pev->owner )
		return;

	// at least until we've bounced once
	pev->owner = NULL;

	pev->angles.x = 0;
	pev->angles.z = 0;

	// avoid bouncing too much
	if( m_flNextHit > gpGlobals->time )
		return;

	// higher pitch as squeeker gets closer to detonation time
	flpitch = 155.0 - 60.0 * ( ( m_flDie - gpGlobals->time ) / PENGUIN_DETONATE_DELAY );

	if( pOther->pev->takedamage && m_flNextAttack < gpGlobals->time )
	{
		// attack!

		// make sure it's me who has touched them
		if( tr.pHit == pOther->edict() )
		{
			// and it's not another penguingrenade
			if( tr.pHit->v.modelindex != pev->modelindex )
			{
				// ALERT( at_console, "hit enemy\n" );
				ClearMultiDamage();
				pOther->TraceAttack( pev, gSkillData.snarkDmgBite, gpGlobals->v_forward, &tr, DMG_SLASH );
				if( m_hOwner != 0 )
					ApplyMultiDamage( pev, m_hOwner->pev );
				else
					ApplyMultiDamage( pev, pev );

				pev->dmg += gSkillData.plrDmgHandGrenade; // add more explosion damage
				pev->dmg = Q_min(pev->dmg, 500);

				// make bite sound
				EMIT_SOUND_DYN( ENT( pev ), CHAN_WEAPON, "penguin/penguin_deploy1.wav", 1.0, ATTN_NORM, 0, (int)flpitch );
				m_flNextAttack = gpGlobals->time + 0.5;
			}
		}
		else
		{
			// ALERT( at_console, "been hit\n" );
		}
	}

	m_flNextHit = gpGlobals->time + 0.1;
	m_flNextHunt = gpGlobals->time;

	if( g_pGameRules->IsMultiplayer() )
	{
		// in multiplayer, we limit how often penguins can make their bounce sounds to prevent overflows.
		if( gpGlobals->time < m_flNextBounceSoundTime )
		{
			// too soon!
			return;
		}
	}

	if( !( pev->flags & FL_ONGROUND ) )
	{
		// play bounce sound
		float flRndSound = RANDOM_FLOAT( 0, 1 );

		if( flRndSound <= 0.33 )
			EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "penguin/penguin_hunt1.wav", 1, ATTN_NORM, 0, (int)flpitch );
		else if( flRndSound <= 0.66 )
			EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "penguin/penguin_hunt2.wav", 1, ATTN_NORM, 0, (int)flpitch );
		else
			EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "penguin/penguin_hunt3.wav", 1, ATTN_NORM, 0, (int)flpitch );
		CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 256, 0.25 );
	}
	else
	{
		// skittering sound
		CSoundEnt::InsertSound( bits_SOUND_COMBAT, pev->origin, 100, 0.1 );
	}

	m_flNextBounceSoundTime = gpGlobals->time + 0.5;// half second.
}
#endif

LINK_ENTITY_TO_CLASS( weapon_penguin, CPenguin )

void CPenguin::Spawn()
{
	Precache();
	m_iId = WEAPON_PENGUIN;
	SET_MODEL(ENT(pev), "models/w_penguinnest.mdl");

	FallInit();//get ready to fall down.

	m_iDefaultAmmo = SNARK_DEFAULT_GIVE;

	pev->sequence = 1;
	pev->animtime = gpGlobals->time;
	pev->framerate = 1.0;
}

void CPenguin::Precache( void )
{
	PRECACHE_MODEL("models/w_penguinnest.mdl");
	PRECACHE_MODEL("models/v_penguin.mdl");
	PRECACHE_MODEL("models/p_penguin.mdl");
	PRECACHE_SOUND("penguin/penguin_hunt2.wav");
	PRECACHE_SOUND("penguin/penguin_hunt3.wav");
	UTIL_PrecacheOther("monster_penguin");

	m_usPenguinFire = PRECACHE_EVENT(1, "events/penguinfire.sc");
}

int CPenguin::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "Penguins";
	p->iMaxAmmo1 = PENGUIN_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 4;
	p->iId = m_iId = WEAPON_PENGUIN;
	p->iWeight = PENGUIN_WEIGHT;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;

	return 1;
}

BOOL CPenguin::Deploy()
{
	// play hunt sound
	float flRndSound = RANDOM_FLOAT( 0, 1 );

	if( flRndSound <= 0.5 )
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "penguin/penguin_hunt2.wav", 1, ATTN_NORM, 0, 100 );
	else
		EMIT_SOUND_DYN( ENT( pev ), CHAN_VOICE, "penguin/penguin_hunt3.wav", 1, ATTN_NORM, 0, 100 );

	m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;

	return DefaultDeploy( "models/v_penguin.mdl", "models/p_penguin.mdl", PENGUIN_UP, "penguin" );
}

void CPenguin::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	if( !m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
	{
		m_pPlayer->pev->weapons &= ~(1 << WEAPON_PENGUIN);
		SetThink(&CPenguin::DestroyItem);
		pev->nextthink = gpGlobals->time + 0.1;
		return;
	}

	SendWeaponAnim( PENGUIN_DOWN );
	EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM );
}

void CPenguin::PreThrow()
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0)
	{
		if( !m_flStartThrow )
		{
			m_flStartThrow = gpGlobals->time;
			m_flReleaseThrow = 0;

			//Retain current position
			for (int i = 0; i < 4; ++i)
			{
				m_WeaponPositions[i] = (m_pPlayer->GetWeaponPosition() - m_pPlayer->GetClientOrigin());
				m_WeaponPositionTimestamps[i] = gpGlobals->time;
			}

			m_flTimeWeaponIdle = UTIL_WeaponTimeBase(); //Kick in weapon idle as soon as trigger is released
		}
		else
		{
			//Record recent weapon position for trajectory
			for (int i = 3; i != 0; --i)
			{
				m_WeaponPositions[i] = m_WeaponPositions[i-1];
				m_WeaponPositionTimestamps[i] = m_WeaponPositionTimestamps[i-1];
			}

			m_WeaponPositions[0] =  (m_pPlayer->GetWeaponPosition() - m_pPlayer->GetClientOrigin());
			m_WeaponPositionTimestamps[0] =  gpGlobals->time;
		}
	}
}

void CPenguin::PrimaryAttack() {
	PreThrow();
}

void CPenguin::Throw(Vector throwVelocity)
{
	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] ) {
		int flags;
#ifdef CLIENT_WEAPONS
		flags = FEV_NOTHOST;
#else
		flags = 0;
#endif
		PLAYBACK_EVENT_FULL(flags, m_pPlayer->edict(), m_usPenguinFire, 0.0, g_vecZero, g_vecZero,
							0.0, 0.0, 0, 0, 0, 0);

		// player "shoot" animation
		m_pPlayer->SetAnimation(PLAYER_ATTACK1);

#ifndef CLIENT_DLL
		Vector vecSrc = m_pPlayer->GetWeaponPosition();
        CBaseEntity *pPenguin = CBaseEntity::Create("monster_penguin", vecSrc, Vector(0, 0, 0),
                                                   m_pPlayer->edict());
        pPenguin->pev->velocity = throwVelocity;
#endif

		// play hunt sound
		float flRndSound = RANDOM_FLOAT(0, 1);

		if (flRndSound <= 0.5)
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "penguin/penguin_hunt2.wav", 1, ATTN_NORM, 0, 105);
		else
			EMIT_SOUND_DYN(ENT(pev), CHAN_VOICE, "penguin/penguin_hunt3.wav", 1, ATTN_NORM, 0, 105);

		m_pPlayer->m_iWeaponVolume = QUIET_GUN_VOLUME;
	}
}

void CPenguin::SecondaryAttack( void )
{
	PrimaryAttack();
}

#define VectorDistance(a, b) (sqrt( VectorDistance2( a, b )))
#define VectorDistance2(a, b) (((a)[0] - (b)[0]) * ((a)[0] - (b)[0]) + ((a)[1] - (b)[1]) * ((a)[1] - (b)[1]) + ((a)[2] - (b)[2]) * ((a)[2] - (b)[2]))

void CPenguin::WeaponIdle( void )
{
	if( m_flTimeWeaponIdle > UTIL_WeaponTimeBase() )
		return;

	if ( m_flStartThrow ) {
		//Caclulate speed between oldest reading and second to last
		float distance = VectorDistance(m_WeaponPositions[1], m_WeaponPositions[3]);
		float t = m_WeaponPositionTimestamps[1] - m_WeaponPositionTimestamps[3];
		float velocity = distance / t;

		//Calculate trajectory
		Vector trajectory = m_WeaponPositions[1] - m_WeaponPositions[3];

		// Reduce velocity
		Vector throwVelocity = trajectory * (velocity * (0.7f));

		//Add in player velocity
		throwVelocity = throwVelocity + m_pPlayer->pev->velocity;

		// throw it!
		Throw(throwVelocity);

		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.25f;
		m_flNextPrimaryAttack = GetNextAttackDelay( 0.3 );

		m_flStartThrow = 0;
		m_flReleaseThrow = gpGlobals->time;

		if (!m_pPlayer->m_rgAmmo[PrimaryAmmoIndex()]) {
			RetireWeapon();
			return;
		}

		return;
	}
	else if (m_flReleaseThrow)
	{
		m_flReleaseThrow = 0;
		SendWeaponAnim( PENGUIN_UP );
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + UTIL_SharedRandomFloat( m_pPlayer->random_seed, 10, 15 );
		return;
	}

	int iAnim;
	float flRand = UTIL_SharedRandomFloat( m_pPlayer->random_seed, 0, 1 );
	if( flRand <= 0.75 )
	{
		iAnim = PENGUIN_IDLE1;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 30.0 / 16 * (2);
	}
	else if( flRand <= 0.875 )
	{
		iAnim = PENGUIN_FIDGETFIT;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 70.0 / 16.0;
	}
	else
	{
		iAnim = PENGUIN_FIDGETNIP;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 80.0 / 16.0;
	}
	SendWeaponAnim( iAnim );
}
#endif
