
use strict;
use Test;


# use a BEGIN block so we print our plan before WebSphere::MQTT::Client is loaded
BEGIN { plan tests => 3 }

# load WebSphere::MQTT::Client
use WebSphere::MQTT::Client;


# Helpful notes.  All note-lines must start with a "#".
print "# I'm testing WebSphere::MQTT::Client version $WebSphere::MQTT::Client::VERSION\n";

# Module has loaded sucessfully 
ok(1);



# Now try creating a new WebSphere::MQTT::Client object
my $mqtt = WebSphere::MQTT::Client->new();

ok( $mqtt );



# Close the socket
$mqtt->close();

ok(1);


exit;

