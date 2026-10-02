#include "button_handler.h"
#include "config.h"

// ============================================================
//  按键模式检测器 — 实现
// ============================================================

// ---- 状态枚举 ----
enum BtnState {
    STATE_IDLE,          // 等待按下
    STATE_PRESSED,       // 正在按住, 计时中
    STATE_WAIT_DOUBLE,   // 短按释放后, 等待可能的第二击 (600ms 窗口)
    STATE_WAIT_RELEASE,  // 双击/长按已触发, 等释放后回 IDLE (不产生单击)
};

static BtnState      state            = STATE_IDLE;
static unsigned long press_time       = 0;   // 按下时刻
static unsigned long release_time     = 0;   // 释放时刻 (用于双击窗口)

// 双击窗口: 第一次释放后, 在此时间内再次按下视为双击
static const unsigned long DOUBLE_GAP_MS = 650;

// 消抖: 引脚电平需稳定此时间才认为有效
static unsigned long  debounce_time    = 0;
static bool           last_raw         = false;  // 上一轮原始电平

// ---- 注册的回调 ----
static ButtonCallback cb_click        = nullptr;
static ButtonCallback cb_double_click = nullptr;
static ButtonCallback cb_long_press   = nullptr;

// ============================================================
//  初始化
// ============================================================
void button_init() {
    pinMode(PIN_USER_BUTTON, INPUT);  // 外部下拉, 按下=HIGH
    state = STATE_IDLE;
}

// ============================================================
//  注册回调
// ============================================================
void button_on_click(ButtonCallback cb)         { cb_click = cb; }
void button_on_double_click(ButtonCallback cb)  { cb_double_click = cb; }
void button_on_long_press(ButtonCallback cb)    { cb_long_press = cb; }

// ============================================================
//  主状态机 (含消抖 + 双击修复)
// ============================================================
void button_update() {
    bool raw = (digitalRead(PIN_USER_BUTTON) == HIGH);
    unsigned long now = millis();

    // ---- 消抖: 电平变化时重置计时, 稳定后才接受 ----
    if (raw != last_raw) {
        debounce_time = now;
        last_raw = raw;
        return;  // 本次跳过, 等下次稳定再处理
    }
    if (now - debounce_time < BUTTON_DEBOUNCE_MS) {
        return;  // 还不够稳定
    }
    // 至此, 'raw' 是消抖后的可靠电平
    bool down = raw;

    switch (state) {

    // ========================================================
    // IDLE: 等待按下
    // ========================================================
    case STATE_IDLE:
        if (down) {
            state = STATE_PRESSED;
            press_time = now;
        }
        break;

    // ========================================================
    // PRESSED: 正在按住
    // ========================================================
    case STATE_PRESSED:
        if (!down) {
            // 释放 → 进入双击等待窗口
            release_time = now;
            state = STATE_WAIT_DOUBLE;
        } else if (now - press_time >= POWER_LONG_PRESS_MS) {
            // 长按 ≥ 3s → 立即触发, 等释放
            state = STATE_WAIT_RELEASE;
            // Serial.println(F("[BUTTON] >>> LONG PRESS detected <<<"));
            if (cb_long_press) cb_long_press();
        }
        break;

    // ========================================================
    // WAIT_DOUBLE: 等待第二击 (600ms 窗口)
    // ========================================================
    case STATE_WAIT_DOUBLE:
        if (down) {
            // 窗口期内再次按下 → 双击 → 进 WAIT_RELEASE 等释放
            state = STATE_WAIT_RELEASE;
            // Serial.println(F("[BUTTON] >>> DOUBLE CLICK detected <<<"));
            if (cb_double_click) cb_double_click();
        } else if (now - release_time > DOUBLE_GAP_MS) {
            // 超时无第二击 → 单击
            state = STATE_IDLE;
            // Serial.println(F("[BUTTON] Single click"));
            if (cb_click) cb_click();
        }
        break;

    // ========================================================
    // WAIT_RELEASE: 双击/长按已触发, 等释放回 IDLE
    // ========================================================
    case STATE_WAIT_RELEASE:
        if (!down) {
            state = STATE_IDLE;
        }
        break;
    }
}
