# Đăng VietType v2027.01 lên GitHub

1. Giải nén gói mã nguồn sạch. Đưa **nội dung bên trong** thư mục `VietType-GitHub` lên kho `phuonglam505/VietType`, gồm thư mục ẩn `.github`, `README.md`, `LICENSE` và `docs/screenshots`.
2. Tạo release với thẻ `v2027.01` trỏ tới commit chứa ảnh. Dùng nội dung của [RELEASE-v2027.01.md](RELEASE-v2027.01.md) và đính kèm `VietType-v2027.01-Windows-x64-Install.zip` cùng `SHA256SUMS.txt`. Ảnh trong bài release được tải từ thẻ này.
3. Sau khi workflow [Build Linux and macOS packages](.github/workflows/platform-packages.yml) có trên nhánh mặc định, vào **Actions** và chọn **Run workflow**. Tải `.deb` và `.dmg` trong **Artifacts** của lần chạy.
4. Kiểm tra cài đặt, chọn phương thức nhập và gõ Telex/VNI trên một máy Debian 12 và trên macOS trước khi đính kèm các gói thử nghiệm. Bản macOS dùng chữ ký ad hoc. Nếu phát hành rộng rãi, chuẩn bị quy trình ký Developer ID và notarization riêng. [Hướng dẫn chạy workflow thủ công của GitHub](https://docs.github.com/en/actions/how-tos/manage-workflow-runs/manually-run-a-workflow)

## Kiểm tra bản Windows trước khi đăng

Giải nén bản ZIP và mở `VietType.exe` để thử chạy trực tiếp. Trên một tài khoản Windows thử nghiệm, chạy `Install-VietType.cmd` rồi mở ứng dụng từ Start. Trình cài đặt sao chép vào `%LOCALAPPDATA%\Programs\VietType` và tạo lối tắt Start cho người dùng hiện tại. Bản ZIP có thể dùng ở chế độ portable.
