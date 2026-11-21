// SPDX-License-Identifier: GPL-3.0-or-later
#include "stdafx.h"
#include "UiLanguage.h"
#include <cwchar>

namespace viettype {
namespace {
bool english = false;
struct Translation { LPCWSTR vietnamese; LPCWSTR english; };
constexpr Translation translations[] = {
    {L"Điều khiển bộ gõ", L"Input controls"},
    {L"&Kiểu gõ", L"Input &method"},
    {L"&Bảng mã", L"&Encoding"},
    {L"Phím chuyển", L"Switch shortcut"},
    {L"Âm báo", L"Beep"},
    {L"Chế độ gõ", L"Input mode"},
    {L"Tiếng &Việt", L"&Vietnamese"},
    {L"Tiếng &Anh", L"&English"},
    {L"Chuyển mã", L"Convert encoding"},
    {L"Khôi phục mặc định", L"Restore defaults"},
    {L"Thoát", L"Exit"},
    {L"Đóng", L"Close"},
    {L"Bộ gõ", L"Input"},
    {L"Gõ tắt", L"Macros"},
    {L"Hệ thống", L"System"},
    {L"Thông tin", L"About"},
    {L"Đặt dấu oà, uý", L"Tone placement: oà, uý"},
    {L"Kiểm tra chính tả", L"Spell check"},
    {L"Sửa gợi ý trong trình duyệt và Excel", L"Fix suggestions in browsers and Excel"},
    {L"Khôi phục phím khi từ sai", L"Restore keys for invalid words"},
    {L"Viết hoa đầu câu", L"Capitalize sentences"},
    {L"Cho phép z, w, j, f làm phụ âm đầu", L"Allow initial z, w, j, f"},
    {L"Ghi nhớ chế độ theo ứng dụng", L"Remember input mode per app"},
    {L"Tạm tắt chính tả bằng Ctrl", L"Pause spell check with Ctrl"},
    {L"Ghi nhớ bảng mã theo ứng dụng", L"Remember encoding per app"},
    {L"Tạm tắt gõ tiếng Việt bằng Alt", L"Pause Vietnamese typing with Alt"},
    {L"Cho phép gõ tắt", L"Enable macros"},
    {L"Gõ nhanh Telex (cc → ch, gg → gi)", L"Quick Telex (cc → ch, gg → gi)"},
    {L"Gõ tắt trong chế độ tiếng Anh", L"Use macros in English mode"},
    {L"Phụ âm đầu (f → ph, j → gi, w → qu)", L"Initials (f → ph, j → gi, w → qu)"},
    {L"Viết hoa theo phím tắt", L"Match capitalization of shortcut"},
    {L"Phụ âm cuối (g → ng, h → nh, k → ch)", L"Finals (g → ng, h → nh, k → ch)"},
    {L"Mở bảng gõ tắt", L"Open macro table"},
    {L"Hỗ trợ ứng dụng Metro", L"Support Metro apps"},
    {L"Sửa lỗi gõ trên Chromium (beta)", L"Chromium typing fix (beta)"},
    {L"Hiện cửa sổ khi khởi động", L"Show window at launch"},
    {L"Biểu tượng khay hiện đại", L"Modern tray icon"},
    {L"Gửi ký tự qua clipboard", L"Send text through the clipboard"},
    {L"Bật quyền quản trị ở lần mở sau", L"Run as administrator next time"},
    {L"Kiểm tra bản mới khi khởi động", L"Check for updates at launch"},
    {L"Khởi động cùng Windows", L"Start with Windows"},
    {L"Tạo lối tắt trên Desktop", L"Create desktop shortcut"},
    {L"Giấy phép GPL", L"GPL license"},
    {L"Cập nhật", L"Update"},

    {L"Giấy phép VietType", L"VietType license"},
    {L"Chỉ dùng phím kết hợp", L"Modifier keys only"},
    {L"Telex - giữ từ tiếng Anh", L"Telex - preserve English words"},
    {L"VNI + Telex - giữ từ tiếng Anh", L"VNI + Telex - preserve English"},
    {L"Unicode tổ hợp", L"Unicode (decomposed)"},
    {L"Unicode dựng sẵn", L"Unicode (precomposed)"},
    {L"VietType - Gõ tắt", L"VietType - Macros"},
    {L"VietType - Chuyển mã", L"VietType - Encoding conversion"},
    {L"Từ gõ tắt", L"Shortcut"},
    {L"Nội dung đầy đủ", L"Expansion"},
    {L"+ Thêm", L"+ Add"},
    {L"+ Sửa", L"+ Edit"},
    {L"- Xóa", L"- Delete"},
    {L"Nạp từ file...", L"Import file..."},
    {L"Xuất ra file...", L"Export file..."},
    {L"Tự động đổi chữ hoa theo phím tắt", L"Match shortcut capitalization"},
    {L"Tùy chọn chung", L"General options"},
    {L"Lựa chọn", L"Settings"},
    {L"Chuyển sang chữ HOA", L"Convert to UPPERCASE"},
    {L"Loại bỏ dấu câu", L"Remove diacritics"},
    {L"Chuyển sang chữ thường", L"Convert to lowercase"},
    {L"Đặt chữ Hoa đầu câu", L"Capitalize sentence starts"},
    {L"Đặt chữ Hoa Sau Mỗi Từ", L"Capitalize every word"},
    {L"Thông báo khi chuyển xong", L"Notify when finished"},
    {L"Chuyển mã trong Clipboard", L"Convert clipboard text"},
    {L"Phím chuyển mã nhanh:", L"Quick convert shortcut:"},
    {L"Bảng mã nguồn", L"Source encoding"},
    {L"Bảng mã đích", L"Target encoding"},
    {L"Đảo", L"Swap"},
    {L"Bật Tiếng Việt", L"Enable Vietnamese"},
    {L"Kiểu gõ Telex", L"Telex input"},
    {L"Kiểu gõ VNI", L"VNI input"},
    {L"Kiểu gõ Simple Telex", L"Simple Telex input"},
    {L"Công cụ chuyển mã...", L"Encoding conversion…"},
    {L"Chuyển mã nhanh", L"Quick conversion"},
    {L"Mở VietType", L"Open VietType"},
    {L"Bảng mã khác", L"Other encodings"},
    {L"Bấm vào đây để bật gõ Tiếng Việt", L"Click to enable Vietnamese typing"},
    {L"Bấm vào đây để tắt gõ Tiếng Việt", L"Click to disable Vietnamese typing"},
    {L"Chuyển mã thành công!\nKết quả đã được lưu trong Clipboard!", L"Conversion complete.\nThe result is on the clipboard."},
    {L"Windows không lưu được tùy chọn này.", L"Windows could not save this setting."},
    {L"Khôi phục tùy chọn mặc định của VietType?", L"Restore the default VietType settings?"},

    {L"Không khởi tạo được bộ gõ. VietType chưa nhận phím.", L"Could not start the input engine. VietType is not processing keys."},
    {L"Không tạo được biểu tượng khay. Bản dự phòng (nếu có) vẫn được giữ.", L"Could not create the tray icon. Any backup will be kept."},
    {L"VietType sẽ chạy với quyền thông thường.", L"VietType will run without administrator privileges."},
    {L"Bạn hãy nhập từ cần gõ tắt!", L"Enter a shortcut."},
    {L"Bạn hãy chọn từ cần xoá!", L"Select a shortcut to delete."},
    {L"Bạn có muốn giữ lại dữ liệu hiện tại không?", L"Keep the existing macros?"},
    {L"Dữ liệu gõ tắt", L"Macro data"},
    {L"Đang tải và xác minh bản cập nhật…", L"Downloading and verifying the update…"},
    {L"Cập nhật VietType", L"VietType update"},
    {L"Đã có VietType ", L"VietType version "},
    {L" Mở tab Thông tin và chọn Cập nhật để cài đặt.", L" is available. Open About and select Update to install it."},
    {L"Cài đặt chưa thành công.", L"Could not install the update."},
    {L"Tải hoặc xác minh bản cập nhật chưa thành công.", L"Could not download or verify the update."},
    {L"Không tải hoặc xác minh được tệp cập nhật. Hãy thử lại sau.", L"Could not download or verify the update. Try again later."},
    {L"Bản mới trên GitHub chưa có tệp cập nhật hợp lệ.", L"The GitHub release has no valid update file."},
    {L"Bạn đang dùng bản mới nhất.", L"You are running the latest version."},
    {L"Chưa tìm thấy bản phát hành trên GitHub.", L"No GitHub release was found."},
    {L"Chưa đọc được số phiên bản trên GitHub.", L"Could not read the version on GitHub."},
    {L"Chưa kiểm tra được bản mới trên GitHub.", L"Could not check for updates on GitHub."},
    {L"Không thể thay tệp VietType.exe. Kiểm tra quyền ghi vào thư mục chứa ứng dụng và thử lại.", L"Could not replace VietType.exe. Check write access to the application folder and try again."},
    {L"Phiên bản bên trong tệp tải về không khớp với bản phát hành GitHub.", L"The downloaded executable version does not match the GitHub release."},
    {L"Không thể mở bản mới hoặc khôi phục tự động. Tệp VietType cũ vẫn nằm ở:\n", L"Could not start the new version or restore automatically. The backup is at:\n"},
    {L"cho Windows x64", L"for Windows x64"},
    {L"Một bản VietType khác đang chạy. Nhấp phải vào biểu tượng V/E ở khay hệ thống, chọn Thoát, rồi mở lại tệp này để xem cửa sổ đầy đủ.",
     L"Another version of VietType is running. Right-click its V/E tray icon, choose Exit, then reopen this file."},
    {L"Không gửi được chữ tiếng Việt. VietType đã chuyển tiếp phím gốc.", L"Could not send Vietnamese text. The original key was passed through."},
    {L"Có sử dụng mã nguồn OpenKey.", L"Uses OpenKey source code."},
    {L"Phát triển theo giấy phép GNU GPL v3.", L"Licensed under GNU GPL v3."},
    {L"Phím đã lưu (", L"Saved key ("},
};

void localizeCaption(HWND window)
{
    wchar_t caption[512]{};
    if (!GetWindowTextW(window, caption, ARRAYSIZE(caption))) return;
    for (const auto& entry : translations) {
        const auto from = english ? entry.vietnamese : entry.english;
        const auto to = english ? entry.english : entry.vietnamese;
        if (wcscmp(caption, from) == 0) {
            SetWindowTextW(window, to);
            return;
        }
        // The version number follows this localized prefix in the update status.
        if (wcscmp(entry.vietnamese, L"Đã có VietType ") == 0 &&
            wcsncmp(caption, from, wcslen(from)) == 0) {
            const auto text = std::wstring(to) + (caption + wcslen(from));
            SetWindowTextW(window, text.c_str());
            return;
        }
    }
}

BOOL CALLBACK localizeControl(HWND window, LPARAM)
{
    wchar_t name[32]{};
    GetClassNameW(window, name, ARRAYSIZE(name));
    if (wcscmp(name, L"Button") == 0 ||
        (wcscmp(name, L"Static") == 0 && (GetWindowLongPtr(window, GWL_STYLE) & SS_TYPEMASK) != SS_ICON)) {
        localizeCaption(window);
    }
    return TRUE;
}
}

void loadUiLanguage()
{
    english = VietTypeHelper::getRegInt(L"uiLanguage", 0) == 1;
}

bool isUiEnglish()
{
    return english;
}

void setUiEnglish(bool enabled)
{
    VietTypeHelper::setRegInt(L"uiLanguage", enabled ? 1 : 0);
    english = enabled;
}

LPCWSTR tr(LPCWSTR vietnamese)
{
    if (english && vietnamese) {
        for (const auto& entry : translations) {
            if (wcscmp(vietnamese, entry.vietnamese) == 0) return entry.english;
        }
    }
    return vietnamese;
}

void localizeDialog(HWND window)
{
    if (!window) return;
    localizeCaption(window);
    EnumChildWindows(window, localizeControl, 0);
}
}
