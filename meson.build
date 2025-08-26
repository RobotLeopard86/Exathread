project('exathread', 'cpp', meson_version: '>=1.2.0', version: '1.0.0', license: 'Apache-2.0', default_options: [ 'cpp_std=c++20', 'b_vscrt=from_buildtype', 'default_library=static' ])

# Get some options
bt = get_option('buildtype')
crtstatic = get_option('crt_static')

# Set some Windows-specific options
if host_machine.system() == 'windows'
	argsyntax = meson.get_compiler('cpp').get_argument_syntax()
	add_project_arguments('-DEXATHREAD_BUILD', language: 'cpp')
	if not crtstatic
		add_project_arguments(['-D_DLL', '-D_MT'], language: 'cpp')
	endif
	if bt.contains('debug')
		add_project_arguments(['-D_ITERATOR_DEBUG_LEVEL=2'], language: 'cpp')
		if not crtstatic
			if argsyntax == 'msvc'
				add_project_arguments('/MDd', language: 'cpp')
			else
				add_project_arguments('-fms-runtime-lib=dll_dbg', language: 'cpp')
			endif
		endif
	else
		if not crtstatic
			if argsyntax == 'msvc'
				add_project_arguments('/MD', language: 'cpp')
			else
				add_project_arguments('-fms-runtime-lib=dll', language: 'cpp')
			endif
		endif
	endif
endif

# Define debug macro
if bt.contains('debug')
	add_project_arguments(['-D_DEBUG'], language: 'cpp')
endif

# Now for the important stuff
inc = include_directories('include')
libexathread = library('exathread', 'src' / 'exathread.cpp', pic: true, include_directories: inc)
exathread_dep = declare_dependency(include_directories: inc, link_with: libexathread)