project('exathread', 'cpp', version: '1.0.0', license: 'Apache-2.0', default_options: [ 'cpp_std=c++20'])

exathread_dep = declare_dependency(include_directories: include_directories('include'))

test('testeroo', executable('testeroo', 'test' / 'testeroo.cpp'), workdir: meson.project_source_root() / 'test')