/**
 * SmartHome.ino — 智能家居控制系统
 *
 * 正常模式: 传感器 + OLED 状态页 + WiFi STA (双击触发)
 * 遥控模式: AP + HTTP Server + 二进制协议 (长按 GPIO15 切换)
 */

#include "config.h"
#include "power_manager.h"
#include "output_control.h"
#include "sensor_manager.h"
#include "button_handler.h"
#include "display.h"
#include "wifi_manager.h"
#include "protocol_handler.h"

// ============================================================
//  系统模式
// ============================================================
enum SystemMode { MODE_NORMAL, MODE_REMOTE };
static SystemMode sys_mode = MODE_NORMAL;

// ============================================================
//  业务回调
// ============================================================

// 单击: 翻转红外发射管
static void on_single_click() {
    if (ir_is_on()) {
        ir_off();
        Serial.println(F("[ACTION] IR OFF"));
    } else {
        ir_on();
        Serial.println(F("[ACTION] IR ON"));
    }
    if (relay_is_on()) {
        relay_off();
        Serial.println(F("[ACTION] relay OFF"));
    } else {
        relay_on();
        Serial.println(F("[ACTION] relay ON"));
    }
}

// 双击: WiFi STA 连接 + NTP
static void on_double_click() {
    Serial.println(F("[ACTION] Double-click: connecting WiFi..."));
    wifi_sta_connect();
}

// 长按: 切换 正常 ↔ 遥控 模式
static void on_long_press() {
    if (sys_mode == MODE_NORMAL) {
        // ---- 进入遥控模式 ----
        Serial.println(F("[MODE] Switching to REMOTE mode"));
        sys_mode = MODE_REMOTE;

        wifi_ap_start();
        protocol_start();

        display_show_remote(WIFI_AP_SSID, wifi_ap_get_ip(),
                            "Waiting for connection");

    } else {
        // ---- 退出遥控模式 ----
        Serial.println(F("[MODE] Switching to NORMAL mode"));
        sys_mode = MODE_NORMAL;

        protocol_stop();
        wifi_ap_stop();  // 保留 STA, 仅关 AP

        // 恢复正常界面 (下次刷新自动)
    }
}

// ============================================================
//  setup()
// ============================================================
void setup() {
    power_latch();

    Serial.begin(SERIAL_BAUD);
    delay(100);

    power_init();
    output_init();
    sensor_init();

    button_init();
    button_on_click(on_single_click);
    button_on_double_click(on_double_click);
    button_on_long_press(on_long_press);

    display_init();
    wifi_init();

    Serial.println(F("[MAIN] === Normal mode ready ==="));
}

// ============================================================
//  loop() — 双模调度
// ============================================================
static unsigned long last_display_ms = 0;

void loop() {
    // ---- 公共任务: 电源 + 按键 (两种模式都需要) ----
    if (power_check_shutdown()) return;
    button_update();

    if (sys_mode == MODE_NORMAL) {
        // ========== 正常模式 ==========
        sensor_update();

        if (millis() - last_display_ms >= 1000) {
            last_display_ms += 1000;

            char time_buf[24];
            wifi_get_time_str(time_buf, sizeof(time_buf));

            display_show_status(
                sensor_get_temp(),
                sensor_get_humi(),
                sensor_get_battery_pct(),
                sensor_pir_triggered(),
                relay_is_on(),
                ir_is_on(),
                wifi_status_str(),
                time_buf
            );
        }

    } else {
        // ========== 遥控模式 ==========
        // 传感器继续后台读取 (供 HTTP API 用)
        sensor_update();

        // HTTP Server 处理客户端请求
        protocol_handle();

        // OLED 更新 (每 500ms 检查客户端连接状态)
        static unsigned long last_remote_ms = 0;
        if (millis() - last_remote_ms >= 500) {
            last_remote_ms = millis();

            const char* status = wifi_ap_is_client_connected()
                ? "Connection successful"
                : "Wait connect...";

            display_show_remote(WIFI_AP_SSID, wifi_ap_get_ip(), status);
        }
    }
}
