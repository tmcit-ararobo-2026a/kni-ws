#include "encoder/encoder.hpp"

#include "encoder/serial_printf.hpp"
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
    HAL_TIM_Encoder_Start(&htim1, TIM_CHANNEL_1);
}

void loop()
{
    uint16_t count;
    count += read_encoder_value();
    serial_printf("%d", count);
    HAL_Delay(100);
}
