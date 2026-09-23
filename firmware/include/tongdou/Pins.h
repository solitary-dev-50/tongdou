#pragma once

#include <Arduino.h>
#include "tongdou/BoardProfile.h"

namespace tongdou { namespace pins {

constexpr gpio_num_t I2C_SDA = GPIO_NUM_6;
constexpr gpio_num_t I2C_SCL = GPIO_NUM_7;

constexpr gpio_num_t QMI8658_INT1 = GPIO_NUM_33;
#if TONGDOU_BOARD_REVISION == 94
constexpr gpio_num_t QMI8658_INT2 = GPIO_NUM_47;
constexpr gpio_num_t BQ27220_GPOUT = GPIO_NUM_34;
#endif

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

// 在编译阶段检查全部已分配的主控信号脚，防止以后重复占用。
constexpr gpio_num_t kAssignedPins[] = {
    I2C_SDA, I2C_SCL, QMI8658_INT1,
#if TONGDOU_BOARD_REVISION == 94
    QMI8658_INT2, BQ27220_GPOUT,
#endif
    PDM_MIC_CLK, PDM_MIC_DATA, USB_CHG, VBAT_ADC, CHRG, STBY,
    I2S_SPK_LRCLK, I2S_SPK_BCLK, I2S_SPK_DATA, SPK_CTRL, LED_DATA,
    LOGO_TOUCH, MOTOR_AIN1, MOTOR_AIN2, MOTOR_BIN2, MOTOR_BIN1,
    MOTOR_SLEEP, MOTOR_FAULT, RADAR_OUT, RADAR_TX_TO_MODULE_RX,
    RADAR_RX_FROM_MODULE_TX, GPIO_NUM_19, GPIO_NUM_20};
constexpr bool assignedPinsAreUnique() {
  for (size_t i = 0; i < sizeof(kAssignedPins) / sizeof(kAssignedPins[0]); ++i) {
    for (size_t j = i + 1; j < sizeof(kAssignedPins) / sizeof(kAssignedPins[0]); ++j) {
      if (kAssignedPins[i] == kAssignedPins[j]) return false;
    }
  }
  return true;
}
static_assert(assignedPinsAreUnique(), "Tong Dou hardware pins overlap");

}}  // namespace tongdou::pins
