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
MQIsdp_version()


## \
## Creates a new socket and joins multicast group \
## \
# \
#mcast_socket_t* \
#_xs_socket_create(host,port,hops) \
#	char *host \
#	int port \
#	int hops \
#  CODE: \
#	RETVAL = mcast_socket_create( \
#		host,	// host \
#		port,	// port \
#		hops,	// ttl \
#		0		// loopback \
#	); \
#  OUTPUT: \
#  	RETVAL \
# \
# \
# \
## \
## Blocks waiting for a packet and returns a  \
## HASHREF containing the data, its length \
## and the address the packet came from \
## \
# \
#HV * \
#_xs_socket_recv(socket) \
#	mcast_socket_t* socket \
#  CODE: \
#	char buffer[SAP_BUFFER_SIZE]; \
#	char from[NI_MAXHOST]; \
#	int size; \
# \
#	size = mcast_socket_recv( \
#				socket, \
#				buffer, SAP_BUFFER_SIZE,  \
#				from, NI_MAXHOST ); \
#	 \
#	 \
#	if (size <= 0) { \
#		RETVAL = NULL; \
#	} else { \
#		HV* hash = newHV(); \
#		hv_store(hash, "from", 4, newSVpv(from, 0), 0); \
#		hv_store(hash, "size", 4, newSViv(size), 0); \
#		hv_store(hash, "data", 4, newSVpv(buffer, size), 0); \
#		RETVAL = hash; \
#		sv_2mortal((SV*)RETVAL); \
#	} \
#	 \
#  OUTPUT: \
#  	RETVAL \
# \
# \
## \
## Sends out a packet \
## \
# \
#int \
#_xs_socket_send(socket,data) \
#	mcast_socket_t* socket \
#	SV* data \
#  CODE: \
#	STRLEN data_len; \
#	char * data_ptr; \
#	data_ptr = SvPV(data, data_len); \
#	RETVAL = mcast_socket_send( socket, data_ptr, data_len); \
#  OUTPUT: \
#  	RETVAL \
# \
# \
## \
## Close multicast socket \
## \
# \
#void \
#_xs_socket_close(socket) \
#	mcast_socket_t*	socket \
#  CODE: \
#	mcast_socket_close( socket ); \
# \
# \
## \
## Returns the protocol family the multicast \
## socket is bound to as a string (ipv4/ipv6) \
## \
# \
#char* \
#_xs_socket_family(socket) \
#	mcast_socket_t*	socket \
#  CODE: \
#	RETVAL = mcast_socket_get_family( socket ); \
#  OUTPUT: \
#  	RETVAL \
# \
# \
## \
## Converts a binary IP address (v4/v6) to a string \
## \
# \
#SV * \
#_xs_ipaddr_to_str(family, address) \
#	char* family \
#	char* address \
#  CODE: \
#  	if (address==NULL) \
#  		XSRETURN_UNDEF; \
#  	if (family==NULL || strlen(family)==0) \
#  		XSRETURN_UNDEF; \
#   \
#	if (strncasecmp(family, "ipv4", 4)==0) { \
#		char string[INET_ADDRSTRLEN]; \
#		inet_ntop(AF_INET, address, string, sizeof(string)); \
#		RETVAL = newSVpv( string, 0 ); \
#	} else if (strncasecmp(family, "ipv6", 4)==0) { \
#		char string[INET6_ADDRSTRLEN]; \
#		inet_ntop(AF_INET6, address, string, sizeof(string)); \
#		RETVAL = newSVpv( string, 0 ); \
#	} else { \
#		croak("Unknown family passed to _xs_ipaddr_to_str()"); \
#		XSRETURN_UNDEF; \
#	} \
#  OUTPUT: \
#  	RETVAL \
# \
## \
## Converts a string to a binary IP address (v4/v6) \
## \
# \
#SV * \
#_xs_str_to_ipaddr(family, string) \
#	char* family \
#	char* string \
#  CODE: \
#  	if (string==NULL || strlen(string)==0) \
#  		XSRETURN_UNDEF; \
#  	if (family==NULL || strlen(family)==0) \
#  		XSRETURN_UNDEF; \
#   \
#	if (strncasecmp(family, "ipv4", 4)==0) { \
#		struct in_addr inaddr; \
#		inet_pton(AF_INET, string, &inaddr); \
#		RETVAL = newSVpv( (char*)&inaddr, sizeof(inaddr) ); \
#	} else if (strncasecmp(family, "ipv6", 4)==0) { \
#		struct in6_addr in6addr; \
#		inet_pton(AF_INET6, string, &in6addr); \
#		RETVAL = newSVpv( (char*)&in6addr, sizeof(in6addr) ); \
#	} else { \
#		croak("Unknown family passed to _xs_str_to_ipaddr()"); \
#		XSRETURN_UNDEF; \
#	} \
#  OUTPUT: \
#  	RETVAL \
# \
# \
## \
## Returns a global IP address for this machine as a string \
## \
# \
#SV * \
#_xs_origin_addr(family) \
#	char* family \
#  CODE: \
#  	if (family==NULL || strlen(family)==0) \
#  		XSRETURN_UNDEF; \
# \
#	if (strncasecmp(family, "ipv4", 4)==0) { \
#		RETVAL = get_origin_address( AF_INET ); \
#	} else if (strncasecmp(family, "ipv6", 4)==0) { \
#		RETVAL = get_origin_address( AF_INET6 ); \
#	} else { \
#		croak("Unknown family passed to _xs_origin_addr()"); \
#		XSRETURN_UNDEF; \
#	} \
#  OUTPUT: \
#  	RETVAL \
# \
# \
##  \
## Uses FVN to calculate hash: \
## \
## http://www.isthe.com/chongo/tech/comp/fnv/ \
## \
# \
#unsigned short \
#_xs_16bit_hash( data ) \
#	SV* data \
#  CODE: \
#	STRLEN data_len; \
#	char * data_ptr; \
#	Fnv32_t hash; \
# \
#  	if (data==NULL) XSRETURN_UNDEF; \
#	data_ptr = SvPV(data, data_len); \
# \
# \
#	// Calulate a 32bit hash and fold it into 16 bits \
#	hash = fnv_32_buf(data, data_len, FNV1_32_INIT); \
#	hash = (hash>>16) ^ (hash & MASK_16); \
# \
#	RETVAL = hash; \
# \
#  OUTPUT: \
#  	RETVAL \
