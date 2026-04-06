/*
 vgui2_host.cpp - VGUI2 host implementation for Xash3D FWGS
 Phase 1: Bootstrap for VGUI2-capable clients
 Phase 2: Real runtime implementation
 */

#include "vgui2_host.h"
#include "vgui2_interfaces.h"
#include "common.h"
#include "client.h"
#include "ref_api.h"

int g_iVGui2Initialized = 0;

static CVGui2Interfaces g_VGui2Interfaces;

// Root embedded panel
static vgui2::VPANEL s_rootPanel = 0;

static void *VGui2_CreateInterface( const char *pName, int *pReturnCode )
{
    if( pReturnCode )
        *pReturnCode = IFACE_OK;

    void *pInterface = g_VGui2Interfaces.CreateInterface( pName, pReturnCode );

    if( pInterface )
        return pInterface;

    if( pReturnCode )
        *pReturnCode = IFACE_FAILED;

    return NULL;
}

void VGui2_Init( void )
{
    if( g_iVGui2Initialized )
        return;

    g_VGui2Interfaces.Init();

    // Get interfaces for root panel creation
    vgui2::IVGui *ivgui = g_VGui2Interfaces.GetIVGui();
    vgui2::IPanel *ipanel = g_VGui2Interfaces.GetIPanel();
    vgui2::ISurface *isurface = g_VGui2Interfaces.GetISurface();

    if( ivgui && ipanel && isurface )
    {
        // Create root embedded panel
        s_rootPanel = ivgui->AllocPanel();
        
        if( s_rootPanel != 0 )
        {
            // Initialize panel
            ipanel->Init( s_rootPanel, NULL );
            
            // Set root panel to full screen
            ipanel->SetPos( s_rootPanel, 0, 0 );
            ipanel->SetSize( s_rootPanel, refState.width, refState.height );
            ipanel->SetVisible( s_rootPanel, true );
            ipanel->SetParent( s_rootPanel, 0 );
            
            // Set as embedded panel
            isurface->SetEmbeddedPanel( s_rootPanel );
            
            Con_Reportf( "VGUI2: Initialized with real runtime, root panel=%d\n", (int)s_rootPanel );
        }
        else
        {
            Con_Reportf( "VGUI2: WARNING - failed to create root panel\n" );
        }
    }
    else
    {
        Con_Reportf( "VGUI2: WARNING - failed to get VGUI2 interfaces\n" );
    }

    g_iVGui2Initialized = 1;
}

void VGui2_Shutdown( void )
{
    if( !g_iVGui2Initialized )
        return;

    // Free root panel
    if( s_rootPanel != 0 )
    {
        vgui2::IVGui *ivgui = g_VGui2Interfaces.GetIVGui();
        if( ivgui )
        {
            ivgui->MarkPanelForDeletion( s_rootPanel );
            ivgui->RunFrame(); // Drain deletion queue
        }
        s_rootPanel = 0;
    }

    g_VGui2Interfaces.Shutdown();
    g_iVGui2Initialized = 0;

    Con_Reportf( "VGUI2: Shutdown\n" );
}

void VGui2_Frame( void )
{
    if( !g_iVGui2Initialized )
        return;

    // 1. RunFrame - processes deletion queue
    vgui2::IVGui *ivgui = g_VGui2Interfaces.GetIVGui();
    if( ivgui )
        ivgui->RunFrame();

    // 2. SolveTraverse then PaintTraverse in correct order
    vgui2::ISurface *isurface = g_VGui2Interfaces.GetISurface();
    if( s_rootPanel != 0 && isurface )
    {
        // Solve absolute positions FIRST
        isurface->SolveTraverse( s_rootPanel, false );
        
        // THEN paint
        isurface->PaintTraverse( s_rootPanel );
    }
}

void VGui2_GetInterfaces( CreateInterfaceFn *pFactory )
{
    *pFactory = VGui2_CreateInterface;
}

CreateInterfaceFn VGui2_GetFactory( void )
{
    return VGui2_CreateInterface;
}

int VGui2_IsInitialized( void )
{
    return g_iVGui2Initialized;
}

void *VGui2_GetInterface( const char *pName, int *pReturnCode )
{
    return VGui2_CreateInterface( pName, pReturnCode );
}

void VGui2_OnResize( int width, int height )
{
    if( !g_iVGui2Initialized || s_rootPanel == 0 )
        return;

    vgui2::IPanel *ipanel = g_VGui2Interfaces.GetIPanel();

    if( ipanel )
    {
        ipanel->SetSize( s_rootPanel, width, height );
        ipanel->SetPos( s_rootPanel, 0, 0 );
        ipanel->Repaint( s_rootPanel );
    }

    Con_Reportf( "VGUI2: Resize to %dx%d\n", width, height );
}
