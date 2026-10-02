#include "output_control.h"
#include "config.h"

// ============================================================
//  输出控制模块 — 实现
// ============================================================

static bool relay_state = false;
static bool ir_state    = false;

// ============================================================
//  初始化: 配置引脚, 默认打开
// ============================================================
void output_init() {
    pinMode(PIN_RELAY, OUTPUT);
    pinMode(PIN_IR_LED, OUTPUT);

    // 默认上电全部打开
    relay_on();
    ir_on();
}

// ============================================================
//  继电器
// ============================================================
void relay_on() {
    digitalWrite(PIN_RELAY, HIGH);
    relay_state = true;
}

void relay_off() {
    digitalWrite(PIN_RELAY, LOW);
    relay_state = false;
}

bool relay_is_on() {
    return relay_state;
}

// ============================================================
//  红外发射管
// ============================================================
void ir_on() {
    digitalWrite(PIN_IR_LED, HIGH);
    ir_state = true;
}

void ir_off() {
    digitalWrite(PIN_IR_LED, LOW);
    ir_state = false;
}

bool ir_is_on() {
    return ir_state;
}
