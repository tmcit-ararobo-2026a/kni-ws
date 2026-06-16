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

constexpr uint32_t k_heartbeat_toggle_interval_ms = 500;

uint32_t heartbeat_last_toggle_time_ms = 0;

/**
 * @brief Toggle heartbeat LED at a fixed interval.
 */
void update_heartbeat_led()
{
    const uint32_t now_ms = HAL_GetTick();
    if ((now_ms - heartbeat_last_toggle_time_ms) >= k_heartbeat_toggle_interval_ms) {
        heartbeat_last_toggle_time_ms = now_ms;
        HAL_GPIO_TogglePin(LED_4_GPIO_Port, LED_4_Pin);
    }
}

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
    fdcan1_driver.init();

    vesc.init();
    HAL_TIM_Encoder_Start(&htim8, TIM_CHANNEL_ALL);
    TIM8->CNT = 0;
    // 原点取り

    while (HAL_GPIO_ReadPin(LIM1_2_GPIO_Port, LIM1_2_Pin) == GPIO_PIN_SET) {
        vesc.comm_can_set_current(45, -0.5f);
        vesc.comm_can_set_duty(45, -0.5f);

        HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_SET);
    }
    HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_RESET);
}

bool vesc_move;

void loop()
{
    int32_t enc_buff = TIM8->CNT;

    if (enc_buff > 1000) {
        HAL_GPIO_WritePin(LED_3_GPIO_Port, LED_3_Pin, GPIO_PIN_SET);
    } else {
        HAL_GPIO_WritePin(LED_3_GPIO_Port, LED_3_Pin, GPIO_PIN_RESET);
    }

    update_heartbeat_led();

    esc_hub.set_encoder_feedbacks(enc_buff);

    esc_hub.get_vesc_command(vesc_move);
    if (vesc_move) {
        vesc.comm_can_set_current(45, -1.2f);
        vesc.comm_can_set_duty(45, -1.2f);
    } else {
        vesc.comm_can_set_current(45, -0.0f);
        vesc.comm_can_set_duty(45, -0.0f);
    }
    // loop()内のエラーチェック部分を変更

    HAL_Delay(1);
}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef* hfdcan, uint32_t RxFifo0ITs)
{
    if (hfdcan->Instance == hfdcan1.Instance) {
        fdcan1_bus.update();
    }
}