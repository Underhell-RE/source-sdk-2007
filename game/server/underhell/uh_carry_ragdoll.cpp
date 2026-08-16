#include "cbase.h"
#include "hl2_player.h"

#include "tier0/memdbgon.h"

static ConVar uh_bodymousedamper( "uh_bodymousedamper", "4", FCVAR_ARCHIVE );

void CHL2_Player::UH_BeginCarryRagdoll( CBaseEntity *pRagdoll )
{
	if ( !pRagdoll || m_hCarryingRagdoll.Get() == pRagdoll )
		return;

	m_hCarryingRagdoll = pRagdoll;
	m_flCarryingRagdollSavedSpeed = MaxSpeed();

	IPhysicsObject *pPhysics = pRagdoll->VPhysicsGetObject();
	if ( pPhysics && pPhysics->GetMass() > 10.0f )
		SetMaxSpeed( m_flCarryingRagdollSavedSpeed * ( 1.0f / 3.0f ) );
}

void CHL2_Player::UH_UpdateCarryRagdollWeight( void )
{
	CBaseEntity *pRagdoll = m_hCarryingRagdoll.Get();
	if ( pRagdoll && IsHoldingEntity( pRagdoll ) )
		return;

	if ( m_flCarryingRagdollSavedSpeed > 0.0f )
		SetMaxSpeed( m_flCarryingRagdollSavedSpeed );
	m_hCarryingRagdoll = NULL;
	m_flCarryingRagdollSavedSpeed = 0.0f;
}
