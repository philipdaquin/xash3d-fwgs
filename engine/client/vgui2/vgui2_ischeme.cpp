/*
 vgui2_ischeme.cpp - ISchemeManager and IScheme implementation for Xash3D FWGS
 Batch 1F: LoadSchemeFromFile with Colors + Fonts sections only
 Uses inline minimal VDF parser to avoid tier1/KeyValues dependency.
 */
#include "vgui2_interfaces.h"
#include "vgui2_host.h"
#include "common.h"
#include "client.h"

#include <Color.h>
#include <vgui/IImage.h>

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

        ref.dllFuncs.Color4ub((byte)m_color.r(), (byte)m_color.g(), (byte)m_color.b(), (byte)m_color.a());
        ref.dllFuncs.R_DrawStretchPic((float)m_x, (float)m_y, (float)m_wide, (float)m_tall,
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
    int tall;
    int weight;
    int flags;
};

struct ColorEntry_t
{
    int r, g, b, a;
    char name[64];
};

class CScheme : public IScheme
{
public:
    const char *GetResourceString(const char *stringName) override { return ""; }
    void *GetBorder(const char *borderName) override { return NULL; }
    HFont GetFont(const char *fontName, bool proportional) override;
    int GetColor(const char *colorName, int defaultColor) override;
    HFont GetFontEx(const char *fontName, bool proportional, bool hdProportional) override
        { return GetFont(fontName, proportional); }

    FontEntry_t m_Fonts[32];
    int m_nFonts;
    ColorEntry_t m_Colors[64];
    int m_nColors;
    char m_szTag[MAX_QPATH];
};

HFont CScheme::GetFont(const char *fontName, bool /*proportional*/)
{
    for( int i = 0; i < m_nFonts; i++ )
    {
        if( !Q_strcmp(m_Fonts[i].name, fontName) )
            return m_Fonts[i].font;
    }
    return INVALID_HFONT;
}

int CScheme::GetColor(const char *colorName, int defaultColor)
{
    for( int i = 0; i < m_nColors; i++ )
    {
        if( !Q_strcmp(m_Colors[i].name, colorName) )
        {
            ColorEntry_t &c = m_Colors[i];
            return (c.a << 24) | (c.r << 16) | (c.g << 8) | c.b;
        }
    }
    return defaultColor;
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

class CSchemeManager : public ISchemeManager
{
public:
    HScheme LoadSchemeFromFile(const char *fileName, const char *tag) override;
    void ReloadSchemes() override {}
    HScheme GetDefaultScheme() override { return m_defaultScheme; }
    HScheme GetScheme(const char *tag) override;
    void *GetImage(const char *imageName, bool hardwareFiltered) override;
    HTexture GetImageID(const char *, bool) override { return 0; }
    IScheme *GetIScheme(HScheme scheme) override;
    void Shutdown(bool) override {}
    int GetProportionalScaledValue(int v) override { return v; }
    int GetProportionalNormalizedValue(int v) override { return v; }
    float GetProportionalScale() override { return 1.0f; }
    int GetHDProportionalScaledValue(int v) override { return v; }
    int GetHDProportionalNormalizedValue(int v) override { return v; }

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

void *CSchemeManager::GetImage(const char *imageName, bool)
{
    if (!imageName || !imageName[0])
        return NULL;

    for (int i = 0; i < ARRAYSIZE(m_Images); ++i)
    {
        if (m_Images[i] && !Q_strcmp(m_ImageNames[i], imageName))
            return m_Images[i];
    }

    for (int i = 0; i < ARRAYSIZE(m_Images); ++i)
    {
        if (!m_Images[i])
        {
            m_Images[i] = new CTextureImage(imageName);
            Q_strncpy(m_ImageNames[i], imageName, sizeof(m_ImageNames[i]) - 1);
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

    fs_offset_t filesize = 0;
    byte *buffer = FS_LoadFile(fileName, &filesize, false);
    if( !buffer || filesize <= 0 )
    {
        Con_Reportf("VGUI2: LoadSchemeFromFile failed to load %s\n", fileName);
        return 0;
    }

    CScheme *scheme = &m_Schemes[m_nSchemes];
    memset(scheme, 0, sizeof(CScheme));
    Q_strncpy(scheme->m_szTag, tag, sizeof(scheme->m_szTag) - 1);
    scheme->m_szTag[sizeof(scheme->m_szTag) - 1] = '\0';
    Q_strncpy(m_schemeTags[m_nSchemes], tag, sizeof(m_schemeTags[m_nSchemes]) - 1);

    int parsed = ParseSchemeFile(scheme, surface, (const char *)buffer, filesize);
    Mem_Free(buffer);

    // Now register fonts from collected data
    ISurface *surf = VGui2_GetInterfacesClient()->GetISurface();
    for( int i = 0; i < scheme->m_nFonts; i++ )
    {
        FontEntry_t *fe = &scheme->m_Fonts[i];
        HFont h = AddFont(surf, fe->name, fe->tall, fe->weight, fe->flags);
        fe->font = h;
        if( h != INVALID_HFONT )
        {
            Con_Reportf("VGUI2: ParseFonts registered '%s' tall=%d weight=%d flags=0x%x -> hfont=%lu\n",
                fe->name, fe->tall, fe->weight, fe->flags, (unsigned long)h);
        }
    }

    HScheme h = (HScheme)(m_nSchemes + 1);
    m_nSchemes++;

    if( m_defaultScheme == 0 )
        m_defaultScheme = h;

    Con_Reportf("VGUI2: LoadSchemeFromFile(%s, %s) -> scheme=%lu fonts=%d colors=%d\n",
        fileName, tag, (unsigned long)h, scheme->m_nFonts, scheme->m_nColors);

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
        return NULL;
    int idx = (int)scheme - 1;
    if( idx < 0 || idx >= m_nSchemes )
        return NULL;
    return &m_Schemes[idx];
}

static CSchemeManager s_ISchemeManager;
ISchemeManager *GetSchemeManager() { return &s_ISchemeManager; }

} // namespace vgui2
