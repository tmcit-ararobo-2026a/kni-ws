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
    if (enc_buff > 32767) {
        return (int16_t)enc_buff * -1;
    } else {
        return (int16_t)enc_buff;
    }
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
        gn10_can::FDCANFrame frame = gn10_can::FDCANFrame::make(
            gn10_can::id::DeviceType::ESCHub, 0, gn10_can::id::MsgTypeESCHub::Encoder
        );
        HAL_GPIO_TogglePin(LED_3_GPIO_Port, LED_3_Pin);

        frame.data[0] = 123;
        frame.dlc     = 1;
        fdcan1_bus.send_frame(frame);
    }

    TIM8->CNT = 0;
}
static int32_t enc_buf;
void loop()
{
    HAL_GPIO_TogglePin(LED_2_GPIO_Port, LED_2_Pin);

    // vesc関連
    static bool vesc_move   = false;
    static int16_t enc_buff = 0;

    bool get_vesc_move = false;
    esc_hub.get_vesc_command(get_vesc_move);

    if (get_vesc_move && !vesc_move) {  // ← !vesc_move で連続押し防止
        vesc_move = true;
        /*
            while (HAL_GPIO_ReadPin(LIM1_2_GPIO_Port, LIM1_2_Pin) == GPIO_PIN_SET) {
                vesc.comm_can_set_current(45, -0.5f);
                vesc.comm_can_set_duty(45, -0.5f);
            }

        TIM8->CNT = 0;
        enc_buff  = 0;  // 開始時にリセット
    }

    enc_buff += read_encoder_value();

    if (vesc_move) {
        vesc.comm_can_set_current(45, -1.2f);
        vesc.comm_can_set_duty(45, -1.2f);
        if (enc_buff > 4000) {
            vesc_move = false;
            HAL_GPIO_WritePin(LED_3_GPIO_Port, LED_3_Pin, GPIO_PIN_SET);
        }
    } else {
        vesc.comm_can_set_current(45, 0.0f);
        vesc.comm_can_set_duty(45, 0.0f);
    }
*/
        vesc.comm_can_set_current(45, -1.2f);
        vesc.comm_can_set_duty(45, -1.2f);

        if (TIM8->CNT > 10) {
            HAL_GPIO_TogglePin(LED_1_GPIO_Port, LED_1_Pin);
            vesc.comm_can_set_current(45, 0.0f);
            vesc.comm_can_set_duty(45, 0.0f);
        }
        esc_hub.set_encoder_feedbacks(1431);

        HAL_Delay(1);
    }
}
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef* hfdcan, uint32_t RxFifo0ITs)
{
    if (hfdcan->Instance == hfdcan1.Instance) {
        fdcan1_bus.update();
        HAL_GPIO_TogglePin(LED_4_GPIO_Port, LED_4_Pin);
    }
}
