/**
 * SmartHome 二进制通信协议 v1.0
 *
 * ESP8266（下位机）与微信小程序（上位机）之间通过 HTTP POST + 二进制 Body 通信。
 * 所有多字节字段均为大端序 (Big-Endian)。
 */

// ── 命令码 ──
export const CMD_QUERY_STATUS = 0x01;
export const CMD_SET_RELAY    = 0x02;
export const CMD_SET_IR       = 0x03;

// ── 响应码 ──
export const RESP_QUERY_STATUS = 0x81;
export const RESP_SET_RESULT   = 0x82;
export const RESP_ERROR        = 0xff;

// ── 状态常量 ──
export const STATE_OFF = 0x00;
export const STATE_ON  = 0x01;

// ── 查询状态响应数据结构 ──
export interface SensorData {
  temperature: number;   // 温度 ℃ (0~50)
  humidity: number;      // 湿度 % (20~90)
  battery: number;       // 电量 % (0~100)
  relay: number;         // 继电器: 0=OFF, 1=ON
  ir: number;            // 红外: 0=OFF, 1=ON
  pir: number;           // 人体感应: 0=无人, 1=有人
}

// ── 设置命令响应数据结构 ──
export interface SetResult {
  result: number;        // 0=成功, 1=失败
  state: number;         // 操作后实际状态
}

// ── 通信错误 ──
export class ProtocolError extends Error {
  code: number;
  constructor(message: string, code: number = -1) {
    super(message);
    this.code = code;
    this.name = 'ProtocolError';
  }
}

/**
 * 构建查询全部状态命令
 * CMD: 0x01 (1 字节)
 */
export function buildQueryStatusCommand(): ArrayBuffer {
  const buf = new ArrayBuffer(1);
  const view = new DataView(buf);
  view.setUint8(0, CMD_QUERY_STATUS);
  return buf;
}

/**
 * 构建继电器控制命令
 * CMD: 0x02, STATE: 0x00=关闭 0x01=打开 (共 2 字节)
 */
export function buildSetRelayCommand(state: number): ArrayBuffer {
  if (state !== STATE_OFF && state !== STATE_ON) {
    throw new ProtocolError(`Invalid relay state: ${state}, must be 0 or 1`);
  }
  const buf = new ArrayBuffer(2);
  const view = new DataView(buf);
  view.setUint8(0, CMD_SET_RELAY);
  view.setUint8(1, state);
  return buf;
}

/**
 * 构建红外发射管控制命令
 * CMD: 0x03, STATE: 0x00=关闭 0x01=打开 (共 2 字节)
 */
export function buildSetIRCommand(state: number): ArrayBuffer {
  if (state !== STATE_OFF && state !== STATE_ON) {
    throw new ProtocolError(`Invalid IR state: ${state}, must be 0 or 1`);
  }
  const buf = new ArrayBuffer(2);
  const view = new DataView(buf);
  view.setUint8(0, CMD_SET_IR);
  view.setUint8(1, state);
  return buf;
}

/**
 * 解析查询状态响应
 * RESP: 0x81 (1B) + temperature (1B) + humidity (1B) + battery (1B)
 *       + relay (1B) + ir (1B) + pir (1B) = 共 7 字节
 */
export function parseQueryStatusResponse(buffer: ArrayBuffer): SensorData {
  if (buffer.byteLength < 7) {
    throw new ProtocolError(
      `Query response too short: expected 7 bytes, got ${buffer.byteLength}`,
      -2
    );
  }
  const view = new DataView(buffer);

  const resp = view.getUint8(0);
  if (resp === RESP_ERROR) {
    throw new ProtocolError('Device returned error: unknown command', resp);
  }
  if (resp !== RESP_QUERY_STATUS) {
    throw new ProtocolError(
      `Unexpected response code: 0x${resp.toString(16)}`,
      resp
    );
  }

  return {
    temperature: view.getUint8(1),
    humidity:    view.getUint8(2),
    battery:     view.getUint8(3),
    relay:       view.getUint8(4),
    ir:          view.getUint8(5),
    pir:         view.getUint8(6),
  };
}

/**
 * 解析设置命令响应
 * RESP: 0x82 (1B) + result (1B) + state (1B) = 共 3 字节
 */
export function parseSetResponse(buffer: ArrayBuffer): SetResult {
  if (buffer.byteLength < 3) {
    throw new ProtocolError(
      `Set response too short: expected 3 bytes, got ${buffer.byteLength}`,
      -2
    );
  }
  const view = new DataView(buffer);

  const resp = view.getUint8(0);
  if (resp === RESP_ERROR) {
    throw new ProtocolError('Device returned error: unknown command', resp);
  }
  if (resp !== RESP_SET_RESULT) {
    throw new ProtocolError(
      `Unexpected response code: 0x${resp.toString(16)}`,
      resp
    );
  }

  return {
    result: view.getUint8(1),
    state:  view.getUint8(2),
  };
}

/**
 * 解析任意响应（自动判断类型）
 */
export function parseResponse(buffer: ArrayBuffer): SensorData | SetResult {
  if (buffer.byteLength < 1) {
    throw new ProtocolError('Empty response');
  }
  const view = new DataView(buffer);
  const resp = view.getUint8(0);

  if (resp === RESP_ERROR) {
    throw new ProtocolError('Device returned error: unknown command', resp);
  }

  if (resp === RESP_QUERY_STATUS) {
    return parseQueryStatusResponse(buffer);
  }

  if (resp === RESP_SET_RESULT) {
    return parseSetResponse(buffer);
  }

  throw new ProtocolError(`Unknown response code: 0x${resp.toString(16)}`, resp);
}

/**
 * 判断是否为查询响应数据
 */
export function isSensorData(data: SensorData | SetResult): data is SensorData {
  return 'temperature' in data;
}
