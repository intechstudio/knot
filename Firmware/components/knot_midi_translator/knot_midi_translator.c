#include "knot_midi_translator.h"
#include <stdint.h>

struct uart_midi_event_packet usb_midi_to_uart(struct usb_midi_event_packet usb_packet) {

  struct uart_midi_event_packet uart_packet = {.length = 0, .byte1 = usb_packet.byte1, .byte2 = usb_packet.byte2, .byte3 = usb_packet.byte3};

  /*

  32-bit USB-MIDI Event Packet

  | ==== Byte 0 ==== | ==== Byte 1 ==== | ==== Byte 2 ==== | ==== Byte 3 ==== |
  | Cable  | Code    |                  |                  |                  |
  | Number | Index   |      MIDI_0      |      MIDI_1      |      MIDI_2      |
  |        | Number  |                  |                  |                  |


  */

  // MIDI Cable Number
  uint8_t cn = ((usb_packet.byte0 & 0b11110000) >> 4);

  // MIDI Code Index Number
  uint8_t cin = (usb_packet.byte0 & 0b00001111);

  switch (cin) {
  case 0x00:
    break; // Miscellaneous function codes. Reserved for future extensions.
  case 0x01:
    break; // Cable events. Reserved for future expansion.
  case 0x02:
    uart_packet.length = 2;
    break; // Two-byte System Common messages like MTC, SongSelect, etc.
  case 0x03:
    uart_packet.length = 3;
    break; // Three-byte System Common messages like SPP, etc.
  case 0x04:
    uart_packet.length = 3;
    break; // SysEx starts or continues
  case 0x05:
    uart_packet.length = 1;
    break; // Single-byte System Common Message or SysEx ends with following single byte.
  case 0x06:
    uart_packet.length = 2;
    break; // SysEx ends with following two bytes.
  case 0x07:
    uart_packet.length = 3;
    break; // SysEx ends with following three bytes.
  case 0x08:
    uart_packet.length = 3;
    break; // Note-off
  case 0x09:
    uart_packet.length = 3;
    break; // Note-on
  case 0x0A:
    uart_packet.length = 3;
    break; // Poly-KeyPress
  case 0x0B:
    uart_packet.length = 3;
    break; // Control Change
  case 0x0C:
    uart_packet.length = 2;
    break; // Program Change
  case 0x0D:
    uart_packet.length = 2;
    break; // Channel Pressure
  case 0x0E:
    uart_packet.length = 3;
    break; // PitchBend Change
  case 0x0F:
    uart_packet.length = 1;
    break; // Single Byte
  }

  return uart_packet;
}

uint8_t uart_midi_is_byte_rtm(uint8_t byte) {

  if (byte >= 0xF8) {
    return 1;
  } else {
    return 0;
  }
}

uint8_t uart_midi_processor_buffer[3] = {0};
uint8_t uart_midi_processor_buffer_index = 0;
uint8_t uart_midi_processor_buffer_limit = 0;
uint8_t uart_midi_processor_state_is_sysex = 0;

struct usb_midi_event_packet midi_uart_to_usb(struct uart_midi_event_packet uart_packet) {

  // initialize return packet
  struct usb_midi_event_packet ev = {.byte0 = 0, .byte1 = 0, .byte2 = 0, .byte3 = 0};

  if (uart_packet.length == 0) {
    return ev;
  }

  if (uart_packet.byte1 == 0xF0 || uart_packet.byte1 < 128) {
    // Sysex start or continue
    if (uart_packet.byte2 == 0xF7) {
      ev.byte0 = 0x06;
      ev.byte1 = uart_packet.byte1;
      ev.byte2 = uart_packet.byte2;
      ev.byte3 = uart_packet.byte3;
    } else if (uart_packet.byte3 == 0xF7) {
      ev.byte0 = 0x07;
      ev.byte1 = uart_packet.byte1;
      ev.byte2 = uart_packet.byte2;
      ev.byte3 = uart_packet.byte3;
    } else {
      ev.byte0 = 0x04;
      ev.byte1 = uart_packet.byte1;
      ev.byte2 = uart_packet.byte2;
      ev.byte3 = uart_packet.byte3;
    }

  } else if (uart_packet.byte1 == 0xF7) {
    // Sysex stop
    ev.byte0 = 0x05;
    ev.byte1 = uart_packet.byte1;
    ev.byte2 = uart_packet.byte2;
    ev.byte3 = uart_packet.byte3;

  } else {
    // Channel Voice Message or real time
    ev.byte0 = (uart_packet.byte1 >> 4);
    ev.byte1 = uart_packet.byte1;
    ev.byte2 = uart_packet.byte2;
    ev.byte3 = uart_packet.byte3;
  }

  return ev;
}

struct uart_midi_event_packet uart_midi_process_byte(uint8_t byte) {

  // initialize return packet
  struct uart_midi_event_packet ev = {.length = 0, .byte1 = 0, .byte2 = 0, .byte3 = 0};

  if (uart_midi_processor_state_is_sysex) {

    // store sysex data
    uart_midi_processor_buffer[uart_midi_processor_buffer_index] = byte;
    uart_midi_processor_buffer_index++;

    if (byte == 0xF7) {
      // sysex end command: switch to normal mode
      uart_midi_processor_state_is_sysex = 0;

      // assemble return packet
      ev = (struct uart_midi_event_packet){
          .length = uart_midi_processor_buffer_index, .byte1 = uart_midi_processor_buffer[0], .byte2 = uart_midi_processor_buffer[1], .byte3 = uart_midi_processor_buffer[2]};

      // Clear buffer
      uart_midi_processor_buffer[0] = 0;
      uart_midi_processor_buffer[1] = 0;
      uart_midi_processor_buffer[2] = 0;
      uart_midi_processor_buffer_index = 0;
      uart_midi_processor_buffer_limit = 0;

    } else if (uart_midi_processor_buffer_index == 3) {
      // no more space, send packet and continue in sysex mode

      // assemble return packet
      ev = (struct uart_midi_event_packet){
          .length = uart_midi_processor_buffer_index, .byte1 = uart_midi_processor_buffer[0], .byte2 = uart_midi_processor_buffer[1], .byte3 = uart_midi_processor_buffer[2]};

      // Clear buffer
      uart_midi_processor_buffer[0] = 0;
      uart_midi_processor_buffer[1] = 0;
      uart_midi_processor_buffer[2] = 0;
      uart_midi_processor_buffer_index = 0;
      uart_midi_processor_buffer_limit = 0;
    }

  } else {

    if (byte == 0xF0) {
      // switch to sysex mode
      uart_midi_processor_state_is_sysex = 1;
      // Clear buffer and store sysex start command
      uart_midi_processor_buffer[0] = byte;
      uart_midi_processor_buffer[1] = 0;
      uart_midi_processor_buffer[2] = 0;
      uart_midi_processor_buffer_index = 1;
      uart_midi_processor_buffer_limit = 3;

    } else {

      if (byte > 127) {

        if (uart_midi_is_byte_rtm(byte)) {

          // assemble return packet but do not clear the buffer
          ev = (struct uart_midi_event_packet){.length = 1, .byte1 = byte, .byte2 = 0, .byte3 = 0};
        } else {

          // Clear buffer and store the command
          uart_midi_processor_buffer[0] = byte;
          uart_midi_processor_buffer[1] = 0;
          uart_midi_processor_buffer[2] = 0;
          uart_midi_processor_buffer_index = 1;

          if ((byte & 0b11110000) == 0xC0 || (byte & 0b11110000) == 0xD0) {
            // set buffer limit to 2 for program change and channel preassure/aftertouch command types
            uart_midi_processor_buffer_limit = 2;
          } else {
            // set buffer limit to 3 for all other channel voice messages
            uart_midi_processor_buffer_limit = 3;
          }
        }

      } else {
        // store incoming byte
        uart_midi_processor_buffer[uart_midi_processor_buffer_index] = byte;
        uart_midi_processor_buffer_index++;

        if (uart_midi_processor_buffer_index == uart_midi_processor_buffer_limit) {
          // assemble return packet
          ev = (struct uart_midi_event_packet){
              .length = uart_midi_processor_buffer_index, .byte1 = uart_midi_processor_buffer[0], .byte2 = uart_midi_processor_buffer[1], .byte3 = uart_midi_processor_buffer[2]};

          // Clear buffer

          uart_midi_processor_buffer[0] = 0;
          uart_midi_processor_buffer[1] = 0;
          uart_midi_processor_buffer[2] = 0;
          uart_midi_processor_buffer_index = 0;
          uart_midi_processor_buffer_limit = 0;
        }
      }
    }
  }

  return ev;
}
