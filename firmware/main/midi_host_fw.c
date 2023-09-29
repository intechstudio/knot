/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_intr_alloc.h"
#include "usb/usb_host.h"

#include <string.h>



#include "driver/gpio.h"

#define DAEMON_TASK_PRIORITY    3
#define CLASS_TASK_PRIORITY     4
#define LED_TASK_PRIORITY       2

#define UART_RX_TASK_PRIORITY      12
#define UART_HOUSEKEEPING_TASK_PRIORITY      11

extern void class_driver_task(void *arg);
extern void led_task(void *arg);

extern void uart_init(void);
extern void uart_rx_task(void *arg);
extern void uart_housekeeping_task(void *arg);

static const char *TAG = "DAEMON";

static void host_lib_daemon_task(void *arg)
{
    SemaphoreHandle_t signaling_sem = (SemaphoreHandle_t)arg;

    ESP_LOGI(TAG, "Installing USB Host Library");
    usb_host_config_t host_config = {
        .skip_phy_setup = false,
        .intr_flags = ESP_INTR_FLAG_LEVEL1,
    };
    ESP_ERROR_CHECK(usb_host_install(&host_config));

    //Signal to the class driver task that the host library is installed
    xSemaphoreGive(signaling_sem);
    vTaskDelay(10); //Short delay to let client task spin up

    bool has_clients = true;
    bool has_devices = true;
    while (has_clients || has_devices ) {
        uint32_t event_flags;
        ESP_ERROR_CHECK(usb_host_lib_handle_events(portMAX_DELAY, &event_flags));
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
            //has_clients = false;
        }
        if (event_flags & USB_HOST_LIB_EVENT_FLAGS_ALL_FREE) {
            //has_devices = false;
        }
    }
    ESP_LOGI(TAG, "No more clients and devices");

    //Uninstall the USB Host Library
    ESP_ERROR_CHECK(usb_host_uninstall());
    //Wait to be deleted
    xSemaphoreGive(signaling_sem);
    vTaskSuspend(NULL);
}


#include "rom/ets_sys.h" // For ets_printf
void grid_platform_printf(char const *fmt, ...){


	va_list ap;

	char temp[200] = {0};

	va_start(ap, fmt);

	vsnprintf(temp, 199, fmt, ap);

	va_end(ap);

    ets_printf(temp);


}

#include "../managed_components/sukuwc__grid_common/include/grid_lua_api.h"

void app_main(void)
{

    ESP_LOGI(TAG, "===== LUA INIT =====");
	grid_lua_init(&grid_lua_state);
    grid_lua_set_memory_target(&grid_lua_state, 80); //80kb
    //grid_lua_start_vm(&grid_lua_state);
    //grid_lua_dostring(&grid_lua_state, "print(123)");


    
    #define USB_NATIVE_SELECT_PIN 11
    #define USB_SOFT_SELECT_PIN 12

    gpio_set_direction(USB_NATIVE_SELECT_PIN, GPIO_MODE_OUTPUT);
    gpio_set_direction(USB_SOFT_SELECT_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(USB_NATIVE_SELECT_PIN, 1);
    gpio_set_level(USB_SOFT_SELECT_PIN, 1);


    #define PMIC_EN_PIN 48

    gpio_set_direction(PMIC_EN_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(PMIC_EN_PIN, 1);

    SemaphoreHandle_t signaling_sem = xSemaphoreCreateBinary();


    TaskHandle_t daemon_task_hdl;
    TaskHandle_t class_driver_task_hdl;
    TaskHandle_t led_task_hdl;

    TaskHandle_t uart_rx_task_hdl;
    TaskHandle_t uart_housekeeping_task_hdl;
    //Create daemon task
    xTaskCreatePinnedToCore(host_lib_daemon_task,
                            "daemon",
                            4096,
                            (void *)signaling_sem,
                            DAEMON_TASK_PRIORITY,
                            &daemon_task_hdl,
                            0);
    //Create the class driver task
    xTaskCreatePinnedToCore(class_driver_task,
                            "class",
                            4096,
                            (void *)signaling_sem,
                            CLASS_TASK_PRIORITY,
                            &class_driver_task_hdl,
                            0);

    //Create the class driver task
    xTaskCreatePinnedToCore(led_task,
                            "led",
                            4096,
                            (void *)signaling_sem,
                            LED_TASK_PRIORITY,
                            &led_task_hdl,
                            0);


    //Create a task to handler UART event from ISR

    vTaskDelay(10);     //Add a short delay to let the tasks run

    uart_init();

    xTaskCreatePinnedToCore(uart_rx_task, 
                            "uart_rx", 
                            2048, 
                            (void *)signaling_sem, 
                            UART_RX_TASK_PRIORITY, 
                            &uart_rx_task_hdl,
                            0);

    xTaskCreatePinnedToCore(uart_housekeeping_task, 
                            "uart_rx_decode", 
                            2048, 
                            (void *)signaling_sem, 
                            UART_HOUSEKEEPING_TASK_PRIORITY, 
                            &uart_housekeeping_task_hdl,
                            0);

    //Wait for the tasks to complete
    for (int i = 0; i < 2; i++) {
        xSemaphoreTake(signaling_sem, portMAX_DELAY);
    }

    //Delete the tasks
    vTaskDelete(class_driver_task_hdl);
    vTaskDelete(daemon_task_hdl);
}
