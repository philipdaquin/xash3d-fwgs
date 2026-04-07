/*
 vgui2_host.cpp - VGUI2 host implementation for Xash3D FWGS
 Phase 1: Bootstrap for VGUI2-capable clients
 Phase 2: Real runtime implementation with visible rendering proof
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

// Test panel for rendering proof (created once)
static vgui2::VPANEL s_testPanel = 0;
static qboolean s_testPanelCreated = false;

// Test injection cvar
static convar_t *vgui2_test = NULL;

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

    // Register test cvar
    vgui2_test = Cvar_Get( "vgui2_test", "1", FCVAR_ARCHIVE, "enable VGUI2 rendering test (rect + text)" );

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

    vgui2::IVGui *ivgui = g_VGui2Interfaces.GetIVGui();

    // Free test panel
    if( s_testPanel != 0 && ivgui )
    {
        ivgui->MarkPanelForDeletion( s_testPanel );
        s_testPanel = 0;
        s_testPanelCreated = false;
    }

    // Free root panel
    if( s_rootPanel != 0 )
    {
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
    vgui2::IPanel *ipanel = g_VGui2Interfaces.GetIPanel();
    vgui2::ISurface *isurface = g_VGui2Interfaces.GetISurface();

    if( ivgui )
        ivgui->RunFrame();

    if( !ipanel || !isurface )
        return;

    // Create test panel once (on first frame)
    if( !s_testPanelCreated && s_rootPanel != 0 )
    {
        s_testPanel = ivgui->AllocPanel();
        if( s_testPanel != 0 )
        {
            ipanel->Init( s_testPanel, NULL );
            ipanel->SetPos( s_testPanel, 100, 100 );
            ipanel->SetSize( s_testPanel, 200, 100 );
            ipanel->SetVisible( s_testPanel, true );
            ipanel->SetParent( s_testPanel, s_rootPanel );
            s_testPanelCreated = true;
            Con_Reportf( "VGUI2: Created test panel=%d\n", (int)s_testPanel );
        }
    }

    // 2. SolveTraverse then PaintTraverse in correct order
    if( s_rootPanel != 0 )
    {
        // Solve absolute positions FIRST
        isurface->SolveTraverse( s_rootPanel, false );
        
        // THEN paint
        isurface->PaintTraverse( s_rootPanel );
    }

    // 3. Direct test injection (red) - only if vgui2_test is enabled
    // Offset to right of green traversal rect so both are visible
    if( vgui2_test && vgui2_test->value && s_testPanel != 0 )
    {
        // Draw a visible test rectangle (red) - offset X by 220 to not overlap green
        isurface->DrawSetColor( 255, 0, 0, 255 );
        isurface->DrawFilledRect( 320, 100, 520, 200 );

        // Draw test text "VGUI2 OK" (direct)
        isurface->DrawSetTextColor( 255, 255, 255, 255 );
        isurface->DrawSetTextPos( 330, 110 );
        
        wchar_t testText[] = L"VGUI2 OK";
        isurface->DrawPrintText( testText, wcslen( testText ) );

        // Draw second text line lower
        isurface->DrawSetTextPos( 330, 130 );
        wchar_t testText2[] = L"RECTANGLE";
        isurface->DrawPrintText( testText2, wcslen( testText2 ) );
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
