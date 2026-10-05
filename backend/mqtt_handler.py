# -*- coding: utf-8 -*-
"""MQTT 订阅与数据处理"""
import json

import paho.mqtt.client as mqtt

from config import (
    MQTT_BROKER, MQTT_PORT, MQTT_KEEPALIVE,
    TOPIC_DATA, TOPIC_HEARTBEAT, TOPIC_ALARM, THRESHOLDS,
)
from database import insert_data, insert_alarm, upsert_device


def _check_threshold(type_, value):
    """返回 (status, level)：越限时 status=warning/alarm，否则 normal"""
    th = THRESHOLDS.get(type_)
    if th is None:
        return "normal", None
    if value >= th:
        return "alarm", "alarm"
    if value >= th * 0.8:
        return "warning", "warning"
    return "normal", None


def _process_item(device_id, item, timestamp):
    type_ = item.get("type")
    value = item.get("value")
    unit = item.get("unit", "")
    status, level = _check_threshold(type_, value)
    insert_data(device_id, type_, value, unit, status, timestamp)
    if level:
        insert_alarm(device_id, type_, value, THRESHOLDS.get(type_), level, timestamp)
        print(f"[告警] {device_id} {type_}={value}{unit} 越限（阈值 {THRESHOLDS.get(type_)}）")
    else:
        print(f"[数据] {device_id} {type_}={value}{unit} [{status}]")


def _on_connect(client, userdata, flags, reason_code, properties):
    if not reason_code.is_failure:
        print(f"[MQTT] 已连接 broker {MQTT_BROKER}:{MQTT_PORT}")
        client.subscribe(TOPIC_DATA)
        client.subscribe(TOPIC_HEARTBEAT)
        client.subscribe(TOPIC_ALARM)
        print(f"[MQTT] 已订阅 {TOPIC_DATA} / {TOPIC_HEARTBEAT} / {TOPIC_ALARM}")
    else:
        print(f"[MQTT] 连接失败 reason_code={reason_code}")


def _on_message(client, userdata, msg):
    try:
        payload = json.loads(msg.payload.decode("utf-8"))
    except Exception as e:
        print(f"[MQTT] 报文解析失败 topic={msg.topic}: {e}")
        return

    device_id = payload.get("device_id", "unknown")
    timestamp = payload.get("timestamp", "")

    if msg.topic.endswith("/data"):
        upsert_device(device_id, timestamp)
        if "data" in payload:          # 批量上报
            for item in payload["data"]:
                _process_item(device_id, item, timestamp)
        else:                          # 单条上报
            _process_item(device_id, payload, timestamp)
    elif msg.topic.endswith("/heartbeat"):
        upsert_device(device_id, timestamp)
        print(f"[心跳] {device_id} 在线")
    elif msg.topic.endswith("/alarm"):
        insert_alarm(
            device_id, payload.get("type"), payload.get("value"),
            payload.get("threshold"), payload.get("level"), timestamp,
        )
        print(f"[告警] {device_id} 上报告警")


def start_mqtt():
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    client.on_connect = _on_connect
    client.on_message = _on_message
    client.connect(MQTT_BROKER, MQTT_PORT, MQTT_KEEPALIVE)
    client.loop_forever()
