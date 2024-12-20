#include "../knot_midi_translator.h"
#include "unity.h"

#include <memory.h>
#include <stdio.h>

void setUp(void) {
  // set stuff up here
}

void tearDown(void) {
  // clean stuff up here
}

void test_function_should_recogniseRealtimeMessages(void) {
  // test stuff
  struct uart_midi_event_packet uart_packet;

  struct usb_midi_event_packet usb_packet;

  // Test Real Time Message

  // Timing CLock
  usb_packet = (struct usb_midi_event_packet){.byte0 = 0x3F, .byte1 = 0xF8, .byte2 = 0, .byte3 = 0};
  uart_packet = usb_midi_to_uart(usb_packet);
  TEST_ASSERT_EQUAL_UINT8(1, uart_packet.length);
  TEST_ASSERT_EQUAL_UINT8(0xF8, uart_packet.byte1);

  // Start
  usb_packet = (struct usb_midi_event_packet){.byte0 = 0x3F, .byte1 = 0xFA, .byte2 = 0, .byte3 = 0};
  uart_packet = usb_midi_to_uart(usb_packet);
  TEST_ASSERT_EQUAL_UINT8(1, uart_packet.length);
  TEST_ASSERT_EQUAL_UINT8(0xFA, uart_packet.byte1);

  // Continue
  usb_packet = (struct usb_midi_event_packet){.byte0 = 0x3F, .byte1 = 0xFB, .byte2 = 0, .byte3 = 0};
  uart_packet = usb_midi_to_uart(usb_packet);
  TEST_ASSERT_EQUAL_UINT8(1, uart_packet.length);
  TEST_ASSERT_EQUAL_UINT8(0xFB, uart_packet.byte1);

  // ....
}

void test_function_should_recogniseNoteMessages(void) {
  // test stuff
  struct uart_midi_event_packet uart_packet;

  struct usb_midi_event_packet usb_packet;

  // Note on
  usb_packet = (struct usb_midi_event_packet){.byte0 = 0x19, .byte1 = 0x90, .byte2 = 0x00, .byte3 = 0x7F};
  uart_packet = usb_midi_to_uart(usb_packet);
  TEST_ASSERT_EQUAL_UINT8(3, uart_packet.length);
  TEST_ASSERT_EQUAL_UINT8(0x90, uart_packet.byte1);
  TEST_ASSERT_EQUAL_UINT8(0x00, uart_packet.byte2);
  TEST_ASSERT_EQUAL_UINT8(0x7F, uart_packet.byte3);

  // Start
  usb_packet = (struct usb_midi_event_packet){.byte0 = 0x18, .byte1 = 0x80, .byte2 = 0x00, .byte3 = 0x00};
  uart_packet = usb_midi_to_uart(usb_packet);
  TEST_ASSERT_EQUAL_UINT8(3, uart_packet.length);
  TEST_ASSERT_EQUAL_UINT8(0x80, uart_packet.byte1);
  TEST_ASSERT_EQUAL_UINT8(0x00, uart_packet.byte2);
  TEST_ASSERT_EQUAL_UINT8(0x00, uart_packet.byte3);

  // ....
}

void test_function_should_recogniseControlChangeMessages(void) {
  // test stuff
  struct uart_midi_event_packet uart_packet;

  struct usb_midi_event_packet usb_packet;

  // ControlChange
  usb_packet = (struct usb_midi_event_packet){.byte0 = 0x19, .byte1 = 0xB0, .byte2 = 0x00, .byte3 = 0x7F};
  uart_packet = usb_midi_to_uart(usb_packet);
  TEST_ASSERT_EQUAL_UINT8(3, uart_packet.length);
  TEST_ASSERT_EQUAL_UINT8(0xB0, uart_packet.byte1);
  TEST_ASSERT_EQUAL_UINT8(0x00, uart_packet.byte2);
  TEST_ASSERT_EQUAL_UINT8(0x7F, uart_packet.byte3);

  // ....
}

void test_function_should_recogniseSysEx(void) {
  // test stuff
  struct uart_midi_event_packet uart_packet;

  struct usb_midi_event_packet usb_packet;

  // SysEx Start or Continue
  usb_packet = (struct usb_midi_event_packet){.byte0 = 0x04, .byte1 = 0xF0, .byte2 = 0x00, .byte3 = 0x01};
  uart_packet = usb_midi_to_uart(usb_packet);
  TEST_ASSERT_EQUAL_UINT8(3, uart_packet.length);
  TEST_ASSERT_EQUAL_UINT8(0xF0, uart_packet.byte1);
  TEST_ASSERT_EQUAL_UINT8(0x00, uart_packet.byte2);
  TEST_ASSERT_EQUAL_UINT8(0x01, uart_packet.byte3);

  // SysEx ends with following single byte
  usb_packet = (struct usb_midi_event_packet){.byte0 = 0x05, .byte1 = 0xF7, .byte2 = 0x00, .byte3 = 0x01};
  uart_packet = usb_midi_to_uart(usb_packet);
  TEST_ASSERT_EQUAL_UINT8(1, uart_packet.length);
  TEST_ASSERT_EQUAL_UINT8(0xF7, uart_packet.byte1);

  // SysEx ends with following two bytes
  usb_packet = (struct usb_midi_event_packet){.byte0 = 0x06, .byte1 = 0x02, .byte2 = 0xF7, .byte3 = 0x01};
  uart_packet = usb_midi_to_uart(usb_packet);
  TEST_ASSERT_EQUAL_UINT8(2, uart_packet.length);
  TEST_ASSERT_EQUAL_UINT8(0x02, uart_packet.byte1);
  TEST_ASSERT_EQUAL_UINT8(0xF7, uart_packet.byte2);

  // SysEx ends with following three bytes
  usb_packet = (struct usb_midi_event_packet){.byte0 = 0x07, .byte1 = 0x02, .byte2 = 0x03, .byte3 = 0xF7};
  uart_packet = usb_midi_to_uart(usb_packet);
  TEST_ASSERT_EQUAL_UINT8(3, uart_packet.length);
  TEST_ASSERT_EQUAL_UINT8(0x02, uart_packet.byte1);
  TEST_ASSERT_EQUAL_UINT8(0x03, uart_packet.byte2);
  TEST_ASSERT_EQUAL_UINT8(0xF7, uart_packet.byte3);

  // ....
}

void test_function_should_recogniseOtherMessages(void) {
  // test stuff
  struct uart_midi_event_packet uart_packet;

  struct usb_midi_event_packet usb_packet;

  // Song Select
  usb_packet = (struct usb_midi_event_packet){.byte0 = 0x12, .byte1 = 0xF3, .byte2 = 0x7F, .byte3 = 0x00};
  uart_packet = usb_midi_to_uart(usb_packet);
  TEST_ASSERT_EQUAL_UINT8(2, uart_packet.length);
  TEST_ASSERT_EQUAL_UINT8(0xF3, uart_packet.byte1);
  TEST_ASSERT_EQUAL_UINT8(0x7F, uart_packet.byte2);

  // MTC
  usb_packet = (struct usb_midi_event_packet){.byte0 = 0x12, .byte1 = 0xF1, .byte2 = 0x7F, .byte3 = 0x00};
  uart_packet = usb_midi_to_uart(usb_packet);
  TEST_ASSERT_EQUAL_UINT8(2, uart_packet.length);
  TEST_ASSERT_EQUAL_UINT8(0xF1, uart_packet.byte1);
  TEST_ASSERT_EQUAL_UINT8(0x7F, uart_packet.byte2);

  // SPP
  usb_packet = (struct usb_midi_event_packet){.byte0 = 0x13, .byte1 = 0xF2, .byte2 = 0x7F, .byte3 = 0x00};
  uart_packet = usb_midi_to_uart(usb_packet);
  TEST_ASSERT_EQUAL_UINT8(3, uart_packet.length);
  TEST_ASSERT_EQUAL_UINT8(0xF2, uart_packet.byte1);
  TEST_ASSERT_EQUAL_UINT8(0x7F, uart_packet.byte2);
  TEST_ASSERT_EQUAL_UINT8(0x00, uart_packet.byte3);

  // Poly-KeyPress
  usb_packet = (struct usb_midi_event_packet){.byte0 = 0x1A, .byte1 = 0xF2, .byte2 = 0x7F, .byte3 = 0x00};
  uart_packet = usb_midi_to_uart(usb_packet);
  TEST_ASSERT_EQUAL_UINT8(3, uart_packet.length);
  TEST_ASSERT_EQUAL_UINT8(0xF2, uart_packet.byte1);
  TEST_ASSERT_EQUAL_UINT8(0x7F, uart_packet.byte2);
  TEST_ASSERT_EQUAL_UINT8(0x00, uart_packet.byte3);

  // PitchBend Change
  usb_packet = (struct usb_midi_event_packet){.byte0 = 0x1E, .byte1 = 0xE0, .byte2 = 0x7F, .byte3 = 0x7F};
  uart_packet = usb_midi_to_uart(usb_packet);
  TEST_ASSERT_EQUAL_UINT8(3, uart_packet.length);
  TEST_ASSERT_EQUAL_UINT8(0xE0, uart_packet.byte1);
  TEST_ASSERT_EQUAL_UINT8(0x7F, uart_packet.byte2);
  TEST_ASSERT_EQUAL_UINT8(0x7F, uart_packet.byte3);
}

void test_function_should_recogniseReservedMessages(void) {
  // test stuff
  struct uart_midi_event_packet uart_packet;

  struct usb_midi_event_packet usb_packet;

  // Miscellaneous function codes. Reserved for future extensions.
  usb_packet = (struct usb_midi_event_packet){.byte0 = 0x10, .byte1 = 0xF3, .byte2 = 0x7F, .byte3 = 0x00};
  uart_packet = usb_midi_to_uart(usb_packet);
  TEST_ASSERT_EQUAL_UINT8(0, uart_packet.length);

  // Cable events. Reserved for future expansion.
  usb_packet = (struct usb_midi_event_packet){.byte0 = 0x11, .byte1 = 0xF1, .byte2 = 0x7F, .byte3 = 0x00};
  uart_packet = usb_midi_to_uart(usb_packet);
  TEST_ASSERT_EQUAL_UINT8(0, uart_packet.length);
}

void uart_midi_is_byte_rtm__should_recogniseRtmMessages(void) {
  // test stuff

  TEST_ASSERT_EQUAL_UINT8(0, uart_midi_is_byte_rtm(0));     // value 0
  TEST_ASSERT_EQUAL_UINT8(0, uart_midi_is_byte_rtm(127));   // value 127
  TEST_ASSERT_EQUAL_UINT8(0, uart_midi_is_byte_rtm(128));   // command note off
  TEST_ASSERT_EQUAL_UINT8(1, uart_midi_is_byte_rtm(0x0F8)); // RTM Timing Clock
  TEST_ASSERT_EQUAL_UINT8(1, uart_midi_is_byte_rtm(0x0F9)); // RTM Undefined
  TEST_ASSERT_EQUAL_UINT8(1, uart_midi_is_byte_rtm(0x0FA)); // RTM Start
  TEST_ASSERT_EQUAL_UINT8(1, uart_midi_is_byte_rtm(0x0FB)); // RTM Continue
  TEST_ASSERT_EQUAL_UINT8(1, uart_midi_is_byte_rtm(0x0FC)); // RTM Stop
  TEST_ASSERT_EQUAL_UINT8(1, uart_midi_is_byte_rtm(0x0FD)); // RTM Undefined
  TEST_ASSERT_EQUAL_UINT8(1, uart_midi_is_byte_rtm(0x0FE)); // RTM Active Sensing
  TEST_ASSERT_EQUAL_UINT8(1, uart_midi_is_byte_rtm(0x0FF)); // RTM System Reset
}

void uart_midi_find_packet_from_buffer__should_recogniseChannelVoiceMessages(void) {

  // control change

  uart_midi_process_byte(0);
  uart_midi_process_byte(1);

  struct uart_midi_event_packet expected = {.length = 0x00, .byte1 = 0x00, .byte2 = 0x00, .byte3 = 0x00};
  struct uart_midi_event_packet actual = {.length = 0x00, .byte1 = 0x00, .byte2 = 0x00, .byte3 = 0x00};

  // Control Change Message
  expected = (struct uart_midi_event_packet){.length = 0x00, .byte1 = 0x00, .byte2 = 0x00, .byte3 = 0x00};
  actual = uart_midi_process_byte(176);
  TEST_ASSERT_EQUAL_MEMORY(&expected, &actual, 4);

  expected = (struct uart_midi_event_packet){.length = 0x00, .byte1 = 0x00, .byte2 = 0x00, .byte3 = 0x00};
  actual = uart_midi_process_byte(0);
  TEST_ASSERT_EQUAL_MEMORY(&expected, &actual, 4);

  expected = (struct uart_midi_event_packet){.length = 0x03, .byte1 = 176, .byte2 = 0x00, .byte3 = 0x00};
  actual = uart_midi_process_byte(0);
  TEST_ASSERT_EQUAL_MEMORY(&expected, &actual, 4);

  // Sysex 0xF0 0xF7
  expected = (struct uart_midi_event_packet){.length = 0x00, .byte1 = 0x00, .byte2 = 0x00, .byte3 = 0x00};
  actual = uart_midi_process_byte(0xF0);
  TEST_ASSERT_EQUAL_MEMORY(&expected, &actual, 4);

  expected = (struct uart_midi_event_packet){.length = 0x02, .byte1 = 0xF0, .byte2 = 0xF7, .byte3 = 0x00};
  actual = uart_midi_process_byte(0xF7);
  TEST_ASSERT_EQUAL_MEMORY(&expected, &actual, 4);

  // Sysex 0xF0 0x00 0x01 0xF7
  expected = (struct uart_midi_event_packet){.length = 0x00, .byte1 = 0x00, .byte2 = 0x00, .byte3 = 0x00};
  actual = uart_midi_process_byte(0xF0);
  TEST_ASSERT_EQUAL_MEMORY(&expected, &actual, 4);

  expected = (struct uart_midi_event_packet){.length = 0x00, .byte1 = 0x00, .byte2 = 0x00, .byte3 = 0x00};
  actual = uart_midi_process_byte(0x00);
  TEST_ASSERT_EQUAL_MEMORY(&expected, &actual, 4);

  expected = (struct uart_midi_event_packet){.length = 0x03, .byte1 = 0xF0, .byte2 = 0x00, .byte3 = 0x01};
  actual = uart_midi_process_byte(0x01);
  TEST_ASSERT_EQUAL_MEMORY(&expected, &actual, 4);

  // Sysex one byte stop
  expected = (struct uart_midi_event_packet){.length = 0x01, .byte1 = 0xF7, .byte2 = 0x00, .byte3 = 0x00};
  actual = uart_midi_process_byte(0xF7);
  TEST_ASSERT_EQUAL_MEMORY(&expected, &actual, 4);

  // RTM
  expected = (struct uart_midi_event_packet){.length = 0x01, .byte1 = 0xF8, .byte2 = 0x00, .byte3 = 0x00};
  actual = uart_midi_process_byte(0xF8);
  TEST_ASSERT_EQUAL_MEMORY(&expected, &actual, 4);

  // Control Change Message WITH RTM in the middle
  expected = (struct uart_midi_event_packet){.length = 0x00, .byte1 = 0x00, .byte2 = 0x00, .byte3 = 0x00};
  actual = uart_midi_process_byte(176);
  TEST_ASSERT_EQUAL_MEMORY(&expected, &actual, 4);

  // RTM
  expected = (struct uart_midi_event_packet){.length = 0x01, .byte1 = 0xF8, .byte2 = 0x00, .byte3 = 0x00};
  actual = uart_midi_process_byte(0xF8);
  TEST_ASSERT_EQUAL_MEMORY(&expected, &actual, 4);

  expected = (struct uart_midi_event_packet){.length = 0x00, .byte1 = 0x00, .byte2 = 0x00, .byte3 = 0x00};
  actual = uart_midi_process_byte(0);
  TEST_ASSERT_EQUAL_MEMORY(&expected, &actual, 4);

  expected = (struct uart_midi_event_packet){.length = 0x03, .byte1 = 176, .byte2 = 0x00, .byte3 = 0x00};
  actual = uart_midi_process_byte(0);
  TEST_ASSERT_EQUAL_MEMORY(&expected, &actual, 4);
}

void midi_uart_to_usb__should_convertAllMessages(void) {

  // control change

  uart_midi_process_byte(0);
  uart_midi_process_byte(1);

  struct usb_midi_event_packet expected = {.byte0 = 0x00, .byte1 = 0x00, .byte2 = 0x00, .byte3 = 0x00};
  struct usb_midi_event_packet actual = {.byte0 = 0x00, .byte1 = 0x00, .byte2 = 0x00, .byte3 = 0x00};

  // Zero length packet should returns all zeros usb midi event packet
  actual = midi_uart_to_usb((struct uart_midi_event_packet){.length = 0x00, .byte1 = 0x00, .byte2 = 0x00, .byte3 = 0x00});
  expected = (struct usb_midi_event_packet){.byte0 = 0x00, .byte1 = 0x00, .byte2 = 0x00, .byte3 = 0x00};
  TEST_ASSERT_EQUAL_MEMORY(&expected, &actual, 4);

  // Note on message on channel 0
  actual = midi_uart_to_usb((struct uart_midi_event_packet){.length = 0x03, .byte1 = 0x90, .byte2 = 0x00, .byte3 = 0x7F});
  expected = (struct usb_midi_event_packet){.byte0 = 0x09, .byte1 = 0x90, .byte2 = 0x00, .byte3 = 0x7F};
  TEST_ASSERT_EQUAL_MEMORY(&expected, &actual, 4);

  // Sysex 0xF0 0xF7
  actual = midi_uart_to_usb((struct uart_midi_event_packet){.length = 0x02, .byte1 = 0xF0, .byte2 = 0xF7, .byte3 = 0x00});
  expected = (struct usb_midi_event_packet){.byte0 = 0x06, .byte1 = 0xF0, .byte2 = 0xF7, .byte3 = 0x00};
  TEST_ASSERT_EQUAL_MEMORY(&expected, &actual, 4);

  // Sysex 0xF0 0x01 0xF7
  actual = midi_uart_to_usb((struct uart_midi_event_packet){.length = 0x03, .byte1 = 0xF0, .byte2 = 0x01, .byte3 = 0xF7});
  expected = (struct usb_midi_event_packet){.byte0 = 0x07, .byte1 = 0xF0, .byte2 = 0x01, .byte3 = 0xF7};
  TEST_ASSERT_EQUAL_MEMORY(&expected, &actual, 4);

  // Sysex 0xF0 0x01 0x02 (long sysex started)
  actual = midi_uart_to_usb((struct uart_midi_event_packet){.length = 0x03, .byte1 = 0xF0, .byte2 = 0x01, .byte3 = 0x02});
  expected = (struct usb_midi_event_packet){.byte0 = 0x04, .byte1 = 0xF0, .byte2 = 0x01, .byte3 = 0x02};
  TEST_ASSERT_EQUAL_MEMORY(&expected, &actual, 4);

  // Sysex 0xF7 (sysex ended with one byte)
  actual = midi_uart_to_usb((struct uart_midi_event_packet){.length = 0x01, .byte1 = 0xF7, .byte2 = 0x00, .byte3 = 0x00});
  expected = (struct usb_midi_event_packet){.byte0 = 0x05, .byte1 = 0xF7, .byte2 = 0x00, .byte3 = 0x00};
  TEST_ASSERT_EQUAL_MEMORY(&expected, &actual, 4);

  // Sysex 0x01 0xF7 (sysex ended with two bytes)
  actual = midi_uart_to_usb((struct uart_midi_event_packet){.length = 0x02, .byte1 = 0x01, .byte2 = 0xF7, .byte3 = 0x00});
  expected = (struct usb_midi_event_packet){.byte0 = 0x06, .byte1 = 0x01, .byte2 = 0xF7, .byte3 = 0x00};
  TEST_ASSERT_EQUAL_MEMORY(&expected, &actual, 4);

  // Sysex 0x01 0x02 0xF7 (sysex ended with three bytes)
  actual = midi_uart_to_usb((struct uart_midi_event_packet){.length = 0x03, .byte1 = 0x01, .byte2 = 0x02, .byte3 = 0xF7});
  expected = (struct usb_midi_event_packet){.byte0 = 0x07, .byte1 = 0x01, .byte2 = 0x02, .byte3 = 0xF7};
  TEST_ASSERT_EQUAL_MEMORY(&expected, &actual, 4);

  // Sysex 0x01 0x02 0x03 (sysex continue with three bytes)
  actual = midi_uart_to_usb((struct uart_midi_event_packet){.length = 0x03, .byte1 = 0x01, .byte2 = 0x02, .byte3 = 0x03});
  expected = (struct usb_midi_event_packet){.byte0 = 0x04, .byte1 = 0x01, .byte2 = 0x02, .byte3 = 0x03};
  TEST_ASSERT_EQUAL_MEMORY(&expected, &actual, 4);

  // Real-Time
  actual = midi_uart_to_usb((struct uart_midi_event_packet){.length = 0x01, .byte1 = 0xF8, .byte2 = 0x00, .byte3 = 0x00});
  expected = (struct usb_midi_event_packet){.byte0 = 0x0F, .byte1 = 0xF8, .byte2 = 0x00, .byte3 = 0x00};
  TEST_ASSERT_EQUAL_MEMORY(&expected, &actual, 4);
}

void test_function_should_doAlsoDoBlah(void) {
  // more test stuff
}

// not needed when using generate_test_runner.rb
int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_function_should_recogniseRealtimeMessages);
  RUN_TEST(test_function_should_recogniseNoteMessages);
  RUN_TEST(test_function_should_recogniseControlChangeMessages);
  RUN_TEST(test_function_should_recogniseSysEx);
  RUN_TEST(test_function_should_recogniseOtherMessages);
  RUN_TEST(test_function_should_recogniseReservedMessages);

  RUN_TEST(uart_midi_is_byte_rtm__should_recogniseRtmMessages);

  RUN_TEST(uart_midi_find_packet_from_buffer__should_recogniseChannelVoiceMessages);
  RUN_TEST(midi_uart_to_usb__should_convertAllMessages);

  return UNITY_END();
}
