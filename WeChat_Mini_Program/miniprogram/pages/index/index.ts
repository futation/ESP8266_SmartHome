import {
  buildQueryStatusCommand,
  buildSetRelayCommand,
  buildSetIRCommand,
  parseQueryStatusResponse,
  parseSetResponse,
  SensorData,
  STATE_ON,
  STATE_OFF,
} from '../../utils/protocol';
import { sendCommand, DeviceConfig, CommunicationError } from '../../utils/communication';

/** 一条日志记录 */
interface LogEntry {
  time: string;
  text: string;
  type: 'info' | 'success' | 'error' | 'recv';
}

Page({
  data: {
    // ── 连接配置 ──
    ip: '',
    port: '80',

    // ── 传感器数据 ──
    sensorData: null as SensorData | null,
    hasData: false,

    // ── UI 状态 ──
    loading: false,
    statusText: '未连接',

    // ── 日志 ──
    logs: [] as LogEntry[],
  },

  // ── 生命周期 ──
  onLoad() {
    // 恢复上次的 IP 和端口
    const savedIp = wx.getStorageSync('device_ip');
    const savedPort = wx.getStorageSync('device_port');
    if (savedIp) {
      this.setData({ ip: savedIp });
    }
    if (savedPort) {
      this.setData({ port: String(savedPort) });
    }
  },

  // ── IP 输入 ──
  onIpInput(e: WechatMiniprogram.Input) {
    const ip = e.detail.value;
    this.setData({ ip });
    wx.setStorageSync('device_ip', ip);
  },

  // ── 端口输入 ──
  onPortInput(e: WechatMiniprogram.Input) {
    const port = e.detail.value;
    this.setData({ port });
    wx.setStorageSync('device_port', port);
  },

  // ── 获取当前设备配置 ──
  getDeviceConfig(): DeviceConfig | null {
    const { ip, port } = this.data;
    if (!ip || !ip.trim()) {
      wx.showToast({ title: '请输入设备 IP 地址', icon: 'none' });
      return null;
    }
    const portNum = parseInt(port, 10);
    if (isNaN(portNum) || portNum < 1 || portNum > 65535) {
      wx.showToast({ title: '端口号无效 (1~65535)', icon: 'none' });
      return null;
    }
    return { ip: ip.trim(), port: portNum };
  },

  // ── 快捷扫码 ──
  async onScanQRCode() {
    // wx.scanCode 自带授权流程，无需预先调用 wx.authorize
    wx.scanCode({
      scanType: ['qrCode'],
      success: (res) => {
        this.parseQRContent(res.result);
      },
      fail: (err) => {
        const errMsg = err.errMsg || '';
        if (errMsg.includes('cancel')) {
          // 用户主动取消扫码，不做处理
          return;
        }
        // 权限被拒绝 或 其他错误
        this.appendLog('摄像头权限不足，请在小程序设置中开启', 'error');
        wx.showModal({
          title: '需要摄像头权限',
          content: '请在「设置 → 权限」中开启摄像头权限，或手动输入 IP 和端口',
          showCancel: false,
        });
      },
    });
  },

  /**
   * 解析二维码内容并自动填入 IP 和端口
   * 支持的格式：
   *   1. JSON:  {"ip":"192.168.4.1","port":80}
   *   2. URL:   smarthome://192.168.4.1:80
   *   3. 纯文本: 192.168.4.1:80
   */
  parseQRContent(raw: string) {
    const content = raw.trim();

    // 尝试 JSON 格式
    if (content.startsWith('{')) {
      try {
        const obj = JSON.parse(content);
        if (obj.ip && obj.port !== undefined) {
          this.setData({ ip: String(obj.ip), port: String(obj.port) });
          wx.setStorageSync('device_ip', String(obj.ip));
          wx.setStorageSync('device_port', String(obj.port));
          this.appendLog(`扫码成功：${obj.ip}:${obj.port}`, 'success');
          // 扫码填入后自动查询一次状态
          setTimeout(() => this.onQueryStatus(), 300);
          return;
        }
      } catch {
        // JSON 解析失败，继续尝试其他格式
      }
    }

    // 尝试 URL scheme 格式
    const urlMatch = content.match(/smarthome:\/\/(.*)/);
    if (urlMatch) {
      const addr = urlMatch[1];
      const parts = addr.split(':');
      if (parts.length === 2) {
        this.setData({ ip: parts[0], port: parts[1] });
        wx.setStorageSync('device_ip', parts[0]);
        wx.setStorageSync('device_port', parts[1]);
        this.appendLog(`扫码成功：${parts[0]}:${parts[1]}`, 'success');
        setTimeout(() => this.onQueryStatus(), 300);
        return;
      }
    }

    // 尝试 IP:Port 纯文本格式
    const addrMatch = content.match(/^(\d{1,3}\.\d{1,3}\.\d{1,3}\.\d{1,3}):(\d{1,5})$/);
    if (addrMatch) {
      this.setData({ ip: addrMatch[1], port: addrMatch[2] });
      wx.setStorageSync('device_ip', addrMatch[1]);
      wx.setStorageSync('device_port', addrMatch[2]);
      this.appendLog(`扫码成功：${addrMatch[1]}:${addrMatch[2]}`, 'success');
      setTimeout(() => this.onQueryStatus(), 300);
      return;
    }

    // 无法识别
    this.appendLog(`无法识别二维码内容: ${content.substring(0, 50)}`, 'error');
    wx.showToast({ title: '无法识别的二维码格式', icon: 'none' });
  },

  // ── 按钮 1: 查询全部状态 ──
  async onQueryStatus() {
    const config = this.getDeviceConfig();
    if (!config) return;

    this.setData({ loading: true, statusText: '查询中...' });
    this.appendLog(`→ 发送查询状态命令 (0x01)`, 'info');

    try {
      const cmd = buildQueryStatusCommand();
      const response = await sendCommand(config, cmd);
      const data = parseQueryStatusResponse(response);

      this.setData({
        sensorData: data,
        hasData: true,
        loading: false,
        statusText: '已连接',
      });

      const relayText = data.relay === STATE_ON ? '开' : '关';
      const irText = data.ir === STATE_ON ? '开' : '关';
      const pirText = data.pir === STATE_ON ? '有人' : '无人';
      this.appendLog(
        `← 温度:${data.temperature}℃ 湿度:${data.humidity}% 电量:${data.battery}% ` +
        `继电器:${relayText} 红外:${irText} PIR:${pirText}`,
        'recv'
      );
    } catch (err) {
      this.setData({ loading: false, statusText: '连接失败' });
      const msg = err instanceof CommunicationError
        ? err.message
        : '查询状态失败';
      this.appendLog(`✗ ${msg}`, 'error');
      wx.showToast({ title: msg.substring(0, 20), icon: 'none' });
    }
  },

  // ── 按钮 2: 打开红外发射管 ──
  async onIROn() {
    await this.setIR(STATE_ON);
  },

  // ── 按钮 3: 关闭红外发射管 ──
  async onIROff() {
    await this.setIR(STATE_OFF);
  },

  // ── 按钮 4: 打开继电器 ──
  async onRelayOn() {
    await this.setRelay(STATE_ON);
  },

  // ── 按钮 5: 关闭继电器 ──
  async onRelayOff() {
    await this.setRelay(STATE_OFF);
  },

  // ── 通用：设置红外 ──
  async setIR(state: number) {
    const config = this.getDeviceConfig();
    if (!config) return;

    const actionText = state === STATE_ON ? '打开' : '关闭';
    this.setData({ loading: true, statusText: `${actionText}红外中...` });
    this.appendLog(`→ 发送${actionText}红外命令 (0x03, 0x${state.toString(16).padStart(2, '0')})`, 'info');

    try {
      const cmd = buildSetIRCommand(state);
      const response = await sendCommand(config, cmd);
      const result = parseSetResponse(response);

      this.setData({ loading: false, statusText: '已连接' });

      if (result.result === 0) {
        const actualText = result.state === STATE_ON ? '已打开' : '已关闭';
        this.appendLog(`← 红外${actionText}成功，当前状态: ${actualText}`, 'success');
        wx.showToast({ title: `红外${actionText}成功`, icon: 'success' });
      } else {
        this.appendLog(`← 红外${actionText}失败`, 'error');
        wx.showToast({ title: `红外${actionText}失败`, icon: 'none' });
      }

      // 操作后自动刷新状态
      setTimeout(() => this.onQueryStatus(), 200);
    } catch (err) {
      this.setData({ loading: false, statusText: '操作失败' });
      const msg = err instanceof CommunicationError
        ? err.message
        : `${actionText}红外失败`;
      this.appendLog(`✗ ${msg}`, 'error');
      wx.showToast({ title: msg.substring(0, 20), icon: 'none' });
    }
  },

  // ── 通用：设置继电器 ──
  async setRelay(state: number) {
    const config = this.getDeviceConfig();
    if (!config) return;

    const actionText = state === STATE_ON ? '打开' : '关闭';
    this.setData({ loading: true, statusText: `${actionText}继电器中...` });
    this.appendLog(`→ 发送${actionText}继电器命令 (0x02, 0x${state.toString(16).padStart(2, '0')})`, 'info');

    try {
      const cmd = buildSetRelayCommand(state);
      const response = await sendCommand(config, cmd);
      const result = parseSetResponse(response);

      this.setData({ loading: false, statusText: '已连接' });

      if (result.result === 0) {
        const actualText = result.state === STATE_ON ? '已打开' : '已关闭';
        this.appendLog(`← 继电器${actionText}成功，当前状态: ${actualText}`, 'success');
        wx.showToast({ title: `继电器${actionText}成功`, icon: 'success' });
      } else {
        this.appendLog(`← 继电器${actionText}失败`, 'error');
        wx.showToast({ title: `继电器${actionText}失败`, icon: 'none' });
      }

      // 操作后自动刷新状态
      setTimeout(() => this.onQueryStatus(), 200);
    } catch (err) {
      this.setData({ loading: false, statusText: '操作失败' });
      const msg = err instanceof CommunicationError
        ? err.message
        : `${actionText}继电器失败`;
      this.appendLog(`✗ ${msg}`, 'error');
      wx.showToast({ title: msg.substring(0, 20), icon: 'none' });
    }
  },

  // ── 添加日志 ──
  appendLog(text: string, type: LogEntry['type'] = 'info') {
    const now = new Date();
    const time = `${String(now.getHours()).padStart(2, '0')}:${String(now.getMinutes()).padStart(2, '0')}:${String(now.getSeconds()).padStart(2, '0')}`;
    const entry: LogEntry = { time, text, type };
    const logs = [entry, ...this.data.logs].slice(0, 100); // 最多保留 100 条
    this.setData({ logs });
  },

  // ── 分享配置（演示用途） ──
  onShareAppMessage() {
    return {
      title: '智能家居遥控器',
      path: '/pages/index/index',
    };
  },
});
