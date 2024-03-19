#pragma once
#include "esp_attr.h"

#include <stdint.h>
#include "knot_midi_translator.h"

#ifdef __cplusplus
extern "C" {
#endif


int IRAM_ATTR knot_midi_queue_usbout_available(void);
int IRAM_ATTR knot_midi_queue_usbout_push(struct usb_midi_event_packet ev);
int IRAM_ATTR knot_midi_queue_usbout_pop(struct usb_midi_event_packet* ev);


#ifdef __cplusplus
}
#endif
