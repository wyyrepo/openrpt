#
# OpenRPT report writer and rendering engine
# Copyright (C) 2001-2016 by OpenMFG, LLC
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
# Please contact info@openmfg.com with any questions on this license.
#

include( ../../global.pri )

TEMPLATE = app
CONFIG += qt warn_on
INCLUDEPATH += ../../OpenRPT/common ../../common ../../../openrpt-build-desktop/common .. .

TARGET=importmqlgui

OBJECTS_DIR = tmp
MOC_DIR     = tmp
UI_DIR      = tmp

QMAKE_LIBDIR = ../../lib $$QMAKE_LIBDIR
LIBS += -lMetaSQL -lopenrptcommon

DESTDIR = ../../bin

# Input
FORMS   += importwindow.ui

HEADERS += importwindow.h \
           ../../OpenRPT/common/builtinSqlFunctions.h

SOURCES += importwindow.cpp \
           ../../OpenRPT/common/builtinSqlFunctions.cpp \
           main.cpp

QT += xml sql widgets
