#include "NetwBase.h"

// #define DEBUG
 
void NetwBase::loopTxReset(){
	// reset tx buffer
	if( txFiFo[txBufOut].timestamp == 0
	 && txBufOut != txBufIn
	){
		// Serial.print (F("fixTxBuf in="));Serial.print (txBufIn);Serial.print (F(" uit="));Serial.println(txBufOut);Serial.flush();
		Serial.println (F("fixTxFiFo"));
		txBufOut++;
		txBufOut = txBufOut % NETW_TX_BUF_CNT;
		resetTxBufCount++;
	}
}  

void NetwBase::loopTX(){

	if( isReady() )
	{
		// output awaiting to be send
		if( txFiFo[txBufOut].timestamp != 0 )
		{
			if(txFiFo[txBufOut].tries > 0){
			 	// Serial.print(F(" write retry ")); Serial.println(txFiFo[txBufOut].tries );
			}

			int ret = writeTxBuf();
			txFiFo[txBufOut].tries++;

			if(ret>=0) {
				txCommit();

			} else {

				lastError = ret;
				#ifdef DEBUG
					{ Serial.print(F("writeTxBuf ret=")); Serial.println(ret);Serial.flush();  }
				#endif

				if(txFiFo[txBufOut].tries>txRetryCount)
				{
					txCancel();

				} else {

					sendRetryCount++;
				}
			}

		} else  {   // tx empty
			
			if( isTime(NW_TIMER_UPLOAD_DEBUG)  
			){
				nextTimerMillis(NW_TIMER_UPLOAD_DEBUG, 25);

				switch(nextDebugIdForUpload){
					case 0:upload(100); break;
					case 1:upload(101); break;
					case 2:upload(102); break;
					case 3:upload(103); break;
					case 4:upload(104); break;
					case 5:upload(105); break;
					case 6:upload(106); break;
					case 7:upload(107); break;
					case 8:upload(108); break;
					case 9:upload(109); break;
	 
				}
 
				nextDebugIdForUpload++;
 
				if(nextDebugIdForUpload>9)
				{
					timerOff(NW_TIMER_UPLOAD_DEBUG);
					nextDebugIdForUpload = 0; 
				}
			}
		}
	}
}

void NetwBase::loopRX(){

	if( rxFiFo[rxBufOut].timestamp >0
	){
		if( user_onReceive ){

			if( isMeshEnabled )
			{
				saveMeshConn( &rxFiFo[rxBufOut].data.msg );
			}

			// int ret = user_onReceive( &rxFiFo[rxBufOut]  );
			user_onReceive( &rxFiFo[rxBufOut]  );
		}

		rxCommit();
		return;
	}
}

void NetwBase::uploadNewErrors(){
	if( lastError != lastErrorUploaded
	 && errorFunc !=0
	 && !isUploadOnRequest
	 && isTxEmpty()
	){
		if( ( lastErrorLogLevel > 0 && ( lastError == ERR_TWI_MTX_SLA_NACK || lastError == ERR_RX_TIMEOUT ))
		 ||	( lastErrorLogLevel > 1 &&   lastError == ERR_TWI_MTX_ARB_LOST  )
		){
			lastErrorUploaded = lastError;
		}
		else
		{
			if(errorFunc(isParent?80:70, lastError) >= 0 )
			{
				lastErrorUploaded = lastError;
			}
		}
	}
}

void NetwBase::loop()  // TODO
{
	loopTxReset();
  
	loopTX();  // send when idle
 
	loopRX();  // process received data

	uploadNewErrors();

	if(autoPing > 0) loopPing();
}

void NetwBase::loopPing()
{
	if( isReady()
	 && isReady(NW_TIMER_PING)
	 && nodeId > 0
	 && uploadFunc != 0
	 && ! isUploadOnRequest
	 && autoPing > 0
	){
		// Serial.println("loopPing"   );
		// upload(5);
		uploadFunc(5, 0, 0);
		nextTimer(NW_TIMER_PING, autoPing);
	}
}

void NetwBase::loopSerial()  // TODO  server or client ???? call only when in parent mode
{
	if(Serial.available() )
	{
		while( Serial.available() )
		{
			pushChar((char)Serial.read());
		}
		findPayLoadRequest( true);
		NetwBase::loop();
	}
}

void NetwBase::localCmd(int cmd, long val)
{
	switch ( cmd)
	{
	case 1:
		nextDebugIdForUpload = 0;
		nextTimer(NW_TIMER_UPLOAD_DEBUG, 0);
        break;

	default:

		Serial.print("localCmd:");
		Serial.print(cmd);	
		Serial.print(", val:");	
		Serial.println(val);	
		break;
	}
}


int NetwBase::upload(int id)
{
	if(uploadFunc==0 ) return 0;

	int ret=0;
	// Serial.print("NetwBase::upload "); 	Serial.println(id ); 
	switch( id )
	{

	// case 80: ret=uploadFunc(id, lastErrorLogLevel, millis() ); break;
	case 5: 
		uploadFunc(5, 0, 0);
		nextTimer(NW_TIMER_PING, autoPing);		
		break;
	case 100: ret=uploadFunc(id, sendCount, millis() ); break;
	case 101: ret=uploadFunc(id, lastError, millis	() ); break;
	case 102: ret=uploadFunc(id, sendRetryCount, millis() ); break;
	case 103: ret=uploadFunc(id, sendErrorCount, millis() ); break;
	case 104: ret=uploadFunc(id, resetTxBufCount, millis() ); break;
	case 105: ret=uploadFunc(id, receiveCount, millis	() ); break;
	case 106: ret=uploadFunc(id, readProcessedCount, millis() ); break;
	case 107: ret=uploadFunc(id, readErrorCount, millis() ); break;
	case 108: ret=uploadFunc(id, readOverflowCount, millis() ); break;
	case 109: ret=uploadFunc(id, txBufFullCount, millis() ); break;
 
	

	default: return ret;
	}
	return ret;
}

int NetwBase::setVal( int id, long value ) //bool sending, int id,
{
	switch(id)
	{
	case 80: lastErrorLogLevel = value;  break;  // network sleep
	case 89: 
		// netwTimer = millis() + (value * 1000L); 
		nextTimer(NW_TIMER_BUSY, value)	;
	 	break;  // network sleep

 	default: return -1; break;
	}
	return 1;
}

int NetwBase::getMeshConn(int node)
{
	int conn;
	int offSet = 1024-node*2;
	EEPROM_readAny(offSet, conn);
	if(conn<1)conn=node;
	#ifdef DEBUG
	 Serial.print(F("getMesh(")); Serial.print(node);Serial.print(F(")>"));Serial.println(conn);
	#endif

	return conn;
}

void NetwBase::saveMeshConn(RxMsg *rxMsg)
{
	#ifdef DEBUG
		 Serial.print(F("saveMesh("));   Serial.print((char)rxMsg->cmd);Serial.print(F(","));Serial.print(rxMsg->node);Serial.print(F(","));Serial.print(rxMsg->conn);Serial.println(F(")"));
	#endif

	if( 1==2
//	 || rxMsg->node==rxMsg->conn
	 || rxMsg->node==2
	 || (rxMsg->cmd != 'U' && rxMsg->cmd != 'u')
//	 || meshTablePtr == 0
	 || rxMsg->node==0
	){
		return;
	}

	#ifdef DEBUG
		 Serial.print(F("saveMesh("));   Serial.print((char)rxMsg->cmd);Serial.print(F(","));Serial.print(rxMsg->node);Serial.print(F(","));Serial.print(rxMsg->conn);Serial.println(F(")"));
	#endif

	int tempInt;
	int offSet = 1024-rxMsg->conn*2;
	EEPROM_readAny(offSet, tempInt);
	if( rxMsg->conn != tempInt )EEPROM_writeAny(offSet, rxMsg->conn);
}

int NetwBase::write( RxData *rxData )
{
	Serial.println(F("?NetwBase::write.RxData?"));
	return -1;
}

void NetwBase::onRollBack( void (*function)(byte id, long val) )
{
	user_onRollBack = function;
}
void NetwBase::rxAddCommit()
{
	receiveCount++;
	uploadNode = rxFiFo[rxBufIn].data.msg.conn;		// save nodeId on incomming to be used for outgoing
	rxFiFo[rxBufIn++].timestamp = millis();
	rxBufIn = rxBufIn % NETW_RX_BUF_CNT;
}
void NetwBase::rxCommit()
{
	// Serial.println("rxCommit");
	readProcessedCount++;
	rxFiFo[rxBufOut++].timestamp = 0;
	rxBufOut=rxBufOut%NETW_RX_BUF_CNT;
}

void NetwBase::txCancel()
{
	if(txFiFo[txBufOut].timestamp != 0)
	{
		sendCount++;
		sendErrorCount++;
		txFiFo[txBufOut].timestamp = 0;
		//Serial.println(F("txCan"));
		txBufOut++;
		txBufOut=txBufOut%NETW_TX_BUF_CNT;
	}
}

void NetwBase::txCommit()
{
	// Serial.println("txCommit");
	timerOff(NW_TIMEOUT_OVERALL);
	timerOff(NW_TIMEOUT_TX);

	if(txFiFo[txBufOut].timestamp != 0)
	{
		sendCount++;
		txFiFo[txBufOut].timestamp = 0;
		//Serial.print(F("txComm @"));Serial.println(millis()); 
		txBufOut++;
		txBufOut=txBufOut%NETW_TX_BUF_CNT;
		if(nodeId==txFiFo[txBufOut].data.msg.node)
		{
			nextTimer(NW_TIMER_PING, autoPing);
		}
	}
}

/*
	pass through upload rxMsg to a send txMsg for sending to the next parent:
	- use deltaMillis to calc the millis from node startup. (the send logic can than calc a new deltaMillis at the moment of forwarding to parent) 
	- the connId is not needed because it will be replaced by this nodeId.

    new name: rx2TxFifo
*/
int NetwBase::putTxBuf(RxItem *rxItem)  // false when full
{
	unsigned long timeStamp = millis() - (rxItem->data.msg.deltaMillis * 100);
	return putTxBuf( rxItem->data.msg.cmd, rxItem->data.msg.node, rxItem->data.msg.id, rxItem->data.msg.val, timeStamp );
}


int NetwBase::putTxBuf(byte cmd, int node, byte id, long val, unsigned long timeStamp)  // false when full
{
	#ifdef DEBUG
		Serial.print(F("putTx*["));Serial.print(txBufIn);Serial.print(F("<"));
		Serial.print(id);Serial.print(F(","));Serial.print(val);
		Serial.print(F(" @"));Serial.println(millis());
	#endif

	// if( isUploadOnRequest)
	// {
	// 	txBuffer.msg.id = id;
	// 	txBuffer.msg.node = node;
	// 	txBuffer.msg.val = val;

	// 	txBufferLength = sizeof(RxData);
	// 	return 1;
	// }


	if(txFiFo[txBufIn].timestamp!=0)
	{
		//Serial.println(F("txBuf full")); Serial.flush();
		lastError =	ERR_TX_FULL;
		txBufFullCount++;
		return ERR_TX_FULL;
	}

	if(timeStamp==0) timeStamp=millis();

	txFiFo[txBufIn].data.msg.cmd = cmd;
	txFiFo[txBufIn].data.msg.node = node;
	txFiFo[txBufIn].data.msg.id = id;
	txFiFo[txBufIn].data.msg.val = val;
	txFiFo[txBufIn].timestamp = timeStamp;
	txFiFo[txBufIn++].tries = 0;
	txBufIn = txBufIn % NETW_TX_BUF_CNT;

	return 1;
}


// writeFromTxFiFo
int NetwBase::writeTxBuf() // opt: 0=all, 1=val, 2=cmd
{
	RxData rxData;

	// if( nodeId<1 || txFiFo[txBufOut].data.msg.id == 0 )
	if( nodeId<1 )
	{
		#ifdef DEBUG
			Serial.println("twSnd: nodeId<1");
		#endif
		lastError = ERR_TX_NODEID;
		return ERR_TX_NODEID;
	}

    int delta = ( millis() - txFiFo[txBufOut].timestamp  ) / 100L;
 	// int delta = millis() < txFiFo[txBufOut].timestamp ?  (txFiFo[txBufOut].timestamp - millis())/100L : (millis()-txFiFo[txBufOut].timestamp)/100L;

	if(delta > 3000)
	{
		lastError = ERR_TX_DELTA_3000;
		txCancel();
		return ERR_TX_DELTA_3000;  // skip > 300 seconds (5 minutes) in the past
	}
	if(delta < 0)
	{
		lastError = ERR_TX_DELTA_LT_0	;
		txCancel();
		return ERR_TX_DELTA_LT_0	;  // skip  in the furture
	}

	rxData.msg.cmd 	= txFiFo[txBufOut].data.msg.cmd;
	rxData.msg.node	= txFiFo[txBufOut].data.msg.node;
	rxData.msg.id	= txFiFo[txBufOut].data.msg.id;
	rxData.msg.conn = nodeId;  	// set current node as connector

	rxData.msg.val 	= txFiFo[txBufOut].data.msg.val;
	rxData.msg.deltaMillis = delta;

	//Serial.print(F("tx["));Serial.print(txBufOut);Serial.print(F(">"));Serial.print(rxData.msg.id);

	return write(&rxData);
}



void NetwBase::serialize( RxMsg *msg, char *txt )
{
	if(msg->deltaMillis > 0)
		sprintf(txt, "{%c,%u,%u,%u,%ld,%u}", msg->cmd, msg->node, msg->id, msg->conn, msg->val, msg->deltaMillis  );
	else if(msg->val != 0)
		sprintf(txt,    "{%c,%u,%u,%u,%ld}", msg->cmd, msg->node, msg->id, msg->conn, msg->val  );
	else
		sprintf(txt,       "{%c,%u,%u,%u}", msg->cmd, msg->node, msg->id, msg->conn  );
}




void NetwBase::debug(const char* id, RxItem *rxItem  )
{
	serialize( &rxItem->data.msg, strTmp);

	Serial.print(id);  Serial.println(strTmp);Serial.flush();
}
void NetwBase::debug(const char* id, RxMsg *rxMsg  )
{
	serialize( rxMsg, strTmp);

	Serial.print(id);  Serial.println(strTmp);Serial.flush();
}

void NetwBase::debugTxBuf( char* id   )
{
	//if(txFiFo[txBufOut].timestamp==0) return;

	sprintf(strTmp, "[%u] {%c,%u,%u,%ld}%lu", txBufOut, txFiFo[txBufOut].data.msg.cmd, txFiFo[txBufOut].data.msg.node, txFiFo[txBufOut].data.msg.id, txFiFo[txBufOut].data.msg.val, txFiFo[txBufOut].timestamp   );

	Serial.print(id);  Serial.println(strTmp);    Serial.flush();
}

void NetwBase::pushChar(char c)
{
	if(c!='\r' && ( c < 0x20 || c > 0x7E )) return;
	if(c=='\r') c='}';
	//if(c==']') c='}';

	// if overflow override oldest request
	if(!empty && payLin == payLout && eolCount<1 ) empty=true;  // just skip all
 	if(!empty && payLin == payLout )
	{
		#ifdef DEBUG
		Serial.println(F("override"));
		#endif
		for(int j=0;j<PAYLOAD_LENGTH;j++)
		//for(;payLout!=payLin;payLout++)
		{
			char cc = payLoad[payLout];

			if( (cc == '{' ) && j>0 )  //||cc == '['
			{
				break; // leave the {
			}

			payLout++;
			payLout=payLout % PAYLOAD_LENGTH;

			if(   cc == '}'  && j>0 )
			{
				eolCount--;
				break;  // } removed
			}
		}
	}

	if(c=='}' )eolCount++;

	if( c=='{'
	 && eolCount==0
	 && payLin!=payLout)
	{
		payLin=payLout;   // trunc before {
	}


	empty = false;
	payLoad[payLin++] = c;
	payLin = payLin % PAYLOAD_LENGTH;
}
void NetwBase::pushChars (char str[])
{
	pushChars( str, PAYLOAD_LENGTH );
}
void NetwBase::pushChars (char str[], int len)
{
	int strPtr=0;
	char c=str[strPtr++];
	while( c != 0x00 && strPtr <  len)
	{
		pushChar(c);
		c=str[strPtr++];
	}
	return;
}

bool NetwBase::charRequestAvailable()
{
	return eolCount > 0;
}


void  NetwBase::resetPayLoad(void)
{
	empty=true;
	eolCount=0;
	payLout=0;
	payLin=0;
	Serial.println(F("pyldRst")); Serial.flush();

}

void  NetwBase::findPayLoadRequest(void)
{
	findPayLoadRequest(false);
}
void  NetwBase::findPayLoadRequest(bool forConsole)
{
 	if( empty
 	 || eolCount < 1
 	 || rxFiFo[rxBufIn].timestamp >0  // full
	){
 		return;
 	}

	char * endPointer;
    char parm[16];
    int  parmPtr = 0;
    int  parmCnt = 0;
    bool truncated = false;
    bool old = false;

    for(unsigned int i=0;i<sizeof(RxMsg);i++) rxFiFo[rxBufIn].data.raw[i]=0x00;
    rxFiFo[rxBufIn].data.msg.node = 2; //local cmd

	while(payLout!=payLin)
	{
		char c = payLoad[payLout++];
		payLout = payLout % PAYLOAD_LENGTH;
		// string to stuct
		if(c == '{' )
		{
			parmCnt=0;
			parmPtr=0;
		}

		else if(c == ';' || c == ',' || c == ':' || c == ' ' || c == '}'   ) //|| payLout==payLin ??
		{
			//Serial.println("nextParm");
			parmCnt++;
			parm[parmPtr]=0x00;
			if(parmPtr>=sizeof(parm))
			{
				truncated = true;
				parmPtr=0;
				//Serial.println(F("truncated"));
			}
			//Serial.print(F("parmCnt++="));Serial.println(parmCnt);

			long xx;

			switch(parmCnt)
			{
		 	case 1:
		 		rxFiFo[rxBufIn].data.msg.cmd =  parm[0];
		 		old = ( rxFiFo[rxBufIn].data.msg.cmd == 's'
		 			 || rxFiFo[rxBufIn].data.msg.cmd == 'r'
		 			 || rxFiFo[rxBufIn].data.msg.cmd == 'u'
		 			 || rxFiFo[rxBufIn].data.msg.cmd == 'e' );
		 		break;
		 	case 2:
		 		if(rxFiFo[rxBufIn].data.msg.cmd == 's')
		 		{
		 			rxFiFo[rxBufIn].data.msg.id = strtol( &parm[0], &endPointer, 10);
		 		}
		 		else if(!old)
				{
					rxFiFo[rxBufIn].data.msg.node = strtol( &parm[0], &endPointer, 10);
				}
				else
				{
					xx = strtol( &parm[0], &endPointer, 10);
					rxFiFo[rxBufIn].data.msg.node = xx/100;
					rxFiFo[rxBufIn].data.msg.id = xx%100;
				}
		 		break;
		 	case 3:
		 		if(rxFiFo[rxBufIn].data.msg.cmd == 's')
		 		{
		 			rxFiFo[rxBufIn].data.msg.val = strtol( &parm[0], &endPointer, 10);
		 		}
		 		else if(!old)
					rxFiFo[rxBufIn].data.msg.id = strtol( &parm[0], &endPointer, 10);
		 		else
		 			rxFiFo[rxBufIn].data.msg.conn = strtol( &parm[0], &endPointer, 10);
		 		break;
		 	case 4:
		 		if(!old)
					rxFiFo[rxBufIn].data.msg.conn = strtol( &parm[0], &endPointer, 10);
		 		else
		 			rxFiFo[rxBufIn].data.msg.val = strtol( &parm[0], &endPointer, 10);
		 		break;
		 	case 5:
		 		if(!old)
					rxFiFo[rxBufIn].data.msg.val = strtol( &parm[0], &endPointer, 10);
		 		else
		 		{
					xx = strtol( &parm[0], &endPointer, 10);
					rxFiFo[rxBufIn].data.msg.deltaMillis = xx;  //strtol( &parm[0], &endPointer, 10);
		 		}
		 		break;
		 	case 6:
		 		if(!old)
		 		{
					xx = strtol( &parm[0], &endPointer, 10);
					rxFiFo[rxBufIn].data.msg.deltaMillis = xx;  //strtol( &parm[0], &endPointer, 10);
		 		}
		 		break;
			default:
				break;
			}


			parmPtr=0;
		}
		else
		{
			//Serial.print("+");Serial.print(c);
			parm[parmPtr++]=c;
		}

		// handle end of line
		if( c=='}'  || payLout==payLin )
		{
			//Serial.print(F("msgEnd"));Serial.println(eolCount);
			if(eolCount>0)eolCount--;

			if(payLout==payLin)
			{
				empty = true;
			}

			if(truncated) return;

			int ret = handleEndOfLine(rxFiFo[rxBufIn].data.msg.cmd, parmCnt );

//			if(ret>0 || isConsole)
			if(ret>=0 || forConsole)
//			if(ret>=0   )
			{
				#ifdef DEBUG
//						Serial.print(F("rx["));	Serial.print(rxBufIn);
//						debug("<", &rxFiFo[rxBufIn] );
//						Serial.flush();
				#endif
			 
				rxAddCommit();
			}
			return;
		}
		// loop next char in buffer
	}
}


int  NetwBase::getCharRequest(RxData *rxData)
{
	//if( eolCount < 0) eolCount=0;
 	if( empty || eolCount < 1  )  // ??? remove || sleepTimer > millis() ????
 	{
 	//	eolCount=0;
 	//	payLout=payLin;
 		return 0;
 	}

	char * endPointer;
    char parm[16];
    int  parmPtr = 0;
    int  parmCnt = 0;
    bool truncated = false;
//    bool nieuw = false;
    bool old = false;

	rxData->msg.cmd = 0x00; //16;
	rxData->msg.node = 2;	// local
	rxData->msg.id = 0;
	rxData->msg.conn = 0;
	rxData->msg.val = 0;
	rxData->msg.deltaMillis = 0L;

	while(payLout!=payLin)
	{
		char c = payLoad[payLout++];
		payLout = payLout % PAYLOAD_LENGTH;
		// string to stuct
		if(c == '{' )
		{
			parmCnt=0;
			parmPtr=0;
//			payloadType = 0;
			//Serial.println(F("new{"));
		}

		else if(c == ';' || c == ',' || c == ':' || c == ' ' || c == '}'   ) //|| payLout==payLin ??
		{
			//Serial.println("nextParm");
			parmCnt++;
			parm[parmPtr]=0x00;
			if(parmPtr>=sizeof(parm))
			{
				truncated = true;
				parmPtr=0;
				//Serial.println(F("truncated"));
			}
			Serial.print(F("parmCnt++="));Serial.println(parmCnt);

			long xx;

			switch(parmCnt)
			{
		 	case 1:
		 		rxData->msg.cmd =  parm[0];
		 		old = (rxData->msg.cmd == 's' || rxData->msg.cmd == 'r' || rxData->msg.cmd == 'u' || rxData->msg.cmd == 'e' );
		 		break;
		 	case 2:
		 		if(rxData->msg.cmd == 'x')
		 		{
		 			rxData->msg.conn = 98;
		 			rxData->msg.id = strtol( &parm[0], &endPointer, 10);
		 		}
		 		else if(!old)
				{
					rxData->msg.node = strtol( &parm[0], &endPointer, 10);
				}
				else
				{
					xx = strtol( &parm[0], &endPointer, 10);
					rxData->msg.node = xx/100;
					rxData->msg.id = xx % 100;
				}
		 		break;
		 	case 3:
		 		if(rxData->msg.cmd == 'x')
		 		{
		 			rxData->msg.conn = 99;
		 			rxData->msg.val = strtol( &parm[0], &endPointer, 10);
//		 			parmCnt = 4;
		 		}
		 		else if(!old)
					rxData->msg.id = strtol( &parm[0], &endPointer, 10);
		 		else
					rxData->msg.conn = strtol( &parm[0], &endPointer, 10);
		 		break;
		 	case 4:
		 		if(!old)
					rxData->msg.conn = strtol( &parm[0], &endPointer, 10);
		 		else
					rxData->msg.val = strtol( &parm[0], &endPointer, 10);
		 		break;
		 	case 5:
		 		if(!old)
					rxData->msg.val = strtol( &parm[0], &endPointer, 10);
		 		else
		 		{
					xx = strtol( &parm[0], &endPointer, 10);
					rxData->msg.deltaMillis = xx;  //strtol( &parm[0], &endPointer, 10);
		 		}
		 		break;
		 	case 6:
		 		if(!old)
		 		{
					xx = strtol( &parm[0], &endPointer, 10);
					rxData->msg.deltaMillis = xx;  //strtol( &parm[0], &endPointer, 10);
		 		}
		 		break;
			default:
				break;
			}


			parmPtr=0;
		}
		else
		{
			//Serial.print("+");Serial.print(c);
			parm[parmPtr++]=c;
		}

		// handle end of line
		if( c=='}'  || payLout==payLin )
		{
			//Serial.print(F("msgEnd"));Serial.println(eolCount);
			if(eolCount>0)eolCount--;
//			if(eolCount<1)
//			{
//				//empty = true;
//				//payLout=payLin;
//				//eolCount=0;
//			}

			if(payLout==payLin)
			{
				empty = true;
				//eolCount=0;
			}

			if(truncated) return -31;
			int ret = handleEndOfLine(rxData->msg.cmd, parmCnt );
		//	if(ret==1 &&  c ==']' ) ret++;

//			Serial.print("<");Serial.print(rxData->msg.id);Serial.print(",");Serial.println(rxData->msg.val);

			if(ret>0) receiveCount++;
			return ret;
//			memcpy(req, &newReqBuf, sizeof(newReqBuf));
//			if(ret==1)
//			return ret;
		}
		// loop next char in buffer
	}
	return 0; // no valid command found
}

int NetwBase::handleEndOfLine(char cmd, int parmCnt )
{
	//if(rxData->msg.conn==0 || rxData->msg.id==0 || rxData->msg.node==0) return -33;
	switch( cmd )
	{
 	case 0x00: // no command found
 		return -32;
 		break;
 	case '}': // empty command found
 	case ']': // empty command found
 		return 0;
 		break;
 	case 'S':
 	case 's':
 		return (parmCnt<4) ? -2:1;
 		break;
 	case 'R':
 	case 'r':
 		return (parmCnt<2) ? -3:1;
 		break;
 	//case 'U':
 	case 'u':
 		return (parmCnt<4) ? -4:1;
 		break;
 	//case 'E':
 	case 'e':
 		return (parmCnt<4) ? -5:1;
 		break;
	}
	return 1;
}

void  NetwBase::flushBuf(char *desc)
{
 	if( empty || eolCount < 1)
 	{
 		return;
 	}

 	Serial.print(desc); Serial.print(F("<"));

	while(payLout!=payLin)
	{
		char c = payLoad[payLout++];
		payLout = payLout % PAYLOAD_LENGTH;
		Serial.print(c);
		if(c == '}' ||c ==']')
		{
			eolCount--;
			break;
		}
	}
	if(payLout==payLin) empty=true;
	Serial.println();
}

void NetwBase::initTimers(int count)
{
	for(int i=0; i<count; i++){
		timers3[i]=0L;
	}

	// nextTimerMillis(DS18B20_TIMER_REQUEST, 500);
}

bool NetwBase::isTime( int id){
	if(timers3[id] == 0L) return false;

	unsigned long delta = millis() > timers3[id] ? millis() - timers3[id] : timers3[id] - millis() ;
	return delta > 0x0fffffff ? false : millis() >= timers3[id];

}

bool NetwBase::isBusy( int id){
	if(timers3[id] == 0L) return false;

	unsigned long delta = millis() > timers3[id] ? millis() - timers3[id] : timers3[id] - millis() ;
	return delta > 0x0fffffff ? true : millis() < timers3[id]; 
}

bool NetwBase::isReady( int id){
	if(timers3[id] == 0L) return true;

	unsigned long delta = millis() > timers3[id] ? millis() - timers3[id] : timers3[id] - millis() ;
	return delta > 0x0fffffff ? false : millis() >= timers3[id]; 
}

bool NetwBase::isTimerActive( int id ){
	return timers3[id] > 0;
}
bool NetwBase::isTimerInactive( int id ){
	return timers3[id] == 0;
}
void NetwBase::timerOff( int id ){
	timers3[id]=0;
}

void NetwBase::nextTimerMillis( int id, unsigned long periode){

	if(periode<0) periode=0;
	timers3[id] = millis() + periode;
	if(timers3[id]==0) timers3[id]=1;
}

