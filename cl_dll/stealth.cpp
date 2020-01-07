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
//
// Stealth.cpp
//
// implementation of CHudStealth class
//

#include "hud.h"
#include "cl_util.h"
#include <string.h>
#include <stdio.h>
#include "parsemsg.h"

DECLARE_MESSAGE( m_Stealth, Stealth )

int CHudStealth::Init( void )
{
	HOOK_MESSAGE( Stealth );

	m_iPos = 0;
	m_iFlags = 0;
	gHUD.AddHudElem( this );

	return 1;
}

int CHudStealth::VidInit( void )
{
	m_hSprite = 0;

	return 1;
}

int CHudStealth::Draw( float fTime )
{
	if( !m_hSprite )
		m_hSprite = LoadSprite( "sprites/s_stealth.spr" );

    int r, g, b, x, y;

    UnpackRGB( r, g, b, RGB_YELLOWISH );
    SPR_Set( m_hSprite, r, g, b );

    y = (int)(ScreenHeight * 0.36f);
    x = (int)((ScreenWidth * 0.66f) + GetStereoDepthOffset() );

    SPR_DrawAdditive( 0, x, y, NULL );

	return 1;
}

int CHudStealth::MsgFunc_Stealth( const char *pszName, int iSize, void *pbuf )
{
	BEGIN_READ( pbuf, iSize );

	// update Stealth data
	m_iPos = READ_BYTE();

	if( m_iPos )
		m_iFlags |= HUD_ACTIVE;
	else
		m_iFlags &= ~HUD_ACTIVE;

	return 1;
}
