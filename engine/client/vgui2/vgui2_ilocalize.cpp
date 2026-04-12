/*
 * vgui2_ilocalize.cpp - VGUI2 ILocalize implementation
 * 
 * Implements localization for VGUI2
 * Handles UTF-16 LE encoded .txt files
 */

#include "vgui2_interfaces.h"
#include "common.h"
#include "client.h"
#include "ref_api.h"
#include "xash3d_types.h"
#include "VFileSystem009.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <wchar.h>

namespace vgui2
{

#define MAX_LOCALIZATION_FILES 16
#define MAX_TOKENS 8192
#define MAX_TOKEN_NAME 256
#define MAX_TOKEN_VALUE 1024
#define INVALID_LOCALIZE_STRING_INDEX ((unsigned long)-1)

struct LocalizationToken_t
{
    char name[MAX_TOKEN_NAME];
    wchar_t* value;
};

struct LocalizationFile_t
{
    char filename[256];
    bool loaded;
    int tokenCount;
    LocalizationToken_t tokens[MAX_TOKENS];
};

static LocalizationFile_t s_files[MAX_LOCALIZATION_FILES];
static int s_fileCount = 0;

static IFileSystem* s_pFileSystem = NULL;

static wchar_t s_tempUnicodeBuffer[MAX_TOKEN_VALUE];

static int FindToken(const char* tokenName)
{
    for (int f = 0; f < s_fileCount; f++)
    {
        if (!s_files[f].loaded)
            continue;
        
        for (int i = 0; i < s_files[f].tokenCount; i++)
        {
            if (!Q_strcmp(s_files[f].tokens[i].name, tokenName))
            {
                return f * MAX_TOKENS + i;
            }
        }
    }
    return -1;
}

static LocalizationToken_t* GetToken(int index)
{
    if (index < 0)
        return NULL;
    
    int fileIdx = index / MAX_TOKENS;
    int tokenIdx = index % MAX_TOKENS;
    
    if (fileIdx >= s_fileCount)
        return NULL;
    
    if (!s_files[fileIdx].loaded)
        return NULL;
    
    if (tokenIdx >= s_files[fileIdx].tokenCount)
        return NULL;
    
    return &s_files[fileIdx].tokens[tokenIdx];
}

static int LoadLocalizationFile(const char* filename)
{
    if (s_fileCount >= MAX_LOCALIZATION_FILES)
    {
        Con_Reportf("ILocalize: Too many localization files loaded!\n");
        return -1;
    }
    
    int fileIdx = s_fileCount;
    LocalizationFile_t* pFile = &s_files[fileIdx];
    
    memset(pFile, 0, sizeof(*pFile));
    Q_strncpy(pFile->filename, filename, sizeof(pFile->filename) - 1);
    pFile->loaded = false;
    
    FileHandle_t fh = s_pFileSystem->Open(filename, "rb", "GAME");
    if (!fh)
    {
        fh = s_pFileSystem->Open(filename, "rb", NULL);
    }
    
    if (!fh)
    {
        Con_Reportf("ILocalize: Could not open localization file: %s\n", filename);
        return -1;
    }
    
    int fileSize = s_pFileSystem->Size(fh);
    if (fileSize <= 2)
    {
        s_pFileSystem->Close(fh);
        Con_Reportf("ILocalize: Empty localization file: %s\n", filename);
        return -1;
    }
    
    unsigned char* buffer = (unsigned char*)malloc(fileSize + 2);
    if (!buffer)
    {
        s_pFileSystem->Close(fh);
        return -1;
    }
    
    memset(buffer, 0, fileSize + 2);
    int bytesRead = s_pFileSystem->Read(buffer, fileSize, fh);
    s_pFileSystem->Close(fh);
    
    if (bytesRead != fileSize)
    {
        free(buffer);
        return -1;
    }
    
    int isUTF16LE = 0;
    if (fileSize >= 2)
    {
        if (buffer[0] == 0xFF && buffer[1] == 0xFE)
            isUTF16LE = 1;
        else if (buffer[0] == 0xFE && buffer[1] == 0xFF)
            isUTF16LE = 0;
        else
            isUTF16LE = 0;
    }
    
    if (isUTF16LE)
    {
        wchar_t* wideBuffer = (wchar_t*)buffer;
        int charCount = (fileSize / 2) - 1;
        
        int tokenCount = 0;
        
        for (int i = 0; i < charCount && tokenCount < MAX_TOKENS; )
        {
            while (i < charCount && (wideBuffer[i] == L'\n' || wideBuffer[i] == L'\r' || wideBuffer[i] == L' ' || wideBuffer[i] == L'\t'))
                i++;
            
            if (i >= charCount)
                break;
            
            if (wideBuffer[i] != L'"')
            {
                while (i < charCount && wideBuffer[i] != L'\n' && wideBuffer[i] != L'\r')
                    i++;
                continue;
            }
            
            i++;
            int nameStart = i;
            while (i < charCount && wideBuffer[i] != L'"' && wideBuffer[i] != L'\n' && wideBuffer[i] != L'\r')
                i++;
            
            if (i >= charCount || wideBuffer[i] != L'"')
            {
                while (i < charCount && wideBuffer[i] != L'\n' && wideBuffer[i] != L'\r')
                    i++;
                continue;
            }
            
            int nameLen = i - nameStart;
            if (nameLen >= MAX_TOKEN_NAME)
                nameLen = MAX_TOKEN_NAME - 1;
            
            char name[MAX_TOKEN_NAME];
            for (int j = 0; j < nameLen; j++)
                name[j] = (char)wideBuffer[nameStart + j];
            name[nameLen] = '\0';
            
            i++;
            
            while (i < charCount && (wideBuffer[i] == L' ' || wideBuffer[i] == L'\t'))
                i++;
            
            if (i < charCount && wideBuffer[i] == L'"')
            {
                i++;
                int valueStart = i;
                while (i < charCount && wideBuffer[i] != L'"' && wideBuffer[i] != L'\n' && wideBuffer[i] != L'\r')
                    i++;
                
                int valueLen = i - valueStart;
                if (valueLen >= MAX_TOKEN_VALUE)
                    valueLen = MAX_TOKEN_VALUE - 1;
                
                wchar_t* value = (wchar_t*)malloc((valueLen + 1) * sizeof(wchar_t));
                if (value)
                {
                    for (int j = 0; j < valueLen; j++)
                        value[j] = wideBuffer[valueStart + j];
                    value[valueLen] = L'\0';
                    
                    Q_strncpy(pFile->tokens[tokenCount].name, name, MAX_TOKEN_NAME - 1);
                    pFile->tokens[tokenCount].value = value;
                    tokenCount++;
                }
            }
            
            while (i < charCount && wideBuffer[i] != L'\n' && wideBuffer[i] != L'\r')
                i++;
        }
        
        pFile->tokenCount = tokenCount;
    }
    else
    {
        char* ansiBuffer = (char*)buffer;
        int tokenCount = 0;
        
        for (int i = 0; i < fileSize && tokenCount < MAX_TOKENS; )
        {
            while (i < fileSize && (ansiBuffer[i] == '\n' || ansiBuffer[i] == '\r' || ansiBuffer[i] == ' ' || ansiBuffer[i] == '\t'))
                i++;
            
            if (i >= fileSize)
                break;
            
            if (ansiBuffer[i] != '"')
            {
                while (i < fileSize && ansiBuffer[i] != '\n' && ansiBuffer[i] != '\r')
                    i++;
                continue;
            }
            
            i++;
            int nameStart = i;
            while (i < fileSize && ansiBuffer[i] != '"' && ansiBuffer[i] != '\n' && ansiBuffer[i] != '\r')
                i++;
            
            if (i >= fileSize || ansiBuffer[i] != '"')
            {
                while (i < fileSize && ansiBuffer[i] != '\n' && ansiBuffer[i] != '\r')
                    i++;
                continue;
            }
            
            int nameLen = i - nameStart;
            if (nameLen >= MAX_TOKEN_NAME)
                nameLen = MAX_TOKEN_NAME - 1;
            
            char name[MAX_TOKEN_NAME];
            memcpy(name, &ansiBuffer[nameStart], nameLen);
            name[nameLen] = '\0';
            
            i++;
            
            while (i < fileSize && (ansiBuffer[i] == ' ' || ansiBuffer[i] == '\t'))
                i++;
            
            if (i < fileSize && ansiBuffer[i] == '"')
            {
                i++;
                int valueStart = i;
                while (i < fileSize && ansiBuffer[i] != '"' && ansiBuffer[i] != '\n' && ansiBuffer[i] != '\r')
                    i++;
                
                int valueLen = i - valueStart;
                if (valueLen >= MAX_TOKEN_VALUE)
                    valueLen = MAX_TOKEN_VALUE - 1;
                
                wchar_t* value = (wchar_t*)malloc((valueLen + 1) * sizeof(wchar_t));
                if (value)
                {
                    for (int j = 0; j < valueLen; j++)
                        value[j] = (wchar_t)(unsigned char)ansiBuffer[valueStart + j];
                    value[valueLen] = L'\0';
                    
                    Q_strncpy(pFile->tokens[tokenCount].name, name, MAX_TOKEN_NAME - 1);
                    pFile->tokens[tokenCount].value = value;
                    tokenCount++;
                }
            }
            
            while (i < fileSize && ansiBuffer[i] != '\n' && ansiBuffer[i] != '\r')
                i++;
        }
        
        pFile->tokenCount = tokenCount;
    }
    
    free(buffer);
    
    pFile->loaded = true;
    s_fileCount++;
    
    Con_Reportf("ILocalize: Loaded localization file: %s (%d tokens)\n", filename, pFile->tokenCount);
    
    return fileIdx;
}

class CLocalize : public ILocalize
{
public:
    bool AddFile(IFileSystem* fileSystem, const char* fileName) override
    {
        if (!fileSystem || !fileName)
            return false;
        
        if (s_pFileSystem == NULL)
            s_pFileSystem = fileSystem;
        
        int result = LoadLocalizationFile(fileName);
        return (result >= 0);
    }
    
    void RemoveAll() override
    {
        for (int f = 0; f < s_fileCount; f++)
        {
            for (int i = 0; i < s_files[f].tokenCount; i++)
            {
                if (s_files[f].tokens[i].value)
                {
                    free(s_files[f].tokens[i].value);
                    s_files[f].tokens[i].value = NULL;
                }
            }
            s_files[f].loaded = false;
            s_files[f].tokenCount = 0;
        }
        s_fileCount = 0;
    }
    
    wchar_t* Find(char const* tokenName) override
    {
        if (!tokenName || !*tokenName)
            return NULL;
        
        int index = FindToken(tokenName);
        if (index < 0)
            return NULL;
        
        LocalizationToken_t* token = GetToken(index);
        if (!token || !token->value)
            return NULL;
        
        return token->value;
    }
    
    int ConvertANSIToUnicode(const char* ansi, wchar_t* unicode, int unicodeBufferSizeInBytes) override
    {
        if (!ansi || !unicode || unicodeBufferSizeInBytes <= 0)
            return 0;
        
        int maxChars = unicodeBufferSizeInBytes / sizeof(wchar_t) - 1;
        
        int i;
        for (i = 0; i < maxChars && ansi[i]; i++)
        {
            unicode[i] = (wchar_t)(unsigned char)ansi[i];
        }
        unicode[i] = L'\0';
        
        return i + 1;
    }
    
    int ConvertUnicodeToANSI(const wchar_t* unicode, char* ansi, int ansiBufferSize) override
    {
        if (!unicode || !ansi || ansiBufferSize <= 0)
            return 0;
        
        int maxChars = ansiBufferSize - 1;
        
        int i;
        for (i = 0; i < maxChars && unicode[i]; i++)
        {
            if (unicode[i] > 255)
                ansi[i] = '?';
            else
                ansi[i] = (char)(unsigned short)unicode[i];
        }
        ansi[i] = '\0';
        
        return i + 1;
    }
    
    unsigned long FindIndex(const char* tokenName) override
    {
        if (!tokenName || !*tokenName)
            return INVALID_LOCALIZE_STRING_INDEX;
        
        int index = FindToken(tokenName);
        if (index < 0)
            return INVALID_LOCALIZE_STRING_INDEX;
        
        return (unsigned long)index;
    }
    
    void ConstructString(wchar_t* unicodeOutput, int unicodeBufferSizeInBytes, wchar_t* formatString, int numFormatParameters, ...) override
    {
        if (!unicodeOutput || !formatString || unicodeBufferSizeInBytes <= 0)
            return;
        
        int maxChars = unicodeBufferSizeInBytes / sizeof(wchar_t);
        
        va_list args;
        va_start(args, numFormatParameters);
        
        int outIdx = 0;
        for (int i = 0; formatString[i] && outIdx < maxChars - 1; i++)
        {
            if (formatString[i] == L'%' && formatString[i+1] >= L'1' && formatString[i+1] <= L'9')
            {
                int paramNum = formatString[i+1] - L'0';
                if (paramNum <= numFormatParameters)
                {
                    wchar_t* arg = va_arg(args, wchar_t*);
                    if (arg)
                    {
                        for (int j = 0; arg[j] && outIdx < maxChars - 1; j++)
                            unicodeOutput[outIdx++] = arg[j];
                    }
                }
                i++;
            }
            else
            {
                unicodeOutput[outIdx++] = formatString[i];
            }
        }
        
        va_end(args);
        
        unicodeOutput[outIdx] = L'\0';
    }
    
    const char* GetNameByIndex(unsigned long index) override
    {
        LocalizationToken_t* token = GetToken((int)index);
        if (!token)
            return "";
        return token->name;
    }
    
    wchar_t* GetValueByIndex(unsigned long index) override
    {
        LocalizationToken_t* token = GetToken((int)index);
        if (!token)
            return NULL;
        return token->value;
    }
    
    unsigned long GetFirstStringIndex() override
    {
        if (s_fileCount > 0 && s_files[0].loaded && s_files[0].tokenCount > 0)
            return 0;
        return INVALID_LOCALIZE_STRING_INDEX;
    }
    
    unsigned long GetNextStringIndex(unsigned long index) override
    {
        int currentFile = (int)index / MAX_TOKENS;
        int currentToken = (int)index % MAX_TOKENS;
        
        while (currentFile < s_fileCount)
        {
            if (s_files[currentFile].loaded && currentToken < s_files[currentFile].tokenCount)
            {
                return index + 1;
            }
            currentFile++;
            currentToken = 0;
        }
        
        return INVALID_LOCALIZE_STRING_INDEX;
    }
    
    void AddString(const char* tokenName, wchar_t* unicodeString, const char* fileName) override
    {
        if (!tokenName || !unicodeString)
            return;
        
        if (s_fileCount >= MAX_LOCALIZATION_FILES)
            return;
        
        LocalizationFile_t* pFile = &s_files[s_fileCount];
        
        if (pFile->tokenCount >= MAX_TOKENS)
            return;
        
        Q_strncpy(pFile->tokens[pFile->tokenCount].name, tokenName, MAX_TOKEN_NAME - 1);
        
        int len = 0;
        while (unicodeString[len] && len < MAX_TOKEN_VALUE - 1)
            len++;
        
        pFile->tokens[pFile->tokenCount].value = (wchar_t*)malloc((len + 1) * sizeof(wchar_t));
        if (pFile->tokens[pFile->tokenCount].value)
        {
            memcpy(pFile->tokens[pFile->tokenCount].value, unicodeString, (len + 1) * sizeof(wchar_t));
            pFile->tokenCount++;
        }
    }
    
    void SetValueByIndex(unsigned long index, wchar_t* newValue) override
    {
        LocalizationToken_t* token = GetToken((int)index);
        if (!token || !newValue)
            return;
        
        if (token->value)
            free(token->value);
        
        int len = 0;
        while (newValue[len] && len < MAX_TOKEN_VALUE - 1)
            len++;
        
        token->value = (wchar_t*)malloc((len + 1) * sizeof(wchar_t));
        if (token->value)
        {
            memcpy(token->value, newValue, (len + 1) * sizeof(wchar_t));
        }
    }
    
    bool SaveToFile(IFileSystem* fileSystem, const char* fileName) override
    {
        return false;
    }
    
    int GetLocalizationFileCount() override
    {
        int count = 0;
        for (int i = 0; i < s_fileCount; i++)
        {
            if (s_files[i].loaded)
                count++;
        }
        return count;
    }
    
    const char* GetLocalizationFileName(int index) override
    {
        int count = 0;
        for (int i = 0; i < s_fileCount; i++)
        {
            if (s_files[i].loaded)
            {
                if (count == index)
                    return s_files[i].filename;
                count++;
            }
        }
        return "";
    }
    
    const char* GetFileNameByIndex(unsigned long index) override
    {
        LocalizationToken_t* token = GetToken((int)index);
        if (!token)
            return "";
        
        for (int i = 0; i < s_fileCount; i++)
        {
            for (int j = 0; j < s_files[i].tokenCount; j++)
            {
                if (&s_files[i].tokens[j] == token)
                    return s_files[i].filename;
            }
        }
        return "";
    }
    
    void ReloadLocalizationFiles(IFileSystem* filesystem) override
    {
        for (int i = 0; i < s_fileCount; i++)
        {
            for (int j = 0; j < s_files[i].tokenCount; j++)
            {
                if (s_files[i].tokens[j].value)
                {
                    free(s_files[i].tokens[j].value);
                    s_files[i].tokens[j].value = NULL;
                }
            }
            s_files[i].loaded = false;
            s_files[i].tokenCount = 0;
        }
        
        if (filesystem)
            s_pFileSystem = filesystem;
    }
    
    void ConstructString(wchar_t* unicodeOutput, int unicodeBufferSizeInBytes, const char* tokenName, KeyValues* localizationVariables) override
    {
        wchar_t* formatStr = Find(tokenName);
        if (!formatStr)
        {
            if (unicodeOutput && unicodeBufferSizeInBytes > 0)
                unicodeOutput[0] = L'\0';
            return;
        }
        
        ConstructString(unicodeOutput, unicodeBufferSizeInBytes, formatStr, 0);
    }
    
    void ConstructString(wchar_t* unicodeOutput, int unicodeBufferSizeInBytes, unsigned long unlocalizedTextSymbol, KeyValues* localizationVariables) override
    {
        wchar_t* formatStr = GetValueByIndex(unlocalizedTextSymbol);
        if (!formatStr)
        {
            if (unicodeOutput && unicodeBufferSizeInBytes > 0)
                unicodeOutput[0] = L'\0';
            return;
        }
        
        ConstructString(unicodeOutput, unicodeBufferSizeInBytes, formatStr, 0);
    }
};

static CLocalize s_Localize;
static IFileSystem* s_pFileSystemInternal = NULL;

ILocalize* GetLocalizeImpl()
{
    return &s_Localize;
}

void SetLocalizeFileSystemImpl(IFileSystem* pFileSystem)
{
    s_pFileSystemInternal = pFileSystem;
}

} // namespace vgui2

extern "C" void SetLocalizeFileSystem(IFileSystem* pFileSystem)
{
    vgui2::SetLocalizeFileSystemImpl(pFileSystem);
}

extern "C" vgui2::ILocalize* GetLocalize()
{
    return vgui2::GetLocalizeImpl();
}
