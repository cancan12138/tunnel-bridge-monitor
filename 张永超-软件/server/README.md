# 隧道/桥梁监测系统 · 服务器 + MQTT 骨架

对应接口规范 v1.0 的数据层实现。技术栈：Python + FastAPI + MQTT(paho) + SQLite。

## 目录结构

```
server/
├── main.py           # FastAPI 入口，启动时拉起 MQTT 订阅线程
├── broker.py         # 纯 Python MQTT Broker（amqtt，替代 mosquitto）
├── mqtt_handler.py   # MQTT 订阅 + 解析 + 入库 + 阈值告警
├── database.py       # SQLite 建表与增查
├── config.py         # 配置（broker 地址、Topic、告警阈值、端口）
├── simulator.py      # 模拟监测节点（无真实硬件时跑通全链路）
├── test_e2e.py       # 端到端自测脚本（验证全链路）
└── requirements.txt  # 依赖
```

## 启动步骤

### 0. 安装依赖

```bash
pip install -r requirements.txt
```

### 1. 启动 MQTT Broker

方式 A（推荐，无需额外安装）：用自带纯 Python broker

```bash
python broker.py
```

方式 B：用 mosquitto（Windows 下载安装后执行 `mosquitto -v`，或 Docker `docker run -d -p 1883:1883 eclipse-mosquitto`）

两者都监听 `127.0.0.1:1883`。

### 2. 启动服务器（REST API + MQTT 订阅）

```bash
cd server
python main.py
```

服务启动后：
- REST API：<http://127.0.0.1:8000>
- 自动订阅 `shm/+/data`、`shm/+/heartbeat`、`shm/+/alarm`

### 3. 启动模拟监测节点（另开一个终端）

```bash
cd server
python simulator.py
```

每 5 秒向 `shm/NODE_001/data` 批量上报 9 类传感器数据。

## 验证

浏览器或命令行访问：

- 设备列表：<http://127.0.0.1:8000/api/devices>
- 历史数据：<http://127.0.0.1:8000/api/data?device_id=NODE_001&type=temperature>
- 告警列表：<http://127.0.0.1:8000/api/alarms>

## 说明

- 数据落库到 `monitor.db`（SQLite，自动创建）。
- 告警阈值在 `config.py` 的 `THRESHOLDS` 中配置，与接口规范一致；越限自动写入 alarms 表。
- 真实 STM32 节点联网后，只需按 `shm/{device_id}/data` 主题发布相同 JSON 报文即可接入，无需改动服务器。
