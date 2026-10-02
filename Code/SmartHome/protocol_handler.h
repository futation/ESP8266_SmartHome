#ifndef PROTOCOL_HANDLER_H
#define PROTOCOL_HANDLER_H

#include <Arduino.h>

/**
 * 协议处理器 — HTTP Server + 二进制协议
 *
 * 端点: POST /api/v1
 * 协议: PROTOCOL.md
 */

/** 启动 HTTP Server */
void protocol_start();

/** 停止 HTTP Server */
void protocol_stop();

/** 主循环调用, 处理客户端请求 */
void protocol_handle();

#endif
