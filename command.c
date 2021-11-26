#include "bus.h"

/*
 * I2C Bus Interface
 * -----------------------------------------------------------------------------
 * 
 * Index    Fields
 *  00      SSSxEEEE
 *              ^^^^ End Point      (Which register file to write)
 *          ^^^      Signal         (When write is complete, send signal to task)
 *  01      AAAAAAAA
 *          ^^^^^^^^ Address        (Address Register)
 * 
 * Example using BusPirate
 * -----------------------------------------------------------------------------
 * Note: Need clock streching patch.
 * 
 * Get Info:
 * 
 * [0x52 0x00 0x00 [0x53 r:2] -> Mfg
 * [0x52 0x00 0x02 [0x53 r:2] -> Identifier
 * [0x52 0x00 0x04 [0x53 r:2] -> Capabilities
 * [0x52 0x00 0x06 [0x53 r:1] -> Version
 * [0x52 0x00 0x00 [0x53 r:8] -> Read Mfg + Identifier + Caps + Version
 * 
 * [0x52 0x00 0x08 [0x53 r:1] -> Total LED Count
 * 
 * [0x52 0x21 0x02 0x00 0x00 0x00 0x00 0x00 0x00  0x00 0x00 0x00 0x00 0x00 0x00] -> Clear Leds (using signal)
 * [0x52 0x01 0x00 0x04 0x01] -> Update
 * [0x52 0x01 0x03 0xFF 0xFF 0xFF 0xFF 0x00 0x00  0x00 0xFF 0x00 0x00 0x00 0xFF] [0x52 0x01 0x00 0x01] -> Set Leds & Update
 * [0x52 0x21 0x02 0xFF 0xFF 0xFF 0xFF 0x00 0x00  0x00 0xFF 0x00 0x00 0x00 0xFF] -> Set Leds (using signal)
 */

#if CONFIG_BUS_ENABLE
#if 0
enum PixelKindTag {
    LedKindWS2812         = 1,
};
typedef uint8_t LedKind;

enum PixelFormatTag {
    LedFormatGRB888       = 1
};
typedef uint8_t LedFormat;
typedef uint8_t LedCount;
#endif

enum {
    Device_CapabilitySignal       = 1<<5
};
typedef uint16_t device_capabilities_t;

typedef struct FirmwareRegisterFileTag {
    uint16_t                    mfg;                                        // 0..1
    uint16_t                    ident;                                      // 2..3
    device_capabilities_t       capabilities;                               // 4..5
    uint8_t                     version;                                    // 6
    uint8_t                     _r0;                                        // 7
    //uint8_t                     led_maximum;                                // 8
    //LedKind                     led_kind;                                   // 9
    //LedFormat                   led_format;                                 // 10
} device_registerfile_t;

const device_registerfile_t Device_RegisterFile = {
    .mfg                        = 0xB5B5,
    .ident                      = FW_IDENT,
    .version                    = FW_VERSION,
    .capabilities               =
#if CONFIG_BUS_SIGNAL
                                    Device_CapabilitySignal |
#endif
                                0 ,
    //.led_maximum                = CONFIG_LED_COUNT,
    //.led_kind                   = LedKindWS2812,
    //.led_format                 = LedFormatGRB888
};

BUS_REGISTER_FILE(Device_RegisterFile, 0, Bus_FlagReadOnly);

extern const Bus_EndPoint               _RegisterFileDescriptors;
extern const uint8_t                    _RegisterFileDescriptorsCount;
#define RegisterFile_Get(index)         (((Bus_EndPoint*)&_RegisterFileDescriptors)[index])
#define RegisterFile_Count()            ((uint8_t)((uintptr_t)&_RegisterFileDescriptorsCount))

void bus_task (message_t message, MessageData data);
PRIVATE_TASK_DEFINE(bus_task, 3);

#define Bus_EndPointGet(index)      (&(((Bus_EndPoint*)&_Bus_RegisterFileEndPoints)[index]))

typedef enum Bus_StateTag {
    Bus_Idle                = 0,
    Bus_ReadOrSelectEp      = 1, // The first write after address will 
    Bus_SetAddr             = 2,
    Bus_ReadWrite           = 3,
    Bus_SendSignal          = 4,
    Bus_Error               = 5,
} bus_state_t;

enum {
    Bus_EpMask              = 0x0F,
    Bus_SignalMask          = 0xE0,
    Bus_SignalBp            = 5
    
};

static struct {
    bus_state_t         state;
    uint8_t             epcmd;
    uint8_t             activeIndex;
    Bus_EndPoint*       active;
} Bus;

#define TWI_CMD_ACK             (TWI_SCMD_RESPONSE_gc | TWI_ACKACT_ACK_gc)
#define TWI_CMD_NACK            (TWI_SCMD_RESPONSE_gc | TWI_ACKACT_NACK_gc)
#define TWI_CMD_DONE            (TWI_SCMD_COMPTRANS_gc)

ISR(TWI0_TWIS_vect) {
    uint8_t s = TWI0.SSTATUS;
    
    if (s & TWI_APIF_bm) {
        if (s & TWI_AP_bm) {
#if CONFIG_BUS_SIGNAL
            if (Bus.state != Bus_SendSignal) {
                TWI0.SCTRLB = TWI_CMD_ACK;
                Bus.state = Bus_ReadOrSelectEp;
            } else {
                TWI0.SCTRLB = TWI_CMD_NACK;
                //DEBUG_BREAKPOINT();
            }
#else
            TWI0.SCTRLB = TWI_CMD_ACK;
            Bus.state = Bus_ReadOrSelectEp;
#endif
        } else {
                TWI0.SCTRLB = TWI_CMD_DONE;
            if (Bus.epcmd & Bus_SignalMask) {
                Bus.state = Bus_SendSignal;
            } else {
                Bus.state = Bus_Idle;
            }
        }
        return;
    }
    
    if (s & TWI_DIF_bm) {
        if (s & TWI_DIR_bm) {
            // ************ Controller Read ****************************************
            if( /*(Bus.state == Bus_ReadOrSelectEp && (s & TWI_RXACK_bm)) ||*/ !Bus.active || Bus.activeIndex >= Bus.active->size) {
                TWI0.SCTRLB = TWI_CMD_DONE;
                Bus.state = Bus_Idle;
                return;
            }
            
            TWI0.SDATA = Bus.active->data[Bus.activeIndex++]; 
            TWI0.SCTRLB = TWI_CMD_ACK;
        } else {
            // ************ Controller Write ***************************************
            switch (Bus.state) {
                case Bus_ReadOrSelectEp:
                {
                    Bus.epcmd = TWI0.SDATA;
                    uint8_t ep = Bus_EpMask & Bus.epcmd;
                    if ( ep < RegisterFile_Count() ) {
                        Bus.activeIndex = 0;
                        Bus.active = &RegisterFile_Get(ep);
                        TWI0.SCTRLB = TWI_CMD_ACK;
                        Bus.state = Bus_SetAddr;
                    } else {
                        Bus.state = Bus_Error; 
                        TWI0.SCTRLB = TWI_CMD_NACK;
                    }
                    Bus.activeIndex = 0;
                    break;
                }
                case Bus_SetAddr:
                {
                    Bus.activeIndex = TWI0.SDATA;
                    TWI0.SCTRLB = TWI_CMD_ACK;
                    Bus.state = Bus_ReadWrite;
                    break;
                }
                case Bus_ReadWrite:
                {
                    uint8_t val = TWI0.SDATA;
                    if (Bus.active && Bus.activeIndex < Bus.active->size && (Bus.active->flags & Bus_FlagReadWrite)) {
                        TWI0.SCTRLB = TWI_CMD_ACK;
                        Bus.active->data[(Bus.activeIndex++)] = val;
                    } else {
                        Bus.state = Bus_Error;
                        TWI0.SCTRLB = TWI_CMD_NACK;
                    }
                    break;
                }
                case Bus_Error:
                default:
                {
                    DEBUG_BREAKPOINT();
                    TWI0.SCTRLB = TWI_CMD_NACK;
                    break;
                }
            }
        }
    }
}

void bus_task (message_t message, MessageData data) {
    switch (message) {
        case SystemMessage_Init:
            //SDASETUP 4CYC; SDAHOLD OFF; FMPEN disabled; 
            TWI0.CTRLA = 0x00;

            //Debug Run
            TWI0.DBGCTRL = 0x00;

            //Peripheral Address
            TWI0.SADDR = CONFIG_BUS_DEFAULT_ADDRESS;

            //ADDRMASK 0; ADDREN disabled; 
            TWI0.SADDRMASK = 0x00;

            //DIEN enabled; APIEN enabled; PIEN disabled; PMEN disabled; SMEN disabled; ENABLE enabled; 
            TWI0.SCTRLA = TWI_APIEN_bm | TWI_DIEN_bm | TWI_PIEN_bm | TWI_ENABLE_bm;

            //ACKACT ACK; SCMD NOACT; 
            TWI0.SCTRLB = 0x00;

            //Peripheral Data
            TWI0.SDATA = 0x00;

            //DIF disabled; APIF disabled; COLL disabled; BUSERR disabled; 
            TWI0.SSTATUS = 0x00;
            break;
        case SystemMessage_Abort:
        case SystemMessage_Finit:
            TWI0.SCTRLA &= ~TWI_ENABLE_bm;
            break;
#if CONFIG_BUS_SIGNAL
        case SystemMessage_Loop:
            if (Bus.state == Bus_SendSignal) {
                uint8_t cmd = ((Bus.epcmd&Bus_SignalMask) >> Bus_SignalBp);
                Bus.epcmd &= ~Bus_SignalMask;
                if (Bus.active) {
                    message_send(Bus.active->task, BusMessage_SignalBase + cmd, MessageData_Empty);
                }
                Bus.state = Bus_Idle;
            }
            break;
#endif
        default: break;
    }
}
#endif