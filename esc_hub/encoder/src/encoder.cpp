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
    fdcan1_driver.init();
}

void loop()
{
    uint16_t enc_buff = TIM8->CNT;
    HAL_GPIO_TogglePin(LED_2_GPIO_Port, LED_2_Pin);

    bool esc_move = false;
    esc_hub.get_vesc_command(esc_move);

    if (esc_move) {
        vesc.comm_can_set_current(45, -1.2f);
        vesc.comm_can_set_duty(45, -1.2f);

    } else {
        vesc.comm_can_set_current(45, 0.0f);
        vesc.comm_can_set_duty(45, 0.0f);
    }

    HAL_Delay(100);
}
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef* hfdcan, uint32_t RxFifo0ITs)
{
    if (hfdcan->Instance == hfdcan1.Instance) {
        fdcan1_bus.update();
        HAL_GPIO_TogglePin(LED_4_GPIO_Port, LED_4_Pin);
    }
}
