//========= Copyright (c) 2008, Mxthe (Underhell). All rights reserved. ============//
//
// Purpose: Underhell bullet time — visible travelling bullets (CBtBullet).
//
// Decompiled reference (servero_diaphora.dll.c). The original class is named
// "CBtBullet" (datadesc name string @sub_10107D90), derives from
// CBaseAnimating, and has exactly two datadesc functions plus one saved field:
//     DEFINE_ENTITYFUNC( Touch )         -> sub_10107B50
//     DEFINE_THINKFUNC ( BulletThink )   -> sub_101078D0
//     DEFINE_FIELD( m_vforward, FIELD_VECTOR )   // player-struct offset 1220
//
// Decoded member layout (byte offsets into the entity):
//     1164  m_iAmmoType      (model selection switch)
//     1208  m_pOwner         (shooter; Touch ignores it)
//     1212  m_bPlayerBullet  (picks which speed convar to use)
//     1216  m_flSpeed        (resolved once at spawn)
//     1220  m_vforward       (unit travel direction)
//
// Spawn (sub_10107970):
//     SetSolidFlags( solidflags | FSOLID_TRIGGER )
//     switch ( m_iAmmoType ) { 3,4: bt_9mm; 5: bt_357; 7: w_pellet; default: bt_762 }
//     SetMoveType( MOVETYPE_FLY )
//     m_flSpeed = m_bPlayerBullet ? bt_playerbulletspeed : bt_enemybulletspeed
//     SetAbsVelocity( direction * m_flSpeed )
//     SetCollisionGroup( COLLISION_GROUP_NONE )
//     UseTriggerBounds( true, 36.0 )
//     AngleVectors( GetAbsAngles(), &m_vforward )
//
// Think (sub_101078D0), every 0.05 s:
//     speed = bt_enabled ? m_flSpeed : 2500.0
//     SetAbsVelocity( m_vforward * speed )
//   NOTE: the original does NOT scale this by bt_timescale. bt_timescale only
//   drives host_timescale; the world is already slowed, so multiplying the
//   bullet speed by it as well slowed the bullets twice over.
//
// Touch (sub_10107B50): traces a short segment around the bullet and, when the
// toucher is not the shooter, calls the impact handler and UTIL_Remove()s
// itself. This is the bullet's real lifecycle — an earlier port used immortal
// prop_physics entities with no removal path at all, which leaked one entity
// (and one motion-system record polled 20x/second) per shot fired.
//
//=============================================================================//

#include "cbase.h"
#include "underhell/uh_bullettime.h"
#include "hl2_player.h"

#include "tier0/memdbgon.h"

static void UH_BulletTimeChanged( IConVar *pVar, const char *pOldValue, float flOldValue );
ConVar bt_enabled( "bt_enabled", "0", FCVAR_CHEAT | FCVAR_REPLICATED, "enable bullet time", UH_BulletTimeChanged );
ConVar bt_timescale( "bt_timescale", "0.3", FCVAR_NONE, "bullet time scale" );
ConVar bt_enemybulletspeed( "bt_enemybulletspeed", "500", FCVAR_NONE, "enemy bullet speed" );
ConVar bt_playerbulletspeed( "bt_playerbulletspeed", "2000", FCVAR_NONE, "player bullet speed" );
ConVar bt_plr_speed( "bt_plr_speed", "250", FCVAR_NONE, "player speed during bullet time" );

// Speed used by the original once bullet time has been switched back off: the
// visible bullet finishes its flight quickly instead of hanging in the air.
#define UH_BTBULLET_NORMAL_SPEED	2500.0f
#define UH_BTBULLET_THINK_INTERVAL	0.05f
#define UH_BTBULLET_TRIGGER_BLOAT	36.0f

bool UH_BulletTimeActive() { return bt_enabled.GetBool(); }
void UH_SetBulletTime( bool bEnabled ) { bt_enabled.SetValue( bEnabled ? 1 : 0 ); }

//-----------------------------------------------------------------------------
// Model per ammo type (sub_10107970 switch on offset 1164).
//-----------------------------------------------------------------------------
static const char *UH_BulletModel( int ammoType )
{
	switch ( ammoType )
	{
	case 3: case 4: return "models/weapons/bt_9mm.mdl";
	case 5: return "models/weapons/bt_357.mdl";
	case 7: return "models/weapons/w_pellet.mdl";
	default: return "models/weapons/bt_762.mdl";
	}
}

//-----------------------------------------------------------------------------
// CBtBullet — the visible bullet-time projectile.
//-----------------------------------------------------------------------------
class CBtBullet : public CBaseAnimating
{
	DECLARE_CLASS( CBtBullet, CBaseAnimating );
	DECLARE_DATADESC();

public:
	void	Precache( void );
	void	Spawn( void );
	void	BulletThink( void );
	void	Touch( CBaseEntity *pOther );

	void	SetupBullet( CBaseEntity *pShooter, const Vector &direction, int ammoType, bool bPlayerBullet );

private:
	Vector	m_vforward;			// original field @1220, the only saved vector
	float	m_flSpeed;			// @1216
	int		m_iBtAmmoType;		// @1164
	bool	m_bPlayerBullet;	// @1212
};

LINK_ENTITY_TO_CLASS( bt_bullet, CBtBullet );

BEGIN_DATADESC( CBtBullet )
	DEFINE_FIELD( m_vforward, FIELD_VECTOR ),
	DEFINE_FIELD( m_flSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( m_iBtAmmoType, FIELD_INTEGER ),
	DEFINE_FIELD( m_bPlayerBullet, FIELD_BOOLEAN ),
	DEFINE_ENTITYFUNC( Touch ),
	DEFINE_THINKFUNC( BulletThink ),
END_DATADESC()

//-----------------------------------------------------------------------------
// sub_10107790: every bullet model is precached up front.
//-----------------------------------------------------------------------------
void CBtBullet::Precache( void )
{
	BaseClass::Precache();

	PrecacheModel( "models/weapons/w_bullet.mdl" );
	PrecacheModel( "models/weapons/bt_9mm.mdl" );
	PrecacheModel( "models/weapons/bt_357.mdl" );
	PrecacheModel( "models/weapons/bt_762.mdl" );
	PrecacheModel( "models/weapons/w_pellet.mdl" );
}

void CBtBullet::Spawn( void )
{
	Precache();

	SetModel( UH_BulletModel( m_iBtAmmoType ) );

	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_TRIGGER | FSOLID_NOT_SOLID );
	SetCollisionGroup( COLLISION_GROUP_NONE );
	UTIL_SetSize( this, vec3_origin, vec3_origin );
	CollisionProp()->UseTriggerBounds( true, UH_BTBULLET_TRIGGER_BLOAT );

	SetMoveType( MOVETYPE_FLY );
	AddEffects( EF_NOSHADOW );

	// Resolved once, exactly like the original: which convar is read depends on
	// who fired, and the value is then cached in m_flSpeed.
	m_flSpeed = m_bPlayerBullet ? bt_playerbulletspeed.GetFloat() : bt_enemybulletspeed.GetFloat();
	SetAbsVelocity( m_vforward * m_flSpeed );

	SetTouch( &CBtBullet::Touch );
	SetThink( &CBtBullet::BulletThink );
	SetNextThink( gpGlobals->curtime + UH_BTBULLET_THINK_INTERVAL );
}

void CBtBullet::SetupBullet( CBaseEntity *pShooter, const Vector &direction, int ammoType, bool bPlayerBullet )
{
	m_vforward = direction;
	VectorNormalize( m_vforward );
	m_iBtAmmoType = ammoType;
	m_bPlayerBullet = bPlayerBullet;

	SetOwnerEntity( pShooter );

	QAngle bulletAngles;
	VectorAngles( m_vforward, bulletAngles );
	SetAbsAngles( bulletAngles );
}

//-----------------------------------------------------------------------------
// sub_101078D0. Re-applies velocity on a fixed cadence so the bullet keeps
// flying at the right speed when bullet time is toggled mid-flight.
//-----------------------------------------------------------------------------
void CBtBullet::BulletThink( void )
{
	SetNextThink( gpGlobals->curtime + UH_BTBULLET_THINK_INTERVAL );

	// Raw m_flSpeed while BT is on — NOT scaled by bt_timescale.
	const float flSpeed = UH_BulletTimeActive() ? m_flSpeed : UH_BTBULLET_NORMAL_SPEED;
	SetAbsVelocity( m_vforward * flSpeed );
}

//-----------------------------------------------------------------------------
// sub_10107B50. The shooter is ignored; anything else ends the bullet's life.
//-----------------------------------------------------------------------------
void CBtBullet::Touch( CBaseEntity *pOther )
{
	if ( !pOther || pOther == GetOwnerEntity() )
		return;

	// Ignore other in-flight bullets so a burst does not delete itself.
	if ( dynamic_cast< CBtBullet * >( pOther ) )
		return;

	if ( !pOther->IsSolid() && !pOther->IsSolidFlagSet( FSOLID_TRIGGER ) )
		return;

	// The original traces a short segment (origin - forward*8 .. origin +
	// forward*4) to place the impact, then removes itself. Damage is already
	// applied by the instant hit-scan FireBullets pass, so this is purely the
	// visual bullet's end of life.
	Vector vecStart = GetAbsOrigin() - m_vforward * 8.0f;
	Vector vecEnd = GetAbsOrigin() + m_vforward * 4.0f;

	trace_t tr;
	UTIL_TraceLine( vecStart, vecEnd, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction < 1.0f )
		UTIL_ImpactTrace( &tr, DMG_BULLET );

	SetTouch( NULL );
	SetThink( NULL );
	UTIL_Remove( this );
}

//-----------------------------------------------------------------------------
// Public spawn helper used by the weapon fire path.
//-----------------------------------------------------------------------------
void UH_BulletTimeSpawnTracer( CBaseCombatCharacter *pShooter, const Vector &start, const Vector &direction, int ammoType, bool bEnemyBullet )
{
	if ( !UH_BulletTimeActive() || !pShooter )
		return;

	CBtBullet *pBullet = (CBtBullet *)CreateEntityByName( "bt_bullet" );
	if ( !pBullet )
		return;

	pBullet->SetAbsOrigin( start );
	pBullet->SetupBullet( pShooter, direction, ammoType, !bEnemyBullet );
	DispatchSpawn( pBullet );
}

//-----------------------------------------------------------------------------
// bt_enabled change callback.
//-----------------------------------------------------------------------------
static void UH_BulletTimeChanged( IConVar *pVar, const char *pOldValue, float flOldValue )
{
	ConVar *pScale = cvar->FindVar( "host_timescale" );
	ConVar *pNormalSpeed = cvar->FindVar( "hl2_normspeed" );
	const bool bOn = bt_enabled.GetBool();

	if ( pScale )
		pScale->SetValue( bOn ? bt_timescale.GetFloat() : 1.0f );

	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CHL2_Player *pPlayer = dynamic_cast< CHL2_Player * >( UTIL_PlayerByIndex( i ) );
		if ( !pPlayer )
			continue;

		pPlayer->SetMaxSpeed( bOn ? bt_plr_speed.GetFloat() : ( pNormalSpeed ? pNormalSpeed->GetFloat() : 150.0f ) );
		pPlayer->EmitSound( bOn ? "Player.bullettimestart" : "Player.bullettimeend" );
		engine->ClientCommand( pPlayer->edict(), bOn ? "r_screenoverlay dev/bullettime" : "r_screenoverlay off" );
	}
}

//-----------------------------------------------------------------------------
// Death cancels bullet time (original sub_102DDB80, called from Event_Killed).
// Without this host_timescale stays at bt_timescale forever — including into
// saves made after the player died.
//-----------------------------------------------------------------------------
void UH_BulletTimePlayerDied( CBasePlayer *pPlayer )
{
	if ( bt_enabled.GetBool() )
		bt_enabled.SetValue( 0 );
}

void CHL2_Player::InputEnableBt( inputdata_t &inputdata ) { UH_SetBulletTime( true ); }
void CHL2_Player::InputDisableBt( inputdata_t &inputdata ) { UH_SetBulletTime( false ); }
