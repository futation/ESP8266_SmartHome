#ifndef POWER_MANAGER_H
#define POWER_MANAGER_H

#include <Arduino.h>

/**
 * 电源管理器 — 掌管系统自动开关机
 *
 * 硬件原理:
 *   短按 SW3 → 硬件短暂上电 → ESP 启动
 *   → power_latch() 立即拉高 GPIO12 → 锁存电源 (必须在 setup() 第一行!)
 *   → power_init() 配置中断等 (在 Serial.begin() 之后)
 *   → 主循环轮询 GPIO13 电平, 持续低电平 >= 3s 即关机
 *   → 拉低 GPIO12 → 系统断电
 */

/** ★ 紧急锁存电源 — setup() 第一行调用, 不得依赖任何外设 ★ */
void power_latch();

/** 初始化中断和状态 (在 Serial.begin() 之后调用) */
void power_init();

/**
 * 在主循环中每次调用, 检查是否需要关机
 * @return true = 已触发关机, 系统即将断电
 */
bool power_check_shutdown();

/** 执行关机: 拉低 GPIO12, 系统断电 */
void power_off();

#endif
