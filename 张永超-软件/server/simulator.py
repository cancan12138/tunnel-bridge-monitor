# -*- coding: utf-8 -*-
"""模拟监测节点：模拟隧道/桥梁传感器数据上报，用于无真实硬件时跑通全链路"""
import json
import random
import time
from datetime import datetime

import paho.mqtt.client as mqtt

from config import MQTT_BROKER, MQTT_PORT

DEVICE_ID = "NODE_001"

# 各传感器类型：(类型, 基准值, 波动幅度, 单位)
SENSORS = [
    ("vibration", 0.02, 0.05, "g"),
    ("stress", 120.0, 80.0, "με"),
    ("temperature", 26.0, 8.0, "℃"),
    ("humidity", 65.0, 15.0, "%"),
    ("water_level", 0.3, 0.2, "m"),
    ("seepage", 5.0, 8.0, "mL/min"),
    ("displacement", 1.0, 2.0, "mm"),
    ("crack", 0.05, 0.05, "mm"),
    ("load", 15.0, 20.0, "t"),
]


def gen_payload():
    now = datetime.now().isoformat(timespec="seconds")
    data = []
    for type_, base, amp, unit in SENSORS:
        value = round(max(0.0, base + random.uniform(-amp, amp)), 3)
        data.append({"type": type_, "value": value, "unit": unit})
    return {"device_id": DEVICE_ID, "timestamp": now, "data": data}


def main():
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    client.connect(MQTT_BROKER, MQTT_PORT, 60)
    client.loop_start()
    print(f"[模拟节点] {DEVICE_ID} 开始上报，broker={MQTT_BROKER}:{MQTT_PORT}")
    try:
        while True:
            payload = gen_payload()
            topic = f"shm/{DEVICE_ID}/data"
            client.publish(topic, json.dumps(payload, ensure_ascii=False))
            print(f"[上报] {payload['timestamp']} 批量 {len(payload['data'])} 项")
            time.sleep(5)  # 每 5 秒上报一次，便于观察
    except KeyboardInterrupt:
        print("\n[模拟节点] 已停止")
        client.loop_stop()


if __name__ == "__main__":
    main()
