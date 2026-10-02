#include "sensor_manager.h"
#include "config.h"
#include <DHT11.h>

// ============================================================
//  传感器管理模块 — 实现
//  DHT11 + PIR + ADC 电池
// ============================================================

static DHT11 dht(PIN_DHT11);

// ---- 定时器 ----
static unsigned long last_slow_ms = 0;   // DHT11 + ADC 共用 2s 定时

// ---- 温湿度 ----
static int  current_temp = 0;
static int  current_humi = 0;

// ---- PIR 人体感应 ----
static bool pir_last    = false;  // 上一轮电平
static bool pir_current = false;  // 当前是否有人

// ---- 电池 ----
static int  battery_mv  = 0;
static int  battery_pct = 0;

// ============================================================
//  初始化
// ============================================================
void sensor_init() {
    // DHT11
    dht.setDelay(1000);

    // PIR
    pinMode(PIN_PIR, INPUT);
    pir_last    = false;
    pir_current = false;

    // ADC (ESP8266 内部, 无需 pinMode)
    // 首次读取占位
    battery_mv  = 0;
    battery_pct = 0;

    Serial.println(F("[SENSOR] DHT11(GPIO14) + PIR(GPIO4) + ADC(A0) ready"));
}

// ============================================================
//  主更新入口
// ============================================================
void sensor_update() {
    // ---- PIR: 每个循环都轮询 (高频, 边沿触发) ----
    bool pir_now = (digitalRead(PIN_PIR) == HIGH);

    if (pir_now && !pir_last) {
        // 上升沿: 有人出现
        Serial.println(F("[SENSOR] PIR >>> Motion DETECTED <<<"));
    } else if (!pir_now && pir_last) {
        // 下降沿: 人离开
        Serial.println(F("[SENSOR] PIR --- Motion CLEARED ---"));
    }
    pir_last    = pir_now;
    pir_current = pir_now;

    // ---- DHT11 + ADC: 每 2 秒一次 ----
    unsigned long now = millis();
    if (now - last_slow_ms < SENSOR_READ_MS) {
        return;
    }
    last_slow_ms = now;

    // --- DHT11 ---
    int temp = 0, humi = 0;
    int result = dht.readTemperatureHumidity(temp, humi);

    if (result == 0) {
        current_temp = temp;
        current_humi = humi;
    }
    // 失败时保留上一次有效值, 不打印刷屏
    // (如需调试可取消下面注释)
    // else { Serial.print(F("[SENSOR] DHT11 err: ")); Serial.println(DHT11::getErrorString(result)); }

    // --- ADC 电池 ---
    int adc_raw = analogRead(PIN_BAT_ADC);
    float v_adc = adc_raw * ADC_REF_VOLTAGE / ADC_MAX_VALUE;   // ADC 引脚电压
    float v_bat = v_adc / BAT_DIVIDER_RATIO;                   // 反推电池电压
    battery_mv  = (int)(v_bat * 1000);

    // 线性映射 3.2V~4.2V → 0%~100%
    battery_pct = (int)((v_bat * 1000 - BAT_EMPTY_MV) * 100 / (BAT_FULL_MV - BAT_EMPTY_MV));
    if (battery_pct < 0)   battery_pct = 0;
    if (battery_pct > 100) battery_pct = 100;

    // 串口合并输出
    Serial.print(F("[SENSOR] Temp: "));
    Serial.print(current_temp);
    Serial.print(F("°C | Humi: "));
    Serial.print(current_humi);
    Serial.print(F("% | Bat: "));
    Serial.print(battery_mv);
    Serial.print(F("mV ("));
    Serial.print(battery_pct);
    Serial.print(F("%) | PIR: "));
    Serial.println(pir_current ? F("YES") : F("no"));
}

// ============================================================
//  访问器
// ============================================================
int  sensor_get_temp()        { return current_temp; }
int  sensor_get_humi()        { return current_humi; }
bool sensor_pir_triggered()   { return pir_current; }
int  sensor_get_battery_pct() { return battery_pct; }
int  sensor_get_battery_mv()  { return battery_mv; }

