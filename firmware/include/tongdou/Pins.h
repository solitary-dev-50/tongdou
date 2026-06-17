#pragma once

#include <Arduino.h>

namespace tongdou::pins {

constexpr gpio_num_t I2C_SDA = GPIO_NUM_1;
constexpr gpio_num_t I2C_SCL = GPIO_NUM_3;

constexpr gpio_num_t VOICE_UART_RX = GPIO_NUM_4;
constexpr gpio_num_t VOICE_UART_TX = GPIO_NUM_5;

constexpr gpio_num_t USB_CHG = GPIO_NUM_9;
constexpr gpio_num_t VBAT_ADC = GPIO_NUM_10;
constexpr gpio_num_t CHRG = GPIO_NUM_11;
constexpr gpio_num_t STBY = GPIO_NUM_12;

constexpr gpio_num_t WS2812_DATA = GPIO_NUM_34;

constexpr gpio_num_t MOTOR_AIN1 = GPIO_NUM_35;
constexpr gpio_num_t MOTOR_AIN2 = GPIO_NUM_36;
constexpr gpio_num_t MOTOR_BIN2 = GPIO_NUM_37;
constexpr gpio_num_t MOTOR_BIN1 = GPIO_NUM_38;

constexpr gpio_num_t RADAR_OT1 = GPIO_NUM_39;
constexpr gpio_num_t RADAR_RX = GPIO_NUM_40;
constexpr gpio_num_t RADAR_OT2 = GPIO_NUM_41;

}  // namespace tongdou::pins
