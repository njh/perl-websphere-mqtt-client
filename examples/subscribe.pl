#!/usr/bin/perl
#
# subscribe.pl
#
# Subscribe to a MQTT topic
# and display message to STDOUT
#

use WebSphere::MQTT::Client;
use Data::Dumper;
use strict;


# 
my $mqtt = new WebSphere::MQTT::Client(
	Hostname => 'smartlab.combe.chem.soton.ac.uk',
	Debug => 1,
);


# Connect to Broker
$mqtt->connect();


print Dumper( $mqtt );


sleep 1;
print "status=".$mqtt->status()."\n";
sleep 1;
print "status=".$mqtt->status()."\n";
sleep 1;
print "status=".$mqtt->status()."\n";


# Disconnect from the broker
$mqtt->disconnect();

sleep 2;

print "status=".$mqtt->status()."\n";

# Clean up
$mqtt->terminate();

print Dumper( $mqtt );

