#include "brucon_adc.h"
#include "brucon_nvs.h"
static esp_adc_cal_characteristics_t *adc_chars;

static const adc_atten_t atten = ADC_ATTEN_DB_11;
static const adc_unit_t unit = ADC_UNIT_1;
#define DEFAULT_VREF 1100
adc1_channel_t chan_alc =ADC1_CHANNEL_7;//gpio 35 !! static to change
adc1_channel_t chan_bat =ADC1_CHANNEL_4;//gpio 32
static esp_adc_cal_characteristics_t *adc_chars;

#define NO_OF_SAMPLES 200
char adcs_setup=0;
void setup_adcs(){

//https://docs.espressif.com/projects/esp-idf/en/latest/api-reference/peripherals/adc.html
  gpio_config_t io_conf;
  io_conf.intr_type = GPIO_PIN_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pull_down_en = 0;
  io_conf.pull_up_en = 0;
  io_conf.pin_bit_mask = (1<<PIN_HEATER_ALC);
  gpio_config(&io_conf);
    
  adc_unit_t unit = ADC_UNIT_1;

  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(chan_alc, atten);
  adc1_config_channel_atten(chan_bat, atten);
  adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
  esp_adc_cal_value_t val_type = esp_adc_cal_characterize(unit, atten, ADC_WIDTH_BIT_12, DEFAULT_VREF, adc_chars);
 
  if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF) {
    printf("eFuse Vref");
  } else if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP) {
    printf("Two Point");
  } else {
    printf("Default");
  }
  adcs_setup = 1;

}
volatile char calAlcSens;
volatile char getAlcSens;
volatile uint16_t Alc_level;

void calAlcTask(){
  TaskHandle_t Tasktemp;
 
  if(!adcs_setup)
    setup_adcs();
  getAlcSens=1;
  gpio_set_level(PIN_HEATER_ALC,1);
  TickType_t tickstart = xTaskGetTickCount();
  printf("Heating for cal");
  while((xTaskGetTickCount() -  tickstart) < 5000 ){
    printf(".\n");
    vTaskDelay(500);
  }
  printf("\n");
  xTaskCreate( &  getAlcTask  , "getAlc" , 4096, NULL , 5| portPRIVILEGE_BIT , &Tasktemp);

  while(getAlcSens){
  vTaskDelay(200);
  }
  vTaskDelete(Tasktemp);
  gpio_set_level(PIN_HEATER_ALC,0);
  printf("CAL Raw: %d\n", Alc_level);
  calAlcSens=0;
  setBruCONConfigUint32("AlcCal",Alc_level);
  setBruCONConfigFlag("haveAlcCal",1);
  while(1){ vTaskDelay(1000);};
}



void getAlcTask(){
  uint16_t ret=0;
  if(!adcs_setup)
    setup_adcs();
  getAlcSens=1;
  uint32_t adc_reading = 0;
  uint32_t max_adc_reading = 0;
  uint32_t min_adc_reading = 0xffffffff;
  //Multisampling
  TickType_t tickstart = xTaskGetTickCount();
  while((xTaskGetTickCount() -  tickstart) < 1500 ){
    for (int i = 0; i < NO_OF_SAMPLES; i++) {
      if (unit == ADC_UNIT_1) {
	adc_reading += adc1_get_raw((adc1_channel_t)chan_alc);
      }
    }
    adc_reading /= NO_OF_SAMPLES;
    if(adc_reading > max_adc_reading)
      max_adc_reading = adc_reading;
    if(adc_reading < min_adc_reading)
      min_adc_reading = adc_reading;
  }
  //Convert adc_reading to voltage in mV
  uint32_t voltage = esp_adc_cal_raw_to_voltage(max_adc_reading, adc_chars);
  printf("ALC Raw: max:%d min:%d\tVoltage: %dmV\n", max_adc_reading,min_adc_reading, voltage);
  Alc_level=voltage;
  getAlcSens=0; 
   while(1){ vTaskDelay(1000);};

}

volatile char CalBat=0;
volatile char getBat=0;
volatile uint16_t Bat_level=0;

void calBatTask(){
if(!adcs_setup)
  setup_adcs();
 while(1){
   vTaskDelay(1000);
 }
}

void getBatTask(){
if(!adcs_setup)
  setup_adcs();
 getBat=1;
 uint32_t adc_reading = 0;
 //Multisampling
 for (int i = 0; i < NO_OF_SAMPLES; i++) {
   if (unit == ADC_UNIT_1) {
     adc_reading += adc1_get_raw((adc1_channel_t)chan_bat);
   }
 }
 adc_reading /= NO_OF_SAMPLES;
 //Convert adc_reading to voltage in mV
 uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars);
 printf("BAT Raw: %d\tVoltage: %dmV\n", adc_reading, voltage);
 Bat_level=adc_reading;
 getBat=0;
  while(1){ vTaskDelay(1000);};
}
