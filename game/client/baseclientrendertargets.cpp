//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Implementation for CBaseClientRenderTargets class.
//			Provides Init functions for common render textures used by the engine.
//			Mod makers can inherit from this class, and call the Create functions for
//			only the render textures the want for their mod.
//=============================================================================//

#include "cbase.h"
#include "baseclientrendertargets.h"						// header	
#include "materialsystem/ITexture.h"
#include "materialsystem/imaterialsystemhardwareconfig.h"	// Hardware config checks

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
	// sub_101299B0 calls IMaterialSystem vtable slot 84 (Ex2) with size mode
	// 5 (RT_SIZE_OFFSCREEN). Its recovered call supplies only name, dimensions,
	// mode and format; the remaining arguments are defaults, including RT flags
	// 0. Marking these camera textures HDR produced the solid brown/grey sample.
	return pMaterialSystem->CreateNamedRenderTargetTextureEx2(
		name, size, size, RT_SIZE_OFFSCREEN,
		pMaterialSystem->GetBackBufferFormat(), MATERIAL_RT_DEPTH_SHARED,
		TEXTUREFLAGS_CLAMPS | TEXTUREFLAGS_CLAMPT,
		0 );
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

	for ( int i = 0; i < 4; ++i )
	{
		ITexture *pTexture = m_CustomCameraTexture[i];
		Msg( "[UH render] allocated custom camera %d ptr=%p name=%s size=%dx%d error=%d\n",
			i + 1, pTexture, pTexture ? pTexture->GetName() : "<null>",
			pTexture ? pTexture->GetActualWidth() : 0,
			pTexture ? pTexture->GetActualHeight() : 0,
			pTexture ? IsErrorTexture( pTexture ) : 1 );
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
		m_CustomCameraTexture[i].Shutdown();
}

ITexture *CBaseClientRenderTargets::GetCustomCameraTextureByIndex( int index )
{
	if ( index < 1 || index > 4 )
		return NULL;
	return m_CustomCameraTexture[index - 1];
}

static CBaseClientRenderTargets g_BaseClientRenderTargets;

ITexture *GetAllocatedCustomCameraTexture( int index )
{
	return g_BaseClientRenderTargets.GetCustomCameraTextureByIndex( index );
}

IClientRenderTargets *g_pClientRenderTargets = &g_BaseClientRenderTargets;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CBaseClientRenderTargets, IClientRenderTargets,
	CLIENTRENDERTARGETS_INTERFACE_VERSION, g_BaseClientRenderTargets );
