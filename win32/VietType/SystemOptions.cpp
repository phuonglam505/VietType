// SPDX-License-Identifier: GPL-3.0-or-later
#include <shlobj.h>
#include <winhttp.h>
#include <wrl/client.h>
#include <array>
#include <thread>
#include <utility>
#include <winrt/Windows.Data.Json.h>
#include "ReleaseVersion.h"
#include "SystemOptions.h"

namespace viettype {
namespace {
constexpr auto runKey = L"Software\\Microsoft\\Windows\\CurrentVersion\\Run";
constexpr auto runValue = L"VietTypePortable";

std::wstring shortcutPath()
{
    PWSTR desktop = nullptr;
    if (FAILED(SHGetKnownFolderPath(FOLDERID_Desktop, 0, nullptr, &desktop))) return {};
    std::wstring path = desktop;
    CoTaskMemFree(desktop);
    return path + L"\\VietType.lnk";
}

constexpr bool deleted(DWORD error)
{
    return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND;
}

struct InternetHandle {
    HINTERNET value;
    ~InternetHandle() { if (value) WinHttpCloseHandle(value); }
};

enum class UpdateResult { unavailable, noRelease, current, newer };
bool checkInProgress = false;
bool interactiveRequested = false;

UpdateResult fetchRelease()
{
    InternetHandle session{WinHttpOpen(L"VietType", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                                       WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0)};
    if (!session.value) return UpdateResult::unavailable;
    WinHttpSetTimeouts(session.value, 3000, 3000, 3000, 3000);
    InternetHandle connection{WinHttpConnect(session.value, L"api.github.com", INTERNET_DEFAULT_HTTPS_PORT, 0)};
    if (!connection.value) return UpdateResult::unavailable;
    InternetHandle request{WinHttpOpenRequest(connection.value, L"GET",
        L"/repos/phuonglam505/VietType/releases/latest", nullptr, WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE)};
    if (!request.value) return UpdateResult::unavailable;
    constexpr auto headers = L"Accept: application/vnd.github+json\r\nX-GitHub-Api-Version: 2026-03-10\r\n";
    if (!WinHttpSendRequest(request.value, headers, DWORD(-1), nullptr, 0, 0, 0) ||
        !WinHttpReceiveResponse(request.value, nullptr)) return UpdateResult::unavailable;

    DWORD status = 0, length = sizeof(status);
    if (!WinHttpQueryHeaders(request.value, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                             WINHTTP_HEADER_NAME_BY_INDEX, &status, &length, WINHTTP_NO_HEADER_INDEX)) {
        return UpdateResult::unavailable;
    }
    if (status == 404) return UpdateResult::noRelease;
    if (status != 200) return UpdateResult::unavailable;

    std::string body;
    std::array<char, 4096> buffer{};
    for (;;) {
        DWORD count = 0;
        if (!WinHttpReadData(request.value, buffer.data(), static_cast<DWORD>(buffer.size()), &count)) {
            return UpdateResult::unavailable;
        }
        if (!count) break;
        if (body.size() + count > 1024 * 1024) return UpdateResult::unavailable;
        body.append(buffer.data(), count);
    }

    const auto release = winrt::Windows::Data::Json::JsonObject::Parse(winrt::to_hstring(body));
    const auto latest = parseReleaseVersion(winrt::to_string(release.GetNamedString(L"tag_name")));
    const auto installed = parseReleaseVersion(winrt::to_string(VIETTYPE_VERSION_DISPLAY));
    if (!latest || !installed) return UpdateResult::unavailable;
    return *latest > *installed ? UpdateResult::newer : UpdateResult::current;
}

}

int showPanelAtLaunch() { return VietTypeHelper::getRegInt(L"showPanelAtLaunch", 1); }
int runElevated() { return VietTypeHelper::getRegInt(L"runElevated", 0); }
int checkUpdatesAtLaunch() { return VietTypeHelper::getRegInt(L"checkUpdatesAtLaunch", 1); }

bool isElevated() { return IsUserAnAdmin() != FALSE; }

bool startElevated(LPCWSTR arguments)
{
    return reinterpret_cast<INT_PTR>(ShellExecute(nullptr, L"runas",
        VietTypeHelper::getFullPath().c_str(), arguments, nullptr, SW_SHOWNORMAL)) > 32;
}

bool startsWithWindows()
{
    return RegGetValue(HKEY_CURRENT_USER, runKey, runValue, RRF_RT_REG_SZ,
                       nullptr, nullptr, nullptr) == ERROR_SUCCESS;
}

bool setStartsWithWindows(bool enabled)
{
    HKEY key = nullptr;
    if (RegCreateKeyEx(HKEY_CURRENT_USER, runKey, 0, nullptr, 0, KEY_SET_VALUE,
                       nullptr, &key, nullptr) != ERROR_SUCCESS) return false;
    const auto command = L"\"" + VietTypeHelper::getFullPath() + L"\" --startup";
    const auto result = enabled ? RegSetValueEx(key, runValue, 0, REG_SZ,
        reinterpret_cast<const BYTE*>(command.c_str()), static_cast<DWORD>((command.size()+1)*sizeof(wchar_t))) :
        RegDeleteValue(key, runValue);
    RegCloseKey(key);
    return result == ERROR_SUCCESS || (!enabled && deleted(result));
}

bool hasDesktopShortcut()
{
    const auto path = shortcutPath();
    return !path.empty() && GetFileAttributes(path.c_str()) != INVALID_FILE_ATTRIBUTES;
}

bool setDesktopShortcut(bool enabled)
{
    const auto path = shortcutPath();
    if (path.empty()) return false;
    
    const auto result = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(result)) return false;
    constexpr auto description = L"Bộ gõ tiếng Việt VietType";
    const auto attributes = GetFileAttributes(path.c_str());
    const auto pathError = attributes == INVALID_FILE_ATTRIBUTES ? GetLastError() : ERROR_SUCCESS;
    bool saved = !enabled && deleted(pathError);
    {
        Microsoft::WRL::ComPtr<IShellLinkW> link;
        Microsoft::WRL::ComPtr<IPersistFile> persist;
        const auto target = VietTypeHelper::getFullPath();
        if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&link))) &&
            SUCCEEDED(link.As(&persist))) {
            wchar_t existingDescription[128]{};
            const bool owned = attributes != INVALID_FILE_ATTRIBUTES &&
                SUCCEEDED(persist->Load(path.c_str(), STGM_READ)) &&
                SUCCEEDED(link->GetDescription(existingDescription, ARRAYSIZE(existingDescription))) &&
                wcscmp(description, existingDescription) == 0;
            if (enabled && (owned || deleted(pathError)) &&
                SUCCEEDED(link->SetPath(target.c_str())) &&
                SUCCEEDED(link->SetDescription(description))) {
                saved = SUCCEEDED(persist->Save(path.c_str(), TRUE));
            } else if (!enabled && owned) {
                saved = DeleteFile(path.c_str()) || deleted(GetLastError());
            }
        }
    }
    CoUninitialize();
    return saved;
}

void checkForUpdate(HWND parent, bool interactive)
{
    interactiveRequested = interactiveRequested || interactive;
    if (checkInProgress) return;
    checkInProgress = true;
    try {
        std::thread([parent] {
            auto result = UpdateResult::unavailable;
            try {
                winrt::init_apartment();
                try { result = fetchRelease(); } catch (...) {}
                winrt::uninit_apartment();
            } catch (...) {}
            PostMessage(parent, updateResultMessage, static_cast<WPARAM>(result), 0);
        }).detach();
    } catch (const std::system_error&) {
        onUpdateResult(parent, static_cast<WPARAM>(UpdateResult::unavailable));
    }
}

void onUpdateResult(HWND parent, WPARAM result)
{
    checkInProgress = false;
    const bool interactive = std::exchange(interactiveRequested, false);
    switch (static_cast<UpdateResult>(result)) {
    case UpdateResult::newer:
        if (MessageBox(parent, L"Đã có bản VietType mới. Mở GitHub Releases để tải?",
                       L"Cập nhật VietType", MB_YESNO | MB_ICONINFORMATION) == IDYES) {
            ShellExecute(parent, L"open", L"https://github.com/phuonglam505/VietType/releases/latest",
                         nullptr, nullptr, SW_SHOWNORMAL);
        }
        return;
    case UpdateResult::current:
        if (interactive) MessageBox(parent, L"Bạn đang dùng bản VietType mới nhất.",
                                     L"Cập nhật VietType", MB_OK | MB_ICONINFORMATION);
        return;
    case UpdateResult::noRelease:
        if (interactive) MessageBox(parent, L"Chưa tìm thấy bản phát hành công khai của VietType trên GitHub.",
                                     L"Cập nhật VietType", MB_OK | MB_ICONINFORMATION);
        return;
    case UpdateResult::unavailable:
        if (interactive) MessageBox(parent, L"Không kiểm tra được bản mới trên GitHub. Hãy thử lại sau.",
                                     L"Cập nhật VietType", MB_OK | MB_ICONWARNING);
        return;
    }
}
}
