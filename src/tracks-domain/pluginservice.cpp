#include "pluginservice.h"

// make sure this is included before windows.h, due to some macro error
#include <boolinq/boolinq.h>

#include <Windows.h>

#include "vstplugin.h"
#include <Wincrypt.h>
#include <iostream>
#include <sstream>
#include <widestringconversions.hpp>

#define BUFSIZE 1024
#define MD5LEN 16

std::string md5(
    const std::wstring &filename)
{
    DWORD dwStatus = 0;
    BOOL bResult = FALSE;
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    HANDLE hFile = NULL;
    BYTE rgbFile[BUFSIZE];
    DWORD cbRead = 0;
    BYTE rgbHash[MD5LEN];
    DWORD cbHash = 0;
    CHAR rgbDigits[] = "0123456789abcdef";

    // Logic to check usage goes here.

    hFile = CreateFile(
        filename.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_SEQUENTIAL_SCAN,
        NULL);

    if (INVALID_HANDLE_VALUE == hFile)
    {
        dwStatus = GetLastError();
        std::wcerr << L"Error opening file " << filename << std::endl
                   << L"Error: " << dwStatus << std::endl;

        return std::string();
    }

    // Get handle to the crypto provider
    if (!CryptAcquireContext(
            &hProv,
            NULL,
            NULL,
            PROV_RSA_FULL,
            CRYPT_VERIFYCONTEXT))
    {
        dwStatus = GetLastError();
        std::wcerr << L"CryptAcquireContext failed:" << dwStatus << std::endl;
        CloseHandle(hFile);

        return std::string();
    }

    if (!CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash))
    {
        dwStatus = GetLastError();
        std::wcerr << L"CryptAcquireContext failed:" << dwStatus << std::endl;
        CloseHandle(hFile);
        CryptReleaseContext(hProv, 0);

        return std::string();
    }

    bResult = ReadFile(hFile, rgbFile, BUFSIZE, &cbRead, NULL);

    while (bResult)
    {
        if (0 == cbRead)
        {
            break;
        }

        if (!CryptHashData(hHash, rgbFile, cbRead, 0))
        {
            dwStatus = GetLastError();
            std::wcerr << L"CryptHashData failed:" << dwStatus << std::endl;
            CryptReleaseContext(hProv, 0);
            CryptDestroyHash(hHash);
            CloseHandle(hFile);

            return std::string();
        }

        bResult = ReadFile(hFile, rgbFile, BUFSIZE, &cbRead, NULL);
    }

    if (!bResult)
    {
        dwStatus = GetLastError();
        std::wcerr << L"ReadFile failed:" << dwStatus << std::endl;
        CryptReleaseContext(hProv, 0);
        CryptDestroyHash(hHash);
        CloseHandle(hFile);

        return std::string();
    }

    std::stringstream ss;

    cbHash = MD5LEN;
    if (CryptGetHashParam(hHash, HP_HASHVAL, rgbHash, &cbHash, 0))
    {
        for (DWORD i = 0; i < cbHash; i++)
        {
            ss << rgbDigits[rgbHash[i] >> 4] << rgbDigits[rgbHash[i] & 0xf];
        }
    }
    else
    {
        dwStatus = GetLastError();
        std::wcerr << L"CryptGetHashParam failed:" << dwStatus << std::endl;
    }

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    CloseHandle(hFile);

    return ss.str();
}

PluginService::PluginService(
    void *owner)
    : _owner(owner)
{
    _db = std::make_unique<sqlitelib::Sqlite>("./plugin-library.sqlite");
    _db->execute(R"(
  CREATE TABLE IF NOT EXISTS plugins (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    path TEXT,
    md5 TEXT,
    type TEXT,
    vendorName TEXT,
    vendorVersion INTEGER,
    effectName TEXT,
    programCount INTEGER,
    paramCount INTEGER,
    inputCount INTEGER,
    outputCount INTEGER,
    isSynth INTEGER,
    hasEditor INTEGER
  )
)");
}

std::shared_ptr<class VstPlugin> PluginService::LoadPlugin(
    const std::string &filename)
{
    return LoadPlugin(ConvertFromBytes(filename));
}

std::shared_ptr<class VstPlugin> PluginService::LoadPlugin(
    const std::wstring &filename)
{
    auto fn = ConvertWideToBytes(filename);
    auto r = md5(filename);

    auto result = std::make_shared<VstPlugin>();

    result->init(fn.c_str());

    struct PluginDescription desc =
        {
            .type = "VST2",
            .path = ConvertWideToBytes(filename),
            .md5 = r,
            .vendorName = result->getVendorName(),
            .vendorVersion = result->getVendorVersion(),
            .effectName = result->getEffectName(),
            .programCount = result->getProgramCount(),
            .paramCount = result->getParamCount(),
            .inputCount = result->getInputCount(),
            .outputCount = result->getOutputCount(),
            .isSynth = result->flagsIsSynth(),
            .hasEditor = result->flagsHasEditor(),
        };

    EnsurePluginDescription(desc);

    return result;
}

std::shared_ptr<VstPlugin> PluginService::LoadFromFileDialog()
{
    wchar_t fn[MAX_PATH + 1] = {'\0'};
    OPENFILENAME ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = L"VSTi DLL(*.dll)\0*.dll\0All Files(*.*)\0*.*\0\0";
    ofn.lpstrFile = fn;
    ofn.nMaxFile = _countof(fn);
    ofn.lpstrTitle = L"Select VST DLL";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_ENABLESIZING;
    ofn.hwndOwner = (HWND)_owner;

    if (GetOpenFileName(&ofn) != TRUE)
    {
        return nullptr;
    }

    return LoadPlugin(fn);
}

void PluginService::EnsurePluginDescription(
    struct PluginDescription desc)
{
    auto stmt = _db->prepare<int>("SELECT id FROM plugins WHERE md5 = ?");

    auto rows = stmt.execute(desc.md5);

    if (rows.empty())
    {
        auto stmt = _db->prepare(R"(
INSERT INTO plugins ( type, path, md5,    vendorName, vendorVersion,  effectName, programCount, paramCount, inputCount, outputCount, isSynth,    hasEditor )
             VALUES ( ?,    ?,    ?,      ?,          ?,              ?,          ?,            ?,          ?,          ?,           ?,          ? )
)");

        stmt.execute(
            desc.type,
            desc.path,
            desc.md5,
            desc.vendorName,
            desc.vendorVersion,
            desc.effectName,
            desc.programCount,
            desc.paramCount,
            desc.inputCount,
            desc.outputCount,
            desc.isSynth ? 1 : 0,
            desc.hasEditor ? 1 : 0);
    }
    else
    {
        auto stmt = _db->prepare(R"(
UPDATE plugins SET
    type = ?, path = ?, md5 = ?, vendorName = ?, vendorVersion = ?, effectName = ?,
    programCount = ?, paramCount = ?, inputCount = ?, outputCount = ?, isSynth = ?, hasEditor = ?
WHERE id = ?
)");

        stmt.execute(
            desc.type,
            desc.path,
            desc.md5,
            desc.vendorName,
            desc.vendorVersion,
            desc.effectName,
            desc.programCount,
            desc.paramCount,
            desc.inputCount,
            desc.outputCount,
            desc.isSynth ? 1 : 0,
            desc.hasEditor ? 1 : 0,
            rows[0]);
    }
}

const char *selectPluginFromSqlite = "SELECT id, type, path, md5, vendorName, vendorVersion, effectName, programCount, paramCount, inputCount, outputCount, isSynth, hasEditor FROM plugins";

PluginDescription mapPluginFromSqlite(
    const std::tuple<int, std::string, std::string, std::string, std::string, int, std::string, int, int, int, int, int, int> &row)
{
    return PluginDescription{
        .id = std::get<0>(row),
        .type = std::get<1>(row),
        .path = std::get<2>(row),
        .md5 = std::get<3>(row),
        .vendorName = std::get<4>(row),
        .vendorVersion = std::get<5>(row),
        .effectName = std::get<6>(row),
        .programCount = std::get<7>(row),
        .paramCount = std::get<8>(row),
        .inputCount = std::get<9>(row),
        .outputCount = std::get<10>(row),
        .isSynth = std::get<11>(row) == 1,
        .hasEditor = std::get<12>(row) == 1,
    };
}

std::vector<struct PluginDescription> PluginService::ListPlugins(
    std::function<bool(const struct PluginDescription &)> filter)
{
    std::vector<struct PluginDescription> result;

    auto stmt = _db->prepare<int, std::string, std::string, std::string, std::string, int, std::string, int, int, int, int, int, int>(selectPluginFromSqlite);

    auto rows = stmt.execute();

    return boolinq::from(rows)
        .select(mapPluginFromSqlite)
        .where(filter)
        .toStdVector();
}
