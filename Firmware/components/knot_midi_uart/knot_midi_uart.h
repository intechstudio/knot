#pragma once

#include "driver/uart.h"
#include "knot_midi_translator.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/ringbuf.h"
#include "freertos/semphr.h"

#define TRS_TX_AB_SELECT 15
#define TRS_RX_AB_SELECT 16

#define EX_UART_NUM UART_NUM_1
#define PATTERN_CHR_NUM (3) /*!< Set the number of consecutive and identical characters received by receiver which defines a UART pattern*/

#define BUF_SIZE (1024)
#define RD_BUF_SIZE (BUF_SIZE)

#include "driver/gpio.h"

#include "grid_led.h"

struct knot_midi_uart_model {

  QueueHandle_t uart0_queue;
  RingbufHandle_t uart_rx_buffer_cvm;
  RingbufHandle_t uart_rx_buffer_rtm;
  uint8_t midi_through_state;
  uint8_t midi_trs_ab_state;
  uint8_t current_is_sysex;
};

extern struct knot_midi_uart_model knot_midi_uart_state;

void knot_midi_uart_init(struct knot_midi_uart_model* midi_uart);
int knot_midi_uart_send_data(struct uart_midi_event_packet ev);
void knot_midi_uart_rx_task(void* arg);

uint8_t knot_midi_uart_get_midithrough_state(struct knot_midi_uart_model* midi_uart);
void knot_midi_uart_set_midithrough_state(struct knot_midi_uart_model* midi_uart, uint8_t state);

uint8_t knot_midi_uart_get_miditrsab_state(struct knot_midi_uart_model* midi_uart);
void knot_midi_uart_set_miditrsab_state(struct knot_midi_uart_model* midi_uart, uint8_t state);

int knot_midi_uart_send_packet(struct uart_midi_event_packet ev);
