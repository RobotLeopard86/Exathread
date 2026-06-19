tests = [
    'pool_create',
    'submit_void',
    'submit_value',
    'submit_detached',
    'exception',
    'batch',
    'then',
    'yield_for',
    'yield_until',
    'multifuture_status',
    'wait_idle',
    'pool_destructor',
    'multifuture_guards',
    'stress',
    'multifuture_then',
    'void_continuations',
    'task_refcount',
    'exception_rethrow',
    'multifuture_construction',
    'corowrap_coroutine',
    'ringbuf_stress',
    'all_scheduling'    
]

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

foreach name : tests
    test(name, executable('TEST_' + name, name + '.cpp', dependencies: exathread_dep, cpp_args: args))
endforeach
