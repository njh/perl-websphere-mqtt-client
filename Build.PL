use Module::Build;

my $build = Module::Build->new
(
	module_name => 'Net::MQTT',
	license => 'perl',
	script_files => [
		'examples/subscribe.pl',
		'examples/publish.pl',
	],
	build_requires => {
		'Module::Build' => '0.20'
	},
	requires => {
		'Test' => '1.00',
		'Getopt::Std' => 0,
		'Pod::Usage' => 0,
		'Data::Dumper' => 0,
	},
	include_dirs => [ 'src' ],
	c_source => 'src',
);

$build->create_build_script;
