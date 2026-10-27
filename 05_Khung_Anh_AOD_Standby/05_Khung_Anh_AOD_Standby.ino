/*
 * =====================================================================
 * DỰ ÁN: KHUNG ẢNH THÔNG MINH CYD VỚI CHẾ ĐỘ MÀN HÌNH CHỜ (AOD STANDBY)
 * ĐỊA CHỈ: D:\ESP32\05_Khung_Anh_AOD_Standby\05_Khung_Anh_AOD_Standby.ino
 * =====================================================================
 * Tính năng chính:
 * 1. Chế độ giống điện thoại thông minh:
 *    - Không chạm sau 30s (tuỳ chỉnh 10s, 15s, 30s, 60s, Never) -> Tự động vào màn hình chờ.
 *    - Chạm vào màn hình bất kỳ đâu -> Thức giấc tức thì, bật đèn nền và hiện lại ảnh chính!
 * 2. Cài đặt trước (Setup trước) màn hình chờ:
 *    - Kiểu 1: Đồng hồ số AOD (Always-On Display) sang trọng, tự đồng bộ giờ điện thoại.
 *    - Kiểu 2: Ảnh chờ phụ riêng biệt (Screensaver photo).
 *    - Kiểu 3: Tắt hẳn màn hình (Sleep Mode) siêu tiết kiệm pin, chạm là sáng.
 *    - Cấu hình lưu bền vững trong LittleFS (/config.json).
 * 3. Chạy độc lập ngoài đường bằng pin dự phòng USB (tự phát WiFi "KhungAnh-ESP32").
 * 4. Hỗ trợ quét mã QR, xoay ảnh, thu phóng, kéo vị trí ảnh mượt mà trên điện thoại.
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
#include <ArduinoJson.h>
#include <time.h>
#include <sys/time.h>
#include "qrcode.h"
#include "web_page.h"

// --- CẤU HÌNH WIFI TỰ PHÁT (ACCESS POINT) ---
const char* AP_SSID = "KhungAnh-ESP32";
const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);

// WiFi nhà kết nối song song (nếu có sẵn)
const char* STA_SSID = "EverGrowth";
const char* STA_PASS = "Evergrowth!2025";

// --- CHÂN CẢM ỨNG XPT2046 (CYD TRÊN VSPI) ---
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
bool needRedrawMain = false;
bool needRedrawStandby = false;
bool isBGR = false;
bool backlightOn = true;

// Cấu hình màn hình chờ
int cfgTimeout = 30;         // Giây chờ (10, 15, 30, 60, 0 = tắt)
String cfgMode = "clock";    // "clock", "photo", "sleep"

enum DisplayState { STATE_MAIN_PHOTO, STATE_STANDBY, STATE_QR };
DisplayState currentState = STATE_MAIN_PHOTO;

unsigned long lastTouchTime = 0;
unsigned long lastActivityTime = 0;
unsigned long lastClockDraw = 0;
int lastDrawnSecond = -1;

const char* wdays[] = {"CN", "T.Hai", "T.Ba", "T.Tu", "T.Nam", "T.Sau", "T.Bay"};

// Thiết lập thanh ghi MADCTL đảo màu RGB / BGR
void applyColorOrder() {
  tft.writecommand(TFT_MADCTL);
  tft.writedata(0x40 | (isBGR ? 0x08 : 0x00));
}

void setBacklight(bool on) {
  backlightOn = on;
  digitalWrite(TFT_BL, on ? HIGH : LOW);
}

// Callback cho TJpg_Decoder
bool tft_output(int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
  if (y >= tft.height()) return 0;
  tft.pushImage(x, y, w, h, bitmap);
  return 1;
}

// Đọc cấu hình từ LittleFS
void loadConfig() {
  if (LittleFS.exists("/config.json")) {
    File f = LittleFS.open("/config.json", "r");
    if (f) {
      JsonDocument doc;
      DeserializationError err = deserializeJson(doc, f);
      if (!err) {
        if (doc["timeout"].is<int>()) cfgTimeout = doc["timeout"].as<int>();
        if (doc["mode"].is<const char*>()) cfgMode = doc["mode"].as<String>();
        if (doc["is_bgr"].is<bool>()) isBGR = doc["is_bgr"].as<bool>();
        Serial.printf("[CONFIG] Da doc: Timeout=%ds, Mode=%s, BGR=%d\n", cfgTimeout, cfgMode.c_str(), isBGR);
      }
      f.close();
    }
  } else {
    Serial.println("[CONFIG] Dung mac dinh: 30s, clock, RGB");
  }
}

// Lưu cấu hình vào LittleFS
void saveConfig() {
  JsonDocument doc;
  doc["timeout"] = cfgTimeout;
  doc["mode"] = cfgMode;
  doc["is_bgr"] = isBGR;
  File f = LittleFS.open("/config.json", "w");
  if (f) {
    serializeJson(doc, f);
    f.close();
    Serial.println("[CONFIG] Da luu config.json thanh cong!");
  }
}

// Vẽ mã QR tại toạ độ
void renderQR(const char* data, int startX, int startY, int scale) {
  QRCode qrcode;
  uint8_t qrcodeData[qrcode_getBufferSize(3)];
  qrcode_initText(&qrcode, qrcodeData, 3, 0, data);

  int qrSize = qrcode.size * scale;
  tft.fillRect(startX - 4, startY - 4, qrSize + 8, qrSize + 8, TFT_WHITE);

  for (uint8_t y = 0; y < qrcode.size; y++) {
    for (uint8_t x = 0; x < qrcode.size; x++) {
      if (qrcode_getModule(&qrcode, x, y)) {
        tft.fillRect(startX + (x * scale), startY + (y * scale), scale, scale, TFT_BLACK);
      }
    }
  }
}

// Màn hình 2 Mã QR hướng dẫn kết nối
void drawTwoQRScreen() {
  setBacklight(true);
  applyColorOrder();
  tft.fillScreen(0x0841); // Nền xanh đen đậm

  // Tiêu đề
  tft.fillRect(0, 0, 240, 28, 0x10A2);
  tft.setTextColor(TFT_YELLOW, 0x10A2);
  tft.drawCentreString("KET NOI VA CAI DAT", 120, 6, 2);

  // Khối Bước 1: WiFi riêng
  tft.fillRoundRect(6, 34, 228, 118, 6, 0x10A2);
  tft.drawRoundRect(6, 34, 228, 118, 6, TFT_CYAN);
  renderQR("WIFI:T:nopass;S:KhungAnh-ESP32;;", 16, 44, 3);

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

  // Khối Bước 2: Mở Web
  tft.fillRoundRect(6, 158, 228, 118, 6, 0x10A2);
  tft.drawRoundRect(6, 158, 228, 118, 6, 0xFD20); // Cam
  renderQR("http://192.168.4.1", 16, 168, 3);

  tft.setTextColor(0xFD20, 0x10A2);
  tft.drawString("BUOC 2:", 118, 168, 2);
  tft.setTextColor(TFT_WHITE, 0x10A2);
  tft.drawString("Quet ma mo", 118, 188, 2);
  tft.drawString("trang web:", 118, 204, 2);
  tft.setTextColor(TFT_YELLOW, 0x10A2);
  tft.drawString("192.168.4.1", 118, 224, 2);
  tft.setTextColor(TFT_LIGHTGREY, 0x10A2);
  tft.drawString("Gui anh & Cai gio", 118, 246, 2);

  // Nút chạm xem ảnh
  tft.fillRoundRect(6, 284, 228, 30, 4, 0x0540);
  tft.drawRoundRect(6, 284, 228, 30, 4, TFT_GREEN);
  tft.setTextColor(TFT_WHITE, 0x0540);
  tft.drawCentreString("[ CHAM DE VE XEM ANH ]", 120, 291, 2);

  currentState = STATE_QR;
}

// Hiển thị ảnh chính
void displayMainPhoto() {
  setBacklight(true);
  if (LittleFS.exists("/photo.jpg")) {
    Serial.println("[TFT] Hien thi anh chinh /photo.jpg");
    applyColorOrder();
    tft.fillScreen(TFT_BLACK);
    TJpgDec.drawFsJpg(0, 0, "/photo.jpg", LittleFS);
    currentState = STATE_MAIN_PHOTO;
  } else {
    drawTwoQRScreen();
  }
}

// Vẽ đồng hồ số AOD
void drawStandbyClock(bool fullRedraw) {
  time_t now = time(nullptr);
  struct tm* t = localtime(&now);

  int sec = t->tm_sec;
  if (!fullRedraw && sec == lastDrawnSecond) return;
  lastDrawnSecond = sec;

  if (fullRedraw) {
    applyColorOrder();
    tft.fillScreen(TFT_BLACK);

    // Tiêu đề nhỏ trên cùng
    tft.setTextColor(0x52AA, TFT_BLACK); // Xám xanh mờ
    tft.drawCentreString("- MAN HINH CHO (AOD) -", 120, 16, 2);
    tft.drawFastHLine(30, 38, 180, 0x2104);

    // Khung nút chạm ở đáy
    tft.drawRoundRect(20, 275, 200, 32, 6, 0x0320);
    tft.setTextColor(0x2408, TFT_BLACK);
    tft.drawCentreString("[ CHAM DE MO ANH ]", 120, 283, 2);
  }

  // 1. Giờ & Phút (Font 7 cực lớn)
  char timeBuf[12];
  snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", t->tm_hour, t->tm_min);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString(timeBuf, 120, 65, 7);

  // 2. Giây (Font 4)
  char secBuf[8];
  snprintf(secBuf, sizeof(secBuf), ":%02d", t->tm_sec);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawCentreString(secBuf, 120, 135, 4);

  // 3. Thứ, Ngày / Tháng / Năm
  char dateBuf[32];
  int wdayIdx = t->tm_wday % 7;
  int year = t->tm_year + 1900;
  snprintf(dateBuf, sizeof(dateBuf), "%s, %02d/%02d/%04d", wdays[wdayIdx], t->tm_mday, t->tm_mon + 1, year);
  tft.setTextColor(0xFD20, TFT_BLACK); // Màu cam vàng
  tft.drawCentreString(dateBuf, 120, 180, 4);

  // 4. Trạng thái mạng / IP
  tft.setTextColor(0x7BEF, TFT_BLACK);
  tft.drawCentreString("AP: KhungAnh-ESP32 (192.168.4.1)", 120, 225, 2);
}

// Bắt đầu chế độ màn hình chờ
void enterStandby() {
  Serial.printf("[STANDBY] Chuyen sang Standby: %s\n", cfgMode.c_str());
  currentState = STATE_STANDBY;

  if (cfgMode == "sleep") {
    setBacklight(false);
  } else if (cfgMode == "photo") {
    setBacklight(true);
    if (LittleFS.exists("/standby.jpg")) {
      applyColorOrder();
      tft.fillScreen(TFT_BLACK);
      TJpgDec.drawFsJpg(0, 0, "/standby.jpg", LittleFS);
    } else {
      // Nếu chưa có ảnh chờ phụ -> Hiện đồng hồ AOD
      drawStandbyClock(true);
    }
  } else {
    // Chế độ "clock" (Mặc định)
    setBacklight(true);
    drawStandbyClock(true);
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n=== KHOI DONG KHUNG ANH CAM UNG AOD STANDBY CYD ===");

  // 1. Đèn nền (GPIO 21)
  pinMode(TFT_BL, OUTPUT);
  setBacklight(true);

  // 2. Màn hình TFT
  tft.init();
  tft.setRotation(0);
  tft.invertDisplay(true);
  applyColorOrder();
  tft.fillScreen(TFT_BLACK);

  // Bộ giải mã JPEG
  TJpgDec.setJpgScale(1);
  TJpgDec.setCallback(tft_output);
  TJpgDec.setSwapBytes(true);

  // 3. Khởi tạo Cảm ứng XPT2046
  touchSpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin(touchSpi);
  ts.setRotation(0);
  Serial.println("[TOUCH] Cam ung XPT2046 da san sang!");

  // 4. Khởi tạo LittleFS
  if (LittleFS.begin(true)) {
    Serial.println("[FS] LittleFS san sang.");
    loadConfig();
  }

  // 5. Cấu hình giờ mặc định (GMT+7)
  configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  // 6. Phát Wi-Fi Access Point độc lập
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(AP_SSID);
  Serial.printf("[WIFI AP] Da phat SSID: %s (IP: 192.168.4.1)\n", AP_SSID);

  // Thử kết nối WiFi nhà nếu có
  WiFi.begin(STA_SSID, STA_PASS);
  int c = 0;
  while (WiFi.status() != WL_CONNECTED && c < 6) {
    delay(300);
    c++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[WIFI STA] Ket noi WiFi nha OK: %s\n", WiFi.localIP().toString().c_str());
  }

  // DNS Captive Portal
  dnsServer.start(DNS_PORT, "*", apIP);

  // Hiển thị giao diện khởi động ban đầu
  if (LittleFS.exists("/photo.jpg")) {
    displayMainPhoto();
  } else {
    drawTwoQRScreen();
  }
  lastActivityTime = millis();

  // 7. CẤU HÌNH WEB SERVER
  server.on("/", HTTP_GET, []() {
    server.send_P(200, "text/html", INDEX_HTML);
  });

  server.on("/hotspot-detect.html", HTTP_GET, []() { server.send_P(200, "text/html", INDEX_HTML); });
  server.on("/generate_204", HTTP_GET, []() { server.send_P(200, "text/html", INDEX_HTML); });
  server.on("/connecttest.txt", HTTP_GET, []() { server.send_P(200, "text/html", INDEX_HTML); });

  // Lấy cấu hình hiện tại
  server.on("/get_config", HTTP_GET, []() {
    String json = "{\"timeout\":" + String(cfgTimeout) + ",\"mode\":\"" + cfgMode + "\",\"is_bgr\":" + (isBGR ? "true" : "false") + "}";
    server.send(200, "application/json", json);
  });

  // Lưu cấu hình chờ
  server.on("/save_config", HTTP_GET, []() {
    if (server.hasArg("timeout")) cfgTimeout = server.arg("timeout").toInt();
    if (server.hasArg("mode")) cfgMode = server.arg("mode");
    saveConfig();
    lastActivityTime = millis();
    server.send(200, "text/plain", "Da luu cau hinh thanh cong!");
  });

  // Đồng bộ thời gian từ điện thoại
  server.on("/sync_time", HTTP_GET, []() {
    if (server.hasArg("epoch")) {
      time_t epoch = (time_t)server.arg("epoch").toInt();
      // Bù GMT+7 nếu epoch là UTC từ trình duyệt JS Date.now()/1000
      struct timeval tv;
      tv.tv_sec = epoch + (7 * 3600);
      tv.tv_usec = 0;
      settimeofday(&tv, NULL);
      Serial.printf("[TIME] Dong bo thoi gian tu dien thoai: %ld\n", epoch);
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "Thieu epoch");
    }
  });

  // Đổi hệ màu RGB / BGR
  server.on("/toggle_color", HTTP_GET, []() {
    isBGR = !isBGR;
    applyColorOrder();
    saveConfig();
    if (currentState == STATE_MAIN_PHOTO) displayMainPhoto();
    else if (currentState == STATE_STANDBY && cfgMode == "photo") enterStandby();
    server.send(200, "text/plain", isBGR ? "BGR (Đảo màu)" : "RGB (Chuẩn)");
  });

  // Upload Ảnh chính
  server.on("/upload", HTTP_POST, []() {
    server.send(200, "text/plain", "OK");
    needRedrawMain = true;
    lastActivityTime = millis();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("[UPLOAD] Nhan anh chinh: %s\n", upload.filename.c_str());
      uploadFile = LittleFS.open("/photo.jpg", "w");
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (uploadFile) uploadFile.write(upload.buf, upload.currentSize);
    } else if (upload.status == UPLOAD_FILE_END) {
      if (uploadFile) {
        uploadFile.close();
        Serial.printf("[UPLOAD] Xong anh chinh! %u bytes\n", upload.totalSize);
      }
    }
  });

  // Upload Ảnh chờ phụ
  server.on("/upload_standby", HTTP_POST, []() {
    server.send(200, "text/plain", "OK");
    needRedrawStandby = true;
    lastActivityTime = millis();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("[UPLOAD] Nhan anh cho: %s\n", upload.filename.c_str());
      uploadFile = LittleFS.open("/standby.jpg", "w");
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (uploadFile) uploadFile.write(upload.buf, upload.currentSize);
    } else if (upload.status == UPLOAD_FILE_END) {
      if (uploadFile) {
        uploadFile.close();
        Serial.printf("[UPLOAD] Xong anh cho! %u bytes\n", upload.totalSize);
      }
    }
  });

  server.begin();
  Serial.println("[SERVER] Web Server san sang!");
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();

  unsigned long now = millis();

  // 1. KIỂM TRA THAO TÁC CHẠM CẢM ỨNG (TOUCH)
  if (ts.touched()) {
    if (now - lastTouchTime > 450) { // Debounce 450ms
      lastTouchTime = now;
      lastActivityTime = now; // Cập nhật thời điểm hoạt động mới nhất

      if (currentState == STATE_STANDBY) {
        Serial.println("[TOUCH] Thuc giac tu Standby -> Hien thi anh chinh!");
        displayMainPhoto();
      } else if (currentState == STATE_MAIN_PHOTO) {
        Serial.println("[TOUCH] Anh chinh -> Chuyen sang Man Hinh QR...");
        drawTwoQRScreen();
      } else if (currentState == STATE_QR) {
        Serial.println("[TOUCH] Man hinh QR -> Quay ve Anh chinh...");
        displayMainPhoto();
      }
    }
  }

  // 2. KIỂM TRA THỜI GIAN CHỜ TỰ ĐỘNG VÀO STANDBY (IDLE TIMEOUT)
  if (cfgTimeout > 0 && currentState != STATE_STANDBY) {
    if (now - lastActivityTime >= (unsigned long)cfgTimeout * 1000UL) {
      enterStandby();
    }
  }

  // 3. NẾU ĐANG Ở STANDBY ĐỒNG HỒ AOD -> CẬP NHẬT MỖI GIÂY
  if (currentState == STATE_STANDBY && cfgMode == "clock") {
    if (now - lastClockDraw >= 1000) {
      lastClockDraw = now;
      drawStandbyClock(false);
    }
  }

  // 4. VẼ LẠI ẢNH KHI VỪA NHẬN UPLOAD XONG
  if (needRedrawMain) {
    needRedrawMain = false;
    displayMainPhoto();
  }
  if (needRedrawStandby) {
    needRedrawStandby = false;
    if (currentState == STATE_STANDBY && cfgMode == "photo") {
      enterStandby();
    }
  }

  delay(2);
}
