#ifndef VGUI2_TYPES_H
#define VGUI2_TYPES_H

#include "xash3d_types.h"

#ifdef __cplusplus
namespace vgui2
{

typedef unsigned int VPANEL;
typedef unsigned long HScheme;
typedef unsigned long HTexture;
typedef unsigned long HCursor;
typedef unsigned long HPanel;
typedef unsigned long HFont;
typedef int HContext;

#define INVALID_PANEL ((VPANEL)0xFFFFFFFF)
#define INVALID_HPANEL ((HPanel)0xFFFFFFFF)
#define INVALID_HFONT ((HFont)0)
#define DEFAULT_VGUI_CONTEXT ((HContext)~0)

class IBaseInterface
{
public:
    virtual ~IBaseInterface() {}
};

class KeyValues;

}

enum CreateInterfaceStatus
{
    IFACE_OK = 0,
    IFACE_FAILED = -1
};

#else

typedef unsigned int VPANEL;
typedef unsigned long HScheme;
typedef unsigned long HTexture;
typedef unsigned long HCursor;
typedef unsigned long HPanel;
typedef unsigned long HFont;
typedef int HContext;

#define INVALID_PANEL ((VPANEL)0xFFFFFFFF)
#define INVALID_HPANEL ((HPanel)0xFFFFFFFF)
#define INVALID_HFONT ((HFont)0)
#define DEFAULT_VGUI_CONTEXT ((HContext)~0)

enum CreateInterfaceStatus
{
    IFACE_OK = 0,
    IFACE_FAILED = -1
};

#endif

typedef void* (*CreateInterfaceFn)(const char *pName, int *pReturnCode);

#define CREATEINTERFACE_PROCNAME "CreateInterface"

#ifdef __cplusplus
extern "C" {
#endif

int VGui2_IsInitialized( void );
void VGui2_Init( void );
void VGui2_Shutdown( void );
void VGui2_Frame( void );
void VGui2_GetInterfaces( CreateInterfaceFn *pFactory );
CreateInterfaceFn VGui2_GetFactory( void );
void *VGui2_GetInterface( const char *pName, int *pReturnCode );

#ifdef __cplusplus
}
#endif

#endif // VGUI2_TYPES_H
