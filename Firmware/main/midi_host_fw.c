#ifdef TEST_CDC_SERIAL

/*
 * SPDX-FileCopyrightText: 2015-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include "esp_err.h"
#include "esp_log.h"
#include "esp_system.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "usb/cdc_acm_host.h"
#include "usb/usb_host.h"

#define EXAMPLE_USB_HOST_PRIORITY (20)
#define EXAMPLE_USB_DEVICE_VID (0x303A)
#define EXAMPLE_USB_DEVICE_PID (0x8123)      // 0x303A:0x4001 (TinyUSB CDC device)
#define EXAMPLE_USB_DEVICE_DUAL_PID (0x4002) // 0x303A:0x4002 (TinyUSB Dual CDC device)
#define EXAMPLE_TX_STRING ("CDC test string!")
#define EXAMPLE_TX_TIMEOUT_MS (1000)

static const char* TAG = "USB-CDC";
static SemaphoreHandle_t device_disconnected_sem;

/**
 * @brief Data received callback
 *
 * @param[in] data     Pointer to received data
 * @param[in] data_len Length of received data in bytes
 * @param[in] arg      Argument we passed to the device open function
 * @return
 *   true:  We have processed the received data
 *   false: We expect more data
 */
static bool handle_rx(const uint8_t* data, size_t data_len, void* arg) {
  ESP_LOGI(TAG, "Data received");
  ESP_LOG_BUFFER_HEXDUMP(TAG, data, data_len, ESP_LOG_INFO);
  return true;
}

/**
 * @brief Device event callback
 *
 * Apart from handling device disconnection it doesn't do anything useful
 *
 * @param[in] event    Device event type and data
 * @param[in] user_ctx Argument we passed to the device open function
 */
static void handle_event(const cdc_acm_host_dev_event_data_t* event, void* user_ctx) {
  switch (event->type) {
  case CDC_ACM_HOST_ERROR:
    ESP_LOGE(TAG, "CDC-ACM error has occurred, err_no = %i", event->data.error);
    break;
  case CDC_ACM_HOST_DEVICE_DISCONNECTED:
    ESP_LOGI(TAG, "Device suddenly disconnected");
    ESP_ERROR_CHECK(cdc_acm_host_close(event->data.cdc_hdl));
    xSemaphoreGive(device_disconnected_sem);
    break;
  case CDC_ACM_HOST_SERIAL_STATE:
    ESP_LOGI(TAG, "Serial state notif 0x%04X", event->data.serial_state.val);
    break;
  case CDC_ACM_HOST_NETWORK_CONNECTION:
  default:
    ESP_LOGW(TAG, "Unsupported CDC event: %i", event->type);
    break;
  }
}

/**
 * @brief USB Host library handling task
 *
 * @param arg Unused
 */
static void usb_lib_task(void* arg) {
  while (1) {
    // Start handling system events
    uint32_t event_flags;
    usb_host_lib_handle_events(portMAX_DELAY, &event_flags);
    if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
      ESP_ERROR_CHECK(usb_host_device_free_all());
    }
    if (event_flags & USB_HOST_LIB_EVENT_FLAGS_ALL_FREE) {
      ESP_LOGI(TAG, "USB: All devices freed");
      // Continue handling USB events to allow device reconnection
    }
  }
}

/**
 * @brief Main application
 *
 * Here we open a USB CDC device and send some data to it
 */

#include "driver/gpio.h"

void app_main(void) {

#define USB_NATIVE_SELECT_PIN 11
#define USB_SOFT_SELECT_PIN 12

  gpio_set_direction(USB_NATIVE_SELECT_PIN, GPIO_MODE_OUTPUT);
  gpio_set_direction(USB_SOFT_SELECT_PIN, GPIO_MODE_OUTPUT);
  gpio_set_level(USB_NATIVE_SELECT_PIN, 1);
  gpio_set_level(USB_SOFT_SELECT_PIN, 1);

#define PMIC_EN_PIN 48

  gpio_set_direction(PMIC_EN_PIN, GPIO_MODE_OUTPUT);
  gpio_set_level(PMIC_EN_PIN, 1);

  device_disconnected_sem = xSemaphoreCreateBinary();
  assert(device_disconnected_sem);

  // Install USB Host driver. Should only be called once in entire application
  ESP_LOGI(TAG, "Installing USB Host");
  const usb_host_config_t host_config = {
      .skip_phy_setup = false,
      .intr_flags = ESP_INTR_FLAG_LEVEL1,
  };
  ESP_ERROR_CHECK(usb_host_install(&host_config));

  // Create a task that will handle USB library events
  BaseType_t task_created = xTaskCreate(usb_lib_task, "usb_lib", 4096, xTaskGetCurrentTaskHandle(), EXAMPLE_USB_HOST_PRIORITY, NULL);
  assert(task_created == pdTRUE);

  ESP_LOGI(TAG, "Installing CDC-ACM driver");
  ESP_ERROR_CHECK(cdc_acm_host_install(NULL));

  const cdc_acm_host_device_config_t dev_config = {.connection_timeout_ms = 1000, .out_buffer_size = 512, .in_buffer_size = 512, .user_arg = NULL, .event_cb = handle_event, .data_cb = handle_rx};

  while (true) {
    cdc_acm_dev_hdl_t cdc_dev = NULL;

    // Open USB device from tusb_serial_device example example. Either single or dual port configuration.
    ESP_LOGI(TAG, "Opening CDC ACM device 0x%04X:0x%04X...", EXAMPLE_USB_DEVICE_VID, EXAMPLE_USB_DEVICE_PID);
    esp_err_t err = cdc_acm_host_open(EXAMPLE_USB_DEVICE_VID, EXAMPLE_USB_DEVICE_PID, 0, &dev_config, &cdc_dev);
    if (ESP_OK != err) {
      ESP_LOGI(TAG, "Opening CDC ACM device 0x%04X:0x%04X...", EXAMPLE_USB_DEVICE_VID, EXAMPLE_USB_DEVICE_DUAL_PID);
      err = cdc_acm_host_open(EXAMPLE_USB_DEVICE_VID, EXAMPLE_USB_DEVICE_DUAL_PID, 0, &dev_config, &cdc_dev);
      if (ESP_OK != err) {
        ESP_LOGI(TAG, "Failed to open device");
        continue;
      }
    }
    cdc_acm_host_desc_print(cdc_dev);
    vTaskDelay(pdMS_TO_TICKS(100));

    // Test sending and receiving: responses are handled in handle_rx callback
    ESP_ERROR_CHECK(cdc_acm_host_data_tx_blocking(cdc_dev, (const uint8_t*)EXAMPLE_TX_STRING, strlen(EXAMPLE_TX_STRING), EXAMPLE_TX_TIMEOUT_MS));
    vTaskDelay(pdMS_TO_TICKS(100));

    // Test Line Coding commands: Get current line coding, change it 9600 7N1 and read again
    ESP_LOGI(TAG, "Setting up line coding");

    cdc_acm_line_coding_t line_coding;
    ESP_ERROR_CHECK(cdc_acm_host_line_coding_get(cdc_dev, &line_coding));
    ESP_LOGI(TAG, "Line Get: Rate: %" PRIu32 ", Stop bits: %" PRIu8 ", Parity: %" PRIu8 ", Databits: %" PRIu8 "", line_coding.dwDTERate, line_coding.bCharFormat, line_coding.bParityType,
             line_coding.bDataBits);

    line_coding.dwDTERate = 9600;
    line_coding.bDataBits = 7;
    line_coding.bParityType = 1;
    line_coding.bCharFormat = 1;
    ESP_ERROR_CHECK(cdc_acm_host_line_coding_set(cdc_dev, &line_coding));
    ESP_LOGI(TAG, "Line Set: Rate: %" PRIu32 ", Stop bits: %" PRIu8 ", Parity: %" PRIu8 ", Databits: %" PRIu8 "", line_coding.dwDTERate, line_coding.bCharFormat, line_coding.bParityType,
             line_coding.bDataBits);

    ESP_ERROR_CHECK(cdc_acm_host_line_coding_get(cdc_dev, &line_coding));
    ESP_LOGI(TAG, "Line Get: Rate: %" PRIu32 ", Stop bits: %" PRIu8 ", Parity: %" PRIu8 ", Databits: %" PRIu8 "", line_coding.dwDTERate, line_coding.bCharFormat, line_coding.bParityType,
             line_coding.bDataBits);

    ESP_ERROR_CHECK(cdc_acm_host_set_control_line_state(cdc_dev, true, false));

    // We are done. Wait for device disconnection and start over
    ESP_LOGI(TAG, "Example finished successfully! You can reconnect the device to run again.");
    xSemaphoreTake(device_disconnected_sem, portMAX_DELAY);
  }
}

#else

#include "esp_intr_alloc.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "usb/usb_host.h"

// must be last freertos relevant header to avoid #error
#include "esp_freertos_hooks.h"

#include <string.h>

#include "knot_midi_queue.h"
#include "knot_midi_uart.h"
#include "knot_midi_usb.h"

#define SW_AB_PIN 35
#define SW_MODE_PIN 36

#include "driver/gpio.h"

#define DAEMON_TASK_PRIORITY 3
#define CLASS_TASK_PRIORITY 4
#define LED_TASK_PRIORITY 2

#define UART_RX_TASK_PRIORITY 0
#define UART_TX_TASK_PRIORITY 0

#define USB_RX_TASK_PRIORITY 0
#define USB_TX_TASK_PRIORITY 0

static const char* TAG = "DAEMON";

void midi_config_file_update(uint8_t state) {

  ESP_LOGI(TAG, "Midi through set: %d", state);

  FILE* filePointer;        // Pointer to file type
  char dataToWrite[] = "0"; // Array to hold data to be written

  if (state) {
    dataToWrite[0] = '1';
  }

  // Open file in write mode
  filePointer = fopen("/littlefs/midithrough.cfg", "w");

  // Check if file opened successfully
  if (filePointer == NULL) {
    printf("Unable to open file.\n");
    return; // Exit program with error code
  }

  // Write data to file
  fputs(dataToWrite, filePointer);

  // Close file
  fclose(filePointer);
}

uint8_t /*is_midi_through_enabled*/ midi_config_file_read(void) {

  FILE* filePointer; // Pointer to file type
  char firstCharacter;

  // Open file in read mode
  filePointer = fopen("/littlefs/midithrough.cfg", "r");

  // Check if file opened successfully
  if (filePointer == NULL) {
    printf("Unable to open file.\n");
    return 0; // Midi through not enabled
  }

  // Read the first character from the file
  firstCharacter = fgetc(filePointer);

  // Check if the end of the file or an error occurred
  if (firstCharacter == EOF) {
    printf("Error reading from file or file is empty.\n");
    fclose(filePointer); // Close file before exiting
    return 0;            // Midi through not enabled
  }

  // Display the first character
  printf("The first character in the file is: %c\n", firstCharacter);

  // Close file
  fclose(filePointer);

  return firstCharacter - '0';
}

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
    portYIELD();
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

#include "knot_platform.h"

#include "grid_ain.h"
#include "grid_led.h"
#include "grid_lua.h"
#include "grid_ui.h"
#include "grid_ui_system.h"

#include "grid_esp32_led.h"
#include "grid_esp32_nvm.h"

void knot_module_ui_init(struct grid_ain_model* ain, struct grid_led_model* led, struct grid_ui_model* ui) {

  // grid_ain_init(ain, 16, 5);

  grid_led_init(led, 3);
  grid_ui_model_init(ui, 0 + 1); // +1 for the system element

  for (uint8_t j = 0; j < 16; j++) {

    // grid_ui_element_init(ui, j, GRID_UI_ELEMENT_POTENTIOMETER);
  }

  grid_ui_element_system_init(&ui->element_list[ui->element_list_length - 1]);

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

extern esp_err_t try_start_in_transfer(void);

bool idle_hook(void) {

  portYIELD();
  return 0; // return 1 causes one trigger / rtos tick
}

void app_main(void) {

  // MIDI A/B SWITCH AND THROUGH BUTTON INTERACTIVITY
  gpio_set_direction(SW_AB_PIN, GPIO_MODE_INPUT);
  gpio_pullup_en(SW_AB_PIN);
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
                          "led", 4096, NULL, LED_TASK_PRIORITY, &led_task_hdl, 1);

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
  grid_lua_init(&grid_lua_state, NULL, NULL);
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
  TaskHandle_t uart_tx_task_hdl;
  TaskHandle_t uart_housekeeping_task_hdl;
  // Create daemon task
  xTaskCreatePinnedToCore(host_lib_daemon_task, "daemon", 4096, (void*)signaling_sem, DAEMON_TASK_PRIORITY, &daemon_task_hdl, 1);
  // Create the class driver task
  xTaskCreatePinnedToCore(class_driver_task, "class", 4096, (void*)signaling_sem, CLASS_TASK_PRIORITY, &class_driver_task_hdl, 1);

  // Create a task to handler UART event from ISR

  vTaskDelay(10); // Add a short delay to let the tasks run

  knot_midi_uart_init(&knot_midi_uart_state);

  grid_esp32_nvm_list_files(&grid_esp32_nvm_state, "/littlefs");

  if (midi_config_file_read()) {
    ESP_LOGI(TAG, "Midi through enabled");
    knot_midi_uart_set_midithrough_state(&knot_midi_uart_state, true);
    grid_led_set_layer_color(&grid_led_state, 2, GRID_LED_LAYER_UI_A, 0, 0, 255); // blue
  } else {
    ESP_LOGI(TAG, "Midi through disabled");
    knot_midi_uart_set_midithrough_state(&knot_midi_uart_state, false);
    grid_led_set_layer_color(&grid_led_state, 2, GRID_LED_LAYER_UI_A, 0, 255, 0); // green
  }

  xTaskCreatePinnedToCore(knot_midi_uart_rx_task, "uart_rx", 4096, (void*)signaling_sem, UART_RX_TASK_PRIORITY, &uart_rx_task_hdl, 1);
  xTaskCreatePinnedToCore(knot_midi_uart_tx_task, "uart_tx", 4096, (void*)signaling_sem, UART_TX_TASK_PRIORITY, &uart_tx_task_hdl, 1);

  xTaskCreatePinnedToCore(knot_midi_usb_rx_task, "usb_rx", 4096, (void*)signaling_sem, USB_RX_TASK_PRIORITY, &uart_rx_task_hdl, 1);
  xTaskCreatePinnedToCore(knot_midi_usb_tx_task, "usb_tx", 4096, (void*)signaling_sem, USB_TX_TASK_PRIORITY, &uart_tx_task_hdl, 1);

  // Register idle hook to force yield from idle task to lowest priority task
  esp_register_freertos_idle_hook_for_cpu(idle_hook, 0);
  esp_register_freertos_idle_hook_for_cpu(idle_hook, 1);

  uint8_t last_button_state = 1;

  UBaseType_t highwatermark = uxTaskGetStackHighWaterMark(xTaskGetCurrentTaskHandle());
  ESP_LOGI(TAG, "highwatermark before main loop: %d", highwatermark);

  while (1) {

    vTaskDelay(pdMS_TO_TICKS(10));

    knot_midi_uart_set_miditrsab_state(&knot_midi_uart_state, !gpio_get_level(SW_AB_PIN));

    uint8_t current_button_state = gpio_get_level(SW_MODE_PIN);

    if (last_button_state == 1 && current_button_state == 0) {

      if (knot_midi_uart_get_midithrough_state(&knot_midi_uart_state)) {
        knot_midi_uart_set_midithrough_state(&knot_midi_uart_state, false);
        grid_led_set_layer_color(&grid_led_state, 2, GRID_LED_LAYER_UI_A, 0, 255, 0);
        midi_config_file_update(false);

      } else {
        knot_midi_uart_set_midithrough_state(&knot_midi_uart_state, true);
        grid_led_set_layer_color(&grid_led_state, 2, GRID_LED_LAYER_UI_A, 0, 0, 255);
        midi_config_file_update(true);
      }
    }

    last_button_state = current_button_state;
  }
}

#endif
