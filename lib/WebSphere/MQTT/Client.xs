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


MODULE = WebSphere::MQTT::Client	PACKAGE = WebSphere::MQTT::Client


##
## Prints library version to STDOUT
##
void
_xs_version()
  CODE:
   MQIsdp_version();


##
## Creates a new socket and joins multicast group
##
#
#mcast_socket_t*
#_xs_socket_create(host,port,hops)
#	char *host
#	int port
#	int hops
#  CODE:
#	RETVAL = mcast_socket_create(
#		host,	// host
#		port,	// port
#		hops,	// ttl
#		0		// loopback
#	);
#  OUTPUT:
#  	RETVAL
#
#
#
