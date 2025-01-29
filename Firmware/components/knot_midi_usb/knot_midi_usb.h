/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "grid_led.h"
#include "usb/usb_host.h"
#include <stdlib.h>
#include <string.h>

#include "knot_midi_translator.h"

typedef struct {
  usb_host_client_handle_t client_hdl;
  uint8_t dev_addr;
  usb_device_handle_t dev_hdl;
  uint32_t actions;
  int claimed_interface;
} class_driver_t;

uint8_t IRAM_ATTR knot_midi_usb_out_isready(void);

int IRAM_ATTR knot_midi_usb_send_packet(struct usb_midi_event_packet ev);
void class_driver_task(void* arg);

void knot_midi_usb_rx_task(void* arg);
void knot_midi_usb_tx_task(void* arg);
