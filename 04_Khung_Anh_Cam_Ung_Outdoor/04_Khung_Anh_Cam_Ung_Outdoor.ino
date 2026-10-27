/*
 * =====================================================================
 * DỰ ÁN: KHUNG ẢNH THÔNG MINH OUTDOOR (CHẠM CẢM ỨNG + PHÁT WIFI RIÊNG)
 * ĐỊA CHỈ: D:\ESP32\04_Khung_Anh_Cam_Ung_Outdoor\04_Khung_Anh_Cam_Ung_Outdoor.ino
 * =====================================================================
 * Tính năng đặc biệt:
 * 1. Chạy hoàn toàn độc lập ngoài đường bằng pin sạc dự phòng USB, KHÔNG CẦN ROUTER HAY INTERNET.
 * 2. Tự phát Wi-Fi riêng: "KhungAnh-ESP32" (không mật khẩu, kết nối tức thì).
 * 3. Chạm cảm ứng vào màn hình để chuyển đổi giữa:
 *    - Chế độ xem ảnh toàn màn hình (Full Screen Photo).
 *    - Chế độ hiển thị 2 Mã QR hướng dẫn (QR1: Vào WiFi, QR2: Mở Web gửi ảnh).
 * 4. Hỗ trợ Captive Portal (tự động bật trang gửi ảnh khi vừa kết nối WiFi).
 * 5. Web trên điện thoại hỗ trợ:
 *    - Chạm và kéo ảnh để căn chỉnh vị trí (Pan).
 *    - Thanh trượt thu phóng (Zoom 1.0x - 3.0x).
 *    - Xoay 90 độ, đổi chế độ Lấp đầy / Vừa vặn.
 *    - Nút đổi hệ màu RGB/BGR trực tiếp.
 *    - Nén và thu nhỏ ngay trên điện thoại gửi qua trong 0.5s.
 * =====================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <LittleFS.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>
#include <XPT2046_Touchscreen.h>
#include "qrcode.h"
#include "web_page.h"

// --- CẤU HÌNH WIFI TỰ PHÁT (ACCESS POINT) ---
const char* AP_SSID = "KhungAnh-ESP32";
const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);

// WiFi nhà dự phòng (nếu có thì kết nối song song)
const char* STA_SSID = "EverGrowth";
const char* STA_PASS = "Evergrowth!2025";

// --- CHÂN CẢM ỨNG CYD (XPT2046 TRÊN VSPI) ---
#define XPT2046_IRQ  36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK  25
#define XPT2046_CS   33

SPIClass touchSpi = SPIClass(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);

#define TFT_BL 21
TFT_eSPI tft = TFT_eSPI();
WebServer server(80);
DNSServer dnsServer;

File uploadFile;
bool needRedraw = false;
bool isBGR = false; // false = RGB chuẩn
enum DisplayState { STATE_PHOTO, STATE_QR };
DisplayState currentState = STATE_QR;
unsigned long lastTouchTime = 0;

// Thiết lập thanh ghi MADCTL để đổi chuẩn màu RGB / BGR
void applyColorOrder() {
  tft.writecommand(TFT_MADCTL);
  tft.writedata(0x40 | (isBGR ? 0x08 : 0x00));
}

// Callback cho TJpg_Decoder
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if (y >= tft.height()) return 0;
  tft.pushImage(x, y, w, h, bitmap);
  return 1;
}

// Vẽ một mã QR tại toạ độ (startX, startY) với kích thước module scale
void renderQR(const char* data, int startX, int startY, int scale) {
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(3)];
  qrcode_initText(&qrcode, qrcodeData, 3, 0, data);

  int qrSize = qrcode.size * scale;
  // Quiet zone trắng xung quanh
  tft.fillRect(startX - 4, startY - 4, qrSize + 8, qrSize + 8, TFT_WHITE);

  for (uint8_t y = 0; y < qrcode.size; y++) {
    for (uint8_t x = 0; x < qrcode.size; x++) {
      if (qrcode_getModule(&qrcode, x, y)) {
        tft.fillRect(startX + (x * scale), startY + (y * scale), scale, scale, TFT_BLACK);
      }
    }
  }
}

// Hiển thị màn hình 2 Mã QR hướng dẫn
void drawTwoQRScreen() {
  applyColorOrder();
  tft.fillScreen(0x0841); // Nền xanh đen đậm

  // 1. THANH TIÊU ĐỀ TRÊN CÙNG
  tft.fillRect(0, 0, 240, 28, 0x10A2);
  tft.setTextColor(TFT_YELLOW, 0x10A2);
  tft.drawCentreString("KET NOI GUI ANH", 120, 6, 2);

  // 2. KHU VỰC BƯỚC 1: KẾT NỐI WIFI TỰ PHÁT (Y: 34 -> 152)
  tft.fillRoundRect(6, 34, 228, 118, 6, 0x10A2);
  tft.drawRoundRect(6, 34, 228, 118, 6, TFT_CYAN);

  // Vẽ QR 1: Cấu hình WiFi tự động (WIFI:T:nopass;S:KhungAnh-ESP32;;)
  renderQR("WIFI:T:nopass;S:KhungAnh-ESP32;;", 16, 44, 3); // 29*3 = 87px

  // Chữ hướng dẫn bên phải QR 1
  tft.setTextColor(TFT_CYAN, 0x10A2);
  tft.drawString("BUOC 1:", 118, 44, 2);
  tft.setTextColor(TFT_WHITE, 0x10A2);
  tft.drawString("Quet ma de", 118, 64, 2);
  tft.drawString("vao Wi-Fi:", 118, 80, 2);
  tft.setTextColor(TFT_GREEN, 0x10A2);
  tft.drawString("KhungAnh", 118, 100, 2);
  tft.drawString("-ESP32", 118, 116, 2);
  tft.setTextColor(TFT_LIGHTGREY, 0x10A2);
  tft.drawString("(Ko mat khau)", 118, 134, 1);

  // 3. KHU VỰC BƯỚC 2: MỞ TRANG WEB TẢI ẢNH (Y: 158 -> 276)
  tft.fillRoundRect(6, 158, 228, 118, 6, 0x10A2);
  tft.drawRoundRect(6, 158, 228, 118, 6, 0xFD20); // Màu cam

  // Vẽ QR 2: Đường link web http://192.168.4.1
  renderQR("http://192.168.4.1", 16, 168, 3);

  // Chữ hướng dẫn bên phải QR 2
  tft.setTextColor(0xFD20, 0x10A2);
  tft.drawString("BUOC 2:", 118, 168, 2);
  tft.setTextColor(TFT_WHITE, 0x10A2);
  tft.drawString("Quet ma de", 118, 188, 2);
  tft.drawString("mo trang web:", 118, 204, 2);
  tft.setTextColor(TFT_YELLOW, 0x10A2);
  tft.drawString("192.168.4.1", 118, 224, 2);
  tft.setTextColor(TFT_LIGHTGREY, 0x10A2);
  tft.drawString("Chon & Gui anh", 118, 246, 2);

  // 4. THANH CHẠM ĐỂ XEM ẢNH DƯỚI CÙNG (Y: 284 -> 314)
  tft.fillRoundRect(6, 284, 228, 30, 4, 0x0540); // Xanh lá đậm
  tft.drawRoundRect(6, 284, 228, 30, 4, TFT_GREEN);
  tft.setTextColor(TFT_WHITE, 0x0540);
  tft.drawCentreString("[ CHAM DE VE XEM ANH ]", 120, 291, 2);

  currentState = STATE_QR;
}

// Hiển thị ảnh toàn màn hình từ LittleFS
void displayPhoto() {
  if (LittleFS.exists("/photo.jpg")) {
    Serial.println("[TFT] Dang hien thi anh /photo.jpg...");
    applyColorOrder();
    tft.fillScreen(TFT_BLACK);
    TJpgDec.drawFsJpg(0, 0, "/photo.jpg", LittleFS);
    currentState = STATE_PHOTO;
  } else {
    drawTwoQRScreen();
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n=== KHOI DONG KHUNG ANH CAM UNG OUTDOOR CYD ===");

  // 1. Đèn nền (GPIO 21)
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  // 2. Màn hình
  tft.init();
  tft.setRotation(0);
  tft.invertDisplay(true);
  applyColorOrder();
  tft.fillScreen(TFT_BLACK);

  // Bộ giải mã JPEG
  TJpgDec.setJpgScale(1);
  TJpgDec.setCallback(tft_output);
  TJpgDec.setSwapBytes(true);

  // 3. Khởi tạo Cảm ứng (Touchscreen XPT2046)
  touchSpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin(touchSpi);
  ts.setRotation(0);
  Serial.println("[TOUCH] Cam ung XPT2046 da san sang!");

  // 4. Khởi tạo LittleFS
  if (!LittleFS.begin(true)) {
    Serial.println("[FS] Khoi tao LittleFS that bai!");
  } else {
    Serial.println("[FS] LittleFS san sang.");
  }

  // 5. Khởi tạo Wi-Fi đa năng (AP + STA song song)
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawCentreString("KHOI DONG MANG RIENG...", 120, 140, 2);

  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(AP_SSID); // Mạng riêng không mật khẩu
  Serial.printf("[WIFI AP] Da phat SSID: %s (IP: 192.168.4.1)\n", AP_SSID);

  // Thử kết nối thêm WiFi nhà (nếu có trong phạm vi)
  WiFi.begin(STA_SSID, STA_PASS);
  int c = 0;
  while (WiFi.status() != WL_CONNECTED && c < 6) {
    delay(400);
    c++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[WIFI STA] Ket noi WiFi nha OK: %s\n", WiFi.localIP().toString().c_str());
  }

  // Khởi động DNS Captive Portal cho AP
  dnsServer.start(DNS_PORT, "*", apIP);
  Serial.println("[DNS] Captive Portal da bat.");

  // Nếu có ảnh, hiển thị ảnh. Nếu chưa có ảnh, hiện màn hình 2 QR
  if (LittleFS.exists("/photo.jpg")) {
    displayPhoto();
  } else {
    drawTwoQRScreen();
  }

  // 6. Cấu hình Web Server
  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", INDEX_HTML);
  });

  // Hỗ trợ tự động mở trang web khi vừa kết nối (Captive Portal cho iOS/Android)
  server.on("/hotspot-detect.html", HTTP_GET, []() { server.send_P(200, "text/html", INDEX_HTML); });
  server.on("/generate_204", HTTP_GET, []() { server.send_P(200, "text/html", INDEX_HTML); });
  server.on("/connecttest.txt", HTTP_GET, []() { server.send_P(200, "text/html", INDEX_HTML); });

  server.on("/upload", HTTP_POST, []() {
    server.send(200, "text/plain", "OK");
    needRedraw = true;
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("[UPLOAD] Nhan anh: %s\n", upload.filename.c_str());
      uploadFile = LittleFS.open("/photo.jpg", "w");
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (uploadFile) uploadFile.write(upload.buf, upload.currentSize);
    } else if (upload.status == UPLOAD_FILE_END) {
      if (uploadFile) {
        uploadFile.close();
        Serial.printf("[UPLOAD] Xong! %u bytes\n", upload.totalSize);
      }
    }
  });

  server.on("/toggle_color", HTTP_GET, []() {
    isBGR = !isBGR;
    applyColorOrder();
    displayPhoto();
    server.send(200, "text/plain", isBGR ? "BGR (Đảo màu)" : "RGB (Chuẩn)");
  });

  server.on("/reset", HTTP_GET, []() {
    if (LittleFS.exists("/photo.jpg")) LittleFS.remove("/photo.jpg");
    drawTwoQRScreen();
    server.send(200, "text/plain", "Reset OK");
  });

  server.begin();
  Serial.println("[SERVER] Web Server san sang!");
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();

  // Kiểm tra thao tác chạm cảm ứng (Touch)
  if (ts.touched()) {
    unsigned long now = millis();
    if (now - lastTouchTime > 400) { // Chống dội chạm (debounce 400ms)
      lastTouchTime = now;
      TS_Point p = ts.getPoint();
      Serial.printf("[TOUCH] Cham man hinh tai (z=%d)\n", p.z);

      // Chuyển đổi qua lại giữa Màn hình ảnh và Màn hình 2 Mã QR
      if (currentState == STATE_PHOTO) {
        Serial.println("[UI] Chuyen sang Man hinh 2 Ma QR...");
        drawTwoQRScreen();
      } else {
        Serial.println("[UI] Chuyen sang Xem anh...");
        displayPhoto();
      }
    }
  }

  // Vẽ lại ảnh ngay sau khi gửi xong từ điện thoại
  if (needRedraw) {
    needRedraw = false;
    displayPhoto();
  }

  delay(2);
}
