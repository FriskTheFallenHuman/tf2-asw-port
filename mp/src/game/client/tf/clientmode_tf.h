//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// Purpose: 
//
//=============================================================================

#ifndef TF_CLIENTMODE_H
#define TF_CLIENTMODE_H
#ifdef _WIN32
#pragma once
#endif

#include "clientmode_shared.h"
#include "tf_viewport.h"
#include "GameUI/igameui.h"

class CHudMenuEngyBuild;
class CHudMenuEngyDestroy;
class CHudMenuSpyDisguise;
class CTFFreezePanel;

#if defined( _X360 )
class CTFClientScoreBoardDialog;
#endif

class ClientModeTFNormal : public ClientModeShared 
{
DECLARE_CLASS( ClientModeTFNormal, ClientModeShared );

private:

// IClientMode overrides.
public:

					ClientModeTFNormal();
	virtual			~ClientModeTFNormal();

	virtual void	Init();
	virtual void	InitViewport();
	virtual void	Shutdown();

	virtual float	GetViewModelFOV( void );
	virtual bool	ShouldDrawViewModel();

	virtual void	LevelInit( const char *newmap );
	virtual void	LevelShutdown( void );

	int				GetDeathMessageStartHeight( void );

	virtual void	FireGameEvent( IGameEvent *event );
	virtual void	PostRenderVGui() {}

	virtual void	DoObjectMotionBlur( const CViewSetup *pSetup );
	virtual void	UpdatePostProcessingEffects();
	virtual void	Update( void );
	virtual void	DoPostScreenSpaceEffects( const CViewSetup *pSetup );
	virtual void	OnColorCorrectionWeightsReset( void );
	virtual float	GetColorCorrectionScale( void ) const { return 1.0f; }
	virtual void	ClearCurrentColorCorrection() { m_pCurrentColorCorrection = NULL; }

private:

	CHudMenuEngyBuild *m_pMenuEngyBuild;
	CHudMenuEngyDestroy *m_pMenuEngyDestroy;
	CHudMenuSpyDisguise *m_pMenuSpyDisguise;
	CTFFreezePanel		*m_pFreezePanel;
	IGameUI			*m_pGameUI;

	const C_PostProcessController *m_pCurrentPostProcessController;
	PostProcessParameters_t m_CurrentPostProcessParameters;
	PostProcessParameters_t m_LerpStartPostProcessParameters, m_LerpEndPostProcessParameters;
	CountdownTimer m_PostProcessLerpTimer;

	CHandle<C_ColorCorrection> m_pCurrentColorCorrection;

#if defined( _X360 )
	CTFClientScoreBoardDialog	*m_pScoreboard;
#endif

	friend class ClientModeTFFullscreen;
};

extern IClientMode *GetClientModeNormal();
extern ClientModeTFNormal* GetClientModeTFNormal();

#endif // TF_CLIENTMODE_H
