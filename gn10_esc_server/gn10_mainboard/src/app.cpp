#include "gn10_mainboard/app.hpp"

#include <cmath>

#include "drivers/stm32_fdcan/driver_stm32_fdcan.hpp"
#include "fdcan.h"
#include "gn10_can/core/can_bus.hpp"
#include "gn10_can/devices/esc_hub_client.hpp"
#include "gn10_can/devices/esc_hub_server.hpp"
#include "gn10_can/devices/motor_driver_client.hpp"
#include "gn10_can/devices/power_manager_client.hpp"
#include "gn10_can/devices/robot_control_hub_server.hpp"
#include "gn10_can/devices/servo_motor_client.hpp"
#include "gn10_mainboard/fdcan_driver.hpp"
#include "gn10_mainboard/four_wheel_omni.hpp"
#include "gn10_mainboard/pid.hpp"
#include "gn10_mainboard/vesc_can.hpp"
#include "robomas_can/c610_can.hpp"
#include "robomas_can/c620_can.hpp"
#include "wiznet_ether/robot_ethernet.hpp"
#include "wiznet_ether/serial_printf.hpp"
namespace {

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
        HAL_GPIO_TogglePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin);
    }
}

}  // namespace

gn10_can::drivers::DriverSTM32FDCAN can1_driver(&hfdcan1);
gn10_can::drivers::FDCANDriver fdcan2_driver(&hfdcan2);
gn10_can::drivers::FDCANDriver fdcan3_driver(&hfdcan3);

gn10_can::FDCANBus fdcan2_bus(fdcan2_driver);

robomas_can::C620CAN wheel_esc(can1_driver);
gn10_can::CANBus can1_bus(can1_driver);
gn10_can::FDCANBus fdcan3_bus(fdcan3_driver);

gn10_can::devices::ESCHubClient esc_hub(fdcan3_bus, 0);
gn10_can::devices::ESCHubServer esc_hub_ser(fdcan2_bus, 1);

int32_t enc_value = 0;

/**
 * @brief Initialize CAN and mainboard application state.
 */
void setup()
{
    can1_driver.init();
    fdcan2_driver.init();
    fdcan3_driver.init();

    // servo_motor.set_init(1000, 1200);
    heartbeat_last_toggle_time_ms = HAL_GetTick();
}

/**
 * @brief Run one control cycle and update status heartbeat LED.
 */
void loop()
{
    esc_hub_ser.set_encoder_feedbacks(1234);
    serial_printf("%d\n", enc_value);
    update_heartbeat_led();
    HAL_Delay(1);
}
extern "C" {
// C言語側の関数のオーバーライド
/**
 * @brief Receive callback for FDCAN FIFO0.
 */
void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef* hfdcan, uint32_t RxFifo0ITs)
{
    if (hfdcan->Instance == hfdcan1.Instance) {
        gn10_can::CANFrame rx_frame;
        can1_driver.receive(rx_frame);
        wheel_esc.receive_data(rx_frame.id, rx_frame.data.data());
    } else if (hfdcan->Instance == hfdcan2.Instance) {
        fdcan2_bus.update();
    } else if (hfdcan->Instance == hfdcan3.Instance) {
        fdcan3_bus.update();
    }
    HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
}
}