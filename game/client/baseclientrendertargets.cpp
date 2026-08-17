//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Implementation for CBaseClientRenderTargets class.
//			Provides Init functions for common render textures used by the engine.
//			Mod makers can inherit from this class, and call the Create functions for
//			only the render textures the want for their mod.
//=============================================================================//

#include "cbase.h"
#include "baseclientrendertargets.h"						// header	
#include "materialsystem/imaterialsystemhardwareconfig.h"	// Hardware config checks
#include "materialsystem/imaterial.h"
#include "tier1/keyvalues.h"

ITexture* CBaseClientRenderTargets::CreateWaterReflectionTexture( IMaterialSystem* pMaterialSystem, int iSize )
{
	return pMaterialSystem->CreateNamedRenderTargetTextureEx2(
		"_rt_WaterReflection",
		iSize, iSize, RT_SIZE_PICMIP,
		pMaterialSystem->GetBackBufferFormat(), 
		MATERIAL_RT_DEPTH_SHARED, 
		TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT,
		CREATERENDERTARGETFLAGS_HDR );
}

ITexture* CBaseClientRenderTargets::CreateWaterRefractionTexture( IMaterialSystem* pMaterialSystem, int iSize )
{
	return pMaterialSystem->CreateNamedRenderTargetTextureEx2(
		"_rt_WaterRefraction",
		iSize, iSize, RT_SIZE_PICMIP,
		// This is different than reflection because it has to have alpha for fog factor.
		IMAGE_FORMAT_RGBA8888, 
		MATERIAL_RT_DEPTH_SHARED, 
		TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT,
		CREATERENDERTARGETFLAGS_HDR );
}

ITexture* CBaseClientRenderTargets::CreateCameraTexture( IMaterialSystem* pMaterialSystem, int iSize )
{
	return pMaterialSystem->CreateNamedRenderTargetTextureEx2(
		"_rt_Camera",
		iSize, iSize, RT_SIZE_DEFAULT,
		pMaterialSystem->GetBackBufferFormat(),
		MATERIAL_RT_DEPTH_SHARED, 
		0,
		CREATERENDERTARGETFLAGS_HDR );
}

ITexture* CBaseClientRenderTargets::CreateCustomCameraTexture( IMaterialSystem* pMaterialSystem, int index, int size )
{
	char name[64];
	Q_snprintf( name, sizeof( name ), "_rt_CustomCamera_%d", index );
	return pMaterialSystem->CreateNamedRenderTargetTextureEx2(
		name, size, size, RT_SIZE_DEFAULT,
		pMaterialSystem->GetBackBufferFormat(), MATERIAL_RT_DEPTH_SHARED,
		TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT,
		CREATERENDERTARGETFLAGS_HDR );
}

//-----------------------------------------------------------------------------
// Purpose: Called by the engine in material system init and shutdown.
//			Clients should override this in their inherited version, but the base
//			is to init all standard render targets for use.
// Input  : pMaterialSystem - the engine's material system (our singleton is not yet inited at the time this is called)
//			pHardwareConfig - the user hardware config, useful for conditional render target setup
//-----------------------------------------------------------------------------
void CBaseClientRenderTargets::InitClientRenderTargets( IMaterialSystem* pMaterialSystem, IMaterialSystemHardwareConfig* pHardwareConfig )
{
	// Water effects
	m_WaterReflectionTexture.Init( CreateWaterReflectionTexture( pMaterialSystem, 1024 ) );
	m_WaterRefractionTexture.Init( CreateWaterRefractionTexture( pMaterialSystem, 1024 ) );

	// Monitors
	m_CameraTexture.Init( CreateCameraTexture( pMaterialSystem, 256 ) );
	m_CustomCameraTexture[0].Init( CreateCustomCameraTexture( pMaterialSystem, 1, 256 ) );
	m_CustomCameraTexture[1].Init( CreateCustomCameraTexture( pMaterialSystem, 2, 256 ) );
	m_CustomCameraTexture[2].Init( CreateCustomCameraTexture( pMaterialSystem, 3, 512 ) );
	m_CustomCameraTexture[3].Init( CreateCustomCameraTexture( pMaterialSystem, 4, 512 ) );

	static const char *materialNames[4] =
	{
		"dev/dev_monitor_256a", "dev/dev_monitor_256b",
		"dev/dev_monitor_512a", "dev/dev_monitor_512b"
	};
	for ( int i = 0; i < 4; ++i )
	{
		char textureName[64];
		Q_snprintf( textureName, sizeof( textureName ), "_rt_CustomCamera_%d", i + 1 );
		KeyValues *vmt = new KeyValues( "UnlitGeneric" );
		vmt->SetString( "$basetexture", textureName );
		vmt->SetInt( "$vertexcolor", 1 );
		vmt->SetInt( "$vertexalpha", 1 );
		vmt->SetString( "$surfaceprop", "glass" );
		m_pCustomCameraMaterial[i] = pMaterialSystem->CreateMaterial( materialNames[i], vmt );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Shut down each CTextureReference we created in InitClientRenderTargets.
//			Called by the engine in material system shutdown.
// Input  :  - 
//-----------------------------------------------------------------------------
void CBaseClientRenderTargets::ShutdownClientRenderTargets()
{
	// Water effects
	m_WaterReflectionTexture.Shutdown();
	m_WaterRefractionTexture.Shutdown();

	// Monitors
	m_CameraTexture.Shutdown();
	for ( int i = 0; i < 4; ++i )
	{
		m_CustomCameraTexture[i].Shutdown();
		if ( m_pCustomCameraMaterial[i] )
		{
			m_pCustomCameraMaterial[i]->DecrementReferenceCount();
			m_pCustomCameraMaterial[i] = NULL;
		}
	}
}

static CBaseClientRenderTargets g_BaseClientRenderTargets;
IClientRenderTargets *g_pClientRenderTargets = &g_BaseClientRenderTargets;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CBaseClientRenderTargets, IClientRenderTargets,
	CLIENTRENDERTARGETS_INTERFACE_VERSION, g_BaseClientRenderTargets );
