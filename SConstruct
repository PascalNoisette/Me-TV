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
import subprocess

try:
    version = file('VERSION').read().strip()
except:
    print 'Version file not present, build will not be undertaken.'
    Exit(1)

osName, _, version_number, _, archName = os.uname()

build_directory = 'Build'

default_bindir_subdirectory = '/bin'
default_datadir_subdirectory = '/share'

default_prefix = '/usr/local'
default_bindir = default_prefix + default_bindir_subdirectory
default_datadir = default_prefix + default_datadir_subdirectory

prefix = default_prefix
bindir = default_bindir
datadir = default_datadir
if 'PREFIX' in os.environ:
    prefix = os.environ['PREFIX']
    bindir = prefix + default_bindir_subdirectory
    datadir = prefix + default_datadir_subdirectory
if 'BINDIR' in os.environ:
    bindir = os.environ['BINDIR']
if 'DATADIR' in os.environ:
    datadir = os.environ['DATADIR']
default_prefix = prefix
default_bindir = bindir
default_datadir = datadir
local_build_options = 'local_build_options.scons'
if os.access(local_build_options, os.R_OK):
    execfile(local_build_options)
if prefix != default_prefix:
    if bindir == default_bindir:
        bindir = prefix + default_bindir_subdirectory
    if datadir == default_datadir:
        datadir = prefix + default_datadir_subdirectory
if 'prefix' in ARGUMENTS:
    prefix = ARGUMENTS.get('prefix')
    bindir = prefix + default_bindir_subdirectory
    datadir = prefix + default_datadir_subdirectory
bindir = ARGUMENTS.get('bindir', bindir)
datadir = ARGUMENTS.get('datadir', datadir)

try:
    catch_directory
except NameError:
    catch_directory = '/usr/include/'

environment = Environment(
    tools=['g++', 'gnulink'],
    PREFIX=prefix,
    BINDIR=bindir,
    DATADIR=datadir,
    VERSION=version,
    GETTEXT_PACKAGE='me-tv',
    PACKAGE_NAME='Me TV',
    PACKAGE_DATA_DIR=datadir + '/me-tv',
    PACKAGE_LOCALE_DIR=datadir + '/locale',
    CXXFLAGS=[
        '-std=c++1y', # -std=c++14 really, but GCC 4.8 doesn't have this.
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
    ],
    build_directory=build_directory,
    catch_directory=catch_directory,
)

main_dependencies = (
    ('sqlite3', '>= 3.8.4.3'),
    ('gtkmm-2.4', '>= 2.24.4'),
    ('giomm-2.4', '>= 2.38.2'),  # 2.40.0
    ('gthread-2.0', '>= 2.38.2'),  # 2.40.0
    ('gconfmm-2.6', '>= 2.28.0'),
    ('unique-1.0', '>= 1.1.6'),
    ('x11', '>= 1.6.1'),  # 1.6.2
    ('dbus-1', '>= 1.6.12'),  # 1.8.2
    ('dbus-glib-1', '>= 0.100.2'),  # 0.102
    ('libmicrohttpd', '>= 0.9.34'),  # 0.9.37
    ('jsoncpp', '>= 0.6'),
)

xine_player_dependencies = (
    ('libxine', '>= 1.2.5'),
    ('x11', '>= 1.6.1'),  # 1.6.2
    ('giomm-2.4', '>= 2.38.2'),
)

def PkgCheckModules(context, library, versionSpecification):
    pattern = '{} {}'.format(library, versionSpecification) if versionSpecification else library
    context.Message('Checking for ' + pattern + ' ... ')
    returnCode = subprocess.call(['pkg-config', '--exists', pattern])
    if returnCode == 0:
        context.Result('ok')
        context.env.MergeFlags('!pkg-config --cflags --libs ' + library)
        return True
    context.Result('failed')
    return False

# Since create_configuration may be called many times and yet only a single copy of the various symbols
# should be written to the config.h file, employ a module scope flag to determine whether to write the
# symbols or not.

config_file_written = False

def create_configuration(environment, dependencies):
    global config_file_written
    configuration = Configure(environment, custom_tests={'PkgCheckModules': PkgCheckModules}, config_h='#src/config.h')  # , clean=False, help=False)
    failedConfiguration = False
    for library, versionPattern in dependencies:
        if not configuration.PkgCheckModules(library, versionPattern):
            failedConfiguration = True
    if failedConfiguration:
        Exit(1)
    for entry in ['VERSION', 'GETTEXT_PACKAGE', 'PACKAGE_NAME', 'PACKAGE_DATA_DIR', 'PACKAGE_LOCALE_DIR']:
        if not config_file_written:
            configuration.Define(entry, '"' + environment[entry] + '"')
        configuration.env.Append(CPPDEFINES='{}=\\"{}\\"'.format(entry, environment[entry]))
    if not config_file_written:
        configuration.Define('ENABLE_NLS', 1)
    config_file_written = True
    configuration.env.Append(CPPDEFINES='HAVE_CONFIG_H')
    configuration.env.Append(CPPPATH='.')  # This is crucial for triggering entering config.h in the DAG.
    return configuration.Finish()

main_environment = create_configuration(environment.Clone(), main_dependencies)
xine_player_environment = create_configuration(environment.Clone(), xine_player_dependencies)

Export('environment', 'main_environment', 'xine_player_environment')

executables = SConscript('src/SConscript', variant_dir=build_directory + '/src', duplicate=0)
built_po_files = SConscript('po/SConscript', variant_dir=build_directory + '/po', duplicate=0)
test_executables = SConscript('tests/SConscript', variant_dir=build_directory + '/tests', duplicate=0)
Depends(test_executables, executables)
built_things = SConscript('SConscript', variant_dir=build_directory, duplicate=0)

Alias('executables', executables)
Alias('mo_files', built_po_files)
Alias('tests', test_executables)
Alias('odds', built_things)

def run_tests(target, source, env):
    assert target[0].name == 'test'
    assert isinstance(source[0].name, str)
    # Return code will indicate whether tests passed or failed but for the moment do not worry about
    # this. For CI though we will have to find a way of percolating the return code out to the CI framework.
    subprocess.call(source[0].abspath)

Command('test', test_executables, run_tests)

Default([executables, built_po_files, built_things])
