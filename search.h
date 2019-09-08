#ifndef SEARCH_H__
#define SEARCH_H__

#include <stdint.h>

// Search
//
// The master requests that all listening units should take part in a search
// by sending a number of timeslots to use. Each device then randomly selects
// a timeslot from those. The master does this by writing the number of slots
// to parameter 0x08.
//
// Slots are 1-indexed. If master selects 10 slots, these will be numbered
// from 1 to 10.
//
// The master then enumerates all slots and expects the units to respond in
// the supplied time slot they selected. The response contain the time slot
// that the unit will use for the next round. The units will also reply with
// their address first in the bull message. The enumeration is sent as a bull
// read message for parameter 0x08 with the slot in the payload.
//
// During this reply phase, all units listens to the responses, and if they
// detect that someone else already have selected their timeslot, they select
// one that have not yet been used. If the unit cannot keep track of the used
// time slots, a random slot is selected anyway.
//
// The master now once again request all units to take part in a new search,
// this time using the timeslot previously announced.
//
// When the master is certain that there are no collissions left, the devices
// can be given a new address based on their previous address (in bull address
// field) and the slot selected for next round. The write command for parameter
// 0x01 can take an extra parameter, slot, and if so it will be matched with
// both the address (as usual) and the selected next slot.
//
// If the unit cannot select a new slot, slot 0 is selected. The master can
// then probe slot 0 and if there is one or more replying, we know we still
// have issues.
//
// To reduce the memory load on the units, the master can inhibit responses
// for all the units that are already known.

// Start a new search.
void search_start(uint8_t nr_slots);

// Respond with a pointer to next selected slot if the supplied slot matches
// our currently selected one.
uint8_t* search_read_slot(uint8_t slot);

// Return true if the selected_slot is in fact our next selection
uint8_t search_is_us(uint8_t slot);


// Store the slot that someone else will use during next search
void search_add_used(uint8_t slot);
#endif
