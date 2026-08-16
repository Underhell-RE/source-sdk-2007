//========= Copyright (c) 2008, Mxthe (Underhell). All rights reserved. ============//
//
// Purpose: Underhell laser sight — client-side beam + impact dot.
//
// CWeaponPistolSocom owns its own env_laserdot lifecycle in the original
// server DLL (create/update on deploy, destroy on holster). The client draws
// the matching beam from the active SOCOM; it is not a player-wide console
// toggle. Sprites match the original laser materials.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "c_basehlplayer.h"
#include "igamesystem.h"
#include "iviewrender_beams.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//-----------------------------------------------------------------------------
// Laser beam + dot, redrawn every rendered frame while the local player has
// the laser sight toggled on.
//-----------------------------------------------------------------------------
class CUHLaserSight : public CAutoGameSystemPerFrame
{
	typedef CAutoGameSystemPerFrame BaseClass;

public:
	CUHLaserSight() : BaseClass( "CUHLaserSight" ), m_iBeamModel( -1 ), m_iDotModel( -1 ) {}

	virtual void LevelInitPostEntity( void )
	{
		// Precache the two sprites (client model indices).
		m_iBeamModel = C_BaseEntity::PrecacheModel( "sprites/laserbeam.vmt" );
		m_iDotModel  = C_BaseEntity::PrecacheModel( "sprites/laserdot.vmt" );
	}

	// Client-side per-frame hook (NOT FrameUpdatePostEntityThink — that is
	// server-only). Called once per rendered frame.
	virtual void Update( float frametime )
	{
		C_BaseHLPlayer *pPlayer = dynamic_cast<C_BaseHLPlayer *>( C_BasePlayer::GetLocalPlayer() );
		if ( !pPlayer )
			return;

		C_BaseCombatWeapon *pWeapon = pPlayer->GetActiveWeapon();
		if ( !pWeapon || !FClassnameIs( pWeapon, "weapon_pistol_socom" ) )
			return;

		if ( !pPlayer->IsAlive() )
			return;

		Vector vecStart = pPlayer->EyePosition();
		Vector forward;
		AngleVectors( pPlayer->EyeAngles(), &forward );

		trace_t tr;
		UTIL_TraceLine( vecStart, vecStart + forward * MAX_TRACE_LENGTH, MASK_SHOT,
			pPlayer, COLLISION_GROUP_NONE, &tr );

		// Transient beam + halo dot at the impact point. A short life means it
		// is redrawn fresh every frame and auto-expires (no leak / bookkeeping).
		beams->CreateBeamPoints( vecStart, tr.endpos, m_iBeamModel, m_iDotModel,
			0.0f,					// halo scale
			0.05f,					// life
			1.0f,					// width
			1.0f,					// end width
			1.0f,					// fade length
			0.0f,					// amplitude
			255.0f,					// brightness
			0.0f,					// speed
			0,						// start frame
			0.0f,					// frame rate
			255.0f, 0.0f, 0.0f );	// red
	}

private:
	int m_iBeamModel;
	int m_iDotModel;
};

static CUHLaserSight g_UHLaserSight;
