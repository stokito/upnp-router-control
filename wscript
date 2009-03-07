#! /usr/bin/env python
# encoding: utf-8
# Daniele Napolitano, 2009

import os
import intltool

# the following two variables are used by the target "waf dist"
VERSION='0.1'
APPNAME='upnp-port-mapper'

# these variables are mandatory ('/' are converted automatically)
srcdir = '.'
blddir = 'build'


def set_options(opt):
    pass

def configure(conf):
    
    conf.check_tool('gcc intltool')
 
    conf.check_cfg(package='gtk+-2.0', uselib_store='GTK', atleast_version='2.14', mandatory=True, args='--cflags --libs')
    conf.check_cfg(package='libglade-2.0', uselib_store='GLADE', atleast_version='2.6', mandatory=True, args='--cflags --libs')
    conf.check_cfg(package='gupnp-1.0', uselib_store='GUPNP', mandatory=True, args='--cflags --libs')

    conf.define('PACKAGE', APPNAME)
    conf.define('VERSION', VERSION)
    conf.define('PACKAGE_NAME', APPNAME)
    conf.define('PACKAGE_VERSION', APPNAME + '-' + VERSION)
    conf.define('PACKAGE_BUGREPORT','https://bugs.launchpad.net/upnp-port-mapper')
    conf.define('GETTEXT_PACKAGE', APPNAME)

    conf.env.append_value('CCFLAGS', '-DHAVE_CONFIG_H')

    conf.write_config_header('src/config.h')
    

def build(bld):
 
    bld.add_subdirs('src data')

    # translations
    if bld.env['INTLTOOL']:
        obj = bld.new_task_gen('intltool_po')
        obj.podir = 'po'
        obj.appname = 'upnp-port-mapper'

    # Launcher
    obj			= bld.new_task_gen('intltool_in')
    obj.source		= 'upnp-port-mapper.desktop.in'
    obj.flags		= '-d'
    obj.install_path	= '${DATADIR}/applications'
