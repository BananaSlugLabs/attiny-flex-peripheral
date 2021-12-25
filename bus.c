#include "bus_internal.h"
#include "sys.h"
#include "avr/eeprom.h"

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
Bus_State               bus_state;
Bus_CommandContext      bus_commandContext;

extern const Bus_MemMap _bus_memmap_start;
extern const uint8_t    _bus_memmap_count;

const Bus_BuildConfig   bus_buildconfig;

Bus_IONvmConfig         bus_iomem_nvm EEMEM = {
    .deviceAddress = 0xFF
};

const Bus_IODeviceInfo  bus_iomem_devinfo = {
    .mfg                = CONFIG_FW_MFG,
    .ident              = CONFIG_FW_IDENT,
    .version            = CONFIG_FW_VERSION,
};

Bus_IOControl           bus_iomem_control;

const Bus_MemMap        bus_iomap_control       = { .data = (uint8_t*)&bus_iomem_control, .length = sizeof(bus_iomem_control) };
const Bus_MemMap        bus_iomap_deviceInfo    = { .data = (uint8_t*)&bus_iomem_devinfo, .length = sizeof(bus_iomem_devinfo) };
uint8_t                 twi_break               = 0;

extern const Bus_Command _bus_command_map_start[1];
extern const Bus_Command _bus_command_map_end[1];

Bus_DefineMemoryMap(bus_iomem_control, BUS_PRIORITY_000); // always slot 0
Bus_DefineMemoryMap(bus_iomem_devinfo, BUS_PRIORITY_001); // always slot 1

#define TWI_CMD_ACK     (TWI_SCMD_RESPONSE_gc | TWI_ACKACT_ACK_gc)
#define TWI_CMD_NACK    (TWI_SCMD_RESPONSE_gc | TWI_ACKACT_NACK_gc)
#define TWI_CMD_DONE    (TWI_SCMD_COMPTRANS_gc)

#define HIMEM_MASK      0xF0
#define CONFIG_PAGE     0
#define DEVINFO_PAGE    1
#define ADDRRW_MASK     0xF000
#define ADDRRW_MATCH    0x3000

#define Map_Get(index)  (&(((Bus_MemMap*)&_bus_memmap_start)[index]))
#define Map_GetActive() Map_Get(bus_state.activePage)
#define Map_Count()     ((uint8_t)((uintptr_t)&_bus_memmap_count))

ISR(TWI0_TWIS_vect) {
    uint8_t s = TWI0.SSTATUS;
    
    if (s & TWI_APIF_bm) {
        if (s & TWI_AP_bm) {
            if (bus_state.state != Bus_IoFinish) {
                TWI0.SCTRLB = TWI_CMD_ACK;
                bus_state.state = Bus_AddressOrRead;
            } else {
                TWI0.SCTRLB = TWI_CMD_NACK;
                //DEBUG_BREAKPOINT();
            }
        } else {
            uint8_t command;
            if (bus_state.state != Bus_ReadWrite) { // No writes were performed
                command = 0;
            } else if (bus_state.activePage == CONFIG_PAGE) {
                command = bus_iomem_control.command;
                bus_iomem_control.command = 0;
            } else {
                command = bus_iomem_control.txCommand;
            }

            if (command > 0) {
                if (bus_commandContext.command != 0) {
                    // Only one command active at a time...
                    bus_iomem_control.status |= Bus_StatusBusError;
                    bus_state.state = Bus_Idle;
                } else {
                    // Reset busy flag after next successful command...
                    bus_iomem_control.status = (bus_iomem_control.status & (~Bus_StatusCode_bm | Bus_StatusBusError)) | Bus_StatusInProgress;
                    bus_commandContext.command = command;
                    bus_commandContext.lastAddress = (void*)Map_GetActive()->data;
                    bus_state.state = Bus_IoFinish;
                }
            } else {
                bus_state.state = Bus_Idle;
            }
            sys_signal(Sys_SignalWakeLock|Sys_SignalWorkerPending);
            // need to defer this since we can't meet timing requirements. May have to replicate in other cases.
            TWI0.SCTRLB = TWI_CMD_DONE;
        }
        return;
    }
    
    if (s & TWI_DIF_bm) {
        sys_signal(Sys_SignalWakeLock);
        if (s & TWI_DIR_bm) {
            // ************ Controller Read ****************************************
            TWI0.SDATA = Map_GetActive()->data[bus_state.offset]; 
            TWI0.SCTRLB = TWI_CMD_ACK;
            
            bus_state.offset = bus_state.offset + 1;
            if (bus_state.offset >= Map_GetActive()->length) {
                bus_state.offset = 0;
            }
        } else {
            // ************ Controller Write ***************************************
            switch (bus_state.state) {
                case Bus_AddressOrRead:
                {
                    uint8_t addr = TWI0.SDATA;
                    if (addr & HIMEM_MASK) {
                        bus_state.activePage    = CONFIG_PAGE;
                        addr                    = addr & ~HIMEM_MASK;
                    } else {
                        bus_state.activePage    = bus_state.page;
                    }
                    if (addr >= Map_GetActive()->length) {
                        TWI0.SCTRLB = TWI_CMD_NACK;
                        bus_state.offset        = 0;
                        bus_state.state         = Bus_Error;
                        DEBUG_BREAKPOINT();
                    } else {
                        TWI0.SCTRLB = TWI_CMD_ACK;
                        bus_state.offset        = addr;
                        if ( (((uintptr_t)Map_GetActive()->data) & ADDRRW_MASK) == ADDRRW_MATCH ) {
                            bus_state.state     = Bus_ReadWrite0;
                        } else {
                            bus_state.state     = Bus_ReadOnly;
                        }
                    }
                    break;
                }
                case Bus_ReadWrite0:
                    bus_state.state = Bus_ReadWrite; // track the first write
                case Bus_ReadWrite:
                {
                    uint8_t val = TWI0.SDATA;
                    TWI0.SCTRLB = TWI_CMD_ACK;
                    Map_GetActive()->data[bus_state.offset] = val;
                    bus_state.offset = bus_state.offset + 1;
                    if (bus_state.offset >= Map_GetActive()->length) {
                        bus_state.offset = 0;
                    }
                    break;
                }
                
                case Bus_ReadOnly:
                {
                    FORCE_READ(TWI0.SDATA); // otherwise the compile optimizes the read
                    TWI0.SCTRLB = TWI_CMD_NACK;
                    break;
                }
                
                case Bus_Error:
                default:
                {
                    FORCE_READ(TWI0.SDATA); // otherwise the compile optimizes the read
                    TWI0.SCTRLB = TWI_CMD_NACK;
                    break;
                }
            }
        }
    }
}

static void bus_init();
static void bus_finit();
static void bus_loop();

SysInit_Subscribe(bus_init,         Signal_Normal);
SysFinit_Subscribe(bus_finit,       Signal_Normal);
SysAbort_Subscribe(bus_finit,       Signal_Normal);
SysLoop_Subscribe(bus_loop,         Signal_Normal);

static void bus_init() {
    TWI0.SCTRLA = 0;
    
    bus_state.page = DEVINFO_PAGE;
    
    TWI0.CTRLA = TWI_SDAHOLD_50NS_gc;
    TWI0.DBGCTRL = 0x00;

    //Peripheral Address
    TWI0.SADDR = CONFIG_BUS_DEFAULT_ADDRESS;
    uint8_t addr = eeprom_read_byte(&bus_iomem_nvm.deviceAddress);
    if ((addr & 0x1) || !addr) {
        TWI0.SADDR = CONFIG_BUS_DEFAULT_ADDRESS;
    } else {
        TWI0.SADDR = addr;
    }

    TWI0.SADDRMASK = 0x00;
    TWI0.SCTRLA = TWI_APIEN_bm | TWI_DIEN_bm | TWI_PIEN_bm | TWI_ENABLE_bm | TWI_SMEN_bm;
    TWI0.SCTRLB = 0x00;
    TWI0.SDATA = 0x00;
    TWI0.SSTATUS = 0x00;
}

static void bus_finit() {
    TWI0.SCTRLA &= ~TWI_ENABLE_bm;
}

void bus_commandUpdateStatusIRQ (uint8_t status) {
    bus_iomem_control.status = (bus_iomem_control.status & ~Bus_StatusCode_bm) | status;
    
    if (status != Bus_StatusInProgress) {
        bus_commandContext.lastCommand = bus_commandContext.command;
        bus_commandContext.command = 0;
    }
}

void bus_commandUpdateStatus (uint8_t status) {
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        bus_commandUpdateStatusIRQ(status);
    }
}

static void bus_loop() {
    if ( bus_state.state == Bus_IoFinish ) {
        bus_status_t status         = Bus_StatusInProgress;
        bus_command_t command       = bus_commandContext.command;
        bus_state.state             = Bus_Idle;
        
        bus_commandUpdateStatus(Bus_StatusInProgress);
        
        if (command < 8) {
            uint8_t param;
            switch (command) {
            case Bus_CommandSetPage:
                param = bus_iomem_control.params[0];
                if (param > 0 && param < Map_Count()) {
                    bus_state.page = param;
                    status = Bus_StatusSuccess;
                } else {
                    status = Bus_StatusErrorArgument;
                }
                break;

            case Bus_CommandSetDevAddr:
                param = bus_iomem_control.params[0];
                if (param == 0 || (param & 1)) {
                    status = Bus_StatusErrorArgument;
                } else {
                    status = Bus_StatusSuccess;
                }
                if (param!=eeprom_read_byte(&bus_iomem_nvm.deviceAddress)) {
                    eeprom_write_byte(&bus_iomem_nvm.deviceAddress, param);
                }
                break;

            case Bus_CommandReset:
                //status = Bus_StatusSuccess; // reset never has an opportunity to report the status
                sys_restart();
                break;

            default:
                status = Bus_StatusErrorCommand;
                break;
            }
        } else {
            bool found = false;
            for (const Bus_Command* cmd = _bus_command_map_start;
                    cmd <= _bus_command_map_end;
                    cmd ++) {
                if ((cmd->mask & command) == cmd->match) {
                    status = cmd->handler();
                    found = true;
                }
            }
            if (!found) {
                status = Bus_StatusErrorCommand;
            }
        }
        bus_commandUpdateStatus(status);
    }
}
#endif