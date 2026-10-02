#ifndef OUTPUT_CONTROL_H
#define OUTPUT_CONTROL_H

#include <Arduino.h>

/**
 * 输出控制模块 — 继电器 + 红外发射管
 *
 * 两个设备均为 GPIO 拉高时打开.
 * 开机默认: 两者均打开 (HIGH).
 */

/** 初始化输出引脚, 默认全部打开 */
void output_init();

// ---- 继电器 ----
void relay_on();
void relay_off();
bool relay_is_on();

// ---- 红外发射管 ----
void ir_on();
void ir_off();
bool ir_is_on();

#endif
