/****************************************************************************/
/*                                                                          */
/* Program name: MQIsdp protocol sample application                         */
/*                                                                          */
/* Description: The application accepts a URL on the command line which     */
/*              contains all the parameters required for publishing a       */
/*              message over the WMQTT protocol.                            */
/*                                                                          */
/*  Statement:  Licensed Materials - Property of IBM                        */
/*                                                                          */
/*              WebSphere MQ SupportPac IA93                                */
/*              (C) Copyright IBM Corp. 2003                                */
/*                                                                          */
/****************************************************************************/
/* Version @(#) IA93/ship/publish.c, SupportPacs, S000 1.3 03/12/05 14:13:37  */
/*                                                                          */
/* Function:                                                                */
/*                                                                          */
/* This application accepts a URL beginning with wmqtt://, parses it and    */
/* publishes the data to the topic specified.                               */
/* Running 'publish -h' or viewing the usage() function will give more help */
/*                                                                          */
/****************************************************************************/
/*                                                                          */
/* Change history:                                                          */
/*                                                                          */
/* V1.0   30-11-2003  IRH  Initial release                                  */
/*==========================================================================*/
/* Module Name: publish.c                                                   */

/* Declare an sccsid variable                                               */
static char *sccsid = "@(#) IA93/ship/publish.c, SupportPacs, S000 1.3 03/12/05 14:13:37";

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
    char  *pBroker;
    int   port;
    char *topic;
    int   qos;
    int   retain;
    char *lwtTopic;
    int   lwtQos;
    int   lwtRetain;
    char *lwtData;
    int   debug;
    int   dataArg;
    MQISDPCH hConn;
    MQISDPMH lastSentMsg;
} PUBPARMS;

#define TOKEN_EOL         0 /* Token runs to the end of the command line  */
#define TOKEN_TERMINATED  1 /* Token is terminated by a special character */

#define QOSOPT       "qos="
#define QOSOPTLEN    4
#define RETAINOPT    "retain="
#define RETAINOPTLEN 7
#define DATAOPT      "data="
#define DATAOPTLEN   5
#define DEBUGOPT     "debug="
#define DEBUGOPTLEN  6

static void usage( void );
static int processCommandLine( int, char**, PUBPARMS* );
static char* getNextToken( char*, char*, int*, int* );
static int demoGetRCtoString( long rc, char *pBuffer, long bufSz );
static void demoSleep( int mSecs );
static int demoMQIsdpConnect( PUBPARMS *ppp, MQISDPTI *pApiTaskInfo );
static int demoMQIsdpDisconnect( PUBPARMS *ppp );
static int demoMQIsdpPublish( PUBPARMS *ppp, char *pData, int dataLength );

int main( int argc, char **argv ) {
    PUBPARMS   pubParms;
    MQISDPTI  *pSendTaskParms;
    MQISDPTI  *pRcvTaskParms;
    MQISDPTI  *pApiTaskParms;
    char      *pBuffer = NULL;
    int        bufLen = 100;
    int        msgLen = 0;
    int        statusCode = 0;
    char       rcBuf[MQISDP_RC_STRING_LENGTH];

    pubParms.port = 1883;
    pubParms.qos = 0;
    pubParms.retain = 0;
    pubParms.debug = 1;
    pubParms.lwtTopic = NULL;
    pubParms.lwtQos = 0;
    pubParms.lwtRetain = 0;
    pubParms.lwtData = NULL;
    pubParms.dataArg = 0;
    pubParms.lastSentMsg = MQISDP_INV_MSG_HANDLE;

    /* Process the command line to build a publish command */
    if ( processCommandLine( argc, argv, &pubParms ) != 0 ) {
        usage();
        return 1;
    }

    /* If data was supplied ont he commandline read it in */
    /* Otherwise enter interactive mode.                  */
    if ( (pubParms.dataArg != 0) && (pubParms.dataArg < argc) ) {
        msgLen = strlen( argv[pubParms.dataArg] );
        pBuffer = (char*)malloc( msgLen + 1 );
        strcpy( pBuffer, argv[pubParms.dataArg] );
    } else {
        pubParms.dataArg = 0;
    }

    if ( pubParms.debug ) {
        printf( "ClientID :%s\n",  pubParms.clientId );
        printf( "Broker   :%s\n",  pubParms.pBroker );
        printf( "Port     :%ld\n", pubParms.port );
        printf( "Topic    :%s\n",  pubParms.topic );
        printf( "QoS      :%ld\n", pubParms.qos );
        printf( "Retain   :%s\n", (pubParms.retain)?"true":"false" );
        printf( "LWTTopic :%s\n",  pubParms.lwtTopic );
        printf( "LWTQoS   :%ld\n", pubParms.lwtQos );
        printf( "LWTRetain:%s\n", (pubParms.lwtRetain)?"true":"false" );
    }
    
    /* Malloc and initialise the WMQTT thread parameters */
    pSendTaskParms = (MQISDPTI*)malloc( sizeof(MQISDPTI) );
    pRcvTaskParms = (MQISDPTI*)malloc( sizeof(MQISDPTI) );
    pApiTaskParms = (MQISDPTI*)malloc( sizeof(MQISDPTI) );
    
    /* Set thread debug level to none for all threads */
    pSendTaskParms->logLevel = LOGNONE;
    pRcvTaskParms->logLevel = LOGNONE;
    pApiTaskParms->logLevel = LOGNONE;
    
    /* Start the WMQTT threads */
    if ( MQIsdp_StartTasks( pApiTaskParms, pSendTaskParms,
                            pRcvTaskParms, pubParms.clientId ) != 0 ) {
        printf( "Failed to start MQIsdp potocol threads\n" );
        return 1;
    }
    
    /* Connect to the MQIsdp broker */
    if ( demoMQIsdpConnect( &pubParms, pApiTaskParms ) != 0 ) {
        return 1;
    }    

    /* Now read data in from stdin */
    /* Terminates when the first empty line is read */
    if ( pubParms.dataArg == 0  ) {
        printf( "Enter data:\n" );
        pBuffer = (char*)malloc( bufLen );
        
        while( 1 ) {
            if ( fgets(pBuffer, bufLen, stdin) != NULL ) {
                /* Remove trailing \n */
                msgLen = strlen(pBuffer) - 1;
                pBuffer[msgLen] = '\0';
                
                if ( msgLen <=0 ) {
                    break;
                }
                demoMQIsdpPublish( &pubParms, pBuffer, msgLen );
            } else {
                /* Break on EOF */
                break;
            }
        }
    } else {
        demoMQIsdpPublish( &pubParms, pBuffer, msgLen );
    }

    /* Wait until the publish has been delivered */
    if ( pubParms.lastSentMsg != MQISDP_INV_MSG_HANDLE) {
        statusCode = MQIsdp_getMsgStatus( pubParms.hConn, pubParms.lastSentMsg );
        while ( statusCode != MQISDP_DELIVERED ) {
            printf( "Waiting for message handle %ld to be delivered....\n", pubParms.lastSentMsg );
            demoSleep( 5000 );
            statusCode = MQIsdp_getMsgStatus( pubParms.hConn, pubParms.lastSentMsg );
        }
        demoGetRCtoString( statusCode, rcBuf, MQISDP_RC_STRING_LENGTH );
        printf( "MQIsdp_getMsgStatus hMsg:%ld, status:%s\n", pubParms.lastSentMsg, rcBuf );
    }
    
    /* Disconnect the protocol and terminate */
    demoMQIsdpDisconnect( &pubParms );

    MQIsdp_terminate();

    free( pubParms.pBroker );

    return 0;
}

static int processCommandLine( int myArgc, char** myArgv, PUBPARMS *pPub ) {
    char *startTagPos = NULL;
    char *token = NULL;
    int   cmdlineLen = 0;
    int   exitStatus = 0;
    char *cmdline = NULL;
    int   syntaxError = 0;
    int   lwtDataFound = 0;

    /* We expect a command line of format: */
    /* wmqtt://user@broker:port/topic?qos=[0|1|2]&retain=[y|n] */

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
    strcpy( pPub->clientId, token );
    
    startTagPos += strlen(token) + 1;
    free(token);


    /***** Next locate : which marks the end of the broker ip address */
    token = getNextToken( startTagPos, ":", &cmdlineLen, &exitStatus );
    if ( exitStatus == TOKEN_EOL ) {
        printf( "Syntax error:No : symbol following broker ip address\n" );
        return 1;
    }
    pPub->pBroker = (char*)malloc( strlen(token) + 1 );
    strcpy( pPub->pBroker, token );
    
    startTagPos += strlen(token) + 1;
    free(token);
    
    /***** Next locate / which marks the end of the port number */
    token = getNextToken( startTagPos, "/", &cmdlineLen, &exitStatus );
    if ( exitStatus == TOKEN_EOL ) {
        printf( "Syntax error:No / symbol after port number\n" );
        return 1;
    }
    pPub->port = atoi(token);
    if ( pPub->port == 0 ) {
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
    
    pPub->topic = (char*)malloc( strlen(token)+ 1 );
    strcpy(pPub->topic, token );
    
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
                   pPub->qos = 0;
                   break;
               case '1':
                   pPub->qos = 1;
                   break;
               case '2':
                   pPub->qos = 2;
                   break;
               default:
                   syntaxError = 1;
                   printf( "Syntax error: Unrecognised QoS:%s\n", token+QOSOPTLEN );
                   break;
               }
           } else if ( strncmp( RETAINOPT, token, RETAINOPTLEN ) == 0 ) {
               switch ( *(token+RETAINOPTLEN) ) {
               case 'y':
               case 'Y':
                   pPub->retain = 1;
                   break;
               case 'n':
               case 'N':
                   pPub->retain = 0;
                   break;
               default:
                   syntaxError = 1;
                   printf( "Syntax error: Unrecognised retain flag:%s\n", token+RETAINOPTLEN );
                   break;
               }
           } else if ( strncmp( DEBUGOPT, token, DEBUGOPTLEN ) == 0 ) {
               switch ( *(token+DEBUGOPTLEN) ) {
               case '0':
                   pPub->debug = 0;
                   break;
               case '1':
                   pPub->debug = 1;
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
            pPub->dataArg = 2;
        } else if ( exitStatus == TOKEN_EOL ) {
            printf( "Syntax error: No '=' separator following LWT\n" );
            return 1;
        } else {
            /* LWT data found */
            lwtDataFound = 1;
            pPub->dataArg = 3;
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

            pPub->lwtTopic = (char*)malloc( strlen(token)+ 1 );
            strcpy(pPub->lwtTopic, token );

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
                           pPub->lwtQos = 0;
                           break;
                       case '1':
                           pPub->lwtQos = 1;
                           break;
                       case '2':
                           pPub->lwtQos = 2;
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
                           pPub->lwtRetain = 1;
                           break;
                       case 'n':
                       case 'N':
                           pPub->lwtRetain = 0;
                           break;
                       default:
                           syntaxError = 1;
                           printf( "Syntax error: Unrecognised LWT retain flag:%s\n", token+RETAINOPTLEN );
                           break;
                       }
                   } else if ( strncmp( DATAOPT, token, DATAOPTLEN ) == 0 ) {
                       
                       pPub->lwtData = (char*)malloc( strlen(token)-DATAOPTLEN+ 1 );
                       strcpy(pPub->lwtData, token+DATAOPTLEN );
                       
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
    printf( "WebSphere MQ Telemetry transport publisher usage:\n" );
    printf( "\n" );
    printf( "publish wmqtt://clientId@broker:port/topic?qos=[0|1|2]&retain=[y|n]&debug=[0|1] " );
    printf( "<LWT=topic?qos=[0|1|2]&retain=[y|n]&data=lwtdata> <data>\n" );
    printf( "\n" );
    printf( "    wmqtt://  - protocol identifier\n" );
    printf( "    clientId  - WMQTT client identifier\n" );
    printf( "    broker    - IP address or hostname of WMQI broker\n" );
    printf( "    port      - Port number of WMQI SCADAInput node\n" );
    printf( "    topic     - Topic to publish data on\n" );
    printf( "    qos       - Optional field. Publication Quality of Service\n" );
    printf( "    retain    - Optional field. Publication retain flag\n" );
    printf( "    debug     - Optional field. Print debug messages?\n" );
    printf( "\n" );
    printf( "  LWT (Last Will and Testament): Optional parameter\n" );
    printf( "    topic     - Topic to publish LWT data on\n" );
    printf( "    qos       - Optional field. LWT Quality of Service\n" );
    printf( "    retain    - Optional field. LWT retain flag\n" );
    printf( "    data      - LWT data to be published\n" );
    printf( "\n" );    
    printf( "    data      - If no data is supplied then interactive mode is entered.\n" );
    printf( "                Each line of data entered is published. Pressing return\n" );
    printf( "                with no data will end interactive mode." );
    printf( "\n" );
    printf( "Example: publish wmqtt://myId@localhost:1883/my/topic/space?qos=1&retain=n \"My Message\"\n" );
    printf( "    This would publish data 'My Message' to topic 'my/topic/space'.\n" );
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

static int demoMQIsdpConnect( PUBPARMS *ppp, MQISDPTI *pApiTaskInfo ) {
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
        connMsgLength += (2 * sizeof(long)) + strlen(ppp->lwtData) + strlen(ppp->lwtTopic);
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
        /*pCp->pPersistFuncs = getPersistenceInterface( "/tmp/wmqtt" );*/
        pCp->pPersistFuncs = NULL;
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

        /* will topic */
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

static int demoMQIsdpDisconnect( PUBPARMS *ppp ) {
    char rcBuf[MQISDP_RC_STRING_LENGTH];
    int  rc;

    rc = MQIsdp_disconnect( &(ppp->hConn) );

    if ( ppp->debug ) {
        demoGetRCtoString( rc, rcBuf, MQISDP_RC_STRING_LENGTH );
        printf( "MQIsdp_disconnect rc:%s\n", rcBuf );
    }

    return rc;
}

static int demoMQIsdpPublish( PUBPARMS *ppp, char *pData, int dataLength ) {
    MQISDPMH   hMsg;
    PUB_PARMS *pPp;
    int        rc = MQISDP_OK;
    char       rcBuf[MQISDP_RC_STRING_LENGTH];

    pPp = (PUB_PARMS*)malloc( sizeof(PUB_PARMS) );

    if ( pPp != NULL ) {
        pPp->strucLength = sizeof(PUB_PARMS);

        /* Set the quality of service for the publication */
        switch ( ppp->qos ) {
        case 0:
            pPp->options = MQISDP_QOS_0;
            break;
        case 1:
            pPp->options = MQISDP_QOS_1;
            break;
        case 2:
            pPp->options = MQISDP_QOS_2;
            break;
        }
    
        /* Is this publication retained? */
        if ( ppp->retain ) {
            pPp->options |= MQISDP_RETAIN;
        }

        /* Add the topic name */
        pPp->topicLength = strlen(ppp->topic);
        pPp->topic = ppp->topic;

        /* Add the data */
        pPp->dataLength = dataLength;
        pPp->data = pData;

        /* Publish */
        rc = MQIsdp_publish( ppp->hConn, &hMsg, pPp );

        if ( (ppp->debug) || (rc != MQISDP_OK) ) {
            demoGetRCtoString( rc, rcBuf, MQISDP_RC_STRING_LENGTH );
            printf( "MQIsdp_publish rc:%s - %.*s\n", rcBuf, dataLength, pData );
        }
        
        ppp->lastSentMsg = hMsg;

        free( pPp );
    }
    
    return rc;
}

