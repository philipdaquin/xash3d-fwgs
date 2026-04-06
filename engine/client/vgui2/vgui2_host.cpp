/*
 vgui2_host.cpp - VGUI2 host implementation for Xash3D FWGS
 Phase 1: Bootstrap for VGUI2-capable clients
 */

#include "vgui2_host.h"
#include "vgui2_interfaces.h"
#include "common.h"

int g_iVGui2Initialized = 0;

static CVGui2Interfaces g_VGui2Interfaces;

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

    g_iVGui2Initialized = 1;

    Con_Reportf( "VGUI2: Initialized (stub mode)\n" );
}

void VGui2_Shutdown( void )
{
    if( !g_iVGui2Initialized )
        return;

    g_VGui2Interfaces.Shutdown();
    g_iVGui2Initialized = 0;

    Con_Reportf( "VGUI2: Shutdown\n" );
}

void VGui2_Frame( void )
{
    if( !g_iVGui2Initialized )
        return;

    g_VGui2Interfaces.RunFrame();
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