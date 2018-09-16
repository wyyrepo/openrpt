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

include( ../global.pri )

TEMPLATE = lib
CONFIG  += qt warn_on
QT      +=  sql xml widgets printsupport
DEFINES += MAKELIB

INCLUDEPATH += ../common ../../openrpt-build-desktop/common
DEPENDPATH += ../common ../../openrpt-build-desktop/common
QMAKE_LIBDIR += ../lib
LIBS         += -lopenrptcommon

DESTDIR     = ../lib
UI_DIR      = tmp
MOC_DIR     = tmp
OBJECTS_DIR = tmp

# override CONFIG warn_on for C files but not C++
QMAKE_CFLAGS_WARN_ON  = -Wno-missing-braces

FORMS   += logoutput.ui \
           metasqlsaveparameters.ui \
           mqledit.ui \
           resultsoutput.ui \
           selectmql.ui \

HEADERS += logoutput.h \
           metasql.h \
           metasqlqueryparser.h \
           metasqlhighlighter.h \
           metasqlsaveparameters.h \
           mqledit.h \
           mqlutil.h \
           regex/regex.h \
           resultsoutput.h \
           selectmql.h \

SOURCES += logoutput.cpp \
           metasql.cpp \
           metasqlqueryparser.cpp \
           metasqlhighlighter.cpp \
           metasqlsaveparameters.cpp \
           mqledit.cpp \
           mqlutil.cpp \
           regex/regex.c \
           resultsoutput.cpp \
           selectmql.cpp \

RESOURCES += ../OpenRPT/images/OpenRPTMetaSQL.qrc \
           metasql.qrc

TRANSLATIONS    = metasql_fr.ts metasql_it.ts metasql_ru.ts metasql_es.ts metasql_ar.ts
