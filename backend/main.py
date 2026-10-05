# -*- coding: utf-8 -*-
"""FastAPI 服务器入口：启动时初始化数据库并拉起 MQTT 订阅线程"""
import threading

import uvicorn
from fastapi import FastAPI

from config import API_HOST, API_PORT
from database import init_db, list_devices, query_data, list_alarms
from mqtt_handler import start_mqtt

app = FastAPI(title="隧道/桥梁监测系统服务器", version="1.0")


@app.on_event("startup")
def _startup():
    init_db()
    threading.Thread(target=start_mqtt, daemon=True).start()
    print("[服务器] 已启动，MQTT 订阅线程运行中")


@app.get("/")
def root():
    return {"service": "隧道/桥梁监测系统服务器", "version": "1.0"}


@app.get("/api/devices")
def api_devices():
    return list_devices()


@app.get("/api/data")
def api_data(device_id: str = None, type: str = None,
             start: str = None, end: str = None, limit: int = 1000):
    return query_data(device_id, type, start, end, limit)


@app.get("/api/alarms")
def api_alarms(limit: int = 200):
    return list_alarms(limit)


if __name__ == "__main__":
    uvicorn.run(app, host=API_HOST, port=API_PORT)
