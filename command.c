#include "common.h"
#if 0
struct ReadOnlyMemoryMap {
    uint16_t deviceId;
    uint16_t firmware;
};

struct IOMemoryMap {
    uint16_t    deviceId;
    uint8_t     firmware;
    uint8_t     status;
    uint8_t     commandRegister;
    uint8_t     parameters[12];
};
static uint8_t RegisterFile [16];

typedef enum {
    DEVICE_ID_L,DEVICE_ID_H,FIRMWARE_VERSION,
            LED_
    
} IORegister;
#endif
// Stub
void Command_Init() {
    //RegisterFile[0] = 0x12;
    //RegisterFile[1] = 0x28;
    //RegisterFile[2] = 0x01;
}

void Command_Task() {
    
}

