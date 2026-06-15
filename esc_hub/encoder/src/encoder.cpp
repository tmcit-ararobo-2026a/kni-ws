#include "encoder/encoder.hpp"

#include "drivers/stm32_fdcan/driver_stm32_fdcan.hpp"
#include "encoder/fdcan_driver.hpp"
#include "encoder/vesc_can.hpp"
#include "gn10_can/core/can_bus.hpp"
#include "gn10_can/core/fdcan_bus.hpp"
#include "gn10_can/devices/esc_hub_server.hpp"
#include "stdio.h"
#include "tim.h"

gn10_can::drivers::FDCANDriver fdcan1_driver(&hfdcan1);
VescCAN vesc(&hfdcan2);

gn10_can::FDCANBus fdcan1_bus(fdcan1_driver);
gn10_can::devices::ESCHubServer esc_hub(fdcan1_bus, 0);

int16_t read_encoder_value(void)
{
    uint16_t enc_buff = TIM8->CNT;
    TIM8->CNT         = 0;

    return (int16_t)enc_buff;
}

void setup()
{
    HAL_TIM_Encoder_Start(&htim8, TIM_CHANNEL_ALL);
    vesc.init();
    fdcan1_driver.init();
    // 原点取り

    while (HAL_GPIO_ReadPin(LIM1_2_GPIO_Port, LIM1_2_Pin) == GPIO_PIN_SET) {
        vesc.comm_can_set_current(45, -0.5f);
        vesc.comm_can_set_duty(45, -0.5f);

        HAL_GPIO_TogglePin(LED_3_GPIO_Port, LED_3_Pin);
    }

    TIM8->CNT = 0;
}
static int32_t enc_buff = 123;

void loop()
{
    HAL_GPIO_TogglePin(LED_2_GPIO_Port, LED_2_Pin);  // ← 先頭に追加

    esc_hub.set_encoder_feedbacks(enc_buff);

    // loop()内のエラーチェック部分を変更

    HAL_Delay(1);
}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef* hfdcan, uint32_t RxFifo0ITs)
{
    if (hfdcan->Instance == hfdcan1.Instance) {
        fdcan1_bus.update();
    }
}