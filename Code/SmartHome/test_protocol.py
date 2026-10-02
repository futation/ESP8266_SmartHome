#!/usr/bin/env python3
"""
SmartHome 二进制协议测试脚本

用法:
  1. 电脑连接 ESP_CTRL WiFi
  2. python test_protocol.py

依赖: requests (pip install requests)
"""

import requests
import time
import sys

ESP_URL = "http://192.168.4.1/api/v1"


def send_command(cmd_bytes: bytes) -> bytes:
    """发送二进制命令, 返回二进制响应"""
    try:
        resp = requests.post(
            ESP_URL,
            data=cmd_bytes,
            headers={"Content-Type": "application/octet-stream"},
            timeout=5,
        )
        return resp.content
    except requests.exceptions.ConnectionError:
        print("  [ERR] 无法连接 ESP8266, 请确认已连上 ESP_CTRL WiFi")
        return None
    except requests.exceptions.Timeout:
        print("  [ERR] 请求超时")
        return None


def test_query():
    """0x01: 查询全部状态"""
    print("\n" + "=" * 50)
    print("  TEST 0x01: QUERY_STATUS")
    print("=" * 50)

    data = send_command(b"\x01")
    if data is None:
        return

    print(f"  原始响应 ({len(data)} bytes): {data.hex(' ')}")

    if len(data) < 7 or data[0] != 0x81:
        print(f"  [FAIL] 响应格式错误! 期望 0x81 开头 + 6 数据字节")
        return

    temp = data[1]
    humi = data[2]
    batt = data[3]
    relay = "ON" if data[4] else "OFF"
    ir = "ON" if data[5] else "OFF"
    pir = "YES" if data[6] else "no"

    print(f"  [OK]   温度: {temp}°C")
    print(f"         湿度: {humi}%")
    print(f"         电量: {batt}%")
    print(f"         继电器: {relay}")
    print(f"         红外: {ir}")
    print(f"         人体感应: {pir}")


def test_set_relay(state: bool):
    """0x02: 控制继电器"""
    label = "ON" if state else "OFF"
    print(f"\n{'=' * 50}")
    print(f"  TEST 0x02: SET_RELAY -> {label}")
    print("=" * 50)

    cmd = b"\x02" + (b"\x01" if state else b"\x00")
    data = send_command(cmd)
    if data is None:
        return

    print(f"  原始响应 ({len(data)} bytes): {data.hex(' ')}")

    if len(data) < 3 or data[0] != 0x82:
        print(f"  [FAIL] 响应格式错误!")
        return

    result = "OK" if data[1] == 0 else "FAIL"
    curr = "ON" if data[2] else "OFF"
    print(f"  [{result}]  结果: {result}, 当前状态: {curr}")


def test_set_ir(state: bool):
    """0x03: 控制红外"""
    label = "ON" if state else "OFF"
    print(f"\n{'=' * 50}")
    print(f"  TEST 0x03: SET_IR -> {label}")
    print("=" * 50)

    cmd = b"\x03" + (b"\x01" if state else b"\x00")
    data = send_command(cmd)
    if data is None:
        return

    print(f"  原始响应 ({len(data)} bytes): {data.hex(' ')}")

    if len(data) < 3 or data[0] != 0x82:
        print(f"  [FAIL] 响应格式错误!")
        return

    result = "OK" if data[1] == 0 else "FAIL"
    curr = "ON" if data[2] else "OFF"
    print(f"  [{result}]  结果: {result}, 当前状态: {curr}")


def test_unknown_cmd():
    """发送未知命令, 期望 0xFF"""
    print(f"\n{'=' * 50}")
    print(f"  TEST 0xFF: Unknown command")
    print("=" * 50)

    data = send_command(b"\x99")
    if data is None:
        return

    print(f"  原始响应 ({len(data)} bytes): {data.hex(' ')}")
    if len(data) >= 1 and data[0] == 0xFF:
        print(f"  [OK]   正确返回 0xFF (未知命令)")
    else:
        print(f"  [FAIL] 期望 0xFF")


def test_polling(seconds: int = 10):
    """持续轮询 N 秒, 观察 PIR 变化"""
    print(f"\n{'=' * 50}")
    print(f"  POLLING: 每 2 秒查询一次, 持续 {seconds}s")
    print(f"  (可挥手触发 PIR 观察变化)")
    print("=" * 50)

    start = time.time()
    while time.time() - start < seconds:
        data = send_command(b"\x01")
        if data and len(data) >= 7:
            pir = "!!有人!!" if data[6] else "无人"
            print(f"  [{time.time()-start:4.1f}s] "
                  f"T:{data[1]}°C H:{data[2]}% "
                  f"Bat:{data[3]}% "
                  f"Relay:{data[4]} IR:{data[5]} "
                  f"PIR:{pir}")
        time.sleep(2)


def main():
    print("SmartHome Protocol Test")
    print(f"Target: {ESP_URL}")
    print("Make sure you're connected to ESP_CTRL WiFi!\n")

    # 1. 查询状态
    test_query()

    # 2. 控制继电器
    test_set_relay(False)   # 关
    time.sleep(0.5)
    test_set_relay(True)    # 开
    time.sleep(0.5)

    # 3. 控制红外
    test_set_ir(False)
    time.sleep(0.5)
    test_set_ir(True)
    time.sleep(0.5)

    # 4. 未知命令
    test_unknown_cmd()

    # 5. 持续轮询 (Ctrl+C 可中断)
    try:
        test_polling(15)
    except KeyboardInterrupt:
        print("\n  Interrupted.")

    print(f"\n{'=' * 50}")
    print("  All tests complete!")
    print("=" * 50)


if __name__ == "__main__":
    main()
