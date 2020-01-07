/***
*
*	Copyright (c) 1996-2001, Valve LLC. All rights reserved.
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
//=========================================================
// shockroach.cpp
//=========================================================

#include	"extdll.h"
#include	"util.h"
#include	"cbase.h"
#include	"monsters.h"
#include	"schedule.h"
#include	"game.h"
#include	"player.h"
#include	"weapons.h"
#include	"headcrab.h"
#include	"shockroach.h"

LINK_ENTITY_TO_CLASS(monster_shockroach, CShockRoach)

TYPEDESCRIPTION	CShockRoach::m_SaveData[] =
{
	DEFINE_FIELD(CShockRoach, m_flBirthTime, FIELD_TIME),
	DEFINE_FIELD(CShockRoach, m_fRoachSolid, FIELD_BOOLEAN),
};

IMPLEMENT_SAVERESTORE(CShockRoach, CHeadCrab)

const char *CShockRoach::pIdleSounds[] =
{
	"shockroach/shock_idle1.wav",
	"shockroach/shock_idle2.wav",
	"shockroach/shock_idle3.wav",
};
const char *CShockRoach::pAlertSounds[] =
{
	"shockroach/shock_angry.wav",
};
const char *CShockRoach::pPainSounds[] =
{
	"shockroach/shock_flinch.wav",
};
const char *CShockRoach::pAttackSounds[] =
{
	"shockroach/shock_jump1.wav",
	"shockroach/shock_jump2.wav",
};

const char *CShockRoach::pDeathSounds[] =
{
	"shockroach/shock_die.wav",
};

const char *CShockRoach::pBiteSounds[] =
{
	"shockroach/shock_bite.wav",
};


//=========================================================
// Spawn
//=========================================================
void CShockRoach::Spawn()
{
	Precache();

	SET_MODEL(ENT(pev), "models/w_shock_rifle.mdl");
	UTIL_SetOrigin(pev, pev->origin);

	pev->solid = SOLID_SLIDEBOX;
	pev->movetype = MOVETYPE_FLY;
	m_bloodColor = BLOOD_COLOR_GREEN;

	pev->effects = 0;
	pev->health = gSkillData.sroachHealth;
	pev->view_ofs = Vector(0, 0, 20);// position of the eyes relative to monster's origin.
	pev->yaw_speed = 5;//!!! should we put this in the monster's changeanim function since turn rates may vary with state/anim?
	m_flFieldOfView = 0.5;// indicates the width of this monster's forward view cone ( as a dotproduct result )
	m_MonsterState = MONSTERSTATE_NONE;

	m_fRoachSolid = 0;
	m_flBirthTime = gpGlobals->time;

	MonsterInit();
}

//=========================================================
// Precache - precaches all resources this monster needs
//=========================================================
void CShockRoach::Precache()
{
	PRECACHE_SOUND_ARRAY(pIdleSounds);
	PRECACHE_SOUND_ARRAY(pAlertSounds);
	PRECACHE_SOUND_ARRAY(pPainSounds);
	PRECACHE_SOUND_ARRAY(pAttackSounds);
	PRECACHE_SOUND_ARRAY(pDeathSounds);
	PRECACHE_SOUND_ARRAY(pBiteSounds);

	PRECACHE_SOUND("shockroach/shock_walk.wav");

	PRECACHE_MODEL("models/w_shock_rifle.mdl");
}

//=========================================================
// LeapTouch - this is the headcrab's touch function when it
// is in the air
//=========================================================
void CShockRoach::LeapTouch(CBaseEntity *pOther)
{
	if (!pOther->pev->takedamage)
	{
		return;
	}

	if (pOther->Classify() == Classify())
	{
		return;
	}

	// Don't hit if back on ground
	if (!FBitSet(pev->flags, FL_ONGROUND))
	{
		EMIT_SOUND_DYN(edict(), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pBiteSounds), GetSoundVolume(), ATTN_IDLE, 0, GetVoicePitch());

		// Give the shockrifle weapon to the player, if not already in possession.
		if (pOther->IsPlayer() && pOther->IsAlive() && !(pOther->pev->weapons & (1 << WEAPON_SHOCKRIFLE))) {
			CBasePlayer* pPlayer = (CBasePlayer*)(pOther);
			pPlayer->GiveNamedItem("weapon_shockrifle");
			pPlayer->pev->weapons |= (1 << WEAPON_SHOCKRIFLE);
			UTIL_Remove(this);
			return;
		}
	
		pOther->TakeDamage(pev, pev, GetDamageAmount(), DMG_SLASH);
	}

	SetTouch(NULL);
}
//=========================================================
// PrescheduleThink
//=========================================================
void CShockRoach::MonsterThink(void)
{
	float lifeTime = (gpGlobals->time - m_flBirthTime);
	if (lifeTime >= 0.2)
	{
		pev->movetype = MOVETYPE_STEP;
	}
	if (!m_fRoachSolid && lifeTime >= 2.0 ) {
		m_fRoachSolid = TRUE;
		UTIL_SetSize(pev, Vector(-12, -12, 0), Vector(12, 12, 4));
	}
	if (lifeTime >= gSkillData.sroachLifespan)
	{
		pev->health = -1;
		Killed(pev, 0);
		return;
	}

	CHeadCrab::MonsterThink();
}

//=========================================================
// IdleSound
//=========================================================
void CShockRoach::IdleSound(void)
{
	EMIT_SOUND_DYN(edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pIdleSounds), GetSoundVolume(), ATTN_IDLE, 0, GetVoicePitch());
}

//=========================================================
// AlertSound 
//=========================================================
void CShockRoach::AlertSound(void)
{
	EMIT_SOUND_DYN(edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pAlertSounds), GetSoundVolume(), ATTN_IDLE, 0, GetVoicePitch());
}

//=========================================================
// AlertSound 
//=========================================================
void CShockRoach::PainSound(void)
{
	EMIT_SOUND_DYN(edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pPainSounds), GetSoundVolume(), ATTN_IDLE, 0, GetVoicePitch());
}

//=========================================================
// DeathSound 
//=========================================================
void CShockRoach::DeathSound(void)
{
	EMIT_SOUND_DYN(edict(), CHAN_VOICE, RANDOM_SOUND_ARRAY(pDeathSounds), GetSoundVolume(), ATTN_IDLE, 0, GetVoicePitch());
}


void CShockRoach::StartTask(Task_t *pTask)
{
	m_iTaskStatus = TASKSTATUS_RUNNING;

	switch (pTask->iTask)
	{
	case TASK_RANGE_ATTACK1:
	{
		m_IdealActivity = ACT_RANGE_ATTACK1;
		SetTouch(&CShockRoach::LeapTouch);
		break;
	}
	default:
		CHeadCrab::StartTask(pTask);
	}
}

int CShockRoach::TakeDamage( entvars_t *pevInflictor, entvars_t *pevAttacker, float flDamage, int bitsDamageType )
{
	if ( gpGlobals->time - m_flBirthTime < 2.0 )
		flDamage = 0.0;
	// Skip headcrab's TakeDamage to avoid unwanted immunity to acid.
	return CBaseMonster::TakeDamage( pevInflictor, pevAttacker, flDamage, bitsDamageType );
}

void CShockRoach::AttackSound()
{
	EMIT_SOUND_DYN(edict(), CHAN_WEAPON, RANDOM_SOUND_ARRAY(pAttackSounds), GetSoundVolume(), ATTN_IDLE, 0, GetVoicePitch());
}
