/**
 * ESP8266 HTTP 通信层
 *
 * 通过 HTTP POST 向下位机发送二进制命令并接收二进制响应。
 * 注意：由于下位机为 ESP8266 AP 模式，需要使用 HTTP（非 HTTPS），
 *       开发调试时在微信开发者工具中勾选"不校验合法域名"。
 */

import { buildQueryStatusCommand } from './protocol';

const API_PATH = '/api/v1';
const REQUEST_TIMEOUT = 5000; // 5 秒超时

export interface DeviceConfig {
  ip: string;
  port: number;
}

export class CommunicationError extends Error {
  code: number | string;
  constructor(message: string, code: number | string = -1) {
    super(message);
    this.code = code;
    this.name = 'CommunicationError';
  }
}

/**
 * 向下位机发送二进制命令
 * @param config - 设备 IP 和端口
 * @param command - ArrayBuffer 命令数据
 * @returns 下位机响应的 ArrayBuffer
 */
export function sendCommand(config: DeviceConfig, command: ArrayBuffer): Promise<ArrayBuffer> {
  const url = `http://${config.ip}:${config.port}${API_PATH}`;

  return new Promise((resolve, reject) => {
    wx.request({
      url,
      method: 'POST',
      data: command,
      dataType: '其他',           // 告诉微信这不是 JSON
      responseType: 'arraybuffer', // 响应以 ArrayBuffer 返回
      timeout: REQUEST_TIMEOUT,
      header: {
        'Content-Type': 'application/octet-stream',
      },
      enableHttp2: false,
      enableQuic: false,
      enableCache: false,
      success: (res) => {
        if (res.statusCode === 200) {
          resolve(res.data as ArrayBuffer);
        } else {
          reject(
            new CommunicationError(
              `HTTP ${res.statusCode}: request failed`,
              res.statusCode
            )
          );
        }
      },
      fail: (err) => {
        // 微信网络请求失败时的错误
        const errMsg = err.errMsg || 'Network request failed';
        if (errMsg.includes('timeout')) {
          reject(new CommunicationError('请求超时，请检查设备是否在线', 'timeout'));
        } else if (errMsg.includes('fail')) {
          reject(
            new CommunicationError(
              '无法连接设备，请确认 IP 和端口是否正确，\n并已在开发者工具中勾选"不校验合法域名"',
              'connection_failed'
            )
          );
        } else {
          reject(new CommunicationError(errMsg, 'network_error'));
        }
      },
    });
  });
}

/**
 * 连接测试：发送查询状态命令，验证连通性
 */
export async function testConnection(config: DeviceConfig): Promise<boolean> {
  try {
    await sendCommand(config, buildQueryStatusCommand());
    return true;
  } catch {
    return false;
  }
}
