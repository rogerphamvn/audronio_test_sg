// =========================================================================
// File cấu hình chân màn hình cho bo mạch ESP32-2432S028 (CYD 2.8")
// Dành cho thư viện TFT_eSPI trong Arduino IDE
// Vị trí copy: Documents\Arduino\libraries\TFT_eSPI\User_Setup.h
// =========================================================================

#define USER_SETUP_INFO "ESP32-2432S028_CYD_Setup"

// Driver màn hình
#define ILI9341_2_DRIVER

// Kích thước chuẩn
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// Màu sắc & Đèn nền
#define TFT_RGB_ORDER TFT_RGB
#define TFT_INVERSION_ON
#define TFT_BL   21
#define TFT_BACKLIGHT_ON HIGH

// Chân SPI kết nối ESP32 với màn hình
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST  -1

// Sử dụng cổng phần cứng HSPI trên ESP32
#define USE_HSPI_PORT

// Các bộ font chữ
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define SMOOTH_FONT

// Tần số SPI tối ưu (40MHz ổn định và mượt mà)
#define SPI_FREQUENCY       40000000
#define SPI_READ_FREQUENCY  20000000
