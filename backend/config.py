# -*- coding: utf-8 -*-
"""全局配置"""
import sys

# Windows 控制台默认 GBK，统一转 UTF-8 避免中文 print 报错
try:
    sys.stdout.reconfigure(encoding="utf-8")
except Exception:
    pass

# MQTT Broker 配置
MQTT_BROKER = "127.0.0.1"
MQTT_PORT = 1883
MQTT_KEEPALIVE = 60

# 订阅 Topic（+ 通配设备 ID）
TOPIC_DATA = "shm/+/data"
TOPIC_HEARTBEAT = "shm/+/heartbeat"
TOPIC_ALARM = "shm/+/alarm"

# SQLite 数据库文件
DB_PATH = "monitor.db"

# 告警阈值（默认值，与接口规范 v1.0 一致）
THRESHOLDS = {
    "vibration": 0.10,      # 振动加速度 g
    "stress": 300.0,        # 应力/应变 με
    "temperature": 50.0,    # 温度 ℃
    "humidity": 90.0,       # 湿度 %
    "water_level": 0.8,     # 水位 m
    "seepage": 30.0,        # 渗水 mL/min
    "displacement": 10.0,   # 位移 mm
    "crack": 0.5,           # 裂缝 mm
    "load": 40.0,           # 车辆荷载 t
}

# REST 服务
API_HOST = "0.0.0.0"
API_PORT = 8000
