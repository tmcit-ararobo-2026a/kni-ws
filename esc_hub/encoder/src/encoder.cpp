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
gn10_can::drivers::FDCANDriver fdcan2_driver(&hfdcan2);

gn10_can::FDCANBus fdcan1_bus(fdcan1_driver);
gn10_can::FDCANBus fdcan2_bus(fdcan2_driver);
gn10_can::devices::ESCHubServer esc_hub(fdcan1_bus, 0);
gn10_can::devices::ESCHubServer esc_hub_2(fdcan2_bus, 0);

int16_t read_encoder_value(void)
{
    uint16_t enc_buff = TIM8->CNT;
    TIM8->CNT         = 0;
    if (enc_buff > 32767) {
        return (int16_t)enc_buff * -1;
    } else {
        return (int16_t)enc_buff;
    }
}

void setup()
{
    HAL_TIM_Encoder_Start(&htim8, TIM_CHANNEL_ALL);
    fdcan1_driver.init();
    fdcan2_driver.init();
    while (HAL_GPIO_ReadPin(LIM1_2_GPIO_Port, LIM1_2_Pin) == GPIO_PIN_SET) {
        esc_hub.set_encoder_feedbacks(1231);
        esc_hub_2.set_encoder_feedbacks(1232);
        HAL_GPIO_TogglePin(LED_2_GPIO_Port, LED_2_Pin);

        HAL_Delay(1);
    }
    HAL_GPIO_TogglePin(LED_3_GPIO_Port, LED_3_Pin);
}

void loop() {}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef* hfdcan, uint32_t RxFifo0ITs)
{
    if (hfdcan->Instance == hfdcan1.Instance) {
        fdcan1_bus.update();
        HAL_GPIO_TogglePin(LED_4_GPIO_Port, LED_4_Pin);
    }
}
