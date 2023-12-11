#pragma once

#include "driver/uart.h"
#include "knot_midi_translator.h"



#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/ringbuf.h"
#include "freertos/queue.h"


void uart_init(void);
int uart_send_data(struct uart_midi_event_packet ev);
void uart_housekeeping_task(void *arg);
void uart_rx_task(void *arg);
