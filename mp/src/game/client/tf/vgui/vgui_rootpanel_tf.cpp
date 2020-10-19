//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "vgui_rootpanel_tf.h"
#include "vgui_int.h"
#include "ienginevgui.h"
#include "vgui_controls/Panel.h"
#include "vgui/IVgui.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

using namespace vgui;

//-----------------------------------------------------------------------------
// C_TFRootPanel implementation.
//-----------------------------------------------------------------------------
C_TFRootPanel::C_TFRootPanel( vgui::VPANEL parent, int slot )
	: BaseClass( NULL, "TF Root Panel" ), m_nSplitSlot( slot )
{
	SetParent( parent );
	SetPaintEnabled( false );
	SetPaintBorderEnabled( false );
	SetPaintBackgroundEnabled( false );

	// This panel does post child painting
	SetPostChildPaintEnabled( true );

	// Make it screen sized
	SetBounds( 0, 0, ScreenWidth(), ScreenHeight() );

	// Ask for OnTick messages
	vgui::ivgui()->AddTickSignal( GetVPanel() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void C_TFRootPanel::PostChildPaint()
{
	BaseClass::PostChildPaint();

	// Draw all panel effects
	RenderPanelEffects();
}

void C_TFRootPanel::PaintTraverse( bool Repaint, bool allowForce /*= true*/ )
{
	ACTIVE_SPLITSCREEN_PLAYER_GUARD( m_nSplitSlot);
	BaseClass::PaintTraverse( Repaint, allowForce );
}

void C_TFRootPanel::OnThink()
{
	ACTIVE_SPLITSCREEN_PLAYER_GUARD( m_nSplitSlot );
	BaseClass::OnThink();
}

static C_TFRootPanel *g_pRootPanel[ MAX_SPLITSCREEN_PLAYERS ];
static C_TFRootPanel *g_pFullscreenRootPanel;

void VGui_GetPanelList( CUtlVector< Panel * > &list )
{
	for ( int i = 0 ; i < MAX_SPLITSCREEN_PLAYERS; ++i )
		list.AddToTail( g_pRootPanel[ i ] );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void VGUI_CreateClientDLLRootPanel( void )
{
	for ( int i = 0 ; i < MAX_SPLITSCREEN_PLAYERS; ++i )
		g_pRootPanel[ i ] = new C_TFRootPanel( enginevgui->GetPanel( PANEL_CLIENTDLL ), i );

	g_pFullscreenRootPanel = new C_TFRootPanel( enginevgui->GetPanel( PANEL_CLIENTDLL ), 0 );
	g_pFullscreenRootPanel->SetZPos( 1 );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void VGUI_DestroyClientDLLRootPanel( void )
{
	for ( int i = 0 ; i < MAX_SPLITSCREEN_PLAYERS; ++i )
	{
		delete g_pRootPanel[ i ];
		g_pRootPanel[ i ] = NULL;
	}

	delete g_pFullscreenRootPanel;
	g_pFullscreenRootPanel = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Game specific root panel
// Output : vgui::Panel
//-----------------------------------------------------------------------------
vgui::VPANEL VGui_GetClientDLLRootPanel( void )
{
	ASSERT_LOCAL_PLAYER_RESOLVABLE();
	return g_pRootPanel[ GET_ACTIVE_SPLITSCREEN_SLOT() ]->GetVPanel();
}

//-----------------------------------------------------------------------------
// Purpose: Fullscreen root panel for shared hud elements during splitscreen
// Output : vgui::Panel
//-----------------------------------------------------------------------------
vgui::Panel *VGui_GetFullscreenRootPanel( void )
{
	return g_pFullscreenRootPanel;
}

vgui::VPANEL VGui_GetFullscreenRootVPANEL( void )
{
	return g_pFullscreenRootPanel->GetVPanel();
}