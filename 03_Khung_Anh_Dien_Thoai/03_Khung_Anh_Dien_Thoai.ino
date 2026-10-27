/*
 * =====================================================================
 * DỰ ÁN: KHUNG ẢNH THÔNG MINH - TRUYỀN ẢNH TỪ ĐIỆN THOẠI LÊN ESP32 CYD 2.8"
 * ĐỊA CHỈ: D:\ESP32\03_Khung_Anh_Dien_Thoai\03_Khung_Anh_Dien_Thoai.ino
 * =====================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>
#include "qrcode.h"
#include "web_page.h"

// --- CẤU HÌNH WIFI ---
const char* WIFI_SSID     = "EverGrowth";
const char* WIFI_PASSWORD = "Evergrowth!2025";

#define TFT_BL 21
TFT_eSPI tft = TFT_eSPI();
WebServer server(80);

File uploadFile;
bool needRedraw = false;
int currentRotation = 0; // 0: Dọc 240x320
bool isBGR = false;      // false = Chuẩn RGB, true = Đảo BGR

// Thiết lập thanh ghi MADCTL để đổi chuẩn màu RGB / BGR
void applyColorOrder() {
  tft.writecommand(TFT_MADCTL);
  // Rotation 0: MX là 0x40. Bit 3 là BGR (0x08). Nếu RGB chuẩn: bit 3 = 0.
  tft.writedata(0x40 | (isBGR ? 0x08 : 0x00));
}

// Callback cho TJpg_Decoder
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if (y >= tft.height()) return 0;
  tft.pushImage(x, y, w, h, bitmap);
  return 1;
}

// Hiển thị mã QR lên màn hình để điện thoại quét
void drawQRCode(const char* url) {
  applyColorOrder();
  tft.fillScreen(TFT_BLACK);

  // Tiêu đề
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawCentreString("KHUNG ANH CYD 2.8", 120, 16, 4);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString("Quet ma QR bang dien thoai:", 120, 50, 2);

  // Tạo mã QR
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(3)];
  qrcode_initText(&qrcode, qrcodeData, 3, 0, url);

  int scale = 4;
  int qrSize = qrcode.size * scale;
  int startX = (240 - qrSize) / 2;
  int startY = 82;

  // Viền trắng bảo vệ mã QR
  tft.fillRect(startX - 8, startY - 8, qrSize + 16, qrSize + 16, TFT_WHITE);

  for (uint8_t y = 0; y < qrcode.size; y++) {
    for (uint8_t x = 0; x < qrcode.size; x++) {
      if (qrcode_getModule(&qrcode, x, y)) {
        tft.fillRect(startX + (x * scale), startY + (y * scale), scale, scale, TFT_BLACK);
      }
    }
  }

  // Dòng hướng dẫn địa chỉ IP
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawCentreString("Hoac truy cap trinh duyet:", 120, 225, 2);

  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawCentreString(url, 120, 250, 4);

  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawCentreString("Chon anh bat ky de gui len!", 120, 290, 2);
}

// Vẽ ảnh từ bộ nhớ LittleFS
void displayPhoto() {
  if (LittleFS.exists("/photo.jpg")) {
    Serial.println("[TFT] Dang hien thi anh /photo.jpg...");
    applyColorOrder();
    tft.fillScreen(TFT_BLACK);
    TJpgDec.drawFsJpg(0, 0, "/photo.jpg", LittleFS);
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n=== KHOI DONG KHUNG ANH ESP32 CYD 2.8 ===");

  // 1. Bật đèn nền (GPIO 21)
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  // 2. Khởi tạo màn hình
  tft.init();
  tft.setRotation(currentRotation);
  tft.invertDisplay(true); // Panel CYD normally-black
  applyColorOrder();       // Áp dụng chuẩn màu RGB
  tft.fillScreen(TFT_BLACK);

  // Cấu hình bộ giải mã JPEG
  TJpgDec.setJpgScale(1);
  TJpgDec.setCallback(tft_output);
  TJpgDec.setSwapBytes(true); // Đảo byte SPI cho ESP32

  // 3. Khởi tạo LittleFS
  if (!LittleFS.begin(true)) {
    Serial.println("[FS] Khoi tao LittleFS that bai!");
  } else {
    Serial.println("[FS] LittleFS san sang.");
  }

  // 4. Kết nối WiFi
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawCentreString("DANG KET NOI WIFI...", 120, 140, 4);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int count = 0;
  while (WiFi.status() != WL_CONNECTED && count < 25) {
    delay(500);
    Serial.print(".");
    count++;
  }

  String localUrl = "http://" + WiFi.localIP().toString();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WIFI] Ket noi thanh cong! IP: " + WiFi.localIP().toString());
    MDNS.begin("cyd");

    // Nếu đã có ảnh từ trước, hiển thị ảnh. Nếu chưa có ảnh, hiện mã QR
    if (LittleFS.exists("/photo.jpg")) {
      displayPhoto();
    } else {
      drawQRCode(localUrl.c_str());
    }
  } else {
    Serial.println("\n[WIFI] Ket noi that bai!");
    tft.fillScreen(TFT_RED);
    tft.setTextColor(TFT_WHITE, TFT_RED);
    tft.drawCentreString("KET NOI WIFI THAT BAI!", 120, 150, 4);
  }

  // 5. Cấu hình Web Server
  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", INDEX_HTML);
  });

  server.on("/upload", HTTP_POST, []() {
    server.send(200, "text/plain", "OK");
    needRedraw = true;
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("[UPLOAD] Bat dau nhan anh: %s\n", upload.filename.c_str());
      uploadFile = LittleFS.open("/photo.jpg", "w");
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (uploadFile) {
        uploadFile.write(upload.buf, upload.currentSize);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (uploadFile) {
        uploadFile.close();
        Serial.printf("[UPLOAD] Hoan tat nhan anh! Tong dung luong: %u bytes\n", upload.totalSize);
      }
    }
  });

  // Đường dẫn đổi hệ màu trực tiếp
  server.on("/toggle_color", HTTP_GET, []() {
    isBGR = !isBGR;
    applyColorOrder();
    displayPhoto();
    server.send(200, "text/plain", isBGR ? "BGR (Đảo màu)" : "RGB (Chuẩn)");
    Serial.printf("[COLOR] Da chuyen he mau sang: %s\n", isBGR ? "BGR" : "RGB");
  });

  // Đường dẫn xoá ảnh và quay lại màn hình QR
  server.on("/reset", HTTP_GET, []() {
    if (LittleFS.exists("/photo.jpg")) {
      LittleFS.remove("/photo.jpg");
    }
    String url = "http://" + WiFi.localIP().toString();
    drawQRCode(url.c_str());
    server.send(200, "text/plain", "Reset OK");
  });

  server.begin();
  Serial.println("[SERVER] Web Server da khoi chay tai cong 80!");
}

void loop() {
  server.handleClient();

  if (needRedraw) {
    needRedraw = false;
    displayPhoto();
  }

  delay(2);
}
