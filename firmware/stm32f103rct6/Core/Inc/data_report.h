/**
 * @file    data_report.h
 * @brief   数据上报：传感器数据打包为 JSON 并发布
 * @author  杨策凡
 */
#ifndef __DATA_REPORT_H
#define __DATA_REPORT_H

#include "sensor_types.h"

/* 设备 ID（与接口规范 device_id 一致，多节点时改为 NODE_002 等） */
#define DEVICE_ID "NODE_001"

/**
 * @brief 将批量传感器数据打包为 JSON（接口规范「批量上报」格式）
 * @return 0 成功
 */
int DataReport_BuildJSON(const SensorData *data, int count, char *out, int max_len);

/**
 * @brief 上报数据：打包 + MQTT 发布到 shm/{device_id}/data
 * @return 0 成功
 */
int DataReport_Publish(const SensorData *data, int count);

/**
 * @brief 发送心跳到 shm/{device_id}/heartbeat
 * @return 0 成功
 */
int DataReport_Heartbeat(void);

#endif /* __DATA_REPORT_H */
