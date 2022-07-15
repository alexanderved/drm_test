project('drm_test',
  'c',
  default_options: [
    'warning_level=3',
    'c_std=c99',
  ]
)

cc = meson.get_compiler('c')

warning_level = 3

defines = ['-D_POSIX_C_SOURCE=200809L']
defines += '-DDEBUG'
if get_option('buildtype') == 'debug' or get_option('buildtype') == 'debugoptimized'
  defines += '-DDEBUG'
endif

deps = [
  dependency('libdrm'),
  dependency('gbm'),
  dependency('egl'),
]

if get_option('glcore')
  deps += dependency('gl')
  defines += '-DGL_GLEXT_PROTOTYPES=1'
  defines += '-DHAVE_GL_CORE=1'
else
  deps += dependency('glesv2')
endif

sources = []

subdir('src')

executable('drm_test',
  sources,
  dependencies: deps,
  c_args: defines,
)
