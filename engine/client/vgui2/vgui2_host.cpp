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

struct CVGui2Interfaces *VGui2_GetInterfacesClient(void)
{
    return &g_VGui2Interfaces;
}

// Root embedded panel
static vgui2::VPANEL s_rootPanel = 0;

// Test panel for rendering proof (created once)
static vgui2::VPANEL s_testPanel = 0;
static qboolean s_testPanelCreated = false;

// Test injection cvar - registered once at init, pointer stored for runtime checks
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

extern "C" EXPORT void VGui2_Init( void )
{
    if( g_iVGui2Initialized )
        return;

    g_VGui2Interfaces.Init();

    // Register test cvar
    vgui2_test = Cvar_Get( "vgui2_test", "1", FCVAR_ARCHIVE, "enable VGUI2 rendering test (rect + text)" );

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

extern "C" EXPORT void VGui2_Shutdown( void )
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

extern "C" EXPORT void VGui2_Frame( void )
{
    if( !g_iVGui2Initialized )
    {
        Con_Printf( S_NOTE "VGUI2: Not initialized, returning\n" );
        return;
    }

    // 1. RunFrame - processes deletion queue
    vgui2::IVGui *ivgui = g_VGui2Interfaces.GetIVGui();
    vgui2::IPanel *ipanel = g_VGui2Interfaces.GetIPanel();
    vgui2::ISurface *isurface = g_VGui2Interfaces.GetISurface();

    if( ivgui )
        ivgui->RunFrame();

    if( !ipanel || !isurface )
        return;

    // DEBUG: Confirm we're in the right frame
    // Con_Printf( S_NOTE "VGUI2: frame running, rootPanel=%d\n", (int)s_rootPanel );

    // =============================================================
    // DEBUG PASS: Direct unclipped rectangle draw
    // Purpose: Test if ref.dllFuncs.FillRGBA works without panel traversal
    // =============================================================

    // Create test panel once (on first frame) - skip if already created
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
            Con_Printf( S_NOTE "VGUI2: Created test panel=%d\n", (int)s_testPanel );
        }
    }

    // Bypass panel traversal entirely - draw DIRECTLY with FillRGBA
    // Large bright red rectangle at (50, 50) size 500x300
    // TEMPORARY DEBUG: Direct FillRGBA call with NO clipping/scissor
    // Using bright red (255,0,0) at a visible screen position
    // Coordinates: x=50, y=50, w=500, h=300 - should be unmistakable
    ref.dllFuncs.FillRGBA( kRenderTransTexture, 50.0f, 50.0f, 500.0f, 300.0f, 255, 0, 0, 255 );

    // =============================================================
    // END DEBUG PASS
    // =============================================================

    // 2. Panel traversal (normal VGUI2 path) - commented out for debug
    /*
    if( s_rootPanel == 0 )
    {
        Con_Printf( S_NOTE "VGUI2: ABORT - s_rootPanel is 0!\n" );
    }
    else
    {
        // Solve absolute positions FIRST
        isurface->SolveTraverse( s_rootPanel, false );

        // THEN paint
        isurface->PaintTraverse( s_rootPanel );
    }
    */

    // 3. Test injection (red rect via surface interface) - only if vgui2_test is enabled
    // Con_Printf( S_NOTE "VGUI2: vgui2_test=%p, value=%.1f\n",
    //     vgui2_test, vgui2_test ? vgui2_test->value : -1 );

    if( vgui2_test && vgui2_test->value && s_testPanel != 0 )
    {
        // This uses the VGUI2 surface interface (not direct FillRGBA)
        isurface->DrawSetColor( 0, 255, 0, 255 );
        isurface->DrawFilledRect( 50, 400, 250, 500 ); // green rect below red one

        // Text is stubbed - won't render
        isurface->DrawSetTextColor( 255, 255, 255, 255 );
        isurface->DrawSetTextPos( 60, 410 );
        wchar_t testText[] = L"VGUI2 TEST";
        isurface->DrawPrintText( testText, wcslen( testText ) );
    }
}

extern "C" EXPORT void VGui2_GetInterfaces( CreateInterfaceFn *pFactory )
{
    *pFactory = VGui2_CreateInterface;
}

extern "C" EXPORT CreateInterfaceFn VGui2_GetFactory( void )
{
    return VGui2_CreateInterface;
}

extern "C" EXPORT int VGui2_IsInitialized( void )
{
    return g_iVGui2Initialized;
}

extern "C" EXPORT void *VGui2_GetInterface( const char *pName, int *pReturnCode )
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
