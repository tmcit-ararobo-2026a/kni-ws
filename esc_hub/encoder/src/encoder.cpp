#include "encoder/encoder.hpp"

#include "drivers/stm32_fdcan/driver_stm32_fdcan.hpp"
#include "encoder/fdcan_driver.hpp"
#include "encoder/vesc_can.hpp"
#include "gn10_can/core/can_bus.hpp"
#include "gn10_can/core/fdcan_bus.hpp"
#include "gn10_can/devices/esc_hub_server.hpp"
#include "stdio.h"
#include "tim.h"
extern volatile uint32_t last_can_id;

gn10_can::drivers::FDCANDriver fdcan1_driver(&hfdcan1);
VescCAN vesc(&hfdcan2);

gn10_can::FDCANBus fdcan1_bus(fdcan1_driver);
gn10_can::devices::ESCHubServer esc_hub(fdcan1_bus, 0);

constexpr uint32_t k_heartbeat_toggle_interval_ms = 500;

uint32_t heartbeat_last_toggle_time_ms = 0;

bool vesc_move;
int32_t origin_taco    = 0;
int32_t last_position  = 0;
int32_t position       = 0;
int32_t prev_position1 = 0;
int32_t prev_position2 = 0;

bool initialized = false;  // グローバルに追加
bool homing_done = false;  // 追加

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

void do_homing()
{
    // リミットスイッチまで戻る
    while (HAL_GPIO_ReadPin(LIM1_2_GPIO_Port, LIM1_2_Pin) == GPIO_PIN_SET) {
        vesc.comm_can_set_rpm(45, -2500);
        HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_SET);
    }
    vesc.comm_can_set_rpm(45, 0);

    HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_RESET);

    // 初期位置設定（後で絶対値に変えます）
    for (int i = 0; i < 25; i++) {
        vesc.comm_can_set_rpm(45, -2300);
        HAL_Delay(10);
    }
    vesc.comm_can_set_rpm(45, 0);

    int32_t prev1 = -1, prev2 = -2;
    while (!(vesc.get_taco() == prev1 && prev1 == prev2)) {
        prev2 = prev1;
        prev1 = vesc.get_taco();
        HAL_Delay(50);
    }

    origin_taco    = vesc.get_taco();
    position       = 0;
    prev_position1 = 0;
    prev_position2 = 0;
    last_position  = 0;
    initialized    = false;
    homing_done    = true;
}

void setup()
{
    fdcan1_driver.init();

    vesc.init();

    do_homing();
}

// 定義類
float vesc_velo[4]            = {0.0f, 0.0f, 0.0f, 0.0f};
float rpm_conversion_constant = 100000.0f;
float vesc_angular_velocity_command;

void loop()
{
    int32_t rpm  = vesc.get_rpm();
    int32_t taco = vesc.get_taco();

    // potision処理
    if (abs(rpm) > 500) {
        int32_t new_position = taco - origin_taco;
        if (abs(new_position - last_position) > 10) {
            prev_position2 = prev_position1;
            prev_position1 = last_position;
            position       = new_position;
            last_position  = new_position;
        }
    } else {
        // RPMが低いときもprevを更新する
        prev_position2 = prev_position1;
        prev_position1 = position;
    }

    if (abs(position) > 200) initialized = true;

    // 3回連続同じ値なら0にリセット
    if (homing_done && initialized && (abs(position) > 200 || abs(position) < 50)) {
        if ((position == prev_position1) && (position == prev_position2)) {
            do_homing();
        }
    }

    // abs(position) > 130 で停止
    bool stuck = (abs(position) > 200);

    if (esc_hub.get_angular_velocities(vesc_velo)) {
        HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_SET);
        if (vesc_velo[0] > 0.0f || vesc_velo[1] > 0.0f || vesc_velo[2] > 0.0f ||
            vesc_velo[3] > 0.0f) {
            HAL_GPIO_WritePin(LED_2_GPIO_Port, LED_2_Pin, GPIO_PIN_SET);
        } else {
            HAL_GPIO_WritePin(LED_2_GPIO_Port, LED_2_Pin, GPIO_PIN_RESET);
        }
    }

    vesc_angular_velocity_command = vesc_velo[0] * rpm_conversion_constant;

    if (!stuck) {
        vesc.comm_can_set_rpm(45, vesc_angular_velocity_command);
    }

    float neko[4] = {vesc_angular_velocity_command, 0.0f, 0.0f, 0.0f};
    esc_hub.set_angular_velocity_feedbacks(neko);

    update_heartbeat_led();
    HAL_Delay(10);
}

void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef* hfdcan, uint32_t RxFifo0ITs)
{
    if (hfdcan->Instance == hfdcan1.Instance) {
        fdcan1_bus.update();
    }
    if (hfdcan->Instance == hfdcan2.Instance) {
        FDCAN_RxHeaderTypeDef rx_header;
        uint8_t rx_data[8];
        if (HAL_FDCAN_GetRxMessage(hfdcan, FDCAN_RX_FIFO0, &rx_header, rx_data) == HAL_OK) {
            vesc.receive_data(rx_header.Identifier, rx_data, 8);
        }
    }
}