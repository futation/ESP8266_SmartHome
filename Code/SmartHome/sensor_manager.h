#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>

/**
 * 传感器管理模块
 *
 *   - DHT11  温湿度 (GPIO14)  — 每 2 秒
 *   - PIR    人体感应 (GPIO4) — 每个循环轮询, 边沿触发打印
 *   - ADC    电池电量 (TOUT)  — 每 2 秒
 */

/** 初始化所有传感器 */
void sensor_init();

/** 主循环调用: 轮询 PIR (高频) + 定时读取 DHT11/ADC (2s) */
void sensor_update();

// ---- 访问器 (供后续 WiFi 上报使用) ----
int   sensor_get_temp();
int   sensor_get_humi();
bool  sensor_pir_triggered();   // 当前是否有人
int   sensor_get_battery_pct(); // 电量百分比 0~100
int   sensor_get_battery_mv();  // 电池电压 mV

#endif
