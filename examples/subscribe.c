/****************************************************************************/
/*                                                                          */
/* Program name: MQIsdp protocol sample application                         */
/*                                                                          */
/* Description: The application accepts a URL on the command line which     */
/*              contains all the parameters required for subscribing for    */
/*              data over the WMQTT protocol.                               */
/*                                                                          */
/*  Statement:  Licensed Materials - Property of IBM                        */
/*                                                                          */
/*              WebSphere MQ SupportPac IA93                                */
/*              (C) Copyright IBM Corp. 2003                                */
/*                                                                          */
/****************************************************************************/
/* Version @(#) IA93/ship/subscribe.c, SupportPacs, S000 1.2 03/12/05 14:13:46  */
/*                                                                          */
/* Function:                                                                */
/*                                                                          */
/* This application accepts a URL beginning with wmqtt://, parses it and    */
/* subscribes to the topic specified waiting for data.                      */
/* Optionally match data can be specified. The subscribe then returns on    */
/* the first message received that matches the match data.                  */
/* If * is specified as the match data then subscribe returns on the first  */
/* message received.                                                        */
/* Running 'subscribe -h' or viewing the usage() function will give more    */
/* help.                                                                    */
/*                                                                          */
/****************************************************************************/
/*                                                                          */
/* Change history:                                                          */
/*                                                                          */
/* V1.0   30-11-2003  IRH  Initial release                                  */
/*==========================================================================*/
/* Module Name: subscribe.c                                                 */

/* Declare an sccsid variable                                               */
static char *sccsid = "@(#) IA93/ship/subscribe.c, SupportPacs, S000 1.2 03/12/05 14:13:46";

#include <stdio.h>
#include <string.h>
#ifdef WIN32
  #include <time.h>
  #include <windows.h>
#else  
  #include <stdlib.h>
  #include <sys/time.h>
  #include <unistd.h>
  #include <stdarg.h>
  #include <sys/types.h>
  #include <sys/ipc.h>
  #include <sys/sem.h>
  #include <pthread.h>
#endif
#include <MQIsdp.h>  /* WMQTT include            */
#include <mspfp.h>   /* File persistence include */

#define CaseRet( x ) \
  case x:            \
    ptr = #x;        \
    break;

typedef struct _MQISDPCMD {
    char  clientId[MQISDP_CLIENT_ID_LENGTH + 1];
    char *pBroker;
    int   port;
    char *topic;
    int   qos;
    int   timeout;
    char *lwtTopic;
    int   lwtQos;
    int   lwtRetain;
    char *lwtData;
    int   debug;
    int   dataArg;
    MQISDPCH hConn;
    MQISDPMH lastSentMsg;
} SUBPARMS;

#define TOKEN_EOL         0 /* Token runs to the end of the command line  */
#define TOKEN_TERMINATED  1 /* Token is terminated by a special character */

#define QOSOPT        "qos="
#define QOSOPTLEN     4
#define TIMEOUTOPT    "timeout="
#define TIMEOUTOPTLEN 8
#define RETAINOPT     "retain="
#define RETAINOPTLEN  7
#define DATAOPT       "data="
#define DATAOPTLEN    5
#define DEBUGOPT      "debug="
#define DEBUGOPTLEN   6

#define DATA_MATCHED  0
#define TIMED_OUT     1

static void usage( void );
static int processCommandLine( int, char**, SUBPARMS* );
static char* getNextToken( char*, char*, int*, int* );
static int demoGetRCtoString( long rc, char *pBuffer, long bufSz );
static void demoSleep( int mSecs );
static int demoMQIsdpConnect( SUBPARMS *ppp, MQISDPTI *pApiTaskInfo );
static int demoMQIsdpDisconnect( SUBPARMS *ppp );
static int demoMQIsdpSubscribe( SUBPARMS *ppp );
static int demoMQIsdpUnsubscribe( SUBPARMS *ppp );
static int demoMQIsdpRcvPub( SUBPARMS *ppp, char *pMatchData );

int main( int argc, char **argv ) {
    SUBPARMS   subParms;
    MQISDPTI  *pSendTaskParms;
    MQISDPTI  *pRcvTaskParms;
    MQISDPTI  *pApiTaskParms;
    char      *pBuffer = NULL;
    int        bufLen = 100;
    int        msgLen = 0;
    int        rc = 1;
    int        statusCode = 0;
    char       rcBuf[MQISDP_RC_STRING_LENGTH];

    subParms.port = 1883;
    subParms.qos = 0;
    subParms.timeout = -1;
    subParms.debug = 1;
    subParms.lwtTopic = NULL;
    subParms.lwtQos = 0;
    subParms.lwtRetain = 0;
    subParms.lwtData = NULL;
    subParms.dataArg = 0;

    /* Process the command line to build a subscribe command */
    if ( processCommandLine( argc, argv, &subParms ) != 0 ) {
        usage();
        return 1;
    }

    /* Check for match data being supplied - dataArg */
    if ( (subParms.dataArg != 0) && (subParms.dataArg < argc) ) {
        msgLen = strlen( argv[subParms.dataArg] );
        pBuffer = (char*)malloc( msgLen + 1 );
        strcpy( pBuffer, argv[subParms.dataArg] );
    } else {
        subParms.dataArg = 0;
    }

    if ( subParms.debug ) {
        printf( "ClientID :%s\n",  subParms.clientId );
        printf( "Broker   :%s\n",  subParms.pBroker );
        printf( "Port     :%ld\n", subParms.port );
        printf( "Topic    :%s\n",  subParms.topic );
        printf( "QoS      :%ld\n", subParms.qos );
        printf( "Timeout  :%ld\n", subParms.timeout );
        printf( "LWTTopic :%s\n",  subParms.lwtTopic );
        printf( "LWTQoS   :%ld\n", subParms.lwtQos );
        printf( "LWTRetain:%s\n", (subParms.lwtRetain)?"true":"false" );
        printf( "MatchData:%s\n",  pBuffer );
    }
    
    /* Allocate the WMQTT thread parameter structures */
    pSendTaskParms = (MQISDPTI*)malloc( sizeof(MQISDPTI) );
    pRcvTaskParms = (MQISDPTI*)malloc( sizeof(MQISDPTI) );
    pApiTaskParms = (MQISDPTI*)malloc( sizeof(MQISDPTI) );

    /* Turn thread tracing off */
    pSendTaskParms->logLevel = LOGNONE;
    pRcvTaskParms->logLevel = LOGNONE;
    pApiTaskParms->logLevel = LOGNONE;

    /* Start the WMQTT threads */
    if ( MQIsdp_StartTasks( pApiTaskParms, pSendTaskParms,
                            pRcvTaskParms, subParms.clientId ) != 0 ) {
        printf( "Failed to start MQIsdp protocol threads\n" );
        return 2;
    }
    
    /* Connect to the WMQTT broker */
    if ( demoMQIsdpConnect( &subParms, pApiTaskParms ) != 0 ) {
        return 2;
    }

    /* Subscribe to the specified topic */
    demoMQIsdpSubscribe( &subParms );

    /* Block until either matching data is received, or the subscribe times out */
    rc = demoMQIsdpRcvPub( &subParms, pBuffer );
    switch ( rc ) {
    case MQISDP_OK:
        rc = 0; /* Data matched */
        break;
    case MQISDP_NO_PUBS_AVAILABLE:
        rc = 1; /* Timed out */
        break;
    default:
        rc = 2; /* Some other problem */
        break;
    }

    /* Unsubscribe */
    if ( demoMQIsdpUnsubscribe( &subParms ) != MQISDP_OK ) {
        rc = 2;
    }
    
    /* Wait until the unsubscribe is complete */
    statusCode = MQIsdp_getMsgStatus( subParms.hConn, subParms.lastSentMsg );
    while ( statusCode != MQISDP_DELIVERED ) {
        printf( "Waiting for message handle %ld to be delivered....\n", subParms.lastSentMsg );
        demoSleep( 5000 );
        statusCode = MQIsdp_getMsgStatus( subParms.hConn, subParms.lastSentMsg );
    }
    demoGetRCtoString( statusCode, rcBuf, MQISDP_RC_STRING_LENGTH );
    printf( "MQIsdp_getMsgStatus hMsg:%ld, status:%s\n", subParms.lastSentMsg, rcBuf );
    
    /* Disconnect and terminate */
    demoMQIsdpDisconnect( &subParms );

    MQIsdp_terminate();

    if ( subParms.debug ) {
        printf( "Return: %ld\n", rc );
    }

    free( subParms.pBroker );

    return rc;
}

static int processCommandLine( int myArgc, char** myArgv, SUBPARMS *pSub ) {
    char *startTagPos = NULL;
    char *token = NULL;
    int   cmdlineLen = 0;
    int   exitStatus = 0;
    char *cmdline = NULL;
    int   syntaxError = 0;
    int   lwtDataFound = 0;

    /* We expect a command line of format: */
    /* wmqtt://user@broker:port/topic?qos=[0|1|2]&timeout=secs */

    if ( myArgc < 2 ) {
        printf("Too few parameters!\n" );
        return 1;
    }

    cmdline = myArgv[1];
    if ( cmdline == NULL ) {
        printf( "NULL command line\n" );
        return 1;
    }

    startTagPos = cmdline;
    cmdlineLen = strlen(cmdline);

    /***** Firstly check for the existence of the wmqtt piece */
    token = getNextToken( startTagPos, "://", &cmdlineLen, &exitStatus );
    if ( strncmp( token, "wmqtt", 5 ) != 0 ) {
        printf( "Syntax error: Unrecognised protocol:%.5s, expected wmqtt\n", cmdline );
        return 1;
    } else if ( exitStatus == TOKEN_EOL ) {
        printf( "Syntax error: No '://' separator following protocol\n" );
        return 1;
    }
    startTagPos += strlen(token) + 3;
    free(token);

    /***** Next locate the @ symbol which marks the end of the client id */
    token = getNextToken( startTagPos, "@", &cmdlineLen, &exitStatus );
    if ( exitStatus == TOKEN_EOL ) {
        printf( "Syntax error:No @ symbol following clientId\n" );
        return 1;
    }
    if ( strlen(token) > MQISDP_CLIENT_ID_LENGTH ) {
        printf( "Syntax error:ClientId is too long ( >%ld chars)\n", MQISDP_CLIENT_ID_LENGTH );
        return 1;
    }
    strcpy( pSub->clientId, token );
    
    startTagPos += strlen(token) + 1;
    free(token);


    /***** Next locate : which marks the end of the broker ip address */
    token = getNextToken( startTagPos, ":", &cmdlineLen, &exitStatus );
    if ( exitStatus == TOKEN_EOL ) {
        printf( "Syntax error:No : symbol following broker ip address\n" );
        return 1;
    }
    pSub->pBroker = (char*)malloc( strlen(token) + 1 );
    strcpy( pSub->pBroker, token );
    
    startTagPos += strlen(token) + 1;
    free(token);
    
    /***** Next locate / which marks the end of the port number */
    token = getNextToken( startTagPos, "/", &cmdlineLen, &exitStatus );
    if ( exitStatus == TOKEN_EOL ) {
        printf( "Syntax error:No / symbol after port number\n" );
        return 1;
    }
    pSub->port = atoi(token);
    if ( pSub->port == 0 ) {
        printf( "Syntax error:Invalid port number (%s)\n", token );
        return 1;
    }
    
    startTagPos += strlen(token) + 1;
    free(token);
    
    /***** Next locate the ? symbol which marks the end of the topic */
    token = getNextToken( startTagPos, "?", &cmdlineLen, &exitStatus );
    if ( token == NULL ) {
        printf( "Syntax error:No topic specified\n" );
        return 1;
    }
    
    pSub->topic = (char*)malloc( strlen(token)+ 1 );
    strcpy(pSub->topic, token );
    
    startTagPos += strlen(token) + 1;
    free(token);

    /***** Check for any optional parameters */
    if ( exitStatus != TOKEN_EOL ) {
       while ( exitStatus != TOKEN_EOL ) {
           token = getNextToken( startTagPos, "&", &cmdlineLen, &exitStatus );
           if ( token == NULL ) {
               break;
           }

           if ( strncmp( QOSOPT, token, QOSOPTLEN ) == 0 ) {
               switch ( *(token+QOSOPTLEN) ) {
               case '0':
                   pSub->qos = 0;
                   break;
               case '1':
                   pSub->qos = 1;
                   break;
               case '2':
                   pSub->qos = 2;
                   break;
               default:
                   syntaxError = 1;
                   printf( "Syntax error: Unrecognised QoS:%.1s\n", token+QOSOPTLEN );
                   break;
               }
           } else if ( strncmp( TIMEOUTOPT, token, TIMEOUTOPTLEN ) == 0 ) {
               pSub->timeout = atol( token+TIMEOUTOPTLEN );
               pSub->timeout *= 1000; /* Convert seconds to milliseconds */
           } else if ( strncmp( DEBUGOPT, token, DEBUGOPTLEN ) == 0 ) {
               switch ( *(token+DEBUGOPTLEN) ) {
               case '0':
                   pSub->debug = 0;
                   break;
               case '1':
                   pSub->debug = 1;
                   break;
               default:
                   syntaxError = 1;
                   printf( "Syntax error: Unrecognised debug flag:%.1s\n", token+DEBUGOPTLEN );
                   break;
               }
           } else {
               syntaxError = 1;
               printf( "Syntax error: Unrecognised optional flag:%s\n", token );
           }


           startTagPos += strlen(token) + 1;
           free(token);
       }
    }
    
    if ( syntaxError == 1 ) {
        return 1;
    }
       
    if ( myArgc > 2 ) {
        /* Check for the presence of a LWT message */
        startTagPos = myArgv[2];
        cmdlineLen = strlen(myArgv[2]);

        /***** Firstly check for the existence of the LWT piece */
        lwtDataFound = 0;
        token = getNextToken( startTagPos, "=", &cmdlineLen, &exitStatus );
        if ( strncmp( token, "LWT", 3 ) != 0 ) {
            /* Data to publish is on the command line */
            pSub->dataArg = 2;
        } else if ( exitStatus == TOKEN_EOL ) {
            printf( "Syntax error: No '=' separator following LWT\n" );
            return 1;
        } else {
            /* LWT data found */
            lwtDataFound = 1;
            pSub->dataArg = 3;
        }
        startTagPos += strlen(token) + 1;
        free(token);
        
        if ( lwtDataFound == 1 ) {
            /***** Next locate the ? symbol which marks the end of the topic */
            token = getNextToken( startTagPos, "?", &cmdlineLen, &exitStatus );
            if ( token == NULL ) {
                printf( "Syntax error:No topic specified\n" );
                return 1;
            }

            pSub->lwtTopic = (char*)malloc( strlen(token)+ 1 );
            strcpy(pSub->lwtTopic, token );

            startTagPos += strlen(token) + 1;
            free(token);

            /***** Check for any optional parameters */
            if ( exitStatus != TOKEN_EOL ) {
               while ( exitStatus != TOKEN_EOL ) {
                   token = getNextToken( startTagPos, "&", &cmdlineLen, &exitStatus );
                   if ( token == NULL ) {
                       break;
                   }

                   if ( strncmp( QOSOPT, token, QOSOPTLEN ) == 0 ) {
                       switch ( *(token+QOSOPTLEN) ) {
                       case '0':
                           pSub->lwtQos = 0;
                           break;
                       case '1':
                           pSub->lwtQos = 1;
                           break;
                       case '2':
                           pSub->lwtQos = 2;
                           break;
                       default:
                           syntaxError = 1;
                           printf( "Syntax error: Unrecognised LWT QoS:%s\n", token+QOSOPTLEN );
                           break;
                       }
                   } else if ( strncmp( RETAINOPT, token, RETAINOPTLEN ) == 0 ) {
                       switch ( *(token+RETAINOPTLEN) ) {
                       case 'y':
                       case 'Y':
                           pSub->lwtRetain = 1;
                           break;
                       case 'n':
                       case 'N':
                           pSub->lwtRetain = 0;
                           break;
                       default:
                           syntaxError = 1;
                           printf( "Syntax error: Unrecognised LWT retain flag:%s\n", token+RETAINOPTLEN );
                           break;
                       }
                   } else if ( strncmp( DATAOPT, token, DATAOPTLEN ) == 0 ) {
                       
                       pSub->lwtData = (char*)malloc( strlen(token)-DATAOPTLEN+ 1 );
                       strcpy(pSub->lwtData, token+DATAOPTLEN );
                       
                   } else {
                       syntaxError = 1;
                       printf( "Syntax error: Unrecognised optional LWT flag:%s\n", token );
                   }


                   startTagPos += strlen(token) + 1;
                   free(token);
               }
            }

            if ( syntaxError == 1 ) {
                return 1;
            }


        }
    }

    return 0;
}

static void usage( void ) {
    printf( "WebSphere MQ Telemetry transport subscriber usage:\n" );
    printf( "\n" );
    printf( "subscribe wmqtt://clientId@broker:port/topic?qos=[0|1|2]&timeout=secs&debug=[0|1]\n" );
    printf( "<LWT=topic?qos=[0|1|2]&retain=[y|n]&data=lwtdata> <data>\n" );
    printf( "\n" );
    printf( "    wmqtt://  - protocol identifier\n" );
    printf( "    clientId  - WMQTT client identifier\n" );
    printf( "    broker    - IP address or hostname of WMQI broker\n" );
    printf( "    port      - Port number of WMQI SCADAInput node\n" );
    printf( "    topic     - Topic to publish data on\n" );
    printf( "    qos       - Optional field. Publication Quality of Service\n" );
    printf( "    timeout   - Optional field. Time to wait (secs) for matching data. Default - forever\n" );
    printf( "    debug     - Optional field. Print debug messages?\n" );
    printf( "\n" );
    printf( "  LWT (Last Will and Testament): Optional parameter\n" );
    printf( "    topic     - Topic to publish LWT data on\n" );
    printf( "    qos       - Optional field. LWT Quality of Service\n" );
    printf( "    retain    - Optional field. LWT retain flag\n" );
    printf( "    data      - LWT data to be published\n" );
    printf( "\n" );
    printf( "    data      - If no data is supplied then every publication received is\n" );
    printf( "                printed out until timeout expires. If data is supplied then\n" );
    printf( "                the program returns when data matches a received publication\n" );
    printf( "                or the timeout expires.\n" );
    printf( "                If data is '*' then the first publicationreceived is matched.\n" );
    printf( "\n" );
    printf( "Returns 0 - Data matched ok\n" );
    printf( "        1 - Timed out\n" );
    printf( "        2 - Connection failed\n" );
    printf( "\n" );
    printf( "Example: subscribe wmqtt://myId@localhost:1883/my/topic/space?qos=1&timeout=60 *\n" );
    printf( "    This would subscribe to topic 'my/topic/space' and return on the first\n" );
    printf( "    publication received.\n" );
}

static char* getNextToken( char* startTagPos, char* endMarker, int *remLength, int* status ) {
    char *endTagPos = NULL;
    int   endMarkerLen = 0;
    char *returnStr = NULL;

    if ( *remLength <= 0 ) {
        *status = TOKEN_EOL;
        return NULL;
    }

    *status = TOKEN_TERMINATED;
    endMarkerLen = strlen(endMarker);

    endTagPos = strstr( startTagPos, endMarker );
    if ( endTagPos == NULL ) {
        *status = TOKEN_EOL;
        /* No end marker found */
        endMarkerLen = 0;
        /* Read to the end of the command line */
        endTagPos = startTagPos + *remLength;
    }

    returnStr = (char*)malloc(endTagPos - startTagPos + 1);
    strncpy(returnStr, startTagPos, (endTagPos - startTagPos) );
    returnStr[endTagPos - startTagPos] = '\0';
    
    /* Step over the endMarker symbol */
    endTagPos += endMarkerLen;

    /* Reduce the command line length by the clientId length plus 1 for the @ */
    *remLength -= (strlen(returnStr) + endMarkerLen);

    return returnStr;
}

static int demoMQIsdpConnect( SUBPARMS *ppp, MQISDPTI *pApiTaskInfo ) {
    CONN_PARMS  *pCp;
    int         rc = MQISDP_OK;
    long        connState = MQISDP_CONNECTING;
    char        rcBuf[MQISDP_RC_STRING_LENGTH];
    char        infoString[MQISDP_INFO_STRING_LENGTH];
    long        infoCode;
    int         willMsg = 0;
    long        connMsgLength;
    char       *pTmpPtr;
    long        tmpLong;

    ppp->hConn = MQISDP_INV_CONN_HANDLE;
    
    if ( (ppp->lwtData != NULL) && (ppp->lwtTopic != NULL) ) {
        willMsg = 1;
    }
    /* ####### CONNECT ####### */
    /* Sizeof connect structure:            */
    /* sizeof CONN_PARMS                    */
    /* No additional MQIsdp brokers         */
    /* No Will message                      */
    connMsgLength = sizeof(CONN_PARMS);
    if ( willMsg ) {
        connMsgLength += 8 + strlen(ppp->lwtData) + strlen(ppp->lwtTopic);
    }

    pCp = (CONN_PARMS*)malloc( connMsgLength );

    pCp->strucLength = connMsgLength;
    
    /* Add a client identifier - already been validated for length */
    strcpy( pCp->clientId, ppp->clientId );

    pCp->retryCount = 10;
    pCp->retryInterval = 10;
    pCp->options = MQISDP_NONE | MQISDP_CLEAN_START;
    if ( willMsg ) {
        /* We have a will message add will qos and retain flags */
        pCp->options |= MQISDP_WILL;
        
        /* will qos */
        switch( ppp->lwtQos ) {
        case 0:
            pCp->options |= MQISDP_QOS_0;
            break;
        case 1:
            pCp->options |= MQISDP_QOS_1;
            break;
        case 2:
            pCp->options |= MQISDP_QOS_2;
            break;
        }

        if ( ppp->lwtRetain ) {
            pCp->options |= MQISDP_WILL_RETAIN;
        }

    }
    pCp->keepAliveTime = (short)10;
    
    /* Persistence interface */
    #ifdef WIN32 
        pCp->pPersistFuncs = getPersistenceInterface( "C:\\temp\\wmqtt" );
    #else 
        pCp->pPersistFuncs = getPersistenceInterface( "/tmp/wmqtt" );
    #endif

    /* Broker port number */
    pCp->brokerPort = ppp->port;
    
    /* Broker Hostname */
    pCp->brokerHostname = ppp->pBroker;

    if ( willMsg ) {
        /* Will topic length */
        pTmpPtr = (char*)pCp + sizeof(CONN_PARMS);
        tmpLong = strlen( ppp->lwtTopic );
        memcpy( pTmpPtr, &tmpLong, sizeof(long) );

        /* will toipic */
        pTmpPtr += sizeof(long);
        memcpy( pTmpPtr, ppp->lwtTopic, tmpLong );

        /* will data length */
        pTmpPtr += tmpLong;
        tmpLong = strlen( ppp->lwtData );
        memcpy( pTmpPtr, &tmpLong, sizeof(long) );

        /* will data */
        pTmpPtr += sizeof(long);
        memcpy( pTmpPtr, ppp->lwtData, tmpLong );
    }

    rc = MQIsdp_connect( &(ppp->hConn), pCp, pApiTaskInfo );
    if ( ppp->debug ) {
        demoGetRCtoString( rc, rcBuf, MQISDP_RC_STRING_LENGTH );
        printf( "MQIsdp_connect rc:%s\n", rcBuf );
    }
    free( pCp );
    
    while ( connState == MQISDP_CONNECTING ) {
        connState = MQIsdp_status(ppp->hConn, MQISDP_INFO_STRING_LENGTH,
                                  &infoCode, infoString );
        if ( ppp->debug ) {
            demoGetRCtoString( connState, rcBuf, MQISDP_RC_STRING_LENGTH );
            printf( "MQIsdp_status state:%s - %s\n", rcBuf, infoString ); 
        }
        demoSleep(1000);
    }

    /* If we haven't successfully connected to the MQIsdp broker then disconnect */
    if ( connState != MQISDP_CONNECTED ) {
        demoMQIsdpDisconnect( ppp );
        rc = 1;
    }

    return rc;
}

static int demoMQIsdpRcvPub( SUBPARMS *pSp, char *pMatchData ) {
    long  bufferSz = 1024;
    char *pBuffer = NULL;
    long  dataLength = 0;
    long  topicLength = 0;
    long  pubOptions = 0;
    long  startTime;
    long  curTime = 0;
    long  timeRemaining = 0;
    long  timeToWait = 10000;
    long  timeWaited = 0;
    int   rc;
    long  status;
    char  rcBuf[MQISDP_RC_STRING_LENGTH];

    if ( pSp->timeout > 0 ) {
        timeRemaining = pSp->timeout;
    } else {
        /* If no timeout is specified then wait forever in 10 second chunks, */
        /* as the API can't do a blocking receive forever.                   */
        timeRemaining = 10000;
    }


    pBuffer = (char*)malloc( bufferSz );

    /* ####### RECEIVE PUBLICATIONS */
    /* Loop while there are publications available */
    rc = MQISDP_PUBS_AVAILABLE;
    while ( timeRemaining > 0 ) {
        
        startTime = time( NULL );
        if ( timeToWait > timeRemaining ) {
            timeToWait = timeRemaining;
        }
        rc = MQIsdp_receivePub( pSp->hConn, timeToWait, &pubOptions, &topicLength, &dataLength, bufferSz, pBuffer );

        demoGetRCtoString( rc, rcBuf, MQISDP_RC_STRING_LENGTH );
        if ( pSp->debug ) {
            printf( "MQIsdp_receivePub rc:%s\n", rcBuf );
        }
        
        switch ( rc ) {
        case MQISDP_DATA_TRUNCATED:
            if ( pBuffer == NULL ) {
                bufferSz = dataLength;
                pBuffer = (char*)malloc( bufferSz + 1 );
            } else {
                bufferSz = dataLength;
                pBuffer = (char*)realloc( pBuffer, bufferSz + 1 );
            }
            break;
        case MQISDP_PUBS_AVAILABLE:
        case MQISDP_OK:
            /* NULL terminate the data */
            pBuffer[dataLength] = '\0';

            if ( pSp->debug ) {
                printf( "    Topic:%.*s\n",topicLength, pBuffer );
                printf( "    Data :%s\n", pBuffer + topicLength );
            }
            
            /* Check for matching data and set the return rc correctly */
            if ( (pMatchData != NULL) && ( (strcmp("*", pMatchData) == 0) || (strcmp(pBuffer+topicLength, pMatchData) == 0)) ) {
                rc = MQISDP_OK;
                timeRemaining = -1; /* Exit the loop */
            } else {
                rc = MQISDP_NO_PUBS_AVAILABLE;
            }

            break;
        default:
            break;
        }

        /* If we are not waiting forever then reduce the timeRemaining */
        /* Deduct the time waited so far..                             */
        if ( pSp->timeout > 0 ) {
            timeWaited = (time(NULL) - startTime) * 1000;  /* milliseconds */
            timeRemaining -= timeWaited;
        }

        /* Check the state of the connection */
        if ( MQIsdp_status( pSp->hConn, MQISDP_RC_STRING_LENGTH, &status, rcBuf ) == MQISDP_CONNECTION_BROKEN ) {
            printf( "MQIsdp_status rc:MQISDP_CONNECTION_BROKEN - %s\n", rcBuf );
            rc = MQISDP_CONNECTION_BROKEN;
            break;
        }
    }
    
    free( pBuffer );

    return rc;
}

static int demoGetRCtoString( long rc, char *pBuffer, long bufSz ) {
    
    char *ptr = NULL;

    switch ( rc ) {
       /* Return codes */
       CaseRet( MQISDP_OK )
       CaseRet( MQISDP_Q_FULL )
       CaseRet( MQISDP_FAILED )
       CaseRet( MQISDP_PUBS_AVAILABLE )
       CaseRet( MQISDP_NO_PUBS_AVAILABLE )
       CaseRet( MQISDP_CONN_HANDLE_ERROR )
       CaseRet( MQISDP_MSG_HANDLE_ERROR )
       CaseRet( MQISDP_NO_WILL_TOPIC )
       CaseRet( MQISDP_INVALID_STRUC_LENGTH )
       CaseRet( MQISDP_DATA_LENGTH_ERROR )
       CaseRet( MQISDP_DATA_TOO_BIG )
       CaseRet( MQISDP_ALREADY_CONNECTED )
       CaseRet( MQISDP_CONNECTION_BROKEN )
       CaseRet( MQISDP_DATA_TRUNCATED )
       CaseRet( MQISDP_OUT_OF_MEMORY )
       CaseRet( MQISDP_PERSISTENCE_FAILED );
       CaseRet( MQISDP_HOSTNAME_NOT_FOUND );
        
       /* Message states */
       CaseRet( MQISDP_DELIVERED )
       CaseRet( MQISDP_RETRYING )
       CaseRet( MQISDP_IN_PROGRESS )
       CaseRet( MQISDP_CONNECTING )
       CaseRet( MQISDP_CONNECTED )
       CaseRet( MQISDP_DISCONNECTED )
        
       /* Info codes */
       CaseRet( MQISDP_PROTOCOL_VERSION_ERROR )
       CaseRet( MQISDP_CLIENT_ID_ERROR )
       CaseRet( MQISDP_BROKER_UNAVAILABLE )
       CaseRet( MQISDP_SOCKET_CLOSED )
        default:
            break;
    }
    
    if ( pBuffer != NULL ) {
        if ( ptr != NULL ) {
            strncpy( pBuffer, ptr, bufSz );
        } else {
            sprintf( pBuffer, "%ld", rc );
        }
    }
    
    return rc;
}

static void demoSleep( int mSecs ) {
    #ifdef WIN32
      Sleep( mSecs );
    #endif
    
    #ifdef UNIX
      struct timeval tv;
      tv.tv_sec = (int)(mSecs / 1000);
      tv.tv_usec = (mSecs % 1000) * 1000;
      select( 0, NULL, NULL, NULL, &tv );
    #endif
}

static int demoMQIsdpDisconnect( SUBPARMS *ppp ) {
    char rcBuf[MQISDP_RC_STRING_LENGTH];
    int  rc;

    rc = MQIsdp_disconnect( &(ppp->hConn) );

    if ( ppp->debug ) {
        demoGetRCtoString( rc, rcBuf, MQISDP_RC_STRING_LENGTH );
        printf( "MQIsdp_disconnect rc:%s\n", rcBuf );
    }

    return rc;
}

static int demoMQIsdpSubscribe( SUBPARMS *ppp ) {
    MQISDPMH   hMsg;
    SUB_PARMS *pSp;
    char      *pTmpPtr;
    long       options = 0;
    long       tLength = 0;
    int        rc = MQISDP_OK;
    char       rcBuf[MQISDP_RC_STRING_LENGTH];
    int        bufSize = 0;

    bufSize = sizeof(SUB_PARMS) + (2 * sizeof(long)) + strlen(ppp->topic);
    pSp = (SUB_PARMS*)malloc( bufSize );

    if ( pSp != NULL ) {
        pSp->strucLength = bufSize;

        /* Set the topic length field */
        pTmpPtr = (char*)pSp + sizeof(long);
        tLength = strlen(ppp->topic);

        memcpy( pTmpPtr, &tLength, sizeof(long) );

        /* Set the topic field */
        pTmpPtr += sizeof(long);
        memcpy( pTmpPtr, ppp->topic, strlen(ppp->topic) );

        /* Set the options field */
        pTmpPtr += strlen(ppp->topic);
        switch ( ppp->qos ) {
        case 0:
            options |= MQISDP_QOS_0;
            break;
        case 1:
            options |= MQISDP_QOS_1;
            break;
        case 2:
            options |= MQISDP_QOS_2;
            break;
        }
        memcpy( pTmpPtr, &options, sizeof(long) );
    

        /* Subscribe */
        rc = MQIsdp_subscribe( ppp->hConn, &hMsg, pSp );

        if ( (ppp->debug) || (rc != MQISDP_OK) ) {
            demoGetRCtoString( rc, rcBuf, MQISDP_RC_STRING_LENGTH );
            printf( "MQIsdp_subscribe rc:%s - %s\n", rcBuf, ppp->topic );
        }
        
        ppp->lastSentMsg = hMsg;

        free( pSp );
    }
    
    return rc;
}

static int demoMQIsdpUnsubscribe( SUBPARMS *ppp ) {
    MQISDPMH   hMsg;
    UNSUB_PARMS *pUp;
    char      *pTmpPtr;
    long       tLength = 0;
    int        rc = MQISDP_OK;
    char       rcBuf[MQISDP_RC_STRING_LENGTH];
    int        bufSize = 0;

    bufSize = sizeof(UNSUB_PARMS) + sizeof(long) + strlen(ppp->topic);
    pUp = (UNSUB_PARMS*)malloc( bufSize );

    if ( pUp != NULL ) {
        pUp->strucLength = bufSize;

        /* Set the topic length field */
        pTmpPtr = (char*)pUp + sizeof(long);
        tLength = strlen(ppp->topic);
        memcpy( pTmpPtr, &tLength, sizeof(long) );

        /* Set the topic field */
        pTmpPtr += sizeof(long);
        memcpy( pTmpPtr, ppp->topic, strlen(ppp->topic) );
    
        /* Unsubscribe */
        rc = MQIsdp_unsubscribe( ppp->hConn, &hMsg, pUp );

        if ( (ppp->debug) || (rc != MQISDP_OK) ) {
            demoGetRCtoString( rc, rcBuf, MQISDP_RC_STRING_LENGTH );
            printf( "MQIsdp_unsubscribe rc:%s - %s\n", rcBuf, ppp->topic );
        }
        
        ppp->lastSentMsg = hMsg;
        
        free( pUp );
    }
    
    return rc;
}

