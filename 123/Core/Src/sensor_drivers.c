/**
 * @file    sensor_drivers.c
 * @brief   9 类传感器底层驱动实现
 *
 * 说明：具体传感器型号未最终确定，本文件按「常见方案」实现框架：
 *   - 模拟量传感器（振动/应力/水位/位移/裂缝/荷载）走 ADC（12 位）采样；
 *   - 温湿度走 I2C（以 SHT30 为例）；
 *   - 渗水走 GPIO 数字量（低电平=有水）。
 * 外设句柄（hadc1/hi2c1/GPIO）由 CubeMX 生成，引脚见 README 配置指南。
 * 换算系数（ADC_XXX_xxx 宏）需根据实际传感器数据手册标定后修改。
 */
#include "sensor_drivers.h"
#include "main.h"                 /* CubeMX 生成：外设句柄声明 */

/* ============ 外设句柄（CubeMX 生成，需按实际命名核对） ============ */
extern ADC_HandleTypeDef  hadc1;  /* ADC1，模拟量采样 */

/* ============ ADC 通道宏（按 CubeMX 配置的通道序号修改） ============
 * 注意：PA2/PA3 留给 USART2（ESP8266），ADC 通道避开，故用 PA0/1/4/5/6/7
 */
#define ADC_CH_VIBRATION      ADC_CHANNEL_0   /* PA0  */
#define ADC_CH_STRESS         ADC_CHANNEL_1   /* PA1  */
#define ADC_CH_WATER_LEVEL    ADC_CHANNEL_4   /* PA4  */
#define ADC_CH_DISPLACEMENT   ADC_CHANNEL_5   /* PA5  */
#define ADC_CH_CRACK          ADC_CHANNEL_6   /* PA6  */
#define ADC_CH_LOAD           ADC_CHANNEL_7   /* PA7  */

/* ============ 换算系数（需按实际传感器标定） ============ */
#define VIBRATION_SCALE       0.0001f   /* 1 LSB 对应 g      */
#define STRESS_SCALE          0.1f      /* 1 LSB 对应 με     */
#define WATER_LEVEL_SCALE     0.001f    /* 1 LSB 对应 m      */
#define DISPLACEMENT_SCALE    0.01f     /* 1 LSB 对应 mm     */
#define CRACK_SCALE           0.001f    /* 1 LSB 对应 mm     */
#define LOAD_SCALE            0.02f     /* 1 LSB 对应 t      */

/* DHT11 单总线数据引脚（PB10，避开 LCD 排针 PB4~PB9） */
#define DHT11_PORT           GPIOB
#define DHT11_PIN            GPIO_PIN_10

/* 渗水检测引脚（PB11，避开 LCD 排针 PB4~PB9） */
#define SEEPAGE_GPIO_PORT     GPIOB
#define SEEPAGE_GPIO_PIN      GPIO_PIN_11

/* ============ 通用 ADC 采样 ============ */
static uint16_t adc_read_channel(uint32_t channel)
{
    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel      = channel;
    sConfig.Rank         = ADC_REGULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_239CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc1, &sConfig);
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, 10);
    uint16_t val = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);
    return val;
}

/* ============ 模拟量传感器 ============ */
void Vibration_Init(void)   {}
void Stress_Init(void)      {}
void WaterLevel_Init(void)  {}
void Displacement_Init(void){}
void Crack_Init(void)       {}
void Load_Init(void)        {}

int Vibration_Read(float *value)
{
    *value = adc_read_channel(ADC_CH_VIBRATION) * VIBRATION_SCALE;
    return 0;
}

int Stress_Read(float *value)
{
    *value = adc_read_channel(ADC_CH_STRESS) * STRESS_SCALE;
    return 0;
}

int WaterLevel_Read(float *value)
{
    *value = adc_read_channel(ADC_CH_WATER_LEVEL) * WATER_LEVEL_SCALE;
    return 0;
}

int Displacement_Read(float *value)
{
    *value = adc_read_channel(ADC_CH_DISPLACEMENT) * DISPLACEMENT_SCALE;
    return 0;
}

int Crack_Read(float *value)
{
    *value = adc_read_channel(ADC_CH_CRACK) * CRACK_SCALE;
    return 0;
}

int Load_Read(float *value)
{
    *value = adc_read_channel(ADC_CH_LOAD) * LOAD_SCALE;
    return 0;
}

/* ============ 温湿度（DHT11，单总线） ============
 * DHT11 单线协议：数据脚 PB6，读取时动态切换输入/输出方向。
 * 微秒延时用 DWT 计数器实现；真板验证若读不到数据，
 * 多因系统时钟偏低（当前 HSI 8MHz），可启用 PLL 升到 72MHz。
 */
void Temperature_Init(void)
{
    /* 使能 DWT 计数器（用于微秒延时） */
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = DHT11_PIN;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(DHT11_PORT, &gpio);
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_SET);  /* 空闲拉高 */
}

void Humidity_Init(void) {}   /* DHT11 温湿度共用一次初始化 */

/* 微秒延时（DWT 计数器） */
static void dht11_delay_us(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * (SystemCoreClock / 1000000U);
    while ((DWT->CYCCNT - start) < ticks) {
    }
}

/* 切换数据脚方向 */
static void dht11_set_mode(GPIO_InitTypeDef *gpio, uint32_t mode)
{
    gpio->Pin = DHT11_PIN;
    gpio->Mode = mode;
    gpio->Speed = GPIO_SPEED_FREQ_HIGH;
    gpio->Pull = (mode == GPIO_MODE_INPUT) ? GPIO_PULLUP : GPIO_NOPULL;
    HAL_GPIO_Init(DHT11_PORT, gpio);
}

/* 读取 DHT11 一次（温湿度），返回 0 成功 */
static int dht11_read(float *temp, float *humi)
{
    uint8_t data[5] = {0};
    GPIO_InitTypeDef gpio = {0};
    uint32_t t;

    /* 起始信号：拉低 >18ms，再拉高 20-40us */
    dht11_set_mode(&gpio, GPIO_MODE_OUTPUT_PP);
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_RESET);
    HAL_Delay(18);
    HAL_GPIO_WritePin(DHT11_PORT, DHT11_PIN, GPIO_PIN_SET);
    dht11_delay_us(30);

    /* 切输入，跳过 DHT11 响应（低 80us + 高 80us） */
    dht11_set_mode(&gpio, GPIO_MODE_INPUT);
    t = 0;
    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET) {
        if (++t > 2000) return -1;
    }
    t = 0;
    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_RESET) {
        if (++t > 2000) return -1;
    }
    t = 0;
    while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET) {
        if (++t > 2000) return -1;
    }

    /* 读 40 位数据：50us 低 + 高电平 26-28us=0 / 70us=1 */
    for (int i = 0; i < 40; i++) {
        t = 0;
        while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_RESET) {
            if (++t > 2000) return -1;
        }
        dht11_delay_us(40);
        if (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET) {
            data[i / 8] |= (0x80 >> (i % 8));
            t = 0;
            while (HAL_GPIO_ReadPin(DHT11_PORT, DHT11_PIN) == GPIO_PIN_SET) {
                if (++t > 2000) return -1;
            }
        }
    }

    /* 校验和 */
    if ((uint8_t)(data[0] + data[1] + data[2] + data[3]) != data[4]) {
        return -1;
    }

    *humi = data[0] + data[1] * 0.1f;   /* 湿度 = 整数 + 小数 */
    *temp = data[2] + data[3] * 0.1f;   /* 温度 = 整数 + 小数 */
    return 0;
}

int Temperature_Read(float *value)
{
    float h;
    return dht11_read(value, &h);
}

int Humidity_Read(float *value)
{
    float t;
    return dht11_read(&t, value);
}

/* ============ 渗水（GPIO 数字量，低电平=有水） ============ */
void Seepage_Init(void)
{
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin  = SEEPAGE_GPIO_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(SEEPAGE_GPIO_PORT, &gpio);
}

int Seepage_Read(float *value)
{
    /* 低电平表示有水，量化为 0 / 1，实际流量需另接流量计换算 */
    if (HAL_GPIO_ReadPin(SEEPAGE_GPIO_PORT, SEEPAGE_GPIO_PIN) == GPIO_PIN_RESET) {
        *value = 1.0f;   /* 有水 */
    } else {
        *value = 0.0f;   /* 无水 */
    }
    return 0;
}
