#! /usr/bin/env python
# encoding: utf-8
# Daniele Napolitano, 2009

import os
import intltool
import gnome

# the following two variables are used by the target "waf dist"
VERSION='0.2'
APPNAME='upnp-router-control'

# these variables are mandatory ('/' are converted automatically)
srcdir = '.'
blddir = '_build_'

def set_options(opt):
    opt.add_option('--disable-libcurl', action='store_true', default=False, help='Compile UPnP Router Control without icon download support')

def configure(conf):

    import Options

    conf.check_tool('gcc intltool')

    conf.check_cfg(package='gtk+-3.0', uselib_store='GTK', atleast_version='3.0', mandatory=True, args='--cflags --libs')
    conf.check_cfg(package='gupnp-1.0', uselib_store='GUPNP', mandatory=True, args='--cflags --libs')

    if Options.options.disable_libcurl == False:
        conf.check_cfg(package='libcurl', uselib_store='LIBCURL', mandatory=False, args='--cflags --libs')
    else:
        print("Disabling libcurl support")

    conf.define('PACKAGE', APPNAME)
    conf.define('VERSION', VERSION)
    conf.define('PACKAGE_NAME', APPNAME)
    conf.define('PACKAGE_VERSION', APPNAME + '-' + VERSION)
    conf.define('PACKAGE_BUGREPORT','https://bugs.launchpad.net/upnp-router-control')
    conf.define('GETTEXT_PACKAGE', APPNAME)
    conf.define('PACKAGE_DATADIR', conf.env['DATADIR'] + '/' + APPNAME)

    conf.env.append_value('CCFLAGS', '-DHAVE_CONFIG_H')

    conf.write_config_header('config.h')

def build(bld):
    bld.env['PACKAGE_DATADIR'] = bld.env['DATADIR'] + '/' + APPNAME

    bld.add_subdirs('src data')

    # translations
    if bld.env['INTLTOOL']:
        bld.add_subdirs('po')


def shutdown():
	# Postinstall tasks:
	gnome.postinstall_icons() # Updating the icon cache
