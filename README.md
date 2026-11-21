# VietType

VietType là bộ gõ tiếng Việt gọn nhẹ cho Windows 64-bit. Tải `VietType.exe` từ [GitHub Releases](https://github.com/phuonglam505/VietType/releases/latest) rồi mở trực tiếp, không cần cài đặt.


## Tính năng chính

- **Kiểu gõ và bảng mã** — Telex, VNI, Simple Telex, Unicode, Unicode tổ hợp, TCVN3 (ABC), VNI Windows và Windows CP1258.
- **Chuyển Việt/Anh** — Dùng `Alt + Z` theo mặc định hoặc bấm biểu tượng V đỏ và E xanh ở khay hệ thống. Có thể đổi phím chuyển và bật âm báo.
- **Hỗ trợ khi gõ** — Kiểm tra chính tả, khôi phục từ gõ sai, tùy chọn cách đặt dấu, viết hoa đầu câu và ghi nhớ chế độ gõ, bảng mã theo từng ứng dụng.
- **Gõ tắt và chuyển mã** — Quản lý, nhập và xuất bảng gõ tắt. Chuyển bảng mã cho văn bản trong clipboard, đổi kiểu chữ hoặc bỏ dấu.
- **Tích hợp Windows** — Menu khay hệ thống, tùy chọn khởi động cùng Windows và kiểm tra bản mới từ GitHub Releases.

Chọn **Đóng** để thu cửa sổ về khay hệ thống hoặc **Thoát** để dừng bộ gõ. Trước khi mở bản mới, hãy thoát bản cũ ở khay.

## Mã nguồn

- `engine/` chứa mã xử lý tiếng Việt, gõ tắt và chuyển mã của VietType.
- `win32/VietType/MainWindow.cpp` và tài nguyên `IDD_VIETTYPE` định nghĩa cửa sổ chính.
- `win32/VietType/VietType.cpp` kết nối lõi xử lý với bàn phím Windows.

Lõi được biên dịch vào cùng tệp thực thi với giao diện và biểu tượng. Thông tin tác giả được giữ trong các tệp nguồn theo `LICENSE`.

## Build Release x64

Cần Visual Studio Build Tools 2022, Windows SDK và CMake 3.21 trở lên.

```powershell
cmake -S . -B build -G 'Visual Studio 17 2022' -A x64
cmake --build build --config Release
.\build\Release\VietType.exe
```

## Tùy chọn VietType

Chọn kiểu gõ, bảng mã, chế độ Việt/Anh và phím chuyển ở đầu cửa sổ. Các tab **Bộ gõ**, **Gõ tắt**, **Hệ thống** và **Thông tin** nằm ngay bên dưới. Các tùy chọn bộ gõ áp dụng ngay. Quyền quản trị áp dụng ở lần mở tiếp theo. **Khôi phục mặc định** đặt lại các tùy chọn và giữ danh sách gõ tắt.

## Cập nhật từ GitHub

Trong tab **Hệ thống**, chọn **Kiểm tra bản mới** hoặc bật **Kiểm tra bản mới khi khởi động**. VietType đọc Release chính thức mới nhất của kho [phuonglam505/VietType](https://github.com/phuonglam505/VietType/releases). Khi có phiên bản cao hơn, bạn có thể mở trang Release để tải. Việc kiểm tra chạy trong nền.

## Phát hành bản 0.2.12

1. Đưa nội dung thư mục mã nguồn này, gồm `LICENSE` và `CMakeLists.txt`, lên thư mục gốc của nhánh `main`.
2. Tạo GitHub Release từ đúng commit vừa tải lên, với tag `v0.2.12`.
3. Đính kèm `VietType.exe`, rồi công bố Release chính thức. Giữ liên kết tải mã nguồn của cùng tag ngay trên trang Release.


VietType là hoạt động giải trí của tác giả và là bản sửa đổi từ [OpenKey](https://open-key.org/) và [UniKey](https://www.unikey.org/), được bổ sung giao diện Windows và kiểm tra cập nhật từ GitHub. Bản 0.2.12 đổi tên dự án và các tệp nguồn ngày 20/11/2026 theo giấy phép GPLv3. Xem `LICENSE` và các thông báo trong mã nguồn.
