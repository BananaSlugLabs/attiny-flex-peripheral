#include "common.h"
#include "event_internal.h"
#include "sys.h"

void _event_handler_bad () {
    sys_abort(SysAbortBadEvent);
}

extern const event_handler_t _event_cb_map[8];
const event_handler_t _event_slot_0 ATTRIBUTES(section("events.slot0"), weak) = _event_handler_bad;
const event_handler_t _event_slot_1 ATTRIBUTES(section("events.slot1"), weak) = _event_handler_bad;
const event_handler_t _event_slot_2 ATTRIBUTES(section("events.slot2"), weak) = _event_handler_bad;
const event_handler_t _event_slot_3 ATTRIBUTES(section("events.slot3"), weak) = _event_handler_bad;
const event_handler_t _event_slot_4 ATTRIBUTES(section("events.slot4"), weak) = _event_handler_bad;
const event_handler_t _event_slot_5 ATTRIBUTES(section("events.slot5"), weak) = _event_handler_bad;
const event_handler_t _event_slot_6 ATTRIBUTES(section("events.slot6"), weak) = _event_handler_bad;
const event_handler_t _event_slot_7 ATTRIBUTES(section("events.slot7"), weak) = _event_handler_bad;

void event_process() {
    uint8_t mskLo = 0;
    while (_EVT_REGISTER) {
        for (uint8_t msk = 0x1, index = 0; _EVT_REGISTER && msk; index++, msk <<= 1) {
            if (msk & _EVT_REGISTER) {
                _EVT_REGISTER &= ~msk;
                _event_cb_map[index]();
            }
            mskLo |= msk;
            if (mskLo & _EVT_REGISTER) {
                break; // higher priority events
            }
        }
    }
}



bool event_assert_index(event_t id) {
    if (id >= 8 || _event_cb_map[id] == _event_handler_bad) {
        return false;
    }
    _EVT_REGISTER |= 1<<id;
    return true;
}