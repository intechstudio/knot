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

/**
 * @brief Convert USB midi event packets to 1-3 byte uart midi packets.
 * @param[in] usb_packet Contains the 4 byte usb event packet
 *
 * @return 1-3 byte uart_midi_event_packet containing the translated message
 */
struct uart_midi_event_packet usb_midi_to_uart(struct usb_midi_event_packet usb_packet);

/**
 * @brief Determine wether a given byte represents a MIDI Real-Time Message or not
 * @param[in] byte MIDI byte
 *
 * @return true or false
 */
uint8_t uart_midi_is_byte_rtm(uint8_t byte);

/**
 * @brief Find next valid packet from buffer if available
 * @param[in] buffer cyclic containing the received bytes
 * @param[in] length length of the cyclic buffer in bytes
 * @param[in] start_index start index where next byte is in buffer
 *
 * @return number of bytes describing next whole packet, 0 if not available
 */
uint8_t uart_midi_find_packet_from_buffer(uint8_t* buffer, uint32_t length, uint32_t start_index);



#ifdef __cplusplus
}
#endif
