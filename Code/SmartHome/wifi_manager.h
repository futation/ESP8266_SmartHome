#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>

/**
 * WiFi 管理器 — STA 连接 + AP 模式 + NTP 时间
 */

// ---- 初始化 ----
void wifi_init();

// ---- STA 模式 (双击触发) ----
bool wifi_sta_connect();
bool wifi_is_online();
const char* wifi_status_str();
void wifi_get_time_str(char* buf, size_t len);

// ---- AP 模式 (长按进入遥控模式) ----
void wifi_ap_start();
void wifi_ap_stop();
bool wifi_ap_is_client_connected();
const char* wifi_ap_get_ip();

#endif
