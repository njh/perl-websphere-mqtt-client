/*

	WebSphere MQ Telemetry Transport
	Perl Interface to IA93

	Nicholas Humfrey
	University of Southampton
	njh@ecs.soton.ac.uk
	
*/

#include "EXTERN.h"
#include "perl.h"
#include "XSUB.h"

/* WMQTT include */
#include "MQIsdp.h"



/* Get the connection handle from hashref */
MQISDPCH
get_handle_from_hv( HV* hash ) {
	SV** svp = NULL;
	IV pointer;
	
	svp = hv_fetch( hash, "handle", 6, 0 );
	if (svp == NULL) {
		warn("Connection handle is missing from hash");
		return NULL;
	}
	
	if (!sv_derived_from(*svp, "MQISDPCH")) {
		//warn("Connection handle isn't isn't of type MQISDPCH");
		return NULL;
	}

	// Re-reference and extract the pointer
	pointer = SvIV((SV*)SvRV(*svp));
	return INT2PTR(MQISDPCH,pointer);
}


/* Get debug settings from hashref */
int
get_debug_from_hv( HV* hash ) {
	SV** svp = NULL;
	
	svp = hv_fetch( hash, "debug", 5, 0 );
	if (svp == NULL) {
		warn("Debug setting is missing from hash");
		return 0;
	}
	
	return SvTRUE( *svp );
}


/* Get task info from hashref */
MQISDPTI*
get_task_info_from_hv( HV* hash, char* name ) {
	SV** svp = NULL;
	IV pointer;
	
	svp = hv_fetch( hash, name, strlen(name), 0 );
	if (svp == NULL) return NULL;
	if (!sv_derived_from(*svp, "MQISDPTIPtr")) return NULL;

	// Re-reference and extract the pointer
	pointer = SvIV((SV*)SvRV(*svp));
	return INT2PTR(MQISDPTI*,pointer);
}


/* Undefine the value of a key */
void
hv_key_undef( HV* hash, char* key ) {
	SV** svp = NULL;

	svp = hv_fetch( hash, key, strlen(key), 0 );
	if (svp) {
		sv_setsv(*svp, &PL_sv_undef);
	} else {
		warn("hv_key_undef: Didn't find key in hash");
	}	
}




#define STATUS_CASE_RET( x ) \
  case x:           \
    return #x;    \
    break;


const char*
get_status_string( int statusCode ) {

 	switch(statusCode) {
		STATUS_CASE_RET( MQISDP_OK );
		STATUS_CASE_RET( MQISDP_PROTOCOL_VERSION_ERROR );
		STATUS_CASE_RET( MQISDP_HOSTNAME_NOT_FOUND );
		STATUS_CASE_RET( MQISDP_Q_FULL );
		STATUS_CASE_RET( MQISDP_FAILED );
		STATUS_CASE_RET( MQISDP_PUBS_AVAILABLE );
		STATUS_CASE_RET( MQISDP_NO_PUBS_AVAILABLE );
		STATUS_CASE_RET( MQISDP_PERSISTENCE_FAILED );
		STATUS_CASE_RET( MQISDP_CONN_HANDLE_ERROR );
		STATUS_CASE_RET( MQISDP_NO_WILL_TOPIC );
		STATUS_CASE_RET( MQISDP_INVALID_STRUC_LENGTH );
		STATUS_CASE_RET( MQISDP_DATA_LENGTH_ERROR );
		STATUS_CASE_RET( MQISDP_DATA_TOO_BIG );
		STATUS_CASE_RET( MQISDP_ALREADY_CONNECTED );
		STATUS_CASE_RET( MQISDP_CONNECTION_BROKEN );
		STATUS_CASE_RET( MQISDP_DATA_TRUNCATED );
		STATUS_CASE_RET( MQISDP_CLIENT_ID_ERROR );
		STATUS_CASE_RET( MQISDP_BROKER_UNAVAILABLE );
		STATUS_CASE_RET( MQISDP_SOCKET_CLOSED );
		STATUS_CASE_RET( MQISDP_OUT_OF_MEMORY );
 		
		STATUS_CASE_RET( MQISDP_DELIVERED );
		STATUS_CASE_RET( MQISDP_RETRYING );
		STATUS_CASE_RET( MQISDP_IN_PROGRESS );
		STATUS_CASE_RET( MQISDP_MSG_HANDLE_ERROR );
		
		STATUS_CASE_RET( MQISDP_CONNECTING );
		STATUS_CASE_RET( MQISDP_CONNECTED );
		STATUS_CASE_RET( MQISDP_DISCONNECTED );
	}
	
	return "MQISDP_UNKNOWN";
}


MODULE = WebSphere::MQTT::Client	PACKAGE = WebSphere::MQTT::Client


##
## Prints library version to STDOUT
##
void
xs_version()
  CODE:
   MQIsdp_version();


##
## Alocate memory for TaskInfo
##
int
xs_start_tasks( self )
	HV* self
  PREINIT:
  	MQISDPTI* pSendTaskInfo = NULL;
  	MQISDPTI* pRcvTaskInfo = NULL;
  	MQISDPTI* pApiTaskInfo = NULL;
  	char *clientid = NULL;
  	SV** svp = NULL;
 	SV* sv = NULL;

  CODE:
  	/* Get the client ID */
  	svp = hv_fetch( self, "clientid", 8, 0 );
  	if (svp != NULL) {
  		clientid = SvPV_nolen( *svp );
  		if (strlen(clientid) < 1 || strlen(clientid) > 23) {
  			croak("clientid is not valid");
  		}
  	} else {
  		croak("clientid is not defined");
   	}
  	
  	
  	fprintf(stderr, "xs_init: clientid=%s\n", clientid);
	
	/* Allocate the WMQTT thread parameter structures */
	pSendTaskInfo = (MQISDPTI*)malloc( sizeof(MQISDPTI) );
	pRcvTaskInfo = (MQISDPTI*)malloc( sizeof(MQISDPTI) );
	pApiTaskInfo = (MQISDPTI*)malloc( sizeof(MQISDPTI) );
	
	/* Zero the memory */
	bzero( pSendTaskInfo, sizeof(MQISDPTI) );
	bzero( pRcvTaskInfo, sizeof(MQISDPTI) );
	bzero( pApiTaskInfo, sizeof(MQISDPTI) );
	
	/* Turn thread tracing off */
	pSendTaskInfo->logLevel = LOGNONE;
	pRcvTaskInfo->logLevel = LOGNONE;
	pApiTaskInfo->logLevel = LOGNONE;


	/* Start the threads (if enabled) */
	if ( MQIsdp_StartTasks( pApiTaskInfo, pSendTaskInfo,
                            pRcvTaskInfo, clientid ) != 0 ) {
        croak("Failed to start MQIsdp protocol threads");
        XSRETURN_UNDEF;
    }
    
    /* Store thread parameter pointers */
	sv=sv_setref_pv(newSV(0), "MQISDPTIPtr", (void *)pSendTaskInfo);
	if (hv_store(self, "send_task_info", 14, sv, 0) == NULL) {
		croak("send_task_info not stored");
	}
	
	sv=sv_setref_pv(newSV(0), "MQISDPTIPtr", (void *)pRcvTaskInfo);
	if (hv_store(self, "recv_task_info", 14, sv, 0) == NULL) {
		croak("recv_task_info not stored");
	}
	
	sv=sv_setref_pv(newSV(0), "MQISDPTIPtr", (void *)pApiTaskInfo);
	if (hv_store(self, "api_task_info", 13, sv, 0) == NULL) {
		croak("api_task_info not stored");
	}

    
    /* Successful */
    RETVAL = 1;
    
  OUTPUT:
	self
	RETVAL

 
	
##
## Get connection status
##
SV*
xs_status( self )
	HV* self

  PREINIT:
  	MQISDPCH	handle = NULL;
	int			statusCode=0;
	int			debug=0;
	char        infoString[MQISDP_INFO_STRING_LENGTH];
	const char	*statusString = NULL;
	
  CODE:
  	/* get the connection handle */
  	handle = get_handle_from_hv( self );  
  	debug = get_debug_from_hv( self );
  
 	/* get the connection status */
 	statusCode = MQIsdp_status( handle, MQISDP_RC_STRING_LENGTH, NULL, infoString );
 	statusString = get_status_string( statusCode );
 	 	
 	if (debug) {
 		fprintf(stderr, "xs_status: status=%s[%d] - %s\n", statusString, statusCode, infoString);
 	}
 	
	RETVAL = newSVpv( statusString, 0 );
 	
  OUTPUT:
	RETVAL
 	
 
 
  
##
## Connect to broker
##
int
xs_connect( self, pApiTaskInfo )
	HV* self
   	MQISDPTI	*pApiTaskInfo

 PREINIT:
  	CONN_PARMS	*pCp = NULL;
  	MQISDPCH	handle = NULL;
  	long        connMsgLength = 0;
  	SV**		svp = NULL;
 	SV*			sv = NULL;
  	
  CODE:
  	/* length of Connect Messgae */
	connMsgLength = sizeof(CONN_PARMS);
	
	/* Create Connect data structure */
	pCp = (CONN_PARMS*)malloc( connMsgLength );
	pCp->strucLength = connMsgLength;



    /* Fill out parameters from hashref */
    svp = hv_fetch( self, "clientid", 8, 0 );
    if (svp && SvPOK(*svp))	strcpy( pCp->clientId, SvPV_nolen(*svp) );
	else		croak("'clientid' setting isn't available");
    
    svp = hv_fetch( self, "retry_count", 11, 0 );
    if (svp && SvIOK(*svp))	pCp->retryCount = SvIV(*svp);
	else		croak("'retry_count' setting isn't available");

    svp = hv_fetch( self, "retry_interval", 14, 0 );
    if (svp && SvIOK(*svp))	pCp->retryInterval = SvIV(*svp);
	else		croak("'retry_interval' setting isn't available");

    svp = hv_fetch( self, "keep_alive", 10, 0 );
    if (svp && SvIOK(*svp))	pCp->keepAliveTime = SvIV(*svp);
	else		croak("'retry_interval' setting isn't available");

    svp = hv_fetch( self, "host", 4, 0 );
    if (svp && SvPOK(*svp))	pCp->brokerHostname = SvPV_nolen(*svp);
	else		croak("'host' setting isn't available");

    svp = hv_fetch( self, "port", 4, 0 );
    if (svp && SvIOK(*svp))	pCp->brokerPort = SvIV(*svp);
	else		croak("'port' setting isn't available");


	/* No Persistence yet */
	pCp->pPersistFuncs = NULL;	

	/* Set options flags */
	pCp->options = MQISDP_NONE;
	svp = hv_fetch( self, "clean_start", 11, 0 );
	if (svp) {
		if (SvIV(*svp)) pCp->options |= MQISDP_CLEAN_START;
	} else {
		croak("'clean_start' setting isn't available");
	}

	/* Perform the connect */
	RETVAL = MQIsdp_connect( &handle, pCp, pApiTaskInfo );
	free( pCp );


    /* Store connection handle pointer */
	sv=sv_setref_pv(newSV(0), "MQISDPCH", (void *)handle);
	if (hv_store(self, "handle", 6, sv, 0) == NULL) {
		croak("connection handle not stored");
	}
	
  OUTPUT:
	RETVAL
	


##
## Disconnect from broker
##
int
xs_disconnect( self )
	HV* self

  PREINIT:
   	MQISDPCH	handle = NULL;
  	SV**		svp = NULL;

  CODE:
  	/* get the connection handle */
  	handle = get_handle_from_hv( self );  	
  	
  	/* perform the disconnect */
  	RETVAL = MQIsdp_disconnect( &handle );
  	
  	/* Undef 'handle' if its value now NULL */
  	if (handle==NULL) hv_key_undef( self, "handle" );
  	
  OUTPUT:
	RETVAL


##
## Free memory and Terminate threads
##
int
xs_terminate( self )
	HV* self

  PREINIT:
	SV** svp = NULL;

  CODE:
  	MQISDPTI *pApiTaskInfo = get_task_info_from_hv( self, "api_task_info" );
  	MQISDPTI *pSendTaskInfo = get_task_info_from_hv( self, "send_task_info" );
  	MQISDPTI *pRcvTaskInfo = get_task_info_from_hv( self, "recv_task_info`" );
  
  	/* Free the memory */
  	if (pApiTaskInfo) free( pApiTaskInfo );
  	if (pSendTaskInfo) free( pSendTaskInfo );
  	if (pRcvTaskInfo) free( pRcvTaskInfo );

	/* Undef them in the hash */
	hv_key_undef( self, "api_task_info");
	hv_key_undef( self, "send_task_info");
	hv_key_undef( self, "recv_task_info");

	/* Terminate threads */
	RETVAL = MQIsdp_terminate();
  	
  OUTPUT:
	RETVAL


