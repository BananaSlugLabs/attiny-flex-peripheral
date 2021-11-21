#include "common.h"

/*
 * Example using BusPirate
 * -----------------------------------------------------------------------------
 * Note: Need clock streching patch.
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

typedef enum BusStateTag {
    BusIdle = 0,
    BusSetIndexOrReadIncrement, // The first write after address will 
    BusReadOrWriteIncrement
} BusState;

#define Bus_RegisterFileBytes           ((uint8_t*)&Bus_RegisterFile)
#define Bus_FirmwareRegisterFileBytes   ((const uint8_t*)&Bus_FirmwareRegisterFile)
#define Bus_TotalRegisterFile           (sizeof(FirmwareRegisterFile)+sizeof(RegisterFile))

RegisterFile                    Bus_RegisterFile;
const FirmwareRegisterFile      Bus_FirmwareRegisterFile = {
    .ident                      = FW_IDENT,
    .version                    = FW_VERSION,
    .led_maximum                = CONFIG_LED_COUNT,
    .led_kind                   = LedKindWS2812,
    .led_format                 = LedFormatGRB888
};
static uint8_t                  Bus_Index = 0x00;
static BusState                 Bus_State = BusIdle;

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
            Bus_State = BusSetIndexOrReadIncrement;
        } else {
            TWI0.SCTRLB = TWI_CMD_DONE;
            Bus_State = BusIdle;
        }
        return;
    }
    
    if (s & TWI_DIF_bm) {
        if (s & TWI_DIR_bm) {
            // ************ Controller Read ****************************************
            if( (Bus_State == BusReadOrWriteIncrement && (s & TWI_RXACK_bm)) || Bus_Index >= Bus_TotalRegisterFile ) {
                TWI0.SCTRLB = TWI_CMD_DONE;
                Bus_State = BusIdle;
                return;
            }
            
            uint8_t val;
            if (Bus_Index < sizeof(FirmwareRegisterFile)) {
                val = Bus_FirmwareRegisterFileBytes[Bus_Index++]; 
            } else {
                val = Bus_RegisterFileBytes[(Bus_Index++)-sizeof(FirmwareRegisterFile)];
            }
            TWI0.SDATA = val; 
            TWI0.SCTRLB = TWI_CMD_ACK;
            Bus_State = BusReadOrWriteIncrement;
        } else {
            // ************ Controller Write ***************************************
            if (Bus_State == BusSetIndexOrReadIncrement) {
                Bus_Index = TWI0.SDATA;
                TWI0.SCTRLB = TWI_CMD_ACK;
                Bus_State = BusReadOrWriteIncrement;
            } else {
                if( Bus_Index < sizeof(FirmwareRegisterFile) ||  Bus_Index >= Bus_TotalRegisterFile ) {
                    TWI0.SCTRLB = TWI_CMD_NACK;
                    return;
                }
                uint8_t val = TWI0.SDATA;
                TWI0.SCTRLB = TWI_CMD_ACK;
                Bus_RegisterFileBytes[(Bus_Index++)-sizeof(FirmwareRegisterFile)] = val;
                return;
            }
        }
    }
}

void Command_Init() {
#if CONFIG_TWI_BUS
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
#endif
}

void Command_Finit() {
#if CONFIG_TWI_BUS
    TWI0.SCTRLA &= ~TWI_ENABLE_bm;
#endif
}

void Command_Task() {
#if CONFIG_TWI_BUS
    if (Bus_RegisterFile.deviceAddress != 0) {
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
            if (Bus_State == BusIdle && TWI0.SADDR != Bus_RegisterFile.deviceAddress) {
                TWI0.CTRLA = 0x00;
                TWI0.SCTRLA &= ~TWI_ENABLE_bm;
                TWI0.SADDR = Bus_RegisterFile.deviceAddress;
                TWI0.SCTRLA = TWI_APIEN_bm | TWI_DIEN_bm | TWI_PIEN_bm | TWI_ENABLE_bm;
                TWI0.SCTRLB = 0x00;
                TWI0.SDATA = 0x00;
                TWI0.SSTATUS = 0x00;
                TWI0.SCTRLA |= TWI_ENABLE_bm;
            }
        }
    }
#endif
}
