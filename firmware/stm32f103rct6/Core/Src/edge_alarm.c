/**
 * @file    edge_alarm.c
 * @brief   边缘阈值告警实现
 * @author  杨策凡
 */
#include "edge_alarm.h"

/* 告警阈值（与服务器 config.py 的 THRESHOLDS 保持一致） */
static const float THRESHOLDS[SENSOR_TYPE_COUNT] = {
    0.10f,    /* vibration    振动加速度  g     */
    300.0f,   /* stress       应力/应变   με    */
    50.0f,    /* temperature  温度        ℃     */
    90.0f,    /* humidity     湿度        %     */
    0.8f,     /* water_level  水位        m     */
    30.0f,    /* seepage      渗水        mL/min */
    10.0f,    /* displacement 位移        mm    */
    0.5f,     /* crack        裂缝        mm    */
    40.0f,    /* load         车辆荷载    t     */
};

AlarmLevel EdgeAlarm_CheckOne(const SensorData *d)
{
    float th = THRESHOLDS[d->type];
    if (d->value >= th) {
        return ALARM_LEVEL_ALARM;
    }
    if (d->value >= th * 0.8f) {   /* 达到阈值 80% 进入预警 */
        return ALARM_LEVEL_WARNING;
    }
    return ALARM_LEVEL_NORMAL;
}
