#include "encoder/encoder.hpp"

#include "drivers/stm32_fdcan/driver_stm32_fdcan.hpp"
#include "encoder/fdcan_driver.hpp"
#include "encoder/vesc_can.hpp"
#include "gn10_can/core/can_bus.hpp"
#include "gn10_can/core/fdcan_bus.hpp"
#include "gn10_can/devices/esc_hub_server.hpp"
#include "tim.h"

gn10_can::drivers::FDCANDriver fdcan1_driver(&hfdcan1);
VescCAN vesc(&hfdcan2);

gn10_can::FDCANBus fdcan1_bus(fdcan1_driver);
gn10_can::devices::ESCHubServer esc_hub(fdcan1_bus, 0);

int16_t read_encoder_value(void)
{
    uint16_t enc_buff = TIM1->CNT;
    TIM1->CNT         = 0;
    if (enc_buff > 32767) {
        return (int16_t)enc_buff * -1;
    } else {
        return (int16_t)enc_buff;
    }
}

void setup()
{
    HAL_TIM_Encoder_Start(&htim8, TIM_CHANNEL_ALL);
    TIM8->CNT = 0;
    vesc.init();
}

void loop()
{
    uint16_t enc_buff = TIM8->CNT;
    HAL_GPIO_TogglePin(LED_1_GPIO_Port, LED_1_Pin);

    vesc.comm_can_set_current(45, -1.2f);
    vesc.comm_can_set_duty(45, -1.2f);

    if (enc_buff < 10000 && 5000 < enc_buff) {
        HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_RESET);
        HAL_GPIO_TogglePin(LED_2_GPIO_Port, LED_2_Pin);
    }
    if (enc_buff < 15000 && 10000 < enc_buff) {
        HAL_GPIO_WritePin(LED_2_GPIO_Port, LED_2_Pin, GPIO_PIN_RESET);
        HAL_GPIO_TogglePin(LED_3_GPIO_Port, LED_3_Pin);
    }
    if (15000 < enc_buff) {
        TIM1->CNT = 0;
        HAL_GPIO_WritePin(LED_3_GPIO_Port, LED_3_Pin, GPIO_PIN_RESET);
    }
    HAL_Delay(100);
}
