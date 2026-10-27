/*
 * =====================================================================
 * BÀI 1: KIỂM TRA MÀN HÌNH CƠ BẢN (TEST MÀN HÌNH CYD 2.8")
 * ĐỊA CHỈ: D:\ESP32\01_Test_Man_Hinh\01_Test_Man_Hinh.ino
 * =====================================================================
 * Mục tiêu:
 * - Kiểm tra bật đèn nền màn hình (GPIO 21).
 * - Kiểm tra hiển thị 3 màu cơ bản: Đỏ (Red), Xanh lá (Green), Xanh dương (Blue).
 * - Kiểm tra hiển thị chữ và bộ đếm thời gian.
 * =====================================================================
 */

#include <Arduino.h>
#include <TFT_eSPI.h>

TFT_eSPI tft = TFT_eSPI();

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n=== BẮT ĐẦU TEST MÀN HÌNH CYD 2.8 ===");

  // 1. Bật đèn nền màn hình (Backlight chân GPIO 21)
  pinMode(21, OUTPUT);
  digitalWrite(21, HIGH);

  // 2. Khởi tạo màn hình
  tft.init();
  tft.setRotation(0);      // Dọc 240x320
  tft.invertDisplay(true); // Khử lỗi âm bản màu sắc chuẩn 100%

  // 3. Test màu ĐỎ
  Serial.println("Kiem tra mau DO (RED)...");
  tft.fillScreen(TFT_RED);
  tft.setTextColor(TFT_WHITE, TFT_RED);
  tft.drawCentreString("MAU DO (RED)", 120, 150, 4);
  delay(1200);

  // 4. Test màu XANH LÁ
  Serial.println("Kiem tra mau XANH LA (GREEN)...");
  tft.fillScreen(TFT_GREEN);
  tft.setTextColor(TFT_BLACK, TFT_GREEN);
  tft.drawCentreString("XANH LA (GREEN)", 120, 150, 4);
  delay(1200);

  // 5. Test màu XANH DƯƠNG
  Serial.println("Kiem tra mau XANH DUONG (BLUE)...");
  tft.fillScreen(TFT_BLUE);
  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  tft.drawCentreString("XANH DUONG (BLUE)", 120, 150, 4);
  delay(1200);

  // 6. Giao diện tĩnh sau khi test xong màu
  tft.fillScreen(TFT_BLACK);
  
  // Khung viền ngoài
  tft.drawRect(4, 4, 232, 312, TFT_WHITE);
  tft.drawRect(6, 6, 228, 308, TFT_CYAN);

  // Tiêu đề
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawCentreString("ESP32 CYD 2.8", 120, 30, 4);

  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawCentreString("MAN HINH HOAT DONG TOT!", 120, 80, 2);

  // Dải màu minh họa
  tft.fillRect(20, 120, 60, 40, TFT_RED);
  tft.fillRect(90, 120, 60, 40, TFT_GREEN);
  tft.fillRect(160, 120, 60, 40, TFT_BLUE);

  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString("RED     GREEN     BLUE", 120, 170, 2);

  tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
  tft.drawCentreString("Thoi gian chay:", 120, 220, 2);
}

int seconds = 0;

void loop() {
  // Cập nhật số giây chạy trên màn hình
  char buffer[32];
  snprintf(buffer, sizeof(buffer), "   %d giay   ", seconds);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawCentreString(buffer, 120, 250, 4);
  
  seconds++;
  delay(1000);
}
