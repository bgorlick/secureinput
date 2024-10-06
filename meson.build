project('secureinput', 'c',
  version : '0.1',
  default_options : ['warning_level=3', 'c_std=c11'])

nasm = find_program('nasm')

add_project_arguments('-fPIC', language : 'c')
add_project_arguments('-DPIC', language : 'nasm')

inc = include_directories('include', 'tests')

# Custom options
enable_debug = get_option('enable_debug')
enable_optimize = get_option('enable_optimize')
enable_sanitize = get_option('enable_sanitize')

c_args = []
link_args = []
nasm_args = ['-f', 'elf64', '-DPIC']

if enable_debug
  c_args += ['-DDEBUG', '-g']
  nasm_args += ['-g']
endif

if enable_optimize
  c_args += ['-O3', '-march=native']
endif

if enable_sanitize == 'address'
  c_args += ['-fsanitize=address']
  link_args += ['-fsanitize=address']
endif

add_project_arguments(c_args, language : 'c')
add_project_arguments('-D_POSIX_C_SOURCE=199309L', language : 'c')
add_project_link_arguments(link_args, language : 'c')

# Add flags to suppress warnings about relocations in read-only section
add_project_link_arguments('-z', 'notext', language : 'c')
add_project_link_arguments('-shared', language : 'c')

asm_gen = generator(nasm,
  output : '@BASENAME@_asm.o',
  arguments : nasm_args + ['-o', '@OUTPUT@', '@INPUT@'])

lib_sources = ['lib/secureinput.c', asm_gen.process('lib/secureinput.asm')]

secureinput_lib = both_libraries('secureinput',
  lib_sources,
  include_directories : inc,
  install : true,
  pic : true,
  c_args : ['-fno-plt'],
  link_args : ['-Wl,-z,notext'])

cc = meson.get_compiler('c')
m_dep = cc.find_library('m', required : false)

executable('secureinput_main',
  'examples/main.c',
  link_with : secureinput_lib,
  include_directories : inc,
  dependencies : m_dep)

executable('bench_secureinput',
  'tests/bench_secureinput.c',
  link_with : secureinput_lib,
  include_directories : inc,
  dependencies : m_dep)

test_const_time = executable('test_const_time',
  'tests/test_const_time.c',
  link_with : secureinput_lib,
  include_directories : inc,
  dependencies : m_dep)

run_target('run_test_const_time',
  command : test_const_time)

install_headers('include/secureinput.h')