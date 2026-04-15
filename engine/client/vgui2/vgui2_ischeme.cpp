/*
 vgui2_ischeme.cpp - ISchemeManager and IScheme implementation for Xash3D FWGS
 Batch 1F: LoadSchemeFromFile with Colors + Fonts sections only
 Uses inline minimal VDF parser to avoid tier1/KeyValues dependency.
 */
#include "vgui2_interfaces.h"
#include "vgui2_host.h"
#include "common.h"
#include "client.h"
#include "../../../../hl1_source_sdk/public/Color.h"
#include <memory>
#include <string>
#include <vector>
#include "vdf_parser.hpp"

#define FONTFLAG_NONE         0
#define FONTFLAG_ITALIC      0x001
#define FONTFLAG_UNDERLINE    0x002
#define FONTFLAG_STRIKEOUT    0x004
#define FONTFLAG_SYMBOL       0x008
#define FONTFLAG_ANTIALIAS    0x010
#define FONTFLAG_GAUSSIANBLUR 0x020
#define FONTFLAG_ROTARY       0x040
#define FONTFLAG_DROPSHADOW   0x080
#define FONTFLAG_ADDITIVE     0x100
#define FONTFLAG_OUTLINE      0x200
#define FONTFLAG_CUSTOM       0x400
#define FONTFLAG_BITMAP       0x800

namespace vgui2
{

void GetCurrentDrawOrigin( int &x, int &y );

class IBorder
{
public:
    virtual void Paint( VPANEL panel ) = 0;
    virtual void Paint( int x0, int y0, int x1, int y1 ) = 0;
    virtual void Paint( int x0, int y0, int x1, int y1, int breakSide, int breakStart, int breakStop ) = 0;
    virtual void SetInset( int left, int top, int right, int bottom ) = 0;
    virtual void GetInset( int &left, int &top, int &right, int &bottom ) = 0;
    virtual void ApplySchemeSettings( IScheme *pScheme, KeyValues *inResourceData ) = 0;
    virtual const char *GetName() = 0;
    virtual void SetName( const char *name ) = 0;

    enum sides_e
    {
        SIDE_LEFT = 0,
        SIDE_TOP = 1,
        SIDE_RIGHT = 2,
        SIDE_BOTTOM = 3
    };
};

class CSchemeManager;

class KeyValues
{
public:
    explicit KeyValues( const char *name = "" )
        : m_name( name ? name : "" )
    {
    }

    KeyValues( KeyValues && ) = default;
    KeyValues &operator=( KeyValues && ) = default;

    KeyValues( const KeyValues & ) = delete;
    KeyValues &operator=( const KeyValues & ) = delete;

    ~KeyValues()
    {
        m_children.clear();
    }

    const char *GetName() const
    {
        return m_name.c_str();
    }

    void SetName( const char *name )
    {
        m_name = name ? name : "";
    }

    void SetString( const char *key, const char *value )
    {
        const char *resolvedValue = value ? value : "";
        if( !key || !key[0] )
        {
            m_value = resolvedValue;
            return;
        }

        for( size_t i = 0; i < m_pairs.size(); ++i )
        {
            if( !Q_stricmp( m_pairs[i].first.c_str(), key ) )
            {
                m_pairs[i].second = resolvedValue;
                return;
            }
        }

        m_pairs.push_back( std::make_pair( std::string( key ), std::string( resolvedValue ) ) );
    }

    const char *GetString( const char *key = "", const char *defaultValue = "" ) const
    {
        if( !key || !key[0] )
            return m_value.empty() ? defaultValue : m_value.c_str();

        for( size_t i = 0; i < m_pairs.size(); ++i )
        {
            if( !Q_stricmp( m_pairs[i].first.c_str(), key ) )
                return m_pairs[i].second.c_str();
        }

        return defaultValue;
    }

    int GetInt( const char *key, int defaultValue = 0 ) const
    {
        const char *value = GetString( key, NULL );
        return value ? atoi( value ) : defaultValue;
    }

    KeyValues *GetFirstSubKey()
    {
        return m_children.empty() ? NULL : m_children.front().get();
    }

    KeyValues *GetNextKey()
    {
        return m_next;
    }

    KeyValues *FindKey( const char *name, bool /*bCreate*/ )
    {
        if( !name || !name[0] )
            return NULL;

        for( size_t i = 0; i < m_children.size(); ++i )
        {
            if( !Q_stricmp( m_children[i]->GetName(), name ) )
                return m_children[i].get();
        }

        return NULL;
    }

    void AddSubKey( KeyValues *child )
    {
        if( !child )
            return;

        if( !m_children.empty() )
            m_children.back()->m_next = child;

        child->m_next = NULL;
        m_children.push_back( std::unique_ptr<KeyValues>( child ) );
    }

    bool LoadFromFile( IFileSystem *filesystem, const char *fileName, const char *pathID );

private:
    std::string m_name;
    std::string m_value;
    std::vector<std::pair<std::string, std::string>> m_pairs;
    std::vector<std::unique_ptr<KeyValues>> m_children;
    KeyValues *m_next = NULL;
};

class IImage
{
public:
    virtual void Paint() = 0;
    virtual void SetPos(int x, int y) = 0;
    virtual void GetContentSize(int &wide, int &tall) = 0;
    virtual void GetSize(int &wide, int &tall) = 0;
    virtual void SetSize(int wide, int tall) = 0;
    virtual void SetColor(Color col) = 0;
    virtual ~IImage() {}
    virtual void SetAdditive(bool) {}
};

namespace
{
bool ReadFileToString( const char *path, std::string &out )
{
    if( !path || !path[0] )
        return false;

    Con_Reportf( "VGUI2: ReadFileToString path='%s'\n", path );
    fs_offset_t size = 0;
    byte *fileData = FS_LoadFile( path, &size, false );
    if( !fileData )
    {
        Con_Reportf( "VGUI2: ReadFileToString FAILED path='%s'\n", path );
        return false;
    }

    out.assign( reinterpret_cast<const char *>( fileData ), static_cast<size_t>( size ) );
    Mem_Free( fileData );
    Con_Reportf( "VGUI2: ReadFileToString OK path='%s' bytes=%d\n", path, (int)size );
    return true;
}

KeyValues *ConvertVdfNodeToKeyValues( const tyti::vdf::multikey_object &node )
{
    KeyValues *kv = new KeyValues( node.name.c_str() );

    for( const auto &attrib : node.attribs )
        kv->SetString( attrib.first.c_str(), attrib.second.c_str() );

    for( const auto &childEntry : node.childs )
    {
        if( !childEntry.second )
            continue;
        kv->AddSubKey( ConvertVdfNodeToKeyValues( *childEntry.second ) );
    }

    return kv;
}
}

bool KeyValues::LoadFromFile( IFileSystem *filesystem, const char *fileName, const char *pathID )
{
    std::string fileData;
    (void)filesystem;
    (void)pathID;

    if( !ReadFileToString( fileName, fileData ) )
        return false;

    bool ok = false;
    tyti::vdf::Options options;
    options.ignore_includes = false;
    options.ignore_all_platform_conditionals = false;
    options.strip_escape_symbols = true;

    tyti::vdf::multikey_object root =
        tyti::vdf::read<tyti::vdf::multikey_object>( fileData.begin(), fileData.end(), &ok, options );
    if( !ok )
        return false;

    std::unique_ptr<KeyValues> converted( ConvertVdfNodeToKeyValues( root ) );
    if( !converted )
        return false;

    *this = std::move( *converted );
    return true;
}

class CTextureImage : public IImage
{
public:
    explicit CTextureImage(const char *imageName)
    {
        Q_strncpy(m_name, imageName ? imageName : "", sizeof(m_name) - 1);
        m_color.SetColor(255, 255, 255, 255);
        LoadTexture();
    }

    void Paint() override
    {
        if (m_textureId == 0)
            return;

        int originX = 0;
        int originY = 0;
        GetCurrentDrawOrigin( originX, originY );

        ref.dllFuncs.Color4ub((byte)m_color.r(), (byte)m_color.g(), (byte)m_color.b(), (byte)m_color.a());
        ref.dllFuncs.R_DrawStretchPic((float)(m_x + originX), (float)(m_y + originY), (float)m_wide, (float)m_tall,
            0.0f, 0.0f, 1.0f, 1.0f, m_textureId);
    }

    void SetPos(int x, int y) override
    {
        m_x = x;
        m_y = y;
    }

    void GetContentSize(int &wide, int &tall) override
    {
        wide = m_wide;
        tall = m_tall;
    }

    void GetSize(int &wide, int &tall) override
    {
        wide = m_wide;
        tall = m_tall;
    }

    void SetSize(int wide, int tall) override
    {
        m_wide = wide;
        m_tall = tall;
    }

    void SetColor(Color col) override
    {
        m_color = col;
    }

private:
    void LoadTexture()
    {
        if (!m_name[0])
            return;

        m_textureId = ref.dllFuncs.GL_LoadTexture(m_name, NULL, 0, TF_IMAGE | TF_NOMIPMAP);
        if (m_textureId == 0)
        {
            char withTga[160];
            Q_snprintf(withTga, sizeof(withTga), "%s.tga", m_name);
            m_textureId = ref.dllFuncs.GL_LoadTexture(withTga, NULL, 0, TF_IMAGE | TF_NOMIPMAP);
            Con_Reportf("VGUI2: Image load name='%s' fallback='%s' tex=%d\n", m_name, withTga, m_textureId);
        }
        else
        {
            Con_Reportf("VGUI2: Image load name='%s' tex=%d\n", m_name, m_textureId);
        }
    }

    char m_name[128] = "";
    int m_textureId = 0;
    int m_x = 0;
    int m_y = 0;
    int m_wide = 64;
    int m_tall = 64;
    Color m_color;
};

struct FontEntry_t
{
    HFont font;
    char name[64];
    char face[64];
    int tall;
    int weight;
    int flags;
};

struct ColorEntry_t
{
    int r, g, b, a;
    char name[64];
};

struct ResourceStringEntry_t
{
    char name[128];
    char value[128];
};

struct BorderSide_t
{
    bool valid;
    Color color;
    int offset[2];

    BorderSide_t()
        : valid(false)
    {
        offset[0] = offset[1] = 0;
        color.SetColor( 0, 0, 0, 0 );
    }
};

class CSchemeBorder : public IBorder
{
public:
    CSchemeBorder()
    {
        Reset();
    }

    void Reset()
    {
        m_name[0] = '\0';
        m_inset[0] = m_inset[1] = m_inset[2] = m_inset[3] = 0;
        for( int i = 0; i < 4; ++i )
            m_sides[i] = BorderSide_t();
    }

    void SetName( const char *name ) override
    {
        Q_strncpy( m_name, name ? name : "", sizeof( m_name ) - 1 );
        m_name[sizeof( m_name ) - 1] = '\0';
    }

    const char *GetName() override
    {
        return m_name;
    }

    void SetInset( int left, int top, int right, int bottom ) override
    {
        m_inset[0] = left;
        m_inset[1] = top;
        m_inset[2] = right;
        m_inset[3] = bottom;
    }

    void GetInset( int &left, int &top, int &right, int &bottom ) override
    {
        left = m_inset[0];
        top = m_inset[1];
        right = m_inset[2];
        bottom = m_inset[3];
    }

    void ApplySchemeSettings( IScheme *, KeyValues * ) override {}

    void Paint( VPANEL panel ) override
    {
        int wide = 0, tall = 0;
        CVGui2Interfaces *interfaces = VGui2_GetInterfacesClient();
        if( interfaces && interfaces->GetIPanel() )
            interfaces->GetIPanel()->GetSize( panel, wide, tall );
        Paint( 0, 0, wide, tall );
    }

    void Paint( int x0, int y0, int x1, int y1 ) override
    {
        PaintSides( x0, y0, x1, y1, -1, 0, 0 );
    }

    void Paint( int x0, int y0, int x1, int y1, int breakSide, int breakStart, int breakStop ) override
    {
        PaintSides( x0, y0, x1, y1, breakSide, breakStart, breakStop );
    }

    void SetSide( int side, const Color &color, int offsetX, int offsetY )
    {
        if( side < 0 || side >= 4 )
            return;

        m_sides[side].valid = true;
        m_sides[side].color = color;
        m_sides[side].offset[0] = offsetX;
        m_sides[side].offset[1] = offsetY;
    }

private:
    void PaintSides( int x0, int y0, int x1, int y1, int breakSide, int breakStart, int breakStop )
    {
        PaintSide( IBorder::SIDE_LEFT, x0, y0, x1, y1, breakSide, breakStart, breakStop );
        PaintSide( IBorder::SIDE_TOP, x0, y0, x1, y1, breakSide, breakStart, breakStop );
        PaintSide( IBorder::SIDE_RIGHT, x0, y0, x1, y1, breakSide, breakStart, breakStop );
        PaintSide( IBorder::SIDE_BOTTOM, x0, y0, x1, y1, breakSide, breakStart, breakStop );
    }

    void PaintSide( int side, int x0, int y0, int x1, int y1, int breakSide, int breakStart, int breakStop )
    {
        if( side < 0 || side >= 4 )
            return;

        const BorderSide_t &bs = m_sides[side];
        if( !bs.valid )
            return;

        CVGui2Interfaces *interfaces = VGui2_GetInterfacesClient();
        vgui2::ISurface *surface = interfaces ? interfaces->GetISurface() : NULL;
        if( !surface )
            return;

        surface->DrawSetColor( bs.color );

        const int ox = bs.offset[0];
        const int oy = bs.offset[1];

        switch( side )
        {
        case IBorder::SIDE_LEFT:
        {
            int x = x0 + ox;
            if( breakSide == side )
            {
                if( breakStart > y0 )
                    surface->DrawFilledRect( x, y0 + oy, x + 1, breakStart );
                if( breakStop < y1 )
                    surface->DrawFilledRect( x, breakStop, x + 1, y1 - oy );
            }
            else
            {
                surface->DrawFilledRect( x, y0 + oy, x + 1, y1 - oy );
            }
            break;
        }
        case IBorder::SIDE_TOP:
        {
            int y = y0 + oy;
            if( breakSide == side )
            {
                if( breakStart > x0 )
                    surface->DrawFilledRect( x0 + ox, y, breakStart, y + 1 );
                if( breakStop < x1 )
                    surface->DrawFilledRect( breakStop, y, x1 - ox, y + 1 );
            }
            else
            {
                surface->DrawFilledRect( x0 + ox, y, x1 - ox, y + 1 );
            }
            break;
        }
        case IBorder::SIDE_RIGHT:
        {
            int x = x1 - 1 + ox;
            if( breakSide == side )
            {
                if( breakStart > y0 )
                    surface->DrawFilledRect( x, y0 + oy, x + 1, breakStart );
                if( breakStop < y1 )
                    surface->DrawFilledRect( x, breakStop, x + 1, y1 - oy );
            }
            else
            {
                surface->DrawFilledRect( x, y0 + oy, x + 1, y1 - oy );
            }
            break;
        }
        case IBorder::SIDE_BOTTOM:
        {
            int y = y1 - 1 + oy;
            if( breakSide == side )
            {
                if( breakStart > x0 )
                    surface->DrawFilledRect( x0 + ox, y, breakStart, y + 1 );
                if( breakStop < x1 )
                    surface->DrawFilledRect( breakStop, y, x1 - ox, y + 1 );
            }
            else
            {
                surface->DrawFilledRect( x0 + ox, y, x1 - ox, y + 1 );
            }
            break;
        }
        }
    }

    char m_name[64];
    int m_inset[4];
    BorderSide_t m_sides[4];
};

class CScheme : public IScheme
{
public:
    CScheme()
    {
        for( int i = 0; i < ARRAYSIZE( m_Borders ); ++i )
            m_Borders[i] = NULL;
        Reset();
    }

    void Reset()
    {
        m_nFonts = 0;
        m_nColors = 0;
        m_nResourceStrings = 0;
        m_nBorders = 0;
        m_szTag[0] = '\0';

        for( int i = 0; i < ARRAYSIZE( m_Fonts ); i++ )
        {
            m_Fonts[i].font = INVALID_HFONT;
            m_Fonts[i].name[0] = '\0';
            m_Fonts[i].face[0] = '\0';
            m_Fonts[i].tall = 0;
            m_Fonts[i].weight = 0;
            m_Fonts[i].flags = 0;
        }

        for( int i = 0; i < ARRAYSIZE( m_Colors ); i++ )
        {
            m_Colors[i].name[0] = '\0';
            m_Colors[i].r = 0;
            m_Colors[i].g = 0;
            m_Colors[i].b = 0;
            m_Colors[i].a = 0;
        }

        for( int i = 0; i < ARRAYSIZE( m_ResourceStrings ); i++ )
        {
            m_ResourceStrings[i].name[0] = '\0';
            m_ResourceStrings[i].value[0] = '\0';
        }

        for( int i = 0; i < ARRAYSIZE( m_Borders ); i++ )
        {
            if( m_Borders[i] )
            {
                delete m_Borders[i];
                m_Borders[i] = NULL;
            }
        }

        SetDefaultResources();
    }

    const char *GetResourceString(const char *stringName) override;
    IBorder *GetBorder(const char *borderName) override;
    HFont GetFont(const char *fontName, bool proportional) override;
    void GetColorInto( const char *colorName, const Color &defaultColor, Color *pOutColor ) override;
    HFont GetFontEx(const char *fontName, bool proportional, bool hdProportional) override
        { return GetFont(fontName, proportional); }

    void SetResourceString( const char *name, const char *value );
    void SetBorder( CSchemeBorder *border );
    void SetDefaultResources();

    FontEntry_t m_Fonts[32];
    int m_nFonts;
    ColorEntry_t m_Colors[64];
    int m_nColors;
    ResourceStringEntry_t m_ResourceStrings[128];
    int m_nResourceStrings;
    CSchemeBorder *m_Borders[64];
    int m_nBorders;
    char m_szTag[MAX_QPATH];
};

HFont CScheme::GetFont(const char *fontName, bool /*proportional*/)
{
    for( int i = 0; i < m_nFonts; i++ )
    {
        if( !Q_stricmp(m_Fonts[i].name, fontName) || !Q_stricmp(m_Fonts[i].face, fontName) )
            return m_Fonts[i].font;
    }
    return INVALID_HFONT;
}

void CScheme::GetColorInto( const char *colorName, const Color &defaultColor, Color *pOutColor )
{
    if( !pOutColor )
        return;

    *pOutColor = defaultColor;
    Con_Reportf("VGUI2: CScheme::GetColor this=%p color='%s' default=(%d,%d,%d,%d)\n",
        this,
        colorName ? colorName : "<null>",
        defaultColor.r(), defaultColor.g(), defaultColor.b(), defaultColor.a());

    for( int i = 0; i < m_nColors; i++ )
    {
        if( !Q_strcmp(m_Colors[i].name, colorName) )
        {
            ColorEntry_t &c = m_Colors[i];
            *pOutColor = Color( c.r, c.g, c.b, c.a );
            return;
        }
    }
}

const char *CScheme::GetResourceString( const char *stringName )
{
    if( !stringName || !stringName[0] )
        return "";

    for( int i = 0; i < m_nResourceStrings; ++i )
    {
        if( !Q_stricmp( m_ResourceStrings[i].name, stringName ) )
            return m_ResourceStrings[i].value;
    }

    Con_Reportf( "VGUI2: GetResourceString MISS name='%s'\n", stringName );
    return "";
}

IBorder *CScheme::GetBorder( const char *borderName )
{
    if( !borderName || !borderName[0] )
        return NULL;

    for( int i = 0; i < m_nBorders; ++i )
    {
        if( m_Borders[i] && !Q_stricmp( m_Borders[i]->GetName(), borderName ) )
        {
            Con_Reportf( "VGUI2: GetBorder HIT name='%s' border=%p\n", borderName, m_Borders[i] );
            return m_Borders[i];
        }
    }

    if( !Q_stricmp( borderName, "RaisedBorder" ) )
        return GetBorder( "ButtonBorder" );
    if( !Q_stricmp( borderName, "LoweredBorder" ) )
        return GetBorder( "ButtonDepressedBorder" );
    if( !Q_stricmp( borderName, "DepressedButtonBorder" ) )
        return GetBorder( "ButtonDepressedBorder" );
    if( !Q_stricmp( borderName, "ScrollBarSliderBorder" ) )
        return GetBorder( "ButtonBorder" );

    Con_Reportf( "VGUI2: GetBorder MISS name='%s'\n", borderName );
    return NULL;
}

void CScheme::SetResourceString( const char *name, const char *value )
{
    if( !name || !name[0] || !value )
        return;

    for( int i = 0; i < m_nResourceStrings; ++i )
    {
        if( !Q_stricmp( m_ResourceStrings[i].name, name ) )
        {
            Q_strncpy( m_ResourceStrings[i].value, value, sizeof( m_ResourceStrings[i].value ) - 1 );
            m_ResourceStrings[i].value[sizeof( m_ResourceStrings[i].value ) - 1] = '\0';
            return;
        }
    }

    if( m_nResourceStrings >= ARRAYSIZE( m_ResourceStrings ) )
        return;

    Q_strncpy( m_ResourceStrings[m_nResourceStrings].name, name, sizeof( m_ResourceStrings[m_nResourceStrings].name ) - 1 );
    m_ResourceStrings[m_nResourceStrings].name[sizeof( m_ResourceStrings[m_nResourceStrings].name ) - 1] = '\0';
    Q_strncpy( m_ResourceStrings[m_nResourceStrings].value, value, sizeof( m_ResourceStrings[m_nResourceStrings].value ) - 1 );
    m_ResourceStrings[m_nResourceStrings].value[sizeof( m_ResourceStrings[m_nResourceStrings].value ) - 1] = '\0';
    ++m_nResourceStrings;
}

void CScheme::SetBorder( CSchemeBorder *border )
{
    if( !border || m_nBorders >= ARRAYSIZE( m_Borders ) )
        return;

    m_Borders[m_nBorders++] = border;
}

void CScheme::SetDefaultResources()
{
    SetResourceString( "FrameTitleBar.Font", "Title" );
    SetResourceString( "FrameTitleBar.SmallFont", "DefaultSmall" );
    SetResourceString( "Frame.TransitionEffectTime", "0.0" );
    SetResourceString( "Frame.FocusTransitionEffectTime", "0.0" );
    SetResourceString( "Frame.ClientInsetX", "5" );
    SetResourceString( "Frame.ClientInsetY", "5" );
    SetResourceString( "Frame.TitleTextInsetX", "28" );
    SetResourceString( "Frame.AutoSnapRange", "10" );
    SetResourceString( "FrameSystemButton.Icon", "resource/icon_steam" );
    SetResourceString( "FrameSystemButton.DisabledIcon", "resource/icon_steam_disabled" );
    SetResourceString( "PropertySheet.TransitionEffectTime", "0.0" );
    SetResourceString( "HTML.SearchInsetY", "5" );
    SetResourceString( "HTML.SearchInsetX", "5" );
    SetResourceString( "HTML.SearchTall", "24" );
    SetResourceString( "HTML.SearchWide", "150" );
    SetResourceString( "HTML.SearchAnimationTime", "0.0" );
    SetResourceString( "RichText.InsetX", "0" );
    SetResourceString( "RichText.InsetY", "0" );
    SetResourceString( "ScrollBar.Wide", "15" );
    SetResourceString( "Menu.TextInset", "6" );
}

static void CollectResourceStrings( CScheme *scheme, KeyValues *node, const char *prefix )
{
    if( !scheme || !node )
        return;

    for( KeyValues *child = node->GetFirstSubKey(); child; child = child->GetNextKey() )
    {
        const char *childName = child->GetName();
        if( !childName || !childName[0] )
            continue;

        char key[256];
        if( prefix && prefix[0] )
            Q_snprintf( key, sizeof( key ), "%s.%s", prefix, childName );
        else
            Q_strncpy( key, childName, sizeof( key ) - 1 );
        key[sizeof( key ) - 1] = '\0';

        if( child->GetFirstSubKey() )
        {
            CollectResourceStrings( scheme, child, key );
        }
        else
        {
            scheme->SetResourceString( key, child->GetString( "" ) );
        }
    }
}

static bool ParseIntPair( const char *text, int &a, int &b )
{
    a = b = 0;
    if( !text || !text[0] )
        return false;
    return sscanf( text, "%d %d", &a, &b ) >= 1;
}

static bool ParseIntQuad( const char *text, int values[4] )
{
    values[0] = values[1] = values[2] = values[3] = 0;
    if( !text || !text[0] )
        return false;
    return sscanf( text, "%d %d %d %d", &values[0], &values[1], &values[2], &values[3] ) >= 1;
}

static int ParseBorderSideIndex( const char *name )
{
    if( !name )
        return -1;
    if( !Q_stricmp( name, "Left" ) )
        return IBorder::SIDE_LEFT;
    if( !Q_stricmp( name, "Top" ) )
        return IBorder::SIDE_TOP;
    if( !Q_stricmp( name, "Right" ) )
        return IBorder::SIDE_RIGHT;
    if( !Q_stricmp( name, "Bottom" ) )
        return IBorder::SIDE_BOTTOM;
    return -1;
}

static void ParseBorderNode( CScheme *scheme, KeyValues *borderNode )
{
    if( !scheme || !borderNode )
        return;

    CSchemeBorder *border = new CSchemeBorder();
    border->SetName( borderNode->GetName() );

    int inset[4] = { 0, 0, 0, 0 };
    ParseIntQuad( borderNode->GetString( "inset", "" ), inset );
    border->SetInset( inset[0], inset[1], inset[2], inset[3] );

    for( KeyValues *sideNode = borderNode->GetFirstSubKey(); sideNode; sideNode = sideNode->GetNextKey() )
    {
        int sideIndex = ParseBorderSideIndex( sideNode->GetName() );
        if( sideIndex < 0 )
            continue;

        KeyValues *segmentNode = sideNode->GetFirstSubKey();
        if( !segmentNode )
            continue;

        const char *colorName = segmentNode->GetString( "color", "" );
        Color color = scheme->GetColor( colorName, Color( 255, 255, 255, 255 ) );

        int offsetX = 0;
        int offsetY = 0;
        ParseIntPair( segmentNode->GetString( "offset", "" ), offsetX, offsetY );

        border->SetSide( sideIndex, color, offsetX, offsetY );
    }

    scheme->SetBorder( border );
}

static int ParseFontFlags( KeyValues *variant )
{
    int flags = 0;
    if( variant->GetInt( "italic", 0 ) )
        flags |= FONTFLAG_ITALIC;
    if( variant->GetInt( "underline", 0 ) )
        flags |= FONTFLAG_UNDERLINE;
    if( variant->GetInt( "strikeout", 0 ) )
        flags |= FONTFLAG_STRIKEOUT;
    if( variant->GetInt( "symbol", 0 ) )
        flags |= FONTFLAG_SYMBOL;
    if( variant->GetInt( "antialias", 0 ) )
        flags |= FONTFLAG_ANTIALIAS;
    if( variant->GetInt( "gaussianblur", 0 ) )
        flags |= FONTFLAG_GAUSSIANBLUR;
    if( variant->GetInt( "rotary", 0 ) )
        flags |= FONTFLAG_ROTARY;
    if( variant->GetInt( "dropshadow", 0 ) )
        flags |= FONTFLAG_DROPSHADOW;
    if( variant->GetInt( "additive", 0 ) )
        flags |= FONTFLAG_ADDITIVE;
    if( variant->GetInt( "outline", 0 ) )
        flags |= FONTFLAG_OUTLINE;
    if( variant->GetInt( "custom", 0 ) )
        flags |= FONTFLAG_CUSTOM;
    if( variant->GetInt( "bitmap", 0 ) )
        flags |= FONTFLAG_BITMAP;
    return flags;
}

static bool FontVariantMatchesScreen( KeyValues *variant, int screenTall )
{
    const char *yres = variant->GetString( "yres", "" );
    if( !yres || !yres[0] )
        return true;

    int low = 0, high = 0;
    if( sscanf( yres, "%d %d", &low, &high ) == 2 )
        return screenTall >= low && screenTall <= high;

    return true;
}

static HFont AddSchemeFont(CSchemeManager *manager, ISurface *surface, const char *fontName, int tall, int weight, int flags);

static void ParseFontFamily( CSchemeManager *manager, CScheme *scheme, ISurface *surface, KeyValues *familyNode )
{
    if( !manager || !scheme || !surface || !familyNode )
        return;

    const char *familyName = familyNode->GetName();
    if( !familyName || !familyName[0] )
        return;

    KeyValues *firstVariant = NULL;
    KeyValues *selectedVariant = NULL;
    int screenTall = ( refState.height > 0 ) ? refState.height : 480;

    for( KeyValues *variant = familyNode->GetFirstSubKey(); variant; variant = variant->GetNextKey() )
    {
        if( !variant->GetFirstSubKey() )
            continue;

        if( !firstVariant )
            firstVariant = variant;

        if( FontVariantMatchesScreen( variant, screenTall ) )
        {
            selectedVariant = variant;
            break;
        }
    }

    if( !selectedVariant )
        selectedVariant = firstVariant;
    if( !selectedVariant )
        return;

    const char *faceName = selectedVariant->GetString( "name", familyName );
    int tall = selectedVariant->GetInt( "tall", 12 );
    int weight = selectedVariant->GetInt( "weight", 400 );
    int flags = ParseFontFlags( selectedVariant );

    HFont h = AddSchemeFont( manager, surface, faceName, tall, weight, flags );
    if( scheme->m_nFonts < ARRAYSIZE( scheme->m_Fonts ) )
    {
        FontEntry_t &entry = scheme->m_Fonts[scheme->m_nFonts++];
        entry.font = h;
        Q_strncpy( entry.name, familyName, sizeof( entry.name ) - 1 );
        entry.name[sizeof( entry.name ) - 1] = '\0';
        Q_strncpy( entry.face, faceName ? faceName : "", sizeof( entry.face ) - 1 );
        entry.face[sizeof( entry.face ) - 1] = '\0';
        entry.tall = tall;
        entry.weight = weight;
        entry.flags = flags;
    }

    Con_Reportf( "VGUI2: Parsed font family '%s' face='%s' tall=%d weight=%d flags=0x%x -> hfont=%lu\n",
        familyName, faceName ? faceName : "<null>", tall, weight, flags, (unsigned long)h );
}

static void ParseFontsSection( CSchemeManager *manager, CScheme *scheme, ISurface *surface, KeyValues *fontsNode )
{
    if( !manager || !scheme || !surface || !fontsNode )
        return;

    for( KeyValues *family = fontsNode->GetFirstSubKey(); family; family = family->GetNextKey() )
    {
        ParseFontFamily( manager, scheme, surface, family );
    }
}

// Minimal VDF parser for .res files
// Handles: key "value" blocks and nested { } sections
// Skips platform conditionals [$WIN32], [$LINUX], etc.

static const char *SkipWhitespace(const char *p)
{
    while( *p && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n') )
        p++;
    return p;
}

static const char *SkipLine(const char *p)
{
    while( *p && *p != '\n' )
        p++;
    return p;
}

static int ParseVDFValue(const char *&p, char *buf, size_t bufsize)
{
    p = SkipWhitespace(p);
    if( *p != '"' )
    {
        buf[0] = '\0';
        return 0;
    }
    p++;
    size_t i = 0;
    while( *p && *p != '"' && i < bufsize - 1 )
        buf[i++] = *p++;
    buf[i] = '\0';
    if( *p == '"' )
        p++;
    return 1;
}

static int ParseVDFKey(const char *&p, char *buf, size_t bufsize)
{
    p = SkipWhitespace(p);
    if( *p == '"' )
        return ParseVDFValue(p, buf, bufsize);
    // unquoted key
    size_t i = 0;
    while( *p && *p > ' ' && *p != '{' && *p != '}' && *p != '"' && i < bufsize - 1 )
        buf[i++] = *p++;
    buf[i] = '\0';
    return (i > 0) ? 1 : 0;
}

static int IsConditional(const char *key)
{
    return (key[0] == '[') ? 1 : 0;
}

static int StringMatchesCondition(const char *cond, const char *platform)
{
    // cond is like "$WIN32" or "!$POSIX"
    if( cond[0] == '!' )
    {
        return Q_strcmp(cond + 1, platform) != 0;
    }
    return Q_strcmp(cond, platform) == 0;
}

static HFont AddSchemeFont(CSchemeManager *manager, ISurface *surface, const char *fontName, int tall, int weight, int flags);

class CSchemeManager : public ISchemeManager
{
public:
    HScheme LoadSchemeFromFile(const char *fileName, const char *tag) override;
    void ReloadSchemes() override {}
    HScheme GetDefaultScheme() override { return m_defaultScheme; }
    HScheme GetScheme(const char *tag) override;
    IImage *GetImage(const char *imageName, bool hardwareFiltered) override;
    HTexture GetImageID(const char *, bool) override { return 0; }
    IScheme *GetIScheme(HScheme scheme) override;
    void Shutdown(bool) override {}
    int GetProportionalScaledValue(int v) override { return (int)(v * GetProportionalScale()); }
    int GetProportionalNormalizedValue(int v) override
    {
        float scale = GetProportionalScale();
        return (scale > 0.0f) ? (int)(v / scale) : v;
    }
    float GetProportionalScale() override
    {
        const int baseTall = 480;
        if (refState.height <= 0)
            return 1.0f;
        return (float)refState.height / (float)baseTall;
    }
    int GetHDProportionalScaledValue(int v) override { return GetProportionalScaledValue(v); }
    int GetHDProportionalNormalizedValue(int v) override { return GetProportionalNormalizedValue(v); }

private:
    HFont AddFont(ISurface *surface, const char *fontName, int tall, int weight, int flags);
    int ParseSchemeFile(CScheme *scheme, ISurface *surface, const char *buffer, size_t buflen);

    CScheme m_Schemes[8];
    int m_nSchemes;
    HScheme m_defaultScheme = 0;
    HFont m_nextFontHandle = 1;
    char m_schemeTags[8][MAX_QPATH];
    CTextureImage *m_Images[64] = {};
    char m_ImageNames[64][MAX_QPATH] = {};
};

static HFont AddSchemeFont(CSchemeManager *manager, ISurface *surface, const char *fontName, int tall, int weight, int flags)
{
    if( !surface )
        return INVALID_HFONT;

    HFont h = surface->CreateFont();
    if( h == INVALID_HFONT )
        return INVALID_HFONT;

    if( !surface->AddGlyphSetToFont( h, fontName, tall, weight, 0, 0, flags, 0, 0xFFFF ) )
        return INVALID_HFONT;

    return h;
}

IImage *CSchemeManager::GetImage(const char *imageName, bool)
{
    if (!imageName || !imageName[0])
        return NULL;

    for (int i = 0; i < ARRAYSIZE(m_Images); ++i)
    {
        if (m_Images[i] && !Q_strcmp(m_ImageNames[i], imageName))
        {
            Con_Reportf("VGUI2: GetImage cache HIT name='%s' image=%p\n", imageName, m_Images[i]);
            return m_Images[i];
        }
    }

    for (int i = 0; i < ARRAYSIZE(m_Images); ++i)
    {
        if (!m_Images[i])
        {
            m_Images[i] = new CTextureImage(imageName);
            Q_strncpy(m_ImageNames[i], imageName, sizeof(m_ImageNames[i]) - 1);
            Con_Reportf("VGUI2: GetImage created name='%s' image=%p\n", imageName, m_Images[i]);
            return m_Images[i];
        }
    }

    Con_Reportf("VGUI2: GetImage cache full for '%s'\n", imageName);
    return NULL;
}

HFont CSchemeManager::AddFont(ISurface *surface, const char *fontName, int tall, int weight, int flags)
{
    HFont h = surface->CreateFont();
    if( h == INVALID_HFONT )
        return INVALID_HFONT;

    if( !surface->AddGlyphSetToFont(h, fontName, tall, weight,
        0, 0, flags, 0, 0xFFFF) )
    {
        return INVALID_HFONT;
    }

    return h;
}

int CSchemeManager::ParseSchemeFile(CScheme *scheme, ISurface *surface, const char *buffer, size_t buflen)
{
    const char *p = buffer;
    int fontsAdded = 0;
    int colorsAdded = 0;
    char key[256];
    char value[256];

    enum ParseState
    {
        PS_TOP,
        PS_COLORS,
        PS_FONTS,
        PS_FONTFAMILY,
        PS_FONTVARIANT
    };

    ParseState state = PS_TOP;
    char currentFamily[64] = {0};
    int currentFamilyTall = 12;
    int currentFamilyWeight = 400;
    int currentFamilyFlags = 0;

    while( *p && (p - buffer) < (int)buflen )
    {
        p = SkipWhitespace(p);
        if( !*p )
            break;

        // Skip comments
        if( p[0] == '/' && p[1] == '/' )
        {
            p = SkipLine(p);
            continue;
        }

        // Handle opening brace
        if( *p == '{' )
        {
            p++;
            continue;
        }

        // Handle closing brace - pop state
        if( *p == '}' )
        {
            p++;
            if( state == PS_FONTVARIANT )
                state = PS_FONTFAMILY;
            else if( state == PS_FONTFAMILY )
                state = PS_FONTS;
            else if( state == PS_FONTS )
                state = PS_TOP;
            else if( state == PS_COLORS )
                state = PS_TOP;
            continue;
        }

        // Try to parse a key
        if( !ParseVDFKey(p, key, sizeof(key)) )
        {
            p = SkipLine(p);
            continue;
        }

        // Skip conditionals like [$WIN32]
        if( IsConditional(key) )
        {
            // Skip this entire line/block
            p = SkipLine(p);
            continue;
        }

        // Check for section headers
        if( !Q_strcmp(key, "Colors") )
        {
            state = PS_COLORS;
            continue;
        }
        if( !Q_strcmp(key, "Fonts") )
        {
            state = PS_FONTS;
            continue;
        }

        // Try to parse value
        if( !ParseVDFValue(p, value, sizeof(value)) )
        {
            p = SkipLine(p);
            continue;
        }

        // Handle value based on state
        if( state == PS_COLORS )
        {
            // Color entry: "ColorName" "r g b a"
            if( scheme->m_nColors < 64 )
            {
                ColorEntry_t *c = &scheme->m_Colors[scheme->m_nColors];
                Q_strncpy(c->name, key, sizeof(c->name) - 1);
                c->name[sizeof(c->name) - 1] = '\0';
                int r = 0, g = 0, b = 0, a = 255;
                sscanf(value, "%d %d %d %d", &r, &g, &b, &a);
                c->r = r;
                c->g = g;
                c->b = b;
                c->a = a;
                scheme->m_nColors++;
                colorsAdded++;
            }
        }
        else if( state == PS_FONTS )
        {
            // Font family: "FamilyName" { ... }
            // value should be "{" for a section header
            if( !Q_strcmp(value, "{") )
            {
                Q_strncpy(currentFamily, key, sizeof(currentFamily) - 1);
                currentFamily[sizeof(currentFamily) - 1] = '\0';
                state = PS_FONTFAMILY;
                currentFamilyTall = 12;
                currentFamilyWeight = 400;
                currentFamilyFlags = 0;
            }
        }
        else if( state == PS_FONTFAMILY )
        {
            // Font property: "name" "Value" or "tall" "12" etc.
            if( !Q_strcmp(key, "name") )
            {
                Q_strncpy(currentFamily, value, sizeof(currentFamily) - 1);
                currentFamily[sizeof(currentFamily) - 1] = '\0';
            }
            else if( !Q_strcmp(key, "tall") )
            {
                currentFamilyTall = atoi(value);
            }
            else if( !Q_strcmp(key, "weight") )
            {
                currentFamilyWeight = atoi(value);
            }
            else if( !Q_strcmp(key, "antialias") )
            {
                if( atoi(value) )
                    currentFamilyFlags |= FONTFLAG_ANTIALIAS;
            }
            else if( !Q_strcmp(key, "underline") )
            {
                if( atoi(value) )
                    currentFamilyFlags |= FONTFLAG_UNDERLINE;
            }
            else if( !Q_strcmp(key, "italic") )
            {
                if( atoi(value) )
                    currentFamilyFlags |= FONTFLAG_ITALIC;
            }
            else if( !Q_strcmp(key, "strikeout") )
            {
                if( atoi(value) )
                    currentFamilyFlags |= FONTFLAG_STRIKEOUT;
            }
            else if( !Q_strcmp(key, "symbol") )
            {
                if( atoi(value) )
                    currentFamilyFlags |= FONTFLAG_SYMBOL;
            }
            else if( !Q_strcmp(value, "{") )
            {
                // This is a font variant entry like "1" { "name" "Verdana" ... }
                state = PS_FONTVARIANT;
            }
        }
        else if( state == PS_FONTVARIANT )
        {
            // We're inside a variant block - parse font properties
            if( !Q_strcmp(key, "name") )
            {
                Q_strncpy(currentFamily, value, sizeof(currentFamily) - 1);
                currentFamily[sizeof(currentFamily) - 1] = '\0';
            }
            else if( !Q_strcmp(key, "tall") )
            {
                currentFamilyTall = atoi(value);
            }
            else if( !Q_strcmp(key, "weight") )
            {
                currentFamilyWeight = atoi(value);
            }
            else if( !Q_strcmp(key, "antialias") )
            {
                if( atoi(value) )
                    currentFamilyFlags |= FONTFLAG_ANTIALIAS;
            }
            else if( !Q_strcmp(key, "underline") )
            {
                if( atoi(value) )
                    currentFamilyFlags |= FONTFLAG_UNDERLINE;
            }
            else if( !Q_strcmp(key, "italic") )
            {
                if( atoi(value) )
                    currentFamilyFlags |= FONTFLAG_ITALIC;
            }
            else if( !Q_strcmp(key, "strikeout") )
            {
                if( atoi(value) )
                    currentFamilyFlags |= FONTFLAG_STRIKEOUT;
            }
            else if( !Q_strcmp(key, "symbol") )
            {
                if( atoi(value) )
                    currentFamilyFlags |= FONTFLAG_SYMBOL;
            }
            else
            {
                // Non-standard key inside variant - might be a number (resolution variant id)
                // If it has nested content, parse it
                if( !Q_strcmp(value, "{") )
                {
                    // nested variant inside current one... reset tall/weight/flags to defaults
                    currentFamilyTall = 12;
                    currentFamilyWeight = 400;
                    currentFamilyFlags = 0;
                }
            }
        }
    }

    return fontsAdded + colorsAdded;
}

HScheme CSchemeManager::LoadSchemeFromFile(const char *fileName, const char *tag)
{
    // Duplicate-tag guard
    for( int i = 0; i < m_nSchemes; i++ )
    {
        if( !Q_strcmp(m_schemeTags[i], tag) )
        {
            HScheme h = (HScheme)(i + 1);
            Con_Reportf("VGUI2: LoadSchemeFromFile(%s, %s) -> existing scheme=%lu\n",
                fileName, tag, (unsigned long)h);
            return h;
        }
    }

    if( m_nSchemes >= 8 )
    {
        Con_Reportf("VGUI2: LoadSchemeFromFile(%s, %s) -> max schemes reached\n", fileName, tag);
        return 0;
    }

    ISurface *surface = VGui2_GetInterfacesClient()->GetISurface();
    if( !surface )
        return 0;

    KeyValues schemeFile( "Scheme" );
    if( !schemeFile.LoadFromFile( NULL, fileName, NULL ) )
    {
        Con_Reportf("VGUI2: LoadSchemeFromFile failed to load KeyValues %s\n", fileName ? fileName : "<null>");
        return 0;
    }

    KeyValues *schemeRoot = &schemeFile;
    const char *schemeRootName = schemeRoot->GetName();
    if( !schemeRootName || Q_stricmp( schemeRootName, "Scheme" ) != 0 )
    {
        KeyValues *childScheme = schemeRoot->FindKey( "Scheme", false );
        if( childScheme )
            schemeRoot = childScheme;
    }

    CScheme *scheme = &m_Schemes[m_nSchemes];
    scheme->Reset();
    Q_strncpy( scheme->m_szTag, tag, sizeof( scheme->m_szTag ) - 1 );
    scheme->m_szTag[sizeof( scheme->m_szTag ) - 1] = '\0';
    Q_strncpy( m_schemeTags[m_nSchemes], tag, sizeof( m_schemeTags[m_nSchemes] ) - 1 );

    KeyValues *colorsNode = schemeRoot->FindKey( "Colors", false );
    if( colorsNode )
    {
        for( KeyValues *colorNode = colorsNode->GetFirstSubKey(); colorNode; colorNode = colorNode->GetNextKey() )
        {
            if( !colorNode->GetName() || !colorNode->GetName()[0] )
                continue;

            if( scheme->m_nColors >= ARRAYSIZE( scheme->m_Colors ) )
                break;

            ColorEntry_t &entry = scheme->m_Colors[scheme->m_nColors++];
            Q_strncpy( entry.name, colorNode->GetName(), sizeof( entry.name ) - 1 );
            entry.name[sizeof( entry.name ) - 1] = '\0';
            int r = 0, g = 0, b = 0, a = 255;
            sscanf( colorNode->GetString( "" ), "%d %d %d %d", &r, &g, &b, &a );
            entry.r = r;
            entry.g = g;
            entry.b = b;
            entry.a = a;
            Con_Reportf( "VGUI2: Color '%s' = %d %d %d %d\n", entry.name, r, g, b, a );
        }
    }

    KeyValues *baseSettings = schemeRoot->FindKey( "BaseSettings", false );
    if( baseSettings )
        CollectResourceStrings( scheme, baseSettings, NULL );

    KeyValues *fontsNode = schemeRoot->FindKey( "Fonts", false );
    if( fontsNode )
        ParseFontsSection( this, scheme, surface, fontsNode );

    KeyValues *bordersNode = schemeRoot->FindKey( "Borders", false );
    if( bordersNode )
    {
        for( KeyValues *borderNode = bordersNode->GetFirstSubKey(); borderNode; borderNode = borderNode->GetNextKey() )
        {
            if( !borderNode->GetName() || !borderNode->GetName()[0] )
                continue;
            ParseBorderNode( scheme, borderNode );
        }
    }

    HScheme h = (HScheme)(m_nSchemes + 1);
    m_nSchemes++;

    if( m_defaultScheme == 0 )
        m_defaultScheme = h;

    Con_Reportf( "VGUI2: LoadSchemeFromFile(%s, %s) -> scheme=%lu fonts=%d colors=%d resources=%d borders=%d\n",
        fileName, tag, (unsigned long)h, scheme->m_nFonts, scheme->m_nColors, scheme->m_nResourceStrings, scheme->m_nBorders );

    return h;
}

HScheme CSchemeManager::GetScheme(const char *tag)
{
    for( int i = 0; i < m_nSchemes; i++ )
    {
        if( !Q_strcmp(m_schemeTags[i], tag) )
            return (HScheme)(i + 1);
    }
    return 0;
}

IScheme *CSchemeManager::GetIScheme(HScheme scheme)
{
    if( scheme == 0 )
    {
        Con_Reportf("VGUI2: CSchemeManager::GetIScheme scheme=0 -> NULL\n");
        return NULL;
    }
    int idx = (int)scheme - 1;
    if( idx < 0 || idx >= m_nSchemes )
    {
        Con_Reportf("VGUI2: CSchemeManager::GetIScheme scheme=%lu idx=%d -> NULL (nSchemes=%d)\n",
            (unsigned long)scheme, idx, m_nSchemes);
        return NULL;
    }
    Con_Reportf("VGUI2: IScheme concrete vtable=%p, CScheme vtable would be=%p\n",
        *(void **)&m_Schemes[idx],
        *(void **)&m_Schemes[0]);
    Con_Reportf("VGUI2: CSchemeManager::GetIScheme scheme=%lu idx=%d -> %p\n",
        (unsigned long)scheme, idx, &m_Schemes[idx]);
    return &m_Schemes[idx];
}

static CSchemeManager s_ISchemeManager;
ISchemeManager *GetSchemeManager() { return &s_ISchemeManager; }

} // namespace vgui2
