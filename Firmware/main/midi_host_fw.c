/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "esp_intr_alloc.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "usb/usb_host.h"

#include <string.h>

#include "knot_midi_uart.h"
#include "knot_midi_usb.h"

#define SW_AB_PIN 35
#define SW_MODE_PIN 36

#include "driver/gpio.h"

#define DAEMON_TASK_PRIORITY 3
#define CLASS_TASK_PRIORITY 4
#define LED_TASK_PRIORITY 2

#define UART_RX_TASK_PRIORITY 12

static const char* TAG = "DAEMON";

static void host_lib_daemon_task(void* arg) {
  SemaphoreHandle_t signaling_sem = (SemaphoreHandle_t)arg;

  ESP_LOGI(TAG, "Installing USB Host Library");
  usb_host_config_t host_config = {
      .skip_phy_setup = false,
      .intr_flags = ESP_INTR_FLAG_LEVEL1,
  };
  ESP_ERROR_CHECK(usb_host_install(&host_config));

  // Signal to the class driver task that the host library is installed
  xSemaphoreGive(signaling_sem);
  vTaskDelay(10); // Short delay to let client task spin up

  bool has_clients = true;
  bool has_devices = true;
  while (has_clients || has_devices) {
    uint32_t event_flags;
    ESP_ERROR_CHECK(usb_host_lib_handle_events(portMAX_DELAY, &event_flags));
    if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
      // has_clients = false;
    }
    if (event_flags & USB_HOST_LIB_EVENT_FLAGS_ALL_FREE) {
      // has_devices = false;
    }
    ESP_LOGI(TAG, "Event: %ld", event_flags);
  }
  ESP_LOGI(TAG, "No more clients and devices");

  // Uninstall the USB Host Library
  ESP_ERROR_CHECK(usb_host_uninstall());
  // Wait to be deleted
  xSemaphoreGive(signaling_sem);
  vTaskSuspend(NULL);
}

#include "rom/ets_sys.h" // For ets_printf
void grid_platform_printf(char const* fmt, ...) {

  va_list ap;

  char temp[200] = {0};

  va_start(ap, fmt);

  vsnprintf(temp, 199, fmt, ap);

  va_end(ap);

  ets_printf(temp);
}

#include "esp_random.h"
uint8_t grid_platform_get_random_8() {
  uint32_t random_number = esp_random();
  return random_number % 256;
}

void grid_platform_delay_ms(uint32_t delay_milliseconds) { ets_delay_us(delay_milliseconds * 1000); }

uint8_t grid_platform_get_adc_bit_depth(void) { return 12; }

#include "grid_ain.h"
#include "grid_led.h"
#include "grid_lua_api.h"
#include "grid_ui.h"

#include "grid_esp32_led.h"
#include "grid_esp32_nvm.h"

void knot_module_ui_init(struct grid_ain_model* ain, struct grid_led_model* led, struct grid_ui_model* ui) {

  // grid_ain_init(ain, 16, 5);

  grid_led_init(led, 3);
  grid_ui_model_init(ui, 0 + 1); // +1 for the system element

  for (uint8_t j = 0; j < 16; j++) {

    // grid_ui_element_init(ui, j, GRID_UI_ELEMENT_POTENTIOMETER);
  }

  grid_ui_element_init(ui, ui->element_list_length - 1, GRID_UI_ELEMENT_SYSTEM);

  // ui->lua_ui_init_callback = grid_lua_ui_init_po16;
}

void knot_lua_ui_init_knot(struct grid_lua_model* mod) {

  // define encoder_init_function

  // grid_lua_dostring(mod, GRID_LUA_P_META_init);

  // create element array
  grid_lua_dostring(mod, GRID_LUA_KW_ELEMENT_short "= {} ");

  // initialize 16 potmeter
  // grid_lua_dostring(mod, "for i=0, 15 do "GRID_LUA_KW_ELEMENT_short"[i] = {index = i} end");
  // grid_lua_dostring(mod, "for i=0, 15 do setmetatable("GRID_LUA_KW_ELEMENT_short"[i], potmeter_meta) end");

  grid_lua_gc_try_collect(mod);

  // initialize the system element
  grid_lua_dostring(mod, GRID_LUA_KW_ELEMENT_short "[0] = {index = 0}");
  grid_lua_dostring(mod, GRID_LUA_SYS_META_init);
  grid_lua_dostring(mod, "setmetatable(" GRID_LUA_KW_ELEMENT_short "[0], system_meta)");
}

void app_main(void) {

  // MIDI A/B SWITCH AND THROUGH BUTTON INTERACTIVITY
  gpio_set_direction(SW_AB_PIN, GPIO_MODE_INPUT);
  gpio_set_direction(SW_MODE_PIN, GPIO_MODE_INPUT);

  SemaphoreHandle_t nvm_or_port = xSemaphoreCreateBinary();
  xSemaphoreGive(nvm_or_port);

  ESP_LOGI(TAG, "===== MAIN START =====");

  // gpio_set_direction(GRID_ESP32_PINS_MAPMODE, GPIO_MODE_INPUT);
  // gpio_pullup_en(GRID_ESP32_PINS_MAPMODE);

  ESP_LOGI(TAG, "===== SYS START =====");
  // grid_sys_init(&grid_sys_state);

  ESP_LOGI(TAG, "===== UI INIT =====");

  knot_module_ui_init(&grid_ain_state, &grid_led_state, &grid_ui_state);
  grid_led_set_pin(&grid_led_state, 1);

  grid_led_set_layer_color(&grid_led_state, 0, 0, 0, 0, 0);
  grid_led_set_layer_color(&grid_led_state, 1, 0, 0, 0, 0);
  grid_led_set_layer_color(&grid_led_state, 2, 0, 0, 0, 0);

  grid_led_set_layer_color(&grid_led_state, 0, 1, 0, 0, 0);
  grid_led_set_layer_color(&grid_led_state, 1, 1, 0, 0, 0);
  grid_led_set_layer_color(&grid_led_state, 2, 1, 0, 0, 0);

  grid_led_set_layer_color(&grid_led_state, 0, 2, 0, 0, 0);
  grid_led_set_layer_color(&grid_led_state, 1, 2, 0, 0, 0);
  grid_led_set_layer_color(&grid_led_state, 2, 2, 0, 0, 0);

  // Create the class driver task
  TaskHandle_t led_task_hdl;
  xTaskCreatePinnedToCore(grid_esp32_led_task, // was led_task
                          "led", 4096, NULL, LED_TASK_PRIORITY, &led_task_hdl, 0);

  ESP_LOGI(TAG, "===== NVM START =====");

  xSemaphoreTake(nvm_or_port, 0);
  grid_esp32_nvm_init(&grid_esp32_nvm_state);

  if (gpio_get_level(SW_MODE_PIN) == 0) {

    grid_alert_all_set(&grid_led_state, GRID_LED_COLOR_YELLOW_DIM, 1000);
    grid_alert_all_set_frequency(&grid_led_state, 4);
    grid_esp32_nvm_erase(&grid_esp32_nvm_state);
    vTaskDelay(pdMS_TO_TICKS(1600));
  }

  xSemaphoreGive(nvm_or_port);

  ESP_LOGI(TAG, "===== LUA INIT =====");
  grid_lua_init(&grid_lua_state);
  grid_lua_set_memory_target(&grid_lua_state, 80); // 80kb
  grid_lua_start_vm(&grid_lua_state);
  grid_lua_dostring(&grid_lua_state, "foo = 123");
  knot_lua_ui_init_knot(&grid_lua_state);
  // grid_lua_dostring(&grid_lua_state, "print(foo, 2)");

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

  TaskHandle_t uart_rx_task_hdl;
  TaskHandle_t uart_housekeeping_task_hdl;
  // Create daemon task
  xTaskCreatePinnedToCore(host_lib_daemon_task, "daemon", 4096, (void*)signaling_sem, DAEMON_TASK_PRIORITY, &daemon_task_hdl, 0);
  // Create the class driver task
  xTaskCreatePinnedToCore(class_driver_task, "class", 4096, (void*)signaling_sem, CLASS_TASK_PRIORITY, &class_driver_task_hdl, 0);

  // Create a task to handler UART event from ISR

  vTaskDelay(10); // Add a short delay to let the tasks run

  knot_midi_uart_init(&knot_midi_uart_state);

  xTaskCreatePinnedToCore(knot_midi_uart_rx_task, "uart_rx", 2048, (void*)signaling_sem, UART_RX_TASK_PRIORITY, &uart_rx_task_hdl, 0);

  uint8_t last_button_state = 1;
  grid_led_set_layer_color(&grid_led_state, 2, GRID_LED_LAYER_UI_A, 0, 255, 0);

  while (1) {

    vTaskDelay(pdMS_TO_TICKS(10));

    knot_midi_uart_set_miditrsab_state(&knot_midi_uart_state, !gpio_get_level(SW_AB_PIN));

    uint8_t current_button_state = gpio_get_level(SW_MODE_PIN);

    if (last_button_state == 1 && current_button_state == 0) {

      if (knot_midi_uart_get_midithrough_state(&knot_midi_uart_state)) {
        knot_midi_uart_set_midithrough_state(&knot_midi_uart_state, false);
        grid_led_set_layer_color(&grid_led_state, 2, GRID_LED_LAYER_UI_A, 0, 255, 0);
      } else {
        knot_midi_uart_set_midithrough_state(&knot_midi_uart_state, true);
        grid_led_set_layer_color(&grid_led_state, 2, GRID_LED_LAYER_UI_A, 0, 0, 255);
      }
    }

    last_button_state = current_button_state;
  }
}
