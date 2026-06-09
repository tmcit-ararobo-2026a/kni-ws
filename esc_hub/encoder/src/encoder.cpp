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
    HAL_TIM_Encoder_Start(&htim1, TIM_CHANNEL_ALL);
    TIM1->CNT = 0;
}

void loop()
{
    uint16_t enc_buff = TIM1->CNT;

    HAL_Delay(100);
}
