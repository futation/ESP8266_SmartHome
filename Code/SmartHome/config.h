#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
//  SmartHome - 全局配置文件
//  所有引脚定义、常量、WiFi 凭证集中管理
// ============================================================

// ============ 引脚定义 (GPIO 编号) ============

// --- 电源管理 ---
#define PIN_POWER_HOLD      12    // GPIO12 - 掌管电源 (HIGH=保持开机, LOW=关机)
#define PIN_POWER_BUTTON    13    // GPIO13 - 电源按键检测 (按下=LOW, 常态=HIGH)

// --- 传感器 ---
#define PIN_DHT11           14    // GPIO14 - DHT11 温湿度传感器
#define PIN_PIR             4     // GPIO4  - 人体感应模块输入
#define PIN_BAT_ADC         A0    // ADC    - 电池电压检测 (TOUT)

// --- 输出控制 ---
#define PIN_IR_LED          5     // GPIO5  - 红外发射管
#define PIN_RELAY           16    // GPIO16 - 继电器

// --- 按键 ---
#define PIN_USER_BUTTON     15    // GPIO15 - 用户按键 (默认下拉, 按下=HIGH)

// --- OLED (选做, 阶段4启用) ---
#define PIN_OLED_SCL        0     // GPIO0  - OLED I2C 时钟
#define PIN_OLED_SDA        2     // GPIO2  - OLED I2C 数据

// ============ 时序常量 (毫秒) ============

#define POWER_LONG_PRESS_MS 3000  // 长按关机阈值: 3 秒
#define POWER_SHORT_PRESS_MS 50   // 短按消抖: 50ms (低于此视为抖动)
#define BUTTON_DEBOUNCE_MS  50    // 通用按键消抖
#define SENSOR_READ_MS      2000  // 传感器读取间隔: 2 秒 (DHT11 最快 1Hz)
#define WIFI_CHECK_MS       5000  // WiFi 状态检查间隔: 5 秒

// ============ 串口 ============

#define SERIAL_BAUD         115200

// ============ WiFi 配置 (阶段3启用) ============

#define WIFI_AP_SSID        "ESP_CTRL"
#define WIFI_AP_PASSWORD    "12345678"            // AP 热点密码 (至少 8 位, 短于 8 位会降级为开放网络)
#define WIFI_STA_SSID       "YourWiFiSSID"        // TODO: 改成你的路由器 WiFi 名称
#define WIFI_STA_PASSWORD   "YourWiFiPassword"    // TODO: 改成你的路由器 WiFi 密码

#define WIFI_CONNECT_TIMEOUT_MS  15000  // WiFi 连接超时 15 秒
#define NTP_SYNC_TIMEOUT_MS      10000  // NTP 同步超时 10 秒
#define NTP_TZ_OFFSET_SEC        (8 * 3600)  // 北京时区 UTC+8

// ============ 电池参数 (阶段2启用) ============
// 分压网络: BAT+ → R30(360k) → NODE → R9(100k) → GND
//                   NODE → R8(1k) → ADC_PIN
// V_ADC ≈ Vbat × R9 / (R30 + R9) = Vbat × 0.2174

#define BAT_DIVIDER_RATIO   0.2174f   // 100k / (360k + 100k)
#define ADC_MAX_VALUE       1024.0f   // 10-bit ADC
#define ADC_REF_VOLTAGE     1.0f      // ESP8266 ADC 量程 0~1V
#define BAT_FULL_MV         4200      // 满电 4.2V
#define BAT_EMPTY_MV        3200      // 低电 3.2V (建议关机)

#endif
