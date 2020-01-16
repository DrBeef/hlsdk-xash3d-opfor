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
#include "gamerules.h"


#define	PIPEWRENCH_BODYHIT_VOLUME 128
#define	PIPEWRENCH_WALLHIT_VOLUME 512

LINK_ENTITY_TO_CLASS( weapon_pipewrench, CPipeWrench );



enum gauss_e {
	PIPEWRENCH_IDLE = 0,
	PIPEWRENCH_DRAW,
	PIPEWRENCH_HOLSTER,
	PIPEWRENCH_ATTACK1HIT,
	PIPEWRENCH_ATTACK1MISS,
	PIPEWRENCH_ATTACK2MISS,
	PIPEWRENCH_ATTACK2HIT,
	PIPEWRENCH_ATTACK3MISS,
	PIPEWRENCH_ATTACK3HIT
};


void CPipeWrench::Spawn( )
{
	Precache( );
	m_iId = WEAPON_PIPEWRENCH;
	SET_MODEL(ENT(pev), "models/w_pipe_wrench.mdl");
	m_iClip = -1;

	FallInit();// get ready to fall down.

#ifndef CLIENT_DLL
	ClearEntitiesHitThisSwing();
#endif
}


void CPipeWrench::Precache( void )
{
	PRECACHE_MODEL("models/v_pipe_wrench.mdl");
	PRECACHE_MODEL("models/w_pipe_wrench.mdl");
	PRECACHE_MODEL("models/p_pipe_wrench.mdl");

	PRECACHE_SOUND("weapons/pwrench_hit1.wav");
	PRECACHE_SOUND("weapons/pwrench_hit2.wav");
	PRECACHE_SOUND("weapons/pwrench_hitbod1.wav");
	PRECACHE_SOUND("weapons/pwrench_hitbod2.wav");
	PRECACHE_SOUND("weapons/pwrench_hitbod3.wav");
	PRECACHE_SOUND("weapons/pwrench_miss1.wav");
	PRECACHE_SOUND("weapons/pwrench_miss2.wav");

	PRECACHE_SOUND("weapons/pwrench_big_hitbod1.wav");
	PRECACHE_SOUND("weapons/pwrench_big_hitbod2.wav");
	PRECACHE_SOUND("weapons/pwrench_big_miss.wav");

	m_usPWrench = PRECACHE_EVENT(1, "events/pipewrench.sc");
}

int CPipeWrench::GetItemInfo(ItemInfo *p)
{
	p->pszName = STRING(pev->classname);
	p->pszAmmo1 = NULL;
	p->iMaxAmmo1 = -1;
	p->pszAmmo2 = NULL;
	p->iMaxAmmo2 = -1;
	p->iMaxClip = WEAPON_NOCLIP;
	p->iSlot = 0;
	p->iPosition = 1;
	p->iId = WEAPON_PIPEWRENCH;
	p->iWeight = WEAPON_PIPEWRENCH;
	return 1;
}



BOOL CPipeWrench::Deploy( )
{
	return DefaultDeploy( "models/v_pipe_wrench.mdl", "models/p_pipe_wrench.mdl", PIPEWRENCH_DRAW, "pipewrench" );
}

void CPipeWrench::Holster( int skiplocal /* = 0 */ )
{
	m_pPlayer->m_flNextAttack = UTIL_WeaponTimeBase() + 0.5;
	SendWeaponAnim( PIPEWRENCH_HOLSTER );
}

#define PIPEWRENCH_MIN_SWING_SPEED 70
#define PIPEWRENCH_LENGTH 24

void CPipeWrench::ItemPostFrame()
{
	MakeLaser();
#ifndef CLIENT_DLL
	Vector weaponVelocity = m_pPlayer->GetWeaponVelocity();
	float speed = weaponVelocity.Length();
	if (speed >= PIPEWRENCH_MIN_SWING_SPEED)
	{
		if (!playedWooshSound)
		{
			// prevent w-w-woo-woosh stutters when player waves pipewrench around frantically
			if (gpGlobals->time > lastWooshSoundTime + 0.5f)
			{
				switch( RANDOM_LONG(0,2) )
				{
				case 0:
					EMIT_SOUND_DYN(ENT(pev), CHAN_ITEM, "weapons/pwrench_miss1.wav", 1, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 3));
				case 1:
					EMIT_SOUND_DYN(ENT(pev), CHAN_ITEM, "weapons/pwrench_miss2.wav", 1, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 3));
				case 2:
					EMIT_SOUND_DYN(ENT(pev), CHAN_ITEM, "weapons/pwrench_big_miss.wav", 1, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 3));
				}

				lastWooshSoundTime = gpGlobals->time;
			}
			playedWooshSound = true;
		}
		CheckSmack(speed);
	}
	else
	{
		playedWooshSound = false;
		hitCount = 0;
		ClearEntitiesHitThisSwing();
	}
#endif
}

//Uncomment to debug the pipewrench
//#define SHOW_PIPEWRENCH_DAMAGE_LINE

void CPipeWrench::MakeLaser( void )
{

#ifndef CLIENT_DLL

	//This is for debugging the pipewrench
#ifdef SHOW_PIPEWRENCH_DAMAGE_LINE
	TraceResult tr;

	// ALERT( at_console, "serverflags %f\n", gpGlobals->serverflags );

	UTIL_MakeVectors (m_pPlayer->GetWeaponViewAngles());
	Vector vecSrc	= m_pPlayer->GetGunPosition();
	Vector vecEnd	= vecSrc + gpGlobals->v_up * PIPEWRENCH_LENGTH;
	UTIL_TraceLine( vecSrc, vecEnd, dont_ignore_monsters, ENT( pev ), &tr );

	float flBeamLength = tr.flFraction;

	if (!g_pLaser || !(g_pLaser->pev)) {
		g_pLaser = CBeam::BeamCreate(g_pModelNameLaser, 3);
	}

	g_pLaser->PointsInit( vecSrc, vecEnd );
	g_pLaser->SetColor( 214, 34, 34 );
	g_pLaser->SetScrollRate( 255 );
	g_pLaser->SetBrightness( 96 );
	g_pLaser->pev->spawnflags |= SF_BEAM_TEMPORARY;	// Flag these to be destroyed on save/restore or level transition
	g_pLaser->pev->owner = m_pPlayer->edict();
#else
	//Normally the pipewrench doesn't have a laser sight
	KillLaser();
#endif //SHOW_PIPEWRENCH_DAMAGE_LINE

#endif
}

#ifndef CLIENT_DLL

void CPipeWrench::CheckSmack(float speed)
{
	UTIL_MakeVectors (m_pPlayer->GetWeaponViewAngles());
	Vector vecSrc	= m_pPlayer->GetGunPosition();
	Vector vecEnd	= vecSrc + gpGlobals->v_up * PIPEWRENCH_LENGTH;

	TraceResult tr;
	UTIL_TraceLine(vecSrc, vecEnd, dont_ignore_monsters, ENT(m_pPlayer->pev), &tr);

	if (!tr.fStartSolid && !tr.fAllSolid && tr.flFraction < 1.0)	// we hit something!
	{
		CBaseEntity *pEntity = CBaseEntity::Instance(tr.pHit);

		if (pEntity && HasNotHitThisEntityThisSwing(pEntity))
		{
			RememberHasHitThisEntityThisSwing(pEntity);

			// play thwack, smack, or dong sound
			float flVol = 1.0;

			hitCount++;

			ClearMultiDamage();
			pEntity->TraceAttack(m_pPlayer->pev, gSkillData.plrDmgPWrench * (speed / PIPEWRENCH_MIN_SWING_SPEED) * (1.f / hitCount), gpGlobals->v_up, &tr, DMG_CLUB);
			ApplyMultiDamage(m_pPlayer->pev, m_pPlayer->pev);

			if ( pEntity->Classify() != CLASS_NONE && pEntity->Classify() != CLASS_MACHINE )
			{
				// play thwack or smack sound
				switch( RANDOM_LONG(0,2) )
				{
				case 0:
					EMIT_SOUND(ENT(pev), CHAN_ITEM, "weapons/pwrench_hitbod1.wav", 1, ATTN_NORM); break;
				case 1:
					EMIT_SOUND(ENT(pev), CHAN_ITEM, "weapons/pwrench_hitbod2.wav", 1, ATTN_NORM); break;
				case 2:
					EMIT_SOUND(ENT(pev), CHAN_ITEM, "weapons/pwrench_hitbod3.wav", 1, ATTN_NORM); break;
				}
				m_pPlayer->m_iWeaponVolume = PIPEWRENCH_BODYHIT_VOLUME;
				if (pEntity->IsAlive())
				{
					flVol = 0.25;
				}
				else
				{
					return; // why?
				}
			}
			else
			{
				float fvolbar = TEXTURETYPE_PlaySound(&tr, vecSrc, vecSrc + (vecEnd - vecSrc) * 2, BULLET_PLAYER_CROWBAR);

				if (g_pGameRules->IsMultiplayer())
				{
					// override the volume here, cause we don't play texture sounds in multiplayer,
					// and fvolbar is going to be 0 from the above call.

					fvolbar = 1;
				}

				// also play pipewrench strike
				switch (RANDOM_LONG(0, 1))
				{
				case 0:
					EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/pwrench_hit1.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 3));
					break;
				case 1:
					EMIT_SOUND_DYN(ENT(m_pPlayer->pev), CHAN_ITEM, "weapons/pwrench_hit2.wav", fvolbar, ATTN_NORM, 0, 98 + RANDOM_LONG(0, 3));
					break;
				}
			}

			//vibrate a bit
			char buffer[256];
			sprintf(buffer, "vibrate 90.0 %i 1.0\n", 1-(int)CVAR_GET_FLOAT("hand"));
			SERVER_COMMAND(buffer);

			m_pPlayer->m_iWeaponVolume = flVol * PIPEWRENCH_WALLHIT_VOLUME;
			DecalGunshot(&tr, BULLET_PLAYER_CROWBAR);
		}
	}
}

bool CPipeWrench::HasNotHitThisEntityThisSwing(CBaseEntity *pEntity)
{
	int hitEntitiesCount = sizeof(hitEntities) / sizeof(EHANDLE);
	for (int i = 0; i < hitEntitiesCount; i++)
	{
		if (((CBaseEntity*)hitEntities[i]) == pEntity)
		{
			return false;
		}
	}
	return true;
}

 void CPipeWrench::RememberHasHitThisEntityThisSwing(CBaseEntity *pEntity)
{
	int hitEntitiesCount = sizeof(hitEntities) / sizeof(EHANDLE);
	for (int i = 0; i < hitEntitiesCount; i++)
	{
		if (hitEntities[i])
		{
			continue;
		}
		else
		{
			hitEntities[i] = pEntity;
			return;
		}
	}
}

 void CPipeWrench::ClearEntitiesHitThisSwing()
{
	int hitEntitiesCount = sizeof(hitEntities) / sizeof(EHANDLE);
	for (int i = 0; i < hitEntitiesCount; i++)
	{
		hitEntities[i] = NULL;
	}
}

#endif

