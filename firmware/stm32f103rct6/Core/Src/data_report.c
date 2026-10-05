/**
 * @file    data_report.c
 * @brief   数据上报实现：手动拼接 JSON（嵌入式环境不引入 JSON 库）
 * @author  杨策凡
 */
#include "data_report.h"
#include "mqtt_client.h"
#include <stdio.h>
#include <string.h>
#include "main.h"

/* 生成时间戳（暂无 RTC，用上电运行时长模拟，保留 ISO8601 格式外壳） */
static void get_timestamp(char *buf, int max_len)
{
    uint32_t sec = HAL_GetTick() / 1000;
    uint32_t h = (sec / 3600) % 24;
    uint32_t m = (sec / 60) % 60;
    uint32_t s = sec % 60;
    snprintf(buf, max_len, "2026-01-01T%02u:%02u:%02u",
             (unsigned)h, (unsigned)m, (unsigned)s);
}

int DataReport_BuildJSON(const SensorData *data, int count, char *out, int max_len)
{
    char ts[32];
    get_timestamp(ts, sizeof(ts));

    int pos = snprintf(out, max_len,
                       "{\"device_id\":\"%s\",\"timestamp\":\"%s\",\"data\":[",
                       DEVICE_ID, ts);
    for (int i = 0; i < count; i++) {
        pos += snprintf(out + pos, max_len - pos,
                        "%s{\"type\":\"%s\",\"value\":%.3f,\"unit\":\"%s\"}",
                        i > 0 ? "," : "",
                        SENSOR_TYPE_STR[data[i].type],
                        data[i].value, data[i].unit);
        if (pos >= max_len) {
            return -1;                 /* 缓冲不足 */
        }
    }
    snprintf(out + pos, max_len - pos, "]}");
    return 0;
}

int DataReport_Publish(const SensorData *data, int count)
{
    char json[512];
    if (DataReport_BuildJSON(data, count, json, sizeof(json)) != 0) {
        return -1;
    }
    char topic[64];
    snprintf(topic, sizeof(topic), TOPIC_DATA, DEVICE_ID);
    return MQTT_Publish(topic, json, (uint16_t)strlen(json), 0);
}

int DataReport_Heartbeat(void)
{
    char ts[32];
    get_timestamp(ts, sizeof(ts));
    char json[128];
    snprintf(json, sizeof(json),
             "{\"device_id\":\"%s\",\"timestamp\":\"%s\",\"type\":\"heartbeat\",\"status\":\"online\"}",
             DEVICE_ID, ts);
    char topic[64];
    snprintf(topic, sizeof(topic), TOPIC_HEARTBEAT, DEVICE_ID);
    return MQTT_Publish(topic, json, (uint16_t)strlen(json), 0);
}
