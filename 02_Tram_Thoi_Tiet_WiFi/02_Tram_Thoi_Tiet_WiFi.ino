/*
 * =====================================================================
 * DỰ ÁN: TRẠM THỜI TIẾT THỜI GIAN THỰC CHO ESP32-2432S028 (CYD 2.8")
 * ĐỊA CHỈ: D:\ESP32\02_Tram_Thoi_Tiet_WiFi\02_Tram_Thoi_Tiet_WiFi.ino
 * =====================================================================
 * Tính năng:
 * 1. Reset sạch cấu hình WiFi cũ, kết nối WiFi: EverGrowth / Evergrowth!2025
 * 2. Tự động nhận diện Địa điểm thực tế qua IP (ip-api.com)
 * 3. Đồng bộ Giờ thực tế qua Internet NTP (GMT+7)
 * 4. Lấy dữ liệu Thời tiết: Nhiệt độ, Độ ẩm, Lưu lượng mưa (Open-Meteo)
 * 5. Giao diện trực quan, mượt mà, không nháy màn hình.
 * =====================================================================
 */

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>
#include <TFT_eSPI.h>

// --- CẤU HÌNH WIFI ---
const char* WIFI_SSID     = "EverGrowth";
const char* WIFI_PASSWORD = "Evergrowth!2025";

// --- MÀN HÌNH ---
#define TFT_BL 21
TFT_eSPI tft = TFT_eSPI();

// Màu sắc giao diện (RGB565)
#define COLOR_BG        0x0000 // Đen
#define COLOR_CARD_BG   0x10A2 // Xanh xám đậm
#define COLOR_BORDER    0x3186 // Xám viền
#define COLOR_ACCENT    0x07E0 // Xanh lá
#define COLOR_TEMP      0xFD20 // Cam đỏ (Nhiệt độ)
#define COLOR_HUMID     0x07FF // Xanh ngọc (Độ ẩm)
#define COLOR_RAIN      0x5DDF // Xanh biển (Mưa)
#define COLOR_TEXT_DIM  0xBDF7 // Xám nhạt

// --- BIẾN TOÀN CỤC ---
String cityName = "Hanoi";
String countryName = "VN";
float latitude = 21.0285;
float longitude = 105.8542;

float temperature = 0.0;
int humidity = 0;
float rainAmount = 0.0;
String weatherDesc = "Dang tai...";
bool weatherLoaded = false;

unsigned long lastWeatherUpdate = 0;
const unsigned long WEATHER_INTERVAL = 15 * 60 * 1000; // Cập nhật mỗi 15 phút

int lastSecond = -1;
int lastMinute = -1;

// Hàm chuyển mã thời tiết WMO sang tiếng Việt không dấu
String getWeatherText(int code) {
  switch (code) {
    case 0: return "Troi quang dang";
    case 1: return "It may, nang";
    case 2: return "May rải rac";
    case 3: return "Troi nhieu may";
    case 45: case 48: return "Co suong mu";
    case 51: case 53: case 55: return "Co mua phuon";
    case 61: return "Mua nho";
    case 63: return "Mua vua";
    case 65: return "Mua to";
    case 80: case 81: case 82: return "Co mua rao";
    case 95: case 96: case 99: return "Mua dong set";
    default: return "Troi mat";
  }
}

// Lấy thông tin vị trí thực tế qua IP
void fetchLocationByIP() {
  if (WiFi.status() != WL_CONNECTED) return;

  Serial.println("[GEO] Dang lay toa do vi tri thuc...");
  HTTPClient http;
  http.begin("http://ip-api.com/json/");
  http.setTimeout(5000);
  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (!error && doc["status"] == "success") {
      cityName = doc["city"].as<String>();
      countryName = doc["countryCode"].as<String>();
      latitude = doc["lat"].as<float>();
      longitude = doc["lon"].as<float>();
      Serial.printf("[GEO] Dia diem: %s, %s (Lat: %.4f, Lon: %.4f)\n", 
                    cityName.c_str(), countryName.c_str(), latitude, longitude);
    }
  } else {
    Serial.printf("[GEO] Khong the lay vi tri (HTTP %d). Dung mac dinh Hanoi.\n", httpCode);
  }
  http.end();
}

// Lấy dữ liệu thời tiết & lưu lượng mưa từ Open-Meteo
void fetchWeatherData() {
  if (WiFi.status() != WL_CONNECTED) return;

  Serial.println("[WEATHER] Dang tai du lieu thoi tiet...");
  HTTPClient http;
  
  char url[256];
  snprintf(url, sizeof(url),
           "http://api.open-meteo.com/v1/forecast?latitude=%.4f&longitude=%.4f&current=temperature_2m,relative_humidity_2m,rain,weather_code&timezone=Asia%%2FBangkok",
           latitude, longitude);

  http.begin(url);
  http.setTimeout(6000);
  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    if (!error && doc.containsKey("current")) {
      temperature = doc["current"]["temperature_2m"].as<float>();
      humidity = doc["current"]["relative_humidity_2m"].as<int>();
      rainAmount = doc["current"]["rain"].as<float>();
      int wCode = doc["current"]["weather_code"].as<int>();
      weatherDesc = getWeatherText(wCode);

      weatherLoaded = true;
      Serial.printf("[WEATHER] Nhiet do: %.1f C, Do am: %d %%, Mua: %.1f mm, TT: %s\n",
                    temperature, humidity, rainAmount, weatherDesc.c_str());
    }
  } else {
    Serial.printf("[WEATHER] Loi tai thoi tiet: HTTP %d\n", httpCode);
  }
  http.end();
}

// Vẽ khung giao diện tĩnh
void drawStaticUI() {
  tft.fillScreen(COLOR_BG);

  // 1. THANH TIÊU ĐỀ TRÊN CÙNG (Y: 0 -> 32)
  tft.fillRect(0, 0, 240, 32, 0x0841);
  tft.drawFastHLine(0, 32, 240, COLOR_BORDER);

  // Tên địa điểm
  tft.setTextColor(TFT_WHITE, 0x0841);
  char locStr[32];
  snprintf(locStr, sizeof(locStr), "LOC: %s, %s", cityName.c_str(), countryName.c_str());
  tft.drawString(locStr, 8, 8, 2);

  // Trạng thái WiFi
  tft.setTextColor(COLOR_ACCENT, 0x0841);
  tft.drawString("WiFi:OK", 182, 8, 2);

  // 2. KHUNG ĐỒNG HỒ & NGÀY THÁNG (Y: 38 -> 118)
  tft.fillRoundRect(6, 38, 228, 78, 6, COLOR_CARD_BG);
  tft.drawRoundRect(6, 38, 228, 78, 6, COLOR_BORDER);

  // 3. CÁC Ô CHỈ SỐ THỜI TIẾT (Y: 122 -> 255)
  // Ô 1: Nhiệt độ (bên trái)
  tft.fillRoundRect(6, 122, 111, 62, 6, COLOR_CARD_BG);
  tft.drawRoundRect(6, 122, 111, 62, 6, COLOR_TEMP);
  tft.setTextColor(COLOR_TEMP, COLOR_CARD_BG);
  tft.drawCentreString("NHIET DO", 61, 126, 2);

  // Ô 2: Độ ẩm (bên phải)
  tft.fillRoundRect(123, 122, 111, 62, 6, COLOR_CARD_BG);
  tft.drawRoundRect(123, 122, 111, 62, 6, COLOR_HUMID);
  tft.setTextColor(COLOR_HUMID, COLOR_CARD_BG);
  tft.drawCentreString("DO AM", 178, 126, 2);

  // Ô 3: Lưu lượng mưa & Tình trạng (băng ngang bên dưới)
  tft.fillRoundRect(6, 190, 228, 65, 6, COLOR_CARD_BG);
  tft.drawRoundRect(6, 190, 228, 65, 6, COLOR_RAIN);
  tft.setTextColor(COLOR_RAIN, COLOR_CARD_BG);
  tft.drawString("LUONG MUA:", 14, 196, 2);

  // 4. THANH TRẠNG THÁI DƯỚI CÙNG (Y: 262 -> 318)
  tft.drawFastHLine(6, 260, 228, COLOR_BORDER);
}

// Cập nhật giá trị thời tiết trên màn hình
void updateWeatherUI() {
  if (!weatherLoaded) return;

  // Cập nhật giá trị Nhiệt độ
  char tempStr[16];
  snprintf(tempStr, sizeof(tempStr), "%.1f C", temperature);
  tft.setTextColor(TFT_WHITE, COLOR_CARD_BG);
  tft.drawCentreString(tempStr, 61, 148, 4);

  // Cập nhật giá trị Độ ẩm
  char humStr[16];
  snprintf(humStr, sizeof(humStr), "%d %%", humidity);
  tft.setTextColor(TFT_WHITE, COLOR_CARD_BG);
  tft.drawCentreString(humStr, 178, 148, 4);

  // Cập nhật Lưu lượng mưa
  char rainStr[20];
  if (rainAmount <= 0.01) {
    snprintf(rainStr, sizeof(rainStr), "0.0 mm (Khong mua)");
  } else {
    snprintf(rainStr, sizeof(rainStr), "%.1f mm/h", rainAmount);
  }
  tft.setTextColor(TFT_WHITE, COLOR_CARD_BG);
  tft.drawString(rainStr, 105, 196, 2);

  // Cập nhật mô tả trạng thái thời tiết
  tft.setTextColor(TFT_YELLOW, COLOR_CARD_BG);
  tft.drawCentreString(weatherDesc, 120, 226, 2);

  // Dòng trạng thái đáy
  char bottomStr[48];
  snprintf(bottomStr, sizeof(bottomStr), "IP: %s", WiFi.localIP().toString().c_str());
  tft.setTextColor(COLOR_TEXT_DIM, COLOR_BG);
  tft.drawString(bottomStr, 10, 270, 2);

  tft.setTextColor(0x07E0, COLOR_BG);
  tft.drawString("Cap nhat: Thoi gian thuc OK", 10, 292, 2);
}

// Cập nhật đồng hồ thời gian thực (gọi mỗi giây)
void updateClockUI() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return;
  }

  // Cập nhật giờ:phút:giây
  if (timeinfo.tm_sec != lastSecond) {
    lastSecond = timeinfo.tm_sec;

    char timeStr[16];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", 
             timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
    
    tft.setTextColor(TFT_YELLOW, COLOR_CARD_BG);
    tft.drawCentreString(timeStr, 120, 48, 6); // Font 6: Chữ số to, nét, đẹp
  }

  // Cập nhật Thứ, Ngày/Tháng/Năm
  if (timeinfo.tm_min != lastMinute) {
    lastMinute = timeinfo.tm_min;

    const char* days[] = {"Chu Nhat", "Thu Hai", "Thu Ba", "Thu Tu", "Thu Nam", "Thu Sau", "Thu Bay"};
    char dateStr[40];
    snprintf(dateStr, sizeof(dateStr), "%s, %02d/%02d/%04d",
             days[timeinfo.tm_wday], timeinfo.tm_mday, timeinfo.tm_mon + 1, timeinfo.tm_year + 1900);

    tft.setTextColor(TFT_CYAN, COLOR_CARD_BG);
    tft.drawCentreString(dateStr, 120, 92, 2);
  }
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println("\n=== KHOI DONG TRAM THOI TIET ESP32 CYD 2.8 ===");

  // 1. Bật đèn nền (GPIO 21)
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  // 2. Khởi tạo màn hình
  tft.init();
  tft.setRotation(0);      // Dọc chuẩn 240x320
  tft.invertDisplay(true); // Chuẩn màu panel CYD
  tft.fillScreen(COLOR_BG);

  // Màn hình chào mừng kết nối WiFi
  tft.setTextColor(TFT_YELLOW, COLOR_BG);
  tft.drawCentreString("TRAM THOI TIET CYD", 120, 50, 4);

  tft.setTextColor(TFT_WHITE, COLOR_BG);
  tft.drawCentreString("Reset WiFi cu...", 120, 110, 2);

  // 3. Reset WiFi cũ hoàn toàn
  WiFi.disconnect(true, true);
  delay(500);
  WiFi.mode(WIFI_STA);

  tft.setTextColor(TFT_CYAN, COLOR_BG);
  tft.drawCentreString("Dang ket noi WiFi:", 120, 140, 2);
  tft.setTextColor(COLOR_ACCENT, COLOR_BG);
  tft.drawCentreString(WIFI_SSID, 120, 165, 4);

  Serial.printf("[WIFI] Dang ket noi toi SSID: %s\n", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    tft.drawString(".", 90 + (attempts % 12) * 5, 205, 4);
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WIFI] Ket noi thanh cong!");
    Serial.printf("[WIFI] IP: %s\n", WiFi.localIP().toString().c_str());

    tft.setTextColor(COLOR_ACCENT, COLOR_BG);
    tft.drawCentreString("WiFi DA KET NOI!", 120, 230, 2);
    delay(800);

    // 4. Đồng bộ thời gian NTP (GMT+7: 7 * 3600 = 25200)
    configTime(7 * 3600, 0, "pool.ntp.org", "time.google.com");

    // 5. Lấy vị trí và thời tiết lần đầu
    fetchLocationByIP();
    fetchWeatherData();
    lastWeatherUpdate = millis();

    // 6. Vẽ giao diện chính
    drawStaticUI();
    updateWeatherUI();
    updateClockUI();
  } else {
    Serial.println("\n[WIFI] Ket noi that bai!");
    tft.setTextColor(TFT_RED, COLOR_BG);
    tft.drawCentreString("KET NOI WIFI THAT BAI!", 120, 230, 2);
    tft.setTextColor(TFT_LIGHTGREY, COLOR_BG);
    tft.drawCentreString("Kiem tra lai SSID & Pass", 120, 260, 2);
  }
}

void loop() {
  if (WiFi.status() == WL_CONNECTED) {
    // Cập nhật đồng hồ mỗi giây
    updateClockUI();

    // Cập nhật thời tiết định kỳ (15 phút)
    if (millis() - lastWeatherUpdate >= WEATHER_INTERVAL) {
      lastWeatherUpdate = millis();
      fetchWeatherData();
      updateWeatherUI();
    }
  } else {
    // Nếu mất kết nối, thử kết nối lại
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    delay(3000);
  }

  delay(200);
}
