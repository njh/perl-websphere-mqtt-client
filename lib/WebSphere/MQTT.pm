package WebSphere::MQTT;

################
#
# MQTT: WebSphere MQ Telemetry Transport
#
# Nicholas Humfrey
# njh@ecs.soton.ac.uk
#

use strict;
use XSLoader;
use Carp;

use vars qw/$VERSION/;

$VERSION="0.01";

XSLoader::load('WebSphere::MQTT', $VERSION);



sub new {
    my $class = shift;
    my ($blah) = @_;
    
	# Store parameters
    my $self = {
    	'client_id'	=> undef,		# 23 chars
    	'broker'	=> '127.0.0.1',	# default is this computer
    	'port'		=> 1883,		# default port 1883
    	'qos'		=> 0,			# quality of service (0/1/2)
    	'timeout'	=> -1,			# forever
    	'match'		=> undef,		# no data to match
    	'debug'		=> 0,			# debugging disabled
    };
    

	### blah ###    

    bless $self, $class;
	return $self;
}

sub connect {

}

sub disconnect {

}

sub publish {

}

sub subscribe {

}

sub unsubscribe {

}

sub status {

}

sub terminate {

}

sub close {
	my $self=shift;
	
	# Close the multicast socket
	#_xs_socket_close( $self->{'sock'} );
	
	#undef $self->{'sock'};
}


sub DESTROY {
    my $self=shift;
    
    #if (exists $self->{'sock'} and defined $self->{'sock'}) {
    #	$self->close();
    # }
}


1;

__END__

=pod

=head1 NAME

WebSphere::MQTT - WebSphere MQ Telemetry Transport

=head1 SYNOPSIS

  use WebSphere::MQTT;

  my $sap = WebSphere::MQTT->new( 'ipv6-global' );

  my $packet = $sap->receive();

  $sap->close();


=head1 DESCRIPTION

Net::SAP allows receiving and sending of SAP (RFC2974) 
multicast packets over IPv4 and IPv6.

=head2 METHODS

=over 4

=item $sap = Net::SAP->new( $group )

The new() method is the constructor for the C<Net::SAP> class.
You must specify the SAP multicast group you want to join:

	ipv4
	ipv6-node
	ipv6-link
	ipv6-site
	ipv6-org
	ipv6-global

Alternatively you may pass the address of the multicast group 
directly. When the C<Net::SAP> object is created, it joins the 
multicast group, ready to start receiving or sending packets.


=item $packet = $sap->receive()

This method blocks until a valid SAP packet has been received.
The packet is parsed, decompressed and returned as a 
C<Net::SAP::Packet> object.


=item $sap->send( $data )

This method sends out SAP packet on the multicast group that the
C<Net::SAP> object to bound to. The $data parameter can either be 
a C<Net::SAP::Packet> object, a C<Net::SDP> object or raw SDP data.

Passing a C<Net::SAP::Packet> object gives the greatest control 
over what is sent. Otherwise default values will be used.

If no origin_address has been set, then it is set to the IP address 
of the first network interface.

Packets greater than 1024 bytes will not be sent. This method 
returns 0 if packet was sent successfully.


=item $group = $sap->group()

Returns the address of the multicast group that the socket is bound to.


=item $sap->close()

Leave the SAP multicast group and close the socket.

=back

=head1 TODO

=over

=item add method of choosing the multicast interface to use

=item ensure that only public v4 addresses are used as origin

=item Packet decryption and validation

=item Improve test script ?

=item Move some XS functions to Net::SAP::Packet ?

=back

=head1 SEE ALSO

L<Net::SAP::Packet>, L<Net::SDP>, perl(1)

L<http://www.ietf.org/rfc/rfc2974.txt>

=head1 BUGS

Please report any bugs or feature requests to
C<bug-net-sap@rt.cpan.org>, or through the web interface at
L<http://rt.cpan.org>.  I will be notified, and then you will automatically
be notified of progress on your bug as I make changes.

=head1 AUTHOR

Nicholas Humfrey, njh@ecs.soton.ac.uk

=head1 COPYRIGHT AND LICENSE

Copyright (C) 2004 University of Southampton

This library is free software; you can redistribute it and/or modify
it under the same terms as Perl itself, either Perl version 5.005 or,
at your option, any later version of Perl 5 you may have available.

=cut
