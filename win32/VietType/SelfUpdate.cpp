#include "UiLanguage.h"
// SPDX-License-Identifier: GPL-3.0-or-later
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <string_view>
#include <windows.h>
#include <winhttp.h>
#include <bcrypt.h>
#include "SelfUpdate.h"

namespace viettype {
namespace {
std::wstring pendingBackup;
constexpr size_t sizeLimit = 32 * 1024 * 1024;

struct Internet {
    HINTERNET value;
    ~Internet() { if (value) WinHttpCloseHandle(value); }
};

struct Handle {
    HANDLE value;
    ~Handle() { if (value && value != INVALID_HANDLE_VALUE) CloseHandle(value); }
};

bool isHex(wchar_t ch)
{
    return (ch >= L'0' && ch <= L'9') || (ch >= L'a' && ch <= L'f');
}

bool validPath(std::wstring_view url)
{
    constexpr std::wstring_view origin = L"https://github.com/phuonglam505/VietType/releases/download/";
    constexpr std::wstring_view filename = L"/VietType.exe";
    if (url.size() <= origin.size() + filename.size() ||
        url.substr(0, origin.size()) != origin || url.substr(url.size() - filename.size()) != filename) return false;
    const auto tag = url.substr(origin.size(), url.size() - origin.size() - filename.size());
    return tag != L"." && tag != L".." && tag.find_first_not_of(
        L"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_.%") == std::wstring_view::npos;
}

bool checkHash(const std::vector<BYTE>& payload, std::wstring_view expected)
{
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0) return false;
    std::array<BYTE, 32> hash{};
    const auto status = BCryptHash(algorithm, nullptr, 0, const_cast<BYTE*>(payload.data()),
                                   static_cast<ULONG>(payload.size()), hash.data(), static_cast<ULONG>(hash.size()));
    BCryptCloseAlgorithmProvider(algorithm, 0);
    if (status < 0) return false;
    constexpr wchar_t digits[] = L"0123456789abcdef";
    for (size_t i = 0; i < hash.size(); ++i) {
        if (expected[7 + 2*i] != digits[hash[i] >> 4] ||
            expected[8 + 2*i] != digits[hash[i] & 15]) return false;
    }
    return true;
}

bool validBinary(const std::vector<BYTE>& bytes)
{
    IMAGE_DOS_HEADER dos{};
    if (bytes.size() < sizeof(dos)) return false;
    std::memcpy(&dos, bytes.data(), sizeof(dos));
    if (dos.e_magic != IMAGE_DOS_SIGNATURE || dos.e_lfanew < 0) return false;
    const auto offset = static_cast<size_t>(dos.e_lfanew);
    IMAGE_NT_HEADERS64 pe{};
    if (offset > bytes.size() || bytes.size() - offset < sizeof(pe)) return false;
    std::memcpy(&pe, bytes.data()+offset, sizeof(pe));
    return pe.Signature == IMAGE_NT_SIGNATURE && pe.FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64 &&
           !(pe.FileHeader.Characteristics & IMAGE_FILE_DLL) &&
           pe.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC;
}

bool matchesVersion(const std::wstring& path, const ReleaseVersion& expected)
{
    DWORD handle = 0;
    const DWORD size = GetFileVersionInfoSizeW(path.c_str(), &handle);
    if (!size) return false;
    std::vector<BYTE> data(size);
    if (!GetFileVersionInfoW(path.c_str(), 0, size, data.data())) return false;
    void* info = nullptr;
    UINT length = 0;
    if (!VerQueryValueW(data.data(), L"\\", &info, &length) || length < sizeof(VS_FIXEDFILEINFO)) return false;
    const auto& version = *static_cast<VS_FIXEDFILEINFO*>(info);
    return version.dwSignature == VS_FFI_SIGNATURE &&
           expected == ReleaseVersion{HIWORD(version.dwProductVersionMS), LOWORD(version.dwProductVersionMS),
                                       HIWORD(version.dwProductVersionLS)};
}

bool restart(const std::wstring& target, const std::wstring& id)
{
    const HANDLE self = GetCurrentProcess();
    Handle wait{nullptr};
    if (!DuplicateHandle(self, self, self, &wait.value, SYNCHRONIZE, TRUE, 0)) return false;

    SECURITY_ATTRIBUTES share{sizeof(share), nullptr, TRUE};
    Handle ready{CreateEventW(&share, TRUE, FALSE, nullptr)};
    if (!ready.value) return false;
    std::array<HANDLE, 2> inherited{wait.value, ready.value};

    SIZE_T bytes = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &bytes);
    if (!bytes) return false;
    std::vector<BYTE> storage(bytes);
    auto list = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(storage.data());
    if (!InitializeProcThreadAttributeList(list, 1, 0, &bytes)) return false;
    STARTUPINFOEXW startup{};
    startup.StartupInfo.cb = sizeof(startup);
    startup.lpAttributeList = list;
    PROCESS_INFORMATION child{};
    const auto handle = std::to_wstring(reinterpret_cast<uintptr_t>(wait.value));
    const auto readyHandle = std::to_wstring(reinterpret_cast<uintptr_t>(ready.value));
    auto command = L"\"" + target + L"\" --finish-update " + handle + L" " + readyHandle + L" " + id;
    const bool started = UpdateProcThreadAttribute(list, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
        inherited.data(), sizeof(inherited), nullptr, nullptr) &&
        CreateProcessW(target.c_str(), command.data(), nullptr, nullptr, TRUE, EXTENDED_STARTUPINFO_PRESENT,
                       nullptr, nullptr, &startup.StartupInfo, &child);
    DeleteProcThreadAttributeList(list);
    if (!started) return false;
    Handle childProcess{child.hProcess};
    CloseHandle(child.hThread);
    const HANDLE states[] = {ready.value, childProcess.value};
    if (WaitForMultipleObjects(ARRAYSIZE(states), states, FALSE, 8000) == WAIT_OBJECT_0) return true;
    if (WaitForSingleObject(childProcess.value, 0) == WAIT_TIMEOUT) {
        TerminateProcess(childProcess.value, 1);
        WaitForSingleObject(childProcess.value, 3000);
    }
    return false;
}
}

bool validUpdateAsset(const UpdateAsset& asset)
{
    return validPath(asset.url) && asset.digest.size() == 71 && asset.digest.compare(0, 7, L"sha256:") == 0 &&
           std::all_of(asset.digest.begin()+7, asset.digest.end(), isHex);
}

std::vector<BYTE> downloadUpdate(const UpdateAsset& asset)
{
    if (!validUpdateAsset(asset)) return {};
    constexpr std::wstring_view origin = L"https://github.com";
    const auto path = asset.url.substr(origin.size());
    Internet session{WinHttpOpen(L"VietType", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
                                 WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0)};
    if (!session.value) return {};
    WinHttpSetTimeouts(session.value, 5000, 5000, 10000, 10000);
    Internet connection{WinHttpConnect(session.value, L"github.com", INTERNET_DEFAULT_HTTPS_PORT, 0)};
    if (!connection.value) return {};
    Internet request{WinHttpOpenRequest(connection.value, L"GET", path.c_str(), nullptr,
        WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE)};
    if (!request.value) return {};
    DWORD redirects = WINHTTP_OPTION_REDIRECT_POLICY_DISALLOW_HTTPS_TO_HTTP;
    if (!WinHttpSetOption(request.value, WINHTTP_OPTION_REDIRECT_POLICY, &redirects, sizeof(redirects))) return {};
    if (!WinHttpSendRequest(request.value, WINHTTP_NO_ADDITIONAL_HEADERS, 0, nullptr, 0, 0, 0) ||
        !WinHttpReceiveResponse(request.value, nullptr)) return {};
    DWORD status = 0, statusSize = sizeof(status);
    if (!WinHttpQueryHeaders(request.value, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                             WINHTTP_HEADER_NAME_BY_INDEX, &status, &statusSize, WINHTTP_NO_HEADER_INDEX)
        || status != 200) return {};
    std::array<BYTE, 16384> buffer{};
    std::vector<BYTE> bytes;
    for (;;) {
        DWORD count = 0;
        if (!WinHttpReadData(request.value, buffer.data(), static_cast<DWORD>(buffer.size()), &count)) return {};
        if (!count) break;
        if (bytes.size() + count > sizeLimit) return {};
        bytes.insert(bytes.end(), buffer.data(), buffer.data()+count);
    }
    if (!validBinary(bytes) || !checkHash(bytes, asset.digest)) return {};
    return bytes;
}

bool replaceAndRestart(const UpdateAsset& asset, const std::vector<BYTE>& payload, std::wstring& error)
{
    error = viettype::tr(L"Không thể thay tệp VietType.exe. Kiểm tra quyền ghi vào thư mục chứa ứng dụng và thử lại.");
    if (payload.size() > sizeLimit || !validUpdateAsset(asset) ||
        !validBinary(payload) || !checkHash(payload, asset.digest)) return false;

    const std::wstring target = VietTypeHelper::getFullPath();
    if (target.empty() || target.size() >= MAX_PATH - 1) return false;
    GUID key{};
    wchar_t id[39]{};
    if (FAILED(CoCreateGuid(&key)) || !StringFromGUID2(key, id, ARRAYSIZE(id))) return false;
    const auto staging = target + L".update-" + id;
    const auto backup = target + L".previous-" + id;
    bool written = false;
    {
        Handle file{CreateFileW(staging.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                                FILE_ATTRIBUTE_NORMAL, nullptr)};
        if (file.value == INVALID_HANDLE_VALUE) return false;
        DWORD count = 0;
        written = WriteFile(file.value, payload.data(), static_cast<DWORD>(payload.size()), &count, nullptr) &&
                  count == payload.size() && FlushFileBuffers(file.value);
    }
    if (!written || !matchesVersion(staging, asset.version)) {
        if (written) error = viettype::tr(L"Phiên bản bên trong tệp tải về không khớp với bản phát hành GitHub.");
        DeleteFileW(staging.c_str());
        return false;
    }
    if (!MoveFileExW(target.c_str(), backup.c_str(), MOVEFILE_WRITE_THROUGH)) {
        DeleteFileW(staging.c_str());
        return false;
    }
    if (MoveFileExW(staging.c_str(), target.c_str(), MOVEFILE_WRITE_THROUGH) && restart(target, id)) return true;

    if (!MoveFileExW(backup.c_str(), target.c_str(), MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
        error = std::wstring(viettype::tr(L"Không thể mở bản mới hoặc khôi phục tự động. Tệp VietType cũ vẫn nằm ở:\n")) + backup;
    }
    DeleteFileW(staging.c_str());
    return false;
}

bool completeUpdate(LPCWSTR arguments)
{
    constexpr std::wstring_view command = L"--finish-update";
    std::wstring_view value(arguments);
    if (value.substr(0, command.size()) != command) return true;
    if (value.size() <= command.size() || value[command.size()] != L' ') return false;
    value.remove_prefix(command.size()+1);
    const auto readHandle = [&value]() -> HANDLE {
        const auto separator = value.find(L' ');
        if (separator == std::wstring_view::npos || !separator) return nullptr;
        uintptr_t number = 0;
        for (const auto ch : value.substr(0, separator)) {
            if (ch < L'0' || ch > L'9' || number > (UINTPTR_MAX - (ch-L'0')) / 10) return nullptr;
            number = number*10 + (ch-L'0');
        }
        value.remove_prefix(separator+1);
        return reinterpret_cast<HANDLE>(number);
    };
    Handle previous{readHandle()};
    Handle ready{readHandle()};
    const std::wstring id(value);
    GUID key{};
    if (!previous.value || !ready.value || id.size() != 38 || id.front() != L'{' || id.back() != L'}' ||
        FAILED(CLSIDFromString(id.c_str(), &key)) || !SetEvent(ready.value)) return false;
    if (WaitForSingleObject(previous.value, 15000) != WAIT_OBJECT_0) return false;
    pendingBackup = VietTypeHelper::getFullPath() + L".previous-" + id;
    return true;
}
void confirmUpdateReady() {
    if (!pendingBackup.empty() && DeleteFileW(pendingBackup.c_str())) pendingBackup.clear();
}
}
