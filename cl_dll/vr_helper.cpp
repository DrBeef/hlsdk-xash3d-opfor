

#include "Matrices.h"
#include "hud.h"
#include "cl_util.h"
#include "r_studioint.h"
#include "ref_params.h"
#include "vr_helper.h"


#define WEAPON_NONE				0
#define WEAPON_CROWBAR			1
#define	WEAPON_GLOCK			2
#define WEAPON_PYTHON			3
#define WEAPON_MP5				4
#define WEAPON_CHAINGUN			5
#define WEAPON_CROSSBOW			6
#define WEAPON_SHOTGUN			7
#define WEAPON_RPG				8
#define WEAPON_GAUSS			9
#define WEAPON_EGON				10
#define WEAPON_HORNETGUN		11
#define WEAPON_HANDGRENADE		12
#define WEAPON_TRIPMINE			13
#define	WEAPON_SATCHEL			14
#define	WEAPON_SNARK			15
#define WEAPON_DISPLACER		16
#define WEAPON_EAGLE			17
#define WEAPON_GRAPPLE			18
#define WEAPON_KNIFE			19
#define	WEAPON_M249				20
#define	WEAPON_PENGUIN			21
#define WEAPON_PIPEWRENCH		22
#define WEAPON_SHOCKRIFLE		23
#define WEAPON_SNIPERRIFLE		24
#define WEAPON_SPORELAUNCHER	25

#ifndef MAX_COMMAND_SIZE
#define MAX_COMMAND_SIZE 256
#endif

bool bIsMultiplayer( void );

VRHelper::VRHelper()
{

}

VRHelper::~VRHelper()
{

}

void VRHelper::Init()
{

}

void VRHelper::Exit(const char* lpErrorMessage)
{
	gEngfuncs.pfnClientCmd("quit");
}

bool VRHelper::UpdatePositions(struct ref_params_s* pparams)
{
	positions.vieworg = pparams->vieworg;
	positions.viewangles = pparams->viewangles;

	UpdateGunPosition(pparams);
	UpdateOffhandPosition(pparams);

	SendPositionUpdateToServer();

	return true;
}

void VRHelper::GetViewAngles(float * angles)
{
	angles[0] = positions.viewangles[0];
	angles[1] = positions.viewangles[1];
	angles[2] = positions.viewangles[2];
}

void VRHelper::UpdateCurrentWeapon( long currentWeaponID )
{
	positions.currentWeapon = currentWeaponID;
}

void VRHelper::UpdateGunPosition(struct ref_params_s* pparams)
{
	cl_entity_t *viewent = gEngfuncs.GetViewModel();
	if (viewent != nullptr)
	{
		//(left/right, forward/backward, up/down)
		cl_entity_t *localPlayer = gEngfuncs.GetLocalPlayer();
		Vector clientPosition = pparams->vieworg;
		positions.weapon.offset =  Vector(pparams->weapon.org.x, pparams->weapon.org.y, clientPosition.z + pparams->weapon.org.z);

		Vector weaponOrigin = clientPosition + pparams->weapon.org;

		//Adjust weapon origin - temporary positional fix
		Vector forward, right, up;
		AngleVectors(pparams->weapon.angles.unadjusted, forward, right, up);

		/*weaponOrigin = weaponOrigin + (forward * -10);
		weaponOrigin = weaponOrigin + (right * -10);
		weaponOrigin = weaponOrigin + (up * 10);*/

		VectorCopy(weaponOrigin, viewent->origin);
		VectorCopy(weaponOrigin, viewent->curstate.origin);
		VectorCopy(weaponOrigin, viewent->latched.prevorigin);

		//Use correct angles
		switch (positions.currentWeapon)
		{
			case WEAPON_CROWBAR:
			case WEAPON_PIPEWRENCH:
			case WEAPON_KNIFE:
				positions.weapon.angles = pparams->weapon.angles.melee;
				viewent->angles = pparams->weapon.angles.melee;
				break;
			case WEAPON_HANDGRENADE:
			case WEAPON_TRIPMINE:
			case WEAPON_SATCHEL:
			case WEAPON_SNARK:
			case WEAPON_PENGUIN:
				positions.weapon.angles = pparams->weapon.angles.unadjusted;
				viewent->angles = pparams->weapon.angles.unadjusted;
				break;
			default:
				//Everything else is adjusted correctly
				positions.weapon.angles = pparams->weapon.angles.adjusted;
				viewent->angles = pparams->weapon.angles.adjusted;
				break;
		}


		VectorCopy(viewent->angles, viewent->curstate.angles);
		VectorCopy(viewent->angles, viewent->latched.prevangles);

		viewent->curstate.velocity = pparams->weapon.velocity;
		positions.weapon.velocity = pparams->weapon.velocity;
	}
}

void VRHelper::UpdateOffhandPosition(struct ref_params_s* pparams)
{
	cl_entity_t *viewent = gEngfuncs.GetViewModel();
	if (viewent != nullptr)
	{
		Vector clientPosition = pparams->vieworg;
		positions.flashlight.offset =  Vector(pparams->flashlight.org.x, pparams->flashlight.org.y, clientPosition.z + pparams->flashlight.org.z);
		positions.flashlight.angles = pparams->flashlight.angles.adjusted;
		positions.flashlight.velocity = pparams->flashlight.velocity;
	}
}

void VRHelper::SendPositionUpdateToServer()
{
	Vector hmdOffset; // Not required
	Vector weaponOffset = positions.weapon.offset;
	Vector weaponAngles = positions.weapon.angles;
	Vector weaponVelocity = positions.weapon.velocity;
	Vector offhandOffset = positions.flashlight.offset;
	Vector offhandAngles = positions.flashlight.angles;

	if (!bIsMultiplayer()) {
        // void CBasePlayer::UpdateVRRelatedPositions(const Vector & hmdOffset, const Vector & weaponoffset, const Vector & weaponAngles, const Vector & weaponVelocity)
        char cmd[MAX_COMMAND_SIZE] = {0};
        sprintf(cmd, "updatevr %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f %.2f",
                hmdOffset.x, hmdOffset.y, hmdOffset.z,
                weaponOffset.x, weaponOffset.y, weaponOffset.z,
                weaponAngles.x, weaponAngles.y, weaponAngles.z,
                weaponVelocity.x, weaponVelocity.y, weaponVelocity.z,
                offhandOffset.x, offhandOffset.y, offhandOffset.z,
                offhandAngles.x, offhandAngles.y, offhandAngles.z
        );

        gEngfuncs.pfnClientCmd(cmd);
    }
}