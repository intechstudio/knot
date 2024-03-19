#include "knot_midi_queue.h"
#include <stdint.h>

#define KNOT_MIDI_QUEUE_USBOUT_LENGTH 50
static uint32_t knot_midi_queueu_usbout_write_index = 0;
static uint32_t knot_midi_queueu_usbout_read_index = 0;
static struct usb_midi_event_packet knot_midi_queue_usbout[KNOT_MIDI_QUEUE_USBOUT_LENGTH] = {0};

int knot_midi_queue_usbout_push(struct usb_midi_event_packet ev) {
  knot_midi_queue_usbout[knot_midi_queueu_usbout_write_index] = ev;
  knot_midi_queueu_usbout_write_index = (knot_midi_queueu_usbout_write_index + 1) % KNOT_MIDI_QUEUE_USBOUT_LENGTH;
  return 0;
}

int knot_midi_queue_usbout_available(void) {
  if (knot_midi_queueu_usbout_read_index == knot_midi_queueu_usbout_write_index) {
    return 0;
  }
  return 1;
}

int knot_midi_queue_usbout_pop(struct usb_midi_event_packet* ev) {

  if (knot_midi_queueu_usbout_read_index == knot_midi_queueu_usbout_write_index) {
    return 1;
  }
  *ev = knot_midi_queue_usbout[knot_midi_queueu_usbout_read_index];
  knot_midi_queueu_usbout_read_index = (knot_midi_queueu_usbout_read_index + 1) % KNOT_MIDI_QUEUE_USBOUT_LENGTH;
  return 0;
}

#define KNOT_MIDI_QUEUE_TRSOUT_LENGTH 50
static uint32_t knot_midi_queueu_trsout_write_index = 0;
static uint32_t knot_midi_queueu_trsout_read_index = 0;
static struct uart_midi_event_packet knot_midi_queue_trsout[KNOT_MIDI_QUEUE_TRSOUT_LENGTH] = {0};

int knot_midi_queue_trsout_push(struct uart_midi_event_packet ev) {
  knot_midi_queue_trsout[knot_midi_queueu_trsout_write_index] = ev;
  knot_midi_queueu_trsout_write_index = (knot_midi_queueu_trsout_write_index + 1) % KNOT_MIDI_QUEUE_TRSOUT_LENGTH;
  return 0;
}

int knot_midi_queue_trsout_available(void) {
  if (knot_midi_queueu_trsout_read_index == knot_midi_queueu_trsout_write_index) {
    return 0;
  }
  return 1;
}

int knot_midi_queue_trsout_pop(struct uart_midi_event_packet* ev) {

  if (knot_midi_queueu_trsout_read_index == knot_midi_queueu_trsout_write_index) {
    return 1;
  }
  *ev = knot_midi_queue_trsout[knot_midi_queueu_trsout_read_index];
  knot_midi_queueu_trsout_read_index = (knot_midi_queueu_trsout_read_index + 1) % KNOT_MIDI_QUEUE_TRSOUT_LENGTH;
  return 0;
}
