#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif



struct usb_midi_event_packet
{
  uint8_t byte0;
  uint8_t byte1;
  uint8_t byte2;
  uint8_t byte3;
};

struct uart_midi_event_packet
{
  uint8_t length;
  uint8_t byte1;
  uint8_t byte2;
  uint8_t byte3;
};


struct uart_midi_event_packet usb_midi_to_uart(struct usb_midi_event_packet usb_packet);

#ifdef __cplusplus
}
#endif
