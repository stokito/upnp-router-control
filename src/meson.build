src_inc = include_directories('.')


headers = files(
  'urc-gui.h',
  'urc-upnp.h',
  'urc-graph.h',
)


sources = files(
  'urc-main.c',
  'urc-gui.c',
  'urc-upnp.c',
  'urc-graph.c',
)

urc_deps = [
  libm_dep,
  glib,
  gtk,
  gssdp,
  gupnp,
]


sources += gnome.compile_resources(
  'urc-resources',
  'upnp-router-control.gresource.xml',
  source_dir: data_dir,
  c_name: '_urc',
  export: true,
)


ldflags = cc.get_supported_link_arguments('-Wall')


urc = executable(
  meson.project_name(),
  'urc-main.c',
  sources: sources,
  dependencies: urc_deps,
  include_directories:  [
      top_inc,
      include_directories('.'),
    ],
  install: true,
  install_dir: bin_dir,
)



