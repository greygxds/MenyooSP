#include "UpdateChecker.h"

#include "../Menu/Keybinds.h"
#include "../Menu/Language.h"
#include "../Menu/Menu.h"
#include "../Natives/natives2.h"
#include "../Natives/types.h"
#include "../Scripting/Game.h"
#include "../Util/FileLogger.h"
#include "../macros.h"

#include <Windows.h>
#include <bcrypt.h>
#include <shellapi.h>
#include <winhttp.h>

#include <algorithm>
#include <atomic>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <json\single_include\nlohmann\json.hpp>

#pragma comment(lib, "bcrypt.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "winhttp.lib")

namespace Updates
{
bool ignoreUpdateChecks = false;

namespace
{
constexpr wchar_t kLatestReleasePath[] = L"/repos/greygxds/MenyooSP/releases/latest";
constexpr char kReleasePageUrl[] = "https://github.com/greygxds/MenyooSP/releases/latest";
constexpr char kReleaseAssetPrefix[] = "https://github.com/greygxds/MenyooSP/releases/download/";

enum class State
{
    NotStarted,
    Checking,
    UpdateAvailable,
    Downloading,
    InstallPending,
    Failed,
    UpToDate
};

struct ReleaseInfo
{
    std::string version;
    std::string assetUrl;
    std::string assetDigest;
    std::string changelogUrl;
};

std::atomic<State> g_state{State::NotStarted};
std::atomic<bool> g_dismissed{false};
std::atomic<bool> g_checkStarted{false};
std::atomic<bool> g_downloadStarted{false};
std::atomic<bool> g_installFailed{false};
std::mutex g_releaseMutex;
ReleaseInfo g_release;
std::wstring g_modulePath;
std::wstring g_targetDirectory;

std::wstring ToWide(const std::string& value)
{
    if (value.empty())
        return {};

    const int characterCount = MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), nullptr, 0);
    if (characterCount <= 0)
        return {};

    std::wstring result(characterCount, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.data(), static_cast<int>(value.size()), result.data(), characterCount);
    return result;
}

HMODULE PinModule()
{
    HMODULE module = nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, reinterpret_cast<LPCWSTR>(&PinModule), &module);
    return module;
}

std::wstring GetModulePath()
{
    HMODULE module = nullptr;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, reinterpret_cast<LPCWSTR>(&GetModulePath), &module))
    {
        return {};
    }

    std::vector<wchar_t> path(32768);
    const DWORD length = GetModuleFileNameW(module, path.data(), static_cast<DWORD>(path.size()));
    if (length == 0 || length >= path.size())
        return {};

    return std::wstring(path.data(), length);
}

bool OpenUrl(const std::string& url)
{
    const std::wstring wideUrl = ToWide(url);
    if (wideUrl.empty())
        return false;

    return reinterpret_cast<INT_PTR>(ShellExecuteW(nullptr, L"open", wideUrl.c_str(), nullptr, nullptr, SW_SHOWNORMAL)) > 32;
}

bool ReadHttp(const std::wstring& url, std::vector<unsigned char>& result)
{
    URL_COMPONENTS components{};
    components.dwStructSize = sizeof(components);
    components.dwSchemeLength = static_cast<DWORD>(-1);
    components.dwHostNameLength = static_cast<DWORD>(-1);
    components.dwUrlPathLength = static_cast<DWORD>(-1);
    components.dwExtraInfoLength = static_cast<DWORD>(-1);

    if (!WinHttpCrackUrl(url.c_str(), static_cast<DWORD>(url.size()), 0, &components))
        return false;
    if (components.nScheme != INTERNET_SCHEME_HTTPS)
        return false;

    const std::wstring host(components.lpszHostName, components.dwHostNameLength);
    std::wstring path(components.lpszUrlPath, components.dwUrlPathLength);
    if (components.dwExtraInfoLength > 0)
        path.append(components.lpszExtraInfo, components.dwExtraInfoLength);

    HINTERNET session = WinHttpOpen(L"MenyooUpdater/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session)
        return false;

    WinHttpSetTimeouts(session, 5000, 5000, 10000, 10000);
    HINTERNET connection = WinHttpConnect(session, host.c_str(), components.nPort, 0);
    if (!connection)
    {
        WinHttpCloseHandle(session);
        return false;
    }

    HINTERNET request = WinHttpOpenRequest(connection, L"GET", path.c_str(), nullptr, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
    if (!request)
    {
        WinHttpCloseHandle(connection);
        WinHttpCloseHandle(session);
        return false;
    }

    const wchar_t headers[] = L"Accept: application/vnd.github+json\r\n";
    bool success = false;
    if (WinHttpSendRequest(request, headers, static_cast<DWORD>(-1), WINHTTP_NO_REQUEST_DATA, 0, 0, 0) && WinHttpReceiveResponse(request, nullptr))
    {
        DWORD statusCode = 0;
        DWORD statusSize = sizeof(statusCode);
        if (WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER, WINHTTP_HEADER_NAME_BY_INDEX, &statusCode, &statusSize, WINHTTP_NO_HEADER_INDEX) && statusCode == 200)
        {
            success = true;
            for (;;)
            {
                DWORD bytesAvailable = 0;
                if (!WinHttpQueryDataAvailable(request, &bytesAvailable))
                {
                    success = false;
                    break;
                }
                if (bytesAvailable == 0)
                    break;

                const size_t offset = result.size();
                result.resize(offset + bytesAvailable);
                DWORD bytesRead = 0;
                if (!WinHttpReadData(request, result.data() + offset, bytesAvailable, &bytesRead))
                {
                    result.resize(offset);
                    success = false;
                    break;
                }
                result.resize(offset + bytesRead);
            }
        }
    }

    WinHttpCloseHandle(request);
    WinHttpCloseHandle(connection);
    WinHttpCloseHandle(session);
    return success;
}

bool DownloadToFile(const std::string& url, const std::filesystem::path& destination)
{
    std::vector<unsigned char> bytes;
    if (!ReadHttp(ToWide(url), bytes))
        return false;

    std::ofstream output(destination, std::ios::binary | std::ios::trunc);
    if (!output)
        return false;

    output.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    return output.good();
}

std::string FindAssetUrl(const nlohmann::json& release, const std::string& assetName)
{
    if (!release.contains("assets") || !release["assets"].is_array())
        return {};

    for (const auto& asset : release["assets"])
    {
        if (asset.value("name", std::string{}) == assetName)
            return asset.value("browser_download_url", std::string{});
    }
    return {};
}

// The SHA-256 hex GitHub publishes on the asset itself ("digest":"sha256:<64hex>").
std::string FindAssetDigest(const nlohmann::json& release, const std::string& assetName)
{
    if (!release.contains("assets") || !release["assets"].is_array())
        return {};

    for (const auto& asset : release["assets"])
    {
        if (asset.value("name", std::string{}) != assetName)
            continue;

        std::string digest = asset.value("digest", std::string{});
        constexpr char kDigestPrefix[] = "sha256:";
        if (digest.rfind(kDigestPrefix, 0) == 0)
            digest.erase(0, sizeof(kDigestPrefix) - 1);
        std::transform(digest.begin(), digest.end(), digest.begin(), [](unsigned char character) { return static_cast<char>(std::tolower(character)); });
        return digest;
    }
    return {};
}

bool IsValidDigest(const std::string& digest)
{
    if (digest.size() != 64)
        return false;

    for (char character : digest)
    {
        if (!std::isxdigit(static_cast<unsigned char>(character)))
            return false;
    }
    return true;
}

std::string MakeChangelogUrl(const std::string& latestTag)
{
#if MENYOO_BUILD_NUMBER > 0
    const std::string installedCommit = MENYOO_COMMIT_SHA;
    if (!installedCommit.empty() && installedCommit != "local")
    {
        return "https://github.com/greygxds/MenyooSP/compare/" + installedCommit + "...tags/" + latestTag;
    }
#endif
    return kReleasePageUrl;
}

void CheckForUpdate()
{
    std::vector<unsigned char> response;
    if (!ReadHttp(kLatestReleasePath, response))
    {
        g_state = State::Failed;
        return;
    }

    const nlohmann::json release = nlohmann::json::parse(response, nullptr, false);
    if (release.is_discarded())
    {
        g_state = State::Failed;
        return;
    }

    const std::string tag = release.value("tag_name", std::string{});
    if (tag.rfind("build-", 0) != 0 ||
        !std::all_of(tag.begin(), tag.end(), [](unsigned char character) { return std::isalnum(character) || character == '-' || character == '_' || character == '.'; }))
    {
        g_state = State::Failed;
        return;
    }

    const size_t buildSeparator = tag.find_last_of('_');
    const size_t buildNumberStart = buildSeparator == std::string::npos ? 6 : buildSeparator + 1;
    const std::string buildNumberText = tag.substr(buildNumberStart);
    if (buildNumberText.empty() || !std::all_of(buildNumberText.begin(), buildNumberText.end(), [](unsigned char character) { return std::isdigit(character); }))
    {
        g_state = State::Failed;
        return;
    }

    int latestBuildNumber = 0;
    try
    {
        latestBuildNumber = std::stoi(buildNumberText);
    }
    catch (...)
    {
        g_state = State::Failed;
        return;
    }

    if (latestBuildNumber <= MENYOO_BUILD_NUMBER)
    {
        g_state = State::UpToDate;
        return;
    }

    ReleaseInfo info;
    info.version = "Build " + tag.substr(6);
    info.assetUrl = FindAssetUrl(release, "Menyoo.asi");
    info.assetDigest = FindAssetDigest(release, "Menyoo.asi");
    info.changelogUrl = MakeChangelogUrl(tag);
    if (info.assetUrl.rfind(kReleaseAssetPrefix, 0) != 0)
    {
        addlog(ige::LogType::LOG_WARNING, "Release " + tag + " has no standalone Menyoo.asi asset; cannot self-update from it.");
        g_state = State::Failed;
        return;
    }
    if (!IsValidDigest(info.assetDigest))
    {
        addlog(ige::LogType::LOG_WARNING, "Release " + tag + " carries no SHA-256 digest for Menyoo.asi; cannot self-update from it.");
        g_state = State::Failed;
        return;
    }

    {
        std::lock_guard<std::mutex> lock(g_releaseMutex);
        g_release = std::move(info);
    }
    g_state = State::UpdateAvailable;
}

std::string CalculateSha256(const std::filesystem::path& filePath)
{
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    BCRYPT_HASH_HANDLE hash = nullptr;
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0)
        return {};

    DWORD objectLength = 0;
    DWORD resultLength = 0;
    if (BCryptGetProperty(algorithm, BCRYPT_OBJECT_LENGTH, reinterpret_cast<PUCHAR>(&objectLength), sizeof(objectLength), &resultLength, 0) < 0)
    {
        BCryptCloseAlgorithmProvider(algorithm, 0);
        return {};
    }

    std::vector<unsigned char> hashObject(objectLength);
    std::vector<unsigned char> digest(32);
    if (BCryptCreateHash(algorithm, &hash, hashObject.data(), objectLength, nullptr, 0, 0) < 0)
    {
        BCryptCloseAlgorithmProvider(algorithm, 0);
        return {};
    }

    std::ifstream input(filePath, std::ios::binary);
    std::vector<unsigned char> buffer(64 * 1024);
    bool success = static_cast<bool>(input);
    while (success && input)
    {
        input.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()));
        const std::streamsize bytesRead = input.gcount();
        if (bytesRead > 0 && BCryptHashData(hash, buffer.data(), static_cast<ULONG>(bytesRead), 0) < 0)
            success = false;
    }
    if (input.bad())
        success = false;
    if (success && BCryptFinishHash(hash, digest.data(), static_cast<ULONG>(digest.size()), 0) < 0)
        success = false;

    BCryptDestroyHash(hash);
    BCryptCloseAlgorithmProvider(algorithm, 0);
    if (!success)
        return {};

    static constexpr char hexDigits[] = "0123456789abcdef";
    std::string result;
    result.reserve(digest.size() * 2);
    for (const unsigned char value : digest)
    {
        result.push_back(hexDigits[value >> 4]);
        result.push_back(hexDigits[value & 0x0F]);
    }
    return result;
}

void StartInstaller(const std::filesystem::path& stagedPath)
{
    const std::filesystem::path modulePath(g_modulePath);
    const std::filesystem::path backupPath = modulePath.wstring() + L".old";

    // A loaded .asi cannot be overwritten but it can be renamed: move it aside, then the new one in.
    // REPLACE_EXISTING also removes a stale .old left behind by a previous update.
    if (!MoveFileExW(modulePath.c_str(), backupPath.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        const DWORD error = GetLastError();
        addlog(ige::LogType::LOG_WARNING, "Could not rename Menyoo.asi to Menyoo.asi.old (Windows error " + std::to_string(error) + "); the installed plugin was left untouched.");
        DeleteFileW(stagedPath.c_str());
        g_installFailed = true;
        g_state = State::Failed;
        return;
    }

    if (!MoveFileExW(stagedPath.c_str(), modulePath.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH))
    {
        const DWORD error = GetLastError();
        MoveFileExW(backupPath.c_str(), modulePath.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
        addlog(ige::LogType::LOG_WARNING, "Could not move the downloaded update into place (Windows error " + std::to_string(error) + "); the old plugin was restored.");
        DeleteFileW(stagedPath.c_str());
        g_installFailed = true;
        g_state = State::Failed;
        return;
    }

    addlog(ige::LogType::LOG_INFO, "Menyoo update installed; the previous version is kept as Menyoo.asi.old. Restart GTA V to apply it.");
    g_state = State::InstallPending;
}

void DownloadAndStage()
{
    ReleaseInfo info;
    {
        std::lock_guard<std::mutex> lock(g_releaseMutex);
        info = g_release;
    }

    const std::filesystem::path stagedPath = std::filesystem::path(g_targetDirectory) / L".Menyoo.asi.update";
    if (!DownloadToFile(info.assetUrl, stagedPath))
    {
        DeleteFileW(stagedPath.c_str());
        g_state = State::Failed;
        return;
    }

    const std::string actualChecksum = CalculateSha256(stagedPath);
    if (actualChecksum.empty() || actualChecksum != info.assetDigest)
    {
        DeleteFileW(stagedPath.c_str());
        g_state = State::Failed;
        return;
    }

    StartInstaller(stagedPath);
}

void StartWorker(bool download)
{
    HMODULE module = PinModule();
    if (!module)
    {
        g_state = State::Failed;
        return;
    }

    try
    {
        std::thread(
            [module, download]
            {
                try
                {
                    if (download)
                        DownloadAndStage();
                    else
                        CheckForUpdate();
                }
                catch (...)
                {
                    g_state = State::Failed;
                }
                if (module)
                    FreeLibrary(module);
            }
        ).detach();
    }
    catch (...)
    {
        if (module)
            FreeLibrary(module);
        g_state = State::Failed;
    }
}

bool IsFormattingTag(const std::string& tag)
{
    return tag == "~r~" || tag == "~b~" || tag == "~g~" || tag == "~y~" || tag == "~p~" || tag == "~w~" || tag == "~o~" || tag == "~c~" || tag == "~m~" || tag == "~u~" || tag == "~s~" || tag == "~n~";
}

bool IsPersistentFormattingTag(const std::string& tag)
{
    return IsFormattingTag(tag) && tag != "~s~" && tag != "~n~";
}

std::string GetVisibleText(const std::string& text)
{
    std::string visible;
    for (std::string::size_type index = 0; index < text.length();)
    {
        if (text[index] == '~')
        {
            const std::string::size_type end = text.find('~', index + 1);
            if (end != std::string::npos)
            {
                const std::string tag = text.substr(index, end - index + 1);
                if (IsFormattingTag(tag))
                {
                    index = end + 1;
                    continue;
                }
            }
        }
        visible += text[index++];
    }
    return visible;
}

std::vector<std::string> WrapNoticeText(const std::string& text, float maxWidth)
{
    std::vector<std::string> lines;
    std::string line;
    std::string word;
    std::string wordStartFormatting;
    std::string activeFormatting;

    auto beginWord = [&]()
    {
        if (word.empty())
            wordStartFormatting = activeFormatting;
    };
    auto appendLine = [&](bool allowEmpty)
    {
        if (allowEmpty || !GetVisibleText(line).empty())
            lines.push_back(line);
        line.clear();
    };
    auto appendWord = [&]()
    {
        if (word.empty())
            return;
        const std::string candidate = line.empty() ? wordStartFormatting + word : line + " " + word;
        if (!line.empty() && Game::Print::GetTextWidth(GetVisibleText(candidate)) > maxWidth)
        {
            appendLine(false);
            line = wordStartFormatting + word;
        }
        else
        {
            line = candidate;
        }
        word.clear();
        wordStartFormatting.clear();
    };

    for (std::string::size_type index = 0; index < text.length();)
    {
        if (text[index] == '\r')
        {
            ++index;
            continue;
        }
        if (text[index] == '\n')
        {
            appendWord();
            appendLine(true);
            ++index;
            continue;
        }
        if (text[index] == '~')
        {
            const std::string::size_type end = text.find('~', index + 1);
            if (end != std::string::npos)
            {
                const std::string tag = text.substr(index, end - index + 1);
                if (tag == "~n~")
                {
                    appendWord();
                    appendLine(true);
                    index = end + 1;
                    continue;
                }
                beginWord();
                word += tag;
                if (tag == "~s~")
                    activeFormatting.clear();
                else if (IsPersistentFormattingTag(tag))
                    activeFormatting += tag;
                index = end + 1;
                continue;
            }
        }
        if (text[index] == ' ' || text[index] == '\t')
        {
            appendWord();
            ++index;
            continue;
        }
        beginWord();
        word += text[index++];
    }

    appendWord();
    if (!line.empty() || lines.empty())
        appendLine(true);
    return lines;
}

void DrawNoticePanel(const std::string& title, const std::vector<std::string>& lines)
{
    constexpr float panelWidth = 0.27f;
    constexpr float stripeWidth = 0.003f;
    constexpr float leftPadding = 0.014f;
    constexpr float rightPadding = 0.012f;
    constexpr float topPadding = 0.012f;
    constexpr float bottomPadding = 0.012f;
    constexpr float titleLineHeight = 0.03f;
    constexpr float descriptionLineHeight = 0.02f;
    constexpr float titleTextScale = 0.40f;
    constexpr float descriptionTextScale = 0.33f;
    constexpr float rightMargin = 0.012f;
    constexpr float bottomMargin = 0.06f;

    const float panelHeight = topPadding + bottomPadding + titleLineHeight + static_cast<float>(lines.size()) * descriptionLineHeight;
    const float panelX = 1.0f - rightMargin - panelWidth;
    const float panelY = 1.0f - bottomMargin - panelHeight;
    const float textX = panelX + stripeWidth + leftPadding;

    DRAW_RECT(panelX + panelWidth / 2.0f, panelY + panelHeight / 2.0f, panelWidth, panelHeight, BG.R, BG.G, BG.B, BG.A, false);
    DRAW_RECT(panelX + stripeWidth / 2.0f, panelY + panelHeight / 2.0f, stripeWidth, panelHeight, titlebox.R, titlebox.G, titlebox.B, titlebox.A, false);

    Game::Print::SetupDraw(font_title, Vector2(titleTextScale, titleTextScale), false, false, true, titletext);
    Game::Print::drawstring("~o~" + title, textX, panelY + topPadding);

    float textY = panelY + topPadding + titleLineHeight;
    for (const auto& line : lines)
    {
        Game::Print::SetupDraw(font_options, Vector2(descriptionTextScale, descriptionTextScale), false, false, true, optiontext, {0.0f, panelX + panelWidth - rightPadding});
        Game::Print::drawstring(line, textX, textY);
        textY += descriptionLineHeight;
    }
}

void DrawNotice(const ReleaseInfo& info, State state)
{
    Game::Print::SetupDraw(font_options, Vector2(0.33f, 0.33f), false, false, false, optiontext);

    std::vector<std::string> bodyLines;
    if (state == State::Downloading || state == State::InstallPending)
    {
        bodyLines = WrapNoticeText(Language::Format(state == State::Downloading ? "message.updater.downloading" : "message.updater.install_pending", {{"version", info.version}}), 0.241f);
    }
    else
    {
        bodyLines = WrapNoticeText(Language::Format("message.updater.available", {{"version", info.version}, {"installed", MENYOO_VERSION_TEXT}}), 0.241f);

        Keybinds::AddBindIB("update_install", Language::Translate("message.updater.prompt_install"));
        Keybinds::AddBindIB("update_changelog", Language::Translate("message.updater.prompt_changelog"));
        Keybinds::AddBindIB("menu_open", Language::Translate("message.updater.prompt_hide"));
    }

    DrawNoticePanel(Language::Translate("message.updater.title"), bodyLines);
}

void LogStateOnce(State state)
{
    static State previousState = State::NotStarted;
    if (previousState == state)
        return;

    previousState = state;
    switch (state)
    {
    case State::Failed:
        addlog(ige::LogType::LOG_WARNING, "Update check or download failed; the installed Menyoo.asi was left untouched.");
        break;
    case State::UpdateAvailable:
        addlog(ige::LogType::LOG_INFO, "A newer Menyoo release is available.");
        break;
    case State::InstallPending:
        addlog(ige::LogType::LOG_INFO, "Menyoo update installed; it takes effect after GTA V restarts.");
        break;
    case State::UpToDate:
        addlog(ige::LogType::LOG_TRACE, "Menyoo is up to date.");
        break;
    default:
        break;
    }
}
} // namespace

void Tick()
{
    if (!g_checkStarted.exchange(true))
    {
        if (ignoreUpdateChecks)
        {
            g_state = State::UpToDate;
            addlog(ige::LogType::LOG_INFO, "Menyoo update checks are disabled in menyooConfig.ini.");
        }
        else
        {
            g_modulePath = GetModulePath();
            if (!g_modulePath.empty())
            {
                g_targetDirectory = std::filesystem::path(g_modulePath).parent_path().wstring();
                g_state = State::Checking;
                StartWorker(false);
            }
            else
            {
                g_state = State::Failed;
            }
        }
    }

    const State currentState = g_state.load();
    LogStateOnce(currentState);

    static bool downloadFailureShown = false;
    if (currentState == State::Failed && g_downloadStarted && !downloadFailureShown)
    {
        downloadFailureShown = true;
        ReleaseInfo info;
        {
            std::lock_guard<std::mutex> lock(g_releaseMutex);
            info = g_release;
        }
        const char* key = g_installFailed ? "message.updater.install_failed" : "message.updater.download_failed";
        Game::Print::ShowNotification(Language::Translate("common.error.title"), Language::Format(key, {{"version", info.version}}));
    }

    if (ignoreUpdateChecks || Menu::activeSubmenu != SUB::CLOSED)
        g_dismissed = true;

    if (currentState == State::UpdateAvailable && !g_dismissed)
    {
        ReleaseInfo info;
        {
            std::lock_guard<std::mutex> lock(g_releaseMutex);
            info = g_release;
        }

        if (Keybinds::WasPressedThisFrame("update_install", Keybinds::Context::Keyboard) && !g_downloadStarted.exchange(true))
        {
            g_state = State::Downloading;
            StartWorker(true);
        }
        else if (Keybinds::WasPressedThisFrame("update_changelog", Keybinds::Context::Keyboard))
        {
            if (!OpenUrl(info.changelogUrl))
                OpenUrl(kReleasePageUrl);
        }
        else
        {
            DrawNotice(info, currentState);
        }
    }
    else if ((currentState == State::Downloading || currentState == State::InstallPending) && !g_dismissed)
    {
        ReleaseInfo info;
        {
            std::lock_guard<std::mutex> lock(g_releaseMutex);
            info = g_release;
        }
        DrawNotice(info, currentState);
    }
}
} // namespace Updates
