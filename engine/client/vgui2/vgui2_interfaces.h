#ifndef VGUI2_INTERFACES_H
#define VGUI2_INTERFACES_H

#include "VGUI2.h"
#include "../../../3rdparty/tier1/Color.h"

class IHTMLResponses;
class IHTMLChromeController;

namespace vgui2
{

class IHTML;
class IHTMLEvents;

struct VGuiVertex
{
    VGuiVertex() : x( 0 ), y( 0 ), u( 0 ), v( 0 ) {}
    VGuiVertex( int xIn, int yIn, float uIn = 0, float vIn = 0 )
        : x( xIn ), y( yIn ), u( uIn ), v( vIn ) {}
    void Init( int xIn, int yIn, float uIn = 0, float vIn = 0 )
    {
        x = xIn;
        y = yIn;
        u = uIn;
        v = vIn;
    }

    int x, y;
    float u, v;
};

class IVGui : public IBaseInterface
{
public:
    virtual bool Init( CreateInterfaceFn *factoryList, int numFactories ) = 0;
    virtual void Shutdown() = 0;
    virtual void Start() = 0;
    virtual void Stop() = 0;
    virtual bool IsRunning() = 0;
    virtual void RunFrame() = 0;
    virtual void ShutdownMessage(unsigned int shutdownID) = 0;
    virtual VPANEL AllocPanel() = 0;
    virtual void FreePanel(VPANEL panel) = 0;
    virtual void DPrintf(const char *format, ...) = 0;
    virtual void DPrintf2(const char *format, ...) = 0;
    virtual void SpewAllActivePanelNames() = 0;
    virtual HPanel PanelToHandle(VPANEL panel) = 0;
    virtual VPANEL HandleToPanel(HPanel index) = 0;
    virtual void MarkPanelForDeletion(VPANEL panel) = 0;
    virtual void AddTickSignal(VPANEL panel, int intervalMilliseconds) = 0;
    virtual void RemoveTickSignal(VPANEL panel) = 0;
    virtual void PostMessage(VPANEL target, KeyValues *params, VPANEL from, float delaySeconds) = 0;
    virtual HContext CreateContext() = 0;
    virtual void DestroyContext( HContext context ) = 0;
    virtual void AssociatePanelWithContext( HContext context, VPANEL pRoot ) = 0;
    virtual void ActivateContext( HContext context ) = 0;
    virtual void SetSleep( bool state) = 0;
    virtual bool GetShouldVGuiControlSleep() = 0;
};

class IPanel : public IBaseInterface
{
public:
    virtual void Init(VPANEL vguiPanel, void *panel) = 0;
    virtual void SetPos(VPANEL vguiPanel, int x, int y) = 0;
    virtual void GetPos(VPANEL vguiPanel, int &x, int &y) = 0;
    virtual void SetSize(VPANEL vguiPanel, int wide,int tall) = 0;
    virtual void GetSize(VPANEL vguiPanel, int &wide, int &tall) = 0;
    virtual void SetMinimumSize(VPANEL vguiPanel, int wide, int tall) = 0;
    virtual void GetMinimumSize(VPANEL vguiPanel, int &wide, int &tall) = 0;
    virtual void SetZPos(VPANEL vguiPanel, int z) = 0;
    virtual int  GetZPos(VPANEL vguiPanel) = 0;
    virtual void GetAbsPos(VPANEL vguiPanel, int &x, int &y) = 0;
    virtual void GetClipRect(VPANEL vguiPanel, int &x0, int &y0, int &x1, int &y1) = 0;
    virtual void SetInset(VPANEL vguiPanel, int left, int top, int right, int bottom) = 0;
    virtual void GetInset(VPANEL vguiPanel, int &left, int &top, int &right, int &bottom) = 0;
    virtual void SetVisible(VPANEL vguiPanel, bool state) = 0;
    virtual bool IsVisible(VPANEL vguiPanel) = 0;
    virtual void SetParent(VPANEL vguiPanel, VPANEL newParent) = 0;
    virtual int GetChildCount(VPANEL vguiPanel) = 0;
    virtual VPANEL GetChild(VPANEL vguiPanel, int index) = 0;
    virtual VPANEL GetParent(VPANEL vguiPanel) = 0;
    virtual void MoveToFront(VPANEL vguiPanel) = 0;
    virtual void MoveToBack(VPANEL vguiPanel) = 0;
    virtual bool HasParent(VPANEL vguiPanel, VPANEL potentialParent) = 0;
    virtual bool IsPopup(VPANEL vguiPanel) = 0;
    virtual void SetPopup(VPANEL vguiPanel, bool state) = 0;
    virtual bool Render_GetPopupVisible( VPANEL vguiPanel ) = 0;
    virtual void Render_SetPopupVisible( VPANEL vguiPanel, bool state ) = 0;
    virtual HScheme GetScheme(VPANEL vguiPanel) = 0;
    virtual bool IsProportional(VPANEL vguiPanel) = 0;
    virtual bool IsAutoDeleteSet(VPANEL vguiPanel) = 0;
    virtual void DeletePanel(VPANEL vguiPanel) = 0;
    virtual void SetKeyBoardInputEnabled(VPANEL vguiPanel, bool state) = 0;
    virtual void SetMouseInputEnabled(VPANEL vguiPanel, bool state) = 0;
    virtual bool IsKeyBoardInputEnabled(VPANEL vguiPanel) = 0;
    virtual bool IsMouseInputEnabled(VPANEL vguiPanel) = 0;
    virtual void Solve(VPANEL vguiPanel) = 0;
    virtual const char *GetName(VPANEL vguiPanel) = 0;
    virtual const char *GetClassName(VPANEL vguiPanel) = 0;
    virtual void SendMessage(VPANEL vguiPanel, KeyValues *params, VPANEL ifromPanel) = 0;
    virtual void Think(VPANEL vguiPanel) = 0;
    virtual void PerformApplySchemeSettings(VPANEL vguiPanel) = 0;
    virtual void PaintTraverse(VPANEL vguiPanel, bool forceRepaint, bool allowForce) = 0;
    virtual void Repaint(VPANEL vguiPanel) = 0;
    virtual VPANEL IsWithinTraverse(VPANEL vguiPanel, int x, int y, bool traversePopups) = 0;
    virtual void OnChildAdded(VPANEL vguiPanel, VPANEL child) = 0;
    virtual void OnSizeChanged(VPANEL vguiPanel, int newWide, int newTall) = 0;
    virtual void InternalFocusChanged(VPANEL vguiPanel, bool lost) = 0;
    virtual bool RequestInfo(VPANEL vguiPanel, KeyValues *outputData) = 0;
    virtual void RequestFocus(VPANEL vguiPanel, int direction) = 0;
    virtual bool RequestFocusPrev(VPANEL vguiPanel, VPANEL existingPanel) = 0;
    virtual bool RequestFocusNext(VPANEL vguiPanel, VPANEL existingPanel) = 0;
    virtual VPANEL GetCurrentKeyFocus(VPANEL vguiPanel) = 0;
    virtual int GetTabPosition(VPANEL vguiPanel) = 0;
    virtual void *Plat(VPANEL vguiPanel) = 0;
    virtual void SetPlat(VPANEL vguiPanel, void *Plat) = 0;
    virtual void *GetPanel(VPANEL vguiPanel, const char *destinationModule) = 0;
    virtual bool IsEnabled(VPANEL vguiPanel) = 0;
    virtual void SetEnabled(VPANEL vguiPanel, bool state) = 0;
    virtual void *Client( VPANEL vguiPanel ) = 0;
    virtual const char* GetModuleName( VPANEL vguiPanel ) = 0;
};

class ISurface : public IBaseInterface
{
public:
    virtual void Shutdown() = 0;
    virtual void RunFrame() = 0;
    virtual VPANEL GetEmbeddedPanel() = 0;
    virtual void SetEmbeddedPanel( VPANEL pPanel ) = 0;
    virtual void PushMakeCurrent(VPANEL panel, bool useInsets) = 0;
    virtual void PopMakeCurrent(VPANEL panel) = 0;
    virtual void DrawSetColor(int r, int g, int b, int a) = 0;
    virtual void DrawSetColor( Color col ) = 0;
    virtual void DrawFilledRect(int x0, int y0, int x1, int y1) = 0;
    virtual void DrawOutlinedRect(int x0, int y0, int x1, int y1) = 0;
    virtual void DrawLine(int x0, int y0, int x1, int y1) = 0;
    virtual void DrawPolyLine(int *px, int *py, int numPoints) = 0;
    virtual void DrawSetTextFont(HFont font) = 0;
    virtual void DrawSetTextColor(int r, int g, int b, int a) = 0;
    virtual void DrawSetTextColor( Color col ) = 0;
    virtual void DrawSetTextPos(int x, int y) = 0;
    virtual void DrawGetTextPos(int& x,int& y) = 0;
    virtual void DrawPrintText(const wchar_t *text, int textLen) = 0;
    virtual void DrawUnicodeChar(wchar_t wch) = 0;
    virtual void DrawUnicodeCharAdd( wchar_t wch ) = 0;
    virtual void DrawFlushText() = 0;
    virtual IHTML *CreateHTMLWindow(vgui2::IHTMLEvents *events,VPANEL context) = 0;
    virtual void PaintHTMLWindow(vgui2::IHTML *htmlwin) = 0;
    virtual void DeleteHTMLWindow(IHTML *htmlwin) = 0;
    virtual void DrawSetTextureFile(int id, const char *filename, int hardwareFilter, bool forceReload) = 0;
    virtual void DrawSetTextureRGBA(int id, const unsigned char *rgba, int wide, int tall, int hardwareFilter, bool forceReload)=0;
    virtual void DrawSetTexture(int id) = 0;
    virtual void DrawGetTextureSize(int id, int &wide, int &tall) = 0;
    virtual void DrawTexturedRect(int x0, int y0, int x1, int y1) = 0;
    virtual bool IsTextureIDValid(int id) = 0;
    virtual int CreateNewTextureID( bool procedural ) = 0;
#ifdef _XBOX
    virtual void DestroyTextureID( int id ) = 0;
    virtual bool IsCachedForRendering( int id, bool bSyncWait ) = 0;
    virtual void CopyFrontBufferToBackBuffer() = 0;
    virtual void UncacheUnusedMaterials() = 0;
#endif
    virtual void GetScreenSize(int &wide, int &tall) = 0;
    virtual void SetAsTopMost(VPANEL panel, bool state) = 0;
    virtual void BringToFront(VPANEL panel) = 0;
    virtual void SetForegroundWindow (VPANEL panel) = 0;
    virtual void SetPanelVisible(VPANEL panel, bool state) = 0;
    virtual void SetMinimized(VPANEL panel, bool state) = 0;
    virtual bool IsMinimized(VPANEL panel) = 0;
    virtual void FlashWindow(VPANEL panel, bool state) = 0;
    virtual void SetTitle(VPANEL panel, const wchar_t *title) = 0;
    virtual void SetAsToolBar(VPANEL panel, bool state) = 0;
    virtual void CreatePopup(VPANEL panel, bool minimised, bool showTaskbarIcon, bool disabled, bool mouseInput, bool kbInput) = 0;
    virtual void SwapBuffers(VPANEL panel) = 0;
    virtual void Invalidate(VPANEL panel) = 0;
    virtual void SetCursor(HCursor cursor) = 0;
    virtual bool IsCursorVisible() = 0;
    virtual void ApplyChanges() = 0;
    virtual bool IsWithin(int x, int y) = 0;
    virtual bool HasFocus() = 0;
    enum SurfaceFeature_e
    {
        ANTIALIASED_FONTS = 1,
        DROPSHADOW_FONTS = 2,
        ESCAPE_KEY = 3,
        OPENING_NEW_HTML_WINDOWS = 4,
        FRAME_MINIMIZE_MAXIMIZE = 5,
        OUTLINE_FONTS = 6,
        DIRECT_HWND_RENDER = 7,
    };
    virtual bool SupportsFeature(SurfaceFeature_e feature) = 0;
    virtual void RestrictPaintToSinglePanel(VPANEL panel) = 0;
    virtual void SetModalPanel(VPANEL) = 0;
    virtual VPANEL GetModalPanel() = 0;
    virtual void UnlockCursor() = 0;
    virtual void LockCursor() = 0;
    virtual void SetTranslateExtendedKeys(bool state) = 0;
    virtual VPANEL GetTopmostPopup() = 0;
    virtual void SetTopLevelFocus(VPANEL panel) = 0;
    virtual HFont CreateFont() = 0;
    virtual bool AddGlyphSetToFont(HFont font, const char *windowsFontName, int tall, int weight, int blur, int scanlines, int flags, int lowRange, int highRange) = 0;
    virtual bool AddCustomFontFile(const char *fontFileName) = 0;
    virtual int GetFontTall(HFont font) = 0;
    virtual void GetCharABCwide(HFont font, int ch, int &a, int &b, int &c) = 0;
    virtual int GetCharacterWidth(HFont font, int ch) = 0;
    virtual void GetTextSize(HFont font, const wchar_t *text, int &wide, int &tall) = 0;
    virtual VPANEL GetNotifyPanel() = 0;
    virtual void SetNotifyIcon(VPANEL context, HTexture icon, VPANEL panelToReceiveMessages, const char *text) = 0;
    virtual void PlaySound(const char *fileName) = 0;
    virtual int GetPopupCount() = 0;
    virtual VPANEL GetPopup(int index) = 0;
    virtual bool ShouldPaintChildPanel(VPANEL childPanel) = 0;
    virtual bool RecreateContext(VPANEL panel) = 0;
    virtual void AddPanel(VPANEL panel) = 0;
    virtual void ReleasePanel(VPANEL panel) = 0;
    virtual void MovePopupToFront(VPANEL panel) = 0;
    virtual void MovePopupToBack(VPANEL panel) = 0;
    virtual void SolveTraverse(VPANEL panel, bool forceApplySchemeSettings) = 0;
    virtual void PaintTraverse(VPANEL panel) = 0;
    virtual void EnableMouseCapture(VPANEL panel, bool state) = 0;
    virtual void GetWorkspaceBounds(int &x, int &y, int &wide, int &tall) = 0;
    virtual void GetAbsoluteWindowBounds(int &x, int &y, int &wide, int &tall) = 0;
    virtual void GetProportionalBase( int &width, int &height ) = 0;
    virtual void CalculateMouseVisible() = 0;
    virtual bool NeedKBInput() = 0;
    virtual bool HasCursorPosFunctions() = 0;
    virtual void SurfaceGetCursorPos(int &x, int &y) = 0;
    virtual void SurfaceSetCursorPos(int x, int y) = 0;
    virtual void DrawTexturedPolygon(VGuiVertex *pVertices, int n) = 0;
    virtual int GetFontAscent( HFont font, wchar_t wch ) = 0;
    virtual void SetAllowHTMLJavaScript( bool state ) = 0;
    virtual void SetLanguage( const char* pchLang ) = 0;
    virtual const char* GetLanguage() = 0;
    virtual bool DeleteTextureByID( int id ) = 0;
    virtual void DrawUpdateRegionTextureBGRA( int nTextureID, int x, int y, const unsigned char *pchData, int wide, int tall ) = 0;
    virtual void DrawSetTextureBGRA( int id, const unsigned char *pchData, int wide, int tall ) = 0;
    virtual void CreateBrowser( vgui2::VPANEL panel, IHTMLResponses *pBrowser, bool bPopupWindow, const char *pchUserAgentIdentifier ) = 0;
    virtual void RemoveBrowser( vgui2::VPANEL panel, IHTMLResponses *pBrowser ) = 0;
    virtual IHTMLChromeController *AccessChromeHTMLController() = 0;
    virtual void DrawTexturedRectAdd(int x0, int y0, int x1, int y1) = 0;
    virtual void SetSupportsEsc(bool bSupportsEsc) = 0;
    virtual int GetFontBlur(vgui2::HFont font) = 0;
    virtual bool IsAdditive(vgui2::HFont font) = 0;
    virtual void SetProportionalBase(int width, int height) = 0;
    virtual void GetHDProportionalBase(int &width, int &height) = 0;
    virtual void SetHDProportionalBase(int nWidth, int nHeight) = 0;
};

class IInputInternal : public IBaseInterface
{
public:
    virtual void RunFrame() = 0;
    virtual void UpdateMouseFocus(int x, int y) = 0;
    virtual void PanelDeleted(VPANEL panel) = 0;
    virtual void InternalCursorMoved(int x,int y) = 0;
    virtual void InternalMousePressed(int code) = 0;
    virtual void InternalMouseDoublePressed(int code) = 0;
    virtual void InternalMouseReleased(int code) = 0;
    virtual void InternalMouseWheeled(int delta) = 0;
    virtual void InternalKeyCodePressed(int code) = 0;
    virtual void InternalKeyCodeTyped(int code) = 0;
    virtual void InternalKeyTyped(wchar_t unichar) = 0;
    virtual void InternalKeyCodeReleased(int code) = 0;
    virtual int CreateInputContext() = 0;
    virtual void DestroyInputContext( int context ) = 0;
    virtual void AssociatePanelWithInputContext( int context, VPANEL pRoot ) = 0;
    virtual void ActivateInputContext( int context ) = 0;
    virtual VPANEL GetMouseCapture() = 0;
    virtual bool IsChildOfModalPanel( VPANEL panel ) = 0;
    virtual void ResetInputContext( int context ) = 0;
};

class IScheme : public IBaseInterface
{
public:
    virtual const char *GetResourceString(const char *stringName) = 0;
    virtual void *GetBorder(const char *borderName) = 0;
    virtual HFont GetFont(const char *fontName, bool proportional) = 0;
    virtual int GetColor(const char *colorName, int defaultColor) = 0;
    virtual vgui2::HFont GetFontEx(const char *fontName, bool proportional, bool hdProportional) = 0;
};

class ISchemeManager : public IBaseInterface
{
public:
    virtual HScheme LoadSchemeFromFile( const char *fileName, const char *tag ) = 0;
    virtual void ReloadSchemes() = 0;
    virtual HScheme GetDefaultScheme() = 0;
    virtual HScheme GetScheme( const char *tag ) = 0;
    virtual void *GetImage( const char *imageName, bool hardwareFiltered ) = 0;
    virtual HTexture GetImageID( const char *imageName, bool hardwareFiltered ) = 0;
    virtual IScheme *GetIScheme( HScheme scheme ) = 0;
    virtual void Shutdown( bool full ) = 0;
    virtual int GetProportionalScaledValue( int normalizedValue ) = 0;
    virtual int GetProportionalNormalizedValue( int scaledValue ) = 0;
    virtual float GetProportionalScale() = 0;
    virtual int GetHDProportionalScaledValue(int normalizedValue) = 0;
    virtual int GetHDProportionalNormalizedValue(int scaledValue) = 0;
};

class ILocalize : public IBaseInterface
{
public:
    virtual bool AddFile( void *fileSystem, const char *fileName) = 0;
    virtual void RemoveAll() = 0;
    virtual wchar_t *Find(char const *tokenName) = 0;
    virtual int ConvertANSIToUnicode(const char *ansi, wchar_t *unicode, int unicodeBufferSizeInBytes) = 0;
    virtual int ConvertUnicodeToANSI(const wchar_t *unicode, char *ansi, int ansiBufferSize) = 0;
    virtual unsigned long FindIndex(const char *tokenName) = 0;
    virtual void ConstructString(wchar_t *unicodeOuput, int unicodeBufferSizeInBytes, wchar_t *formatString, int numFormatParameters, ...) = 0;
    virtual const char *GetNameByIndex(unsigned long index) = 0;
    virtual wchar_t *GetValueByIndex(unsigned long index) = 0;
    virtual unsigned long GetFirstStringIndex() = 0;
    virtual unsigned long GetNextStringIndex(unsigned long index) = 0;
    virtual void AddString(const char *tokenName, wchar_t *unicodeString, const char *fileName) = 0;
    virtual void SetValueByIndex(unsigned long index, wchar_t *newValue) = 0;
    virtual bool SaveToFile(void *fileSystem, const char *fileName) = 0;
    virtual int GetLocalizationFileCount() = 0;
    virtual const char *GetLocalizationFileName(int index) = 0;
    virtual const char *GetFileNameByIndex(unsigned long index) = 0;
    virtual void ReloadLocalizationFiles(void *filesystem) = 0;
    virtual void ConstructString(wchar_t *unicodeOutput, int unicodeBufferSizeInBytes, const char *tokenName, void *localizationVariables) = 0;
    virtual void ConstructString(wchar_t *unicodeOutput, int unicodeBufferSizeInBytes, unsigned long unlocalizedTextSymbol, void *localizationVariables) = 0;
};

class ISystem : public IBaseInterface
{
public:
    virtual void Shutdown() = 0;
    virtual void RunFrame() = 0;
    virtual void ShellExecute(const char *command, const char *file) = 0;
    virtual double GetFrameTime() = 0;
    virtual double GetCurrentTime() = 0;
    virtual long GetTimeMillis() = 0;
    virtual int GetClipboardTextCount() = 0;
    virtual void SetClipboardText(const char *text, int textLen) = 0;
    virtual void SetClipboardText(const wchar_t *text, int textLen) = 0;
    virtual int GetClipboardText(int offset, char *buf, int bufLen) = 0;
    virtual int GetClipboardText(int offset, wchar_t *buf, int bufLen) = 0;
    virtual bool SetRegistryString(const char *key, const char *value) = 0;
    virtual bool GetRegistryString(const char *key, char *value, int valueLen) = 0;
    virtual bool SetRegistryInteger(const char *key, int value) = 0;
    virtual bool GetRegistryInteger(const char *key, int &value) = 0;
    virtual void *GetUserConfigFileData(const char *dialogName, int dialogID) = 0;
    virtual void SetUserConfigFile(const char *fileName, const char *pathName) = 0;
    virtual void SaveUserConfigFile() = 0;
    virtual bool SetWatchForComputerUse(bool state) = 0;
    virtual double GetTimeSinceLastUse() = 0;
    virtual int GetAvailableDrives(char *buf, int bufLen) = 0;
    virtual bool CommandLineParamExists(const char *paramName) = 0;
    virtual const char *GetFullCommandLine() = 0;
    virtual bool GetCurrentTimeAndDate(int *year, int *month, int *dayOfWeek, int *day, int *hour, int *minute, int *second) = 0;
    virtual double GetFreeDiskSpace(const char *path) = 0;
    virtual bool CreateShortcut(const char *linkFileName, const char *targetPath, const char *arguments, const char *workingDirectory, const char *iconFile) = 0;
    virtual bool GetShortcutTarget(const char *linkFileName, char *targetPath, char *arguments, int destBufferSizes) = 0;
    virtual bool ModifyShortcutTarget(const char *linkFileName, const char *targetPath, const char *arguments, const char *workingDirectory) = 0;
    virtual bool GetCommandLineParamValue(const char *paramName, char *value, int valueBufferSize) = 0;
    virtual bool DeleteRegistryKey(const char *keyName) = 0;
    virtual const char *GetDesktopFolderPath() = 0;
    virtual int KeyCode_VirtualKeyToVGUI( int keyCode ) = 0;
    virtual int KeyCode_VGUIToVirtualKey( int keyCode ) = 0;
    virtual const char *GetStartMenuFolderPath() = 0;
    virtual const char *GetAllUserDesktopFolderPath() = 0;
    virtual const char *GetAllUserStartMenuFolderPath() = 0;
};

}

#define VGUI_IVGUI_INTERFACE_VERSION_GS "VGUI_ivgui006"
#define VGUI_PANEL_INTERFACE_VERSION_GS "VGUI_Panel007"
#define VGUI_SURFACE_INTERFACE_VERSION_GS "VGUI_Surface026"
#define VGUI_INPUTINTERNAL_INTERFACE_VERSION "VGUI_InputInternal001"
#define VGUI_SCHEME_INTERFACE_VERSION_GS "VGUI_Scheme009"
#define VGUI_LOCALIZE_INTERFACE_VERSION "VGUI_Localize003"
#define VGUI_SYSTEM_INTERFACE_VERSION_GS "VGUI_System009"

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
