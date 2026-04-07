/*
 vgui2_interfaces.cpp - VGUI2 implementation for Xash3D FWGS
 Phase 2: Real runtime implementation with visible rendering
 */
#include "vgui2_interfaces.h"
#include "common.h"
#include "client.h"
#include "ref_common.h"

#define CONPRINTF(...) Con_Reportf(__VA_ARGS__)

#define STUB_PRINTF(...) CONPRINTF("VGUI2 stub: %s:%d - ", __FILE__, __LINE__); CONPRINTF(__VA_ARGS__)

namespace vgui2
{

#define MAX_PANELS 256
#define MAX_CHILDREN 32

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
    VPANEL parent;
    VPANEL children[MAX_CHILDREN];
    int childCount;
};

static PanelData_t s_panelData[MAX_PANELS];
static unsigned int s_panelCount = 1;
static int s_currentClip[4] = { 0, 0, 99999, 99999 };
static int s_clipStack[32][4];
static int s_clipStackDepth = 0;

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
        static unsigned int panelCounter = 1;
        return (VPANEL)(panelCounter++);
    }
    
    void FreePanel(VPANEL) override
    {
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
    void Init(VPANEL vguiPanel, void *) override
    {
        PanelData_t *p = GetPanelData(vguiPanel);
        if (!p) return;
        p->pos[0] = p->pos[1] = 0;
        p->size[0] = p->size[1] = 64;
        p->visible = true;
        p->enabled = true;
        p->needsSolve = true;
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
    HScheme GetScheme(VPANEL) override { return 0; }
    bool IsProportional(VPANEL) override { return false; }
    bool IsAutoDeleteSet(VPANEL) override { return false; }
    void DeletePanel(VPANEL) override {}
    void SetKeyBoardInputEnabled(VPANEL, bool) override {}
    void SetMouseInputEnabled(VPANEL, bool) override {}
    bool IsKeyBoardInputEnabled(VPANEL) override { return false; }
    bool IsMouseInputEnabled(VPANEL) override { return false; }
    
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
    
    const char *GetName(VPANEL) override { return ""; }
    const char *GetClassName(VPANEL) override { return ""; }
    void SendMessage(VPANEL, KeyValues *, VPANEL) override {}
    void Think(VPANEL) override {}
    void PerformApplySchemeSettings(VPANEL) override {}
    void PaintTraverse(VPANEL, bool, bool) override {}
    void Repaint(VPANEL) override {}
    VPANEL IsWithinTraverse(VPANEL, int, int, bool) override { return INVALID_PANEL; }
    void OnChildAdded(VPANEL, VPANEL) override {}
    void OnSizeChanged(VPANEL, int, int) override {}
    void InternalFocusChanged(VPANEL, bool) override {}
    bool RequestInfo(VPANEL, KeyValues *) override { return false; }
    void RequestFocus(VPANEL, int) override {}
    bool RequestFocusPrev(VPANEL, VPANEL) override { return false; }
    bool RequestFocusNext(VPANEL, VPANEL) override { return false; }
    VPANEL GetCurrentKeyFocus(VPANEL) override { return INVALID_PANEL; }
    int GetTabPosition(VPANEL) override { return 0; }
    void *Plat(VPANEL) override { return NULL; }
    void SetPlat(VPANEL, void *) override {}
    void *GetPanel(VPANEL, const char *) override { return NULL; }
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
    void *Client( VPANEL ) override { return NULL; }
    const char* GetModuleName( VPANEL ) override { return ""; }
};

// ISurface real implementation
class CSurfaceReal : public ISurface
{
public:
    void Shutdown() override {}
    void RunFrame() override {}
    VPANEL GetEmbeddedPanel() override { return (VPANEL)1; }
    void SetEmbeddedPanel( VPANEL ) override {}
    
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
    
    void DrawFilledRect(int x0, int y0, int x1, int y1) override
    {
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
        int clipX = x0 < s_currentClip[0] ? s_currentClip[0] : x0;
        int clipY = y0 < s_currentClip[1] ? s_currentClip[1] : y0;
        int clipX1 = x1 > s_currentClip[0] + s_currentClip[2] ? s_currentClip[0] + s_currentClip[2] : x1;
        int clipY1 = y1 > s_currentClip[1] + s_currentClip[3] ? s_currentClip[1] + s_currentClip[3] : y1;
        
        if (clipX >= clipX1 || clipY >= clipY1)
        {
            Con_Reportf("VGUI2: DrawFilledRect clipped out! clip=(%d,%d) to (%d,%d)\n", 
                clipX, clipY, clipX1, clipY1);
            return;
        }
        
        Con_Reportf("VGUI2: DrawFilledRect calling FillRGBA at (%.0f,%.0f) size (%.0f,%.0f)\n",
            (float)clipX, (float)clipY, (float)(clipX1 - clipX), (float)(clipY1 - clipY));
        
        ref.dllFuncs.FillRGBA(kRenderTransTexture, 
            (float)clipX, (float)clipY, 
            (float)(clipX1 - clipX), (float)(clipY1 - clipY),
            (byte)m_color[0], (byte)m_color[1], (byte)m_color[2], (byte)m_color[3]);
    }
    
    void DrawOutlinedRect(int x0, int y0, int x1, int y1) override {}
    void DrawLine(int, int, int, int) override {}
    void DrawPolyLine(int *, int *, int) override {}
    void DrawSetTextFont(HFont) override {}
    void DrawSetTextColor(int, int, int, int) override {}
    void DrawSetTextPos(int, int) override {}
    void DrawGetTextPos(int& x,int& y) override { x = 0; y = 0; }
    void DrawPrintText(const wchar_t *, int) override {}
    void DrawUnicodeChar(wchar_t) override {}
    void DrawFlushText() override {}
    void DrawSetTextureFile(int, const char *, int, bool) override {}
    void DrawSetTextureRGBA(int, const unsigned char *, int, int, int, bool) override {}
    void DrawSetTexture(int) override {}
    void DrawGetTextureSize(int, int &, int &) override {}
    void DrawTexturedRect(int, int, int, int) override {}
    bool IsTextureIDValid(int) override { return false; }
    int CreateNewTextureID( bool ) override { return 0; }
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
    bool SupportsFeature(int) override { return false; }
    void RestrictPaintToSinglePanel(VPANEL) override {}
    void SetModalPanel(VPANEL) override {}
    VPANEL GetModalPanel() override { return INVALID_PANEL; }
    void UnlockCursor() override {}
    void LockCursor() override {}
    void SetTranslateExtendedKeys(bool) override {}
    VPANEL GetTopmostPopup() override { return INVALID_PANEL; }
    void SetTopLevelFocus(VPANEL) override {}
    HFont CreateFont() override { return INVALID_HFONT; }
    bool AddGlyphSetToFont(HFont, const char *, int, int, int, int, int, int, int) override { return false; }
    bool AddCustomFontFile(const char *) override { return false; }
    int GetFontTall(HFont) override { return 0; }
    void GetCharABCwide(HFont, int, int &, int &, int &) override {}
    int GetCharacterWidth(HFont, int) override { return 0; }
    void GetTextSize(HFont, const wchar_t *, int &, int &) override {}
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
    void DrawTexturedPolygon(void *, int) override {}
    int GetFontAscent( HFont, wchar_t ) override { return 0; }
    void SetAllowHTMLJavaScript( bool ) override {}
    void SetLanguage( const char* ) override {}
    const char* GetLanguage() override { return "english"; }
    bool DeleteTextureByID( int ) override { return false; }
    void DrawUpdateRegionTextureBGRA( int, int, int, const unsigned char *, int, int ) override {}
    void DrawSetTextureBGRA( int, const unsigned char *, int, int ) override {}
    void CreateBrowser( vgui2::VPANEL, void *, bool, const char * ) override {}
    void RemoveBrowser( vgui2::VPANEL, void * ) override {}
    void *AccessChromeHTMLController() override { return NULL; }
    void DrawTexturedRectAdd(int, int, int, int) override {}
    void SetSupportsEsc(bool) override {}
    int GetFontBlur(vgui2::HFont) override { return 0; }
    bool IsAdditive(vgui2::HFont) override { return false; }
    void SetProportionalBase(int, int) override {}
    void GetHDProportionalBase(int &, int &) override {}
    void SetHDProportionalBase(int, int) override {}

private:
    int m_color[4] = {255, 255, 255, 255};
    
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
        
        // Draw panel rect in GREEN to distinguish from direct injection (red)
        DrawSetColor( 0, 255, 0, 255 );
        DrawFilledRect(p->absPos[0], p->absPos[1], 
                       p->absPos[0] + p->size[0], 
                       p->absPos[1] + p->size[1]);
        
        for (int i = 0; i < p->childCount; i++)
        {
            if (p->children[i] != INVALID_PANEL)
                PaintTraverse_Recursive(p->children[i]);
        }
        
        PopMakeCurrent(panel);
    }
};

static CSurfaceReal s_TheSurface;

// IInputInternal stub implementation
class CInputInternalStub : public IInputInternal
{
public:
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
    void *GetBorder(const char *) override { return NULL; }
    HFont GetFont(const char *, bool) override { return INVALID_HFONT; }
    int GetColor(const char *, int defaultColor) override { return defaultColor; }
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
    void *GetImage( const char *, bool ) override { return NULL; }
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
    bool AddFile( void *, const char *) override { return true; }
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
    bool SaveToFile(void *, const char *) override { return false; }
    int GetLocalizationFileCount() override { return 0; }
    const char *GetLocalizationFileName(int) override { return ""; }
    const char *GetFileNameByIndex(unsigned long) override { return ""; }
    void ReloadLocalizationFiles(void *) override {}
    void ConstructString(wchar_t *, int, const char *, void *) override {}
    void ConstructString(wchar_t *, int, unsigned long, void *) override {}
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
    void *GetUserConfigFileData(const char *, int) override { return NULL; }
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
    m_pILocalize = &vgui2::s_ILocalize;
    m_pISystem = &vgui2::s_ISystem;
    
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
    else if( !Q_strcmp( pName, FILESYSTEM_INTERFACE_VERSION ) )
        pInterface = m_pIVGui;
    else if( !Q_strcmp( pName, IBASEUI_NAME ) )
        pInterface = m_pIVGui;
    else if( !Q_strcmp( pName, VENGINE_VGUI_VERSION ) )
        pInterface = m_pIVGui;
    else if( !Q_strcmp( pName, IGAMEUIFUNCS_NAME ) )
        pInterface = m_pISystem;
    else
        Con_Reportf("VGUI2: Unknown interface requested: %s\n", pName);
    
    if( pReturnCode )
        *pReturnCode = pInterface ? IFACE_OK : IFACE_FAILED;
    
    return pInterface;
}