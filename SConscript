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

Import('environment')

desktop = Command('me-tv.desktop', 'me-tv.desktop.in',
    'intltool-merge  -d -u -c ./po/.intltool-merge-cache ./po $SOURCE $TARGET'
)

schemas = Command('me-tv.schemas', 'me-tv.schemas.in',
    'intltool-merge  -s -u -c ./po/.intltool-merge-cache ./po $SOURCE $TARGET'
)

built_things = [desktop, schemas]

SideEffect('po/.intltool-merge-cache', built_things)

Alias('install', [
    Install(environment['DATADIR'] + '/pixmaps/', Glob('*.png')),
    Install(environment['DATADIR'] + '/man/man1/', Glob('me-tv*.1')),
    Install(environment['DATADIR'] + '/applications/', desktop),
    Install(environment['PREFIX'] + '/etc/gconf/schemas/', schemas),
])

Return('built_things')
