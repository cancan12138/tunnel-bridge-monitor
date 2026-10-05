/**
 * @file    edge_alarm.h
 * @brief   边缘阈值告警：本地越限判定
 * @author  杨策凡
 */
#ifndef __EDGE_ALARM_H
#define __EDGE_ALARM_H

#include "sensor_types.h"

/* 告警级别 */
typedef enum {
    ALARM_LEVEL_NORMAL = 0,   /* 正常   */
    ALARM_LEVEL_WARNING,      /* 预警   */
    ALARM_LEVEL_ALARM,        /* 告警   */
} AlarmLevel;

/**
 * @brief 判定单个传感器数据的告警级别（阈值与服务器 config.py 一致）
 * @return 告警级别
 */
AlarmLevel EdgeAlarm_CheckOne(const SensorData *d);

#endif /* __EDGE_ALARM_H */
