#ifndef VGUI2_INTERFACES_H
#define VGUI2_INTERFACES_H

#include "vgui2_shared_interfaces.h"

#define FILESYSTEM_INTERFACE_VERSION "VFileSystem009"
#define IBASEUI_NAME "BaseUI001"
#define VENGINE_VGUI_VERSION "VEngineVGui001"
#define IGAMEUIFUNCS_NAME "VENGINE_GAMEUIFUNCS_VERSION001"

class CVGui2Interfaces
{
public:
	CVGui2Interfaces() : m_bInitialized( false ), m_pIVGui( NULL ), m_pIPanel( NULL ), m_pISurface( NULL ), m_pIInputInternal( NULL ), m_pISchemeManager( NULL ), m_pILocalize( NULL ), m_pISystem( NULL ) {}

	void Init();
	void Shutdown();
	void RunFrame();

	void *CreateInterface( const char *pName, int *pReturnCode );

	vgui2::IVGui *GetIVGui() { return m_pIVGui; }
	vgui2::IPanel *GetIPanel() { return m_pIPanel; }
	vgui2::ISurface *GetISurface() { return m_pISurface; }
	vgui2::IInputInternal *GetIInputInternal() { return m_pIInputInternal; }
	vgui2::ISchemeManager *GetISchemeManager() { return m_pISchemeManager; }
	vgui2::ILocalize *GetILocalize() { return m_pILocalize; }
	vgui2::ISystem *GetISystem() { return m_pISystem; }

private:
	bool m_bInitialized;

	vgui2::IVGui *m_pIVGui;
	vgui2::IPanel *m_pIPanel;
	vgui2::ISurface *m_pISurface;
	vgui2::IInputInternal *m_pIInputInternal;
	vgui2::ISchemeManager *m_pISchemeManager;
	vgui2::ILocalize *m_pILocalize;
	vgui2::ISystem *m_pISystem;
};

#endif // VGUI2_INTERFACES_H
