project('exathread', 'cpp', version: '1.0.0', license: 'Apache-2.0', default_options: [ 'cpp_std=c++20'])

exathread_dep = declare_dependency(include_directories: include_directories('include'))

defs = []
if host_machine.system() == 'windows'
	defs += '-D_DLL'
	defs += '-D_MT'
	if get_option('buildtype') == 'debug'
		defs += '-D_DEBUG'
		defs += '-D_ITERATOR_DEBUG_LEVEL=2'
		defs += '-fms-runtime-lib=dll_dbg'
	else
		defs += '-fms-runtime-lib=dll'
	endif
endif

test('testeroo', executable('testeroo', 'test' / 'testeroo.cpp', cpp_args: defs), workdir: meson.project_source_root() / 'test')