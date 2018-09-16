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
INCLUDEPATH += ../common ../../common ../../../openrpt-build-desktop/common ../renderer

TARGET = RPTrender
unix:TARGET = rptrender

OBJECTS_DIR = tmp
MOC_DIR     = tmp
UI_DIR      = tmp

QMAKE_LIBDIR = ../../lib $$QMAKE_LIBDIR
LIBS += -lrenderer -lopenrptcommon -ldmtx -lMetaSQL -lqzint

win32-msvc* {
  PRE_TARGETDEPS += ../../lib/renderer.$${LIBEXT} \
                    ../../lib/openrptcommon.$${LIBEXT}
} else {
  PRE_TARGETDEPS += ../../lib/librenderer.$${LIBEXT} \
                    ../../lib/libopenrptcommon.$${LIBEXT}
}

DESTDIR = ../../bin

RC_FILE = renderapp.rc
macx:RC_FILE = ../images/OpenRPT.icns

# Input
FORMS   += renderwindow.ui \
           ../wrtembed/dbfiledialog.ui

HEADERS += ../wrtembed/dbfiledialog.h \
           renderwindow.h

SOURCES += ../wrtembed/dbfiledialog.cpp \
           renderwindow.cpp \
           main.cpp


RESOURCES += renderapp.qrc

QT += xml sql network widgets printsupport
TRANSLATIONS    = renderapp_fr.ts renderapp_it.ts renderapp_ru.ts renderapp_es.ts renderapp_ar.ts
