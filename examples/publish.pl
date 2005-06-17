#!/usr/bin/perl
#
# publish.pl
#

use WebSphere::MQTT::Client;
use strict;


my $mqtt = new WebSphere::MQTT::Client( 'smartlab.combe.chem.soton.ac.uk' );


# Connect to Broker
$mqtt->connect();


$mqq->dump_config();



## Not finished yet ##

