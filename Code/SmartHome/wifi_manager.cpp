#include "wifi_manager.h"
#include "config.h"
#include <ESP8266WiFi.h>
#include <time.h>

// ============================================================
//  WiFi 管理器 — STA + NTP
// ============================================================

enum WifiState {
    MY_WIFI_OFF,       // 未连接
    MY_WIFI_CONNECTED, // WiFi 已连但无互联网
    MY_WIFI_ONLINE,    // NTP 已同步 = 互联网 OK
};

static WifiState wifi_state = MY_WIFI_OFF;
static bool ntp_configured = false;  // 防 configTime() 重复调用

// ============================================================
//  初始化
// ============================================================
void wifi_init() {
    WiFi.mode(WIFI_STA);  // 仅 STA 模式, 暂不连接
    wifi_state = MY_WIFI_OFF;
    Serial.println(F("[WIFI] STA mode ready (not connected)"));
}

// ============================================================
//  连接 + NTP 同步 (双击触发, 阻塞式)
// ============================================================
bool wifi_sta_connect() {
    Serial.println(F("[WIFI] === Starting connection ==="));

    // 如果已经连上了, 只尝试重新同步 NTP (不重复 configTime)
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println(F("[WIFI] Already connected, skipping WiFi.begin()"));
    } else {
        Serial.print(F("[WIFI] Connecting to: "));
        Serial.println(WIFI_STA_SSID);
        WiFi.begin(WIFI_STA_SSID, WIFI_STA_PASSWORD);

        // ---- 等 WiFi 连接 (超时 15s) ----
        unsigned long start = millis();
        while (WiFi.status() != WL_CONNECTED) {
            if (millis() - start > WIFI_CONNECT_TIMEOUT_MS) {
                Serial.println(F("[WIFI] WiFi connect TIMEOUT"));
                wifi_state = MY_WIFI_OFF;
                return false;
            }
            delay(500);
            Serial.print(F("."));
        }
        Serial.println(F(""));
    }

    Serial.print(F("[WIFI] Connected! IP: "));
    Serial.println(WiFi.localIP());
    wifi_state = MY_WIFI_CONNECTED;

    // ---- 配置 NTP (仅一次, 避免重复注册 SNTP 任务导致跳秒) ----
    if (!ntp_configured) {
        configTime(NTP_TZ_OFFSET_SEC, 0, "ntp.aliyun.com", "cn.ntp.org.cn");
        ntp_configured = true;
    }

    // ---- 等 NTP 同步 (超时 10s) ----
    Serial.print(F("[WIFI] Syncing NTP..."));
    unsigned long start = millis();
    time_t now = time(nullptr);
    while (now < NTP_TZ_OFFSET_SEC) {
        if (millis() - start > NTP_SYNC_TIMEOUT_MS) {
            Serial.println(F(" TIMEOUT"));
            return false;
        }
        delay(200);
        now = time(nullptr);
    }

    // ---- 同步成功 ----
    wifi_state = MY_WIFI_ONLINE;
    Serial.println(F(" OK"));

    struct tm* t = localtime(&now);
    Serial.printf("[WIFI] NTP time: %04d-%02d-%02d %02d:%02d:%02d\n",
                  t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
                  t->tm_hour, t->tm_min, t->tm_sec);

    return true;
}

// ============================================================
//  状态查询
// ============================================================
bool wifi_is_online() {
    return (wifi_state == MY_WIFI_ONLINE);
}

const char* wifi_status_str() {
    switch (wifi_state) {
        case MY_WIFI_ONLINE:    return "OK";
        case MY_WIFI_CONNECTED: return "STA";
        default:             return "no";
    }
}

// ============================================================
//  时间格式化
// ============================================================
void wifi_get_time_str(char* buf, size_t len) {
    if (wifi_state != MY_WIFI_ONLINE) {
        strncpy(buf, "uninternet...", len);
        return;
    }

    time_t now = time(nullptr);
    struct tm* t = localtime(&now);

    snprintf(buf, len, "%04d-%02d-%02d %02d:%02d:%02d",
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
             t->tm_hour, t->tm_min, t->tm_sec);
}

// ============================================================
//  AP 模式 (遥控模式)
// ============================================================
static bool ap_active = false;

void wifi_ap_start() {
    // 对标官方例程: 不预先设 mode, 让 softAP() 自行处理
    // 密码 < 8 字符则 WPA2 无效, 降级为开放网络
    const char* pwd = WIFI_AP_PASSWORD;
    if (strlen(pwd) < 8) {
        Serial.println(F("[WIFI] AP password too short (<8), using OPEN network"));
        WiFi.softAP(WIFI_AP_SSID);
    } else {
        WiFi.softAP(WIFI_AP_SSID, pwd);
    }

    ap_active = true;

    Serial.print(F("[WIFI] AP started: "));
    Serial.print(WIFI_AP_SSID);
    Serial.print(F("  IP: "));
    Serial.println(WiFi.softAPIP());
}

void wifi_ap_stop() {
    WiFi.softAPdisconnect(true);
    ap_active = false;
    Serial.println(F("[WIFI] AP stopped"));
}

bool wifi_ap_is_client_connected() {
    if (!ap_active) return false;
    return (WiFi.softAPgetStationNum() > 0);
}

const char* wifi_ap_get_ip() {
    static char ip_str[16];
    IPAddress ip = WiFi.softAPIP();
    snprintf(ip_str, sizeof(ip_str), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    return ip_str;
}
