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

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "monsters.h"
#include "weapons.h"
#include "nodes.h"
#include "player.h"

#define	HANDGRENADE_PRIMARY_VOLUME		450

enum handgrenade_e
{
	HANDGRENADE_IDLE = 0,
	HANDGRENADE_FIDGET,
	HANDGRENADE_PINPULL,
	HANDGRENADE_THROW1,	// toss
	HANDGRENADE_THROW2,	// medium
	HANDGRENADE_THROW3,	// hard
	HANDGRENADE_HOLSTER,
	HANDGRENADE_DRAW
};

LINK_ENTITY_TO_CLASS( weapon_handgrenade, CHandGrenade )

void CHandGrenade::Spawn()
{
	Precache();
	m_iId = WEAPON_HANDGRENADE;
	SET_MODEL( ENT( pev ), "models/w_grenade.mdl" );

#ifndef CLIENT_DLL
	pev->dmg = gSkillData.plrDmgHandGrenade;
#endif
	m_iDefaultAmmo = HANDGRENADE_DEFAULT_GIVE;

	FallInit();// get ready to fall down.
}

void CHandGrenade::Precache( void )
{
	PRECACHE_MODEL( "models/w_grenade.mdl" );
	PRECACHE_MODEL( "models/v_grenade.mdl" );
	PRECACHE_MODEL( "models/p_grenade.mdl" );
}

int CHandGrenade::GetItemInfo( ItemInfo *p )
{
	p->pszName = STRING( pev->classname );
	p->pszAmmo1 = "Hand Grenade";
	p->iMaxAmmo1 = HANDGRENADE_MAX_CARRY;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 4;
	p->iPosition = 0;
	p->iId = m_iId = WEAPON_HANDGRENADE;
	p->iWeight = HANDGRENADE_WEIGHT;
	p->iFlags = ITEM_FLAG_LIMITINWORLD | ITEM_FLAG_EXHAUSTIBLE;

	return 1;
}

BOOL CHandGrenade::Deploy()
{
	m_flReleaseThrow = 0;
	return DefaultDeploy( "models/v_grenade.mdl", "models/p_grenade.mdl", HANDGRENADE_DRAW, "crowbar" );
}

BOOL CHandGrenade::CanHolster( void )
{
	// can only holster hand grenades when not primed!
	return ( m_flStartThrow == 0 );
}

void CHandGrenade::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;

	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
	{
		SendWeaponAnim( HANDGRENADE_HOLSTER );
	}
	else
	{
		// no more grenades!
		m_pPlayer->pev->weapons &= ~( 1 << WEAPON_HANDGRENADE );
		DestroyItem();
	}

	if( m_flStartThrow )
	{
		m_flStartThrow = 0;
		m_flReleaseThrow = 0;
	}

	EMIT_SOUND( ENT( m_pPlayer->pev ), CHAN_WEAPON, "common/null.wav", 1.0, ATTN_NORM );
}

void CHandGrenade::PrimaryAttack()
{
	if (m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] > 0)
	{
		if( !m_flStartThrow )
		{
			//Use start throw to record when the grenade is going to explode.. add a small amount of unpredictability to the fuse
			//length which is common for chemical fused grenades apparently
			m_flStartThrow = gpGlobals->time + 5.0f + UTIL_SharedRandomFloat( m_pPlayer->random_seed, -0.25f, 0.25f );
		m_flReleaseThrow = 0;

			//Retain current position
			for (int i = 0; i < 4; ++i)
			{
				m_WeaponPositions[i] = (m_pPlayer->GetWeaponPosition() - m_pPlayer->GetClientOrigin());
				m_WeaponPositionTimestamps[i] = gpGlobals->time;
			}

		SendWeaponAnim( HANDGRENADE_PINPULL );
			m_flTimeWeaponIdle = UTIL_WeaponTimeBase(); //Kick in weapon idle as soon as trigger is released

			//vibrate a bit
			char buffer[256];
			sprintf(buffer, "vibrate 80.0 %i 0.4\n", 1-(int)CVAR_GET_FLOAT("hand"));
			SERVER_COMMAND(buffer);
	}
		else
{
			// check remaining fuse length
			float fuseRemaining = m_flStartThrow - gpGlobals->time;
			if (fuseRemaining < 0)
			{
				//Oh dear, silly player still holding grenade and it is going to explode!
				Vector nullVelocity;
				CGrenade::ShootTimed(m_pPlayer->pev, m_pPlayer->GetWeaponPosition(), nullVelocity, 0.05f);

				//Reset in case player survived that foolishness
				m_flStartThrow = 0;
				m_flReleaseThrow = 0;

				m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ]--;
				if ( !m_pPlayer->m_rgAmmo[ m_iPrimaryAmmoType ] )
				{
					// just threw last grenade
					m_flTimeWeaponIdle = m_flNextSecondaryAttack = m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5f;
					RetireWeapon();
				}
				else
	{
					SendWeaponAnim( HANDGRENADE_DRAW );
					m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5f;
				}
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
}

#define VectorDistance(a, b) (sqrt( VectorDistance2( a, b )))
#define VectorDistance2(a, b) (((a)[0] - (b)[0]) * ((a)[0] - (b)[0]) + ((a)[1] - (b)[1]) * ((a)[1] - (b)[1]) + ((a)[2] - (b)[2]) * ((a)[2] - (b)[2]))

void CHandGrenade::WeaponIdle( void )
{
	if ( m_flStartThrow )
	{
		//Caclulate speed between oldest reading and second to last
		float distance = VectorDistance(m_WeaponPositions[1], m_WeaponPositions[3]);
		float t = m_WeaponPositionTimestamps[1] - m_WeaponPositionTimestamps[3];
		float velocity = distance / t;

		//Calculate trajectory
		Vector trajectory = m_WeaponPositions[1] - m_WeaponPositions[3];

		// Reduce velocity by 1/3 for a bit of weight otherwise it is too light
		Vector throwVelocity = trajectory * (velocity * (2.0f/3.0f));

		//Add in player velocity
		throwVelocity = throwVelocity + m_pPlayer->pev->velocity;

		// Calculate remaining fuse time
		float fuseRemaining = m_flStartThrow - gpGlobals->time;
		if (fuseRemaining < 0)
		{
			fuseRemaining = 0;
		}
		CGrenade::ShootTimed(m_pPlayer->pev, m_pPlayer->GetWeaponPosition(), throwVelocity, fuseRemaining);

		//Player "shoot" animation is not needed, player has provided the "animation" for us by actually throwing
		//So just go back to "idle"
		m_pPlayer->SetAnimation( PLAYER_IDLE );

		m_flStartThrow = 0;
		m_flReleaseThrow = gpGlobals->time;
		m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5f;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase();

		m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType]--;

		if( !m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
		{
			// just threw last grenade
			// set attack times in the future, and weapon idle will automatically retire the weapon for us.
			m_flTimeWeaponIdle = m_flNextSecondaryAttack = m_flNextPrimaryAttack = UTIL_WeaponTimeBase() + 0.5f;// ensure that the animation can finish playing
		}

		return;
	}
	else if( m_flReleaseThrow > 0 )
	{
		// we've finished the throw, restart.
		m_flReleaseThrow = 0;

		if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
		{
			SendWeaponAnim( HANDGRENADE_DRAW );
		}
		else
		{
			RetireWeapon();
			return;
		}

		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 10.0f;

		return;
	}

	if( m_pPlayer->m_rgAmmo[m_iPrimaryAmmoType] )
	{
		int iAnim = HANDGRENADE_IDLE;
		m_flTimeWeaponIdle = UTIL_WeaponTimeBase() + 0.5f;
		SendWeaponAnim( iAnim );
	}
}