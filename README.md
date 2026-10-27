# ESP32-2432S028 (CYD) Weather Station & Display Toolkit

Bộ công cụ và dự án hoàn chỉnh cho mạch **ESP32-2432S028** (thường được cộng đồng gọi là **CYD - Cheap Yellow Display**, màn hình màu cảm ứng 2.8 inch 240x320 SPI TFT).

Dự án bao gồm:
1. **Bài 1 - Test Màn Hình Cơ Bản (`01_Test_Man_Hinh`):** Kiểm tra phần cứng màn hình, màu sắc chuẩn RGB (khử lỗi âm bản), kiểm tra đèn nền và bộ đếm thời gian.
2. **Bài 2 - Trạm Thời Tiết WiFi Thực Tế (`02_Tram_Thoi_Tiet_WiFi`):**
   * Tự động reset và kết nối WiFi.
   * Tự động định vị địa điểm thực tế qua IP (`ip-api.com`).
   * Đồng bộ thời gian thực Internet NTP (GMT+7).
   * Cập nhật thời tiết trực tiếp: Nhiệt độ (°C), Độ ẩm (%), Lưu lượng mưa (mm) từ Open-Meteo API (không cần API Key).
   * Giao diện Dashboard hiện đại, mượt mà, không nhấp nháy màn hình.
3. **Bài 3 - Khung Ảnh Thông Minh Từ Điện Thoại (`03_Khung_Anh_Dien_Thoai`):**
   * Tự động tạo Web Server và hiển thị mã QR trên màn hình CYD.
   * Dùng Camera điện thoại quét mã QR để mở giao diện Web điều khiển.
   * Hỗ trợ chọn ảnh từ thư viện hoặc chụp ảnh trực tiếp.
   * Xử lý cắt/chỉnh/xoay (Crop/Fit) 240x320 trực tiếp trên trình duyệt điện thoại và truyền qua WiFi trong ~0.5 giây.
   * Tự động lưu ảnh vào bộ nhớ Flash LittleFS (không bị mất khi tắt nguồn).
4. **Bài 4 - Khung Ảnh Outdoor Cảm Ứng Độc Lập (`04_Khung_Anh_Cam_Ung_Outdoor`):**
   * Hoạt động 100% ngoài trời chỉ với pin dự phòng, không cần Internet hay Router WiFi.
   * Chạm cảm ứng vào màn hình để lật qua lại giữa ảnh và màn hình 2 Mã QR.
   * Hiển thị **2 Mã QR riêng biệt**:
     * QR 1: Quét để tự động kết nối mạng WiFi do ESP32 phát (`KhungAnh-ESP32`).
     * QR 2: Quét để mở trang web gửi ảnh (`http://192.168.4.1`).
   * Giao diện điện thoại cao cấp: Kéo rê ảnh (Pan), thanh trượt thu phóng (Zoom), xoay, đổi chuẩn màu RGB/BGR trực tiếp.
5. **Bài 5 - Khung Ảnh & Màn Hình Chờ Điện Thoại AOD (`05_Khung_Anh_AOD_Standby`):**
   * **Cơ chế Smartphone:** Sau 30 giây (tuỳ chỉnh 10s, 15s, 30s, 60s, Never) không chạm màn hình -> ESP32 tự động chuyển sang Màn hình chờ (Standby / AOD).
   * **Đánh thức tức thì:** Chạm tay vào màn hình bất cứ lúc nào -> Đèn nền bật sáng và quay lại hiển thị ảnh chính ngay lập tức.
   * **Tự do Set up kiểu màn hình chờ trước:**
     * *Kiểu 1:* Đồng hồ số Always-On Display (AOD) nền đen sang trọng, tự đồng bộ giờ điện thoại qua Web / NTP GMT+7.
     * *Kiểu 2:* Ảnh chờ riêng biệt (tải ảnh screensaver phụ độc lập với ảnh chính).
     * *Kiểu 3:* Chế độ ngủ tắt màn hình (Deep Sleep Backlight Off), tiết kiệm pin tối đa.
   * Cấu hình và ảnh lưu bền vững trong LittleFS Flash.
6. **Cấu hình phần cứng chuẩn (`hardware_config/User_Setup.h`):** Khắc phục lỗi hiển thị đảo màu trên thư viện `TFT_eSPI`.
7. **Bộ nạp tự động thông minh (`tools/flash_tool.py`):** Lắng nghe và nạp firmware tức thì khi bấm tổ hợp nút bootloader.
8. **Skill Tái Sử Dụng (`skills/esp32-cyd-weather-station/SKILL.md`):** Tài liệu chuẩn hoá quy trình cho AI và lập trình viên.

---

## 📌 Sơ Đồ Chân Phần Cứng (CYD 2.8" SPI TFT)

| Chân Chức Năng | Chân ESP32 GPIO | Mô Tả |
| :--- | :--- | :--- |
| `TFT_MOSI` | `GPIO 13` | SPI MOSI (Dữ liệu truyền tới màn hình) |
| `TFT_MISO` | `GPIO 12` | SPI MISO |
| `TFT_SCLK` | `GPIO 14` | SPI Clock |
| `TFT_CS`   | `GPIO 15` | Chip Select (Chọn chip màn hình) |
| `TFT_DC`   | `GPIO 2`  | Data / Command Control |
| `TFT_RST`  | `-1`      | Nối chung với chân Reset của ESP32 |
| `TFT_BL`   | `GPIO 21` | Điều khiển đèn nền (Backlight, `HIGH = Bật`) |

---

## ⚠️ Các Lỗi Phần Cứng Thường Gặp & Cách Khắc Phục Triệt Để

### 1. Lỗi Màn Hình Bị Đảo Màu (Màu Âm Bản / Negative)
* **Hiện tượng:** Màu đỏ bị thành xanh da trời, bầu trời xanh thành màu vàng/cam, ảnh như phim âm bản.
* **Nguyên nhân:** Tấm nền TFT 2.8" trên CYD là loại **Normally-Black**, cần thanh ghi đảo màu `INVON (0x21)`.
* **Cách khắc phục:**
  * Trong code khởi tạo: Thêm lệnh `tft.invertDisplay(true);` ngay sau `tft.init();`.
  * Trong tệp `User_Setup.h`: Đảm bảo bật `#define TFT_INVERSION_ON` và chọn driver `#define ILI9341_2_DRIVER`.

### 2. Lỗi Không Nạp Được Code (`Wrong boot mode detected 0x13`)
* **Hiện tượng:** Arduino IDE báo lỗi kết nối `Failed to connect to ESP32: Wrong boot mode detected (0x13)`.
* **Nguyên nhân:** Mạch CYD dùng chip nạp CH340C không có mạch tự động kéo GPIO0 xuống mức LOW khi đang chạy firmware.
* **Cách khắc phục (Tổ hợp 3 bước):**
  1. Nhấn và **GIỮ** nút `BOOT` (ở cạnh dưới màn hình).
  2. Bấm rồi **NHẢ** nút `RST` (ở cạnh trên màn hình) 1 cái.
  3. **THẢ** nút `BOOT` ra.
  * Mạch sẽ vào trạng thái `boot:0x3 (DOWNLOAD_BOOT)` sẵn sàng nhận code 100%.

### 3. Lỗi `Missing FQBN` Trong Arduino IDE
* **Cách khắc phục:** Mỗi thư mục dự án đã có sẵn tệp `sketch.yaml` định cấu hình sẵn cho bo mạch `ESP32 Dev Module` với tốc độ nạp `921600` và phân vùng `Huge APP` (3MB cho code).

### 4. Giới Hạn Băng Tần WiFi
* Mạch ESP32 chỉ hỗ trợ băng tần **WiFi 2.4 GHz**, không hỗ trợ băng tần 5 GHz. Do đó hãy luôn kết nối vào SSID băng tần 2.4 GHz.

---

## 🚀 Hướng Dẫn Sử Dụng

### Cách 1: Sử dụng Arduino IDE (Khuyên Dùng Cho Người Mới)
1. Cài đặt thư viện: Mở Arduino IDE -> Library Manager -> Cài đặt `TFT_eSPI` (Bodmer) và `ArduinoJson` (Benoit Blanchon).
2. Copy tệp [hardware_config/User_Setup.h](file:///hardware_config/User_Setup.h) ghi đè vào thư mục `Arduino/libraries/TFT_eSPI/User_Setup.h`.
3. Mở dự án mong muốn:
   * [01_Test_Man_Hinh/01_Test_Man_Hinh.ino](file:///01_Test_Man_Hinh/01_Test_Man_Hinh.ino) để kiểm tra phần cứng.
   * [02_Tram_Thoi_Tiet_WiFi/02_Tram_Thoi_Tiet_WiFi.ino](file:///02_Tram_Thoi_Tiet_WiFi/02_Tram_Thoi_Tiet_WiFi.ino) để chạy trạm thời tiết.
4. Bấm **Upload**. Khi màn hình console xuất hiện dòng kết nối `Connecting...`: Thực hiện thao tác 3 bước nút bấm để nạp.

### Cách 2: Sử Dụng Công Cụ Nạp Tự Động (`tools/flash_tool.py`)
Chạy công cụ lắng nghe tự động:
```powershell
python tools/flash_tool.py
```
Sau đó bấm tổ hợp nút bấm trên mạch, code sẽ tự động được nạp trong vòng 10 giây!

---

## 📄 Bản Quyền & Đóng Góp
Dự án được cấu trúc và kiểm thử thực tế trên mạch phần cứng ESP32-2432S028. Hoàn toàn mã nguồn mở cho cộng đồng DIY Maker.
