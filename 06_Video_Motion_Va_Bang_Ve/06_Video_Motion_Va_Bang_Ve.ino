/*
 * =====================================================================
 * DỰ ÁN 06: ĐIỆN THOẠI THU NHỎ ESP32 CYD (BẢN CAO CẤP HOÀN THIỆN)
 * KHUNG ẢNH, VIDEO MJPEG/GIF XOAY 90°, BẢNG VIẾT TAY & SMARTWATCH CHRONOGRAPH
 * ĐỊA CHỈ: D:\ESP32\06_Video_Motion_Va_Bang_Ve\06_Video_Motion_Va_Bang_Ve.ino
 * =====================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <DNSServer.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <LittleFS.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <TJpg_Decoder.h>
#include <XPT2046_Touchscreen.h>
#include <ArduinoJson.h>
#include <AnimatedGIF.h>
#include <time.h>
#include <sys/time.h>
#include "qrcode.h"
#include "web_page.h"

// --- CẤU HÌNH WIFI ĐA NĂNG (AP + STA) ---
const char* AP_SSID = "KhungAnh-ESP32";
const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);

// WiFi nhà tự động kết nối để lấy giờ & thời tiết
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

// --- TRẠNG THÁI ỨNG DỤNG (MINI SMARTPHONE) ---
enum AppState {
  STATE_STANDBY_WATCH,  // Màn hình khóa Smartwatch Chronograph (Hình 2)
  STATE_HOME_APPS,      // Màn hình chính 4 App Icons như Smartphone
  STATE_MAIN_PHOTO,     // Xem ảnh toàn màn hình
  STATE_WHITEBOARD,     // Bảng viết tay bằng bút cảm ứng
  STATE_MOTION,         // Phát Video MP4/MJPEG / GIF / Matrix Rain
  STATE_QR              // 2 Mã QR kết nối Wi-Fi & Web
};
AppState currentState = STATE_STANDBY_WATCH;

// Cấu hình
int cfgTimeout = 30;           // Giây chờ (10, 15, 30, 60, 0 = tắt)
String cfgMode = "watch";      // "watch", "motion", "note", "photo", "sleep"
String motionMode = "matrix";  // "matrix", "starfield", "wave", "mjpeg", "gif"
bool isBGR = false;
bool backlightOn = true;

// Dữ liệu Thời tiết thực tế (Tự động cập nhật Open-Meteo)
float currentTemp = 28.0;
float currentRain = 0.0;
int currentWeatherCode = 0;
String cityName = "TP. Ho Chi Minh";
String weatherDesc = "Troi nang dep";
float latitude = 10.8231;
float longitude = 106.6297;
unsigned long lastWeatherFetch = 0;

File uploadFile;
File mjpegFile;
uint8_t mjpegBuf[12288]; // 12KB buffer cho 1 frame JPEG

AnimatedGIF gif;
File gifFile;
bool gifPlaying = false;

unsigned long lastTouchTime = 0;
unsigned long lastActivityTime = 0;
unsigned long lastClockDraw = 0;
unsigned long lastMjpegFrameTime = 0;

// Bộ nhớ góc kim đồng hồ cũ để xoá flicker-free
float oldHourAngle = -1, oldMinAngle = -1, oldSecAngle = -1;

// --- DỮ LIỆU BẢNG VIẾT TAY (WHITEBOARD) ---
const uint16_t PALETTE[] = { TFT_WHITE, TFT_YELLOW, TFT_GREEN, TFT_CYAN, TFT_RED, TFT_BLACK };
int currentPenColorIdx = 0; // Mặc định màu Trắng
int lastPenX = -1, lastPenY = -1;
bool penWasDown = false;

struct StrokeSegment {
  uint8_t x0;
  uint16_t y0;
  uint8_t x1;
  uint16_t y1;
  uint8_t colorIdx;
};

// --- DỮ LIỆU MATRIX DIGITAL RAIN ---
#define MATRIX_COLS 24
int matrixY[MATRIX_COLS];

// --- DỮ LIỆU STARFIELD 3D ---
#define NUM_STARS 70
struct Star { float x, y, z; int oldSx, oldSy; };
Star stars[NUM_STARS];

// --- DỮ LIỆU NEON WAVE VISUALIZER ---
#define NUM_BARS 16
int barHeights[NUM_BARS];
int barTargets[NUM_BARS];

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

// --- CALLBACK CHO ANIMATED GIF TỪ LITTLEFS ---
void * GIFOpenFile(const char *fname, int32_t *pSize) {
  gifFile = LittleFS.open(fname, "r");
  if (gifFile) {
    *pSize = gifFile.size();
    return (void *)&gifFile;
  }
  return NULL;
}
void GIFCloseFile(void *pHandle) { if (gifFile) gifFile.close(); }
int32_t GIFReadFile(GIFFILE *pFile, uint8_t *pBuf, int32_t iLen) {
  if (!gifFile) return 0;
  return gifFile.read(pBuf, iLen);
}
int32_t GIFSeekFile(GIFFILE *pFile, int32_t iPosition) {
  if (!gifFile) return 0;
  return gifFile.seek(iPosition);
}
void GIFDraw(GIFDRAW *pDraw) {
  uint8_t *s = pDraw->pPixels;
  uint16_t *usPalette = pDraw->pPalette;
  int y = pDraw->iY + pDraw->y;
  int iWidth = pDraw->iWidth;
  if (y >= tft.height() || pDraw->iX >= tft.width() || iWidth < 1) return;
  if (iWidth + pDraw->iX > tft.width()) iWidth = tft.width() - pDraw->iX;

  uint16_t lineBuf[240];
  int count = (iWidth > 240) ? 240 : iWidth;
  if (!pDraw->ucHasTransparency) {
    for (int i = 0; i < count; i++) lineBuf[i] = usPalette[s[i]];
    tft.pushImage(pDraw->iX, y, count, 1, lineBuf);
  } else {
    uint8_t ucTransparent = pDraw->ucTransparent;
    int runStart = -1, runLen = 0;
    for (int i = 0; i < count; i++) {
      uint8_t c = s[i];
      if (c != ucTransparent) {
        if (runStart < 0) { runStart = i; runLen = 0; }
        lineBuf[runLen++] = usPalette[c];
      } else {
        if (runStart >= 0) {
          tft.pushImage(pDraw->iX + runStart, y, runLen, 1, lineBuf);
          runStart = -1; runLen = 0;
        }
      }
    }
    if (runStart >= 0) tft.pushImage(pDraw->iX + runStart, y, runLen, 1, lineBuf);
  }
}

// Đọc & lưu cấu hình LittleFS
void loadConfig() {
  if (LittleFS.exists("/config.json")) {
    File f = LittleFS.open("/config.json", "r");
    if (f) {
      JsonDocument doc;
      DeserializationError err = deserializeJson(doc, f);
      if (!err) {
        if (doc["timeout"].is<int>()) cfgTimeout = doc["timeout"].as<int>();
        if (doc["mode"].is<const char*>()) cfgMode = doc["mode"].as<String>();
        if (doc["motion"].is<const char*>()) motionMode = doc["motion"].as<String>();
        if (doc["is_bgr"].is<bool>()) isBGR = doc["is_bgr"].as<bool>();
        if (doc["temp"].is<float>()) currentTemp = doc["temp"].as<float>();
        if (doc["rain"].is<float>()) currentRain = doc["rain"].as<float>();
      }
      f.close();
    }
  }
}

void saveConfig() {
  JsonDocument doc;
  doc["timeout"] = cfgTimeout;
  doc["mode"] = cfgMode;
  doc["motion"] = motionMode;
  doc["is_bgr"] = isBGR;
  doc["temp"] = currentTemp;
  doc["rain"] = currentRain;
  File f = LittleFS.open("/config.json", "w");
  if (f) {
    serializeJson(doc, f);
    f.close();
  }
}

// Chuyển toạ độ cảm ứng XPT2046 sang màn hình 240x320
int touchToScreenX(int rawX) {
  int x = map(rawX, 300, 3800, 0, 240);
  return constrain(x, 0, 239);
}
int touchToScreenY(int rawY) {
  int y = map(rawY, 240, 3800, 0, 320);
  return constrain(y, 0, 319);
}

// Vẽ mã QR
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

// =====================================================================
// TỰ ĐỘNG CẬP NHẬT THỜI TIẾT TỪ OPEN-METEO
// =====================================================================
void fetchRealtimeWeather() {
  if (WiFi.status() != WL_CONNECTED) return;
  Serial.println("[WEATHER] Dang tu dong cap nhat thoi tiet tu Open-Meteo...");
  HTTPClient http;
  char url[256];
  snprintf(url, sizeof(url),
           "http://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f&current=temperature_2m,relative_humidity_2m,rain,weather_code&timezone=Asia%%2FBangkok",
           latitude, longitude);

  http.begin(url);
  http.setTimeout(4000);
  int code = http.GET();
  if (code == 200) {
    String payload = http.getString();
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);
    if (!err && doc.containsKey("current")) {
      currentTemp = doc["current"]["temperature_2m"].as<float>();
      currentRain = doc["current"]["rain"].as<float>();
      currentWeatherCode = doc["current"]["weather_code"].as<int>();

      if (currentWeatherCode == 0) weatherDesc = "Troi quang";
      else if (currentWeatherCode <= 3) weatherDesc = "Co may";
      else if (currentWeatherCode <= 67) weatherDesc = "Co mua";
      else weatherDesc = "Dong bao";

      Serial.printf("[WEATHER] OK: Temp=%.1fC, Rain=%.1fmm, %s\n", currentTemp, currentRain, weatherDesc.c_str());
      saveConfig();
      if (currentState == STATE_STANDBY_WATCH) drawWatchFaceStatic();
    }
  }
  http.end();
}

// =====================================================================
// 1. MẶT ĐỒNG HỒ SMARTWATCH CHRONOGRAPH CAO CẤP (HÌNH 2)
// =====================================================================
void drawWatchFaceStatic() {
  applyColorOrder();
  tft.fillScreen(TFT_BLACK);

  int cx = 120, cy = 132;
  int r = 100;

  // 1. Vành ngoài đồng hồ titan & nền carbon
  tft.drawCircle(cx, cy, r + 2, 0x4228);
  tft.drawCircle(cx, cy, r + 1, 0x2104);
  tft.fillCircle(cx, cy, r, 0x0841); // Nền xanh đen sâu

  // Vạch chia 60 phút thể thao
  for (int i = 0; i < 60; i++) {
    float a = (i * 6 - 90) * 0.0174532925;
    int len = (i % 5 == 0) ? ((i % 15 == 0) ? 9 : 6) : 3;
    int x1 = cx + cos(a) * (r - len);
    int y1 = cy + sin(a) * (r - len);
    int x2 = cx + cos(a) * (r - 1);
    int y2 = cy + sin(a) * (r - 1);
    uint16_t tickCol = (i % 15 == 0) ? TFT_CYAN : ((i % 5 == 0) ? TFT_WHITE : 0x4A69);
    tft.drawLine(x1, y1, x2, y2, tickCol);
  }

  // 2. Cung tròn năng lượng đỉnh (⚡ 100% / 86% màu cyan)
  for (int a = -135; a <= -45; a += 3) {
    float rad = a * 0.0174532925;
    tft.drawPixel(cx + cos(rad) * (r - 12), cy + sin(rad) * (r - 12), 0x07FF);
    tft.drawPixel(cx + cos(rad) * (r - 13), cy + sin(rad) * (r - 13), 0x07FF);
  }
  tft.setTextColor(0x07FF, 0x0841);
  tft.drawCentreString("PWR 100%", cx, cy - 80, 1);

  // 3. Sub-dial Góc Trên Trái: Lịch & Ngày Tháng
  tft.fillRoundRect(36, 92, 50, 42, 6, 0x10A2);
  tft.drawRoundRect(36, 92, 50, 42, 6, 0x2965);
  time_t now = time(nullptr);
  struct tm* t = localtime(&now);
  const char* wdays[] = {"CN", "T.HAI", "T.BA", "T.TU", "T.NAM", "T.SAU", "T.BAY"};
  char dateBuf[16];
  snprintf(dateBuf, sizeof(dateBuf), "%02d/%02d", t->tm_mon + 1, t->tm_mday);
  tft.setTextColor(TFT_WHITE, 0x10A2);
  tft.drawCentreString(dateBuf, 61, 96, 2);
  tft.setTextColor(0xFD20, 0x10A2);
  tft.drawCentreString(wdays[t->tm_wday % 7], 61, 114, 2);

  // 4. Sub-dial Góc Dưới Trái: Thời Tiết & Lượng Mưa Thực Tế
  tft.fillRoundRect(36, 140, 50, 44, 6, 0x10A2);
  tft.drawRoundRect(36, 140, 50, 44, 6, 0x0320);
  char tempBuf[12];
  snprintf(tempBuf, sizeof(tempBuf), "%.0f C", currentTemp);
  tft.setTextColor(TFT_WHITE, 0x10A2);
  tft.drawCentreString(tempBuf, 61, 144, 2);
  char rainBuf[12];
  snprintf(rainBuf, sizeof(rainBuf), "%.1fmm", currentRain);
  tft.setTextColor(TFT_CYAN, 0x10A2);
  tft.drawCentreString(rainBuf, 61, 162, 2);

  // 5. Huy hiệu Thương hiệu Góc Phải (CYD ULTRA)
  tft.fillRoundRect(154, 116, 50, 36, 6, 0x10A2);
  tft.drawRoundRect(154, 116, 50, 36, 6, 0xFD20);
  tft.setTextColor(0x07E0, 0x10A2);
  tft.drawCentreString("CYD", 179, 120, 2);
  tft.setTextColor(0xFD20, 0x10A2);
  tft.drawCentreString("ULTRA", 179, 136, 1);

  // 6. Cung tròn hoạt động đáy (🔥 1526 màu cam)
  for (int a = 45; a <= 135; a += 3) {
    float rad = a * 0.0174532925;
    tft.drawPixel(cx + cos(rad) * (r - 12), cy + sin(rad) * (r - 12), 0xFD20);
    tft.drawPixel(cx + cos(rad) * (r - 13), cy + sin(rad) * (r - 13), 0xFD20);
  }
  tft.setTextColor(0xFD20, 0x0841);
  tft.drawCentreString("LIVE 1526", cx, cy + 68, 1);

  // 7. Dock Kỹ thuật số đáy (Y: 252 -> 316)
  tft.fillRoundRect(8, 252, 224, 62, 10, 0x10A2);
  tft.drawRoundRect(8, 252, 224, 62, 10, 0x29E8);

  char digiBuf[16];
  snprintf(digiBuf, sizeof(digiBuf), "%02d:%02d:%02d", t->tm_hour, t->tm_min, t->tm_sec);
  tft.setTextColor(TFT_YELLOW, 0x10A2);
  tft.drawCentreString(digiBuf, 120, 258, 4);

  tft.setTextColor(TFT_GREEN, 0x10A2);
  tft.drawCentreString("[ + CHAM DE MO KHOA + ]", 120, 290, 2);

  oldHourAngle = -1;
  oldMinAngle = -1;
  oldSecAngle = -1;
}

// Vẽ thanh kim thể thao rỗng (Hollow Sport Hand)
void drawSportHand(int cx, int cy, float angleDeg, int length, int width, uint16_t color) {
  float rad = (angleDeg - 90) * 0.0174532925;
  float perp = rad + 1.5707963;
  int cosP = cos(perp) * width / 2;
  int sinP = sin(perp) * width / 2;
  int tipX = cx + cos(rad) * length;
  int tipY = cy + sin(rad) * length;

  tft.drawLine(cx + cosP, cy + sinP, tipX + cosP, tipY + sinP, color);
  tft.drawLine(cx - cosP, cy - sinP, tipX - cosP, tipY - sinP, color);
  tft.drawLine(tipX - cosP, tipY - sinP, tipX + cosP, tipY + sinP, color);
  tft.drawLine(cx - cosP, cy - sinP, cx + cosP, cy + sinP, color);
}

// Cập nhật kim đồng hồ chuyển động mượt mà
void updateWatchHands() {
  time_t now = time(nullptr);
  struct tm* t = localtime(&now);

  int cx = 120, cy = 132;
  float sAngle = t->tm_sec * 6.0;
  float mAngle = t->tm_min * 6.0 + t->tm_sec * 0.1;
  float hAngle = (t->tm_hour % 12) * 30.0 + t->tm_min * 0.5;

  if (sAngle == oldSecAngle) return;

  // 1. Xoá kim cũ bằng màu nền (0x0841)
  if (oldSecAngle >= 0) {
    float sRad = (oldSecAngle - 90) * 0.0174532925;
    tft.drawLine(cx - cos(sRad) * 14, cy - sin(sRad) * 14, cx + cos(sRad) * 82, cy + sin(sRad) * 82, 0x0841);
  }
  if (oldMinAngle >= 0) {
    drawSportHand(cx, cy, oldMinAngle, 68, 5, 0x0841);
  }
  if (oldHourAngle >= 0) {
    drawSportHand(cx, cy, oldHourAngle, 45, 6, 0x0841);
  }

  // 2. Vẽ lại kim mới
  drawSportHand(cx, cy, hAngle, 45, 6, 0xFD20); // Kim Giờ Vàng Cam
  drawSportHand(cx, cy, mAngle, 68, 5, 0x07FF); // Kim Phút Xanh Băng

  // Kim Giây Xanh Neon
  float sRad = (sAngle - 90) * 0.0174532925;
  tft.drawLine(cx - cos(sRad) * 14, cy - sin(sRad) * 14, cx + cos(sRad) * 82, cy + sin(sRad) * 82, 0x07E0);

  // Tâm trục Center Cap
  tft.fillCircle(cx, cy, 4, 0x07E0);
  tft.fillCircle(cx, cy, 2, TFT_BLACK);

  // Cập nhật đồng hồ số ở dock
  char digiBuf[16];
  snprintf(digiBuf, sizeof(digiBuf), "%02d:%02d:%02d", t->tm_hour, t->tm_min, t->tm_sec);
  tft.setTextColor(TFT_YELLOW, 0x10A2);
  tft.drawCentreString(digiBuf, 120, 258, 4);

  oldHourAngle = hAngle;
  oldMinAngle = mAngle;
  oldSecAngle = sAngle;
}

// =====================================================================
// 2. MÀN HÌNH CHÍNH SMARTPHONE (HOME APPS)
// =====================================================================
void drawHomeScreen() {
  gifPlaying = false;
  if (mjpegFile) mjpegFile.close();
  setBacklight(true);
  applyColorOrder();
  tft.fillScreen(0x0841);

  // Status Bar trên cùng
  tft.fillRect(0, 0, 240, 24, 0x10A2);
  tft.setTextColor(TFT_CYAN, 0x10A2);
  tft.drawString("KhungAnh-ESP32", 8, 4, 2);
  tft.setTextColor(TFT_GREEN, 0x10A2);
  tft.drawRightString("100%", 232, 4, 2);

  // Tiêu đề
  tft.setTextColor(TFT_WHITE, 0x0841);
  tft.drawCentreString("SMARTPHONE MINI", 120, 32, 2);
  tft.drawFastHLine(20, 52, 200, 0x2104);

  // 4 Icon Ứng dụng
  // App 1: Khung Ảnh (X: 16, Y: 60)
  tft.fillRoundRect(16, 60, 98, 86, 12, 0x1A4F);
  tft.drawRoundRect(16, 60, 98, 86, 12, TFT_CYAN);
  tft.setTextColor(TFT_WHITE, 0x1A4F);
  tft.drawCentreString("ANH", 65, 84, 4);
  tft.setTextColor(TFT_CYAN, 0x1A4F);
  tft.drawCentreString("Khung Anh", 65, 120, 2);

  // App 2: Video Motion (X: 126, Y: 60)
  tft.fillRoundRect(126, 60, 98, 86, 12, 0x4808);
  tft.drawRoundRect(126, 60, 98, 86, 12, 0xFD20);
  tft.setTextColor(TFT_WHITE, 0x4808);
  tft.drawCentreString("VIDEO", 175, 84, 4);
  tft.setTextColor(0xFD20, 0x4808);
  tft.drawCentreString("Clip / GIF", 175, 120, 2);

  // App 3: Bảng Viết Tay (X: 16, Y: 160)
  tft.fillRoundRect(16, 160, 98, 86, 12, 0x0320);
  tft.drawRoundRect(16, 160, 98, 86, 12, TFT_GREEN);
  tft.setTextColor(TFT_WHITE, 0x0320);
  tft.drawCentreString("VE", 65, 184, 4);
  tft.setTextColor(TFT_GREEN, 0x0320);
  tft.drawCentreString("But Cam Ung", 65, 220, 2);

  // App 4: QR & Cài Đặt (X: 126, Y: 160)
  tft.fillRoundRect(126, 160, 98, 86, 12, 0x3186);
  tft.drawRoundRect(126, 160, 98, 86, 12, TFT_YELLOW);
  tft.setTextColor(TFT_WHITE, 0x3186);
  tft.drawCentreString("QR", 175, 184, 4);
  tft.setTextColor(TFT_YELLOW, 0x3186);
  tft.drawCentreString("Ket Noi Web", 175, 220, 2);

  // Nút Khóa màn hình
  tft.fillRoundRect(16, 262, 208, 44, 8, 0x2082);
  tft.drawRoundRect(16, 262, 208, 44, 8, 0x05BF);
  tft.setTextColor(0x05BF, 0x2082);
  tft.drawCentreString("[ DONG HO SMARTWATCH ]", 120, 276, 2);

  currentState = STATE_HOME_APPS;
}

// =====================================================================
// 3. BẢNG VIẾT TAY (FIXED: ĐỔI MÀU KHÔNG BỊ RESET NÉT VẼ)
// =====================================================================
void drawTopPalette() {
  tft.fillRect(0, 0, 240, 40, 0x18E3);
  const int swatchX[] = { 10, 48, 86, 124, 162, 200 };
  for (int i = 0; i < 5; i++) {
    tft.fillRoundRect(swatchX[i], 6, 28, 28, 4, PALETTE[i]);
    if (i == currentPenColorIdx) {
      tft.drawRoundRect(swatchX[i] - 2, 4, 32, 32, 5, TFT_WHITE);
    }
  }
  tft.fillRoundRect(swatchX[5], 6, 28, 28, 4, TFT_BLACK);
  tft.drawRoundRect(swatchX[5], 6, 28, 28, 4, TFT_WHITE);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString("T", swatchX[5] + 14, 13, 2);
  if (currentPenColorIdx == 5) {
    tft.drawRoundRect(swatchX[5] - 2, 4, 32, 32, 5, TFT_CYAN);
  }
  tft.drawFastHLine(0, 41, 240, 0x4208);
}

void drawWhiteboardUI(bool reloadStrokes) {
  gifPlaying = false;
  if (mjpegFile) mjpegFile.close();
  setBacklight(true);
  applyColorOrder();

  drawTopPalette();

  if (reloadStrokes) {
    tft.fillRect(0, 42, 240, 237, TFT_BLACK);
    if (LittleFS.exists("/note.strokes")) {
      File f = LittleFS.open("/note.strokes", "r");
      if (f) {
        StrokeSegment seg;
        while (f.read((uint8_t*)&seg, sizeof(StrokeSegment)) == sizeof(StrokeSegment)) {
          uint16_t c = (seg.colorIdx < 6) ? PALETTE[seg.colorIdx] : TFT_WHITE;
          tft.drawLine(seg.x0, seg.y0, seg.x1, seg.y1, c);
          tft.fillCircle(seg.x1, seg.y1, 1, c);
        }
        f.close();
      }
    } else {
      tft.setTextColor(0x4208, TFT_BLACK);
      tft.drawCentreString("[ Dung but cam ung viet vao day ]", 120, 150, 2);
    }
  }

  tft.drawFastHLine(0, 279, 240, 0x4208);
  tft.fillRect(0, 280, 240, 40, 0x18E3);

  tft.fillRoundRect(6, 285, 70, 30, 4, 0x8000);
  tft.drawRoundRect(6, 285, 70, 30, 4, TFT_RED);
  tft.setTextColor(TFT_WHITE, 0x8000);
  tft.drawCentreString("XOA", 41, 292, 2);

  tft.fillRoundRect(84, 285, 72, 30, 4, 0x0320);
  tft.drawRoundRect(84, 285, 72, 30, 4, TFT_GREEN);
  tft.setTextColor(TFT_WHITE, 0x0320);
  tft.drawCentreString("LUU", 120, 292, 2);

  tft.fillRoundRect(164, 285, 70, 30, 4, 0x3186);
  tft.drawRoundRect(164, 285, 70, 30, 4, TFT_CYAN);
  tft.setTextColor(TFT_WHITE, 0x3186);
  tft.drawCentreString("THOAT", 199, 292, 2);

  currentState = STATE_WHITEBOARD;
  penWasDown = false;
  lastPenX = -1;
  lastPenY = -1;
}

// =====================================================================
// 4. PHÁT VIDEO MJPEG & MOTION GRAPHICS
// =====================================================================
void playNextMjpegFrame() {
  if (!mjpegFile) {
    if (!LittleFS.exists("/motion.mjpg")) return;
    mjpegFile = LittleFS.open("/motion.mjpg", "r");
    if (!mjpegFile) return;
  }

  int b = mjpegFile.read();
  while (b != -1) {
    if (b == 0xFF) {
      int nextB = mjpegFile.read();
      if (nextB == 0xD8) {
        mjpegBuf[0] = 0xFF;
        mjpegBuf[1] = 0xD8;
        int bufIdx = 2;
        while (bufIdx < (int)sizeof(mjpegBuf) - 1) {
          int byteVal = mjpegFile.read();
          if (byteVal == -1) break;
          mjpegBuf[bufIdx++] = (uint8_t)byteVal;
          if (byteVal == 0xD9 && mjpegBuf[bufIdx - 2] == 0xFF) {
            TJpgDec.drawJpg(0, 0, mjpegBuf, bufIdx);
            return;
          }
        }
        break;
      }
    }
    b = mjpegFile.read();
  }
  mjpegFile.seek(0);
}

void initMatrixRain() {
  tft.fillScreen(TFT_BLACK);
  for (int i = 0; i < MATRIX_COLS; i++) matrixY[i] = random(-300, 0);
}

void updateMatrixRain() {
  for (int col = 0; col < MATRIX_COLS; col++) {
    int x = col * 10;
    int y = matrixY[col];
    if (y >= 0 && y < 310) {
      char c = (char)random(33, 126);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.drawChar(c, x, y, 1);
      if (y >= 10) {
        tft.setTextColor(0x07E0, TFT_BLACK);
        tft.drawChar((char)random(33, 126), x, y - 10, 1);
      }
      if (y >= 40) tft.fillRect(x, y - 40, 8, 8, TFT_BLACK);
    }
    matrixY[col] += 8;
    if (matrixY[col] > 320) matrixY[col] = random(-60, 0);
  }
}

void initStarfield() {
  tft.fillScreen(TFT_BLACK);
  for (int i = 0; i < NUM_STARS; i++) {
    stars[i].x = random(-120, 120);
    stars[i].y = random(-160, 160);
    stars[i].z = random(20, 255);
    stars[i].oldSx = -1;
    stars[i].oldSy = -1;
  }
}

void updateStarfield() {
  for (int i = 0; i < NUM_STARS; i++) {
    if (stars[i].oldSx >= 0 && stars[i].oldSx < 240 && stars[i].oldSy >= 0 && stars[i].oldSy < 320) {
      tft.drawPixel(stars[i].oldSx, stars[i].oldSy, TFT_BLACK);
    }
    stars[i].z -= 6;
    if (stars[i].z <= 10) {
      stars[i].x = random(-120, 120);
      stars[i].y = random(-160, 160);
      stars[i].z = 255;
    }
    int k = (int)(stars[i].z);
    int sx = 120 + (int)(stars[i].x * 128 / k);
    int sy = 160 + (int)(stars[i].y * 128 / k);
    if (sx >= 0 && sx < 240 && sy >= 0 && sy < 320) {
      uint16_t col = (stars[i].z < 100) ? TFT_WHITE : ((stars[i].z < 180) ? TFT_CYAN : 0x4228);
      tft.drawPixel(sx, sy, col);
      stars[i].oldSx = sx;
      stars[i].oldSy = sy;
    } else {
      stars[i].z = 255;
    }
  }
}

void initNeonWave() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawCentreString("- NEON WAVE VISUALIZER -", 120, 20, 2);
  for (int i = 0; i < NUM_BARS; i++) {
    barHeights[i] = 10;
    barTargets[i] = random(20, 180);
  }
}

void updateNeonWave() {
  int barW = 10;
  int gap = 4;
  int startX = (240 - (NUM_BARS * (barW + gap))) / 2;
  const uint16_t waveColors[] = { TFT_CYAN, 0x05BF, TFT_GREEN, 0x07E0, TFT_YELLOW, 0xFD20, TFT_RED, TFT_MAGENTA };

  for (int i = 0; i < NUM_BARS; i++) {
    if (barHeights[i] < barTargets[i]) barHeights[i] += 4;
    else if (barHeights[i] > barTargets[i]) barHeights[i] -= 4;
    else barTargets[i] = random(15, 200);

    int x = startX + i * (barW + gap);
    int h = barHeights[i];
    int y = 260 - h;

    tft.fillRect(x, 60, barW, 200 - h, TFT_BLACK);
    uint16_t col = waveColors[(i + (millis() / 200)) % 8];
    tft.fillRoundRect(x, y, barW, h, 3, col);
  }
}

void startMotionMode() {
  gifPlaying = false;
  if (mjpegFile) { mjpegFile.close(); }
  setBacklight(true);
  applyColorOrder();
  tft.fillScreen(TFT_BLACK);
  currentState = STATE_MOTION;

  Serial.printf("[MOTION] Khoi chay che do: %s\n", motionMode.c_str());

  if (motionMode == "matrix") {
    initMatrixRain();
  } else if (motionMode == "starfield") {
    initStarfield();
  } else if (motionMode == "wave") {
    initNeonWave();
  } else if (motionMode == "gif" && LittleFS.exists("/motion.gif")) {
    gif.begin(GIF_PALETTE_RGB565_BE);
    if (gif.open("/motion.gif", GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw)) {
      gifPlaying = true;
    }
  } else if (motionMode == "mjpeg" && LittleFS.exists("/motion.mjpg")) {
    mjpegFile = LittleFS.open("/motion.mjpg", "r");
  } else {
    motionMode = "matrix";
    initMatrixRain();
  }
}

// =====================================================================
// 5. MÀN HÌNH QR & XEM ẢNH
// =====================================================================
void drawTwoQRScreen() {
  gifPlaying = false;
  if (mjpegFile) mjpegFile.close();
  setBacklight(true);
  applyColorOrder();
  tft.fillScreen(0x0841);

  tft.fillRect(0, 0, 240, 26, 0x10A2);
  tft.setTextColor(TFT_YELLOW, 0x10A2);
  tft.drawCentreString("KET NOI VA MENU DIEU KHIEN", 120, 5, 2);

  tft.fillRoundRect(6, 30, 228, 105, 6, 0x10A2);
  tft.drawRoundRect(6, 30, 228, 105, 6, TFT_CYAN);
  renderQR("WIFI:T:nopass;S:KhungAnh-ESP32;;", 14, 38, 3);
  tft.setTextColor(TFT_CYAN, 0x10A2);
  tft.drawString("BUOC 1:", 112, 38, 2);
  tft.setTextColor(TFT_WHITE, 0x10A2);
  tft.drawString("Vao Wi-Fi:", 112, 56, 2);
  tft.setTextColor(TFT_GREEN, 0x10A2);
  tft.drawString("KhungAnh-ESP32", 112, 74, 2);
  tft.setTextColor(TFT_LIGHTGREY, 0x10A2);
  tft.drawString("(Khong mat khau)", 112, 94, 1);

  tft.fillRoundRect(6, 140, 228, 105, 6, 0x10A2);
  tft.drawRoundRect(6, 140, 228, 105, 6, 0xFD20);
  renderQR("http://192.168.4.1", 14, 148, 3);
  tft.setTextColor(0xFD20, 0x10A2);
  tft.drawString("BUOC 2:", 112, 148, 2);
  tft.setTextColor(TFT_WHITE, 0x10A2);
  tft.drawString("Mo trinh duyet:", 112, 166, 2);
  tft.setTextColor(TFT_YELLOW, 0x10A2);
  tft.drawString("192.168.4.1", 112, 186, 2);
  tft.setTextColor(TFT_LIGHTGREY, 0x10A2);
  tft.drawString("Gui anh/video/ve", 112, 206, 2);

  tft.fillRoundRect(16, 256, 208, 48, 8, 0x0320);
  tft.drawRoundRect(16, 256, 208, 48, 8, TFT_GREEN);
  tft.setTextColor(TFT_WHITE, 0x0320);
  tft.drawCentreString("[ VE MAN HINH CHINH ]", 120, 272, 2);

  currentState = STATE_QR;
}

void displayMainPhoto() {
  gifPlaying = false;
  if (mjpegFile) mjpegFile.close();
  setBacklight(true);
  if (LittleFS.exists("/photo.jpg")) {
    applyColorOrder();
    tft.fillScreen(TFT_BLACK);
    TJpgDec.drawFsJpg(0, 0, "/photo.jpg", LittleFS);
    currentState = STATE_MAIN_PHOTO;
  } else {
    drawHomeScreen();
  }
}

void enterStandby() {
  Serial.printf("[STANDBY] Vao che do: %s\n", cfgMode.c_str());
  if (cfgMode == "sleep") {
    setBacklight(false);
    currentState = STATE_STANDBY_WATCH;
  } else if (cfgMode == "motion") {
    startMotionMode();
  } else if (cfgMode == "photo") {
    displayMainPhoto();
  } else {
    setBacklight(true);
    drawWatchFaceStatic();
    currentState = STATE_STANDBY_WATCH;
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n=== DIEN THOAI THU NHO ESP32 CYD (ULTRA EDITION) ===");

  pinMode(TFT_BL, OUTPUT);
  setBacklight(true);

  tft.init();
  tft.setRotation(0);
  tft.invertDisplay(true);
  applyColorOrder();
  tft.fillScreen(TFT_BLACK);

  TJpgDec.setJpgScale(1);
  TJpgDec.setCallback(tft_output);
  TJpgDec.setSwapBytes(true);

  touchSpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin(touchSpi);
  ts.setRotation(0);

  if (LittleFS.begin(true)) {
    loadConfig();
  }

  configTime(7 * 3600, 0, "pool.ntp.org", "time.google.com");

  // Khởi động Wi-Fi song song: AP riêng + kết nối mạng nhà để lấy thời tiết
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(AP_SSID);
  dnsServer.start(DNS_PORT, "*", apIP);
  WiFi.begin(STA_SSID, STA_PASS);

  // Hiển thị Màn hình khóa Smartwatch Chronograph (Hình 2)
  drawWatchFaceStatic();
  lastActivityTime = millis();

  // --- CẤU HÌNH WEB SERVER ---
  server.on("/", HTTP_GET, []() { server.send_P(200, "text/html", INDEX_HTML); });
  server.on("/hotspot-detect.html", HTTP_GET, []() { server.send_P(200, "text/html", INDEX_HTML); });
  server.on("/generate_204", HTTP_GET, []() { server.send_P(200, "text/html", INDEX_HTML); });
  server.on("/connecttest.txt", HTTP_GET, []() { server.send_P(200, "text/html", INDEX_HTML); });

  server.on("/get_config", HTTP_GET, []() {
    String json = "{\"timeout\":" + String(cfgTimeout) + ",\"mode\":\"" + cfgMode + "\",\"motion\":\"" + motionMode + "\"}";
    server.send(200, "application/json", json);
  });

  server.on("/save_config", HTTP_GET, []() {
    if (server.hasArg("timeout")) cfgTimeout = server.arg("timeout").toInt();
    if (server.hasArg("mode")) cfgMode = server.arg("mode");
    saveConfig();
    lastActivityTime = millis();
    server.send(200, "text/plain", "Da luu!");
  });

  server.on("/sync_time", HTTP_GET, []() {
    if (server.hasArg("epoch")) {
      time_t epoch = (time_t)server.arg("epoch").toInt();
      struct timeval tv;
      tv.tv_sec = epoch + (7 * 3600);
      tv.tv_usec = 0;
      settimeofday(&tv, NULL);
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "Thieu epoch");
    }
  });

  server.on("/sync_weather", HTTP_GET, []() {
    if (server.hasArg("temp")) currentTemp = server.arg("temp").toFloat();
    if (server.hasArg("rain")) currentRain = server.arg("rain").toFloat();
    saveConfig();
    if (currentState == STATE_STANDBY_WATCH) drawWatchFaceStatic();
    server.send(200, "text/plain", "OK");
  });

  server.on("/show_watch", HTTP_GET, []() {
    drawWatchFaceStatic();
    currentState = STATE_STANDBY_WATCH;
    server.send(200, "text/plain", "Dang hien thi Smartwatch!");
  });

  server.on("/toggle_color", HTTP_GET, []() {
    isBGR = !isBGR;
    applyColorOrder();
    saveConfig();
    if (currentState == STATE_STANDBY_WATCH) drawWatchFaceStatic();
    else if (currentState == STATE_HOME_APPS) drawHomeScreen();
    else if (currentState == STATE_MAIN_PHOTO) displayMainPhoto();
    server.send(200, "text/plain", isBGR ? "BGR" : "RGB");
  });

  server.on("/whiteboard", HTTP_GET, []() {
    drawWhiteboardUI(true);
    server.send(200, "text/plain", "Da mo bang ve!");
  });

  server.on("/view_note", HTTP_GET, []() {
    drawWhiteboardUI(true);
    server.send(200, "text/plain", "Dang xem ghi chu");
  });

  server.on("/clear_note", HTTP_GET, []() {
    if (LittleFS.exists("/note.strokes")) LittleFS.remove("/note.strokes");
    if (currentState == STATE_WHITEBOARD) drawWhiteboardUI(true);
    server.send(200, "text/plain", "Da xoa!");
  });

  server.on("/set_effect", HTTP_GET, []() {
    if (server.hasArg("eff")) {
      motionMode = server.arg("eff");
      saveConfig();
      startMotionMode();
      server.send(200, "text/plain", "Dang phat: " + motionMode);
    } else {
      server.send(400, "text/plain", "Thieu eff");
    }
  });

  server.on("/upload", HTTP_POST, []() {
    server.send(200, "text/plain", "OK");
    lastActivityTime = millis();
    displayMainPhoto();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) uploadFile = LittleFS.open("/photo.jpg", "w");
    else if (upload.status == UPLOAD_FILE_WRITE && uploadFile) uploadFile.write(upload.buf, upload.currentSize);
    else if (upload.status == UPLOAD_FILE_END && uploadFile) uploadFile.close();
  });

  server.on("/upload_gif", HTTP_POST, []() {
    server.send(200, "text/plain", "OK");
    motionMode = "gif";
    saveConfig();
    startMotionMode();
    lastActivityTime = millis();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) uploadFile = LittleFS.open("/motion.gif", "w");
    else if (upload.status == UPLOAD_FILE_WRITE && uploadFile) uploadFile.write(upload.buf, upload.currentSize);
    else if (upload.status == UPLOAD_FILE_END && uploadFile) uploadFile.close();
  });

  server.on("/upload_mjpeg", HTTP_POST, []() {
    server.send(200, "text/plain", "OK");
    motionMode = "mjpeg";
    saveConfig();
    startMotionMode();
    lastActivityTime = millis();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      if (mjpegFile) mjpegFile.close();
      uploadFile = LittleFS.open("/motion.mjpg", "w");
    } else if (upload.status == UPLOAD_FILE_WRITE && uploadFile) {
      uploadFile.write(upload.buf, upload.currentSize);
    } else if (upload.status == UPLOAD_FILE_END && uploadFile) {
      uploadFile.close();
    }
  });

  server.begin();
  Serial.println("[SERVER] San sang!");
}

void loop() {
  dnsServer.processNextRequest();
  server.handleClient();

  unsigned long now = millis();

  // Tự động kiểm tra thời tiết mỗi 15 phút (khi có mạng)
  if (WiFi.status() == WL_CONNECTED) {
    if (now - lastWeatherFetch >= 900000UL || lastWeatherFetch == 0) {
      lastWeatherFetch = now;
      fetchRealtimeWeather();
    }
  }

  // ===================================================================
  // 1. XỬ LÝ CẢM ỨNG (TOUCH / STYLUS INPUT)
  // ===================================================================
  if (ts.touched()) {
    TS_Point p = ts.getPoint();
    int sx = touchToScreenX(p.x);
    int sy = touchToScreenY(p.y);

    // --- TRƯỜNG HỢP A: BẢNG VIẾT TAY (WHITEBOARD) ---
    if (currentState == STATE_WHITEBOARD) {
      lastActivityTime = now;

      // 1. Chạm vùng Bảng màu: CHỈ CẬP NHẬT THANH MÀU!
      if (sy < 42) {
        if (now - lastTouchTime > 250) {
          lastTouchTime = now;
          const int swatchX[] = { 10, 48, 86, 124, 162, 200 };
          for (int i = 0; i < 6; i++) {
            if (sx >= swatchX[i] - 4 && sx <= swatchX[i] + 32) {
              currentPenColorIdx = i;
              drawTopPalette(); // KHÔNG XOÁ BẢNG!
              break;
            }
          }
        }
      }
      // 2. Chạm vùng Điều khiển dưới đáy (Y >= 280)
      else if (sy >= 280) {
        if (now - lastTouchTime > 300) {
          lastTouchTime = now;
          // Nút Xoá
          if (sx >= 6 && sx <= 74) {
            if (LittleFS.exists("/note.strokes")) LittleFS.remove("/note.strokes");
            tft.fillRect(0, 42, 240, 237, TFT_BLACK);
          }
          // Nút Lưu
          else if (sx >= 84 && sx <= 156) {
            tft.setTextColor(TFT_GREEN, 0x18E3);
            tft.drawCentreString("DA LUU!", 120, 292, 2);
            delay(400);
            tft.drawCentreString("LUU", 120, 292, 2);
          }
          // Nút Thoát
          else if (sx >= 164 && sx <= 234) {
            drawHomeScreen();
          }
        }
      }
      // 3. VÙNG VẼ CHÍNH BẰNG BÚT CẢM ỨNG (Y: 42 -> 278)
      else {
        uint16_t penColor = (currentPenColorIdx < 6) ? PALETTE[currentPenColorIdx] : TFT_WHITE;
        if (penColor == TFT_BLACK) {
          tft.fillCircle(sx, sy, 8, TFT_BLACK);
        } else {
          if (penWasDown && lastPenX >= 0) {
            tft.drawLine(lastPenX, lastPenY, sx, sy, penColor);
            tft.fillCircle(sx, sy, 1, penColor);

            File f = LittleFS.open("/note.strokes", "a");
            if (f) {
              StrokeSegment seg = { (uint8_t)lastPenX, (uint16_t)lastPenY, (uint8_t)sx, (uint16_t)sy, (uint8_t)currentPenColorIdx };
              f.write((uint8_t*)&seg, sizeof(StrokeSegment));
              f.close();
            }
          } else {
            tft.fillCircle(sx, sy, 1, penColor);
          }
        }
        lastPenX = sx;
        lastPenY = sy;
        penWasDown = true;
      }
    }
    // --- TRƯỜNG HỢP B: ĐỒNG HỒ SMARTWATCH (CHẠM ĐỂ MỞ KHÓA VÀO HOME) ---
    else if (currentState == STATE_STANDBY_WATCH) {
      if (now - lastTouchTime > 400) {
        lastTouchTime = now;
        lastActivityTime = now;
        setBacklight(true);
        drawHomeScreen();
      }
    }
    // --- TRƯỜNG HỢP C: MÀN HÌNH CHÍNH SMARTPHONE (CHỌN APP) ---
    else if (currentState == STATE_HOME_APPS) {
      if (now - lastTouchTime > 350) {
        lastTouchTime = now;
        lastActivityTime = now;

        // App 1: Khung Ảnh (Y: 60-146, X: 16-114)
        if (sy >= 60 && sy <= 146 && sx >= 16 && sx <= 114) {
          displayMainPhoto();
        }
        // App 2: Video (Y: 60-146, X: 126-224)
        else if (sy >= 60 && sy <= 146 && sx >= 126 && sx <= 224) {
          startMotionMode();
        }
        // App 3: Bảng Viết Tay (Y: 160-246, X: 16-114)
        else if (sy >= 160 && sy <= 246 && sx >= 16 && sx <= 114) {
          drawWhiteboardUI(true);
        }
        // App 4: QR & Cài Đặt (Y: 160-246, X: 126-224)
        else if (sy >= 160 && sy <= 246 && sx >= 126 && sx <= 224) {
          drawTwoQRScreen();
        }
        // Nút Khóa về Đồng Hồ Smartwatch ở đáy
        else if (sy >= 260) {
          drawWatchFaceStatic();
          currentState = STATE_STANDBY_WATCH;
        }
      }
    }
    // --- TRƯỜNG HỢP D: ĐANG Ở XEM ẢNH, VIDEO HOẶC QR ---
    else {
      if (now - lastTouchTime > 400) {
        lastTouchTime = now;
        lastActivityTime = now;
        drawHomeScreen();
      }
    }
  } else {
    penWasDown = false;
    lastPenX = -1;
    lastPenY = -1;
  }

  // ===================================================================
  // 2. PHÁT VIDEO MJPEG & MOTION GRAPHICS
  // ===================================================================
  if (currentState == STATE_MOTION) {
    if (motionMode == "matrix") {
      updateMatrixRain();
      delay(25);
    } else if (motionMode == "starfield") {
      updateStarfield();
      delay(15);
    } else if (motionMode == "wave") {
      updateNeonWave();
      delay(20);
    } else if (motionMode == "mjpeg") {
      if (now - lastMjpegFrameTime >= 80) { // ~12 fps
        lastMjpegFrameTime = now;
        playNextMjpegFrame();
      }
    } else if (motionMode == "gif" && gifPlaying) {
      if (!gif.playFrame(true, NULL)) {
        gif.reset();
      }
    }
  }

  // ===================================================================
  // 3. TỰ ĐỘNG KHÓA VÀ VÀO SMARTWATCH SAU 30S KHÔNG CHẠM
  // ===================================================================
  if (cfgTimeout > 0 && currentState != STATE_STANDBY_WATCH && currentState != STATE_WHITEBOARD) {
    if (now - lastActivityTime >= (unsigned long)cfgTimeout * 1000UL) {
      enterStandby();
    }
  }

  // ===================================================================
  // 4. CẬP NHẬT KIM MẶT ĐỒNG HỒ SMARTWATCH
  // ===================================================================
  if (currentState == STATE_STANDBY_WATCH && backlightOn) {
    updateWatchHands();
  }

  delay(2);
}
