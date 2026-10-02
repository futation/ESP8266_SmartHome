# ESP8266 智能家居遥控器 (SmartHome)

一款电池供电的 ESP8266 手持智能家居控制器:自带温湿度 / 人体感应 / 电量监测与 OLED 显示,可通过继电器与红外发射管控制家电;长按按键进入遥控模式后,开启热点并显示二维码,用配套微信小程序**扫码直连**即可查看状态与控制设备。全套设计(固件 + PCB + 微信小程序)开源,适合作为 ESP8266 / 物联网入门项目。

## 功能特点

- 双模式运行:**正常模式**(传感器采集 + OLED 状态页)/ **遥控模式**(AP 热点 + HTTP 二进制协议),长按用户按键切换
- 传感器:DHT11 温湿度、PIR 人体感应、电池电压 ADC 采样(百分比换算)
- 输出控制:继电器 + 红外发射管,既可本机按键操作,也可由小程序远程控制
- 电源管理:单键开关机(硬件电源锁存电路),短按开机 / 长按 3 秒关机
- OLED 显示:状态页 + 遥控模式下显示含 IP 的二维码,小程序扫码自动连接
- 通信协议:HTTP POST + 二进制 Body(非 JSON),小程序端扫码 / 手输 IP 直连,详见 [Code/SmartHome/PROTOCOL.md](Code/SmartHome/PROTOCOL.md)

## 仓库结构

| 目录 | 内容 |
| --- | --- |
| `Code/SmartHome/` | ESP8266 固件源码(Arduino,模块化拆分:电源 / 按键 / 传感器 / 显示 / WiFi / 协议) |
| `PCB/` | 立创EDA专业版 PCB 工程(`ESP12F_Board.epro2`,基于 ESP-12F) |
| `WeChat_Mini_Program/` | 配套微信小程序上位机(TypeScript + Less,官方 TS 模板改造) |

## 快速上手

### 1. 硬件

- 用立创EDA专业版打开 `PCB/ESP12F_Board.epro2` 查看电路(含电源锁存、电池分压采样等),打样焊接
- 接入 DHT11(GPIO14)、PIR 模块(GPIO4)、继电器(GPIO16)、红外发射管(GPIO5)、OLED(GPIO0/2)等,引脚分配见 [`Code/SmartHome/config.h`](Code/SmartHome/config.h)

### 2. 固件

1. 安装 Arduino IDE,添加 ESP8266 开发板支持
2. 安装第三方库:**DHT11**(dhrubasaha08 版)、**Adafruit SSD1306**、**Adafruit GFX**、**QRCode**(ricmoo 版)
3. 打开 `Code/SmartHome/SmartHome.ino`,在 `config.h` 中填入你家路由器的 WiFi 名称与密码(用于 STA 模式对时,不填不影响遥控模式)
4. 选择 ESP8266 开发板烧录

### 3. 微信小程序

1. 用微信开发者工具「导入项目」选择 `WeChat_Mini_Program/` 目录(AppID 选「测试号」即可)
2. 详情 → 本地设置 → 勾选**「不校验合法域名」**(ESP8266 为 HTTP 直连,必须勾选,真机预览同理)
3. 烧录固件后,长按设备用户按键进入遥控模式,OLED 显示热点名与二维码
4. 手机连接设备热点 `ESP_CTRL`(密码 `12345678`),打开小程序**扫码**或手动输入 `192.168.4.1:80`,即可查看温湿度 / 电量 / 人体感应并控制继电器与红外

### 4. 协议测试

电脑连接设备热点后运行 `python Code/SmartHome/test_protocol.py`,可脱机验证协议(依赖 `requests`)。

## 开发环境

- 主控:ESP-12F(ESP8266),Arduino IDE + ESP8266 core
- 显示:SSD1306 128x64 OLED(I2C),二维码使用 QRCode 库生成后逐点绘制
- 上位机:微信开发者工具,TypeScript + Less + Skyline 渲染
- EDA:立创EDA专业版(直接打开 `PCB/ESP12F_Board.epro2`)

## 注意事项

- 若继电器用于市电控制,请务必做好绝缘与隔离,注意用电安全
- AP 模式为局域网直连、无鉴权加密,请仅在可信环境下使用;小程序需在开发者工具中关闭域名校验,正式发布需自行处理 HTTPS/合法域名要求
- `config.h` 中的电池分压参数与你的硬件分压网络相关,改动前请核对原理图

## 许可证

本项目以 [MIT](LICENSE) 协议开源。
