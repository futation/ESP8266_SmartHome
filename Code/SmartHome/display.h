#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>

/**
 * OLED 显示模块 — SSD1306 128x64 I2C
 *
 * 引脚: SCL=GPIO0, SDA=GPIO2
 * 地址: 0x3C
 */

/** 初始化 OLED (I2C + SSD1306) */
void display_init();

/** 清屏 */
void display_clear();

/** 刷新显示 (将缓冲区写入屏幕) */
void display_update();

/** 在第 row 行显示文本 (0~7 行, 每行 8px 高) */
void display_text(uint8_t row, const char* text);

/** 格式化显示: 在第 row 行显示 "标签: 数值" */
void display_value(uint8_t row, const char* label, int value, const char* unit);

/** 显示完整状态页 */
void display_show_status(int temp, int humi, int battery_pct, bool pir,
                         bool relay, bool ir,
                         const char* wifi_str, const char* time_str);

/** 显示遥控模式界面 (AP QR code + 状态行) */
void display_show_remote(const char* ssid, const char* ip,
                         const char* status_line);

/** 显示关机提示界面 */
void display_show_shutdown();

#endif
