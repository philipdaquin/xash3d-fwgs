/*
 vgui2_interfaces.cpp - VGUI2 implementation for Xash3D FWGS
 Phase 2: Real runtime implementation with visible rendering
 */
#include "vgui2_interfaces.h"
#include "common.h"
#include "client.h"
#include "ref_common.h"
#include "VFileSystem009.h"

#include "../../../../hl1_source_sdk/public/Color.h"
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

#define CONPRINTF(...) Con_Reportf(__VA_ARGS__)

#define STUB_PRINTF(...) CONPRINTF("VGUI2 stub: %s:%d - ", __FILE__, __LINE__); CONPRINTF(__VA_ARGS__)

namespace vgui2
{

class KeyValues;

typedef int HKeySymbol;

class IKeyValues : public IBaseInterface
{
public:
    virtual void RegisterSizeofKeyValues( int size ) = 0;
    virtual void *AllocKeyValuesMemory( int size ) = 0;
    virtual void FreeKeyValuesMemory( void *pMem ) = 0;
    virtual HKeySymbol GetSymbolForString( const char *name ) = 0;
    virtual const char *GetStringForSymbol( HKeySymbol symbol ) = 0;
    virtual void GetLocalizedFromANSI( const char* ansi, wchar_t* outBuf, int unicodeBufferSizeInBytes ) = 0;
    virtual void GetANSIFromLocalized( const wchar_t* unicode, char* outBuf, int ansiBufferSizeInBytes ) = 0;
    virtual void AddKeyValuesToMemoryLeakList( void *pMem, HKeySymbol name ) = 0;
    virtual void RemoveKeyValuesFromMemoryLeakList( void *pMem ) = 0;
};

extern ILocalize *GetLocalizeImpl();
extern void SetLocalizeFileSystemImpl(IFileSystem *pFileSystem);

#define MAX_PANELS 1024
#define MAX_CHILDREN 32
#define MAX_VGUI2_FONTS 128
#define MAX_VGUI2_TEXTURES 256

struct FontData_t
{
    bool valid;
    int tall;
    int weight;
    int blur;
    int scanlines;
    int flags;
    int lowRange;
    int highRange;
    int charWidth;
    char name[64];
};

struct TextureData_t
{
    bool valid;
    int glTexnum;
    int wide;
    int tall;
    char name[128];
};

struct PanelData_t
{
    int pos[2];
    int size[2];
    int absPos[2];
    int insets[4];
    int zpos;
    bool visible;
    bool enabled;
    bool popup;
    bool needsSolve;
    bool keyboardInputEnabled;
    bool mouseInputEnabled;
    VPANEL parent;
    VPANEL children[MAX_CHILDREN];
    int childCount;
    vgui2::IClientPanel *clientPanel;
};

static PanelData_t s_panelData[MAX_PANELS];
static unsigned int s_panelCount = 0;
static int s_currentClip[4] = { 0, 0, 99999, 99999 };
static int s_clipStack[32][4];
static int s_clipStackDepth = 0;
static VPANEL s_embeddedPanel = 0;
static FontData_t s_fontData[MAX_VGUI2_FONTS];
static TextureData_t s_textureData[MAX_VGUI2_TEXTURES];
static int s_nextTextureId = 1;
static const char *KEYVALUES_INTERFACE_VERSION = "KeyValues003";

static IFileSystem *GetVFileSystem()
{
    return (IFileSystem *)FS_GetNativeObject(FILESYSTEM_INTERFACE_VERSION);
}

class CKeyValuesStub : public IKeyValues
{
public:
    void RegisterSizeofKeyValues( int size ) override
    {
        m_keyValueSize = size;
        Con_Reportf( "VGUI2: KeyValues RegisterSizeofKeyValues(%d)\n", size );
    }

    void *AllocKeyValuesMemory( int size ) override
    {
        if( size <= 0 )
            size = m_keyValueSize > 0 ? m_keyValueSize : 1;
        return malloc( (size_t)size );
    }

    void FreeKeyValuesMemory( void *pMem ) override
    {
        free( pMem );
    }

    HKeySymbol GetSymbolForString( const char *name ) override
    {
        const std::string key = name ? name : "";
        for( size_t i = 0; i < m_symbols.size(); ++i )
        {
            if( m_symbols[i] == key )
                return (HKeySymbol)( i + 1 );
        }

        m_symbols.push_back( key );
        return (HKeySymbol)m_symbols.size();
    }

    const char *GetStringForSymbol( HKeySymbol symbol ) override
    {
        const int index = symbol - 1;
        if( index < 0 || index >= (int)m_symbols.size() )
            return "";
        return m_symbols[index].c_str();
    }

    void GetLocalizedFromANSI( const char* ansi, wchar_t* outBuf, int unicodeBufferSizeInBytes ) override
    {
        if( !outBuf || unicodeBufferSizeInBytes <= 0 )
            return;

        const int wcharCount = unicodeBufferSizeInBytes / (int)sizeof( wchar_t );
        if( wcharCount <= 0 )
            return;

        int i = 0;
        if( ansi )
        {
            for( ; i < wcharCount - 1 && ansi[i]; ++i )
                outBuf[i] = (unsigned char)ansi[i];
        }
        outBuf[i] = L'\0';
    }

    void GetANSIFromLocalized( const wchar_t* unicode, char* outBuf, int ansiBufferSizeInBytes ) override
    {
        if( !outBuf || ansiBufferSizeInBytes <= 0 )
            return;

        int i = 0;
        if( unicode )
        {
            for( ; i < ansiBufferSizeInBytes - 1 && unicode[i]; ++i )
                outBuf[i] = ( unicode[i] >= 0 && unicode[i] <= 0x7f ) ? (char)unicode[i] : '?';
        }
        outBuf[i] = '\0';
    }

    void AddKeyValuesToMemoryLeakList( void *, HKeySymbol ) override {}
    void RemoveKeyValuesFromMemoryLeakList( void * ) override {}

private:
    int m_keyValueSize = 0;
    std::vector<std::string> m_symbols;
};

static inline PanelData_t *GetPanelData(VPANEL panel)
{
    unsigned int idx = (unsigned int)panel - 1;
    if (idx >= MAX_PANELS)
        return NULL;
    return &s_panelData[idx];
}

static inline VPANEL CreatePanel()
{
    if (s_panelCount >= MAX_PANELS)
        return INVALID_PANEL;
    
    unsigned int idx = s_panelCount++;
    PanelData_t *p = &s_panelData[idx];
    
    memset(p, 0, sizeof(*p));
    p->pos[0] = p->pos[1] = 0;
    p->size[0] = p->size[1] = 64;
    p->absPos[0] = p->absPos[1] = 0;
    p->visible = true;
    p->enabled = true;
    p->needsSolve = true;
    p->parent = INVALID_PANEL;
    p->childCount = 0;
    p->zpos = 0;
    
    return (VPANEL)(idx + 1);
}

// IVGui implementation
class CVGuiStub : public IVGui
{
public:
    bool Init( CreateInterfaceFn *factoryList, int numFactories ) override
    {
        STUB_PRINTF("IVGui::Init called\n");
        return true;
    }
    
    void Shutdown() override
    {
        STUB_PRINTF("IVGui::Shutdown called\n");
    }
    
    void Start() override
    {
        STUB_PRINTF("IVGui::Start called\n");
    }
    
    void Stop() override
    {
        STUB_PRINTF("IVGui::Stop called\n");
    }
    
    bool IsRunning() override
    {
        return false;
    }
    
    void RunFrame() override
    {
    }
    
    void ShutdownMessage(unsigned int) override
    {
    }
    
	VPANEL AllocPanel() override
	{
		VPANEL result = CreatePanel();
		Con_Reportf("AllocPanel -> %u count=%u\n", result, s_panelCount);
		return result;
	}
    
    void FreePanel(VPANEL panel) override
    {
        PanelData_t *p = GetPanelData(panel);
        if (!p)
            return;

        memset(p, 0, sizeof(*p));
        if (s_embeddedPanel == panel)
            s_embeddedPanel = 0;
    }
    
    void DPrintf(const char *format, ...) override
    {
        STUB_PRINTF("IVGui::DPrintf: %s\n", format);
    }
    
    void DPrintf2(const char *format, ...) override
    {
    }
    
    void SpewAllActivePanelNames() override
    {
    }
    
    HPanel PanelToHandle(VPANEL panel) override
    {
        return (HPanel)(uintptr_t)panel;
    }
    
    VPANEL HandleToPanel(HPanel index) override
    {
        return (VPANEL)(uintptr_t)index;
    }
    
    void MarkPanelForDeletion(VPANEL) override
    {
    }
    
    void AddTickSignal(VPANEL, int) override
    {
    }
    
    void RemoveTickSignal(VPANEL) override
    {
    }
    
    void PostMessage(VPANEL, KeyValues *, VPANEL, float) override
    {
    }
    
    HContext CreateContext() override
    {
        return DEFAULT_VGUI_CONTEXT;
    }
    
    void DestroyContext( HContext ) override
    {
    }
    
    void AssociatePanelWithContext( HContext, VPANEL ) override
    {
    }
    
    void ActivateContext( HContext ) override
    {
    }
    
    void SetSleep( bool ) override
    {
    }
    
    bool GetShouldVGuiControlSleep() override
    {
        return false;
    }
};

// IPanel real implementation
class CPanelReal : public IPanel
{
public:
    void Init(VPANEL vguiPanel, IClientPanel *panel) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        if (!p) return;
        p->pos[0] = p->pos[1] = 0;
        p->size[0] = p->size[1] = 64;
        p->visible = true;
        p->enabled = true;
        p->keyboardInputEnabled = false;
        p->mouseInputEnabled = false;
        p->needsSolve = true;
        p->clientPanel = panel;
    }
    
    void SetPos(VPANEL vguiPanel, int x, int y) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        if (!p) return;
        p->pos[0] = x;
        p->pos[1] = y;
        p->needsSolve = true;
    }
    
    void GetPos(VPANEL vguiPanel, int &x, int &y) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        if (!p) { x = y = 0; return; }
        x = p->pos[0];
        y = p->pos[1];
    }
    
    void SetSize(VPANEL vguiPanel, int wide, int tall) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        if (!p) return;
        p->size[0] = wide;
        p->size[1] = tall;
        if (p->clientPanel)
            p->clientPanel->OnSizeChanged(wide, tall);
    }
    
    void GetSize(VPANEL vguiPanel, int &wide, int &tall) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        if (!p) { wide = tall = 0; return; }
        wide = p->size[0];
        tall = p->size[1];
    }
    
    void SetMinimumSize(VPANEL, int, int) override {}
    void GetMinimumSize(VPANEL, int &wide, int &tall) override { wide = 0; tall = 0; }
    
    void SetZPos(VPANEL vguiPanel, int z) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        if (!p) return;
        p->zpos = z;
    }
    
    int GetZPos(VPANEL vguiPanel) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        if (!p) return 0;
        return p->zpos;
    }
    
    void GetAbsPos(VPANEL vguiPanel, int &x, int &y) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        if (!p) { x = y = 0; return; }
        x = p->absPos[0];
        y = p->absPos[1];
    }
    
    void GetClipRect(VPANEL, int &x0, int &y0, int &x1, int &y1) override
    {
        x0 = s_currentClip[0];
        y0 = s_currentClip[1];
        x1 = s_currentClip[0] + s_currentClip[2];
        y1 = s_currentClip[1] + s_currentClip[3];
    }
    
    void SetInset(VPANEL vguiPanel, int left, int top, int right, int bottom) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        if (!p) return;
        p->insets[0] = left;
        p->insets[1] = top;
        p->insets[2] = right;
        p->insets[3] = bottom;
    }
    
    void GetInset(VPANEL vguiPanel, int &left, int &top, int &right, int &bottom) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        if (!p) { left = top = right = bottom = 0; return; }
        left = p->insets[0];
        top = p->insets[1];
        right = p->insets[2];
        bottom = p->insets[3];
    }
    
    void SetVisible(VPANEL vguiPanel, bool state) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        if (!p) return;
        p->visible = state;
    }
    
    bool IsVisible(VPANEL vguiPanel) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        if (!p) return false;
        
        if (!p->visible)
            return false;
        
        if (p->parent != INVALID_PANEL && p->parent != 0)
            return IsVisible(p->parent);
        
        return true;
    }
    
    void SetParent(VPANEL vguiPanel, VPANEL newParent) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        PanelData_t *oldParent = NULL;
        
        if (!p) return;
        
        if (p->parent != INVALID_PANEL && p->parent != 0)
        {
            oldParent = GetPanelData(p->parent);
            if (oldParent)
            {
                for (int i = 0; i < oldParent->childCount; i++)
                {
                    if (oldParent->children[i] == vguiPanel)
                    {
                        for (int j = i; j < oldParent->childCount - 1; j++)
                            oldParent->children[j] = oldParent->children[j + 1];
                        oldParent->childCount--;
                        break;
                    }
                }
            }
        }
        
        p->parent = newParent;
        p->needsSolve = true;
        
        if (newParent != INVALID_PANEL && newParent != 0)
        {
            PanelData_t *newP = GetPanelData(newParent);
            if (newP && newP->childCount < MAX_CHILDREN)
            {
                newP->children[newP->childCount++] = vguiPanel;
                // TEMPORARY ISOLATION:
                // Avoid synchronous engine->client child-added callbacks during SetParent
                // while we verify whether panel construction is hanging on this boundary.
            }
        }
    }
    
    int GetChildCount(VPANEL vguiPanel) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        if (!p) return 0;
        return p->childCount;
    }
    
    VPANEL GetChild(VPANEL vguiPanel, int index) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        if (!p || index < 0 || index >= p->childCount) return INVALID_PANEL;
        return p->children[index];
    }
    
    VPANEL GetParent(VPANEL vguiPanel) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        if (!p) return INVALID_PANEL;
        return p->parent;
    }
    
    void MoveToFront(VPANEL) override {}
    void MoveToBack(VPANEL) override {}
    
    bool HasParent(VPANEL vguiPanel, VPANEL potentialParent) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        if (!p) return false;
        
        if (p->parent == potentialParent)
            return true;
        
        if (p->parent != INVALID_PANEL && p->parent != 0)
            return HasParent(p->parent, potentialParent);
        
        return false;
    }
    
    bool IsPopup(VPANEL vguiPanel) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        if (!p) return false;
        return p->popup;
    }
    
    void SetPopup(VPANEL vguiPanel, bool state) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        if (!p) return;
        p->popup = state;
    }
    
    bool Render_GetPopupVisible( VPANEL ) override { return true; }
    void Render_SetPopupVisible( VPANEL, bool ) override {}
    HScheme GetScheme(VPANEL vguiPanel) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        return (p && p->clientPanel) ? p->clientPanel->GetScheme() : 0;
    }
    bool IsProportional(VPANEL vguiPanel) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        return (p && p->clientPanel) ? p->clientPanel->IsProportional() : false;
    }
    bool IsAutoDeleteSet(VPANEL vguiPanel) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        return (p && p->clientPanel) ? p->clientPanel->IsAutoDeleteSet() : false;
    }
    void DeletePanel(VPANEL vguiPanel) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        if (p && p->clientPanel)
            p->clientPanel->DeletePanel();
    }
    void SetKeyBoardInputEnabled(VPANEL vguiPanel, bool state) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        if (!p) return;
        p->keyboardInputEnabled = state;
    }
    void SetMouseInputEnabled(VPANEL vguiPanel, bool state) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        if (!p) return;
        p->mouseInputEnabled = state;
    }
    bool IsKeyBoardInputEnabled(VPANEL vguiPanel) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        return p ? p->keyboardInputEnabled : false;
    }
    bool IsMouseInputEnabled(VPANEL vguiPanel) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        return p ? p->mouseInputEnabled : false;
    }
    
    void Solve(VPANEL vguiPanel) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        if (!p) return;
        
        p->absPos[0] = p->pos[0];
        p->absPos[1] = p->pos[1];
        
        if (p->parent != INVALID_PANEL && p->parent != 0)
        {
            PanelData_t *parent = GetPanelData(p->parent);
            if (parent)
            {
                p->absPos[0] += parent->absPos[0];
                p->absPos[1] += parent->absPos[1];
            }
        }
        
        p->needsSolve = false;
    }
    
    const char *GetName(VPANEL vguiPanel) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        return (p && p->clientPanel) ? p->clientPanel->GetName() : "";
    }
    const char *GetClassName(VPANEL vguiPanel) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        return (p && p->clientPanel) ? p->clientPanel->GetClassName() : "";
    }
    void SendMessage(VPANEL vguiPanel, KeyValues *params, VPANEL ifromPanel) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        if (p && p->clientPanel)
            p->clientPanel->OnMessage(params, ifromPanel);
    }
    void Think(VPANEL vguiPanel) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        if (p && p->clientPanel)
            p->clientPanel->Think();
    }
    void PerformApplySchemeSettings(VPANEL vguiPanel) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        if (p && p->clientPanel)
            p->clientPanel->PerformApplySchemeSettings();
    }
    void PaintTraverse(VPANEL vguiPanel, bool forceRepaint, bool allowForce) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        if (p && p->clientPanel)
            p->clientPanel->PaintTraverse(forceRepaint, allowForce);
    }
    void Repaint(VPANEL vguiPanel) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        if (p && p->clientPanel)
            p->clientPanel->Repaint();
    }
    VPANEL IsWithinTraverse(VPANEL, int, int, bool) override { return INVALID_PANEL; }
    void OnChildAdded(VPANEL vguiPanel, VPANEL child) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        if (p && p->clientPanel)
            p->clientPanel->OnChildAdded(child);
    }
    void OnSizeChanged(VPANEL vguiPanel, int newWide, int newTall) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        if (p && p->clientPanel)
            p->clientPanel->OnSizeChanged(newWide, newTall);
    }
    void InternalFocusChanged(VPANEL vguiPanel, bool lost) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        if (p && p->clientPanel)
            p->clientPanel->InternalFocusChanged(lost);
    }
    bool RequestInfo(VPANEL vguiPanel, KeyValues *outputData) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        return (p && p->clientPanel) ? p->clientPanel->RequestInfo(outputData) : false;
    }
    void RequestFocus(VPANEL vguiPanel, int direction) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        if (p && p->clientPanel)
            p->clientPanel->RequestFocus(direction);
    }
    bool RequestFocusPrev(VPANEL vguiPanel, VPANEL existingPanel) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        return (p && p->clientPanel) ? p->clientPanel->RequestFocusPrev(existingPanel) : false;
    }
    bool RequestFocusNext(VPANEL vguiPanel, VPANEL existingPanel) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        return (p && p->clientPanel) ? p->clientPanel->RequestFocusNext(existingPanel) : false;
    }
    VPANEL GetCurrentKeyFocus(VPANEL vguiPanel) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        return (p && p->clientPanel) ? p->clientPanel->GetCurrentKeyFocus() : INVALID_PANEL;
    }
    int GetTabPosition(VPANEL vguiPanel) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        return (p && p->clientPanel) ? p->clientPanel->GetTabPosition() : 0;
    }
    SurfacePlat *Plat(VPANEL) override { return NULL; }
    void SetPlat(VPANEL, SurfacePlat *) override {}
    Panel *GetPanel(VPANEL vguiPanel, const char *destinationModule) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        if (!p || !p->clientPanel)
            return NULL;

        const char *moduleName = p->clientPanel->GetModuleName();
        if (destinationModule && destinationModule[0] && moduleName && moduleName[0] && Q_stricmp(destinationModule, moduleName))
            return NULL;

        return (Panel *)p->clientPanel->GetPanel();
    }
    bool IsEnabled(VPANEL vguiPanel) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        if (!p) return false;
        return p->enabled;
    }
    void SetEnabled(VPANEL vguiPanel, bool state) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        if (!p) return;
        p->enabled = state;
    }
    IClientPanel *Client( VPANEL vguiPanel ) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        return p ? p->clientPanel : NULL;
    }
    const char* GetModuleName( VPANEL vguiPanel ) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        return (p && p->clientPanel) ? p->clientPanel->GetModuleName() : "";
    }
};

// ISurface real implementation
class CSurfaceReal : public ISurface
{
public:
    // Remaining non-blocking stubs are kept explicit here on purpose:
    // popup/window management, browser integration, advanced cursor/focus handling,
    // and texture update helpers below are still minimal until the client needs them.
    void Shutdown() override {}
    void RunFrame() override {}
    VPANEL GetEmbeddedPanel() override { return s_embeddedPanel; }
    void SetEmbeddedPanel( VPANEL panel ) override { s_embeddedPanel = panel; }
    
    void PushMakeCurrent(VPANEL panel, bool useInsets) override
    {
        PanelData_t *p = GetPanelData(panel);
        if (!p) return;
        
        int insets[4] = {0, 0, 0, 0};
        if (useInsets)
        {
            insets[0] = p->insets[0];
            insets[1] = p->insets[1];
            insets[2] = p->insets[2];
            insets[3] = p->insets[3];
        }
        
        int x = p->absPos[0] + insets[0];
        int y = p->absPos[1] + insets[1];
        int w = p->size[0] - insets[0] - insets[2];
        int h = p->size[1] - insets[1] - insets[3];
        
        if (x < s_currentClip[0]) {
            w -= (s_currentClip[0] - x);
            x = s_currentClip[0];
        }
        if (y < s_currentClip[1]) {
            h -= (s_currentClip[1] - y);
            y = s_currentClip[1];
        }
        if (x + w > s_currentClip[0] + s_currentClip[2])
            w = s_currentClip[0] + s_currentClip[2] - x;
        if (y + h > s_currentClip[1] + s_currentClip[3])
            h = s_currentClip[1] + s_currentClip[3] - y;
        
        if (w <= 0 || h <= 0) return;
        
        if (s_clipStackDepth < 32)
        {
            memcpy(s_clipStack[s_clipStackDepth], s_currentClip, sizeof(int) * 4);
            s_clipStackDepth++;
        }
        
        s_currentClip[0] = x;
        s_currentClip[1] = y;
        s_currentClip[2] = w;
        s_currentClip[3] = h;
    }
    
    void PopMakeCurrent(VPANEL) override
    {
        if (s_clipStackDepth > 0)
        {
            s_clipStackDepth--;
            memcpy(s_currentClip, s_clipStack[s_clipStackDepth], sizeof(int) * 4);
        }
    }
    
    void DrawSetColor(int r, int g, int b, int a) override
    {
        m_color[0] = r;
        m_color[1] = g;
        m_color[2] = b;
        m_color[3] = a;
    }
    void DrawSetColor(Color col) override
    {
        m_color[0] = col.r();
        m_color[1] = col.g();
        m_color[2] = col.b();
        m_color[3] = col.a();
    }
    
    void DrawFilledRect(int x0, int y0, int x1, int y1) override
    {
        /*
        // Log current clip state at entry
        Con_Reportf("VGUI2: DrawFilledRect clip_state=(%d,%d,%d,%d) x0=%d y0=%d x1=%d y1=%d color=%d,%d,%d,%d\n", 
            s_currentClip[0], s_currentClip[1], s_currentClip[2], s_currentClip[3],
            x0, y0, x1, y1, m_color[0], m_color[1], m_color[2], m_color[3]);
        
        // TEMPORARY BYPASS ALL CLIPPING FOR TESTING
        // This will draw even if outside clip bounds
        Con_Reportf("VGUI2: DrawFilledRect UNCIPPED calling FillRGBA at (%.0f,%.0f) size (%.0f,%.0f)\n",
            (float)x0, (float)y0, (float)(x1 - x0), (float)(y1 - y0));
        
        ref.dllFuncs.FillRGBA(kRenderTransTexture, 
            (float)x0, (float)y0, 
            (float)(x1 - x0), (float)(y1 - y0),
            (byte)m_color[0], (byte)m_color[1], (byte)m_color[2], (byte)m_color[3]);
        return;
        // END TEMPORARY BYPASS
        
        // Original clipped code (disabled for testing):
        */
        int clipX = x0 < s_currentClip[0] ? s_currentClip[0] : x0;
        int clipY = y0 < s_currentClip[1] ? s_currentClip[1] : y0;
        int clipX1 = x1 > s_currentClip[0] + s_currentClip[2] ? s_currentClip[0] + s_currentClip[2] : x1;
        int clipY1 = y1 > s_currentClip[1] + s_currentClip[3] ? s_currentClip[1] + s_currentClip[3] : y1;
        
        if (clipX >= clipX1 || clipY >= clipY1)
        {
            return;
        }

        ref.dllFuncs.FillRGBA(kRenderTransTexture, 
            (float)clipX, (float)clipY, 
            (float)(clipX1 - clipX), (float)(clipY1 - clipY),
            (byte)m_color[0], (byte)m_color[1], (byte)m_color[2], (byte)m_color[3]);
    }
    
    void DrawOutlinedRect(int x0, int y0, int x1, int y1) override {}
    void DrawLine(int, int, int, int) override {}
    void DrawPolyLine(int *, int *, int) override {}
    void DrawSetTextFont(HFont font) override { m_textFont = font; }
    void DrawSetTextColor(int r, int g, int b, int a) override
    {
        m_textColor[0] = r;
        m_textColor[1] = g;
        m_textColor[2] = b;
        m_textColor[3] = a;
    }
    void DrawSetTextColor(Color) override {}
    void DrawSetTextPos(int x, int y) override
    {
        m_textPos[0] = x;
        m_textPos[1] = y;
    }
    void DrawGetTextPos(int& x,int& y) override
    {
        x = m_textPos[0];
        y = m_textPos[1];
    }
    void DrawPrintText(const wchar_t *text, int textLen) override
    {
        if (!text || textLen <= 0)
            return;

        char ansi[2048];
        int len = 0;
        for (int i = 0; i < textLen && len < (int)sizeof(ansi) - 1; ++i)
        {
            wchar_t ch = text[i];
            if (ch == L'\0')
                break;
            ansi[len++] = (ch >= 32 && ch < 127) ? (char)ch : '?';
        }
        ansi[len] = '\0';

        rgba_t color = {
            (byte)m_textColor[0],
            (byte)m_textColor[1],
            (byte)m_textColor[2],
            (byte)m_textColor[3]
        };

        m_textPos[0] = Con_DrawString(m_textPos[0], m_textPos[1], ansi, color);
    }
    void DrawUnicodeChar(wchar_t ch) override
    {
        wchar_t text[2] = { ch, 0 };
        DrawPrintText(text, 1);
    }
    void DrawUnicodeCharAdd( wchar_t ch ) override
    {
        DrawUnicodeChar( ch );
    }
    void DrawFlushText() override {}
    IHTML *CreateHTMLWindow(vgui2::IHTMLEvents *, VPANEL) override { return NULL; }
    void PaintHTMLWindow(vgui2::IHTML *) override {}
    void DeleteHTMLWindow(IHTML *) override {}
    void DrawSetTextureFile(int id, const char *filename, int, bool) override
    {
        if (id <= 0 || id >= MAX_VGUI2_TEXTURES || !filename || !filename[0])
            return;

        TextureData_t &tex = s_textureData[id];
        if (!tex.valid || Q_strcmp(tex.name, filename))
        {
            Q_strncpy(tex.name, filename, sizeof(tex.name) - 1);
            tex.glTexnum = ref.dllFuncs.GL_LoadTexture(filename, NULL, 0, TF_IMAGE | TF_NOMIPMAP);
            tex.wide = 0;
            tex.tall = 0;
            tex.valid = (tex.glTexnum != 0);
        }
    }
    void DrawSetTextureRGBA(int id, const unsigned char *rgba, int wide, int tall, int, bool) override
    {
        if (id <= 0 || id >= MAX_VGUI2_TEXTURES || !rgba || wide <= 0 || tall <= 0)
            return;

        TextureData_t &tex = s_textureData[id];
        Q_snprintf(tex.name, sizeof(tex.name), "*vgui2_%d", id);
        tex.glTexnum = ref.dllFuncs.GL_CreateTexture(tex.name, wide, tall, rgba, (texFlags_t)(TF_IMAGE | TF_NOMIPMAP));
        tex.wide = wide;
        tex.tall = tall;
        tex.valid = (tex.glTexnum != 0);
    }
    void DrawSetTexture(int id) override { m_boundTexture = id; }
    void DrawGetTextureSize(int id, int &wide, int &tall) override
    {
        if (id > 0 && id < MAX_VGUI2_TEXTURES && s_textureData[id].valid)
        {
            wide = s_textureData[id].wide;
            tall = s_textureData[id].tall;
            return;
        }

        wide = 0;
        tall = 0;
    }
    void DrawTexturedRect(int x0, int y0, int x1, int y1) override
    {
        if (m_boundTexture <= 0 || m_boundTexture >= MAX_VGUI2_TEXTURES)
            return;

        TextureData_t &tex = s_textureData[m_boundTexture];
        if (!tex.valid || tex.glTexnum == 0)
            return;

        ref.dllFuncs.Color4ub((byte)m_color[0], (byte)m_color[1], (byte)m_color[2], (byte)m_color[3]);
        ref.dllFuncs.R_DrawStretchPic((float)x0, (float)y0, (float)(x1 - x0), (float)(y1 - y0),
            0.0f, 0.0f, 1.0f, 1.0f, tex.glTexnum);
    }
    bool IsTextureIDValid(int id) override
    {
        return id > 0 && id < MAX_VGUI2_TEXTURES && s_textureData[id].valid;
    }
    int CreateNewTextureID( bool ) override
    {
        if (s_nextTextureId >= MAX_VGUI2_TEXTURES)
            return 0;
        return s_nextTextureId++;
    }
    void GetScreenSize(int &wide, int &tall) override 
    { 
        wide = refState.width; 
        tall = refState.height; 
    }
    void SetAsTopMost(VPANEL, bool) override {}
    void BringToFront(VPANEL) override {}
    void SetForegroundWindow (VPANEL) override {}
    void SetPanelVisible(VPANEL, bool) override {}
    void SetMinimized(VPANEL, bool) override {}
    bool IsMinimized(VPANEL) override { return false; }
    void FlashWindow(VPANEL, bool) override {}
    void SetTitle(VPANEL, const wchar_t *) override {}
    void SetAsToolBar(VPANEL, bool) override {}
    void CreatePopup(VPANEL, bool, bool, bool, bool, bool) override {}
    void SwapBuffers(VPANEL) override {}
    void Invalidate(VPANEL) override {}
    void SetCursor(HCursor) override {}
    bool IsCursorVisible() override { return false; }
    void ApplyChanges() override {}
    bool IsWithin(int, int) override { return false; }
    bool HasFocus() override { return false; }
    bool SupportsFeature(SurfaceFeature_e) override { return false; }
    void RestrictPaintToSinglePanel(VPANEL) override {}
    void SetModalPanel(VPANEL) override {}
    VPANEL GetModalPanel() override { return INVALID_PANEL; }
    void UnlockCursor() override {}
    void LockCursor() override {}
    void SetTranslateExtendedKeys(bool) override {}
    VPANEL GetTopmostPopup() override { return INVALID_PANEL; }
    void SetTopLevelFocus(VPANEL) override {}
    HFont CreateFont() override
    {
        for (int i = 1; i < MAX_VGUI2_FONTS; ++i)
        {
            if (!s_fontData[i].valid)
            {
                memset(&s_fontData[i], 0, sizeof(s_fontData[i]));
                s_fontData[i].valid = true;
                s_fontData[i].tall = 12;
                s_fontData[i].charWidth = 8;
                return (HFont)i;
            }
        }
        return INVALID_HFONT;
    }
    bool AddGlyphSetToFont(HFont font, const char *windowsFontName, int tall, int weight, int blur, int scanlines, int flags, int lowRange, int highRange) override
    {
        if (font <= 0 || font >= MAX_VGUI2_FONTS)
            return false;

        FontData_t &fontData = s_fontData[font];
        if (!fontData.valid)
            return false;

        Q_strncpy(fontData.name, windowsFontName ? windowsFontName : "Default", sizeof(fontData.name) - 1);
        fontData.tall = tall > 0 ? tall : 12;
        fontData.weight = weight;
        fontData.blur = blur;
        fontData.scanlines = scanlines;
        fontData.flags = flags;
        fontData.lowRange = lowRange;
        fontData.highRange = highRange;
        fontData.charWidth = Q_max(4, fontData.tall / 2);
        return true;
    }
    bool AddCustomFontFile(const char *) override { return true; }
    int GetFontTall(HFont font) override
    {
        if (font > 0 && font < MAX_VGUI2_FONTS && s_fontData[font].valid)
            return s_fontData[font].tall;
        return 12;
    }
    void GetCharABCwide(HFont font, int, int &a, int &b, int &c) override
    {
        a = 0;
        b = GetCharacterWidth(font, 0);
        c = 0;
    }
    int GetCharacterWidth(HFont font, int) override
    {
        if (font > 0 && font < MAX_VGUI2_FONTS && s_fontData[font].valid)
            return s_fontData[font].charWidth;
        return 8;
    }
    void GetTextSize(HFont font, const wchar_t *text, int &wide, int &tall) override
    {
        if (!text)
        {
            wide = 0;
            tall = GetFontTall(font);
            return;
        }

        int len = 0;
        while (text[len] != 0)
            ++len;

        wide = len * GetCharacterWidth(font, 0);
        tall = GetFontTall(font);
    }
    VPANEL GetNotifyPanel() override { return INVALID_PANEL; }
    void SetNotifyIcon(VPANEL, HTexture, VPANEL, const char *) override {}
    void PlaySound(const char *) override {}
    int GetPopupCount() override { return 0; }
    VPANEL GetPopup(int) override { return INVALID_PANEL; }
    bool ShouldPaintChildPanel(VPANEL) override { return true; }
    bool RecreateContext(VPANEL) override { return false; }
    void AddPanel(VPANEL) override {}
    void ReleasePanel(VPANEL) override {}
    void MovePopupToFront(VPANEL) override {}
    void MovePopupToBack(VPANEL) override {}
    
    void SolveTraverse(VPANEL panel, bool) override
    {
        SolveTraverse_Recursive(panel);
    }
    
    void PaintTraverse(VPANEL panel) override
    {
        Con_Reportf("VGUI2: CSurfaceReal::PaintTraverse called panel=%d\n", (int)panel);
        PaintTraverse_Recursive(panel);
    }
    
    void EnableMouseCapture(VPANEL, bool) override {}
    void GetWorkspaceBounds(int &x, int &y, int &wide, int &tall) override { x = y = 0; wide = refState.width; tall = refState.height; }
    void GetAbsoluteWindowBounds(int &x, int &y, int &wide, int &tall) override { x = y = 0; wide = refState.width; tall = refState.height; }
    void GetProportionalBase( int &, int &) override {}
    void CalculateMouseVisible() override {}
    bool NeedKBInput() override { return false; }
    bool HasCursorPosFunctions() override { return false; }
    void SurfaceGetCursorPos(int &x, int &y) override { x = y = 0; }
    void SurfaceSetCursorPos(int, int) override {}
    void DrawTexturedPolygon(VGuiVertex *, int) override {}
    int GetFontAscent( HFont font, wchar_t ) override { return Q_max(0, GetFontTall(font) - 2); }
    void SetAllowHTMLJavaScript( bool ) override {}
    void SetLanguage( const char* ) override {}
    const char* GetLanguage() override { return "english"; }
    bool DeleteTextureByID( int ) override { return false; }
    void DrawUpdateRegionTextureBGRA( int, int, int, const unsigned char *, int, int ) override {}
    void DrawSetTextureBGRA( int, const unsigned char *, int, int ) override {}
    void CreateBrowser( vgui2::VPANEL, IHTMLResponses *, bool, const char * ) override {}
    void RemoveBrowser( vgui2::VPANEL, IHTMLResponses * ) override {}
    IHTMLChromeController *AccessChromeHTMLController() override { return NULL; }
    void DrawTexturedRectAdd(int, int, int, int) override {}
    void SetSupportsEsc(bool) override {}
    int GetFontBlur(vgui2::HFont) override { return 0; }
    bool IsAdditive(vgui2::HFont) override { return false; }
    void SetProportionalBase(int, int) override {}
    void GetHDProportionalBase(int &, int &) override {}
    void SetHDProportionalBase(int, int) override {}

private:
    int m_color[4] = {255, 255, 255, 255};
    int m_textColor[4] = {255, 255, 255, 255};
    int m_textPos[2] = {0, 0};
    HFont m_textFont = INVALID_HFONT;
    int m_boundTexture = 0;
    
    bool IsPanelVisible(VPANEL panel)
    {
        PanelData_t *p = GetPanelData(panel);
        if (!p) return false;
        
        if (!p->visible)
            return false;
        
        if (p->parent != INVALID_PANEL && p->parent != 0)
            return IsPanelVisible(p->parent);
        
        return true;
    }
    
    void SolveTraverse_Recursive(VPANEL panel)
    {
        PanelData_t *p = GetPanelData(panel);
        if (!p) return;
        
        if (p->needsSolve)
        {
            p->absPos[0] = p->pos[0];
            p->absPos[1] = p->pos[1];
            
            if (p->parent != INVALID_PANEL && p->parent != 0)
            {
                PanelData_t *parent = GetPanelData(p->parent);
                if (parent)
                {
                    p->absPos[0] += parent->absPos[0];
                    p->absPos[1] += parent->absPos[1];
                }
            }
            p->needsSolve = false;
        }
        
        for (int i = 0; i < p->childCount; i++)
        {
            if (p->children[i] != INVALID_PANEL)
                SolveTraverse_Recursive(p->children[i]);
        }
    }
    
    void PaintTraverse_Recursive(VPANEL panel)
    {
        PanelData_t *p = GetPanelData(panel);
        if (!p) return;
        
        if (!IsPanelVisible(panel))
        {
            // Debug log for test panel visibility check
            if (p->size[0] == 200 && p->size[1] == 100 && p->pos[0] == 100 && p->pos[1] == 100)
            {
                Con_Reportf("VGUI2: PaintTraverse SKIPPING test panel id=%d - NOT VISIBLE (visible=%d)\n", 
                    (int)panel, p->visible);
            }
            return;
        }
        
        // Debug log for test panel traversal
        if (p->size[0] == 200 && p->size[1] == 100 && p->pos[0] == 100 && p->pos[1] == 100)
        {
            Con_Reportf("VGUI2: PaintTraverse PAINTING test panel id=%d at (%d,%d) size=%dx%d\n", 
                (int)panel, p->absPos[0], p->absPos[1], p->size[0], p->size[1]);
        }
        
        PushMakeCurrent(panel, true);
        
        /*
        // Draw panel rect in GREEN to distinguish from direct injection (red)
        DrawSetColor( 0, 255, 0, 255 );
        DrawFilledRect(p->absPos[0], p->absPos[1], 
                       p->absPos[0] + p->size[0], 
                       p->absPos[1] + p->size[1]);
        */

        if (p->clientPanel)
            p->clientPanel->PaintTraverse(true, true);
        
        for (int i = 0; i < p->childCount; i++)
        {
            if (p->children[i] != INVALID_PANEL)
                PaintTraverse_Recursive(p->children[i]);
        }
        
        PopMakeCurrent(panel);
    }
};

static CSurfaceReal s_TheSurface;
static CKeyValuesStub s_IKeyValues;

// IInputInternal stub implementation
class CInputInternalStub : public IInputInternal
{
public:
    void SetMouseFocus(VPANEL) override {}
    void SetMouseCapture(VPANEL) override {}
    void GetKeyCodeText(int, char *buf, int buflen) override
    {
        if (buf && buflen > 0)
            buf[0] = '\0';
    }
    VPANEL GetFocus() override { return INVALID_PANEL; }
    VPANEL GetMouseOver() override { return INVALID_PANEL; }
    void SetCursorPos(int, int) override {}
    void GetCursorPos(int &x, int &y) override { x = 0; y = 0; }
    bool WasMousePressed(int) override { return false; }
    bool WasMouseDoublePressed(int) override { return false; }
    bool IsMouseDown(int) override { return false; }
    void SetCursorOveride(HCursor) override {}
    HCursor GetCursorOveride() override { return 0; }
    bool WasMouseReleased(int) override { return false; }
    bool WasKeyPressed(int) override { return false; }
    bool IsKeyDown(int) override { return false; }
    bool WasKeyTyped(int) override { return false; }
    bool WasKeyReleased(int) override { return false; }
    VPANEL GetAppModalSurface() override { return INVALID_PANEL; }
    void SetAppModalSurface(VPANEL) override {}
    void ReleaseAppModalSurface() override {}
    void GetCursorPosition(int &x, int &y) override { x = 0; y = 0; }
    void RunFrame() override {}
    void UpdateMouseFocus(int, int) override {}
    void PanelDeleted(VPANEL) override {}
    void InternalCursorMoved(int, int) override {}
    void InternalMousePressed(int) override {}
    void InternalMouseDoublePressed(int) override {}
    void InternalMouseReleased(int) override {}
    void InternalMouseWheeled(int) override {}
    void InternalKeyCodePressed(int) override {}
    void InternalKeyCodeTyped(int) override {}
    void InternalKeyTyped(wchar_t) override {}
    void InternalKeyCodeReleased(int) override {}
    int CreateInputContext() override { return 0; }
    void DestroyInputContext( int ) override {}
    void AssociatePanelWithInputContext( int, VPANEL ) override {}
    void ActivateInputContext( int ) override {}
    VPANEL GetMouseCapture() override { return INVALID_PANEL; }
    bool IsChildOfModalPanel( VPANEL ) override { return false; }
    void ResetInputContext( int ) override {}
};

// IScheme stub implementation
class CSchemeStub : public IScheme
{
public:
    const char *GetResourceString(const char *) override { return ""; }
    IBorder *GetBorder(const char *) override { return NULL; }
    HFont GetFont(const char *, bool) override { return INVALID_HFONT; }
    Color GetColor(const char *, Color defaultColor) override { return defaultColor; }
    vgui2::HFont GetFontEx(const char *, bool, bool) override { return INVALID_HFONT; }
};

// ISchemeManager stub implementation
class CSchemeManagerStub : public ISchemeManager
{
public:
    HScheme LoadSchemeFromFile( const char *, const char * ) override { return 0; }
    void ReloadSchemes() override {}
    HScheme GetDefaultScheme() override { return 0; }
    HScheme GetScheme( const char * ) override { return 0; }
    IImage *GetImage( const char *, bool ) override { return NULL; }
    HTexture GetImageID( const char *, bool ) override { return 0; }
    IScheme *GetIScheme( HScheme ) override { return &m_Scheme; }
    void Shutdown( bool ) override {}
    int GetProportionalScaledValue( int normalizedValue ) override { return normalizedValue; }
    int GetProportionalNormalizedValue( int scaledValue ) override { return scaledValue; }
    float GetProportionalScale() override { return 1.0f; }
    int GetHDProportionalScaledValue(int normalizedValue) override { return normalizedValue; }
    int GetHDProportionalNormalizedValue(int scaledValue) override { return scaledValue; }

private:
    CSchemeStub m_Scheme;
};

// ILocalize stub implementation
class CLocalizeStub : public ILocalize
{
public:
    bool AddFile( IFileSystem *, const char *) override { return true; }
    void RemoveAll() override {}
    wchar_t *Find(char const *) override { return NULL; }
    int ConvertANSIToUnicode(const char *, wchar_t *, int) override { return 0; }
    int ConvertUnicodeToANSI(const wchar_t *, char *, int) override { return 0; }
    unsigned long FindIndex(const char *) override { return 0; }
    void ConstructString(wchar_t *, int, wchar_t *, int, ...) override {}
    const char *GetNameByIndex(unsigned long) override { return ""; }
    wchar_t *GetValueByIndex(unsigned long) override { return NULL; }
    unsigned long GetFirstStringIndex() override { return 0; }
    unsigned long GetNextStringIndex(unsigned long) override { return 0; }
    void AddString(const char *, wchar_t *, const char *) override {}
    void SetValueByIndex(unsigned long, wchar_t *) override {}
    bool SaveToFile(IFileSystem *, const char *) override { return false; }
    int GetLocalizationFileCount() override { return 0; }
    const char *GetLocalizationFileName(int) override { return ""; }
    const char *GetFileNameByIndex(unsigned long) override { return ""; }
    void ReloadLocalizationFiles(IFileSystem *) override {}
    void ConstructString(wchar_t *, int, const char *, KeyValues *) override {}
    void ConstructString(wchar_t *, int, unsigned long, KeyValues *) override {}
};

// ISystem stub implementation
class CSysStub : public ISystem
{
public:
    void Shutdown() override {}
    void RunFrame() override {}
    void ShellExecute(const char *, const char *) override {}
    double GetFrameTime() override { return 0.0; }
    double GetCurrentTime() override { return 0.0; }
    long GetTimeMillis() override { return 0; }
    int GetClipboardTextCount() override { return 0; }
    void SetClipboardText(const char *, int) override {}
    void SetClipboardText(const wchar_t *, int) override {}
    int GetClipboardText(int, char *, int) override { return 0; }
    int GetClipboardText(int, wchar_t *, int) override { return 0; }
    bool SetRegistryString(const char *, const char *) override { return false; }
    bool GetRegistryString(const char *, char *, int) override { return false; }
    bool SetRegistryInteger(const char *, int) override { return false; }
    bool GetRegistryInteger(const char *, int &) override { return false; }
    KeyValues *GetUserConfigFileData(const char *, int) override { return NULL; }
    void SetUserConfigFile(const char *, const char *) override {}
    void SaveUserConfigFile() override {}
    bool SetWatchForComputerUse(bool) override { return false; }
    double GetTimeSinceLastUse() override { return 0.0; }
    int GetAvailableDrives(char *, int) override { return 0; }
    bool CommandLineParamExists(const char *) override { return false; }
    const char *GetFullCommandLine() override { return ""; }
    bool GetCurrentTimeAndDate(int *, int *, int *, int *, int *, int *, int *) override { return false; }
    double GetFreeDiskSpace(const char *) override { return 0.0; }
    bool CreateShortcut(const char *, const char *, const char *, const char *, const char *) override { return false; }
    bool GetShortcutTarget(const char *, char *, char *, int) override { return false; }
    bool ModifyShortcutTarget(const char *, const char *, const char *, const char *) override { return false; }
    bool GetCommandLineParamValue(const char *, char *, int) override { return false; }
    bool DeleteRegistryKey(const char *) override { return false; }
    const char *GetDesktopFolderPath() override { return ""; }
    int KeyCode_VirtualKeyToVGUI( int keyCode ) override { return keyCode; }
    int KeyCode_VGUIToVirtualKey( int keyCode ) override { return keyCode; }
    const char *GetStartMenuFolderPath() override { return ""; }
    const char *GetAllUserDesktopFolderPath() override { return ""; }
    const char *GetAllUserStartMenuFolderPath() override { return ""; }
};

// Static instances
static CVGuiStub s_IVGui;
static CPanelReal s_IPanel;
static CSurfaceReal s_ISurface;
static CInputInternalStub s_IInputInternal;
static vgui2::ISchemeManager *s_ISchemeManager = NULL;
static CLocalizeStub s_ILocalize;
static CSysStub s_ISystem;

ISchemeManager *GetSchemeManager();

} // namespace vgui2

// CVGui2Interfaces implementation
void CVGui2Interfaces::Init()
{
    if( m_bInitialized )
        return;
    
    m_pIVGui = &vgui2::s_IVGui;
    m_pIPanel = &vgui2::s_IPanel;
    m_pISurface = &vgui2::s_ISurface;
    m_pIInputInternal = &vgui2::s_IInputInternal;
    m_pISchemeManager = vgui2::GetSchemeManager();
    m_pILocalize = vgui2::GetLocalizeImpl();
    m_pISystem = &vgui2::s_ISystem;
    vgui2::SetLocalizeFileSystemImpl(vgui2::GetVFileSystem());
    
    m_bInitialized = true;
    
    Con_Reportf("VGUI2 interfaces: IVGui=%p IPanel=%p ISurface=%p IInputInternal=%p ISchemeManager=%p ILocalize=%p ISystem=%p\n",
        m_pIVGui, m_pIPanel, m_pISurface, m_pIInputInternal, m_pISchemeManager, m_pILocalize, m_pISystem);
}

void CVGui2Interfaces::Shutdown()
{
    if( !m_bInitialized )
        return;
    
    m_bInitialized = false;
}

void CVGui2Interfaces::RunFrame()
{
    if( !m_bInitialized )
        return;
}

void *CVGui2Interfaces::CreateInterface( const char *pName, int *pReturnCode )
{
    if( !pName )
    {
        if( pReturnCode )
            *pReturnCode = IFACE_FAILED;
        return NULL;
    }
    
    void *pInterface = NULL;
    
    if( !Q_strcmp( pName, VGUI_IVGUI_INTERFACE_VERSION_GS ) )
        pInterface = m_pIVGui;
    else if( !Q_strcmp( pName, VGUI_PANEL_INTERFACE_VERSION_GS ) )
        pInterface = m_pIPanel;
    else if( !Q_strcmp( pName, VGUI_SURFACE_INTERFACE_VERSION_GS ) )
        pInterface = m_pISurface;
    else if( !Q_strcmp( pName, VGUI_INPUTINTERNAL_INTERFACE_VERSION ) )
        pInterface = m_pIInputInternal;
    else if( !Q_strcmp( pName, VGUI_SCHEME_INTERFACE_VERSION_GS ) )
        pInterface = m_pISchemeManager;
    else if( !Q_strcmp( pName, VGUI_LOCALIZE_INTERFACE_VERSION ) )
        pInterface = m_pILocalize;
    else if( !Q_strcmp( pName, VGUI_SYSTEM_INTERFACE_VERSION_GS ) )
        pInterface = m_pISystem;
    else if( !Q_strcmp( pName, vgui2::KEYVALUES_INTERFACE_VERSION ) )
        pInterface = &vgui2::s_IKeyValues;
    else if( !Q_strcmp( pName, FILESYSTEM_INTERFACE_VERSION ) )
        pInterface = vgui2::GetVFileSystem();
    else if( !Q_strcmp( pName, IBASEUI_NAME ) )
        Con_Reportf("VGUI2: Unsupported interface requested: %s (returning NULL)\n", pName);
    else if( !Q_strcmp( pName, VENGINE_VGUI_VERSION ) )
        Con_Reportf("VGUI2: Unsupported interface requested: %s (returning NULL)\n", pName);
    else if( !Q_strcmp( pName, IGAMEUIFUNCS_NAME ) )
        Con_Reportf("VGUI2: Unsupported interface requested: %s (returning NULL)\n", pName);
    else
        Con_Reportf("VGUI2: Unknown interface requested: %s\n", pName);

    Con_Reportf("VGUI2: CreateInterface('%s') -> %p\n", pName, pInterface);
    
    if( pReturnCode )
        *pReturnCode = pInterface ? IFACE_OK : IFACE_FAILED;
    
    return pInterface;
}
