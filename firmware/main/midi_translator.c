#include <stdint.h>
#include "midi_translator.h"


struct uart_midi_event_packet usb_midi_to_uart(struct usb_midi_event_packet usb_packet){

  struct uart_midi_event_packet uart_packet = {
    .length = 0,
    .byte1 = usb_packet.byte1,
    .byte2 = usb_packet.byte2,
    .byte3 = usb_packet.byte3
  };


  /*
  
  32-bit USB-MIDI Event Packet

  | ==== Byte 0 ==== | ==== Byte 1 ==== | ==== Byte 2 ==== | ==== Byte 3 ==== |
  | Cable  | Code    |                  |                  |                  |
  | Number | Index   |      MIDI_0      |      MIDI_1      |      MIDI_2      |
  |        | Number  |                  |                  |                  |
  
  
  */

  // MIDI Cable Number
  uint8_t cn = ((usb_packet.byte0 & 0b11110000)>>4);

  // MIDI Code Index Number
  uint8_t cin = (usb_packet.byte0 & 0b00001111);

  switch(cin){
    case 0x00: break; // Miscellaneous function codes. Reserved for future extensions.
    case 0x01: break; // Cable events. Reserved for future expansion.
    case 0x02: uart_packet.length = 2; break; // Two-byte System Common messages like MTC, SongSelect, etc.
    case 0x03: uart_packet.length = 3; break; // Three-byte System Common messages like SPP, etc.
    case 0x04: uart_packet.length = 3; break; // SysEx starts or continues
    case 0x05: uart_packet.length = 1; break; // Single-byte System Common Message or SysEx ends with following single byte.
    case 0x06: uart_packet.length = 2; break; // SysEx ends with following two bytes.
    case 0x07: uart_packet.length = 3; break; // SysEx ends with following three bytes.
    case 0x08: uart_packet.length = 3; break; // Note-off
    case 0x09: uart_packet.length = 3; break; // Note-on
    case 0x0A: uart_packet.length = 3; break; // Poly-KeyPress
    case 0x0B: uart_packet.length = 3; break; // Control Change
    case 0x0C: uart_packet.length = 2; break; // Program Change
    case 0x0D: uart_packet.length = 2; break; // Channel Pressure
    case 0x0E: uart_packet.length = 3; break; // PitchBend Change
    case 0x0F: uart_packet.length = 1; break; // Single Byte
    
  }


  return uart_packet;

}