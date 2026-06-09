#include "encoder/encoder.hpp"

#include "tim.h"

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
}

void loop()
{
    uint16_t enc_buff = TIM8->CNT;
    HAL_GPIO_TogglePin(LED_1_GPIO_Port, LED_1_Pin);

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
