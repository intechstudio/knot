#pragma once
#include "esp_attr.h"

#include "knot_midi_translator.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

int knot_midi_queue_usbout_available(void);
int knot_midi_queue_usbout_push(struct usb_midi_event_packet ev);
int knot_midi_queue_usbout_pop(struct usb_midi_event_packet* ev);

int knot_midi_queue_trsout_available(void);
int knot_midi_queue_trsout_push(struct uart_midi_event_packet ev);
int knot_midi_queue_trsout_pop(struct uart_midi_event_packet* ev);

#ifdef __cplusplus
}
#endif
