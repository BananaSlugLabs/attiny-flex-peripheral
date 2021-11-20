#include "common.h"

/*
 * Example using BusPirate
 * -----------------------------------------------------------------------------
 * Note: Need clock streching patch.
 * 
 *       Addr White          Green           Red            Blue                  Addr  Update
 * [0x52 0x04 0xFF 0xFF 0xFF 0xFF 0x00 0x00  0x00 0xFF 0x00 0x00 0x00 0xFF] [0x52 0x00 0x01]
 *       Addr  Update
 * [0x52 0x00 0x01]
 */

typedef enum BusStateTag {
    BusIdle = 0,
    BusWaitIndex,
    BusTransact
} BusState;

#define Bus_RegisterFileBytes ((uint8_t*)&Bus_RegisterFile)
RegisterFile    Bus_RegisterFile;
static uint8_t  Bus_Index = 0x00;
static BusState Bus_State = BusIdle;

#define TWI_CMD_ACK             (TWI_SCMD_RESPONSE_gc | TWI_ACKACT_ACK_gc)
#define TWI_CMD_NACK            (TWI_SCMD_RESPONSE_gc | TWI_ACKACT_NACK_gc)
#define TWI_CMD_DONE            (TWI_SCMD_COMPTRANS_gc)

#define TWI_STATUS_ERR(s)       ((s) & (TWI_COLL_bm|TWI_BUSERR_bm))
#define TWI_STATUS_WRITE(s)     (((s) & (TWI_DIF_bm|TWI_DIR_bm)) == (TWI_DIF_bm))
#define TWI_STATUS_READ(s)      (((s) & (TWI_DIF_bm|TWI_DIR_bm)) == (TWI_DIF_bm|TWI_DIR_bm))
#define TWI_STATUS_RXNAK(s)     ((s) & (TWI_RXACK_bm))
#define TWI_STATUS_ADDR(s)      (((s) & (TWI_APIF_bm|TWI_AP_bm)) == (TWI_APIF_bm|TWI_AP_bm))
#define TWI_STATUS_STOP(s)      (((s) & (TWI_APIF_bm|TWI_AP_bm)) == (TWI_APIF_bm))

ISR(TWI0_TWIS_vect) {
    uint8_t s = TWI0.SSTATUS;                //status copy
#if 0
    uint8_t isErr =    s & 0x0C;             //either- COLL, BUSERR
    uint8_t masterR = (s & 0x82) == 0x82;    //DIF, DIR(1=R)
    uint8_t masterW = (s & 0x82) == 0x80;    //DIF, DIR(0=W)
    uint8_t rxnack =   s & 0x10;             //RXACK(0=ACK,1=NACK)
    uint8_t isAddr =  (s & 0x41) == 0x41;    //APIF, AP(1=addr)
    uint8_t isStop =  (s & 0x41) == 0x40;    //APIF, AP(0=stop)
#endif
#if 0
    uint8_t isErr       = TWI_STATUS_ERR(s);
    uint8_t masterR     = TWI_STATUS_READ(s);
    uint8_t masterW     = TWI_STATUS_WRITE(s);
    uint8_t rxnack      = TWI_STATUS_RXNAK(s);
    uint8_t isAddr      = TWI_STATUS_ADDR(s);
    uint8_t isStop      = TWI_STATUS_STOP(s);
#endif
    // collision, buserror, or stop
#if 0
    if( isErr || isStop ) {
        TWI0.SCTRLB = TWI_CMD_DONE; // CMD: Done
        Bus_State = BusIdle;
        return;
    }
    
    if( isAddr ){ 
        TWI0.SCTRLB = TWI_CMD_ACK; // CMD: ACK
        Bus_State = BusWaitIndex;
        return;
    }
#else
    if (s & TWI_APIF_bm) {
        if (s & TWI_AP_bm) {
            TWI0.SCTRLB = TWI_CMD_ACK;
            Bus_State = BusWaitIndex;
        } else {
            TWI0.SCTRLB = TWI_CMD_DONE;
            Bus_State = BusIdle;
        }
        return;
    }
#endif
#if 0
    //data, master read
    if( masterR ){
        if( (Bus_Index && rxnack) || (Bus_Index >= sizeof(Bus_RegisterFile)) ) {
            TWI0.SCTRLB = TWI_CMD_DONE; // CMD: Complete
            return;
        }
        TWI0.SDATA = Bus_RegisterFileBytes[Bus_Index++];
        TWI0.SCTRLB = TWI_CMD_ACK; // CMD: ACK
        return;
    }
    //data, master write
    if( masterW ){
        if (Bus_State == BusWaitIndex) {
            Bus_Index = TWI0.SDATA;
            Bus_State = BusTransact;
            TWI0.SCTRLB = TWI_CMD_ACK; // CMD: ACK
        } else {
            if( Bus_Index >= sizeof(Bus_RegisterFile) ) {
                TWI0.SCTRLB = TWI_CMD_NACK; // CMD: NACK
                return;
            }
            Bus_RegisterFileBytes[Bus_Index++] = TWI0.SDATA;
            TWI0.SCTRLB = TWI_CMD_ACK; // CMD: ACK
            return;
        }
    }
#else
    if (s & TWI_DIF_bm) {
        if (s & TWI_DIR_bm) {
            // ************ Master Read ****************************************
            if( (Bus_Index && (s & TWI_RXACK_bm)) || (Bus_Index >= sizeof(Bus_RegisterFile)) ) {
                TWI0.SCTRLB = TWI_CMD_DONE;
                return;
            }
            TWI0.SDATA = Bus_RegisterFileBytes[Bus_Index++];
            TWI0.SCTRLB = TWI_CMD_ACK;
        } else {
            // ************ Master Write ***************************************
            if (Bus_State == BusWaitIndex) {
                Bus_Index = TWI0.SDATA;
                Bus_State = BusTransact;
                TWI0.SCTRLB = TWI_CMD_ACK;
            } else {
                if( Bus_Index >= sizeof(Bus_RegisterFile) ) {
                    TWI0.SCTRLB = TWI_CMD_NACK;
                    return;
                }
                Bus_RegisterFileBytes[Bus_Index++] = TWI0.SDATA;
                TWI0.SCTRLB = TWI_CMD_ACK;
                return;
            }
        }
    }
#endif
}

#if 0
uint8_t twis_lastAddress;   //last address we responded as
bool    twis_isBusy;        //busy from address acked until stop
uint8_t twis_buf[16];       //both rx and tx buffer
void    twis_irqAllOn   ()  { TWI0.SCTRLA |= 0xE0; }
void    twis_irqAllOff  ()  { TWI0.SCTRLA &= ~0xE0; }
void    twis_clearFlags ()  { TWI0.SSTATUS = 0xCC; }
void    twis_ack        ()  { TWI0.SCTRLB = 3; } //RESPONSE, ACK
void    twis_nack       ()  { TWI0.SCTRLB = 7; } //RESPONSE, NACK
void    twis_complete   ()  { TWI0.SCTRLB = 2; } //COMPTRANS
void    twis_end        ()  { twis_complete(); twis_isBusy = false; } 
void    twis_baudReg    (uint8_t v) { TWI0.MBAUD = v; }
#endif
#if 0
#define TWI_ACK()   TWI0.SCTRLB      = TWI_ACKACT_ACK_gc | TWI_SCMD_RESPONSE_gc
#define TWI_NACK()  TWI0.SCTRLB      = TWI_SCMD_RESPONSE_gc
#define TWI_DONE()  TWI0.SCTRLB      = TWI_SCMD_COMPTRANS_gc

ISR(TWI0_TWIS_vect) {
    static uint8_t idx;                      //index into buffer
    uint8_t s = TWI0.SSTATUS;                //status copy

    uint8_t isErr =    s & 0x0C;             //either- COLL, BUSERR
    uint8_t masterR = (s & 0x82) == 0x82;    //DIF, DIR(1=R)
    uint8_t masterW = (s & 0x82) == 0x80;    //DIF, DIR(0=W)
    uint8_t rxnack =   s & 0x10;             //RXACK(0=ACK,1=NACK)
    uint8_t isAddr =  (s & 0x41) == 0x41;    //APIF, AP(1=addr)
    uint8_t isStop =  (s & 0x41) == 0x40;    //APIF, AP(0=stop)

    // collision, buserror, or stop
    if( isErr || isStop ) {
        TWI_DONE();
        Bus.state = BusIdle;
        return;
    }
    //address
    if( isAddr ){ 
        idx = 0; 
        Bus.state = BusBusy; 
        Bus.lastAddress = TWI0.SDATA>>1; //back to a real address
        TWI_ACK();
        return;
    }
    //data, master read
    if( masterR ){
        if( rxnack ) {
            TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
            return;
        }
        TWI0.SDATA = Command_TestReg;
        TWI_ACK();
        return;
    }
    //data, master write
    if( masterW ){
        //if( idx >= sizeof(twis_buf) ) return twis_nack(); //cannot accept
        Command_TestReg = TWI0.SDATA;
        TWI0.SDATA = Command_TestReg;
        TWI_ACK();
        return;
    }                 
}     
#endif

void Command_Init() {
    //SDASETUP 4CYC; SDAHOLD OFF; FMPEN disabled; 
    TWI0.CTRLA = 0x00;
    
    //Debug Run
    TWI0.DBGCTRL = 0x00;
    
    //Slave Address
    TWI0.SADDR = CONFIG_TWI_ADDR_DEFAULT;
    
    //ADDRMASK 0; ADDREN disabled; 
    TWI0.SADDRMASK = 0x00;
    
    //DIEN enabled; APIEN enabled; PIEN disabled; PMEN disabled; SMEN disabled; ENABLE enabled; 
    TWI0.SCTRLA = TWI_APIEN_bm | TWI_DIEN_bm | TWI_ENABLE_bm;
    
    //ACKACT ACK; SCMD NOACT; 
    TWI0.SCTRLB = 0x00;
    
    //Slave Data
    TWI0.SDATA = 0x00;
    
    //DIF disabled; APIF disabled; COLL disabled; BUSERR disabled; 
    TWI0.SSTATUS = 0x00;
}

void Command_Task() {
    
}

#if 0
#define TWI_AP_bm  0x01  /* Slave Address or Stop bit mask. */
#define TWI_AP_bp  0  /* Slave Address or Stop bit position. */
#define TWI_DIR_bm  0x02  /* Read/Write Direction bit mask. */
#define TWI_DIR_bp  1  /* Read/Write Direction bit position. */
/* TWI_BUSERR  is already defined. */
#define TWI_COLL_bm  0x08  /* Collision bit mask. */
#define TWI_COLL_bp  3  /* Collision bit position. */
/* TWI_RXACK  is already defined. */
/* TWI_CLKHOLD  is already defined. */
#define TWI_APIF_bm  0x40  /* Address/Stop Interrupt Flag bit mask. */
#define TWI_APIF_bp  6  /* Address/Stop Interrupt Flag bit position. */
#define TWI_DIF_bm  0x80  /* Data Interrupt Flag bit mask. */
#define TWI_DIF_bp  7  /* Data Interrupt Flag bit position. */
#endif

#if 0
static inline bool isError (uint8_t v)  { return v & 0x0C; } //either- COLL, BUSERR
static inline bool isDataRead (uint8_t v)  { return (v & 0x82) == 0x82; } //DIF, DIR(1=R)
static inline bool isDataWrite (uint8_t v)  { return (v & 0x82) == 0x80; } //DIF, DIR(0=W)
static inline bool isAddress (uint8_t v)  { return (v & 0x41) == 0x41; } //APIF, AP(1=addr)
static inline bool isStop (uint8_t v)  { return (v & 0x41) == 0x40; } //APIF, AP(0=stop)
static inline bool isRxNack (uint8_t v)  { return v & 0x10; } //RXACK(0=ACK,1=NACK)
static inline bool isIrq (uint8_t v)  { return v & 0xC0; } //DIF or APIF

ISR(TWI0_TWIS_vect) {
    uint8_t status = TWI0.SSTATUS;
    if ( isError(status) || isStop(status)) {
        TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc;
            return;
    }
    if (isAddress(status)) {
        Command_Addr=TWI0.SDATA;
    }
    
    if( isDataRead(status) ){
        if( isRxNack(status) ) {
            TWI0.SCTRLB = TWI_SCMD_RESPONSE_gc;
            return;
        }
        Command_TestReg = TWI0.SDATA;
        TWI0.SCTRLB = TWI_ACKACT_ACK_gc | TWI_SCMD_RESPONSE_gc;
        return;
    }
    
    if( isDataWrite(status) ){
        Command_TestReg = TWI0.SDATA;
        TWI0.SCTRLB = TWI_ACKACT_ACK_gc | TWI_SCMD_RESPONSE_gc;
        return;
    }
}
#endif

#if 0
uint8_t twis_lastAddress;   //last address we responded as
bool    twis_isBusy;        //busy from address acked until stop
uint8_t twis_buf[16];       //both rx and tx buffer
void    twis_irqAllOn   ()  { TWI0.SCTRLA |= 0xE0; }
void    twis_irqAllOff  ()  { TWI0.SCTRLA &= ~0xE0; }
void    twis_clearFlags ()  { TWI0.SSTATUS = 0xCC; }
void    twis_ack        ()  { TWI0.SCTRLB = 3; } //RESPONSE, ACK
void    twis_nack       ()  { TWI0.SCTRLB = 7; } //RESPONSE, NACK
void    twis_complete   ()  { TWI0.SCTRLB = 2; } //COMPTRANS
void    twis_end        ()  { twis_complete(); twis_isBusy = false; } 
void    twis_baudReg    (uint8_t v) { TWI0.MBAUD = v; }

void twis_on (uint8_t addr) { 
    twis_complete();
    twis_irqAllOn(); 
    TWI0.SADDR = addr<<1; //use a real address
    if( TWI0.MBAUD == 0 ) {
        twis_baudReg(95); //a safe value if cpu speed unknown
    }
    TWI0.SCTRLA |= 1; 
    sei();
}
void twis_off () { 
    twis_irqAllOff();
    TWI0.SCTRLA &= ~1;
}

ISR(TWI0_TWIS_vect) {
    static uint8_t idx;                      //index into buffer
    uint8_t s = TWI0.SSTATUS;                //status copy

    uint8_t isErr =    s & 0x0C;             //either- COLL, BUSERR
    uint8_t masterR = (s & 0x82) == 0x82;    //DIF, DIR(1=R)
    uint8_t masterW = (s & 0x82) == 0x80;    //DIF, DIR(0=W)
    uint8_t rxnack =   s & 0x10;             //RXACK(0=ACK,1=NACK)
    uint8_t isAddr =  (s & 0x41) == 0x41;    //APIF, AP(1=addr)
    uint8_t isStop =  (s & 0x41) == 0x40;    //APIF, AP(0=stop)

    // collision, buserror, or stop
    if( isErr || isStop ) return twis_end();
    //address
    if( isAddr ){ 
        idx = 0; 
        twis_isBusy = true; 
        twis_lastAddress = TWI0.SDATA>>1; //back to a real address
        return twis_ack();
    }
    //data, master read
    if( masterR ){
        if( (idx && rxnack) || (idx >= sizeof(twis_buf)) ) return twis_complete();
        TWI0.SDATA = twis_buf[idx++];
        return twis_ack();
    }
    //data, master write
    if( masterW ){
        if( idx >= sizeof(twis_buf) ) return twis_nack(); //cannot accept
        twis_buf[idx++] = TWI0.SDATA;
        return twis_ack();
    }                 
}           
#endif
#if 0
/* I2C Internal API's */
/* Slave */
bool I2C0_SlaveIsAddressInterrupt(void);
bool I2C0_SlaveIsDataInterrupt(void);
bool I2C0_SlaveIsStopInterrupt(void);
void I2C0_SlaveOpen(void);
void I2C0_SlaveClose(void);
bool I2C0_SlaveDIR(void);
char I2C0_SlaveReset(void);
uint8_t I2C0_SlaveRead(void);
char I2C0_SlaveWrite(uint8_t data);
bool I2C0_SlaveIsNack(void);
void I2C0_SlaveSendAck(void);
void I2C0_SlaveSendNack(void);
bool I2C0_SlaveIsBusCollision(void);
bool I2C0_SlaveIsBusError(void);
bool I2C0_SlaveIsTxComplete(void);

// Read Event Interrupt Handlers
void I2C0_ReadCallback(void);
void (*I2C0_ReadInterruptHandler)(void);

// Write Event Interrupt Handlers
void I2C0_WriteCallback(void);
void (*I2C0_WriteInterruptHandler)(void);

// Address Event Interrupt Handlers
void I2C0_AddressCallback(void);
void (*I2C0_AddressInterruptHandler)(void);

// Stop Event Interrupt Handlers
void I2C0_StopCallback(void);
void (*I2C0_StopInterruptHandler)(void);

// Bus Collision Event Interrupt Handlers
void I2C0_CollisionCallback(void);
void (*I2C0_CollisionInterruptHandler)(void);

// Bus Error Event Interrupt Handlers
void I2C0_BusErrorCallback(void);
void (*I2C0_BusErrorInterruptHandler)(void);

uint8_t I2C0_Initialize()
{
    //SDASETUP 4CYC; SDAHOLD OFF; FMPEN disabled; 
    TWI0.CTRLA = 0x00;
    
    //Debug Run
    TWI0.DBGCTRL = 0x00;
    
    //Slave Address
    TWI0.SADDR = 0x00;
    
    //ADDRMASK 0; ADDREN disabled; 
    TWI0.SADDRMASK = 0x00;
    
    //DIEN enabled; APIEN enabled; PIEN disabled; PMEN disabled; SMEN disabled; ENABLE enabled; 
    TWI0.SCTRLA = 0xC1;
    
    //ACKACT ACK; SCMD NOACT; 
    TWI0.SCTRLB = 0x00;
    
    //Slave Data
    TWI0.SDATA = 0x00;
    
    //DIF disabled; APIF disabled; COLL disabled; BUSERR disabled; 
    TWI0.SSTATUS = 0x00;
    
    I2C0_SetWriteCallback(NULL);
    I2C0_SetReadCallback(NULL);
    I2C0_SetAddressCallback(NULL);
    I2C0_SetStopCallback(NULL);
    I2C0_SetCollisionCallback(NULL);
    I2C0_SetBusErrorCallback(NULL);
  
    return 0;
}

void I2C0_Open(void)
{
    I2C0_SlaveOpen();
}

void I2C0_Close(void)
{
    I2C0_SlaveClose();
}

ISR(TWI0_TWIS_vect)
{
    if (I2C0_SlaveIsBusCollision()) {
        I2C0_CollisionCallback();
        return;
    }

    if (I2C0_SlaveIsBusError()) {
        I2C0_BusErrorCallback();
        return;
    }

    if (I2C0_SlaveIsAddressInterrupt()) {
        I2C0_AddressCallback();
        if (I2C0_SlaveDIR()) {
            // Master wishes to read from slave
            I2C0_ReadCallback();
            I2C0_SlaveSendAck();
        }
        return;
    }
    if (I2C0_SlaveIsDataInterrupt()) {
        if (I2C0_SlaveDIR()) {
            // Master wishes to read from slave
            if (!I2C0_SlaveIsNack()) {
                // Received ACK from master
                I2C0_ReadCallback();
                I2C0_SlaveSendAck();
            } else {
                // Received NACK from master
                I2C0_GotoUnaddressed();
            }
        } else // Master wishes to write to slave
        {
            I2C0_WriteCallback();
        }
        return;
    }

    // Check if STOP was received
    if (I2C0_SlaveIsStopInterrupt()) {
        I2C0_StopCallback();
        I2C0_SlaveIsTxComplete(); // To check the status of the transaction
        return;
    }
}


uint8_t I2C0_Read(void)
{
    return I2C0_SlaveRead();
}

void I2C0_Write(uint8_t data)
{
    I2C0_SlaveWrite(data);
}

void I2C0_Enable(void)
{
    I2C0_SlaveOpen();
}

void I2C0_SendAck(void)
{
    I2C0_SlaveSendAck();
}

void I2C0_SendNack(void)
{
    I2C0_SlaveSendNack();
}

void I2C0_GotoUnaddressed(void)
{
    // Reset module
    I2C0_SlaveReset();
}

// Read Event Interrupt Handlers
void I2C0_ReadCallback(void)
{
    if (I2C0_ReadInterruptHandler) {
        I2C0_ReadInterruptHandler();
    }
}

void I2C0_SetReadCallback(TWI0_callback handler)
{
    I2C0_ReadInterruptHandler = handler;
}

// Write Event Interrupt Handlers
void I2C0_WriteCallback(void)
{
    if (I2C0_WriteInterruptHandler) {
        I2C0_WriteInterruptHandler();
    }
}

void I2C0_SetWriteCallback(TWI0_callback handler)
{
    I2C0_WriteInterruptHandler = handler;
}

// Address Event Interrupt Handlers
void I2C0_AddressCallback(void)
{
    if (I2C0_AddressInterruptHandler) {
        I2C0_AddressInterruptHandler();
    }
}

void I2C0_SetAddressCallback(TWI0_callback handler)
{
    I2C0_AddressInterruptHandler = handler;
}

// Stop Event Interrupt Handlers
void I2C0_StopCallback(void)
{
    if (I2C0_StopInterruptHandler) {
        I2C0_StopInterruptHandler();
    }
}

void I2C0_SetStopCallback(TWI0_callback handler)
{
    I2C0_StopInterruptHandler = handler;
}

// Bus Collision Event Interrupt Handlers
void I2C0_CollisionCallback(void)
{
    if (I2C0_CollisionInterruptHandler) {
        I2C0_CollisionInterruptHandler();
    }
}

void I2C0_SetCollisionCallback(TWI0_callback handler)
{
    I2C0_CollisionInterruptHandler = handler;
}

// Bus Error Event Interrupt Handlers
void I2C0_BusErrorCallback(void)
{
    if (I2C0_BusErrorInterruptHandler) {
        I2C0_BusErrorInterruptHandler();
    }
}

void I2C0_SetBusErrorCallback(TWI0_callback handler)
{
    I2C0_BusErrorInterruptHandler = handler;
}


/* Slave Configurations */
void I2C0_SlaveOpen(void)
{
    TWI0.SCTRLA |= TWI_ENABLE_bm;
}

void I2C0_SlaveClose(void)
{
    TWI0.SCTRLA &= ~TWI_ENABLE_bm;
}

bool I2C0_SlaveIsBusCollision(void)
{
    return TWI0.SSTATUS & TWI_COLL_bm;
}

bool I2C0_SlaveIsBusError(void)
{
    return TWI0.SSTATUS & TWI_BUSERR_bm;
}

bool I2C0_SlaveIsAddressInterrupt(void)
{
    return (TWI0.SSTATUS & TWI_APIF_bm) && (TWI0.SSTATUS & TWI_AP_bm);
}

bool I2C0_SlaveIsDataInterrupt(void)
{
    return TWI0.SSTATUS & TWI_DIF_bm;
}

bool I2C0_SlaveIsStopInterrupt(void)
{
    return (TWI0.SSTATUS & TWI_APIF_bm) && (!(TWI0.SSTATUS & TWI_AP_bm));
}

bool I2C0_SlaveDIR(void)
{
    return TWI0.SSTATUS & TWI_DIR_bm;
}

void I2C0_SlaveSendAck(void)
{
    TWI0.SCTRLB = TWI_ACKACT_ACK_gc | TWI_SCMD_RESPONSE_gc;
}

void I2C0_SlaveSendNack(void)
{
    TWI0.SCTRLB = TWI_ACKACT_NACK_gc | TWI_SCMD_COMPTRANS_gc;
}

bool I2C0_SlaveIsNack(void)
{
    return TWI0.SSTATUS & TWI_RXACK_bm;
}

bool I2C0_SlaveIsTxComplete(void)
{
    TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
    return TWI0.SCTRLB;
}

uint8_t I2C0_SlaveRead(void)
{
    return TWI0.SDATA;
}

char I2C0_SlaveWrite(uint8_t data)
{
    TWI0.SDATA = data;
    TWI0.SCTRLB |= TWI_SCMD_RESPONSE_gc;
    return TWI0.SDATA;
}

char I2C0_SlaveReset(void)
{
    TWI0.SSTATUS |= (TWI_DIF_bm | TWI_APIF_bm);
    TWI0.SCTRLB = TWI_SCMD_COMPTRANS_gc;
    return TWI0.SSTATUS;
}


#endif