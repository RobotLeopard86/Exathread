project('exathread', 'cpp', version: '3.0.0', license: 'Apache-2.0', default_options: [ 'cpp_std=c++20'])

# Library
exathread_dep = declare_dependency(include_directories: include_directories('include'))
install_headers('include' / 'exathread.hpp')

# Tests
if get_option('testing')
    subdir('test')
endif
