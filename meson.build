project('upnp-router-control', 'c',
  version: '0.3.1',
  meson_version: '>= 0.50.0',
  license: 'GPLv3+',
)


#################
# Default paths #
#################

prefix = get_option('prefix')
bin_dir = join_paths(prefix, get_option('bindir'))
locale_dir = join_paths(prefix, get_option('localedir'))
data_dir = join_paths(prefix, get_option('datadir'))
man_dir = join_paths(prefix, get_option('mandir'))
po_dir = join_paths(meson.source_root(), 'po')


###########
# Config #
###########

application_id = 'org.upnproutercontrol.UPnPRouterControl'

config_h = configuration_data()

# package
set_defines = [
  ['PACKAGE', meson.project_name()],
  ['PACKAGE_NAME', meson.project_name()],
  ['PACKAGE_STRING', '@0@ @1@'.format(meson.project_name(), meson.project_version())],
  ['PACKAGE_TARNAME', meson.project_name()],
  ['PACKAGE_VERSION', meson.project_version()],
  ['VERSION', meson.project_version()],
  ['GETTEXT_PACKAGE', meson.project_name()],
  ['APPLICATION_ID', application_id]
]

foreach define: set_defines
  config_h.set_quoted(define[0], define[1])
endforeach

configure_file(
         output: 'config.h',
  configuration: config_h
)

cc = meson.get_compiler('c')

# Compiler flags
common_flags = [
  '-DHAVE_CONFIG_H',
  '-DPACKAGE_LOCALE_DIR="@0@"'.format(locale_dir),
]

add_project_arguments(common_flags, language: 'c')


libm_dep = cc.find_library('m')
check_math_functions_required = [
  'ceil',
]


foreach func: check_math_functions_required
  assert(cc.has_function(func, dependencies: libm_dep), func + ' not found')
endforeach


################
# Dependencies #
################

glib = dependency('glib-2.0', version: '>= 2.67.5')
gtk = dependency('gtk+-3.0', version: '>= 3.10')
gssdp = dependency('gssdp-1.2', version: '>= 1.2')
gupnp = dependency('gupnp-1.2', version: '>= 1.2')

i18n = import('i18n')
gnome = import('gnome')

top_inc = include_directories('.')


# Subdirs
subdir('src')
subdir('data')
subdir('po')
