# -*- coding: utf-8 -*-
"""端到端测试：broker + MQTT 订阅 + 发布 + 入库 + 告警，单进程跑通验证"""
import asyncio
import json
import threading
import time
from datetime import datetime

import paho.mqtt.client as mqtt
from amqtt.broker import Broker

import database
from mqtt_handler import _on_connect, _on_message


async def _run_broker():
    broker = Broker({
        "listeners": {"default": {"type": "tcp", "bind": "127.0.0.1:1883"}},
        "sys_interval": 10,
        "topic-check": {"enabled": False},
        "auth": {"allow-anonymous": True},
    })
    await broker.start()
    await asyncio.Event().wait()


def _start_broker():
    asyncio.run(_run_broker())


def _start_subscriber():
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    client.on_connect = _on_connect
    client.on_message = _on_message
    client.connect("127.0.0.1", 1883, 60)
    client.loop_forever()


def main():
    database.init_db()

    threading.Thread(target=_start_broker, daemon=True).start()
    time.sleep(2)
    threading.Thread(target=_start_subscriber, daemon=True).start()
    time.sleep(2)

    pub = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
    pub.connect("127.0.0.1", 1883, 60)
    pub.loop_start()

    payload = {
        "device_id": "NODE_TEST",
        "timestamp": datetime.now().isoformat(timespec="seconds"),
        "data": [
            {"type": "temperature", "value": 26.5, "unit": "℃"},
            {"type": "water_level", "value": 0.9, "unit": "m"},  # 越限，应触发告警
        ],
    }
    pub.publish("shm/NODE_TEST/data", json.dumps(payload, ensure_ascii=False))
    time.sleep(2)

    devices = database.list_devices()
    rows = database.query_data(device_id="NODE_TEST")
    alarms = database.list_alarms()

    print("=== 设备 ===")
    for d in devices:
        print(" ", d["device_id"], "最后在线:", d["last_seen"])
    print("=== 数据 ===")
    for r in rows:
        print(f"  {r['type']}={r['value']}{r['unit']} [{r['status']}]")
    print("=== 告警 ===")
    for a in alarms:
        print(f"  {a['device_id']} {a['type']}={a['value']} 级别={a['level']}")

    ok = True
    if not any(d["device_id"] == "NODE_TEST" for d in devices):
        ok = False
        print("[失败] 设备未入库")
    if not any(r["type"] == "water_level" and r["status"] == "alarm" for r in rows):
        ok = False
        print("[失败] 越限告警未触发")
    if not any(a["device_id"] == "NODE_TEST" for a in alarms):
        ok = False
        print("[失败] 告警未写入 alarms 表")

    print("\n[测试通过] 全链路：发布 → MQTT 订阅 → 入库 → 告警 均正常" if ok else "\n[测试失败]")


if __name__ == "__main__":
    main()
