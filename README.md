# 隧道/桥梁嵌入式监测系统

隧道和桥梁结构健康监测嵌入式系统：以 STM32 采集节点 + 9 类传感器 + WiFi/MQTT 组网，实现结构状态的可感知、可预警、可追溯。

- 团队：第六组 · 超说的都队
- 周期：1 个月

## 目录结构

| 目录 | 说明 | 负责人 |
|------|------|--------|
| [`123/`](123/) | STM32F103RCT6 固件（STM32CubeMX + Keil MDK） | 杨策凡（嵌入式） |
| [`张永超-软件/server/`](张永超-软件/server/) | Python 服务器（FastAPI + MQTT + SQLite） | 张永超（软件） |
| [`docs/`](docs/) | 项目计划书、总体规划、接口规范 v1.0、分工日志 | 李昌（组长） |

## 系统架构

感知层（STM32F103RCT6 采集节点 + 9 类传感器）→ WiFi（ESP8266）→ MQTT Broker → 数据层（FastAPI + SQLite + 告警引擎）→ 应用层（Web / GIS / 三维可视化）。

## 成员分工

| 成员 | 角色 |
|------|------|
| 李昌 | 组长 / 架构 / 文档 |
| 姚凯洋 | 硬件（原理图 / PCB） |
| 严鹏远 | 硬件（传感器电路接入 / 焊接调试 / 硬件测试） |
| 杨策凡 | 嵌入式固件 |
| 张永超 | 软件（服务器 / Web / 3D） |
| 王学博 | 测试 |
| 宋柄深 | 运维 |

## 固件（`123/`）

- STM32F103RCT6 + ESP8266，HAL 库开发
- 9 类传感器驱动（ADC / I2C / GPIO）与传感器抽象层
- MQTT 3.1.1 客户端（CONNECT / PUBLISH / SUBSCRIBE）
- 数据 JSON 打包、边缘阈值告警、心跳保活
- TFT（ST7735）本地显示

## 服务器（`张永超-软件/server/`）

- FastAPI + MQTT（amqtt/paho）+ SQLite
- 数据入库、阈值告警、REST API
- `simulator.py` 模拟节点、`test_e2e.py` 端到端自测

启动与验证步骤见 [`张永超-软件/server/README.md`](张永超-软件/server/README.md)。
