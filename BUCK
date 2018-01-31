cxx_library(
  name = 'lib',
  header_namespace = '',
  srcs = glob(['lib/*.cpp']),
  headers = glob(['lib/*.h']),
  exported_headers = glob(['lib/*.h']),
  deps = ['//:tf'],
  compiler_flags = ['-I.'],
  visibility = [
    'PUBLIC',
  ],
)

prebuilt_cxx_library(
  name = 'tf',
  header_only = True,
  header_dirs = ['tf'],
  visibility = [
    'PUBLIC',
  ],
)

prebuilt_cxx_library(
  name = 'libtensorflow_cc',
  shared_lib = 'tf/lib/libtensorflow_cc.so',
  preferred_linkage = 'shared',
  visibility = [
    'PUBLIC',
  ],
)
prebuilt_cxx_library(
  name = 'libtensorflow_framework',
  shared_lib = 'tf/lib/libtensorflow_framework.so',
  preferred_linkage = 'shared',
  visibility = [
    'PUBLIC',
  ],
)

cxx_binary(
  name = 'hearts',
  srcs = [
    'hearts.cpp',
  ],
  compiler_flags = ['-I.'],
  deps = ['//:tf', '//:lib', '//:libtensorflow_cc', '//:libtensorflow_framework'],
)

cxx_binary(
  name = 'analyze',
  srcs = [
    'analyze.cpp',
  ],
  compiler_flags = ['-I.'],
  deps = ['//:tf', '//:lib', '//:libtensorflow_cc', '//:libtensorflow_framework'],
)

cxx_binary(
  name = 'deal',
  srcs = [
    'deal.cpp',
  ],
  compiler_flags = ['-I.'],
  deps = ['//:tf', '//:lib', '//:libtensorflow_cc', '//:libtensorflow_framework'],
)

cxx_binary(
  name = 'disttest',
  srcs = [
    'disttest.cpp',
  ],
  compiler_flags = ['-I.'],
  deps = ['//:tf', '//:lib', '//:libtensorflow_cc', '//:libtensorflow_framework'],
)

cxx_binary(
  name = 'tournament',
  srcs = [
    'tournament.cpp',
  ],
  compiler_flags = ['-I.'],
  deps = ['//:tf', '//:lib', '//:libtensorflow_cc', '//:libtensorflow_framework'],
)

cxx_binary(
  name = 'validate',
  srcs = [
    'validate.cpp',
  ],
  compiler_flags = ['-I.'],
  deps = ['//:tf', '//:lib', '//:libtensorflow_cc', '//:libtensorflow_framework'],
)

cxx_test(
    name = "tests",
    srcs = glob(["tests/*.cpp"]),
    deps = ['//:tf', '//:lib', '//:libtensorflow_cc', '//:libtensorflow_framework'],
)
