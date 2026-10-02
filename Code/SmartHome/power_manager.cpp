#include "power_manager.h"
#include "output_control.h"
#include "display.h"
#include "config.h"

// ============================================================
//  电源管理模块 — 实现 (简洁消抖版)
//
//  设计原则:
//    中断仅用于唤醒, 主循环直接读引脚电平来判断"按住"/"松开".
//    主循环每秒数千次轮询, 天然消抖, 不依赖双边沿捕获.
//    长按 >= 3s 立即关机, 无论是否松开按键.
// ============================================================

// --- 中断唤醒标志 ---
static volatile bool    btn_wakeup      = false;

// --- 主循环状态 (无需 volatile, 仅主循环访问) ---
static unsigned long    press_start_ms  = 0;     // 按下时刻; 0 = 当前未按住
static bool             shutdown_done   = false;

// ============================================================
//  GPIO13 中断: 仅设唤醒标志, 不做任何逻辑判断
// ============================================================
static void ICACHE_RAM_ATTR power_button_isr() {
    btn_wakeup = true;
}

// ============================================================
//  ★ 紧急锁存 — setup() 第一行, 无任何依赖 ★
// ============================================================
void power_latch() {
    pinMode(PIN_POWER_HOLD, OUTPUT);
    digitalWrite(PIN_POWER_HOLD, HIGH);
}

// ============================================================
//  初始化 (在 Serial.begin() 之后调用)
// ============================================================
void power_init() {
    // 按键引脚
    pinMode(PIN_POWER_BUTTON, INPUT_PULLUP);

    // 中断 — 仅 FALLING, 减少触发次数
    attachInterrupt(
        digitalPinToInterrupt(PIN_POWER_BUTTON),
        power_button_isr,
        FALLING
    );

    Serial.println(F("[POWER] GPIO13 interrupt armed, power stable"));
}

// ============================================================
//  主循环: 读引脚电平 → 简单计时 → 超时关机
// ============================================================
bool power_check_shutdown() {
    if (shutdown_done) {
        return true;
    }

    // 没有被唤醒且没有正在计时 → 快速返回
    if (!btn_wakeup && press_start_ms == 0) {
        return false;
    }
    btn_wakeup = false;

    // 直接读引脚真实电平 (INPUT_PULLUP: 按下=LOW, 松开=HIGH)
    bool button_down = (digitalRead(PIN_POWER_BUTTON) == LOW);

    if (button_down) {
        // 按键正在按住
        if (press_start_ms == 0) {
            press_start_ms = millis();          // 开始计时
        } else if (millis() - press_start_ms >= POWER_LONG_PRESS_MS) {
            power_off();                         // 超时 → 关机
            shutdown_done = true;
            return true;
        }
    } else {
        // 按键已松开 → 重置计时
        press_start_ms = 0;
    }

    return false;
}

// ============================================================
//  执行关机
// ============================================================
void power_off() {
    // 1. 关闭外设
    ir_off();
    relay_off();

    // 2. OLED 显示关机提示
    display_show_shutdown();
    delay(200);  // 给用户看清的时间

    // 3. 刷出串口缓冲
    Serial.println(F("[POWER] System OFF"));
    Serial.flush();
    delay(100);

    // 4. ★ 拉低 GPIO12 → 关闭 MOSFET → 系统断电 ★
    digitalWrite(PIN_POWER_HOLD, LOW);

    // 如果因某种原因没有断电, 死循环等待
    while (true) {
        delay(1000);
    }
}
