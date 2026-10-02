#ifndef BUTTON_HANDLER_H
#define BUTTON_HANDLER_H

#include <Arduino.h>

/**
 * 按键模式检测器 — GPIO15 用户按键
 *
 * 检测三种模式并分别回调:
 *   单击:  按下后 < 1s 内释放, 且 1s 内无第二次按下
 *   双击:  两次按下间隔 < 1s
 *   长按:  持续按住 > 3s (立即触发, 不等待释放)
 *
 * 回调通过函数指针注册, 业务逻辑与按键检测完全解耦.
 */

typedef void (*ButtonCallback)();

/** 初始化按键引脚 */
void button_init();

/** 主循环轮询, 检测点击模式并触发回调 */
void button_update();

// ---- 注册回调 ----
void button_on_click(ButtonCallback cb);        // 单击
void button_on_double_click(ButtonCallback cb); // 双击
void button_on_long_press(ButtonCallback cb);   // 长按

#endif
