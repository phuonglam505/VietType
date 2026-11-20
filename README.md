# VietType

Bộ gõ tiếng Việt độc lập cho Windows x64. Giao diện VietType cung cấp chuyển Việt/Anh, Telex, VNI, Simple Telex, bảng mã, gõ tắt và chuyển mã. Dùng **Alt + Z** để chuyển Việt/Anh. Chọn **Đóng** để thu về khay hệ thống hoặc **Thoát** để dừng bộ gõ.

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

## Phát hành bản 0.2.7

1. Đưa nội dung thư mục mã nguồn này, gồm `LICENSE` và `CMakeLists.txt`, lên thư mục gốc của nhánh `main`.
2. Tạo GitHub Release từ đúng commit vừa tải lên, với tag `v0.2.7`.
3. Đính kèm `VietType.exe`, rồi công bố Release chính thức. Giữ liên kết tải mã nguồn của cùng tag ngay trên trang Release.

Với bản tiếp theo, tăng phiên bản trong `CMakeLists.txt` và phần thông tin phiên bản trong `win32/VietType/VietType.rc`, rồi build và tạo Release với tag tương ứng, ví dụ `v0.2.8`. GitHub Releases dùng tag dạng `vMAJOR.MINOR.PATCH` để VietType so sánh phiên bản.

VietType là bản sửa đổi từ OpenKey, được bổ sung giao diện Windows và kiểm tra cập nhật từ GitHub. Bản 0.2.7 đổi tên dự án và các tệp nguồn ngày 20/11/2026 theo giấy phép GPLv3. Xem `LICENSE` và các thông báo trong mã nguồn.
