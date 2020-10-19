//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// $Header: $
// $NoKeywords: $
//
//=============================================================================
#include "cbase.h"
#include "hud.h"
#include "clientmode_tf.h"
#include "cdll_client_int.h"
#include "iinput.h"
#include "vgui/isurface.h"
#include "vgui/ipanel.h"
#include "GameUI/igameui.h"
#include <vgui_controls/AnimationController.h>
#include "ivmodemanager.h"
#include "BuyMenu.h"
#include "filesystem.h"
#include "vgui/ivgui.h"
#include "hud_chat.h"
#include "view_shared.h"
#include "view.h"
#include "ivrenderview.h"
#include "model_types.h"
#include "iefx.h"
#include "dlight.h"
#include <imapoverview.h>
#include "c_playerresource.h"
#include <keyvalues.h>
#include "text_message.h"
#include "panelmetaclassmgr.h"
#include "c_tf_player.h"
#include "ienginevgui.h"
#include "in_buttons.h"
#include "voice_status.h"
#include "tf_hud_menu_engy_build.h"
#include "tf_hud_menu_engy_destroy.h"
#include "tf_hud_menu_spy_disguise.h"
#include "tf_statsummary.h"
#include "tf_hud_freezepanel.h"
#include "viewpostprocess.h"
#include "shaderapi/ishaderapi.h"
#include "tier2/renderutils.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "engine/IEngineSound.h"
#include "object_motion_blur_effect.h"
#include "ivieweffects.h"
#include "glow_outline_effect.h"
#include "c_tf_player.h"
#include "tf_weaponbase.h"

#if defined( _X360 )
#include "tf_clientscoreboard.h"
#endif

ConVar default_fov( "default_fov", "75", FCVAR_CHEAT );

void HUDMinModeChangedCallBack( IConVar *var, const char *pOldString, float flOldValue )
{
	engine->ExecuteClientCmd( "hud_reloadscheme" );
}
ConVar cl_hud_minmode( "cl_hud_minmode", "0", FCVAR_ARCHIVE, "Set to 1 to turn on the advanced minimalist HUD mode.", HUDMinModeChangedCallBack );

extern ConVar mat_object_motion_blur_enable;

//--------------------------------------------------------------------------------------------------------
static IClientMode *g_pClientMode[ MAX_SPLITSCREEN_PLAYERS ];
IClientMode *GetClientMode()
{
	Assert( engine->IsLocalPlayerResolvable() );
	return g_pClientMode[ engine->GetActiveSplitScreenPlayerSlot() ];
}

//--------------------------------------------------------------------------------------------------------
ClientModeTFNormal g_ClientModeNormal[ MAX_SPLITSCREEN_PLAYERS ];
IClientMode *GetClientModeNormal()
{
	Assert( engine->IsLocalPlayerResolvable() );
	return &g_ClientModeNormal[ engine->GetActiveSplitScreenPlayerSlot() ];
}

//--------------------------------------------------------------------------------------------------------
ClientModeTFNormal* GetClientModeTFNormal()
{
	Assert( engine->IsLocalPlayerResolvable() );
	return &g_ClientModeNormal[ engine->GetActiveSplitScreenPlayerSlot() ];
}

// Voice data
void VoxCallback( IConVar *var, const char *oldString, float oldFloat )
{
	if ( engine && engine->IsConnected() )
	{
		ConVarRef voice_vox( var->GetName() );
		if ( voice_vox.GetBool() /*&& voice_modenable.GetBool()*/ )
		{
			engine->ClientCmd( "voicerecord_toggle on\n" );
		}
		else
		{
			engine->ClientCmd( "voicerecord_toggle off\n" );
		}
	}
}
ConVar voice_vox( "voice_vox", "0", FCVAR_ARCHIVE, "Voice chat uses a vox-style always on", false, 0, true, 1, VoxCallback );

//--------------------------------------------------------------------------------------------------------
class CVoxManager : public CAutoGameSystem
{
public:
	CVoxManager() : CAutoGameSystem( "VoxManager" ) { }

	virtual void LevelInitPostEntity( void )
	{
		if ( voice_vox.GetBool() /*&& voice_modenable.GetBool()*/ )
		{
			engine->ClientCmd( "voicerecord_toggle on\n" );
		}
	}


	virtual void LevelShutdownPreEntity( void )
	{
		if ( voice_vox.GetBool() )
		{
			engine->ClientCmd( "voicerecord_toggle off\n" );
		}
	}
};


//--------------------------------------------------------------------------------------------------------
static CVoxManager s_VoxManager;


// --------------------------------------------------------------------------------- //
// CTFModeManager.
// --------------------------------------------------------------------------------- //
class CTFModeManager : public IVModeManager
{
public:
	virtual void	Init();
	virtual void	SwitchMode( bool commander, bool force ) {}
	virtual void	LevelInit( const char *newmap );
	virtual void	LevelShutdown( void );
	virtual void	ActivateMouse( bool isactive ) {}
};

static CTFModeManager g_ModeManager;
IVModeManager *modemanager = ( IVModeManager * )&g_ModeManager;


// --------------------------------------------------------------------------------- //
// CTFModeManager implementation.
// --------------------------------------------------------------------------------- //

#define SCREEN_FILE		"scripts/vgui_screens.txt"

void CTFModeManager::Init()
{
	for( int i = 0; i < MAX_SPLITSCREEN_PLAYERS; ++i )
	{
		ACTIVE_SPLITSCREEN_PLAYER_GUARD( i );
		g_pClientMode[ i ] = GetClientModeNormal();
	}
	
	PanelMetaClassMgr()->LoadMetaClassDefinitionFile( SCREEN_FILE );

	// Load the objects.txt file.
	LoadObjectInfos( ::filesystem );

	GetClientVoiceMgr()->SetHeadLabelOffset( 40 );
}

void CTFModeManager::LevelInit( const char *newmap )
{
	for( int i = 0; i < MAX_SPLITSCREEN_PLAYERS; ++i )
	{
		ACTIVE_SPLITSCREEN_PLAYER_GUARD( i );
		GetClientMode()->LevelInit( newmap );
	}

	ConVarRef voice_steal( "voice_steal" );

	if ( voice_steal.IsValid() )
	{
		voice_steal.SetValue( 1 );
	}
}

void CTFModeManager::LevelShutdown( void )
{
	for( int i = 0; i < MAX_SPLITSCREEN_PLAYERS; ++i )
	{
		ACTIVE_SPLITSCREEN_PLAYER_GUARD( i );
		GetClientMode()->LevelShutdown();
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
ClientModeTFNormal::ClientModeTFNormal()
{
	m_pMenuEngyBuild = NULL;
	m_pMenuEngyDestroy = NULL;
	m_pMenuSpyDisguise = NULL;
	m_pGameUI = NULL;
	m_pFreezePanel = NULL;

#if defined( _X360 )
	m_pScoreboard = NULL;
#endif
}

//-----------------------------------------------------------------------------
// Purpose: If you don't know what a destructor is by now, you are probably going to get fired
//-----------------------------------------------------------------------------
ClientModeTFNormal::~ClientModeTFNormal()
{
}

// See interface.h/.cpp for specifics:  basically this ensures that we actually Sys_UnloadModule the dll and that we don't call Sys_LoadModule 
//  over and over again.
static CDllDemandLoader g_GameUI( "gameui" );

class FullscreenTFViewport : public TFViewport
{
private:
	DECLARE_CLASS_SIMPLE( FullscreenTFViewport, TFViewport );

private:
	virtual void InitViewportSingletons( void )
	{
		SetAsFullscreenViewportInterface();
	}
};

class ClientModeTFFullscreen : public	ClientModeTFNormal
{
	DECLARE_CLASS_SIMPLE( ClientModeTFFullscreen, ClientModeTFNormal );
public:
	virtual void InitViewport()
	{
		// Skip over BaseClass!!!
		BaseClass::InitViewport();

		m_pViewport = new FullscreenTFViewport();
		m_pViewport->Start( gameuifuncs, gameeventmanager );
	}

	virtual void Init()
	{
		m_pMenuEngyBuild = ( CHudMenuEngyBuild * )GET_HUDELEMENT( CHudMenuEngyBuild );
		Assert( m_pMenuEngyBuild );

		m_pMenuEngyDestroy = ( CHudMenuEngyDestroy * )GET_HUDELEMENT( CHudMenuEngyDestroy );
		Assert( m_pMenuEngyDestroy );

		m_pMenuSpyDisguise = ( CHudMenuSpyDisguise * )GET_HUDELEMENT( CHudMenuSpyDisguise );
		Assert( m_pMenuSpyDisguise );

		m_pFreezePanel = ( CTFFreezePanel * )GET_HUDELEMENT( CTFFreezePanel );
		Assert( m_pFreezePanel );

		CreateInterfaceFn gameUIFactory = g_GameUI.GetFactory();
		if ( gameUIFactory )
		{
			m_pGameUI = (IGameUI *) gameUIFactory(GAMEUI_INTERFACE_VERSION, NULL );
			if ( NULL != m_pGameUI )
			{
				// insert stats summary panel as the loading background dialog
				CTFStatsSummaryPanel *pPanel = GStatsSummaryPanel();
				pPanel->InvalidateLayout( false, true );
				pPanel->SetVisible( false );
				pPanel->MakePopup( false );
				m_pGameUI->SetLoadingBackgroundDialog( pPanel->GetVPanel() );
			}		
		}

#if defined( _X360 )
		m_pScoreboard = (CTFClientScoreBoardDialog *)( GetViewPortInterface()->FindPanelByName( PANEL_SCOREBOARD ) );
		Assert( m_pScoreboard );
#endif

		// Skip over BaseClass!!!
		BaseClass::Init();
	}

	void Shutdown()
	{
		DestroyStatsSummaryPanel();
	}

	// Purpose: See if hud elements want key input. Return 0 if the key is swallowed
	int	HudElementKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
	{
		// Let scoreboard handle input first because on X360 we need gamertags and
		// gamercards accessible at all times when gamertag is visible.
	#if defined( _X360 )
		if ( m_pScoreboard )
		{
			if ( !m_pScoreboard->HudElementKeyInput( down, keynum, pszCurrentBinding ) )
			{
				return 0;
			}
		}
	#endif

		// check for hud menus
		if ( m_pMenuEngyBuild )
		{
			if ( !m_pMenuEngyBuild->HudElementKeyInput( down, keynum, pszCurrentBinding ) )
				return 0;
		}

		if ( m_pMenuEngyDestroy )
		{
			if ( !m_pMenuEngyDestroy->HudElementKeyInput( down, keynum, pszCurrentBinding ) )
				return 0;
		}

		if ( m_pMenuSpyDisguise )
		{
			if ( !m_pMenuSpyDisguise->HudElementKeyInput( down, keynum, pszCurrentBinding ) )
				return 0;
		}

		if ( m_pFreezePanel )
			m_pFreezePanel->HudElementKeyInput( down, keynum, pszCurrentBinding );

		return BaseClass::HudElementKeyInput( down, keynum, pszCurrentBinding );
	}

	// Purpose: See if spectator input occurred. Return 0 if the key is swallowed.
	int HandleSpectatorKeyInput( int down, ButtonCode_t keynum, const char *pszCurrentBinding )
	{
#if defined( _X360 )
		// On X360 when we have scoreboard up in spectator menu we cannot
		// steal any input because gamertags must be selectable and gamercards
		// must be accessible.
		// We cannot rely on any keybindings in this case since user could have
		// remapped everything.
		if ( m_pScoreboard && m_pScoreboard->IsVisible() )
		{
			return 1;
		}
#endif

		return BaseClass::HandleSpectatorKeyInput( down, keynum, pszCurrentBinding );
	}
};

//--------------------------------------------------------------------------------------------------------
static ClientModeTFFullscreen g_FullscreenClientMode;
IClientMode *GetFullscreenClientMode( void )
{
	return &g_FullscreenClientMode;
}

void ClientModeTFNormal::Init()
{
	BaseClass::Init();

	gameeventmanager->AddListener( this, "game_newmap", false );
}

void ClientModeTFNormal::Shutdown()
{
	DestroyStatsSummaryPanel();
}

void ClientModeTFNormal::InitViewport()
{
	m_pViewport = new TFViewport();
	m_pViewport->Start( gameuifuncs, gameeventmanager );
}

extern ConVar v_viewmodel_fov;
float g_flViewModelFOV = 75;
float ClientModeTFNormal::GetViewModelFOV( void )
{
	return v_viewmodel_fov.GetFloat();
//	return g_flViewModelFOV;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool ClientModeTFNormal::ShouldDrawViewModel()
{
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer )
	{
		if ( pPlayer->m_Shared.InCond( TF_COND_ZOOMED ) )
			return false;
	}

	return true;
}

int ClientModeTFNormal::GetDeathMessageStartHeight( void )
{
	return m_pViewport->GetDeathMessageStartHeight();
}

void ClientModeTFNormal::LevelInit( const char *newmap )
{
	// reset ambient light
	static ConVarRef mat_ambient_light_r( "mat_ambient_light_r" );
	static ConVarRef mat_ambient_light_g( "mat_ambient_light_g" );
	static ConVarRef mat_ambient_light_b( "mat_ambient_light_b" );

	if ( mat_ambient_light_r.IsValid() )
		mat_ambient_light_r.SetValue( "0" );

	if ( mat_ambient_light_g.IsValid() )
		mat_ambient_light_g.SetValue( "0" );

	if ( mat_ambient_light_b.IsValid() )
		mat_ambient_light_b.SetValue( "0" );

	BaseClass::LevelInit( newmap );

	// clear any DSP effects
	CLocalPlayerFilter filter;
	enginesound->SetRoomType( filter, 0 );
	enginesound->SetPlayerDSP( filter, 0, true );
}

void ClientModeTFNormal::LevelShutdown( void )
{
	BaseClass::LevelShutdown();
}

void ClientModeTFNormal::FireGameEvent( IGameEvent *event )
{
	const char *eventname = event->GetName();

	if ( !eventname || !eventname[0] )
		return;

	if ( Q_strcmp( "game_newmap", eventname ) == 0 )
		engine->ClientCmd("exec newmapsettings\n");
	else if ( Q_strcmp( "player_changename", eventname ) == 0 )
		return; // server sends a colorized text string for this

	BaseClass::FireGameEvent( event );
}

void ClientModeTFNormal::Update( void )
{
	UpdatePostProcessingEffects();
}

void ClientModeTFNormal::DoPostScreenSpaceEffects( const CViewSetup *pSetup )
{
	CMatRenderContextPtr pRenderContext( materials );
	
	if ( mat_object_motion_blur_enable.GetBool() )
	{
		DoObjectMotionBlur( pSetup );
	}
	
	// Render object glows and selectively-bloomed objects
	g_GlowObjectManager.RenderGlowEffects( pSetup, GetSplitScreenPlayerSlot() );
}

void ClientModeTFNormal::OnColorCorrectionWeightsReset( void )
{
	C_ColorCorrection *pNewColorCorrection = NULL;
	C_ColorCorrection *pOldColorCorrection = m_pCurrentColorCorrection;
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer )
		pNewColorCorrection = pPlayer->GetActiveColorCorrection();

	// Only blend between environmental color corrections if there is no failure/infested-induced color correction
	if ( pNewColorCorrection != pOldColorCorrection )
	{
		if ( pOldColorCorrection )
			pOldColorCorrection->EnableOnClient( false );

		if ( pNewColorCorrection )
			pNewColorCorrection->EnableOnClient( true, pOldColorCorrection == NULL );

		m_pCurrentColorCorrection = pNewColorCorrection;
	}
}

void ClientModeTFNormal::DoObjectMotionBlur( const CViewSetup *pSetup )
{
	if ( g_ObjectMotionBlurManager.GetDrawableObjectCount() <= 0 )
		return;

	CMatRenderContextPtr pRenderContext( materials );

	ITexture *pFullFrameFB1 = materials->FindTexture( "_rt_FullFrameFB1", TEXTURE_GROUP_RENDER_TARGET );

	//
	// Render Velocities into a full-frame FB1
	//
	IMaterial *pGlowColorMaterial = materials->FindMaterial( "dev/glow_color", TEXTURE_GROUP_OTHER, true );
	
	pRenderContext->PushRenderTargetAndViewport();
	pRenderContext->SetRenderTarget( pFullFrameFB1 );
	pRenderContext->Viewport( 0, 0, pSetup->width, pSetup->height );

	// Red and Green are x- and y- screen-space velocities biased and packed into the [0,1] range.
	// A value of 127 gets mapped to 0, a value of 0 gets mapped to -1, and a value of 255 gets mapped to 1.
	//
	// Blue is set to 1 within the object's bounds and 0 outside, and is used as a mask to ensure that
	// motion blur samples only pull from the core object itself and not surrounding pixels (even though
	// the area being blurred is larger than the core object).
	//
	// Alpha is not used
	pRenderContext->ClearColor4ub( 127, 127, 0, 0 );
	// Clear only color, not depth & stencil
	pRenderContext->ClearBuffers( true, false, false );

	// Save off state
	Vector vOrigColor;
	render->GetColorModulation( vOrigColor.Base() );

	// Use a solid-color unlit material to render velocity into the buffer
	g_pStudioRender->ForcedMaterialOverride( pGlowColorMaterial );
	g_ObjectMotionBlurManager.DrawObjects();	
	g_pStudioRender->ForcedMaterialOverride( NULL );

	render->SetColorModulation( vOrigColor.Base() );
	
	pRenderContext->PopRenderTargetAndViewport();

	//
	// Render full-screen pass
	//
	IMaterial *pMotionBlurMaterial;
	IMaterialVar *pFBTextureVariable;
	IMaterialVar *pVelocityTextureVariable;
	bool bFound1 = false, bFound2 = false;

	// Make sure our render target of choice has the results of the engine post-process pass
	ITexture *pFullFrameFB = materials->FindTexture( "_rt_FullFrameFB", TEXTURE_GROUP_RENDER_TARGET );
	pRenderContext->CopyRenderTargetToTexture( pFullFrameFB );

	pMotionBlurMaterial = materials->FindMaterial( "effects/object_motion_blur", TEXTURE_GROUP_OTHER, true );
	pFBTextureVariable = pMotionBlurMaterial->FindVar( "$fb_texture", &bFound1, true );
	pVelocityTextureVariable = pMotionBlurMaterial->FindVar( "$velocity_texture", &bFound2, true );
	if ( bFound1 && bFound2 )
	{
		pFBTextureVariable->SetTextureValue( pFullFrameFB );
		
		pVelocityTextureVariable->SetTextureValue( pFullFrameFB1 );

		int nWidth, nHeight;
		pRenderContext->GetRenderTargetDimensions( nWidth, nHeight );

		pRenderContext->DrawScreenSpaceRectangle( pMotionBlurMaterial, 0, 0, nWidth, nHeight, 0.0f, 0.0f, nWidth - 1, nHeight - 1, nWidth, nHeight );
	}
}

void ClientModeTFNormal::UpdatePostProcessingEffects()
{
	C_PostProcessController *pNewPostProcessController = NULL;
	C_TFPlayer *pPlayer = C_TFPlayer::GetLocalTFPlayer();
	if ( pPlayer )
		pNewPostProcessController = pPlayer->GetActivePostProcessController();

	// Figure out new endpoints for parameter lerping
	if ( pNewPostProcessController != m_pCurrentPostProcessController )
	{
		m_LerpStartPostProcessParameters = m_CurrentPostProcessParameters;
		m_LerpEndPostProcessParameters = pNewPostProcessController ? pNewPostProcessController->m_PostProcessParameters : PostProcessParameters_t();
		m_pCurrentPostProcessController = pNewPostProcessController;

		float flFadeTime = pNewPostProcessController ? pNewPostProcessController->m_PostProcessParameters.m_flParameters[ PPPN_FADE_TIME ] : 0.0f;
		if ( flFadeTime <= 0.0f )
		{
			flFadeTime = 0.001f;
		}
		m_PostProcessLerpTimer.Start( flFadeTime );
	}

	// Lerp between start and end
	float flLerpFactor = 1.0f - m_PostProcessLerpTimer.GetRemainingRatio();
	for ( int nParameter = 0; nParameter < POST_PROCESS_PARAMETER_COUNT; ++ nParameter )
	{
		m_CurrentPostProcessParameters.m_flParameters[ nParameter ] = 
			Lerp( 
				flLerpFactor, 
				m_LerpStartPostProcessParameters.m_flParameters[ nParameter ], 
				m_LerpEndPostProcessParameters.m_flParameters[ nParameter ] );
	}
	SetPostProcessParams( &m_CurrentPostProcessParameters );
}