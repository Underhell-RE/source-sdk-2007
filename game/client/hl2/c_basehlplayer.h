//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $Workfile:     $
// $NoKeywords: $
//=============================================================================//

#if !defined( C_BASEHLPLAYER_H )
#define C_BASEHLPLAYER_H
#ifdef _WIN32
#pragma once
#endif


#include "c_baseplayer.h"
#include "c_hl2_playerlocaldata.h"
#include "underhell/uh_inventory.h"

class C_BaseHLPlayer : public C_BasePlayer
{
public:
	DECLARE_CLASS( C_BaseHLPlayer, C_BasePlayer );
	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();

						C_BaseHLPlayer();

	virtual void		OnDataChanged( DataUpdateType_t updateType );

	void				Weapon_DropPrimary( void );
		
	float				GetFOV();
	void				Zoom( float FOVOffset, float time );
	float				GetZoom( void );
	bool				IsZoomed( void )	{ return m_HL2Local.m_bZooming; }

	//Tony; minor cosmetic really, fix confusion by simply renaming this one; everything calls IsSprinting(), and this isn't really even used.
	bool				IsSprintActive( void ) { return m_HL2Local.m_bitsActiveDevices & bits_SUIT_DEVICE_SPRINT; }
	bool				IsFlashlightActive( void ) { return m_HL2Local.m_bitsActiveDevices & bits_SUIT_DEVICE_FLASHLIGHT; }
	bool				IsBreatherActive( void ) { return m_HL2Local.m_bitsActiveDevices & bits_SUIT_DEVICE_BREATHER; }

	virtual int			DrawModel( int flags );

	LadderMove_t		*GetLadderMove() { return &m_HL2Local.m_LadderMove; }
	virtual void		ExitLadder();
	bool				IsSprinting() const { return m_fIsSprinting; }

	// Input handling
	virtual bool	CreateMove( float flInputSampleTime, CUserCmd *pCmd );
	void			PerformClientSideObstacleAvoidance( float flFrameTime, CUserCmd *pCmd );
	void			PerformClientSideNPCSpeedModifiers( float flFrameTime, CUserCmd *pCmd );

	bool				IsWeaponLowered( void ) { return m_HL2Local.m_bWeaponLowered; }

public:

	// Underhell inventory state. Declaration order mirrors the original
	// client binary layout (m_iInventory @5052, the bools @5284+, the ints
	// @5292+). Do not reorder.
	int							m_iInventory[UH_INVENTORY_SLOTS];	// item ids, 0 = empty slot

	C_HL2PlayerLocalData		m_HL2Local;
	EHANDLE						m_hClosestNPC;
	float						m_flSpeedModTime;
	bool						m_fIsSprinting;

	// Underhell flags/counters (networked, see DT_HL2_Player recv table).
	bool						m_bShoulderFlashlight;				// shoulder-mounted flashlight fitted
	bool						m_bFlashlightOn;					// inventory flashlight state
	bool							m_bDisplayHermitCard;				// hermit card deck shown
	bool							m_bInventoryEnabled;				// inventory system enabled
	int								m_iUHBatteryCount;					// battery items held
	int								m_iUHHermitCardsCount;				// collected hermit cards
	int								m_iUHHermitCurrentQuestCount;		// current hermit quest progress
	int								m_iUHHermitTotalQuestCount;			// total hermit quest progress

	// Underhell endurance / hunger state (networked; drawn by CHudEndurance).
	int								m_iEndurance;						// "hunger" meter, 0..100
	int								m_iBleedCounter;					// bleeding state (0 = clean)
	float							m_flUHBatteryCharge;				// 0..100 current battery charge
	bool								m_bIronSighted;						// ironsight active
	float								m_fIronsightedTime;					// last ironsight toggle time
	bool								m_bHavePistolSilencer;				// silencer gear (client mirror)
	bool								m_bHaveRifleSilencer;
	bool								m_bLaserToggleState;

private:
	C_BaseHLPlayer( const C_BaseHLPlayer & ); // not defined, not accessible
	
	bool				TestMove( const Vector &pos, float fVertDist, float radius, const Vector &objPos, const Vector &objDir );

	float				m_flZoomStart;
	float				m_flZoomEnd;
	float				m_flZoomRate;
	float				m_flZoomStartTime;

	bool				m_bPlayUseDenySound;		// Signaled by PlayerUse, but can be unset by HL2 ladder code...
	float				m_flSpeedMod;
	float				m_flExitSpeedMod;


friend class CHL2GameMovement;
};


#endif
