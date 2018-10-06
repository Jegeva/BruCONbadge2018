#ifndef __BRUCON_ADC_H
#define __BRUCON_ADC_H

#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_adc_cal.h"
#include "settings.h"

#define PIN_ADC_BAT    CONFIG_BAT_ADC_PIN
#define PIN_ADC_ALC    CONFIG_ALCSENSE_ADC_PIN
#define PIN_HEATER_ALC CONFIG_ALCSENSE_HEATER_PIN


#define ADC_NO_OF_SAMPLES 60

extern volatile char getBat;
extern volatile uint16_t Bat_level;
extern volatile char CalBat;
extern volatile char getAlcSens;
extern volatile uint16_t Alc_level;
extern volatile char calAlcSens;

void calAlcTask();
void getAlcTask();
void calBatTask();
void getBatTask();

#endif
