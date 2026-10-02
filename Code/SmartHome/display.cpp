#include "display.h"
#include "config.h"
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include "qrcode.h"

// ============================================================
//  OLED 显示模块 — SSD1306 128x64 I2C
//  SCL=GPIO0  SDA=GPIO2  Addr=0x3C
// ============================================================

static const int SCREEN_WIDTH  = 128;
static const int SCREEN_HEIGHT = 64;
static const int I2C_ADDRESS   = 0x3C;

static Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
static bool oled_ok = false;

// ============================================================
//  初始化
// ============================================================
void display_init() {
    Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
    Wire.setClock(400000);

    if (!oled.begin(SSD1306_SWITCHCAPVCC, I2C_ADDRESS)) {
        Serial.println(F("[DISPLAY] SSD1306 init FAILED"));
        oled_ok = false;
        return;
    }

    oled_ok = true;
    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(SSD1306_WHITE);
    oled.setCursor(0, 0);
    oled.println(F("SmartHome v0.3"));
    oled.println(F("OLED OK!"));
    oled.display();

    Serial.println(F("[DISPLAY] SSD1306 128x64 ready (0x3C)"));
}

// ============================================================
//  基础操作
// ============================================================
void display_clear() {
    if (!oled_ok) return;
    oled.clearDisplay();
}

void display_update() {
    if (!oled_ok) return;
    oled.display();
}

// ============================================================
//  文本显示
// ============================================================
void display_text(uint8_t row, const char* text) {
    if (!oled_ok) return;
    oled.setCursor(0, row * 8);
    oled.print(text);
}

void display_value(uint8_t row, const char* label, int value, const char* unit) {
    if (!oled_ok) return;
    oled.setCursor(0, row * 8);
    oled.print(label);
    oled.print(F(": "));
    oled.print(value);
    oled.print(unit);
}

// ============================================================
//  状态页
// ============================================================
void display_show_status(int temp, int humi, int battery_pct, bool pir,
                         bool relay, bool ir,
                         const char* wifi_str, const char* time_str) {
    if (!oled_ok) return;
    oled.clearDisplay();

    oled.setCursor(0, 0);
    oled.print(F("=== SmartHome ==="));

    oled.setCursor(0, 10);
    oled.print(F("T:"));
    oled.print(temp);
    oled.print(F("C  H:"));
    oled.print(humi);
    oled.print(F("%"));

    oled.setCursor(0, 20);
    oled.print(F("Battery: "));
    oled.print(battery_pct);
    oled.print(F("%"));

    oled.setCursor(0, 30);
    oled.print(F("PR:"));
    oled.print(pir ? F("YES") : F("no"));
    oled.print(F(" IR:"));
    oled.print(ir ? F("ON") : F("OFF"));
    oled.print(F(" Rel:"));
    oled.print(relay ? F("ON") : F("OFF"));

    oled.setCursor(0, 40);
    oled.print(F("WiFi: "));
    oled.print(wifi_str);

    oled.setCursor(0, 50);
    oled.print(time_str);

    oled.display();
}

// ============================================================
//  遥控模式界面 (含 QR 码)
// ============================================================
void display_show_remote(const char* ssid, const char* ip,
                         const char* status_line) {
    if (!oled_ok) return;
    oled.clearDisplay();

    // 标题
    oled.setCursor(0, 0);
    oled.print(F("=== Remote Mode ==="));

    // ---- 生成 QR 码: {"ip":"192.168.4.1","port":80} ----
    // Version 3, ECC_LOW: 29x29 模块, 容纳 33 字节
    static const char* QR_TEXT = "{\"ip\":\"192.168.4.1\",\"port\":80}";
    static const uint8_t QR_VERSION = 3;

    QRCode qrcode;
    uint8_t qrcodeData[qrcode_getBufferSize(QR_VERSION)];
    qrcode_initText(&qrcode, qrcodeData, QR_VERSION, ECC_LOW, QR_TEXT);

    // 居中绘制 (1 像素/模块)
    int qr_x = (SCREEN_WIDTH  - qrcode.size) / 2;   // (128-29)/2 ≈ 49
    int qr_y = 18;  // 紧贴标题下方

    for (uint8_t y = 0; y < qrcode.size; y++) {
        for (uint8_t x = 0; x < qrcode.size; x++) {
            if (qrcode_getModule(&qrcode, x, y)) {
                oled.drawPixel(qr_x + x, qr_y + y, SSD1306_WHITE);
            }
        }
    }

    // 状态行 (QR 底部留 2px 间距)
    oled.setCursor(0, 55);
    oled.print(status_line);

    oled.display();
}

// ============================================================
//  关机界面
// ============================================================
void display_show_shutdown() {
    if (!oled_ok) return;
    oled.clearDisplay();

    oled.setTextSize(2);
    oled.setCursor(10, 8);
    oled.print(F("SHUTDOWN"));

    oled.setTextSize(1);
    oled.setCursor(16, 32);
    oled.print(F("3s reached"));

    oled.setCursor(4, 46);
    oled.print(F("Release to OFF"));

    oled.display();
}

