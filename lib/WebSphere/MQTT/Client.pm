package WebSphere::MQTT::Client;

################
#
# MQTT: WebSphere MQ Telemetry Transport
#
# Nicholas Humfrey
# njh@ecs.soton.ac.uk
#

use strict;
use Sys::Hostname;
use Time::HiRes;
use XSLoader;
use Carp;

use vars qw/$VERSION/;

$VERSION="0.01";

XSLoader::load('WebSphere::MQTT::Client', $VERSION);



sub new {
    my $class = shift;
    my (%args) = @_;
    
 	# Store parameters
    my $self = {
    	'host'		=> '127.0.0.1',	# broker's hostname (localhost)
    	'port'		=> 1883,		# broker's port
    	'clientid'	=> undef,		# our client ID
    	'debug'		=> 0,			# debugging disabled
  	
 
    	# Advanced options (with sensible defaults)
    	'clean_start'	=> 1,			# set CleanStart flag ?
    	'keep_alive'	=> 10,			# timeout (in seconds) for receiving data
    	'retry_count'	=> 10,
    	'retry_interval' => 10,

		# Used internally only    	
  		'handle'			=> undef,			# Connection Handle
 		'send_task_info'	=> undef,			# Send Thread Parameters
 		'recv_task_info'	=> undef,			# Receive Thread Parameters
		'api_task_info'		=> undef,			# API Thread Parameters

		# TODO: LWT stuff
		#'lwt_enabled'	=> 0,
		#'lwt_message'	=> undef,
		#'lwt_qos'		=> 0,
		#'lwt_topic'	=> undef,
		#'lwt_retain'	=> 0,

    };
    
    # Bless the hash into an object
    bless $self, $class;

    # Arguments specified ?
    if (defined %args) {
		foreach (keys %args) {
			my $key = $_;
			$key =~ tr/A-Z/a-z/;
			$key =~ s/\W/_/g;
			$key = 'host' if ($key eq 'hostname');
			$self->{$key} = $args{$_};
		}
    }
    
    # Generate a Client ID if we don't have one 
    if (defined $self->{'clientid'}) {
    	$self->{'clientid'} = substr($self->{'clientid'}, 0, 23);
  	} else {
		my $hostname = hostname();
		my ($host, $domain) = ($hostname =~ /^([^\.]+)\.?(.*)$/);
    	$self->{'clientid'} = substr($host, 0, 22-length($$)).'-'.$$;
    }

	# Start threads (if enabled)
	$self->xs_start_tasks() or die("xs_start_tasks() failed");

	# Dump configuration if Debug is enabled
	$self->dump_config() if ($self->{'debug'});

	return $self;
}


sub dump_config {
	my $self = shift;
	
	print "\n";
	print "WebSphere::MQTT::Client config\n";
	print "==============================\n";
	foreach( sort keys %$self ) {
		printf(" %15s: %s\n", $_, $self->{$_});
	}
	print "\n";

}


sub debug {
	my $self = shift;
	my ($debug) = @_;
	
	if (defined $debug) {
		if ($debug) { $self->{'debug'} = 1; }
		else		{ $self->{'debug'} = 0; }
	}
	
	return $self->{'debug'};
}



sub connect {
	my $self = shift;	
	
	# Connect
	my $result = $self->xs_connect( $self->{'api_task_info'} );
	return $result unless($result eq 'OK');

	# Print the result if debugging enabled
	print "xs_connect: $result\n" if ($self->{'debug'});
	
	# Wait until we are connected
	# FIXME: *with timeout*
	my $state = 'CONNECTING';
	while ($state eq 'CONNECTING') {
		$state = $self->status();
		sleep 0.5;
	}
	
	# Failed to connect ?
	if ($state ne 'CONNECTED') {
		$self->disconnect();
		return 'FAILED';
	}
	
	# Success
	return 0;
}

sub disconnect {
	my $self = shift;

	# Disconnect
	my $result = $self->xs_disconnect();
	
	# Print the result if debugging enabled
	print "xs_disconnect: $result\n" if ($self->{'debug'});
				
	# Return 0 if result is OK
	return 0 if ($result eq 'OK');
	return $result;
}

sub publish {
	my $self = shift;
 		# params for subscribe/publish
#		'qos'			=> 0,			# quality of service (0/1/2)
#    	'retain',		=> 0,			# broker will retain messgae until 
    									#   another publication is received 
    									#   for the same topic.  

}

sub subscribe {
	my $self = shift;
	my ($topic, $qos) = @_;
	
	croak("Usage: subscribe(topic, [qos])") unless (defined $topic);
	$qos = 0 unless (defined $qos);

	# Subscribe
	my $result = $self->xs_subscribe( $topic, $qos );
	
	# Print the result if debugging enabled
	print "xs_subscribe[$topic]: $result\n" if ($self->{'debug'});
				
	# Return 0 if result is OK
	return 0 if ($result eq 'OK');
	return $result;
}


sub receivePub {
	my $self = shift;
#    	'match'			=> undef,		# only receive messages which look like this

}

sub unsubscribe {
	my $self = shift;
	my ($topic) = @_;
	
	croak("Usage: unsubscribe(topic)") unless (defined $topic);

	# Subscribe
	my $result = $self->xs_unsubscribe( $topic );
	
	# Print the result if debugging enabled
	print "xs_unsubscribe[$topic]: $result\n" if ($self->{'debug'});
				
	# Return 0 if result is OK
	return 0 if ($result eq 'OK');
	return $result;
}


sub status {
	my $self = shift;
	return $self->xs_status();
}

sub terminate {
	my $self = shift;

	# Disconnect first (if connected)
    if (exists $self->{'handle'} and defined $self->{'handle'}) {
    	$self->disconnect();
	}

	# Terminate threads and free memory
	my $result = $self->xs_terminate();	
	
	# Return 0 if result is OK
	return 0 if ($result eq 'OK');
	return $result;
}

sub libversion {
	return eval { xs_version(); };
}


sub DESTROY {
    my $self=shift;
    
    $self->terminate();
}


1;

__END__

=pod

=head1 NAME

WebSphere::MQTT::Client - WebSphere MQ Telemetry Transport Client

=head1 SYNOPSIS

  use WebSphere::MQTT::Client;

  my $mqtt = WebSphere::MQTT::Client->new( 'localhost' );

  $mqtt->disconnect();


=head1 DESCRIPTION

WebSphere::MQTT::Client

Publish and Subscribe to broker.

=head2 METHODS

=over 4

=item $mqtt = WebSphere::MQTT::Client->new( $host, $port )

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
