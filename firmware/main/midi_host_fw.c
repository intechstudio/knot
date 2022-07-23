#include <stdio.h>
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

void app_main(void)
{

  gpio_set_direction(GPIO_NUM_35, GPIO_MODE_OUTPUT);


  while(1){

    gpio_set_level(GPIO_NUM_35, 0);
    vTaskDelay(1000/portTICK_PERIOD_MS);
    gpio_set_level(GPIO_NUM_35, 1);
    vTaskDelay(1000/portTICK_PERIOD_MS);

  }
}
