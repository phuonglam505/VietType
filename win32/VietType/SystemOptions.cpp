// SPDX-License-Identifier: GPL-3.0-or-later
#include <shlobj.h>
#include <winhttp.h>
#include <wrl/client.h>
#include <array>
#include <thread>
#include <mutex>
#include <memory>
#include <utility>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Data.Json.h>
#include "SelfUpdate.h"
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

enum class UpdateResult { unavailable, noRelease, unrecognizedVersion, current, newer, noAsset, downloaded, downloadFailed };
struct UpdateNotice {
    UpdateResult result = UpdateResult::unavailable;
    UpdateAsset asset;
    std::vector<BYTE> payload;
};

struct UpdateMailbox {
    std::mutex mutex;
    std::optional<UpdateNotice> pending;
};

const auto updateMailbox = std::make_shared<UpdateMailbox>();
bool updateInProgress = false;
std::optional<UpdateAsset> availableUpdate;
std::optional<ReleaseVersion> notifiedVersion;

std::wstring updateVersionLabel(const ReleaseVersion& version)
{
    return std::to_wstring(version[0]) + L"." + std::to_wstring(version[1]) + L"." +
           std::to_wstring(version[2]);
}

void publishUpdate(const std::shared_ptr<UpdateMailbox>& mailbox, HWND parent, UpdateNotice notice)
{
    const std::lock_guard<std::mutex> guard(mailbox->mutex);
    mailbox->pending = std::move(notice);
    PostMessage(parent, updateResultMessage, 0, 0);
}


UpdateResult fetchRelease(UpdateAsset& asset)
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
    const auto latest = parseGitHubReleaseVersion(
        winrt::to_string(release.GetNamedString(L"tag_name")),
        winrt::to_string(release.GetNamedString(L"name", L"")));
    const auto installed = parseReleaseVersion(winrt::to_string(VIETTYPE_VERSION_DISPLAY));
    if (!latest || !installed) return UpdateResult::unrecognizedVersion;
    if (*latest <= *installed) return UpdateResult::current;
    for (const auto& value : release.GetNamedArray(L"assets")) {
        const auto file = value.GetObject();
        if (file.GetNamedString(L"name", L"") != L"VietType.exe") continue;
        asset = {*latest, std::wstring(file.GetNamedString(L"browser_download_url", L"")),
                 std::wstring(file.GetNamedString(L"digest", L""))};
        if (validUpdateAsset(asset)) return UpdateResult::newer;
    }
    return UpdateResult::noAsset;
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

void checkForUpdate(HWND parent)
{
    if (updateInProgress) return;
    updateInProgress = true;
    try {
        std::thread([parent, mailbox=updateMailbox] {
            UpdateNotice notice;
            try {
                winrt::init_apartment();
                try { notice.result = fetchRelease(notice.asset); } catch (...) {}
                winrt::uninit_apartment();
            } catch (...) {}
            publishUpdate(mailbox, parent, std::move(notice));
        }).detach();
    } catch (const std::system_error&) {
        publishUpdate(updateMailbox, parent, {});
    }
}

static void beginUpdateDownload(HWND parent, UpdateAsset asset)
{
    updateInProgress = true;
    try {
        std::thread([parent, mailbox=updateMailbox, asset=std::move(asset)]() mutable {
            UpdateNotice notice;
            notice.asset = std::move(asset);
            notice.result = UpdateResult::downloadFailed;
            try {
                notice.payload = downloadUpdate(notice.asset);
                if (!notice.payload.empty()) notice.result = UpdateResult::downloaded;
            } catch (...) {}
            publishUpdate(mailbox, parent, std::move(notice));
        }).detach();
    } catch (const std::system_error&) {
        publishUpdate(updateMailbox, parent, {UpdateResult::downloadFailed, {}, {}});
    }
}

void installAvailableUpdate(HWND parent, HWND button, HWND status)
{
    if (updateInProgress || !availableUpdate) return;
    EnableWindow(button, FALSE);
    SetWindowText(status, L"Đang tải và xác minh bản cập nhật…");
    beginUpdateDownload(parent, *availableUpdate);
}

void onUpdateResult(HWND parent, HWND button, HWND status)
{
    std::optional<UpdateNotice> notice;
    {
        const std::lock_guard<std::mutex> guard(updateMailbox->mutex);
        notice = std::exchange(updateMailbox->pending, std::nullopt);
    }
    if (!notice) return;
    updateInProgress = false;
    constexpr auto caption = L"Cập nhật VietType";
    switch (notice->result) {
    case UpdateResult::newer: {
        availableUpdate = std::move(notice->asset);
        const auto version = availableUpdate->version;
        const auto message = L"Đã có VietType " + updateVersionLabel(version) + L".";
        SetWindowText(status, message.c_str());
        EnableWindow(button, TRUE);
        if (notifiedVersion != version) {
            notifiedVersion = version;
            const auto details = message + L" Mở tab Thông tin và chọn Cập nhật để cài đặt.";
            MessageBox(parent, details.c_str(), caption, MB_OK | MB_ICONINFORMATION);
        }
        return;
    }
    case UpdateResult::downloaded: {
        std::wstring error;
        if (replaceAndRestart(notice->asset, notice->payload, error)) {
            PostQuitMessage(0);
            return;
        }
        EnableWindow(button, TRUE);
        SetWindowText(status, L"Cài đặt chưa thành công.");
        MessageBox(parent, error.c_str(), caption, MB_OK | MB_ICONERROR);
        return;
    }
    case UpdateResult::downloadFailed:
        EnableWindow(button, TRUE);
        SetWindowText(status, L"Tải hoặc xác minh bản cập nhật chưa thành công.");
        MessageBox(parent, L"Không tải hoặc xác minh được tệp cập nhật. Hãy thử lại sau.",
                   caption, MB_OK | MB_ICONWARNING);
        return;
    case UpdateResult::noAsset:
        availableUpdate.reset();
        SetWindowText(status, L"Bản mới trên GitHub chưa có tệp cập nhật hợp lệ.");
        break;
    case UpdateResult::current:
        availableUpdate.reset();
        SetWindowText(status, L"Bạn đang dùng bản mới nhất.");
        break;
    case UpdateResult::noRelease:
        availableUpdate.reset();
        SetWindowText(status, L"Chưa tìm thấy bản phát hành trên GitHub.");
        break;
    case UpdateResult::unrecognizedVersion:
        SetWindowText(status, L"Chưa đọc được số phiên bản trên GitHub.");
        break;
    case UpdateResult::unavailable:
        SetWindowText(status, L"Chưa kiểm tra được bản mới trên GitHub.");
        break;
    }
    EnableWindow(button, availableUpdate.has_value());
}
}
