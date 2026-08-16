// Underhell bullet-time visual bullet implementation.
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

class CUHBullet : public CBaseAnimating
{
	DECLARE_CLASS( CUHBullet, CBaseAnimating );
	DECLARE_DATADESC();
public:
	void Spawn() { SetSolid( SOLID_NONE ); SetMoveType( MOVETYPE_FLY ); AddEffects( EF_NOSHADOW ); SetThink( &CUHBullet::BulletThink ); SetNextThink( gpGlobals->curtime ); }
	void BulletThink() { float speed = UH_BulletTimeActive() ? m_flSpeed * bt_timescale.GetFloat() : 2500.0f; SetAbsVelocity( m_vecDirection * speed ); m_flRemainingDistance -= speed * 0.05f; SetNextThink( gpGlobals->curtime + 0.05f ); if ( m_flRemainingDistance <= 0.0f ) UTIL_Remove( this ); }
	Vector m_vecDirection; float m_flSpeed; float m_flRemainingDistance;
};
LINK_ENTITY_TO_CLASS( uh_bullet, CUHBullet );
BEGIN_DATADESC( CUHBullet )
	DEFINE_FIELD( m_vecDirection, FIELD_VECTOR ),
	DEFINE_FIELD( m_flSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( m_flRemainingDistance, FIELD_FLOAT ),
	DEFINE_THINKFUNC( BulletThink ),
END_DATADESC()

bool UH_BulletTimeActive() { return bt_enabled.GetBool(); }
void UH_SetBulletTime( bool bEnabled ) { bt_enabled.SetValue( bEnabled ? 1 : 0 ); }

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

void UH_BulletTimeSpawnTracer( CBaseCombatCharacter *pShooter, const Vector &start, const Vector &direction, int ammoType, bool bEnemyBullet )
{
	if ( !UH_BulletTimeActive() || !pShooter ) return;
	// Use prop_physics for the visible bullet: unlike the old custom
	// CBaseAnimating entity it has a stock server/client network class and a
	// replicated VPhysics object, matching the original CBullet's visible
	// physics/entity behavior.
	CBaseEntity *pBullet = CreateEntityByName( "prop_physics" );
	if ( !pBullet ) return;
	const char *pModel = UH_BulletModel( ammoType );
	CBaseEntity::PrecacheModel( pModel );
	Vector dir = direction;
	VectorNormalize( dir );
	QAngle bulletAngles;
	VectorAngles( dir, bulletAngles );
	pBullet->SetModel( pModel );
	pBullet->SetAbsOrigin( start );
	pBullet->SetAbsAngles( bulletAngles );
	pBullet->SetOwnerEntity( pShooter );
	DispatchSpawn( pBullet );
	IPhysicsObject *pPhysics = pBullet->VPhysicsGetObject();
	if ( pPhysics )
	{
		float speed = ( bEnemyBullet ? bt_enemybulletspeed.GetFloat() : bt_playerbulletspeed.GetFloat() ) * bt_timescale.GetFloat();
		Vector velocity = dir * speed;
		pPhysics->SetVelocity( &velocity, NULL );
	}
	pBullet->SetContextThink( &CBaseEntity::SUB_Remove, gpGlobals->curtime + 120.0f, "BulletTimeLifetime" );
}

static void UH_BulletTimeChanged( IConVar *pVar, const char *pOldValue, float flOldValue )
{
	ConVar *pScale = cvar->FindVar( "host_timescale" );
	ConVar *pNormalSpeed = cvar->FindVar( "hl2_normspeed" );
	if ( pScale ) pScale->SetValue( bt_enabled.GetBool() ? bt_timescale.GetFloat() : 1.0f );
	for ( int i = 1; i <= gpGlobals->maxClients; ++i )
	{
		CHL2_Player *pPlayer = dynamic_cast< CHL2_Player * >( UTIL_PlayerByIndex( i ) );
		if ( !pPlayer ) continue;
		pPlayer->SetMaxSpeed( bt_enabled.GetBool() ? bt_plr_speed.GetFloat() : ( pNormalSpeed ? pNormalSpeed->GetFloat() : 150.0f ) );
		pPlayer->EmitSound( bt_enabled.GetBool() ? "Player.bullettimestart" : "Player.bullettimeend" );
		engine->ClientCommand( pPlayer->edict(), bt_enabled.GetBool() ? "r_screenoverlay dev/bullettime" : "r_screenoverlay off" );
	}
}

void UH_BulletTimePlayerDied( CBasePlayer *pPlayer )
{
	if ( bt_enabled.GetBool() ) bt_enabled.SetValue( 0 );
}

void CHL2_Player::InputEnableBt( inputdata_t &inputdata ) { UH_SetBulletTime( true ); }
void CHL2_Player::InputDisableBt( inputdata_t &inputdata ) { UH_SetBulletTime( false ); }
