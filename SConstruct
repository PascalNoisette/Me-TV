import os

try:
    version = file('VERSION').read().strip()
except:
    print 'Version file not present, build will not be undertaken.'
    Exit(1)

osName, _, _, _, archName = os.uname()

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
    tools=['c++', 'link'],
    VERSION=version, GETTEXT_PACKAGE='me-tv', PACKAGE_NAME='Me TV',
    PREFIX=prefix, BINDIR=bindir, DATADIR=datadir,
    PIXMAPDIR=datadir + '/pixmaps', PACKAGE_DATA_DIR=datadir + '/me-tv', LOCALEDIR=datadir + '/locale',
    CXXFLAGS=[
        #'--std=c++1y',
        '-O3',
        '-W', '-Wall', '-Wundef', '-Wcast-align', '-Wno-unused-parameter', #'-Wshadow', #'-Wredundant-decls',
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

environment['dependencies'] = (
    ('sqlite3', '>= 3.8.3'),
    ('gtkmm-2.4', '>= 2.24.4'),
    ('giomm-2.4', '>= 2.10.0'),
    ('gthread-2.0', '>= 2.10.0'),
    ('gconfmm-2.6', '>= 2.0'),
    ('unique-1.0', ''),
    ('x11', ''),
    ('dbus-1', ''),
    ('dbus-glib-1', ''),
    ('libxine', '>= 1.1.7'),
)

Export('environment')

metv, metvplayer = SConscript('src/SConscript', variant_dir=buildDirectory, duplicate=0)
