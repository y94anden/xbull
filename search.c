#include "search.h"
#include "random.h"
#include <string.h>

#define MAXSLOTS 16
uint8_t srch_used_slots[MAXSLOTS];
uint8_t srch_selected_slot;
uint8_t srch_next_slot = 0;
uint8_t srch_nr_slots;

uint8_t srch_taken(uint8_t slot) {
  uint8_t i;
  for (i = 0; i < MAXSLOTS && srch_used_slots[i] != 0x00; i++) {
    if (srch_used_slots[i] == slot) {
      return 1;
    }
  }
  return 0;
}

void srch_select_next_slot() {
  uint16_t i;

  for (i = 0; i < MAXSLOTS<<2; i++) {
    srch_next_slot = 1 + rnd_integer(srch_nr_slots-1);
    if (!srch_taken(srch_next_slot)) {
      return;
    }
  }
  // We have now tried four times more than MAXSLOTS and failed. Select
  // slot 0 as an indicator of failure.
  srch_next_slot = 0;
}

void search_start(uint8_t nr_slots) {
  srch_nr_slots = nr_slots;
  memset(srch_used_slots, 0, MAXSLOTS); // 0 indicates it is not used.
  if (srch_next_slot == 0 || srch_next_slot > nr_slots) {
    srch_select_next_slot();
  }
  srch_selected_slot = srch_next_slot;
}

uint8_t* search_read_slot(uint8_t slot) {
  if (slot == srch_selected_slot) {
    // It is our turn. Select a new slot and return it.
    srch_select_next_slot();
    return &srch_next_slot;
  }

  return 0;
}

uint8_t search_is_us(uint8_t slot) {
  return slot == srch_next_slot;
}

void search_add_used(uint8_t slot) {
  uint8_t i;
  for (i = 0; i < MAXSLOTS; i++) {
    if (srch_used_slots[i] == 0x00) {
      srch_used_slots[i] = slot;
      return;
    }
  }

  // Ending up here means we did not have space to store the new slot. Oh well,
  // hope it works better next time.
}

