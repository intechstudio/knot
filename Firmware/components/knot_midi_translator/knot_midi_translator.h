#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct usb_midi_event_packet {
  uint8_t byte0;
  uint8_t byte1;
  uint8_t byte2;
  uint8_t byte3;
};

struct uart_midi_event_packet {
  uint8_t length;
  uint8_t byte1;
  uint8_t byte2;
  uint8_t byte3;
};

/**
 * @brief Convert USB midi event packets to 1-3 byte uart midi packets.
 * @param[in] usb_packet Contains the 4 byte usb event packet
 *
 * @return 1-3 byte uart_midi_event_packet containing the translated message
 */
struct uart_midi_event_packet usb_midi_to_uart(struct usb_midi_event_packet usb_packet);

/**
 * @brief Convert 1-3 byte uart midi packets to 4 byte USB midi event packets.
 * @param[in] uart_packet 1-3 byte uart_midi_event_packet
 *
 * @return The 4 byte usb event packet
 */
struct usb_midi_event_packet midi_uart_to_usb(struct uart_midi_event_packet uart_packet);

/**
 * @brief Determine whether a given byte represents a MIDI Real-Time Message or not
 * @param[in] byte MIDI byte
 *
 * @return true or false
 */
uint8_t uart_midi_is_byte_rtm(uint8_t byte);

/**
 * @brief Store next byte and return event packet once completed
 * @param[in] byte incoming 8-bit data chunk
 *
 * @return event packet if available, all zeros if not complete yet
 */
struct uart_midi_event_packet uart_midi_process_byte(uint8_t byte);

#ifdef __cplusplus
}
#endif
