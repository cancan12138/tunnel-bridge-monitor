# -*- coding: utf-8 -*-
"""SQLite 数据库操作"""
import sqlite3

from config import DB_PATH

SCHEMA = """
CREATE TABLE IF NOT EXISTS devices (
    device_id   TEXT PRIMARY KEY,
    name        TEXT DEFAULT '',
    location    TEXT DEFAULT '',
    last_seen   TEXT
);
CREATE TABLE IF NOT EXISTS data (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    device_id   TEXT NOT NULL,
    type        TEXT NOT NULL,
    value       REAL,
    unit        TEXT,
    status      TEXT,
    timestamp   TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS alarms (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    device_id   TEXT NOT NULL,
    type        TEXT NOT NULL,
    value       REAL,
    threshold   REAL,
    level       TEXT,
    timestamp   TEXT NOT NULL
);
CREATE INDEX IF NOT EXISTS idx_data_device_time ON data(device_id, timestamp);
"""


def get_conn():
    conn = sqlite3.connect(DB_PATH, check_same_thread=False)
    conn.row_factory = sqlite3.Row
    return conn


def init_db():
    conn = get_conn()
    conn.executescript(SCHEMA)
    conn.commit()
    conn.close()


def upsert_device(device_id, timestamp):
    conn = get_conn()
    conn.execute(
        "INSERT INTO devices (device_id, last_seen) VALUES (?, ?) "
        "ON CONFLICT(device_id) DO UPDATE SET last_seen=excluded.last_seen",
        (device_id, timestamp),
    )
    conn.commit()
    conn.close()


def insert_data(device_id, type_, value, unit, status, timestamp):
    conn = get_conn()
    conn.execute(
        "INSERT INTO data (device_id, type, value, unit, status, timestamp) "
        "VALUES (?, ?, ?, ?, ?, ?)",
        (device_id, type_, value, unit, status, timestamp),
    )
    conn.commit()
    conn.close()


def insert_alarm(device_id, type_, value, threshold, level, timestamp):
    conn = get_conn()
    conn.execute(
        "INSERT INTO alarms (device_id, type, value, threshold, level, timestamp) "
        "VALUES (?, ?, ?, ?, ?, ?)",
        (device_id, type_, value, threshold, level, timestamp),
    )
    conn.commit()
    conn.close()


def list_devices():
    conn = get_conn()
    rows = conn.execute("SELECT * FROM devices ORDER BY device_id").fetchall()
    conn.close()
    return [dict(r) for r in rows]


def query_data(device_id=None, type_=None, start=None, end=None, limit=1000):
    sql = "SELECT * FROM data WHERE 1=1"
    params = []
    if device_id:
        sql += " AND device_id=?"
        params.append(device_id)
    if type_:
        sql += " AND type=?"
        params.append(type_)
    if start:
        sql += " AND timestamp>=?"
        params.append(start)
    if end:
        sql += " AND timestamp<=?"
        params.append(end)
    sql += " ORDER BY timestamp DESC LIMIT ?"
    params.append(limit)
    conn = get_conn()
    rows = conn.execute(sql, params).fetchall()
    conn.close()
    return [dict(r) for r in rows]


def list_alarms(limit=200):
    conn = get_conn()
    rows = conn.execute(
        "SELECT * FROM alarms ORDER BY timestamp DESC LIMIT ?", (limit,)
    ).fetchall()
    conn.close()
    return [dict(r) for r in rows]
