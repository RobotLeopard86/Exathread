project('exathread', 'cpp', version: '2.0.1', license: 'Apache-2.0', default_options: [ 'cpp_std=c++20'])

exathread_dep = declare_dependency(include_directories: include_directories('include'))

args = []
if host_machine.system() == 'windows'
	args += '-D_DLL'
	args += '-D_MT'
	if get_option('buildtype') == 'debug'
		args += '-D_DEBUG'
		args += '-D_ITERATOR_DEBUG_LEVEL=2'
		args += '-fms-runtime-lib=dll_dbg'
	else
		args += '-fms-runtime-lib=dll'
	endif
endif

test('testeroo', executable('testeroo', 'test' / 'testeroo.cpp', cpp_args: args), workdir: meson.project_source_root() / 'test')
