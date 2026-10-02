#include "protocol_handler.h"
#include "config.h"
#include "sensor_manager.h"
#include "output_control.h"
#include <ESP8266WiFi.h>

// ============================================================
//  协议处理器 — WiFiServer + 二进制 HTTP
//
//  不使用 ESP8266WebServer (它会将 POST body 当字符串消费,
//  破坏二进制数据). 直接用 WiFiServer 手动解析 HTTP.
// ============================================================

static WiFiServer server(80);
static bool server_running = false;

// ============================================================
//  处理单个客户端连接
// ============================================================
static void handle_client(WiFiClient &client) {
    // 等数据到达 (超时 1s)
    unsigned long deadline = millis() + 1000;
    while (!client.available() && millis() < deadline) { delay(1); }
    if (!client.available()) return;

    // ---- 读 HTTP 请求行 ----
    String line = client.readStringUntil('\n');
    bool is_post = line.startsWith("POST");

    // ---- 读 HTTP 头, 提取 Content-Length ----
    int content_length = 0;
    while (client.connected()) {
        line = client.readStringUntil('\n');
        line.trim();
        if (line.length() == 0) break;  // 空行 = 头部结束

        if (line.startsWith("Content-Length:")) {
            content_length = line.substring(15).toInt();
        }
    }

    if (!is_post || content_length <= 0) {
        client.print("HTTP/1.0 405\r\nContent-Length: 0\r\n\r\n");
        client.stop();
        return;
    }

    // ---- 等 body 全部到达 ----
    deadline = millis() + 2000;
    while (client.available() < content_length && millis() < deadline) {
        delay(1);
    }
    if (client.available() < content_length) {
        client.stop();
        return;
    }

    // ---- 读命令字节 ----
    uint8_t cmd = client.read();

    // ---- 构造响应 ----
    uint8_t resp[8];
    size_t resp_len = 0;

    switch (cmd) {

    case 0x01:  // 查询全部状态
        resp[0] = 0x81;
        resp[1] = (uint8_t)sensor_get_temp();
        resp[2] = (uint8_t)sensor_get_humi();
        resp[3] = (uint8_t)sensor_get_battery_pct();
        resp[4] = relay_is_on() ? 0x01 : 0x00;
        resp[5] = ir_is_on()    ? 0x01 : 0x00;
        resp[6] = sensor_pir_triggered() ? 0x01 : 0x00;
        resp_len = 7;
        break;

    case 0x02:  // 控制继电器
        if (content_length >= 2) {
            uint8_t st = client.read();
            if (st) relay_on(); else relay_off();
            resp[0] = 0x82; resp[1] = 0x00; resp[2] = relay_is_on() ? 0x01 : 0x00;
        } else {
            resp[0] = 0x82; resp[1] = 0x01; resp[2] = 0x00;
        }
        resp_len = 3;
        break;

    case 0x03:  // 控制红外
        if (content_length >= 2) {
            uint8_t st = client.read();
            if (st) ir_on(); else ir_off();
            resp[0] = 0x82; resp[1] = 0x00; resp[2] = ir_is_on() ? 0x01 : 0x00;
        } else {
            resp[0] = 0x82; resp[1] = 0x01; resp[2] = 0x00;
        }
        resp_len = 3;
        break;

    default:  // 未知命令
        resp[0] = 0xFF;
        resp_len = 1;
        break;
    }

    // ---- 发送响应 ----
    client.printf(
        "HTTP/1.0 200 OK\r\n"
        "Content-Type: application/octet-stream\r\n"
        "Content-Length: %d\r\n"
        "Connection: close\r\n"
        "\r\n",
        resp_len
    );
    client.write(resp, resp_len);
    client.flush();
    delay(5);
    client.stop();
}

// ============================================================
//  启动 / 停止
// ============================================================
void protocol_start() {
    server.begin();
    server_running = true;
    Serial.println(F("[PROTO] TCP server started on port 80"));
}

void protocol_stop() {
    server.stop();
    server_running = false;
    Serial.println(F("[PROTO] TCP server stopped"));
}

void protocol_handle() {
    if (!server_running) return;

    WiFiClient client = server.available();
    if (client) {
        handle_client(client);
    }
}

