/*
 * SPDX-FileCopyrightText: 2021-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */

#include "knot_midi_uart.h"

#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/ringbuf.h"
#include "freertos/semphr.h"
#include <stdlib.h>
#include <string.h>

#include "driver/gpio.h"

#include "rom/ets_sys.h" // For ets_printf

#include "driver/uart.h"
#include "freertos/queue.h"
#include "knot_midi_translator.h"
#include "knot_midi_usb.h"

struct knot_midi_uart_model knot_midi_uart_state;

static const char* TAG = "KNOT_MIDI_UART";

uint8_t knot_midi_uart_get_midithrough_state(struct knot_midi_uart_model* midi_uart) { return midi_uart->midi_through_state; }
void knot_midi_uart_set_midithrough_state(struct knot_midi_uart_model* midi_uart, uint8_t state) { midi_uart->midi_through_state = state; }

uint8_t knot_midi_uart_get_miditrsab_state(struct knot_midi_uart_model* midi_uart) { return midi_uart->midi_trs_ab_state; }
void knot_midi_uart_set_miditrsab_state(struct knot_midi_uart_model* midi_uart, uint8_t state) {
  gpio_set_level(TRS_TX_AB_SELECT, state);
  midi_uart->midi_trs_ab_state = state;
}

static void midi_uart_init(QueueHandle_t* uart_queue) {

  /* Configure parameters of an UART driver,
   * communication pins and install the driver */
  uart_config_t uart_config = {
      .baud_rate = 31250,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .source_clk = UART_SCLK_DEFAULT,
  };
  // Install UART driver, and get the queue.
  uart_driver_install(EX_UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 20, uart_queue, 0);
  uart_param_config(EX_UART_NUM, &uart_config);

  uart_set_pin(EX_UART_NUM, 17, 18, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);

  uart_set_line_inverse(EX_UART_NUM, UART_SIGNAL_TXD_INV);
}

void knot_midi_uart_init(struct knot_midi_uart_model* midi_uart) {

  memset(midi_uart, 0, sizeof(struct knot_midi_uart_model));
  midi_uart->current_is_sysex = false;
  midi_uart->midi_through_state = false;
  midi_uart->midi_trs_ab_state = false;

  ESP_LOGI(TAG, "UART init");

  midi_uart_init(&midi_uart->uart0_queue);

  gpio_set_direction(TRS_TX_AB_SELECT, GPIO_MODE_OUTPUT);
  gpio_set_level(TRS_TX_AB_SELECT, 0);

// Channel Voice Message Buffer
#define UART_RX_BUFFER_CVM_SIZE 1600
  midi_uart->uart_rx_buffer_cvm = xRingbufferCreate(UART_RX_BUFFER_CVM_SIZE, RINGBUF_TYPE_NOSPLIT);
  if (midi_uart->uart_rx_buffer_cvm == NULL) {
    ets_printf("Failed to create ring buffer\n");
  }

// Real-Time Message Buffer
#define UART_RX_BUFFER_RTM_SIZE 20
  midi_uart->uart_rx_buffer_rtm = xRingbufferCreate(UART_RX_BUFFER_RTM_SIZE, RINGBUF_TYPE_NOSPLIT);
  if (midi_uart->uart_rx_buffer_rtm == NULL) {
    ets_printf("Failed to create ring buffer\n");
  }
}

int knot_midi_uart_send_packet(struct uart_midi_event_packet ev) {

  grid_alert_one_set(&grid_led_state, 1, 200, 200, 200, 30);

  // led_tx_effect_start();

  const int txBytes = uart_write_bytes(EX_UART_NUM, &ev.byte1, ev.length);
  // printf("MIDI: %d : %d %d %d\n", ev.length, ev.byte1, ev.byte2, ev.byte3);
  // ESP_LOGI(logName, "Wrote %d bytes %d %d %d", txBytes, data[0], data[1], data[2]);
  return txBytes;
}

void knot_midi_uart_rx_task(void* arg) {

  SemaphoreHandle_t signaling_sem = (SemaphoreHandle_t)arg;
  // xSemaphoreTake(signaling_sem, portMAX_DELAY);

  uart_event_t event;
  size_t buffered_size;
  uint8_t* dtmp = (uint8_t*)malloc(RD_BUF_SIZE);

  ESP_LOGI(TAG, "UART RX init done");

  for (;;) {

    // Waiting for UART event.
    if (xQueueReceive(knot_midi_uart_state.uart0_queue, (void*)&event, (TickType_t)portMAX_DELAY)) {
      bzero(dtmp, RD_BUF_SIZE);
      // ESP_LOGI(TAG, "uart[%d] event:", EX_UART_NUM);
      switch (event.type) {
      // Event of UART receiving data
      /*We'd better handler data event fast, there would be much more data events than
      other types of events. If we take too much time on data event, the queue might
      be full.*/
      case UART_DATA:

        // led_rx_effect_start();

        grid_alert_one_set(&grid_led_state, 0, 200, 200, 200, 30);

        // ESP_LOGI(TAG, "[UART DATA]: %d", event.size);
        uart_read_bytes(EX_UART_NUM, dtmp, event.size, portMAX_DELAY);

        for (uint8_t i = 0; i < event.size; i++) {

          struct uart_midi_event_packet uart_ev = uart_midi_process_byte(dtmp[i]);

          if (uart_ev.length) {
            struct usb_midi_event_packet usb_ev = midi_uart_to_usb(uart_ev);
            knot_midi_usb_send_packet(usb_ev);

            if (knot_midi_uart_state.midi_through_state) {
              knot_midi_uart_send_packet(uart_ev);
              ets_printf("USB + MIDI: %d %d %d %d\n", usb_ev.byte0, usb_ev.byte1, usb_ev.byte2, usb_ev.byte3);
            } else {
              ets_printf("USB: %d %d %d %d\n", usb_ev.byte0, usb_ev.byte1, usb_ev.byte2, usb_ev.byte3);
            }
          }
        }

        // ESP_LOGI(TAG, "[DATA EVT]: %d %d %d %d", dtmp[0], dtmp[1], dtmp[2], dtmp[3]);
        break;
      // Event of HW FIFO overflow detected
      case UART_FIFO_OVF:
        ESP_LOGI(TAG, "hw fifo overflow");
        // If fifo overflow happened, you should consider adding flow control for your application.
        // The ISR has already reset the rx FIFO,
        // As an example, we directly flush the rx buffer here in order to read more data.
        uart_flush_input(EX_UART_NUM);
        xQueueReset(knot_midi_uart_state.uart0_queue);
        break;
      // Event of UART ring buffer full
      case UART_BUFFER_FULL:
        ESP_LOGI(TAG, "ring buffer full");
        // If buffer full happened, you should consider encreasing your buffer size
        // As an example, we directly flush the rx buffer here in order to read more data.
        uart_flush_input(EX_UART_NUM);
        xQueueReset(knot_midi_uart_state.uart0_queue);
        break;
      // Event of UART RX break detected
      case UART_BREAK:
        ESP_LOGI(TAG, "uart rx break");
        break;
      // Event of UART parity check error
      case UART_PARITY_ERR:
        ESP_LOGI(TAG, "uart parity error");
        break;
      // Event of UART frame error
      case UART_FRAME_ERR:
        ESP_LOGI(TAG, "uart frame error");
        break;
      // UART_PATTERN_DET
      case UART_PATTERN_DET:
        uart_get_buffered_data_len(EX_UART_NUM, &buffered_size);
        int pos = uart_pattern_pop_pos(EX_UART_NUM);
        ESP_LOGI(TAG, "[UART PATTERN DETECTED] pos: %d, buffered size: %d", pos, buffered_size);
        if (pos == -1) {
          // There used to be a UART_PATTERN_DET event, but the pattern position queue is full so that it can not
          // record the position. We should set a larger queue size.
          // As an example, we directly flush the rx buffer here.
          uart_flush_input(EX_UART_NUM);
        } else {
          uart_read_bytes(EX_UART_NUM, dtmp, pos, 100 / portTICK_PERIOD_MS);
          uint8_t pat[PATTERN_CHR_NUM + 1];
          memset(pat, 0, sizeof(pat));
          uart_read_bytes(EX_UART_NUM, pat, PATTERN_CHR_NUM, 100 / portTICK_PERIOD_MS);
          ESP_LOGI(TAG, "read data: %s", dtmp);
          ESP_LOGI(TAG, "read pat : %s", pat);
        }
        break;
      // Others
      default:
        ESP_LOGI(TAG, "uart event type: %d", event.type);
        break;
      }
    }

    // ESP_LOGI(TAG, "UART RX loop");
  }
  free(dtmp);
  dtmp = NULL;
  vTaskDelete(NULL);
}
