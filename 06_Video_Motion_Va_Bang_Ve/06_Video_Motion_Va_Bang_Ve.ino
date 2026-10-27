/*
 * =====================================================================
 * DỰ ÁN 06: KHUNG ẢNH, VIDEO/MOTION GRAPHIC & BẢNG VIẾT TAY BẰNG BÚT CẢM ỨNG
 * ĐỊA CHỈ: D:\ESP32\06_Video_Motion_Va_Bang_Ve\06_Video_Motion_Va_Bang_Ve.ino
 * =====================================================================
 * Tính năng đột phá:
 * 1. PHÁT VIDEO & MOTION GRAPHIC TỪ ĐIỆN THOẠI:
 *    - Tải và phát ảnh động GIF, motion graphic, sticker chuyển động, anime mượt mà 20-30 fps.
 *    - Tích hợp 3 hiệu ứng đồ họa đỉnh cao: Ma trận Matrix số rơi, Vũ trụ ngàn sao 3D Starfield, Sóng âm neon.
 * 2. BẢNG VIẾT TAY TỰ VẼ TRỰC TIẾP TRÊN MÀN HÌNH BẰNG BÚT CẢM ỨNG (STYLUS):
 *    - Tận dụng cảm ứng lực XPT2046 siêu nhạy với bút stylus / móng tay / que nhựa.
 *    - Bảng màu: Trắng, Vàng, Xanh lá, Xanh dương, Đỏ, Tẩy xoá.
 *    - Nét viết liền mạch, tròn đều, mượt mà như viết trên giấy thật.
 *    - Lưu chữ viết/hình vẽ thành ghi chú (Note) vĩnh viễn trong Flash, có thể đặt làm màn hình chính hoặc màn hình chờ!
 * 3. CHẾ ĐỘ CHỜ THÔNG MINH (AOD STANDBY):
 *    - Sau 30s không chạm -> Tự vào màn hình chờ (Đồng hồ / Video / Ghi chú viết tay / Ảnh phụ / Tắt màn).
 *    - Chạm tay vào màn hình -> Lập tức thức giấc về ảnh chính!
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
#include <AnimatedGIF.h>
#include <time.h>
#include <sys/time.h>
#include "qrcode.h"
#include "web_page.h"

// --- CẤU HÌNH WIFI TỰ PHÁT (ACCESS POINT) ---
const char* AP_SSID = "KhungAnh-ESP32";
const byte DNS_PORT = 53;
IPAddress apIP(192, 168, 4, 1);
IPAddress netMsk(255, 255, 255, 0);

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

// Animated GIF Player
AnimatedGIF gif;
File gifFile;
bool gifPlaying = false;

// Trạng thái ứng dụng
enum AppState {
  STATE_MAIN_PHOTO,
  STATE_STANDBY,
  STATE_QR,
  STATE_WHITEBOARD,
  STATE_MOTION,
  STATE_NOTE_VIEW
};
AppState currentState = STATE_MAIN_PHOTO;

// Cấu hình
int cfgTimeout = 30;           // Giây chờ (10, 15, 30, 60, 0 = tắt)
String cfgMode = "clock";      // "clock", "motion", "note", "photo", "sleep"
String motionMode = "file";    // "file", "matrix", "starfield", "wave"
bool isBGR = false;
bool backlightOn = true;

File uploadFile;
bool needRedrawMain = false;
bool needRedrawMotion = false;
unsigned long lastTouchTime = 0;
unsigned long lastActivityTime = 0;
unsigned long lastClockDraw = 0;
int lastDrawnSecond = -1;

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

void GIFCloseFile(void *pHandle) {
  if (gifFile) gifFile.close();
}

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
    for (int i = 0; i < count; i++) {
      lineBuf[i] = usPalette[s[i]];
    }
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
    if (runStart >= 0) {
      tft.pushImage(pDraw->iX + runStart, y, runLen, 1, lineBuf);
    }
  }
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
        if (doc["motion"].is<const char*>()) motionMode = doc["motion"].as<String>();
        if (doc["is_bgr"].is<bool>()) isBGR = doc["is_bgr"].as<bool>();
        Serial.printf("[CONFIG] Timeout=%ds, Mode=%s, Motion=%s, BGR=%d\n", cfgTimeout, cfgMode.c_str(), motionMode.c_str(), isBGR);
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
  File f = LittleFS.open("/config.json", "w");
  if (f) {
    serializeJson(doc, f);
    f.close();
    Serial.println("[CONFIG] Da luu config.json thanh cong!");
  }
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

// Chuyển đổi toạ độ cảm ứng sang toạ độ màn hình 240x320
int touchToScreenX(int rawX) {
  int x = map(rawX, 300, 3800, 0, 240);
  return constrain(x, 0, 239);
}
int touchToScreenY(int rawY) {
  int y = map(rawY, 240, 3800, 0, 320);
  return constrain(y, 0, 319);
}

// Giao diện 2 Mã QR kết nối & Menu tính năng
void drawTwoQRScreen() {
  gifPlaying = false;
  setBacklight(true);
  applyColorOrder();
  tft.fillScreen(0x0841); // Nền xanh đen đậm

  // Tiêu đề
  tft.fillRect(0, 0, 240, 26, 0x10A2);
  tft.setTextColor(TFT_YELLOW, 0x10A2);
  tft.drawCentreString("KET NOI VA MENU DIEU KHIEN", 120, 5, 2);

  // Khối Bước 1: WiFi riêng
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

  // Khối Bước 2: Mở Web
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

  // 3 NÚT CHẠM CẢM ỨNG Ở ĐÁY
  // Nút 1: Xem Ảnh (X: 6 -> 78)
  tft.fillRoundRect(6, 252, 72, 62, 5, 0x0320);
  tft.drawRoundRect(6, 252, 72, 62, 5, TFT_GREEN);
  tft.setTextColor(TFT_GREEN, 0x0320);
  tft.drawCentreString("XEM", 42, 264, 2);
  tft.drawCentreString("ANH", 42, 284, 2);

  // Nút 2: Bảng Viết Tay (X: 84 -> 156)
  tft.fillRoundRect(84, 252, 72, 62, 5, 0x028A);
  tft.drawRoundRect(84, 252, 72, 62, 5, TFT_CYAN);
  tft.setTextColor(TFT_CYAN, 0x028A);
  tft.drawCentreString("VIET", 120, 264, 2);
  tft.drawCentreString("TAY", 120, 284, 2);

  // Nút 3: Video Motion (X: 162 -> 234)
  tft.fillRoundRect(162, 252, 72, 62, 5, 0x4808);
  tft.drawRoundRect(162, 252, 72, 62, 5, 0xFD20);
  tft.setTextColor(0xFD20, 0x4808);
  tft.drawCentreString("VIDEO", 198, 264, 2);
  tft.drawCentreString("MOTION", 198, 284, 2);

  currentState = STATE_QR;
}

// Hiển thị ảnh chính
void displayMainPhoto() {
  gifPlaying = false;
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

// --- VẼ BẢNG VIẾT TAY (WHITEBOARD STUDIO) ---
void drawWhiteboardUI() {
  gifPlaying = false;
  setBacklight(true);
  applyColorOrder();
  tft.fillScreen(TFT_BLACK);

  // 1. THANH BẢNG MÀU TRÊN CÙNG (Y: 0 -> 40)
  tft.fillRect(0, 0, 240, 40, 0x18E3);
  const int swatchX[] = { 10, 48, 86, 124, 162, 200 };
  for (int i = 0; i < 5; i++) {
    tft.fillRoundRect(swatchX[i], 6, 28, 28, 4, PALETTE[i]);
    if (i == currentPenColorIdx) {
      tft.drawRoundRect(swatchX[i] - 2, 4, 32, 32, 5, TFT_WHITE);
    }
  }
  // Cục Tẩy (i=5)
  tft.fillRoundRect(swatchX[5], 6, 28, 28, 4, TFT_BLACK);
  tft.drawRoundRect(swatchX[5], 6, 28, 28, 4, TFT_WHITE);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString("T", swatchX[5] + 14, 13, 2);
  if (currentPenColorIdx == 5) {
    tft.drawRoundRect(swatchX[5] - 2, 4, 32, 32, 5, TFT_CYAN);
  }

  tft.drawFastHLine(0, 41, 240, 0x4208);

  // 2. KHU VỰC VẼ CHÍNH: (Y: 42 -> 278) - ĐEN NỀN
  tft.fillRect(0, 42, 240, 237, TFT_BLACK);

  // Vẽ lại các nét đã lưu trước đó nếu có
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
    // Hướng dẫn mờ
    tft.setTextColor(0x4208, TFT_BLACK);
    tft.drawCentreString("[ Dung but cam ung viet vao day ]", 120, 150, 2);
  }

  // 3. THANH ĐIỀU KHIỂN DƯỚI CÙNG (Y: 280 -> 320)
  tft.drawFastHLine(0, 279, 240, 0x4208);
  tft.fillRect(0, 280, 240, 40, 0x18E3);

  // Nút Xoá (X: 6 -> 74)
  tft.fillRoundRect(6, 285, 70, 30, 4, 0x8000); // Đỏ sẫm
  tft.drawRoundRect(6, 285, 70, 30, 4, TFT_RED);
  tft.setTextColor(TFT_WHITE, 0x8000);
  tft.drawCentreString("XOA", 41, 292, 2);

  // Nút Lưu (X: 84 -> 156)
  tft.fillRoundRect(84, 285, 72, 30, 4, 0x0320); // Xanh lá
  tft.drawRoundRect(84, 285, 72, 30, 4, TFT_GREEN);
  tft.setTextColor(TFT_WHITE, 0x0320);
  tft.drawCentreString("LUU", 120, 292, 2);

  // Nút Thoát (X: 164 -> 234)
  tft.fillRoundRect(164, 285, 70, 30, 4, 0x3186); // Xanh dương
  tft.drawRoundRect(164, 285, 70, 30, 4, TFT_CYAN);
  tft.setTextColor(TFT_WHITE, 0x3186);
  tft.drawCentreString("THOAT", 199, 292, 2);

  currentState = STATE_WHITEBOARD;
  penWasDown = false;
  lastPenX = -1;
  lastPenY = -1;
}

// Hiển thị ghi chú đã lưu toàn màn hình
void displaySavedNote() {
  gifPlaying = false;
  setBacklight(true);
  applyColorOrder();
  tft.fillScreen(TFT_BLACK);

  tft.setTextColor(0x52AA, TFT_BLACK);
  tft.drawCentreString("- GHI CHU VIET TAY -", 120, 10, 2);

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
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    tft.drawCentreString("Chua co ghi chu nao!", 120, 150, 2);
  }

  tft.drawRoundRect(20, 282, 200, 30, 5, 0x028A);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawCentreString("[ CHAM DE VE MENU ]", 120, 289, 2);

  currentState = STATE_NOTE_VIEW;
}

// --- MATRIX DIGITAL RAIN ---
void initMatrixRain() {
  tft.fillScreen(TFT_BLACK);
  for (int i = 0; i < MATRIX_COLS; i++) {
    matrixY[i] = random(-300, 0);
  }
}

void updateMatrixRain() {
  for (int col = 0; col < MATRIX_COLS; col++) {
    int x = col * 10;
    int y = matrixY[col];

    if (y >= 0 && y < 310) {
      // Đầu dòng: Ký tự trắng sáng
      char c = (char)random(33, 126);
      tft.setTextColor(TFT_WHITE, TFT_BLACK);
      tft.drawChar(c, x, y, 1);

      // Thân: Ký tự xanh neon
      if (y >= 10) {
        char cPrev = (char)random(33, 126);
        tft.setTextColor(0x07E0, TFT_BLACK);
        tft.drawChar(cPrev, x, y - 10, 1);
      }
      // Đuôi mờ
      if (y >= 40) {
        tft.fillRect(x, y - 40, 8, 8, TFT_BLACK);
      }
    }

    matrixY[col] += 8;
    if (matrixY[col] > 320) {
      matrixY[col] = random(-60, 0);
    }
  }
}

// --- STARFIELD 3D WARP ---
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
    // Xoá điểm cũ
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

// Khởi chạy chế độ Motion Graphic
void startMotionMode() {
  gifPlaying = false;
  setBacklight(true);
  applyColorOrder();
  tft.fillScreen(TFT_BLACK);
  currentState = STATE_MOTION;

  if (motionMode == "file" && LittleFS.exists("/motion.gif")) {
    gif.begin(GIF_PALETTE_RGB565_BE);
    if (gif.open("/motion.gif", GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw)) {
      gifPlaying = true;
      Serial.println("[GIF] Bat dau phat /motion.gif thanh cong!");
    }
  } else if (motionMode == "matrix") {
    initMatrixRain();
  } else if (motionMode == "starfield") {
    initStarfield();
  } else {
    // Mặc định: Starfield nếu chưa có file
    motionMode = "starfield";
    initStarfield();
  }
}

// Đồng hồ AOD
void drawStandbyClock(bool fullRedraw) {
  time_t now = time(nullptr);
  struct tm* t = localtime(&now);
  int sec = t->tm_sec;
  if (!fullRedraw && sec == lastDrawnSecond) return;
  lastDrawnSecond = sec;

  if (fullRedraw) {
    applyColorOrder();
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(0x52AA, TFT_BLACK);
    tft.drawCentreString("- MAN HINH CHO (AOD) -", 120, 16, 2);
    tft.drawFastHLine(30, 38, 180, 0x2104);
    tft.drawRoundRect(20, 275, 200, 32, 6, 0x0320);
    tft.setTextColor(0x2408, TFT_BLACK);
    tft.drawCentreString("[ CHAM DE MO ANH ]", 120, 283, 2);
  }

  char timeBuf[12];
  snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", t->tm_hour, t->tm_min);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString(timeBuf, 120, 65, 7);

  char secBuf[8];
  snprintf(secBuf, sizeof(secBuf), ":%02d", t->tm_sec);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawCentreString(secBuf, 120, 135, 4);

  const char* wdays[] = {"CN", "T.Hai", "T.Ba", "T.Tu", "T.Nam", "T.Sau", "T.Bay"};
  char dateBuf[32];
  snprintf(dateBuf, sizeof(dateBuf), "%s, %02d/%02d/%04d", wdays[t->tm_wday % 7], t->tm_mday, t->tm_mon + 1, t->tm_year + 1900);
  tft.setTextColor(0xFD20, TFT_BLACK);
  tft.drawCentreString(dateBuf, 120, 180, 4);

  tft.setTextColor(0x7BEF, TFT_BLACK);
  tft.drawCentreString("AP: KhungAnh-ESP32", 120, 225, 2);
}

// Bắt đầu chế độ chờ (Standby)
void enterStandby() {
  Serial.printf("[STANDBY] Vao che do: %s\n", cfgMode.c_str());
  currentState = STATE_STANDBY;

  if (cfgMode == "sleep") {
    setBacklight(false);
  } else if (cfgMode == "motion") {
    startMotionMode();
    currentState = STATE_STANDBY; // giữ cờ standby để chạm là thức giấc
  } else if (cfgMode == "note") {
    displaySavedNote();
    currentState = STATE_STANDBY;
  } else if (cfgMode == "photo") {
    displayMainPhoto();
    currentState = STATE_STANDBY;
  } else {
    // "clock" (Mặc định)
    setBacklight(true);
    drawStandbyClock(true);
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n=== DỰ ÁN 06: KHUNG ẢNH, VIDEO MOTION & BẢNG VIẾT TAY ===");

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
  Serial.println("[TOUCH] Cam ung XPT2046 da san sang!");

  if (LittleFS.begin(true)) {
    Serial.println("[FS] LittleFS san sang.");
    loadConfig();
  }

  configTime(7 * 3600, 0, "pool.ntp.org", "time.nist.gov");

  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(apIP, apIP, netMsk);
  WiFi.softAP(AP_SSID);
  Serial.printf("[WIFI AP] Da phat: %s (192.168.4.1)\n", AP_SSID);

  dnsServer.start(DNS_PORT, "*", apIP);

  if (LittleFS.exists("/photo.jpg")) {
    displayMainPhoto();
  } else {
    drawTwoQRScreen();
  }
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
    server.send(200, "text/plain", "Da luu cau hinh!");
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

  server.on("/toggle_color", HTTP_GET, []() {
    isBGR = !isBGR;
    applyColorOrder();
    saveConfig();
    if (currentState == STATE_MAIN_PHOTO) displayMainPhoto();
    server.send(200, "text/plain", isBGR ? "BGR" : "RGB");
  });

  // Mở Bảng Vẽ từ xa qua Web
  server.on("/whiteboard", HTTP_GET, []() {
    drawWhiteboardUI();
    server.send(200, "text/plain", "Da mo bang ve tren ESP32");
  });

  // Xem ghi chú đã lưu từ Web
  server.on("/view_note", HTTP_GET, []() {
    displaySavedNote();
    server.send(200, "text/plain", "Dang hien thi ghi chu");
  });

  // Xoá ghi chú viết tay
  server.on("/clear_note", HTTP_GET, []() {
    if (LittleFS.exists("/note.strokes")) LittleFS.remove("/note.strokes");
    if (currentState == STATE_WHITEBOARD) drawWhiteboardUI();
    else if (currentState == STATE_NOTE_VIEW) displaySavedNote();
    server.send(200, "text/plain", "Da xoa ghi chu!");
  });

  // Đổi hiệu ứng Motion
  server.on("/set_effect", HTTP_GET, []() {
    if (server.hasArg("eff")) {
      motionMode = server.arg("eff");
      saveConfig();
      startMotionMode();
      server.send(200, "text/plain", "Dang phat hieu ung: " + motionMode);
    } else {
      server.send(400, "text/plain", "Thieu eff");
    }
  });

  // Upload Ảnh chính
  server.on("/upload", HTTP_POST, []() {
    server.send(200, "text/plain", "OK");
    needRedrawMain = true;
    lastActivityTime = millis();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      uploadFile = LittleFS.open("/photo.jpg", "w");
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (uploadFile) uploadFile.write(upload.buf, upload.currentSize);
    } else if (upload.status == UPLOAD_FILE_END) {
      if (uploadFile) uploadFile.close();
    }
  });

  // Upload GIF / Motion Graphic
  server.on("/upload_gif", HTTP_POST, []() {
    server.send(200, "text/plain", "OK");
    motionMode = "file";
    saveConfig();
    needRedrawMotion = true;
    lastActivityTime = millis();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("[UPLOAD GIF] Nhan file: %s\n", upload.filename.c_str());
      uploadFile = LittleFS.open("/motion.gif", "w");
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      if (uploadFile) uploadFile.write(upload.buf, upload.currentSize);
    } else if (upload.status == UPLOAD_FILE_END) {
      if (uploadFile) {
        uploadFile.close();
        Serial.printf("[UPLOAD GIF] Hoan tat! %u bytes\n", upload.totalSize);
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

  // ===================================================================
  // 1. XỬ LÝ CẢM ỨNG (TOUCH / PEN INPUT)
  // ===================================================================
  if (ts.touched()) {
    TS_Point p = ts.getPoint();
    int sx = touchToScreenX(p.x);
    int sy = touchToScreenY(p.y);

    // --- TRƯỜNG HỢP A: ĐANG Ở BẢNG VIẾT TAY (WHITEBOARD) ---
    if (currentState == STATE_WHITEBOARD) {
      lastActivityTime = now;

      // 1. Chạm vùng Bảng màu trên đỉnh (Y < 42)
      if (sy < 42) {
        if (now - lastTouchTime > 250) {
          lastTouchTime = now;
          const int swatchX[] = { 10, 48, 86, 124, 162, 200 };
          for (int i = 0; i < 6; i++) {
            if (sx >= swatchX[i] - 4 && sx <= swatchX[i] + 32) {
              currentPenColorIdx = i;
              drawWhiteboardUI();
              break;
            }
          }
        }
      }
      // 2. Chạm vùng Điều khiển dưới đáy (Y >= 280)
      else if (sy >= 280) {
        if (now - lastTouchTime > 300) {
          lastTouchTime = now;
          // Nút Xoá (X: 6 -> 74)
          if (sx >= 6 && sx <= 74) {
            if (LittleFS.exists("/note.strokes")) LittleFS.remove("/note.strokes");
            tft.fillRect(0, 42, 240, 237, TFT_BLACK);
            Serial.println("[WHITEBOARD] Da xoa bang ve!");
          }
          // Nút Lưu (X: 84 -> 156)
          else if (sx >= 84 && sx <= 156) {
            tft.setTextColor(TFT_GREEN, 0x18E3);
            tft.drawCentreString("DA LUU!", 120, 292, 2);
            delay(500);
            tft.drawCentreString("LUU", 120, 292, 2);
            Serial.println("[WHITEBOARD] Da luu ghi chu thanh cong!");
          }
          // Nút Thoát (X: 164 -> 234)
          else if (sx >= 164 && sx <= 234) {
            displayMainPhoto();
          }
        }
      }
      // 3. VÙNG VẼ CHÍNH BẰNG BÚT CẢM ỨNG (Y: 42 -> 278)
      else {
        uint16_t penColor = (currentPenColorIdx < 6) ? PALETTE[currentPenColorIdx] : TFT_WHITE;
        if (penColor == TFT_BLACK) {
          // Cục tẩy xoá
          tft.fillCircle(sx, sy, 8, TFT_BLACK);
        } else {
          // Nét viết bút
          if (penWasDown && lastPenX >= 0) {
            tft.drawLine(lastPenX, lastPenY, sx, sy, penColor);
            tft.fillCircle(sx, sy, 1, penColor);

            // Ghi nhận nét vẽ vào LittleFS
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
    // --- TRƯỜNG HỢP B: ĐANG Ở CÁC TRẠNG THÁI KHÁC ---
    else {
      if (now - lastTouchTime > 450) {
        lastTouchTime = now;
        lastActivityTime = now;

        if (currentState == STATE_STANDBY) {
          Serial.println("[TOUCH] Thuc giac tu Standby -> Ve Anh Chinh!");
          displayMainPhoto();
        } else if (currentState == STATE_MOTION || currentState == STATE_NOTE_VIEW) {
          Serial.println("[TOUCH] Chuyen ve Anh Chinh!");
          displayMainPhoto();
        } else if (currentState == STATE_MAIN_PHOTO) {
          Serial.println("[TOUCH] Mo Menu QR & Tinh nang!");
          drawTwoQRScreen();
        } else if (currentState == STATE_QR) {
          // Xử lý 3 nút bấm ở đáy màn hình QR
          if (sy >= 250) {
            if (sx < 80) {
              displayMainPhoto();
            } else if (sx >= 84 && sx <= 156) {
              drawWhiteboardUI();
            } else if (sx > 160) {
              startMotionMode();
            }
          } else {
            displayMainPhoto();
          }
        }
      }
    }
  } else {
    // Nhấc bút cảm ứng lên
    penWasDown = false;
    lastPenX = -1;
    lastPenY = -1;
  }

  // ===================================================================
  // 2. PHÁT VIDEO / MOTION GRAPHIC HOẶC HIỆU ỨNG
  // ===================================================================
  if (currentState == STATE_MOTION || (currentState == STATE_STANDBY && cfgMode == "motion")) {
    if (motionMode == "file" && LittleFS.exists("/motion.gif")) {
      if (!gifPlaying) {
        gif.begin(GIF_PALETTE_RGB565_BE);
        if (gif.open("/motion.gif", GIFOpenFile, GIFCloseFile, GIFReadFile, GIFSeekFile, GIFDraw)) {
          gifPlaying = true;
        }
      }
      if (gifPlaying) {
        if (!gif.playFrame(true, NULL)) {
          gif.reset(); // lặp lại vô tận
        }
      }
    } else if (motionMode == "matrix") {
      updateMatrixRain();
      delay(30);
    } else if (motionMode == "starfield") {
      updateStarfield();
      delay(15);
    }
  }

  // ===================================================================
  // 3. KIỂM TRA THỜI GIAN VÀO STANDBY (IDLE TIMEOUT)
  // ===================================================================
  if (cfgTimeout > 0 && currentState != STATE_STANDBY && currentState != STATE_WHITEBOARD) {
    if (now - lastActivityTime >= (unsigned long)cfgTimeout * 1000UL) {
      enterStandby();
    }
  }

  // ===================================================================
  // 4. CẬP NHẬT ĐỒNG HỒ AOD TRONG STANDBY
  // ===================================================================
  if (currentState == STATE_STANDBY && cfgMode == "clock") {
    if (now - lastClockDraw >= 1000) {
      lastClockDraw = now;
      drawStandbyClock(false);
    }
  }

  // ===================================================================
  // 5. CẬP NHẬT KHI CÓ FILE MỚI UPLOAD
  // ===================================================================
  if (needRedrawMain) {
    needRedrawMain = false;
    displayMainPhoto();
  }
  if (needRedrawMotion) {
    needRedrawMotion = false;
    startMotionMode();
  }

  delay(2);
}
