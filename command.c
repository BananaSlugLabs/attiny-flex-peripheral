#include "bus.h"

/*
 * Example using BusPirate
 * -----------------------------------------------------------------------------
 * Note: Need clock streching patch.
 * 
 * [0x52 0x00 0x00 [0x53 r:2]
 * 
 * Get Info:
 *         Addr
 *   [0x52 0x00 [0x53 r:2] -> Identifier           (0x2812 LE)
 *   [0x52 0x02 [0x53 r:2] -> Version              (0x1)
 *   [0x52 0x03 [0x53 r:2] -> Total LED Count      (24)
 *   [0x52 0x04 [0x53 r:1] -> LED Kind             (0x1 WS2812B-like)
 *   [0x52 0x05 [0x53 r:1] -> LED Format           (0x1 GRB888)
 *   [0x52 0x00 [0x53 r:6]
 * 
 * Set I2C Addr:
 *         Addr I2cAddr (Originally 0x82)
 *   [0x52 0x0A 0x50]
 * 
 * Set LEDs:
 *         Addr White          Green           Red            Blue
 *   [0x52 0x12 0xFF 0xFF 0xFF 0xFF 0x00 0x00  0x00 0xFF 0x00 0x00 0x00 0xFF]
 * 
 * Set Update bit in LED Control Register:
 *         Addr Upd
 *   [0x52 0x11 0x01]
 * 
 * Set LED Count:
 *         Addr Cnt
 *   [0x52 0x10 0x02]
 * 
 * Set LED Count & Update:
 *         Addr Cnt  Upd
 *   [0x52 0x10 0x03 0x1]
 * 
 * Read LED Contents:
 *   [0x52 0x12 [0x53 r:32]]
 * 
 * And together...
 *         Addr LED Data....................................................        Addr Cnt  Upd
 *   [0x52 0x12 0xFF 0xFF 0xFF 0xFF 0x00 0x00  0x00 0xFF 0x00 0x00 0x00 0xFF] [0x52 0x10 0x04 0x1]
 * 
 * 
 *   [0x52 0x0A 0x80]
 *   [0x50 0x12 0xFF 0xFF 0xFF 0xFF 0x00 0x00  0x00 0xFF 0x00 0x00 0x00 0xFF] [0x50 0x10 0x04 0x1]
 */

#if CONFIG_TWI_BUS
enum PixelKindTag {
    LedKindWS2812         = 1,
};
typedef uint8_t LedKind;

enum PixelFormatTag {
    LedFormatGRB888       = 1
};
typedef uint8_t LedFormat;
typedef uint8_t LedCount;


enum PersistKeyTag {
    PersistKeyValue       = 0x9C
};
typedef uint8_t PersistKey;

typedef struct PersistControlTag {
    bool    saveLed     : 1;
    bool    storeAddr   : 1;
    bool    reload      : 1;
    bool    busy        : 1;
} PersistControl;

typedef struct FirmwareRegisterFileTag {
    uint16_t        ident;                                                  // 0..1
    uint8_t         version;                                                // 2
    uint8_t         led_maximum;                                            // 3
    LedKind         led_kind;                                               // 4
    LedFormat       led_format;                                             // 5
    uint16_t        _r1;                                                    // 6..7
} device_registerfile_t;

const device_registerfile_t Device_RegisterFile = {
    .ident                      = FW_IDENT,
    .version                    = FW_VERSION,
    .led_maximum                = CONFIG_LED_COUNT,
    .led_kind                   = LedKindWS2812,
    .led_format                 = LedFormatGRB888
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
    Bus_Done                = 4,
    Bus_Error               = 5,
} bus_state_t;

enum {
    Bus_EpMask              = 0x0F,
    Bus_CmdMask             = 0xE0
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

// Not needed, but kept around for reference.
#if 0
#define TWI_STATUS_ERR(s)       ((s) & (TWI_COLL_bm|TWI_BUSERR_bm))
#define TWI_STATUS_WRITE(s)     (((s) & (TWI_DIF_bm|TWI_DIR_bm)) == (TWI_DIF_bm))
#define TWI_STATUS_READ(s)      (((s) & (TWI_DIF_bm|TWI_DIR_bm)) == (TWI_DIF_bm|TWI_DIR_bm))
#define TWI_STATUS_RXNAK(s)     ((s) & (TWI_RXACK_bm))
#define TWI_STATUS_ADDR(s)      (((s) & (TWI_APIF_bm|TWI_AP_bm)) == (TWI_APIF_bm|TWI_AP_bm))
#define TWI_STATUS_STOP(s)      (((s) & (TWI_APIF_bm|TWI_AP_bm)) == (TWI_APIF_bm))
#endif

ISR(TWI0_TWIS_vect) {
    uint8_t s = TWI0.SSTATUS;

    if (s & TWI_APIF_bm) {
        if (s & TWI_AP_bm) {
            TWI0.SCTRLB = TWI_CMD_ACK;
            Bus.state = Bus_ReadOrSelectEp;
        } else {
            TWI0.SCTRLB = TWI_CMD_DONE;
            Bus.state = Bus_Idle;
        }
        return;
    }
    
    if (s & TWI_DIF_bm) {
        if (s & TWI_DIR_bm) {
            // ************ Controller Read ****************************************
            if( /*(Bus.state == Bus_ReadOrSelectEp && (s & TWI_RXACK_bm)) ||*/ !Bus.active || Bus.activeIndex >= Bus.active->size) {
                DEBUG_BREAKPOINT();
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
                    DEBUG_BREAKPOINT();
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
            TWI0.SADDR = CONFIG_TWI_ADDR_DEFAULT;

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
        default: break;
    }
}
#endif