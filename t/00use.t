
use strict;
use Test;


# use a BEGIN block so we print our plan before WebSphere::MQTT is loaded
BEGIN { plan tests => 3 }

# load WebSphere::MQTT
use WebSphere::MQTT;


# Helpful notes.  All note-lines must start with a "#".
print "# I'm testing WebSphere::MQTT version $WebSphere::MQTT::VERSION\n";

# Module has loaded sucessfully 
ok(1);



# Now try creating a new WebSphere::MQTT object
my $mqtt = WebSphere::MQTT->new();

ok( $mqtt );



# Close the socket
$mqtt->close();

ok(1);


exit;

