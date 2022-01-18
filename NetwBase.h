/*
 *  s 311 44  S 3 11 11 44
 *  e
 *  u
 *  r
 *
 * */

#include "Arduino.h"
#include <inttypes.h>
#include <EEPROM.h>
 

#ifndef NETWBASE_H
#define NETWBASE_H

//#include <avr/wdt.h>        // watchdog

#define NW_TIMERS_COUNT 7  		
#define NW_TIMER_BUSY 0
#define NW_TIMER_PING 1
#define NW_TIMEOUT_OVERALL 2   // ?? when stop
#define NW_TIMEOUT_RX 3
#define NW_TIMEOUT_TX 4
#define NW_TIMER_UPLOAD_DEBUG 5
#define NW_TIMER2_UPLOAD_DEBUG 6
 


// #define PING_TIMER 						7000L
#define TIMEOUT 						20000L

#define NETW_SEND_SERIAL_DATA_INTERVAL 	7L

#define NETW_I2C_SEND_ERROR_INTERVAL	70L

#define TWI_FREQ 100000L  // (250.000L TWBR=24),  100.000L > TWBR=72  50.000L > 152
// change  TWI_FREQ  		100000=1020 12byte treansactions, 50000L=520 12Byte transactions   	32000L// slow down iic for RPI bug
#define TWI_TX_TIMEOUT			2
#define TWI_RX_TIMEOUT			2
#define TWI_SEND_INTERVAL 		2
#define TWI_SEND_ERROR_INTERVAL	70

 

#define SPI_SEND_INTERVAL 		2L

#define TCP_SEND_INTERVAL 		2
#define TCP_SEND_ERROR_INTERVAL	500
#define TCP_SEND_TIMEOUT 		100

#define BLUE_SEND_INTERVAL 		2L
#define BLE_RESPONSE_ERROR_SLEEP	500


#define NETW_RX_BUF_CNT    	7

#ifndef NETW_TX_BUF_CNT
	#define NETW_TX_BUF_CNT    	7
#endif


#define NETW_MESH_TABLE_SIZE 12
#define NETW_MESH_TABLE_KEEPALIVE 24500L  // milliseconds

#define PAYLOAD_LENGTH 32
#define NETW_MSG_LENGTH 32

#define ERR_NW_TIMEOUT				-40
#define ERR_TX_TIMEOUT				-41
#define ERR_RX_TIMEOUT				-42

#define ERR_TX_FULL					-43
#define ERR_RX_FULL					-44

#define ERR_TX_DELTA_3000			-45
#define ERR_TX_DELTA_LT_0			-46

#define ERR_TX_NODEID				-47
#define ERR_TWI_NOT_READY			-48

#define ERR_TWI_MTX_SLA_NACK		-49
#define ERR_TWI_MTX_DATA_NACK		-50
#define ERR_TWI_MTX_ARB_LOST		-51
#define ERR_TWI_SRX_FULL			-52
#define ERR_TWI_BUS_ERROR			-53

#define ERR_TCP_NOT_READY			-54
#define ERR_TCP_SEND				-55

#define ERR_BLE_CONN				-56
#define ERR_BLE_DISC				-57
#define ERR_BLE_INIT				-58
#define ERR_BLE_QUERY				-59
#define ERR_BLE_SET					-60
#define ERR_BLE_MASTER				-61


	template <class T> int EEPROM_writeAny(int ee, const T& value)
	{
		const byte* p = (const byte*)(const void*)&value;
		unsigned int i;
		for (i = 0; i < sizeof(value); i++)
			  EEPROM.write(ee++, *p++);
		return (i);
	}

	template <class T> int EEPROM_readAny(int ee, T& value)
	{
		byte* p = (byte*)(void*)&value;
		unsigned int i;
		for (i = 0; i < sizeof(value); i++)
			  *p++ = EEPROM.read(ee++);
		return (i);
	}


#if defined ESP8266  
	struct __attribute((__packed__)) RxMsg //   12 lang   cmd 21 = getParm,   cmd 14, 16 =setParm
	{
		byte 		cmd;   			// 'U', 'E', 'S', 'N' bootMessages=0; "B" boot, "F" Flicker
		int16_t 	node;  			// original source node
		byte 		id;    			// sensorId
		int16_t  	conn;  			// via node
		int32_t		val;  			// always long
		uint16_t  	deltaMillis;   	// max 64K * 0.1 seconde = 1,82 uur  of 0.01 seconde  = 10,9 minuten
	};
#elif defined ARDUINO_ARCH_RP2040
	struct __attribute((__packed__)) RxMsg //   12 lang   cmd 21 = getParm,   cmd 14, 16 =setParm
	{
		byte 		cmd;   			// 'U', 'E', 'S', 'N' bootMessages=0; "B" boot, "F" Flicker
		int16_t 	node;  			// original source node
		byte 		id;    			// sensorId
		int16_t  	conn;  			// via node
		int32_t		val;  			// always long
		uint16_t  	deltaMillis;   	// max 64K * 0.1 seconde = 1,82 uur  of 0.01 seconde  = 10,9 minuten
	};
#else
	struct __attribute((__packed__)) RxMsg //   12 lang   cmd 21 = getParm,   cmd 14, 16 =setParm
	{
		byte 			cmd;   			// 'U', 'E', 'S', 'N' bootMessages=0; "B" boot, "F" Flicker
		int16_t 		node;  			// original source node
		byte 			id;    			// sensorId
		int16_t 		conn;  			// via node
		int32_t			val;  			// always long
		uint16_t 	deltaMillis;   	// max 64K * 0.1 seconde = 1,82 uur  of 0.01 seconde  = 10,9 minuten
	};
#endif

union RxData
{
	byte 				raw[sizeof(RxMsg)];
	RxMsg				msg;
};

struct RxItem
{
	RxData			data;
	// unsigned long   timestamp;
	uint32_t		timestamp;
};


#if defined ESP8266  
	struct __attribute((__packed__)) CxMsg //   12 lang   cmd 21 = getParm,   cmd 14, 16 =setParm
	{
		byte 		id;    			// sensorId
		int16_t 	node;  			// original source node
		int32_t		val;  			// always long
	};
#elif defined ARDUINO_ARCH_RP2040
	struct __attribute((__packed__)) CxMsg //   12 lang   cmd 21 = getParm,   cmd 14, 16 =setParm
	{
		byte 		id;    			// sensorId
		int16_t 	node;  			// original source node
		int32_t		val;  			// always long
	};
#else
	struct __attribute((__packed__)) CxMsg //   Clone msg
	{
		byte 			id;    			// sensorId
		int16_t        node;
		int32_t			val;  			// always long
	};
#endif
 
union CxData
{
	byte 			raw[ sizeof(CxMsg)];
	CxMsg			msg;
};


#if defined ESP8266  
	struct __attribute((__packed__)) TxMsg //   13 lang
	{
		byte 			cmd;   			// upl(1), err(3), set(16), refresh(21)
		int16_t		node;  			// origianl source node
		// int 			node;  			// origianl source node
		byte 			id;    			// sensorId
		int32_t		val;  			// always long
		// long			val;  			// always long
	};
#elif defined ARDUINO_ARCH_RP2040
	struct __attribute((__packed__)) TxMsg //   13 lang
	{
		byte 			cmd;   			// upl(1), err(3), set(16), refresh(21)
		int16_t		node;  			// origianl source node
		byte 			id;    			// sensorId
		int32_t		val;  			// always long
	};
#else
	struct __attribute((__packed__)) TxMsg //   13 lang
	{
		byte 			cmd;   			// upl(1), err(3), set(16), refresh(21)
		int16_t		node;  			// origianl source node
		byte 			id;    			// sensorId
		int32_t		val;  			// always long
	};

#endif

union TxData
{
	byte 			raw[ sizeof(TxMsg)];
	TxMsg			msg;
};
struct TxItem
{
	TxData 			data;
	uint32_t   		timestamp;
	uint8_t   		tries;
};


//struct MeshItem
//{
//     int 			node;
//     int 			conn;
//     unsigned long 	lastMillis;
//};

/* Setup before using the Netw* libs
 *
 *	parentNode.onReceive( handleParentReq);
	parentNode.onError(uploadError);
	parentNode.onUpload(upload);
	parentNode.nodeId = nodeId;
	parentNode.isParent = true;
 *
 *  parentNode.isMeshEnabled = true;  for passthrough messages to child nodes. i.e. for TWI nodes
 *
 *
 *
 *
 * */





/*
 *  for mesh networks we use the EEPROM to store the inbetween nodeId.
 *  the EEPROM is used from top to bottom indexed by the nodeId*2 of the receiver
 *  we store the nodeId the connector node to the receiver for every upload message.
 *
 *  i.e.:  send from 1 to 14 via 11
 *         upload from 14 to 1 will store 11 @ offset 1024-(14*2) in the EEPROM of 1.
 *         on 1 a twiSend to 14 will send to 11 because it find 11 in the EEPROM offset 1024-(14*2)
 *
 *  PS  the offset is nodeId * 2 because we store the nodeId as a 2 byte int!!
 *
 * */


class NetwBase
{
public:

	NetwBase( )
	{
		// netwTimer = 100;
//		bootTimer = 100;
		// pingTimer = 100;
		for(int i=0;i<NETW_RX_BUF_CNT;i++) rxFiFo[i].timestamp =0;
		for(int i=0;i<NETW_TX_BUF_CNT;i++) txFiFo[i].timestamp =0;

		initTimers(NW_TIMERS_COUNT);
		nextTimerMillis(NW_TIMER_BUSY, 100);
		nextTimer(NW_TIMER_PING, 7);
		
	}
	virtual ~NetwBase(){}

	volatile unsigned long timers3[NW_TIMERS_COUNT];

    volatile int  lastError=0;
    int lastErrorUploaded = lastError;
    uint8_t lastErrorLogLevel = 1;
    uint8_t txRetryCount = 1;

	int nodeId = 0;
	// int8_t id = 80;
	bool isParent = false;
	int autoPing = 7;

	bool isMeshEnabled = false;
	int  uploadNode=0;
//	bool skipCR = false;
    int ramFree = 2048;

	int nextDebugIdForUpload = 0;
	int nextDebugId2ForUpload = 0;

//    char payloadLower = '<';
    char payLoad[PAYLOAD_LENGTH];
//    char payloadUpper = '>';
    volatile int8_t  payLin = 1;  // one based pointer
    volatile int8_t  payLout = 1;
    volatile bool 	 empty = true;
    volatile int8_t  eolCount = 0;

	bool    txAutoCommit = true;
    TxItem 	txFiFo[NETW_TX_BUF_CNT];  //NETW_RX_BUF_CNT
	bool isTxFull(void)   {return txFiFo[txBufIn].timestamp > 0 ;}
	bool isTxEmpty(void)   {return txFiFo[txBufOut].timestamp == 0;}

	// bool isTxBufFull(void){return txFiFo[txBufIn].timestamp != 0;}


	// special tx buffer for non async uploads. We store data and wait for a request
	bool isUploadOnRequest = false;   // !! not in use anymore
	volatile CxData txBuffer;
	volatile uint8_t txBufferIndex;		// used by TWI
	volatile uint8_t txBufferLength;
    volatile uint8_t txBufIn = 0;
    volatile uint8_t txBufOut = 0;
    // volatile uint8_t txBufCurrOut = 0;
    // volatile uint8_t txBufIndex = 0;

    // volatile unsigned long txTimeOut = 0;

    volatile unsigned int receiveCount=0;
	         unsigned int readProcessedCount=0;
	volatile unsigned int readErrorCount=0;
	volatile unsigned int readOverflowCount=0;
	volatile unsigned int sendCount=0;
	volatile unsigned int sendErrorCount=0;
	volatile unsigned int sendRetryCount=0;
	volatile unsigned int resetTxBufCount=0;
	volatile unsigned int txBufFullCount=0;
 


//	unsigned int sendCountNextUpload=3;
	// unsigned int sendErrorNextUpload=0;
//	unsigned int sendRetryNextUpload=0;

	bool isRxFull(void){return rxFiFo[rxBufIn].timestamp > 0 ;}
	RxItem 	 rxFiFo[NETW_RX_BUF_CNT];  // timestamp = 0 > empty
    volatile uint8_t rxBufIn = 0;
    volatile uint8_t rxBufOut = 0;
    // volatile uint8_t rxBufIndex = 0;

//	unsigned int readCountNextUpload=0;

    char  strTmp[NETW_MSG_LENGTH];

    int getMeshConn(int node);
    void saveMeshConn( RxMsg *rxMsg);

    int (*user_onReceive) (RxItem *rxItem);
    void onReceive( int (*function)(RxItem *rxItem) )
    {
    	user_onReceive = function;
    }

    void (*user_onRollBack)(byte id, long val);
    void onRollBack( void (*)(byte id, long val));

	int (*uploadFunc) (int id, long val, unsigned long timeStamp) = 0;
    void onUpload( int (*function)(int id, long val, unsigned long timeStamp) )
    {
    	uploadFunc = function;
    }

	int (*errorFunc) (int id, long val ) = 0;
    void onError( int (*function)(int id, long val ) )
    {
    	errorFunc = function;
    }

	bool (*checkForReboot)() = 0;
    void onReboot( bool (*function)() )
    {
    	checkForReboot = function;
    }



    int upload(int id);

	virtual void localCmd(int cmd, long val);

//    void clone(int id);

//	int getVal( int id, long * value );
	int setVal( int id, long value );

	void pushChar (char c);
	void pushChars (char str[]);
	void pushChars (char str[], int len);
	bool charRequestAvailable();
	void flushBuf(char *desc);
	int  getCharRequest(RxData *req);
	void resetPayLoad(void);
	void findPayLoadRequest(void);
	void findPayLoadRequest(bool console);
	int  handleEndOfLine(char cmd, int parmCnt );

	char getPayloadChar()
	{
		char x = payLoad[payLout];
		payLoad[payLout++]=0x00;
		payLout = payLout % PAYLOAD_LENGTH;
		return x;
	}
	int getPayloadInt()
	{
		int x = payLoad[payLout];
		payLoad[payLout++]=0x00;
		payLout = payLout % PAYLOAD_LENGTH;
		return x;
	}
	virtual void loop();
			void loopTxReset(void);
			void loopTX(void);
			void loopRX(void);
			void uploadNewErrors(void);
			void loopPing(void);
			void loopSerial();

	virtual void txCommit(void);
	virtual void txCancel(void);
	virtual void rxCommit(void);
			void rxAddCommit(void); //

	// virtual bool isBusy() {return ( millis() <= netwTimer);}  // isBusy( NW_TIMER_BUSY )  diff between isBusy and isReady
	// virtual bool isReady(){return ( millis() >  netwTimer);}  // isReady( NW_TIMER_BUSY )
	virtual bool isBusy() {return isBusy( NW_TIMER_BUSY);}    
	virtual bool isReady(){return isReady( NW_TIMER_BUSY);}   

	void initTimers(int count);
	bool isTime( int id);
	bool isReady( int id);
	bool isBusy( int id);
	bool isTimerActive( int id );
	bool isTimerInactive( int id );
	void nextTimer( int id){ nextTimerMillis(id, 59000L );}	
	void nextTimer( int id, int periode){ nextTimerMillis(id, periode * 1000L );}	
	void nextTimerMillis( int id, unsigned long periode);
	void timerOff( int id );

	// put into sendBuf
	int txUpload(byte id, long val) 							{return putTxBuf('U', nodeId, id, val, millis());}
	int txUpload(byte id, long val, unsigned long timeStamp) 	{return putTxBuf('U', nodeId, id, val, timeStamp);}
	int txError (byte id, long val) 							{return putTxBuf('E', nodeId, id, val, millis());}
	int txCmd   (byte cmd, int nodeId)  						{return putTxBuf(cmd, nodeId, 0,    0, millis());}
	int txCmd   (byte cmd, int nodeId, byte id)					{return putTxBuf(cmd, nodeId, id,   0, millis());}
	int txCmd   (byte cmd, int nodeId, byte id, long val)		{return putTxBuf(cmd, nodeId, id, val, millis());}

	int putTxBuf(byte cmd, int nodeId, byte id, long val, unsigned long timeStamp);
	int putTxBuf(RxItem *rxItem);

	// physical write
	virtual int write( RxData *rxData);
    int writeTxBuf(void);

	void serialize( RxMsg *msg, char *txt );
	void trace(char* id);
	void debug(const char* id, RxItem *rxItem );
	void debug(const char* id, RxMsg *rxMsg );
	void debugTxBuf( char* id   );

	void pgmcpy(char *src,  const char *pgm, int len )
	{
		for(int i = 0; i < len; i++) src[i] = pgm_read_word_near(pgm +i);
	}

private:

};

#endif



//
//struct Err //   10 lang   cmd 21 = getParm,   cmd 14, 16 =setParm
//{
//	byte 	cmd;   // 'E'
//	int 	node;  // server=0
//	byte 	id;    // sensorId
//	int 	conn;  // via node
//	long	val;  // always long
//};
//
//struct Set //   12 lang   cmd 21 = getParm,   cmd 14, 16 =setParm
//{
//	byte 	cmd;   // 'S'
//	int 	node;  // server=0
//	byte 	id;    // sensorId
//	int 	conn;  // via node
//	long	val;  // always long
//};
//struct CmdS //   12 lang   cmd 21 = getParm,   cmd 14, 16 =setParm
//{
//	byte 	cmd;   // 'S'
//	int 	node;  // server=0
//	byte 	id;    // sensorId
//	int 	conn;  // via node
//	long	val;  // always long
//};
//struct Cmd //   12 lang   cmd 21 = getParm,   cmd 14, 16 =setParm
//{
//	byte 	cmd;   // 'C'
//	int 	node;  // server=0
//	byte 	id;    // sensorId
//	int     conn;
//	long    val;
//};
//struct CmdP //   12 lang   cmd 21 = getParm,   cmd 14, 16 =setParm
//{
//	byte 	cmd;
//	char    text[];
//};
//struct Upload //   12 lang   cmd 21 = getParm,   cmd 14, 16 =setParm
//{
//	byte 			cmd;   // upl(1), err(3), set(16), refresh(21)
//	int 			node;  // server=0
//	byte 			id;    // sensorId
//	int 			conn;  // via node
//	long			val;  // always long
//	unsigned int 	deltaMillis;
//};
