# -*- mode:python; coding:utf-8; -*-

#  Me TV — A DVB-T player.
#
#  Copyright © 2014 Russel Winder
#
#  This program is free software: you can redistribute it and/or modify it under the terms of the GNU
#  General Public License as published by the Free Software Foundation, either version 3 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
#  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
#  License for more details.
#
#  You should have received a copy of the GNU General Public License along with this program.  If not, see
#  <http://www.gnu.org/licenses/>.
#
#  Author : Russel Winder <russel@winder.org.uk>

import os

try:
    version = file('VERSION').read().strip()
except:
    print 'Version file not present, build will not be undertaken.'
    Exit(1)

osName, _, versionNumber, _, archName = os.uname()

buildDirectory = 'Build'

defaultBindirSubdirectory = '/bin'
defaultDatadirSubdirectory = '/share'

defaultPrefix = '/usr/local'
defaultBindir = defaultPrefix + defaultBindirSubdirectory
defaultDatadir = defaultPrefix + defaultDatadirSubdirectory

prefix = defaultPrefix
bindir = defaultBindir
datadir = defaultDatadir
if 'PREFIX' in os.environ:
    prefix = os.environ['PREFIX']
    bindir = prefix + defaultBindirSubdirectory
    datadir = prefix + defaultDatadirSubdirectory
if 'BINDIR' in os.environ:
    bindir = os.environ['BINDIR']
if 'DATADIR' in os.environ:
    datadir = os.environ['DATADIR']
defaultPrefix = prefix
defaultBindir = bindir
defaultDatadir = datadir
localBuildOptions = 'local_build_options.scons'
if os.access(localBuildOptions, os.R_OK):
    execfile(localBuildOptions)
if prefix != defaultPrefix:
    if bindir == defaultBindir:
        bindir = prefix + defaultBindirSubdirectory
    if datadir == defaultDatadir:
        datadir = prefix + defaultDatadirSubdirectory
if 'prefix' in ARGUMENTS:
    prefix = ARGUMENTS.get('prefix')
    bindir = prefix + defaultBindirSubdirectory
    datadir = prefix + defaultDatadirSubdirectory
bindir = ARGUMENTS.get('bindir', bindir)
datadir = ARGUMENTS.get('datadir', datadir)

environment = Environment(
    tools=['g++', 'gnulink'],
    CXX = 'g++-4.9' if 'fc2' not in versionNumber else 'g++',
    PREFIX=prefix,
    BINDIR=bindir,
    DATADIR=datadir,
    VERSION=version,
    GETTEXT_PACKAGE='me-tv',
    PACKAGE_NAME='Me TV',
    PACKAGE_DATA_DIR=datadir + '/me-tv',
    PACKAGE_LOCALE_DIR=datadir + '/locale',
    CXXFLAGS=[
        '-std=c++1y',
        '-g',
        '-O2',
        '-W',
        '-Wall',
        '-Wundef',
        '-Wcast-align',
        '-Wno-unused-parameter',
        #'-Wshadow',
        #'-Wredundant-decls',
        '-Wextra',
        '-Wcast-align',
        '-Wcast-qual',
        '-Wcomment',
        '-Wformat',
        '-Wmissing-braces',
        '-Wpacked',
        '-Wparentheses',
        '-Wpointer-arith',
        '-Wreturn-type',
        '-Wsequence-point',
        '-Wstrict-aliasing',
        '-Wstrict-aliasing=2',
        '-Wswitch-default',
        '-Wundef',
    ]
)

environment['main_dependencies'] = (
    ('sqlite3', '>= 3.8.4.3'),
    ('gtkmm-2.4', '>= 2.24.4'),
    ('giomm-2.4', '>= 2.38.2'),  # 2.40.0
    ('gthread-2.0', '>= 2.38.2'),  # 2.40.0
    ('gconfmm-2.6', '>= 2.28.0'),
    ('unique-1.0', '>= 1.1.6'),
    ('x11', '>= 1.6.1'),  # 1.6.2
    ('dbus-1', '>= 1.6.12'),  # 1.8.2
    ('dbus-glib-1', '>= 0.100.2'),  # 0.102
)

environment['xine_player_dependencies'] = (
    ('libxine', '>= 1.2.5'),
    ('x11', '>= 1.6.1'),  # 1.6.2
    ('giomm-2.4', '>= 2.38.2'),
)

Export('environment')

metv, metvXinePlayer = SConscript('src/SConscript', variant_dir=buildDirectory + '/src', duplicate=0)
SConscript('po/SConscript', variant_dir=buildDirectory + '/po', duplicate=0)

desktop = Command('me-tv.desktop', ['me-tv.desktop.in', 'po/.intltool-merge-cache'],
    'intltool-merge  -d -u -c ./po/.intltool-merge-cache ./po $SOURCE $TARGET'
)

schemas = Command('me-tv.schemas', ['me-tv.schemas.in', 'po/.intltool-merge-cache'],
    'intltool-merge  -s -u -c ./po/.intltool-merge-cache ./po $SOURCE $TARGET'
)

Alias('install', [
    Install(datadir + '/pixmaps/', Glob('*.png')),
    Install(datadir + '/man/man1/', Glob('me-tv*.1')),
    Install(datadir + '/applications/', desktop),
    Install(prefix + '/etc/gconf/schemas/', schemas),
])
