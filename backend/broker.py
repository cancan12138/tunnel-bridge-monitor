# -*- coding: utf-8 -*-
"""纯 Python MQTT Broker（amqtt），替代 mosquitto，本地开发用"""
import asyncio
import logging

from amqtt.broker import Broker

logging.basicConfig(level=logging.WARNING)  # 降低 amqtt 日志噪音

CONFIG = {
    "listeners": {
        "default": {"type": "tcp", "bind": "0.0.0.0:1883"},
    },
    "sys_interval": 10,
    "topic-check": {"enabled": False},
    "auth": {"allow-anonymous": True},
}


async def _main():
    broker = Broker(CONFIG)
    await broker.start()
    print("[Broker] 已启动，监听 127.0.0.1:1883（Ctrl+C 停止）")
    try:
        await asyncio.Event().wait()
    finally:
        await broker.shutdown()


def main():
    try:
        asyncio.run(_main())
    except KeyboardInterrupt:
        print("[Broker] 已停止")


if __name__ == "__main__":
    main()
