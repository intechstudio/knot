/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "knot_midi_usb.h"

#include "esp_log.h"

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "grid_led.h"
#include "usb/usb_host.h"
#include <stdlib.h>
#include <string.h>

#include "knot_midi_queue.h"
#include "knot_midi_translator.h"
#include "knot_midi_uart.h"

#include "rom/ets_sys.h" // For ets_printf

#define CLIENT_NUM_EVENT_MSG 5

#define ACTION_OPEN_DEV 0x01
#define ACTION_GET_DEV_INFO 0x02
#define ACTION_GET_DEV_DESC 0x04
#define ACTION_GET_CONFIG_DESC 0x08
#define ACTION_GET_STR_DESC 0x10
#define ACTION_CLOSE_DEV 0x20
#define ACTION_EXIT 0x40

static const char* TAG = "KNOT_MIDI_USB";

static void client_event_cb(const usb_host_client_event_msg_t* event_msg, void* arg) {

  class_driver_t* driver_obj = (class_driver_t*)arg;

  ets_printf("client event %d\r\n", event_msg->event);

  switch (event_msg->event) {
  case USB_HOST_CLIENT_EVENT_NEW_DEV:
    if (driver_obj->dev_addr == 0) {
      driver_obj->dev_addr = event_msg->new_dev.address;
      // Open the device next
      driver_obj->actions |= ACTION_OPEN_DEV;
    }
    break;
  case USB_HOST_CLIENT_EVENT_DEV_GONE:
    if (driver_obj->dev_hdl != NULL) {
      // Cancel any other actions and close the device next
      driver_obj->actions = ACTION_CLOSE_DEV;
    }
    break;
  default:
    // Should never occur
    abort();
  }
}

static void action_open_dev(class_driver_t* driver_obj) {
  assert(driver_obj->dev_addr != 0);
  ESP_LOGI(TAG, "Opening device at address %d", driver_obj->dev_addr);
  ESP_ERROR_CHECK(usb_host_device_open(driver_obj->client_hdl, driver_obj->dev_addr, &driver_obj->dev_hdl));
  // Get the device's information next
  driver_obj->actions &= ~ACTION_OPEN_DEV;
  driver_obj->actions |= ACTION_GET_DEV_INFO;
}

static void action_get_info(class_driver_t* driver_obj) {
  assert(driver_obj->dev_hdl != NULL);
  ESP_LOGI(TAG, "Getting device information");
  usb_device_info_t dev_info;
  ESP_ERROR_CHECK(usb_host_device_info(driver_obj->dev_hdl, &dev_info));
  ESP_LOGI(TAG, "\t%s speed", (dev_info.speed == USB_SPEED_LOW) ? "Low" : "Full");
  ESP_LOGI(TAG, "\tbConfigurationValue %d", dev_info.bConfigurationValue);
  // Todo: Print string descriptors

  // Get the device descriptor next
  driver_obj->actions &= ~ACTION_GET_DEV_INFO;
  driver_obj->actions |= ACTION_GET_DEV_DESC;
}

unsigned long loopcounter = 0;

static esp_err_t midi_find_intf_and_ep_desc(class_driver_t* driver_obj, const usb_ep_desc_t** in_ep, const usb_ep_desc_t** out_ep, int* intf_num) {
  bool interface_found = false;
  const usb_config_desc_t* config_desc;
  const usb_device_desc_t* device_desc;
  int intf_idx = -1;
  int desc_offset = 0;

  // Get required descriptors
  ESP_ERROR_CHECK(usb_host_get_device_descriptor(driver_obj->dev_hdl, &device_desc));
  ESP_ERROR_CHECK(usb_host_get_active_config_descriptor(driver_obj->dev_hdl, &config_desc));

#define USB_SUBCLASS_NULL 0x00
#define USB_SUBCLASS_COMMON 0x02

#define USB_PROTOCOL_NULL 0x00
#define USB_DEVICE_PROTOCOL_IAD 0x01

  if (((device_desc->bDeviceClass == USB_CLASS_MISC) && (device_desc->bDeviceSubClass == USB_SUBCLASS_COMMON) && (device_desc->bDeviceProtocol == USB_DEVICE_PROTOCOL_IAD)) ||
      ((device_desc->bDeviceClass == USB_CLASS_PER_INTERFACE) && (device_desc->bDeviceSubClass == USB_SUBCLASS_NULL) && (device_desc->bDeviceProtocol == USB_PROTOCOL_NULL))) {

    ESP_LOGI(TAG, "IAD");

    const usb_standard_desc_t* this_desc = (const usb_standard_desc_t*)config_desc;
    do {

      ESP_LOGI(TAG, "TRY");
      intf_idx++;

      this_desc = usb_parse_next_descriptor_of_type(this_desc, config_desc->wTotalLength, USB_B_DESCRIPTOR_TYPE_INTERFACE, &desc_offset);

      if (this_desc == NULL) {
        ESP_LOGI(TAG, "INTERFACE NOT FOUND");
        break; // Reached end of configuration descriptor
      }

      const usb_intf_desc_t* intf_desc = (const usb_intf_desc_t*)this_desc;

      ESP_LOGI(TAG, "Interface %d %d %d %d", intf_desc->bInterfaceNumber, intf_desc->bInterfaceClass, intf_desc->bInterfaceSubClass, intf_desc->bInterfaceProtocol);

#define USB_SUBCLASS_MIDISTREAMING 0x03
      if (intf_desc->bInterfaceClass == USB_CLASS_AUDIO && intf_desc->bInterfaceSubClass == USB_SUBCLASS_MIDISTREAMING && intf_desc->bInterfaceProtocol == 0x00) {

        ESP_LOGI(TAG, "MIDI INTERFACE FOUND %d", intf_desc->bInterfaceNumber);
        *intf_num = intf_desc->bInterfaceNumber;
        interface_found = true;
      }

    } while (!interface_found);

    if (interface_found) {

      usb_intf_desc_t* intf_desc = usb_parse_interface_descriptor(config_desc, *intf_num, 0, &desc_offset);

      ESP_LOGI(TAG, "wTotal, offset: 0x%lx %d %d", (unsigned long int)intf_desc, config_desc->wTotalLength, desc_offset);

      int temp_offset = desc_offset;
      for (int i = 0; i < 2; i++) {
        const usb_ep_desc_t* this_ep = usb_parse_endpoint_descriptor_by_index(intf_desc, i, config_desc->wTotalLength, &desc_offset);
        assert(this_ep);
        if (USB_EP_DESC_GET_EP_DIR(this_ep)) {
          *in_ep = this_ep;
          ESP_LOGI(TAG, "IN EP FOUND %d", this_ep->bEndpointAddress);
        } else {
          *out_ep = this_ep;
          ESP_LOGI(TAG, "OUT EP FOUND %d", this_ep->bEndpointAddress);
        }
        desc_offset = temp_offset;
      }

      return ESP_OK;
    }
  } else {
    ESP_LOGI(TAG, "NOT IAD");

    const usb_standard_desc_t* this_desc = (const usb_standard_desc_t*)config_desc;
    do {

      ESP_LOGI(TAG, "TRY");
      intf_idx++;

      this_desc = usb_parse_next_descriptor_of_type(this_desc, config_desc->wTotalLength, USB_B_DESCRIPTOR_TYPE_INTERFACE, &desc_offset);

      if (this_desc == NULL) {
        ESP_LOGI(TAG, "INTERFACE NOT FOUND");
        break; // Reached end of configuration descriptor
      }

      const usb_intf_desc_t* intf_desc = (const usb_intf_desc_t*)this_desc;

#define USB_SUBCLASS_MIDISTREAMING 0x03
      if (intf_desc->bInterfaceClass == USB_CLASS_AUDIO && intf_desc->bInterfaceSubClass == USB_SUBCLASS_MIDISTREAMING && intf_desc->bInterfaceProtocol == 0x00) {

        ESP_LOGI(TAG, "MIDI INTERFACE FOUND %d", intf_idx);
        *intf_num = intf_idx;
        interface_found = true;
      }

    } while (!interface_found);

    if (interface_found) {

      usb_intf_desc_t* intf_desc = usb_parse_interface_descriptor(config_desc, intf_idx, 0, &desc_offset);

      int temp_offset = desc_offset;
      for (int i = 0; i < 2; i++) {
        const usb_ep_desc_t* this_ep = usb_parse_endpoint_descriptor_by_index(intf_desc, i, config_desc->wTotalLength, &desc_offset);
        assert(this_ep);
        if (USB_EP_DESC_GET_EP_DIR(this_ep)) {
          *in_ep = this_ep;
          ESP_LOGI(TAG, "IN EP FOUND %d", this_ep->bEndpointAddress);
        } else {
          *out_ep = this_ep;
          ESP_LOGI(TAG, "OUT EP FOUND %d", this_ep->bEndpointAddress);
        }
        desc_offset = temp_offset;
      }

      return ESP_OK;
    }
  }

  return ESP_ERR_NOT_FOUND;
}

static usb_transfer_t* out_transfer;

static usb_transfer_t* in_transfer;

static bool is_transfer_completed(usb_transfer_t* transfer) {
  bool completed = false;

  switch (transfer->status) {
  case USB_TRANSFER_STATUS_COMPLETED:
    completed = true;
    break;
  case USB_TRANSFER_STATUS_NO_DEVICE: // User is notified about device disconnection from usb_event_cb
  case USB_TRANSFER_STATUS_CANCELED:
    break;
  case USB_TRANSFER_STATUS_ERROR:
  case USB_TRANSFER_STATUS_TIMED_OUT:
  case USB_TRANSFER_STATUS_STALL:
  case USB_TRANSFER_STATUS_OVERFLOW:
  case USB_TRANSFER_STATUS_SKIPPED:
  default:
  }
  return completed;
}

volatile uint8_t DRAM_ATTR usb_in_ready = 0;

static portMUX_TYPE in_mutex = portMUX_INITIALIZER_UNLOCKED;
#define CDC_ACM_ENTER_CRITICAL() portENTER_CRITICAL(&in_mutex)
#define CDC_ACM_EXIT_CRITICAL() portEXIT_CRITICAL(&in_mutex)
esp_err_t try_start_in_transfer(void) {
  CDC_ACM_ENTER_CRITICAL();

  if (usb_in_ready == 0) {

    CDC_ACM_EXIT_CRITICAL();
    return -1;
  }

  esp_err_t err = usb_host_transfer_submit(in_transfer);
  usb_in_ready = 0;
  CDC_ACM_EXIT_CRITICAL();

  // printf("S\n");
  return err;
}

static void in_transfer_cb(usb_transfer_t* in_transfer) {
  // CDC_ACM_ENTER_CRITICAL();
  printf("TIME: %lld IN: Transfer status %d, actual number of bytes transferred %d\n", esp_timer_get_time(), in_transfer->status, in_transfer->actual_num_bytes);

  struct usb_midi_event_packet usb_ev = {.byte0 = in_transfer->data_buffer[0], .byte1 = in_transfer->data_buffer[1], .byte2 = in_transfer->data_buffer[2], .byte3 = in_transfer->data_buffer[3]};

  struct uart_midi_event_packet uart_ev = usb_midi_to_uart(usb_ev);

  knot_midi_queue_trsout_push(uart_ev);

  // usb_in_ready = 1;
  // CDC_ACM_EXIT_CRITICAL();
  usb_host_transfer_submit(in_transfer);

  ets_printf("IN: %d %d %d %d\n", esp_timer_get_time(), in_transfer->status, in_transfer->data_buffer[0], in_transfer->data_buffer[1], in_transfer->data_buffer[2], in_transfer->data_buffer[3]);
}

volatile uint8_t DRAM_ATTR usb_out_ready = 1;

static void IRAM_ATTR out_transfer_cb(usb_transfer_t* out_transfer) {
  // ets_printf("Ready\n");
  usb_out_ready = 1;

  struct usb_midi_event_packet usb_ev = {0, 0, 0, 0};

  int status = knot_midi_queue_usbout_pop(&usb_ev);

  if (status == 1) {
    return;
  }

  knot_midi_usb_send_packet(usb_ev);
}

uint8_t IRAM_ATTR knot_midi_usb_out_isready(void) { return usb_out_ready; }

int IRAM_ATTR knot_midi_usb_send_packet(struct usb_midi_event_packet ev) {

  if (out_transfer == NULL) {
    return 1;
  }

  out_transfer->num_bytes = 4;

  out_transfer->data_buffer[0] = ev.byte0;
  out_transfer->data_buffer[1] = ev.byte1;
  out_transfer->data_buffer[2] = ev.byte2;
  out_transfer->data_buffer[3] = ev.byte3;
  out_transfer->callback = out_transfer_cb;

  usb_out_ready = 0;
  esp_err_t err = usb_host_transfer_submit(out_transfer);

  return err;
}

static void action_get_dev_desc(class_driver_t* driver_obj) {
  assert(driver_obj->dev_hdl != NULL);
  ESP_LOGI(TAG, "Getting device descriptor");
  const usb_device_desc_t* dev_desc;
  ESP_ERROR_CHECK(usb_host_get_device_descriptor(driver_obj->dev_hdl, &dev_desc));
  usb_print_device_descriptor(dev_desc);
  // Get the device's config descriptor next
  driver_obj->actions &= ~ACTION_GET_DEV_DESC;
  driver_obj->actions |= ACTION_GET_CONFIG_DESC;
}

static void action_get_config_desc(class_driver_t* driver_obj) {
  assert(driver_obj->dev_hdl != NULL);
  ESP_LOGI(TAG, "Getting config descriptor");
  const usb_config_desc_t* config_desc;
  ESP_ERROR_CHECK(usb_host_get_active_config_descriptor(driver_obj->dev_hdl, &config_desc));
  usb_print_config_descriptor(config_desc, NULL);

  const usb_ep_desc_t* in_ep = NULL;
  const usb_ep_desc_t* out_ep = NULL;
  int intf_num = -1;
  midi_find_intf_and_ep_desc(driver_obj, &in_ep, &out_ep, &intf_num);

  if (intf_num != -1) {

    ESP_LOGI(TAG, "SUKU claim interface %d nice", intf_num);
    ESP_ERROR_CHECK(usb_host_interface_claim(driver_obj->client_hdl, driver_obj->dev_hdl, intf_num, 0));

    driver_obj->claimed_interface = intf_num;

    // Send an OUT transfer to EP3
    if (out_transfer == NULL) {
      usb_host_transfer_alloc(4, 0, &out_transfer);
    }
    memset(out_transfer->data_buffer, 0xAA, 4);
    out_transfer->num_bytes = 4;
    out_transfer->device_handle = driver_obj->dev_hdl;
    out_transfer->bEndpointAddress = out_ep->bEndpointAddress;
    out_transfer->callback = out_transfer_cb;
    out_transfer->context = (void*)&driver_obj;
    ESP_LOGI(TAG, "Setup OUT transfer on ep %02x nice", out_ep->bEndpointAddress);
    // usb_host_transfer_submit(in_transfer);

    // SETUP IN TRANSFER
    if (in_transfer == NULL) {
      usb_host_transfer_alloc(USB_EP_DESC_GET_MPS(in_ep), 0, &in_transfer);
    }
    memset(in_transfer->data_buffer, 0, 4);
    in_transfer->num_bytes = USB_EP_DESC_GET_MPS(in_ep);
    in_transfer->device_handle = driver_obj->dev_hdl;
    in_transfer->bEndpointAddress = in_ep->bEndpointAddress;
    in_transfer->timeout_ms = 5000;
    in_transfer->callback = in_transfer_cb;
    in_transfer->context = (void*)&driver_obj;

    ESP_LOGI(TAG, "SUKU start IN transfer on ep %02x nice", in_ep->bEndpointAddress);
    usb_host_transfer_submit(in_transfer);

    // Get the device's string descriptors next
    driver_obj->actions &= ~ACTION_GET_CONFIG_DESC;
    driver_obj->actions |= ACTION_GET_STR_DESC;

  } else {
    ESP_LOGI(TAG, "Iface not found, try later!");
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

static void action_get_str_desc(class_driver_t* driver_obj) {

  if (loopcounter == 0) {
    assert(driver_obj->dev_hdl != NULL);
    usb_device_info_t dev_info;
    ESP_ERROR_CHECK(usb_host_device_info(driver_obj->dev_hdl, &dev_info));
    if (dev_info.str_desc_manufacturer) {
      ESP_LOGI(TAG, "Getting Manufacturer string descriptor");
      usb_print_string_descriptor(dev_info.str_desc_manufacturer);
    }
    if (dev_info.str_desc_product) {
      ESP_LOGI(TAG, "Getting Product string descriptor");
      usb_print_string_descriptor(dev_info.str_desc_product);
    }
    if (dev_info.str_desc_serial_num) {
      ESP_LOGI(TAG, "Getting Serial Number string descriptor");
      usb_print_string_descriptor(dev_info.str_desc_serial_num);
    }
  }

  // if (loopcounter%10 == 1){

  //     usb_host_transfer_submit(transfer);

  // }

  loopcounter++;

  // Nothing to do until the device disconnects
  // driver_obj->actions &= ~ACTION_GET_STR_DESC;
}

static void aciton_close_dev(class_driver_t* driver_obj) {

  ESP_LOGI(TAG, "action_close_dev");
  out_transfer = NULL;

  if (driver_obj->claimed_interface != -1) {

    ESP_LOGI(TAG, "release interface %lx %lx %d", (long unsigned int)driver_obj->client_hdl, (long unsigned int)driver_obj->client_hdl, driver_obj->claimed_interface);
    usb_host_interface_release(driver_obj->client_hdl, driver_obj->dev_hdl, driver_obj->claimed_interface);
    driver_obj->claimed_interface = -1;
  }

  ESP_LOGI(TAG, "close");

  ESP_ERROR_CHECK(usb_host_device_close(driver_obj->client_hdl, driver_obj->dev_hdl));
  driver_obj->dev_hdl = NULL;
  driver_obj->dev_addr = 0;

  ESP_ERROR_CHECK(usb_host_client_deregister(driver_obj->client_hdl));

  driver_obj->client_hdl = NULL;

  ESP_LOGI(TAG, "close: %lx %lx", (long unsigned int)driver_obj->dev_hdl, (long unsigned int)driver_obj->client_hdl);

  // We need to exit the event handler loop
  driver_obj->actions &= ~ACTION_CLOSE_DEV;
  driver_obj->actions |= ACTION_OPEN_DEV;

  // driver_obj->actions |= ACTION_EXIT;
}

void class_driver_task(void* arg) {
  SemaphoreHandle_t signaling_sem = (SemaphoreHandle_t)arg;
  class_driver_t driver_obj = {0};
  driver_obj.claimed_interface = -1;

  in_transfer = NULL;
  out_transfer = NULL;

  // Wait until daemon task has installed USB Host Library
  xSemaphoreTake(signaling_sem, portMAX_DELAY);

  grid_alert_all_set(&grid_led_state, 100, 100, 100, -1);

  while (1) {

    // ESP_LOGI(TAG, "actions: %ld, loopcounter: %ld", driver_obj.actions, loopcounter);

    while (driver_obj.client_hdl == NULL) {

      ESP_LOGI(TAG, "Try Registering Client");
      usb_host_client_config_t client_config = {
          .is_synchronous = false, // Synchronous clients currently not supported. Set this to false
          .max_num_event_msg = CLIENT_NUM_EVENT_MSG,
          .async =
              {
                  .client_event_callback = client_event_cb,
                  .callback_arg = (void*)&driver_obj,
              },
      };
      ESP_ERROR_CHECK(usb_host_client_register(&client_config, &driver_obj.client_hdl));

      vTaskDelay(pdMS_TO_TICKS(1));
    }

    usb_host_client_handle_events(driver_obj.client_hdl, 10);

    if (driver_obj.actions & ACTION_OPEN_DEV) {
      if (driver_obj.dev_addr != 0) {

        // stop waiting pulse animation
        grid_alert_all_set(&grid_led_state, 0, 0, 0, 0);

        action_open_dev(&driver_obj);
      } else {
        ESP_LOGI(TAG, "Pending");
      }
    }
    if (driver_obj.actions & ACTION_GET_DEV_INFO) {
      action_get_info(&driver_obj);
    }
    if (driver_obj.actions & ACTION_GET_DEV_DESC) {
      action_get_dev_desc(&driver_obj);
      // led_connect_effect_start();
    }
    if (driver_obj.actions & ACTION_GET_CONFIG_DESC) {
      action_get_config_desc(&driver_obj);
    }
    if (driver_obj.actions & ACTION_GET_STR_DESC) {
      action_get_str_desc(&driver_obj);
    }
    if (driver_obj.actions & ACTION_CLOSE_DEV) {
      aciton_close_dev(&driver_obj);
      grid_alert_all_set(&grid_led_state, 100, 100, 100, -1);
    }
    if (driver_obj.actions & ACTION_EXIT) {
      break;
    }
    portYIELD();
  }

  ESP_LOGI(TAG, "Deregistering Client");
  ESP_ERROR_CHECK(usb_host_client_deregister(driver_obj.client_hdl));

  // Wait to be deleted
  xSemaphoreGive(signaling_sem);
  vTaskSuspend(NULL);
}

void knot_midi_usb_rx_task(void* arg) {
  while (1) {

    try_start_in_transfer();

    portYIELD();
  }
}

void knot_midi_usb_tx_task(void* arg) {

  while (1) {

    if (knot_midi_usb_out_isready()) {

      struct usb_midi_event_packet usb_midi_ev = {0};

      if (0 == knot_midi_queue_usbout_pop(&usb_midi_ev)) {

        int status = knot_midi_usb_send_packet(usb_midi_ev);

        if (status == 0) {
          // ets_printf("USB OUT: %d %d %d %d", usb_midi_ev.byte0, usb_midi_ev.byte1, usb_midi_ev.byte2, usb_midi_ev.byte3);
        } else if (status == 1) {
          // ets_printf("USB not connected");
        } else {
          // ets_printf("USB error: %d", status);
        }
      }
    }

    portYIELD();
  }
}
