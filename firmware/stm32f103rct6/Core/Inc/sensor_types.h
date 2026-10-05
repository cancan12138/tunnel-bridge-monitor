/**
 * @file    sensor_types.h
 * @brief   传感器类型定义（与接口规范 v1.0 的 type 编码一一对应）
 * @author  杨策凡
 */
#ifndef __SENSOR_TYPES_H
#define __SENSOR_TYPES_H

/* 传感器类型枚举，顺序与接口规范 type 编码一致 */
typedef enum {
    SENSOR_VIBRATION = 0,   /* 振动加速度  g      */
    SENSOR_STRESS,          /* 应力/应变   με     */
    SENSOR_TEMPERATURE,     /* 温度        ℃      */
    SENSOR_HUMIDITY,        /* 湿度        %      */
    SENSOR_WATER_LEVEL,     /* 水位        m      */
    SENSOR_SEEPAGE,         /* 渗水        mL/min */
    SENSOR_DISPLACEMENT,    /* 位移        mm     */
    SENSOR_CRACK,           /* 裂缝        mm     */
    SENSOR_LOAD,            /* 车辆荷载    t      */
    SENSOR_TYPE_COUNT       /* 传感器类型总数     */
} SensorType;

/* 单条传感器数据 */
typedef struct {
    SensorType type;        /* 传感器类型   */
    float      value;       /* 数值         */
    char       unit[16];    /* 单位         */
} SensorData;

/* 类型编码字符串（用于 JSON 上报，与接口规范一致） */
extern const char *SENSOR_TYPE_STR[SENSOR_TYPE_COUNT];
/* 单位字符串 */
extern const char *SENSOR_UNIT_STR[SENSOR_TYPE_COUNT];

#endif /* __SENSOR_TYPES_H */
