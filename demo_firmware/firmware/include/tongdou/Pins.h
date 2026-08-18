#pragma once

#include <Arduino.h>

namespace tongdou::pins {

constexpr gpio_num_t I2C_SDA = GPIO_NUM_6;
constexpr gpio_num_t I2C_SCL = GPIO_NUM_7;

constexpr gpio_num_t PDM_MIC_CLK = GPIO_NUM_1;
constexpr gpio_num_t PDM_MIC_DATA = GPIO_NUM_2;

constexpr gpio_num_t USB_CHG = GPIO_NUM_17;
constexpr gpio_num_t VBAT_ADC = GPIO_NUM_8;
constexpr gpio_num_t CHRG = GPIO_NUM_35;
constexpr gpio_num_t STBY = GPIO_NUM_48;

constexpr gpio_num_t I2S_SPK_LRCLK = GPIO_NUM_12;
constexpr gpio_num_t I2S_SPK_BCLK = GPIO_NUM_13;
constexpr gpio_num_t I2S_SPK_DATA = GPIO_NUM_14;
constexpr gpio_num_t SPK_CTRL = GPIO_NUM_15;

constexpr gpio_num_t LED_DATA = GPIO_NUM_9;

constexpr gpio_num_t LOGO_TOUCH = GPIO_NUM_4;

constexpr gpio_num_t MOTOR_AIN1 = GPIO_NUM_38;
constexpr gpio_num_t MOTOR_AIN2 = GPIO_NUM_39;
constexpr gpio_num_t MOTOR_BIN2 = GPIO_NUM_40;
constexpr gpio_num_t MOTOR_BIN1 = GPIO_NUM_41;
constexpr gpio_num_t MOTOR_SLEEP = GPIO_NUM_42;
constexpr gpio_num_t MOTOR_FAULT = GPIO_NUM_37;

constexpr gpio_num_t RADAR_OUT = GPIO_NUM_5;
constexpr gpio_num_t RADAR_TX_TO_MODULE_RX = GPIO_NUM_10;
constexpr gpio_num_t RADAR_RX_FROM_MODULE_TX = GPIO_NUM_11;

}  // namespace tongdou::pins
