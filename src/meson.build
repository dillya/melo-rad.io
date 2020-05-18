# Melo radio.net module

# Module sources
src = [
	'melo_radio_net_browser.c',
	'melo_radio_net.c'
]

# Library dependencies
libmelo_dep = dependency('melo', version : '>=1.0.0')
libmelo_proto_dep = dependency('melo_proto', version : '>=1.0.0')

# Generate module
shared_library(
	'melo_radio_net',
	src,
	dependencies : [libmelo_dep, libmelo_proto_dep],
	install : true,
	install_dir : libmelo_dep.get_pkgconfig_variable('moduledir'))
